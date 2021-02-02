#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nepi_edge_sdk.h"
#include "nepi_edge_lb_interface.h"
#include "nepi_edge_hb_interface.h"

/* Helper for config and general message params */
void printParam(NEPI_EDGE_LB_Param_Id_Type_t id_type, NEPI_EDGE_LB_Param_Id_t id,
                 NEPI_EDGE_LB_Param_Value_Type_t value_type, NEPI_EDGE_LB_Param_Value_t val)
{
  switch (id_type)
  {
    case NEPI_EDGE_LB_PARAM_ID_TYPE_STRING:
      printf("\t\tID (string): %s\n", id.id_string); break;
    case NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER:
      printf("\t\tID (number): %u\n", id.id_number); break;
    default:
      printf("\t\tID Type Unknown!!!\n"); break;
  }

  switch(value_type)
  {
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL:
      printf("\t\tVal (boolean): %s\n", (val.bool_val == 0)? "false" : "true"); break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64:
      printf("\t\tVal (int64): %ld\n", val.int64_val); break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_UINT64:
      printf("\t\tVal (uint64): %lu\n", val.uint64_val); break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_FLOAT:
      printf("\t\tVal (float): %f\n", val.float_val); break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE:
      printf("\t\tVal (double): %f\n", val.double_val); break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING:
      printf("\t\tVal (string): %s\n", val.string_val); break;
    case NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES:
      printf("\t\tVal (byte array): %lu entries\n\t\t[", val.bytes_val.length);
      for (size_t i = 0; i < val.bytes_val.length; ++i)
      {
        printf("0x%02X%s", *(val.bytes_val.val + i), (i % 8 == 7)? "\n\t\t" : " ");
      }
      printf("]\n"); break;
    default: printf("\t\tValue Type Unknown!!!\n");
  }
}

/* Return codes not checked for brevity; you should check them in your code */
int main(int argc, char **argv)
{
  printf("Testing NEPIEdgeSdk (C-language bindings)\n");
  /* First, set the the NEPI-EDGE filesys interface path */
  NEPI_EDGE_SetBotBaseFilePath("./nepi_sdk_example_filesys");

  /* Now we can report the NUID for this instance */
  const char* nuid = NEPI_EDGE_GetBotNUID();
  printf("Detected NUID: %s\n", nuid);

  /* Now create the status */
  NEPI_EDGE_LB_Status_t status;
  NEPI_EDGE_LBStatusCreate(&status, "2020-09-03 17:14:25.2-04:00"); // Timestamp is required; hence included in Create method
  /* Populate some of the optional fields */
  NEPI_EDGE_LBStatusSetNavSatFixTime(status, "2020-08-21T09:49:00.0-04:00"); // Either RFC3339 time/date separator is allowed.
  NEPI_EDGE_LBStatusSetLatitude(status, 47.6062f); // Seattle
  NEPI_EDGE_LBStatusSetLongitude(status, -122.3321f); // Seattle
  NEPI_EDGE_LBStatusSetHeading(status, NEPI_EDGE_HEADING_REF_MAG_NORTH, 45.0f); // North-east
  NEPI_EDGE_LBStatusSetRollAngle(status, 5.5f);
  NEPI_EDGE_LBStatusSetPitchAngle(status, -17.6f); // Diving
  NEPI_EDGE_LBStatusSetTemperature(status, 40.0f); // Balmy!
  NEPI_EDGE_LBStatusSetPowerState(status, 94.2f); // Mostly charged
  const uint8_t device_status[5] = {1,2,3,4,5}; // Will be copied into allocated memory, so this can be a temporary variable
  NEPI_EDGE_LBStatusSetDeviceStatus(status, device_status, 5);

  /* Next create a couple of data snippets */
  NEPI_EDGE_LB_Data_Snippet_t data_snippets[2];
  NEPI_EDGE_LBDataSnippetCreate(&data_snippets[0], "cls", 0); // Type and instance are required fields; hence in the Create method
  /* And populate some of the optional fields */
  NEPI_EDGE_LBDataSnippetSetDataTimestamp(data_snippets[0], "2020-09-03 17:14:25.6-04:00"); // 169ms after the status timestamp
  NEPI_EDGE_LBDataSnippetSetLatitude(data_snippets[0], 47.607f); // Small motion northward from position at status timestamp
  NEPI_EDGE_LBDataSnippetSetLongitude(data_snippets[0], -122.33f); // Small motion eastward from position at status timestamp
  NEPI_EDGE_LBDataSnippetSetHeading(data_snippets[0], 22.5f); // North-north east
  NEPI_EDGE_LBDataSnippetSetRollAngle(data_snippets[0], 6.5f); // Another degree of roll from attitude at status timestamp
  NEPI_EDGE_LBDataSnippetSetPitchAngle(data_snippets[0], -18.2f); // 0.6 degrees more downward pitch
  NEPI_EDGE_LBDataSnippetSetScores(data_snippets[0], 0.33f, 1.0f, 0.5f);
  const char *simulated_file_path = "./example_files/left_img_detections.txt"; // Will be copied into allocated memory, so this can be a temporary variable
  NEPI_EDGE_LBDataSnippetSetDataFile(data_snippets[0], simulated_file_path, 0); // Last arg means don't delete file after export

  /* Now the second data snippet -- empty except for the required type and instance fields and optional data file field */
  NEPI_EDGE_LBDataSnippetCreate(&data_snippets[1], "cls", 1);
  const char *simulated_file_path_2 = "./example_files/left_img_small.jpeg"; // Will be copied into allocated memory, so this can be a temporary variable
  NEPI_EDGE_LBDataSnippetSetDataFile(data_snippets[1], simulated_file_path_2, 0); // Last arg means don't delete file after export

  /* And export the status+data */
  NEPI_EDGE_LBExportData(status, data_snippets, 2);
  printf("Created status and two associated data snippet files\n");

  /* Now create a general message */
  NEPI_EDGE_LB_General_t general_1;
  NEPI_EDGE_LBGeneralCreate(&general_1);
  /* And populate its sole payload field -- set it up for a string-keyed, floating-point valued payload*/
  const char *general_1_payload_key = "WantSomePI?";
  NEPI_EDGE_LBGeneralSetPayloadStrFloat(general_1, general_1_payload_key, 3.14159f);
  /* And export it to a file for NEPI-BOT to consume */
  NEPI_EDGE_LBExportGeneral(general_1);
  printf("Created a DO General file\n");

  /* Let's do another general message */
  NEPI_EDGE_LB_General_t general_2;
  NEPI_EDGE_LBGeneralCreate(&general_2);
  /* Populate this one with a numerical key and an arbitrary byte array for its value */
  uint8_t byte_array[4] = {0xDE,0xAD,0xBE,0xEF};
  NEPI_EDGE_LBGeneralSetPayloadIntBytes(general_2, 12345, byte_array, 4);
  /* And export it to a file */
  NEPI_EDGE_LBExportGeneral(general_2);
  printf("Created another DO General file\n");

  /* Now we demonstrate import -- First a collection of Config messages */
  NEPI_EDGE_LB_Config_t *config_array;
  size_t config_count;
  NEPI_EDGE_LBImportAllConfig(&config_array, &config_count); // Will find this in the lb/config folder
  printf("Imported %lu Config messages\n", config_count);
  for (size_t i = 0; i < config_count; ++i)
  {
    size_t cfg_param_count;
    NEPI_EDGE_LB_Config_t *config_entry;
    NEPI_EDGE_LBConfigGetArrayEntry(config_array, i, &config_entry);
    NEPI_EDGE_LBConfigGetParamCount(config_entry, &cfg_param_count);
    printf("Config message %lu has %lu parameters\n", i, cfg_param_count);
    for (size_t j = 0; j < cfg_param_count; ++j)
    {
      NEPI_EDGE_LB_Param_Id_Type_t id_type;
      NEPI_EDGE_LB_Param_Id_t id;
      NEPI_EDGE_LB_Param_Value_Type_t value_type;
      NEPI_EDGE_LB_Param_Value_t value;
      NEPI_EDGE_LBConfigGetParam(config_entry, j, &id_type, &id, &value_type, &value);
      printf("\tParam %lu:\n", j);
      printParam(id_type, id, value_type, value);
    }
  }

  /* Now import a collection of General DT messages */
  NEPI_EDGE_LB_General_t *general_dt_array;
  size_t general_dt_count;
  NEPI_EDGE_LBImportAllGeneral(&general_dt_array, &general_dt_count); // Imports all from lb/dt-msg folder
  printf("Imported %lu General-DT messages\n", general_dt_count);
  for (size_t i = 0; i < general_dt_count; ++i)
  {
    NEPI_EDGE_LB_General_t *general_entry;
    NEPI_EDGE_LBGeneralGetArrayEntry(general_dt_array, i, &general_entry);

    NEPI_EDGE_LB_Param_Id_Type_t id_type;
    NEPI_EDGE_LB_Param_Id_t id;
    NEPI_EDGE_LB_Param_Value_Type_t value_type;
    NEPI_EDGE_LB_Param_Value_t value;
    NEPI_EDGE_LBGeneralGetParam(general_entry, &id_type, &id, &value_type, &value);
    printf("\tGeneral-DT Msg %lu Param:\n", i);
    printParam(id_type, id, value_type, value);
  }

  /* Now some HB testing. Setup data export. */
  /* First, create a temporary/testing data folder */
  struct stat st = {0};
  char cwd[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  char test_directory[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  char test_file[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  getcwd(cwd, sizeof(cwd));
  snprintf(test_directory, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/testing_data", cwd);
  snprintf(test_file, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/testfile.txt", test_directory);

  if (-1 == stat(test_directory, &st))
  {
    mkdir(test_directory, 0777);
  }
  FILE *f = fopen(test_file, "w");
  fprintf(f, "This is a nepi_sdk_example_session testfile -- you can delete it and the parent directory");
  fclose(f);
  /* Now link the test directory to NEPI for HB data export */
  NEPI_EDGE_HBLinkDataFolder(test_directory);
  printf("Linked HB Data Offload folder to %s\n", test_directory);

  /* Here is how you actually launch the Bot and check the status afterwards */
  // TODO: Launch bot and wait for it to terminate

  /* Now import the execution status to get information about how it went */
  NEPI_EDGE_Exec_Status_t exec_status;
  NEPI_EDGE_ExecStatusCreate(&exec_status);
  NEPI_EDGE_ImportExecStatus(exec_status);
  printf("Imported the BOT Execution Status\n");

  size_t lb_counts;
  size_t hb_counts;
  NEPI_EDGE_ExecStatusGetCounts(exec_status, &lb_counts, &hb_counts);
  for (size_t i = 0; i < lb_counts; ++i)
  {
    printf("\tLB Connection Status %zu:\n", i);
    char *comms_type;
    NEPI_EDGE_ExecStatusGetLBCommsType(exec_status, i, &comms_type);
    printf("\t\tComms Type: %s\n", comms_type);

    NEPI_EDGE_COMMS_STATUS_t comms_status;
    NEPI_EDGE_ExecStatusGetLBCommsStatus(exec_status, i, &comms_status);
    printf("\t\tComms Status: %d\n", comms_status);

    char *start_time;
    char *stop_time;
    NEPI_EDGE_ExecStatusGetLBCommsTimestamps(exec_status, i, &start_time, &stop_time);
    printf("\t\tStart Time: %s\n", start_time);
    printf("\t\tStop Time: %s\n", stop_time);

    size_t warning_count;
    size_t error_count;
    NEPI_EDGE_ExecStatusGetLBCommsGetWarnErrCount(exec_status, i, &warning_count, &error_count);
    printf("\t\tWarnings:\n");
    for (size_t j = 0; j < warning_count; ++j)
    {
      char *warning_string;
      NEPI_EDGE_ExecStatusGetLBCommsGetWarning(exec_status, i, j, &warning_string);
      printf("\t\t  %zu. %s\n",j, warning_string);
    }

    printf("\t\tErrors:\n");
    for (size_t j = 0; j < error_count; ++j)
    {
      char *error_string;
      NEPI_EDGE_ExecStatusGetLBCommsGetError(exec_status, i, j, &error_string);
      printf("\t\t  %zu. %s\n", j, error_string);
    }

    size_t msgs_sent;
    size_t pkts_sent;
    size_t msgs_rcvd;
    NEPI_EDGE_ExecStatusGetLBCommsStatistics(exec_status, i, &msgs_sent, &pkts_sent, &msgs_rcvd);
    printf("\t\tMessages Sent: %zu\n", msgs_sent);
    printf("\t\tPackets Sent: %zu\n", pkts_sent);
    printf("\t\tMessages Received: %zu\n", msgs_rcvd);
  }

  for (size_t i = 0; i < hb_counts; ++i)
  {
    printf("\tHB Connection Status %zu:\n", i);
    char *comms_type;
    NEPI_EDGE_ExecStatusGetHBCommsType(exec_status, i, &comms_type);
    printf("\t\tComms Type: %s\n", comms_type);

    NEPI_EDGE_COMMS_STATUS_t comms_status;
    NEPI_EDGE_ExecStatusGetHBCommsStatus(exec_status, i, &comms_status);
    printf("\t\tComms Status: %d\n", comms_status);

    char *start_time;
    char *stop_time;
    NEPI_EDGE_ExecStatusGetHBCommsTimestamps(exec_status, i, &start_time, &stop_time);
    printf("\t\tStart Time: %s\n", start_time);
    printf("\t\tStop Time: %s\n", stop_time);

    size_t warning_count;
    size_t error_count;
    NEPI_EDGE_ExecStatusGetHBCommsGetWarnErrCount(exec_status, i, &warning_count, &error_count);
    printf("\t\tWarnings:\n");
    for (size_t j = 0; j < warning_count; ++j)
    {
      char *warning_string;
      NEPI_EDGE_ExecStatusGetHBCommsGetWarning(exec_status, i, j, &warning_string);
      printf("\t\t  %zu. %s\n",j, warning_string);
    }

    printf("\t\tErrors:\n");
    for (size_t j = 0; j < error_count; ++j)
    {
      char *error_string;
      NEPI_EDGE_ExecStatusGetHBCommsGetError(exec_status, i, j, &error_string);
      printf("\t\t  %zu. %s\n", j, error_string);
    }

    NEPI_EDGE_HB_DIRECTION_t direction;
    NEPI_EDGE_ExecStatusGetHBCommsDirection(exec_status, i, &direction);
    printf("\t\tDirection: %d\n", direction);

    size_t datasent_kB;
    size_t datareceived_kB;
    NEPI_EDGE_ExecStatusGetHBCommsStatistics(exec_status, i, &datasent_kB, &datareceived_kB);
    printf("\t\tData Sent: %zukB\n", datasent_kB);
    printf("\t\tData Received: %zukB\n", datareceived_kB);
  }

  /* Always destroy what you create */
  NEPI_EDGE_LBStatusDestroy(status);
  NEPI_EDGE_LBDataSnippetDestroy(data_snippets[0]);
  NEPI_EDGE_LBDataSnippetDestroy(data_snippets[1]);
  NEPI_EDGE_LBGeneralDestroy(general_1);
  NEPI_EDGE_LBGeneralDestroy(general_2);
  NEPI_EDGE_LBConfigDestroyArray(config_array, config_count); // ImportAll does creation under the hood
  NEPI_EDGE_LBGeneralDestroyArray(general_dt_array, general_dt_count); // ImportAll does creation under the hood
  NEPI_EDGE_ExecStatusDestroy(exec_status);

  return 0;
}
