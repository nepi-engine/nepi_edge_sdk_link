#ifndef __NEPI_LB_INTERFACE_IMPL_H
#define __NEPI_LB_INTERFACE_IMPL_H

#include <stdint.h>
#include <stddef.h>

#include "nepi_edge_lb_consts.h"

typedef enum NEPI_EDGE_LB_MSG_ID
{
  NEPI_EDGE_LB_MSG_ID_STATUS,
  NEPI_EDGE_LB_MSG_ID_DATA,
  NEPI_EDGE_LB_MSG_ID_CONFIG,
  NEPI_EDGE_LB_MSG_ID_GENERAL
} NEPI_EDGE_LB_MSG_ID_t;

typedef struct NEPI_EDGE_LB_Opaque_Helper
{
  NEPI_EDGE_LB_MSG_ID_t msg_id;
  uint32_t fields_set;
} NEPI_EDGE_LB_Opaque_Helper_t;

struct NEPI_EDGE_LB_Status
{
  char timestamp_rfc3339[NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH]; // Obtained e.g., via date --rtc3339=ns
  char navsat_fix_time_rfc3339[NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH];
  float latitude_deg;
  float longitude_deg;
  float heading_deg;
  NEPI_EDGE_Heading_Ref_t heading_ref;
  float roll_angle_deg;
  float pitch_angle_deg;
  float temperature_c;
  uint8_t power_state_percentage;
  const uint8_t *device_status_entries;
  size_t device_status_entry_count;

  NEPI_EDGE_LB_Opaque_Helper_t opaque_helper;
};

typedef enum NEPI_EDGE_LB_Status_Fields_Bitmask
{
  NEPI_EDGE_LB_Status_Fields_Timestamp = (1u << 0),
  NEPI_EDGE_LB_Status_Fields_NavSatFixTime = (1u << 1),
  NEPI_EDGE_LB_Status_Fields_Latitude = (1u << 2),
  NEPI_EDGE_LB_Status_Fields_Longitude = (1u << 3),
  NEPI_EDGE_LB_Status_Fields_HeadingAndRef = (1u << 4),
  NEPI_EDGE_LB_Status_Fields_RollAngle = (1u << 5),
  NEPI_EDGE_LB_Status_Fields_PitchAngle = (1u << 6),
  NEPI_EDGE_LB_Status_Fields_Temperature = (1u << 7),
  NEPI_EDGE_LB_Status_Fields_PowerState = (1u << 8),
  NEPI_EDGE_LB_Status_Fields_DeviceStatus = (1u << 9)
} NEPI_EDGE_LB_Status_Fields_Bitmask_t;

struct NEPI_EDGE_LB_Data_Snippet
{
  char type[NEPI_EDGE_DATA_SNIPPET_TYPE_LENGTH];
  uint32_t instance;
  char data_time_rfc3339[NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH];
  float latitude_deg;
  float longitude_deg;
  float heading_deg;
  float roll_angle_deg;
  float pitch_angle_deg;
  float quality_score;
  float type_score;
  float event_score;
  char data_file[NEPI_EDGE_MAX_DATA_SNIPPET_FILE_NAME_LENGTH];

  NEPI_EDGE_LB_Opaque_Helper_t opaque_helper;
};

typedef enum NEPI_EDGE_LB_Data_Snippet_Fields_Bitmask
{
  NEPI_EDGE_LB_Data_Snippet_Fields_TypeAndInstance = (1u << 0),
  NEPI_EDGE_LB_Data_Snippet_Fields_Data_Time = (1u << 1),
  NEPI_EDGE_LB_Data_Snippet_Fields_Latitude = (1u << 2),
  NEPI_EDGE_LB_Data_Snippet_Fields_Longitude = (1u << 3),
  NEPI_EDGE_LB_Data_Snippet_Fields_Heading = (1u << 4),
  NEPI_EDGE_LB_Data_Snippet_Fields_RollAngle = (1u << 5),
  NEPI_EDGE_LB_Data_Snippet_Fields_PitchAngle = (1u << 6),
  NEPI_EDGE_LB_Data_Snippet_Fields_Scores = (1u << 7),
  NEPI_EDGE_LB_Data_Snippet_Fields_DataFile = (1u << 8)
} NEPI_EDGE_LB_Data_Snippet_Fields_Bitmask_t;

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

typedef struct NEPI_EDGE_LB_Param
{
  NEPI_EDGE_LB_Param_Id_Type_t id_type;
  NEPI_EDGE_LB_Param_Id_t id;

  NEPI_EDGE_LB_Param_Value_Type_t value_type;
  NEPI_EDGE_LB_Param_Value_t value;

  struct NEPI_EDGE_LB_Param *next; // Linked list pointer for use in the CONFIG struct
} NEPI_EDGE_LB_Param_t;

typedef enum NEPI_EDGE_LB_Config_Fields_Bitmask
{
  NEPI_EDGE_LB_Config_Fields_Params = (1u << 0),
} NEPI_EDGE_LB_Config_Fields_Bitmask_t;

struct NEPI_EDGE_LB_Config
{
  NEPI_EDGE_LB_Param_t *params; // Linked list

  NEPI_EDGE_LB_Opaque_Helper_t opaque_helper;
};

typedef enum NEPI_EDGE_LB_General_Fields_Bitmask
{
  NEPI_EDGE_LB_General_Fields_Payload = (1u << 0),
} NEPI_EDGE_LB_General_Fields_Bitmask_t;

struct NEPI_EDGE_LB_General
{
  NEPI_EDGE_LB_Param_t param;

  NEPI_EDGE_LB_Opaque_Helper_t opaque_helper;
};

#endif //__NEPI_LB_INTERFACE_IMPL_H
