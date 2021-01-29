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

/* **************** Comms Status API **************** */
typedef enum NEPI_EDGE_COMMS_STATUS
{
  NEPI_EDGE_COMMS_STATUS_SUCCESS,
  NEPI_EDGE_COMMS_STATUS_CONN_FAILED,
  NEPI_EDGE_COMMS_STATUS_DISABLED,
  NEPI_EDGE_COMMS_STATUS_UNKNOWN
} NEPI_EDGE_COMMS_STATUS_t;

typedef void* NEPI_EDGE_Exec_Status_t;
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusCreate(NEPI_EDGE_Exec_Status_t *exec_status);
NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusDestroy(NEPI_EDGE_Exec_Status_t exec_status);

NEPI_EDGE_RET_t NEPI_EDGE_ImportExecStatus(NEPI_EDGE_Exec_Status_t exec_status);

#endif //__NEPI_EDGE_SDK_H
