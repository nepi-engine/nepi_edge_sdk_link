#ifndef __NEPI_EDGE_SDK_IMPL_H
#define __NEPI_EDGE_SDK_IMPL_H

#include <stdint.h>
#include <stdlib.h>

#include "nepi_edge_sdk.h"

// In case we want to provide arena allocator, etc. someday, don't call
// malloc() and free() directly
#define NEPI_EDGE_MALLOC(x) malloc((x))
#define NEPI_EDGE_FREE(x) free((x))

#define VALIDATE_OPAQUE_TYPE(x,t,s) \
  if (NULL == (x)) return NEPI_EDGE_RET_UNINIT_OBJ;\
  struct s *p = (struct s*)(x);\
  if (p->opaque_helper.msg_id != t) return NEPI_EDGE_RET_WRONG_OBJ_TYPE;

NEPI_EDGE_RET_t NEPI_EDGE_SDKCheckPath(const char* path);

typedef enum NEPI_EDGE_OPAQUE_TYPE_ID
{
  NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS
} NEPI_EDGE_OPAQUE_TYPE_ID;

typedef struct NEPI_EDGE_Opaque_Helper
{
  NEPI_EDGE_OPAQUE_TYPE_ID msg_id;
} NEPI_EDGE_Opaque_Helper_t;

#define NEPI_EDGE_MAX_LB_CONNECTIONS_PER_EXEC     32

#define NEPI_EDGE_MAX_COMMS_TYPE_STRLENGTH   32
#define NEPI_EDGE_MAX_COMMS_ERROR_STRLENGTH       64
#define NEPI_EDGE_MAX_COMMS_ERROR_CNT             8

#define NEPI_EDGE_COMMS_STATUS_STRING_SUCCESS       "success"
#define NEPI_EDGE_COMMS_STATUS_STRING_CONN_FAILED   "connfailed"
#define NEPI_EDGE_COMMS_STATUS_STRING_DISABLED      "disabled"

struct NEPI_EDGE_Connection_Status_Common_Hdr
{
  char comms_type[NEPI_EDGE_MAX_COMMS_TYPE_STRLENGTH];
  NEPI_EDGE_COMMS_STATUS_t status;
  char start_time_rfc3339[NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH];
  char stop_time_rfc3339[NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH];
  char warnings[NEPI_EDGE_MAX_COMMS_ERROR_CNT][NEPI_EDGE_MAX_COMMS_ERROR_STRLENGTH];
  size_t warning_count;
  uint8_t currently_parsing_warnings; // helper
  char errors[NEPI_EDGE_MAX_COMMS_ERROR_CNT][NEPI_EDGE_MAX_COMMS_ERROR_STRLENGTH];
  size_t error_count;
  uint8_t currently_parsing_errors; // helper

  uint32_t fields_set;
};

typedef enum NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Bitmask
{
  NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Comms_Type = (1u << 0),
  NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Status = (1u << 1),
  NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Start_Time = (1u << 2),
  NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Stop_Time = (1u << 3),
  NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Warnings = (1u << 4),
  NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Errors = (1u << 5)
  // If adding a new entry here, other enum start values must be adjusted below
} NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Bitmask_t;

struct NEPI_EDGE_LB_Connection_Status
{
  struct NEPI_EDGE_Connection_Status_Common_Hdr hdr;

  uint32_t messages_sent;
  uint32_t packets_sent;
  uint32_t messages_received;

  struct NEPI_EDGE_LB_Connection_Status *next; // Linked list helper
};

typedef enum NEPI_EDGE_LB_Connection_Status_Fields_Bitmask
{
  NEPI_EDGE_LB_Connection_Status_Fields_Messages_Sent = (1u << 6),
  NEPI_EDGE_LB_Connection_Status_Fields_Packets_Sent = (1u << 7),
  NEPI_EDGE_LB_Connection_Status_Fields_Messages_Received = (1u << 8)
} NEPI_EDGE_LB_Connection_Status_Fields_Bitmask_t;

typedef enum NEPI_EDGE_HB_Connection_Dtype
{
  NEPI_EDGE_HB_Connection_Dtype_DO,
  NEPI_EDGE_HB_Connection_Dtype_DT,
  NEPI_EDGE_HB_Connection_Dtype_Unknown
} NEPI_EDGE_HB_Connection_Dtype_t;

struct NEPI_EDGE_HB_Connection_Status
{
  struct NEPI_EDGE_Connection_Status_Common_Hdr hdr;

  uint32_t datasent_kb;
  uint32_t datareceived_kb;
  NEPI_EDGE_HB_Connection_Dtype_t dtype;

  struct NEPI_EDGE_HB_Connection_Status *next; // Linked list helper
};

typedef enum NEPI_EDGE_HB_Connection_Status_Fields_Bitmask
{
  NEPI_EDGE_HB_Connection_Status_Fields_Data_Sent = (1u << 6),
  NEPI_EDGE_HB_Connection_Status_Fields_Data_Received = (1u << 7),
  NEPI_EDGE_HB_Connection_Status_Fields_Dtype = (1u << 8),
} NEPI_EDGE_HB_Connection_Status_Fields_Bitmask_t;

struct NEPI_EDGE_Exec_Status
{
  struct NEPI_EDGE_LB_Connection_Status *lb_conn_status; // Linked list

  struct NEPI_EDGE_HB_Connection_Status *hb_conn_status; // Linked list

  NEPI_EDGE_Opaque_Helper_t opaque_helper;
};

#endif //__NEPI_EDGE_SDK_IMPL_H
