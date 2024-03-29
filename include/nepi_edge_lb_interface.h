/*
 * Copyright (c) 2024 Numurus, LLC <https://www.numurus.com>.
 *
 * This file is part of nepi-engine
 * (see https://github.com/nepi-engine).
 *
 * License: 3-clause BSD, see https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __NEPI_EDGE_LB_INTERFACE_H
#define __NEPI_EDGE_LB_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

#include "nepi_edge_lb_consts.h"
#include "nepi_edge_errors.h"

/* **************** Config/General Retreival Types and Support API **************** */
typedef enum NEPI_EDGE_LB_Param_Id_Type
{
  NEPI_EDGE_LB_PARAM_ID_TYPE_STRING,
  NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER,
  NEPI_EDGE_LB_PARAM_ID_TYPE_UNKNOWN
} NEPI_EDGE_LB_Param_Id_Type_t;

typedef union NEPI_EDGE_LB_Param_Id
{
  char *id_string;
  uint32_t id_number;
} NEPI_EDGE_LB_Param_Id_t;

typedef enum NEPI_EDGE_LB_Param_Value_Type
{
  NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL,
  NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64,
  NEPI_EDGE_LB_PARAM_VALUE_TYPE_UINT64,
  NEPI_EDGE_LB_PARAM_VALUE_TYPE_FLOAT,
  NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE,
  NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING,
  NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES,
  NEPI_EDGE_LB_PARAM_VALUE_TYPE_UNKNOWN
} NEPI_EDGE_LB_Param_Value_Type_t;

typedef struct NEPI_EDGE_LB_Param_Bytes
{
  uint8_t *val;
  size_t length;
} NEPI_EDGE_LB_Param_Bytes_t;

typedef union NEPI_EDGE_LB_Param_Value
{
  uint8_t bool_val;
  int64_t int64_val;
  uint64_t uint64_val;
  float float_val;
  double double_val;
  char *string_val;
  NEPI_EDGE_LB_Param_Bytes_t bytes_val;
} NEPI_EDGE_LB_Param_Value_t;

/* **************** Status and Data Message API **************** */
typedef void* NEPI_EDGE_LB_Status_t;
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusCreate(NEPI_EDGE_LB_Status_t *status, const char* timestamp_rfc3339);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusDestroy(NEPI_EDGE_LB_Status_t status);

NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetNavSatFixTime(NEPI_EDGE_LB_Status_t status, const char* timestamp_rfc3339);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetLatitude(NEPI_EDGE_LB_Status_t status, float latitude_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetLongitude(NEPI_EDGE_LB_Status_t status, float longitude_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetHeading(NEPI_EDGE_LB_Status_t status, NEPI_EDGE_Heading_Ref_t heading_ref, float heading_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetRollAngle(NEPI_EDGE_LB_Status_t status, float roll_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetPitchAngle(NEPI_EDGE_LB_Status_t status, float pitch_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetTemperature(NEPI_EDGE_LB_Status_t status, float temperature_c);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetPowerState(NEPI_EDGE_LB_Status_t status, float power_state_percentage);
NEPI_EDGE_RET_t NEPI_EDGE_LBStatusSetDeviceStatus(NEPI_EDGE_LB_Status_t status, const uint8_t *status_entries, size_t status_entry_count);

typedef void* NEPI_EDGE_LB_Data_Snippet_t;
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetCreate(NEPI_EDGE_LB_Data_Snippet_t *snippet, const char type[NEPI_EDGE_DATA_SNIPPET_TYPE_LENGTH], uint32_t instance);
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetDestroy(NEPI_EDGE_LB_Data_Snippet_t snippet);

NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetDataTimestamp(NEPI_EDGE_LB_Data_Snippet_t snippet, const char* timestamp_rfc3339);
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetLatitude(NEPI_EDGE_LB_Data_Snippet_t snippet, float latitude_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetLongitude(NEPI_EDGE_LB_Data_Snippet_t snippet, float longitude_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetHeading(NEPI_EDGE_LB_Data_Snippet_t snippet, float heading_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetRollAngle(NEPI_EDGE_LB_Data_Snippet_t snippet, float roll_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetPitchAngle(NEPI_EDGE_LB_Data_Snippet_t snippet, float pitch_deg);
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetScores(NEPI_EDGE_LB_Data_Snippet_t snippet, float quality_score, float type_score, float event_score);
NEPI_EDGE_RET_t NEPI_EDGE_LBDataSnippetSetDataFile(NEPI_EDGE_LB_Data_Snippet_t snippet, const char *data_file_with_path, uint8_t delete_on_export);

NEPI_EDGE_RET_t NEPI_EDGE_LBExportData(const NEPI_EDGE_LB_Status_t status, const NEPI_EDGE_LB_Data_Snippet_t *snippets, size_t snippet_count);

/* **************** Config Message API **************** */
typedef void* NEPI_EDGE_LB_Config_t;
NEPI_EDGE_RET_t NEPI_EDGE_LBConfigCreate(NEPI_EDGE_LB_Config_t *config);
NEPI_EDGE_RET_t NEPI_EDGE_LBConfigDestroy(NEPI_EDGE_LB_Config_t config);
NEPI_EDGE_RET_t NEPI_EDGE_LBConfigDestroyArray(NEPI_EDGE_LB_Config_t *config_array, size_t count);

NEPI_EDGE_RET_t NEPI_EDGE_LBImportConfig(NEPI_EDGE_LB_Config_t config, const char* filename);
NEPI_EDGE_RET_t NEPI_EDGE_LBImportAllConfig(NEPI_EDGE_LB_Config_t **config_array, size_t *count);
NEPI_EDGE_RET_t NEPI_EDGE_LBConfigGetArrayEntry(NEPI_EDGE_LB_Config_t *config_array, size_t index, NEPI_EDGE_LB_Config_t **config_entry);
NEPI_EDGE_RET_t NEPI_EDGE_LBConfigGetParamCount(NEPI_EDGE_LB_Config_t *config, size_t *item_count);
NEPI_EDGE_RET_t NEPI_EDGE_LBConfigGetParam(NEPI_EDGE_LB_Config_t config, size_t param_index,
                                           NEPI_EDGE_LB_Param_Id_Type_t *id_type, NEPI_EDGE_LB_Param_Id_t *id,
                                           NEPI_EDGE_LB_Param_Value_Type_t *value_type, NEPI_EDGE_LB_Param_Value_t *value);

/* **************** General Message API **************** */
typedef void* NEPI_EDGE_LB_General_t;
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralCreate(NEPI_EDGE_LB_General_t *general);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralDestroy(NEPI_EDGE_LB_General_t general);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralDestroyArray(NEPI_EDGE_LB_General_t *general_array, size_t count);

NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrBool(NEPI_EDGE_LB_General_t general, const char *id, uint8_t val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrInt64(NEPI_EDGE_LB_General_t general, const char *id, int64_t val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrUInt64(NEPI_EDGE_LB_General_t general, const char *id, uint64_t val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrFloat(NEPI_EDGE_LB_General_t general, const char *id, float val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrDouble(NEPI_EDGE_LB_General_t general, const char *id, double val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrStr(NEPI_EDGE_LB_General_t general, const char *id, const char *val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadStrBytes(NEPI_EDGE_LB_General_t general, const char *id, const uint8_t *val, const size_t length);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntBool(NEPI_EDGE_LB_General_t general, uint32_t id, uint8_t val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntInt64(NEPI_EDGE_LB_General_t general, uint32_t id, int64_t val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntUInt64(NEPI_EDGE_LB_General_t general, uint32_t id, uint64_t val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntFloat(NEPI_EDGE_LB_General_t general, uint32_t id, float val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntDouble(NEPI_EDGE_LB_General_t general, uint32_t id, double val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntStr(NEPI_EDGE_LB_General_t general, uint32_t id, const char *val);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralSetPayloadIntBytes(NEPI_EDGE_LB_General_t general, uint32_t id, const uint8_t *val, const size_t length);

NEPI_EDGE_RET_t NEPI_EDGE_LBExportGeneral(NEPI_EDGE_LB_General_t general);

NEPI_EDGE_RET_t NEPI_EDGE_LBImportGeneral(NEPI_EDGE_LB_General_t general, const char* filename);
NEPI_EDGE_RET_t NEPI_EDGE_LBImportAllGeneral(NEPI_EDGE_LB_General_t **general_array, size_t *count);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralGetArrayEntry(NEPI_EDGE_LB_General_t *general_array, size_t index, NEPI_EDGE_LB_General_t **general_entry);
NEPI_EDGE_RET_t NEPI_EDGE_LBGeneralGetParam(NEPI_EDGE_LB_General_t general, NEPI_EDGE_LB_Param_Id_Type_t *id_type, NEPI_EDGE_LB_Param_Id_t *id,
                                            NEPI_EDGE_LB_Param_Value_Type_t *value_type, NEPI_EDGE_LB_Param_Value_t *value);


#endif //__NEPI_EDGE_LB_INTERFACE_H
