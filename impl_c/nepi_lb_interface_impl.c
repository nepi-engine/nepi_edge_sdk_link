#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <dirent.h>

#include "nepi_edge_lb_interface.h"
#include "nepi_lb_interface_impl.h"
#include "nepi_edge_sdk.h"
#include "nepi_edge_sdk_impl.h"
#include "nepi_edge_errors.h"

#include "frozen/frozen.h"

// In case we want to provide arena allocator, etc. someday, don't call
// malloc() and free() directly
#define NEPI_EDGE_MALLOC(x) malloc((x))
#define NEPI_EDGE_FREE(x) free((x))

#define VALIDATE_OPAQUE_TYPE(x,t,s) \
  if (NULL == (x)) return NEPI_EDGE_RET_UNINIT_OBJ;\
  struct s *p = (struct s*)(x);\
  if (p->opaque_helper.msg_id != t) return NEPI_EDGE_RET_WRONG_OBJ_TYPE;


#define VALIDATE_NUMERICAL_RANGE(x,l,u) \
  if ((x) < (l) || (x) > (u)) return NEPI_EDGE_RET_ARG_OUT_OF_RANGE;

#define ENSURE_FIELD_PRESENT(p,f) \
  if (0 == ((p)->opaque_helper.fields_set & (f))) return NEPI_EDGE_RET_REQUIRED_FIELD_MISSING;

#define CHECK_FIELD_PRESENT(p,f) ((p)->opaque_helper.fields_set & (f))

static long int month_to_days(long int month, long int year)
{
  long int days = 0;
  if (month > 1) days += 31;
  if (month > 2)
  {
    if ((year / 4 == 0) && (year / 100 != 0)) days += 29;
    else days += 28;
  }
  if (month > 3) days += 31;
  if (month > 4) days += 30;
  if (month > 5) days += 31;
  if (month > 6) days += 30;
  if (month > 7) days += 31;
  if (month > 8) days += 31;
  if (month > 9) days += 30;
  if (month > 10) days += 31;
  if (month > 11) days += 30;

  return days;
}

static int64_t subtract_rfc3339_timestamps(const char *tstamp_1, const char *tstamp_2)
{
  // Must copy the strings because strtok modifies them
  char tstamp_1_copy[NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH];
  strncpy(tstamp_1_copy, tstamp_1, NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH);
  char tstamp_2_copy[NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH];
  strncpy(tstamp_2_copy, tstamp_2, NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH);
  char *tmp_tok;

  // Parse the strings: e.g., 2020-08-21 09:50:25.431396857-04:00
  // We'll assume that local offset (last token) is the same
  //const char delimiters[5] = "-T:,";
  const long int year_1 = strtol(strtok(tstamp_1_copy, "-"), NULL, 10);
  tmp_tok = strtok(NULL, "-");
  const long int month_1 = (tmp_tok == NULL)? 0 : strtol(tmp_tok, NULL, 10);
  tmp_tok = strtok(NULL, " T");
  const long int day_1 = (tmp_tok == NULL)? 0 : strtol(tmp_tok, NULL, 10);
  tmp_tok = strtok(NULL, ":");
  const long int hour_1 = (tmp_tok == NULL)? 0 : strtol(tmp_tok, NULL, 10);
  tmp_tok = strtok(NULL, ":");
  const long int min_1 = (tmp_tok == NULL)? 0 : strtol(tmp_tok, NULL, 10);
  tmp_tok = strtok(NULL, "+-Z");
  const double sec_1 = (tmp_tok == NULL)? 0.0 : strtod(tmp_tok, NULL);

  const long int year_2 = strtol(strtok(tstamp_2_copy, "-"), NULL, 10);
  tmp_tok = strtok(NULL, "-");
  const long int month_2 = (tmp_tok == NULL)? 0 : strtol(tmp_tok, NULL, 10);
  tmp_tok = strtok(NULL, " T");
  const long int day_2 = (tmp_tok == NULL)? 0 : strtol(tmp_tok, NULL, 10);
  tmp_tok = strtok(NULL, ":");
  const long int hour_2 = (tmp_tok == NULL)? 0 : strtol(tmp_tok, NULL, 10);
  tmp_tok = strtok(NULL, ":");
  const long int min_2 = (tmp_tok == NULL)? 0 : strtol(tmp_tok, NULL, 10);
  tmp_tok = strtok(NULL, "-+Z");
  const double sec_2 = (tmp_tok == NULL)? 0 : strtod(tmp_tok, NULL);

  const long int days_1 = ((year_1 * 365) + (year_1 / 4) - (year_1 / 100)) +
                           month_to_days(month_1, year_1) + (day_1 - 1);
  const long int days_2 = ((year_2 * 365) + (year_2 / 4) - (year_2 / 100)) +
                           month_to_days(month_2, year_2) + (day_2 - 1);
  const int64_t msecs_1 = (days_1 * 86400000) + (hour_1 * 3600000) + (min_1 * 60000) + ((int64_t)(sec_1 * 1000.0));
  const int64_t msecs_2 = (days_2 * 86400000) + (hour_2 * 3600000) + (min_2 * 60000) + ((int64_t)(sec_2 * 1000.0));

  return msecs_1 - msecs_2;
}

static void writeParamToJsonFile(FILE* open_file, const NEPI_EDGE_LB_Param_t *param)
{
  // First the identifier
  fprintf(open_file, "\t\"identifier\":");
  if (param->id_type == NEPI_EDGE_LB_PARAM_ID_TYPE_STRING)
  {
    fprintf(open_file, "\"%s\",\n", param->id.id_string);
  }
  else
  {
    fprintf(open_file, "%u,\n", param->id.id_number);
  }
  // Then the value
  fprintf(open_file, "\t\"value\":");

  switch(param->value_type)
  {
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL:
      fprintf(open_file, "%s\n", (param->value.bool_val == 0)? "false" : "true");
      break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64:
      fprintf(open_file, "%ld\n", param->value.int64_val);
      break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_UINT64:
      fprintf(open_file, "%lu\n", param->value.uint64_val);
      break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_FLOAT:
      fprintf(open_file, "%f\n", param->value.float_val);
      break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE:
      fprintf(open_file, "%f\n", param->value.double_val);
      break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING:
      fprintf(open_file, "\"%s\"\n", param->value.string_val);
      break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES:
    {
      if (param->value.bytes_val.length > 0)
      {
        const size_t byte_count = param->value.bytes_val.length;
        fprintf(open_file, "[");
        for (size_t i = 0; i < byte_count; ++i)
        {
          fprintf(open_file, "%u%s", param->value.bytes_val.val[i], (i == byte_count - 1)? "]" : ",");
        }
      }
      else
      {
        fprintf(open_file, "[]\n");
      }
    } break;
  }
}


NEPI_EDGE_RET_t NEPI_EDGE_LBStatusCreate(NEPI_EDGE_LB_Status_t *status, const char* timestamp_rfc3339)
{
  *status = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_LB_Status));
  if (NULL == *status) return NEPI_EDGE_RET_MALLOC_ERR;

  struct NEPI_EDGE_LB_Status *p = (struct NEPI_EDGE_LB_Status*)(*status);
  strncpy(p->timestamp_rfc3339, timestamp_rfc3339, NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH);
  p->opaque_helper.msg_id = NEPI_EDGE_LB_MSG_ID_STATUS;
  p->opaque_helper.fields_set = NEPI_EDGE_LB_Status_Fields_Timestamp;

  p->device_status_entries = NULL;
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusDestroy(NEPI_EDGE_LB_Status_t status)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  // Free allocated memory for device status if there was any
  if (NULL != p->device_status_entries)
  {
    NEPI_EDGE_FREE(p->device_status_entries);
  }

  NEPI_EDGE_FREE(status);
  status = NULL;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetNavSatFixTime(NEPI_EDGE_LB_Status_t status, const char* timestamp_rfc3339)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  strncpy(p->navsat_fix_time_rfc3339, timestamp_rfc3339, NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH);
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_NavSatFixTime;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetLatitude(NEPI_EDGE_LB_Status_t status, float latitude_deg)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  VALIDATE_NUMERICAL_RANGE(latitude_deg, -90.0f, 90.0f)
  p->latitude_deg = latitude_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_Latitude;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetLongitude(NEPI_EDGE_LB_Status_t status, float longitude_deg)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  VALIDATE_NUMERICAL_RANGE(longitude_deg, -180.0f, 180.0f)
  p->longitude_deg = longitude_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_Longitude;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetHeading(NEPI_EDGE_LB_Status_t status, NEPI_EDGE_Heading_Ref_t heading_ref, float heading_deg)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  VALIDATE_NUMERICAL_RANGE(heading_deg, -360.0f, 360.0f)
  VALIDATE_NUMERICAL_RANGE(heading_ref, NEPI_EDGE_HEADING_REF_TRUE_NORTH, NEPI_EDGE_HEADING_REF_MAG_NORTH)
  p->heading_ref = heading_ref;
  p->heading_deg = heading_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_HeadingAndRef;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetRollAngle(NEPI_EDGE_LB_Status_t status, float roll_deg)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  VALIDATE_NUMERICAL_RANGE(roll_deg, -360.0f, 360.0f)
  p->roll_angle_deg = roll_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_RollAngle;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetPitchAngle(NEPI_EDGE_LB_Status_t status, float pitch_deg)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  VALIDATE_NUMERICAL_RANGE(pitch_deg, -360.0f, 360.0f)
  p->pitch_angle_deg = pitch_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_PitchAngle;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetTemperature(NEPI_EDGE_LB_Status_t status, float temperature_c)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  p->temperature_c = temperature_c;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_Temperature;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetPowerState(NEPI_EDGE_LB_Status_t status, float power_state_percentage)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  VALIDATE_NUMERICAL_RANGE(power_state_percentage, 0.0f, 100.0f)
  p->power_state_percentage = power_state_percentage;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_PowerState;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetDeviceStatus(NEPI_EDGE_LB_Status_t status, const uint8_t *status_entries, size_t status_entry_count)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)

  if (status_entries != NULL && status_entry_count > 0) NEPI_EDGE_RET_UNINIT_OBJ;

  p->device_status_entries = NEPI_EDGE_MALLOC(status_entry_count);
  memcpy(p->device_status_entries, status_entries, status_entry_count);

  p->device_status_entry_count = status_entry_count;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Status_Fields_DeviceStatus;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetCreate(NEPI_EDGE_LB_Data_Snippet_t *snippet, const char type[NEPI_EDGE_DATA_SNIPPET_TYPE_LENGTH], uint32_t instance)
{
  *snippet = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_LB_Data_Snippet));
  if (NULL == *snippet) return NEPI_EDGE_RET_MALLOC_ERR;

  struct NEPI_EDGE_LB_Data_Snippet *p = (struct NEPI_EDGE_LB_Data_Snippet*)(*snippet);
  memcpy(p->type, type, NEPI_EDGE_DATA_SNIPPET_TYPE_LENGTH);
  p->instance = instance;
  p->opaque_helper.msg_id = NEPI_EDGE_LB_MSG_ID_DATA;
  p->opaque_helper.fields_set = NEPI_EDGE_LB_Data_Snippet_Fields_TypeAndInstance;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetDestroy(NEPI_EDGE_LB_Data_Snippet_t snippet)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  NEPI_EDGE_FREE(snippet);
  snippet = NULL;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetDataTimestamp(NEPI_EDGE_LB_Data_Snippet_t snippet, const char* data_time_rfc3339)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  strncpy(p->data_time_rfc3339, data_time_rfc3339, NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH);
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Data_Snippet_Fields_Data_Time;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetLatitude(NEPI_EDGE_LB_Data_Snippet_t snippet, float latitude_deg)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  VALIDATE_NUMERICAL_RANGE(latitude_deg, -90.0f, 90.0f)
  p->latitude_deg = latitude_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Data_Snippet_Fields_Latitude;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetLongitude(NEPI_EDGE_LB_Data_Snippet_t snippet, float longitude_deg)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  VALIDATE_NUMERICAL_RANGE(longitude_deg, -180.0f, 180.0f)
  p->longitude_deg = longitude_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Data_Snippet_Fields_Longitude;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetHeading(NEPI_EDGE_LB_Data_Snippet_t snippet, float heading_deg)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  VALIDATE_NUMERICAL_RANGE(heading_deg, -360.0f, 360.0f)
  p->heading_deg = heading_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Data_Snippet_Fields_Heading;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetRollAngle(NEPI_EDGE_LB_Data_Snippet_t snippet, float roll_deg)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  VALIDATE_NUMERICAL_RANGE(roll_deg, -360.0f, 360.0f)
  p->roll_angle_deg = roll_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Data_Snippet_Fields_RollAngle;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetPitchAngle(NEPI_EDGE_LB_Data_Snippet_t snippet, float pitch_deg)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  VALIDATE_NUMERICAL_RANGE(pitch_deg, -360.0f, 360.0f)
  p->pitch_angle_deg = pitch_deg;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Data_Snippet_Fields_PitchAngle;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetScores(NEPI_EDGE_LB_Data_Snippet_t snippet, float quality_score, float type_score, float event_score)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  VALIDATE_NUMERICAL_RANGE(quality_score, 0.0f, 1.0f)
  VALIDATE_NUMERICAL_RANGE(type_score, 0.0f, 1.0f)
  VALIDATE_NUMERICAL_RANGE(event_score, 0.0f, 1.0f)
  p->quality_score = quality_score;
  p->type_score = type_score;
  p->event_score = event_score;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Data_Snippet_Fields_Scores;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetDataFile(NEPI_EDGE_LB_Data_Snippet_t snippet, const char *data_file_with_path, uint8_t delete_on_export)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)

  // p->data_file is a pre-allocated array, so no need to allocate memory here
  strncpy(p->data_file, data_file_with_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH);
  p->delete_on_export = delete_on_export;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_Data_Snippet_Fields_DataFile;

  return NEPI_EDGE_RET_OK;
}

static NEPI_EDGE_RET_t export_status(const NEPI_EDGE_LB_Status_t status, const char* data_path)
{
  // Get the status timestamp; we'll need this later
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)
  ENSURE_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_Timestamp)

  // Now create the status file
  char tmp_filename[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(tmp_filename, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", data_path, NEPI_EDGE_LB_STATUS_FILENAME);
  FILE *status_file = fopen(tmp_filename, "w");
  if (NULL == status_file) return NEPI_EDGE_RET_FILE_OPEN_ERR;

  // Write the JSON
  fprintf(status_file, "{\n");
  fprintf(status_file, "\t\"timestamp\":\"%s\"", p->timestamp_rfc3339); // Timestamp is required and verified above
  // All other fields are optional
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_NavSatFixTime))
  {
    const int64_t navsat_delta_ms = subtract_rfc3339_timestamps(p->navsat_fix_time_rfc3339, p->timestamp_rfc3339);
    fprintf(status_file, ",\n\t\"navsat_fix_time_offset\":%ld", navsat_delta_ms);
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_Latitude))
  {
    fprintf(status_file, ",\n\t\"latitude\":%f", p->latitude_deg);
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_Longitude))
  {
    fprintf(status_file, ",\n\t\"longitude\":%f", p->longitude_deg);
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_HeadingAndRef))
  {
    //fprintf(status_file, ",\n\t\"heading\":%f", p->heading_deg);
    fprintf(status_file, ",\n\t\"heading\":%d", (int)(round(1000.0 * p->heading_deg)));
    fprintf(status_file, ",\n\t\"heading_true_north\":%s", (p->heading_ref == NEPI_EDGE_HEADING_REF_TRUE_NORTH)? "true" : "false");
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_RollAngle))
  {
    //fprintf(status_file, ",\n\t\"roll_angle\":%f", p->roll_angle_deg);
    fprintf(status_file, ",\n\t\"roll_angle\":%d", (int)(round(1000.0 * p->roll_angle_deg)));
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_PitchAngle))
  {
    //fprintf(status_file, ",\n\t\"pitch_angle\":%f", p->pitch_angle_deg);
    fprintf(status_file, ",\n\t\"pitch_angle\":%d", (int)(round(1000.0 * p->pitch_angle_deg)));
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_Temperature))
  {
    //fprintf(status_file, ",\n\t\"temperature\":%f", p->temperature_c);
    fprintf(status_file, ",\n\t\"temperature\":%d", (int)(round(10 * p->temperature_c)));
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_PowerState))
  {
    fprintf(status_file, ",\n\t\"power_state\":%u", p->power_state_percentage);
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_DeviceStatus))
  {
    fprintf(status_file, ",\n\t\"device_status\":[");
    for (size_t i = 0; i < p->device_status_entry_count; ++i)
    {
      fprintf(status_file, "%u%s", p->device_status_entries[i], (i == p->device_status_entry_count - 1)? "]" : ",");
    }
  }
  fprintf(status_file, "\n}");

  fclose(status_file);
  return NEPI_EDGE_RET_OK;
}

static int copy_file(const char *src_filename, const char *destination_filename)
{
  FILE *src = fopen(src_filename, "r");
  FILE *dest = fopen(destination_filename, "w");

  if (src == NULL || dest == NULL)
  {
    return -1;
  }

  char cpy_buf[BUFSIZ];
  size_t block_byte_count = 0;
  while ((block_byte_count = fread(cpy_buf, sizeof(char), sizeof(cpy_buf), src)) > 0)
  {
    if (fwrite(cpy_buf, sizeof(char), block_byte_count, dest) != block_byte_count)
    {
      return -1;
    }
  }

  fclose(src);
  fclose(dest);
}

static NEPI_EDGE_RET_t export_data_snippet(NEPI_EDGE_LB_Data_Snippet_t snippet, const char* data_path, const struct NEPI_EDGE_LB_Status* status)
{
  VALIDATE_OPAQUE_TYPE(snippet, NEPI_EDGE_LB_MSG_ID_DATA, NEPI_EDGE_LB_Data_Snippet)
  ENSURE_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_TypeAndInstance)

  // Move the snippet data file if one exists
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_DataFile))
  {
    // Get the new filename by finding the last path separator character in the old file
    char *data_filename_ptr = strrchr(p->data_file, '/');
    char data_filename[NEPI_EDGE_MAX_FILE_PATH_LENGTH]; // Must create a copy to avoid overlapping strcpy later
    if (data_filename_ptr == NULL) // has no path characters
    {
      strncpy(data_filename, p->data_file, NEPI_EDGE_MAX_FILE_PATH_LENGTH);
    }
    else
    {
      strncpy(data_filename, (data_filename_ptr + 1), NEPI_EDGE_MAX_FILE_PATH_LENGTH);
    }
    char new_filename_with_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
    snprintf(new_filename_with_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", data_path, data_filename);

    // Copy or move it, depending on what was specified when the data file was added
    if (p->delete_on_export)
    {
      if (-1 == rename(p->data_file, new_filename_with_path))
      {
        return NEPI_EDGE_RET_FILE_MOVE_ERROR;
      }
    }
    else
    {
      if (-1 == copy_file(p->data_file, new_filename_with_path))
      {
        return NEPI_EDGE_RET_FILE_MOVE_ERROR;
      }
    }

    // Update the filename in the data structure
    strncpy(p->data_file, data_filename, NEPI_EDGE_MAX_FILE_PATH_LENGTH);
  }

  char tmp_filename[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(tmp_filename, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%c%c%c%u.json", data_path, p->type[0], p->type[1], p->type[2], p->instance);
  FILE *snippet_file = fopen(tmp_filename, "w");
  if (NULL == snippet_file) return NEPI_EDGE_RET_FILE_OPEN_ERR;

  // Now write the JSON
  fprintf(snippet_file, "{\n");
  fprintf(snippet_file, "\t\"type\":\"%c%c%c\"", p->type[0], p->type[1], p->type[2]);
  fprintf(snippet_file, ",\n\t\"instance\":%u", p->instance);

  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_Data_Time))
  {
    int64_t data_time_delta_ms = subtract_rfc3339_timestamps(p->data_time_rfc3339, status->timestamp_rfc3339);
    fprintf(snippet_file, ",\n\t\"data_time_offset\":%ld", data_time_delta_ms);
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_Latitude))
  {
    fprintf(snippet_file, ",\n\t\"latitude_offset\":%f", p->latitude_deg - status->latitude_deg);
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_Longitude))
  {
    fprintf(snippet_file, ",\n\t\"longitude_offset\":%f", p->longitude_deg - status->longitude_deg);
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_Heading))
  {
    //fprintf(snippet_file, ",\n\t\"heading_offset\":%f", p->heading_deg - status->heading_deg);
    fprintf(snippet_file, ",\n\t\"heading_offset\":%d", (int)(round(1000.0f * (p->heading_deg - status->heading_deg))));
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_RollAngle))
  {
    //fprintf(snippet_file, ",\n\t\"roll_offset\":%f", p->roll_angle_deg - status->roll_angle_deg);
    fprintf(snippet_file, ",\n\t\"roll_offset\":%d", (int)(round(1000.0f * (p->roll_angle_deg - status->roll_angle_deg))));
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_PitchAngle))
  {
    //fprintf(snippet_file, ",\n\t\"pitch_offset\":%f", p->pitch_angle_deg - status->pitch_angle_deg);
    fprintf(snippet_file, ",\n\t\"pitch_offset\":%d", (int)(round(1000.0f * (p->pitch_angle_deg - status->pitch_angle_deg))));
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_Scores))
  {
    fprintf(snippet_file, ",\n\t\"quality_score\":%f", p->quality_score);
    fprintf(snippet_file, ",\n\t\"type_score\":%f", p->type_score);
    fprintf(snippet_file, ",\n\t\"event_score\":%f", p->event_score);
  }
  if (CHECK_FIELD_PRESENT(p, NEPI_EDGE_LB_Data_Snippet_Fields_DataFile))
  {
    fprintf(snippet_file, ",\n\t\"data_file\":\"%s\"", p->data_file);
  }

  fprintf(snippet_file, "\n}");

  fclose(snippet_file);
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBExportData(const NEPI_EDGE_LB_Status_t status, const NEPI_EDGE_LB_Data_Snippet_t *snippets, size_t snippet_count)
{
  VALIDATE_OPAQUE_TYPE(status, NEPI_EDGE_LB_MSG_ID_STATUS, NEPI_EDGE_LB_Status)
  ENSURE_FIELD_PRESENT(p, NEPI_EDGE_LB_Status_Fields_Timestamp)

  // Ensure the data path exists
  char data_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(data_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s/%s",
           NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_LB_DATA_FOLDER_PATH,
           p->timestamp_rfc3339);

  NEPI_EDGE_RET_t ret = NEPI_EDGE_SDKCheckPath(data_path);
  if (NEPI_EDGE_RET_OK != ret) return ret;

  // Export the status
  ret = export_status(status, data_path);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // Now export each of the data snippets
  for (size_t i = 0; i < snippet_count; ++i)
  {
    ret = export_data_snippet(snippets[i], data_path, p);
    if (ret != NEPI_EDGE_RET_OK) return ret;
  }

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBConfigCreate(NEPI_EDGE_LB_Config_t *config)
{
  *config = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_LB_Config));
  if (NULL == *config) return NEPI_EDGE_RET_MALLOC_ERR;

  struct NEPI_EDGE_LB_Config *p = (struct NEPI_EDGE_LB_Config*)(*config);

  p->opaque_helper.msg_id = NEPI_EDGE_LB_MSG_ID_CONFIG;
  p->opaque_helper.fields_set = 0;
  p->params = NULL;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBConfigDestroy(NEPI_EDGE_LB_Config_t config)
{
  VALIDATE_OPAQUE_TYPE(config, NEPI_EDGE_LB_MSG_ID_CONFIG, NEPI_EDGE_LB_Config)

  NEPI_EDGE_LB_Param_t *param = p->params;
  NEPI_EDGE_LB_Param_t *tmp;
  while (param != NULL)
  {
    tmp = param;
    if (param->id_type == NEPI_EDGE_LB_PARAM_ID_TYPE_STRING)
    {
      NEPI_EDGE_FREE(param->id.id_string);
    }
    if (param->value_type == NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING)
    {
      NEPI_EDGE_FREE(param->value.string_val);
    }
    else if (param->value_type == NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES)
    {
      NEPI_EDGE_FREE(param->value.bytes_val.val);
    }
    param = param->next;
    NEPI_EDGE_FREE(tmp); // Make sure to free the param, itself
  }

  NEPI_EDGE_FREE(config);
  config = NULL;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBConfigDestroyArray(NEPI_EDGE_LB_Config_t *config_array, size_t count)
{
  // First, free any allocated sub-fields
  for (size_t i = 0; i < count; ++i)
  {
    NEPI_EDGE_LB_Config_t config_entry = (struct NEPI_EDGE_LB_Config*)config_array + i;
    VALIDATE_OPAQUE_TYPE(config_entry, NEPI_EDGE_LB_MSG_ID_CONFIG, NEPI_EDGE_LB_Config)

    NEPI_EDGE_LB_Param_t *param = p->params;
    NEPI_EDGE_LB_Param_t *tmp;
    while (param != NULL)
    {
      tmp = param;
      if (param->id_type == NEPI_EDGE_LB_PARAM_ID_TYPE_STRING)
      {
        NEPI_EDGE_FREE(param->id.id_string);
      }
      if (param->value_type == NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING)
      {
        NEPI_EDGE_FREE(param->value.string_val);
      }
      else if (param->value_type == NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES)
      {
        NEPI_EDGE_FREE(param->value.bytes_val.val);
      }
      param = param->next;
      NEPI_EDGE_FREE(tmp); // Make sure to free the param, itself
    }
  }

  // Now free the array block
  NEPI_EDGE_FREE(config_array);
  config_array = NULL;
  return NEPI_EDGE_RET_OK;
}

static void parse_param_identifier(const struct json_token* token, NEPI_EDGE_LB_Param_t *param)
{
  if (token->type == JSON_TYPE_STRING)
  {
    param->id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_STRING;
    param->id.id_string = NEPI_EDGE_MALLOC(token->len + 1); // Freed in the Destroy method
    memcpy(param->id.id_string, token->ptr, token->len);
    param->id.id_string[token->len] = '\0';
  }
  else if (token->type == JSON_TYPE_NUMBER)
  {
    param->id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER;
    param->id.id_number = strtol(token->ptr, NULL, 10);
  }
}

static void parse_param_value(const struct json_token* token, NEPI_EDGE_LB_Param_t *param)
{
  static uint8_t in_byte_array = 0; // This method is stateful

  if (0 != in_byte_array)
  {
    if (token->type == JSON_TYPE_NUMBER)
    {
      // Check if we need to allocate more memory
      if (0 == (param->value.bytes_val.length % NEPI_EDGE_BYTE_ARRAY_BLOCK_SIZE))
      {
        // Free the existing memory
        if (param->value.bytes_val.length > 0)
        {
          NEPI_EDGE_FREE(param->value.bytes_val.val);
        }
        // Allocate a larger block of memory
        param->value.bytes_val.val = NEPI_EDGE_MALLOC(param->value.bytes_val.length + NEPI_EDGE_BYTE_ARRAY_BLOCK_SIZE);
      }

      param->value.bytes_val.val[param->value.bytes_val.length] = strtol(token->ptr, NULL, 10);
      ++(param->value.bytes_val.length);
    }
    else if (token->type == JSON_TYPE_ARRAY_END)
    {
      in_byte_array = 0;
    }
    return;
  }

  // Now check the type and proceed accordingly
  if (token->type == JSON_TYPE_STRING)
  {
    param->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING;
    param->value.string_val = NEPI_EDGE_MALLOC(token->len + 1); // Freed in the Destroy method
    memcpy(param->value.string_val, token->ptr, token->len);
    param->value.string_val[token->len] = '\0';
  }
  else if (token->type == JSON_TYPE_ARRAY_START)
  {
    param->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES;
    //param->value.bytes_val.val = NEPI_EDGE_MALLOC(NEPI_EDGE_BYTE_ARRAY_BLOCK_SIZE);
    param->value.bytes_val.length = 0;
    in_byte_array = 1; // Method state variable
  }
  else if (token->type == JSON_TYPE_NUMBER)
  {
    // If there is a decimal point, we'll make it a double, otherwise an int64_t
    uint8_t has_decimal_point = 0;
    for (int i = 0; i < token->len; ++i)
    {
      if (*(token->ptr + i) == '.')
      {
        has_decimal_point = 1;
        break;
      }
    }
    if (1 == has_decimal_point)
    {
      param->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE;
      param->value.double_val = strtod(token->ptr, NULL);
    }
    else
    {
      param->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64;
      param->value.int64_val = strtol(token->ptr, NULL, 10);
    }
  }
  else if (token->type == JSON_TYPE_TRUE)
  {
    param->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL;
    param->value.bool_val = 1;
  }
  else if (token->type == JSON_TYPE_FALSE)
  {
    param->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL;
    param->value.bool_val = 0;
  }
}

static void json_walk_config_callback(void *callback_data, const char *name, size_t name_len, const char *path, const struct json_token *token)
{
  if (token->type == JSON_TYPE_OBJECT_START || token->type == JSON_TYPE_OBJECT_END) return;

  struct NEPI_EDGE_LB_Config *p = (struct NEPI_EDGE_LB_Config*)callback_data;

  char tmp_name[1024];
  size_t tmp_len = (name_len < 1024)? name_len : 1024;
  strncpy(tmp_name, name, tmp_len);
  tmp_name[tmp_len] = '\0';

  if (0 == strcmp(tmp_name, "params")) return; // The start of the params array, nothing to do

  // Find the last valid entry in the param linked list
  NEPI_EDGE_LB_Param_t *param = p->params;
  if (param != NULL)
  {
    while (param->next != NULL)
    {
      param = param->next;
    }
  }
  else // First entry, so allocate and initialize
  {
    p->params = NEPI_EDGE_MALLOC(sizeof(NEPI_EDGE_LB_Param_t));
    p->params->next = NULL;
    p->params->id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_UNKNOWN;
    p->params->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_UNKNOWN;
    param = p->params;
    p->opaque_helper.fields_set |= NEPI_EDGE_LB_Config_Fields_Params;
  }

  if (0 == strcmp(tmp_name, "identifier"))
  {
    // Check if this is a new param entry and if so, close out the last one and create the new one
    if (param->id_type != NEPI_EDGE_LB_PARAM_ID_TYPE_UNKNOWN)
    {
      param->next = NEPI_EDGE_MALLOC(sizeof(NEPI_EDGE_LB_Param_t));
      param = param->next;
      param->next = NULL;
      param->id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_UNKNOWN;
      param->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_UNKNOWN;
    }

    parse_param_identifier(token, param);
    p->opaque_helper.fields_set |= NEPI_EDGE_LB_Config_Fields_Params; // No harm in setting this every time
  }
  else if (0 == strcmp(tmp_name, "value"))
  {
    // Check if this is a new param entry and if so, close out the last one and create the new one
    if (param->value_type != NEPI_EDGE_LB_PARAM_VALUE_TYPE_UNKNOWN)
    {
      param->next = NEPI_EDGE_MALLOC(sizeof(NEPI_EDGE_LB_Param_t));
      param = param->next;
      param->next = NULL;
      param->id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_UNKNOWN;
      param->value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_UNKNOWN;
    }

    parse_param_value(token, param);
    p->opaque_helper.fields_set |= NEPI_EDGE_LB_Config_Fields_Params; // No harm in setting this every time
  }
  else if ((token->type == JSON_TYPE_NUMBER) && (path[strlen(path) - 1] == ']'))
  {
    // Might be inside a byte-array, in that case this will be a JSON_NUMBER and the last path character will be a closing bracket
    parse_param_value(token, param);
  }
  else if (token->type == JSON_TYPE_ARRAY_END) // Must catch this one to update the state in parse_param_value
  {
    parse_param_value(token, param);
  }
}

NEPI_EDGE_RET_t NEPI_EDGE_LBImportConfig(NEPI_EDGE_LB_Config_t config, const char* filename)
{
  VALIDATE_OPAQUE_TYPE(config, NEPI_EDGE_LB_MSG_ID_CONFIG, NEPI_EDGE_LB_Config)

  // First read the file into a string
  char filename_with_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(filename_with_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s/%s",
           NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_LB_CONFIG_FOLDER_PATH, filename);
  char *json_string = json_fread(filename_with_path);
  if (NULL == json_string) return NEPI_EDGE_RET_INVALID_FILE_FORMAT;
  //printf("%s\n", json_string); // Debugging
  json_walk(json_string, strlen(json_string), json_walk_config_callback, p);
  free(json_string); // Must free the frozen-malloc'd string

  return NEPI_EDGE_RET_OK;
}

static NEPI_EDGE_RET_t NEPI_EDGE_LBConfigCreateArray(struct NEPI_EDGE_LB_Config **config_array, size_t config_count)
{
  *config_array = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_LB_Config) * config_count);
  if (NULL == *config_array) return NEPI_EDGE_RET_MALLOC_ERR;

  for (size_t i = 0; i < config_count; ++i)
  {
    struct NEPI_EDGE_LB_Config *p = *config_array + i;

    p->opaque_helper.msg_id = NEPI_EDGE_LB_MSG_ID_CONFIG;
    p->opaque_helper.fields_set = 0;
    p->params = NULL;
  }

  return NEPI_EDGE_RET_OK;
}

static NEPI_EDGE_RET_t countJsonFilesInFolder(const char *path, size_t *count)
{
  DIR *dir = opendir(path);
  if (dir == NULL) return NEPI_EDGE_RET_FILE_OPEN_ERR;

  struct dirent *de;
  *count = 0;
  while (de = readdir(dir))
  {
    // Check that it is a JSON file and if so, count it
    const char *dot = strrchr(de->d_name, '.');
    if ((dot != NULL) && (0 == strncmp(dot, ".json", 5)))
    {
      ++(*count);
    }
  }
  closedir(dir);
}

NEPI_EDGE_RET_t NEPI_EDGE_LBImportAllConfig(NEPI_EDGE_LB_General_t **config_array, size_t *config_count)
{
  // Get the path to the Config DT folder
  char path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_LB_CONFIG_FOLDER_PATH);

  // Get the number of JSON files so that we can do the array allocation
  size_t config_file_count;
  NEPI_EDGE_RET_t ret = countJsonFilesInFolder(path, &config_file_count);
  if (NEPI_EDGE_RET_OK != ret) return ret;

  ret = NEPI_EDGE_LBConfigCreateArray((struct NEPI_EDGE_LB_Config**)config_array, config_file_count);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // Update the output count at this point so that if a failure occurs below this, the caller still knows
  // how much space to deallocate
  *config_count = config_file_count;

  // Now open the directory stream, processing each JSON file in turn
  DIR *dir = opendir(path);
  if (dir == NULL) return NEPI_EDGE_RET_FILE_OPEN_ERR;

  struct dirent *de;
  size_t config_index = 0;
  while (de = readdir(dir))
  {
    // Check that it is a JSON file and if so, process it
    const char *dot = strrchr(de->d_name, '.');
    if ((dot != NULL) && (0 == strncmp(dot, ".json", 5)))
    {
      NEPI_EDGE_LB_Config_t config_entry = (struct NEPI_EDGE_LB_Config*)(*config_array) + config_index;
      ret = NEPI_EDGE_LBImportConfig(config_entry, de->d_name);
      if (ret != NEPI_EDGE_RET_OK) return ret;

      ++config_index;
    }
  }
  closedir(dir);

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBConfigGetArrayEntry(NEPI_EDGE_LB_Config_t *config_array, size_t index, NEPI_EDGE_LB_Config_t **config_entry)
{
  *config_entry = (NEPI_EDGE_LB_Config_t*)((struct NEPI_EDGE_LB_Config*)(config_array) + index);
  VALIDATE_OPAQUE_TYPE(config_entry, NEPI_EDGE_LB_MSG_ID_CONFIG, NEPI_EDGE_LB_Config)

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBConfigGetParamCount(NEPI_EDGE_LB_Config_t *config, size_t *item_count)
{
  VALIDATE_OPAQUE_TYPE(config, NEPI_EDGE_LB_MSG_ID_CONFIG, NEPI_EDGE_LB_Config)

  *item_count = 0;
  NEPI_EDGE_LB_Param_t *param = p->params;
  while(param != NULL)
  {
    ++(*item_count);
    param = param->next;
  }

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBConfigGetParam(NEPI_EDGE_LB_Config_t config, size_t item_index,
                                           NEPI_EDGE_LB_Param_Id_Type_t *id_type, NEPI_EDGE_LB_Param_Id_t *id,
                                           NEPI_EDGE_LB_Param_Value_Type_t *value_type, NEPI_EDGE_LB_Param_Value_t *value)
{

  VALIDATE_OPAQUE_TYPE(config, NEPI_EDGE_LB_MSG_ID_CONFIG, NEPI_EDGE_LB_Config)

  // First, navigate through linked list to the correct param (item)
  NEPI_EDGE_LB_Param_t *param = p->params;
  size_t count = 0;
  while(param != NULL)
  {
    if (count == item_index) break;
    param = param->next;
    ++count;
  }
  if (count != item_index) return NEPI_EDGE_RET_ARG_OUT_OF_RANGE;

  *id_type = param->id_type;
  *id = param->id;
  *value_type = param->value_type;
  *value = param->value;
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralCreate(NEPI_EDGE_LB_General_t *general)
{
  *general = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_LB_General));
  if (NULL == *general) return NEPI_EDGE_RET_MALLOC_ERR;

  struct NEPI_EDGE_LB_General *p = (struct NEPI_EDGE_LB_General*)(*general);
  p->opaque_helper.msg_id = NEPI_EDGE_LB_MSG_ID_GENERAL;
  p->opaque_helper.fields_set = 0;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralDestroy(NEPI_EDGE_LB_General_t general)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  // Depending on identifier and value type, might need to free some internal pointers that
  // are malloc'd when the fields are populated
  if (p->param.id_type == NEPI_EDGE_LB_PARAM_ID_TYPE_STRING)
  {
    NEPI_EDGE_FREE(p->param.id.id_string);
  }
  if (p->param.value_type == NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING ||
      p->param.value_type == NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES)
  {
    NEPI_EDGE_FREE(p->param.value.string_val);
  }

  NEPI_EDGE_FREE(general);
  general = NULL;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralDestroyArray(NEPI_EDGE_LB_General_t *general_array, size_t count)
{
  // First, free any allocated sub-fields
  for (size_t i = 0; i < count; ++i)
  {
    NEPI_EDGE_LB_General_t general_entry = (struct NEPI_EDGE_LB_General*)general_array + i;
    VALIDATE_OPAQUE_TYPE(general_entry, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

    if (p->param.id_type == NEPI_EDGE_LB_PARAM_ID_TYPE_STRING)
    {
      NEPI_EDGE_FREE(p->param.id.id_string);
    }
    if (p->param.value_type == NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING ||
        p->param.value_type == NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES)
    {
      NEPI_EDGE_FREE(p->param.value.string_val);
    }
  }

  // Now free the type_array
  NEPI_EDGE_FREE(general_array);
  general_array = NULL;
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrBool(NEPI_EDGE_LB_General_t general, const char *id, uint8_t val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_STRING;
  p->param.id.id_string = NEPI_EDGE_MALLOC(strlen(id) + 1);
  strcpy(p->param.id.id_string,id);
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL;
  p->param.value.bool_val = (val == 0)? 0 : 1;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrInt64(NEPI_EDGE_LB_General_t general, const char *id, int64_t val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_STRING;
  p->param.id.id_string = NEPI_EDGE_MALLOC(strlen(id) + 1);
  strcpy(p->param.id.id_string,id);
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64;
  p->param.value.int64_val = val;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrUInt64(NEPI_EDGE_LB_General_t general, const char *id, uint64_t val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_STRING;
  p->param.id.id_string = NEPI_EDGE_MALLOC(strlen(id) + 1);
  strcpy(p->param.id.id_string,id);
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_UINT64;
  p->param.value.uint64_val = val;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrFloat(NEPI_EDGE_LB_General_t general, const char *id, float val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_STRING;
  p->param.id.id_string = NEPI_EDGE_MALLOC(strlen(id) + 1);
  strcpy(p->param.id.id_string,id);
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_FLOAT;
  p->param.value.float_val = val;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrDouble(NEPI_EDGE_LB_General_t general, const char *id, double val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_STRING;
  p->param.id.id_string = NEPI_EDGE_MALLOC(strlen(id) + 1);
  strcpy(p->param.id.id_string,id);
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE;
  p->param.value.double_val = val;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrStr(NEPI_EDGE_LB_General_t general, const char *id, const char *val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_STRING;
  p->param.id.id_string = NEPI_EDGE_MALLOC(strlen(id) + 1);
  strcpy(p->param.id.id_string,id);
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING;
  p->param.value.string_val = NEPI_EDGE_MALLOC(strlen(val) + 1);
  strcpy(p->param.value.string_val, val);
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrBytes(NEPI_EDGE_LB_General_t general, const char *id, const uint8_t *val, const size_t length)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_STRING;
  p->param.id.id_string = NEPI_EDGE_MALLOC(strlen(id) + 1);
  strcpy(p->param.id.id_string,id);
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES;
  p->param.value.bytes_val.val = NEPI_EDGE_MALLOC(length);
  memcpy(p->param.value.bytes_val.val, val, length);
  p->param.value.bytes_val.length = length;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntBool(NEPI_EDGE_LB_General_t general, uint32_t id, uint8_t val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER;
  p->param.id.id_number = id;
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL;
  p->param.value.bool_val = (val == 0)? 0 : 1;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntInt64(NEPI_EDGE_LB_General_t general, uint32_t id, int64_t val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER;
  p->param.id.id_number = id;
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64;
  p->param.value.int64_val = val;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntUInt64(NEPI_EDGE_LB_General_t general, uint32_t id, uint64_t val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER;
  p->param.id.id_number = id;
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_UINT64;
  p->param.value.uint64_val = val;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntFloat(NEPI_EDGE_LB_General_t general, uint32_t id, float val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER;
  p->param.id.id_number = id;
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_FLOAT;
  p->param.value.float_val = val;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntDouble(NEPI_EDGE_LB_General_t general, uint32_t id, double val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER;
  p->param.id.id_number = id;
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE;
  p->param.value.double_val = val;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntStr(NEPI_EDGE_LB_General_t general, uint32_t id, const char *val)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER;
  p->param.id.id_number = id;
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING;
  p->param.value.string_val = NEPI_EDGE_MALLOC(strlen(val) + 1);
  strcpy(p->param.value.string_val, val);
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntBytes(NEPI_EDGE_LB_General_t general, uint32_t id, const uint8_t *val, const size_t length)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  p->param.id_type = NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER;
  p->param.id.id_number = id;
  p->param.value_type = NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES;
  p->param.value.bytes_val.val = NEPI_EDGE_MALLOC(length);
  memcpy(p->param.value.bytes_val.val, val, length);
  p->param.value.bytes_val.length = length;
  p->opaque_helper.fields_set |= NEPI_EDGE_LB_General_Fields_Payload;

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBExportGeneral(NEPI_EDGE_LB_General_t general)
{
  static uint32_t general_do_file_count = 0;

  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  char path_qualified_filename[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(path_qualified_filename, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s/general_do_%u.json",
           NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_LB_GENERAL_DO_FOLDER_PATH, general_do_file_count);

  FILE *general_do_file = fopen(path_qualified_filename, "w");
  if (NULL == general_do_file) return NEPI_EDGE_RET_FILE_OPEN_ERR;

  ENSURE_FIELD_PRESENT(p, NEPI_EDGE_LB_General_Fields_Payload)

  fprintf(general_do_file, "{\n");
  writeParamToJsonFile(general_do_file, &(p->param));
  fprintf(general_do_file, "\n}");

  fclose(general_do_file);

  ++general_do_file_count; // Always increment to ensure files have unique names
  return NEPI_EDGE_RET_OK;
}

static void json_walk_general_callback(void *callback_data, const char *name, size_t name_len, const char *path, const struct json_token *token)
{
  if (token->type == JSON_TYPE_OBJECT_START || token->type == JSON_TYPE_OBJECT_END) return;

  struct NEPI_EDGE_LB_General *p = (struct NEPI_EDGE_LB_General*)callback_data;

  char tmp_name[1024];
  size_t tmp_len = (name_len < 1024)? name_len : 1024;
  strncpy(tmp_name, name, tmp_len);
  tmp_name[tmp_len] = '\0';

  if (0 == strcmp(tmp_name, "identifier"))
  {
    parse_param_identifier(token, &(p->param));
    p->opaque_helper.fields_set |= NEPI_EDGE_LB_Config_Fields_Params; // No harm in setting this every time
  }
  else if (0 == strcmp(tmp_name, "value"))
  {
    parse_param_value(token, &(p->param));
    p->opaque_helper.fields_set |= NEPI_EDGE_LB_Config_Fields_Params; // No harm in setting this every time
  }
  else if ((token->type == JSON_TYPE_NUMBER) && (path[strlen(path) - 1] == ']')) // Inside a byte array
  {
    parse_param_value(token, &(p->param));
  }
  else if (token->type == JSON_TYPE_ARRAY_END) // Must catch this one to update the state in parse_param_value
  {
    parse_param_value(token, &(p->param));
  }
}

NEPI_EDGE_RET_t NEPI_EDGE_LBImportGeneral(NEPI_EDGE_LB_General_t general, const char* filename)
{
  VALIDATE_OPAQUE_TYPE(general, NEPI_EDGE_LB_MSG_ID_GENERAL, NEPI_EDGE_LB_General)

  // First read the file into a string
  char filename_with_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(filename_with_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s/%s",
           NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_LB_GENERAL_DT_FOLDER_PATH, filename);
  char *json_string = json_fread(filename_with_path);
  if (NULL == json_string) return NEPI_EDGE_RET_INVALID_FILE_FORMAT;
  //printf("%s\n", json_string); // Debugging
  json_walk(json_string, strlen(json_string), json_walk_general_callback, p);
  free(json_string); // Must free the frozen-malloc'd string

  return NEPI_EDGE_RET_OK;
}

static NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralCreateArray(struct NEPI_EDGE_LB_General **general_array, size_t general_count)
{
  *general_array = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_LB_General) * general_count);
  if (NULL == *general_array) return NEPI_EDGE_RET_MALLOC_ERR;

  for (size_t i = 0; i < general_count; ++i)
  {
    struct NEPI_EDGE_LB_General *p = *general_array + i;
    p->opaque_helper.msg_id = NEPI_EDGE_LB_MSG_ID_GENERAL;
    p->opaque_helper.fields_set = 0;
  }

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_LBImportAllGeneral(NEPI_EDGE_LB_General_t **general_array, size_t *general_count)
{
  // Get the path to the General DT folder
  char path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_LB_GENERAL_DT_FOLDER_PATH);

  // Now walk that directory to count the number of files so that we can do the array allocation
  size_t general_file_count;
  NEPI_EDGE_RET_t ret = countJsonFilesInFolder(path, &general_file_count);
  if (NEPI_EDGE_RET_OK != ret) return ret;

  ret = NEPI_EDGE_LBGeneralCreateArray((struct NEPI_EDGE_LB_General**)general_array, general_file_count);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // Update the output count at this point so that if a failure occurs below this, the caller still knows
  // how much space to deallocate
  *general_count = general_file_count;

  // Now open the directory stream, processing each JSON file in turn
  DIR *dir = opendir(path);
  if (dir == NULL) return NEPI_EDGE_RET_FILE_OPEN_ERR;

  struct dirent *de;
  size_t general_index = 0;
  while (de = readdir(dir))
  {
    // Check that it is a JSON file and if so, process it
    const char *dot = strrchr(de->d_name, '.');
    if ((dot != NULL) && (0 == strncmp(dot, ".json", 5)))
    {
      NEPI_EDGE_LB_General_t general_entry = (struct NEPI_EDGE_LB_General*)(*general_array) + general_index;
      ret = NEPI_EDGE_LBImportGeneral(general_entry, de->d_name);
      if (ret != NEPI_EDGE_RET_OK)
      {
        return ret;
      }
      ++general_index;
    }
  }
  closedir(dir);

  return NEPI_EDGE_RET_OK;
}
