#ifndef __NEPI_EDGE_SDK_H
#define __NEPI_EDGE_SDK_H

#include "nepi_edge_errors.h"
#include "nepi_edge_lb_consts.h"
#include "nepi_edge_hb_consts.h"

#define NEPI_EDGE_MAX_FILE_PATH_LENGTH  1024
#define NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH  64 // Plenty big enough for RTC3339

#define NEPI_EDGE_LB_EXEC_STAT_FILE_PATH      "log/lb_execution_status.json"
#define NEPI_EDGE_HB_EXEC_STAT_FILE_PATH      "log/hb_execution_status.json"

NEPI_EDGE_RET_t NEPI_EDGE_SetBotBaseFilePath(const char* path);
const char* NEPI_EDGE_GetBotBaseFilePath(void);

/* **************** Exec Status API **************** */
typedef enum NEPI_EDGE_COMMS_STATUS
{
  NEPI_EDGE_COMMS_STATUS_DISABLED         = 0,
  NEPI_EDGE_COMMS_STATUS_SUCCESS          = 1,
  NEPI_EDGE_COMMS_STATUS_CONN_FAILED      = 2,
  NEPI_EDGE_COMMS_STATUS_UNKNOWN          = 3

} NEPI_EDGE_COMMS_STATUS_t;

typedef enum NEPI_EDGE_HB_DIRECTION
{
  NEPI_EDGE_HB_DIRECTION_DO      = 0,
  NEPI_EDGE_HB_DIRECTION_DT      = 1,
  NEPI_EDGE_HB_DIRECTION_UNKNOWN = 2
} NEPI_EDGE_HB_DIRECTION_t;

typedef void* NEPI_EDGE_Exec_Status_t;
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusCreate(NEPI_EDGE_Exec_Status_t *exec_status);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusDestroy(NEPI_EDGE_Exec_Status_t exec_status);

NEPI_EDGE_RET_t NEPI_EDGE_ImportExecStatus(NEPI_EDGE_Exec_Status_t exec_status);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetCounts(NEPI_EDGE_Exec_Status_t exec_status, size_t *lb_counts, size_t *hb_counts);

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsType(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, char **comms_type);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsStatus(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, NEPI_EDGE_COMMS_STATUS_t *comms_status);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsTimestamps(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, char **start_time_rfc3339, char **stop_time_rfc3339);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsGetWarnErrCount(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, size_t *warning_count, size_t *error_count);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsGetWarning(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, size_t warning_index, char **warning_msg);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsGetError(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, size_t error_index, char **error_msg);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsStatistics(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, size_t *msgs_sent, size_t *pkts_sent, size_t *msgs_rcvd);

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsType(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, char **comms_type);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsStatus(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, NEPI_EDGE_COMMS_STATUS_t *comms_status);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsTimestamps(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, char **start_time_rfc3339, char **stop_time_rfc3339);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsGetWarnErrCount(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, size_t *warning_count, size_t *error_count);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsGetWarning(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, size_t warning_index, char **warning_msg);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsGetError(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, size_t error_index, char **error_msg);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsDirection(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, NEPI_EDGE_HB_DIRECTION_t *direction);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsStatistics(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, size_t *datasent_kB, size_t *datareceived_kB);

#endif //__NEPI_EDGE_SDK_H
