#ifndef __NEPI_EDGE_ERRORS_H
#define __NEPI_EDGE_ERRORS_H

typedef enum NEPI_EDGE_RET
{
  NEPI_EDGE_RET_OK = 0,
  NEPI_EDGE_RET_UNINIT_OBJ = -1,
  NEPI_EDGE_RET_MALLOC_ERR = -2,
  NEPI_EDGE_RET_WRONG_OBJ_TYPE = -3,
  NEPI_EDGE_RET_ARG_OUT_OF_RANGE = -4,
  NEPI_EDGE_RET_ARG_TOO_LONG = -5,
  NEPI_EDGE_RET_FILE_OPEN_ERR = -6,
  NEPI_EDGE_RET_INCOMPLETE_OBJ = -7,
  NEPI_EDGE_RET_REQUIRED_FIELD_MISSING = -8,
  NEPI_EDGE_RET_FILE_PERMISSION_ERR = -9,
  NEPI_EDGE_RET_FILE_MISSING = -10,
  NEPI_EDGE_RET_INVALID_FILE_FORMAT = -11,
  NEPI_EDGE_RET_BAD_PARAM = -12,
  NEPI_EDGE_RET_FILE_MOVE_ERROR = -13,
  NEPI_EDGE_RET_FILE_DELETE_ERROR = -14,
  NEPI_EDGE_RET_SYMLINK_CREATE_ERROR = -15,
  NEPI_EDGE_RET_INVALID_BOT_PATH = -16,
  NEPI_EDGE_RET_BOT_ALREADY_RUNNING = -17,
  NEPI_EDGE_RET_BOT_NOT_RUNNING = -18,
  NEPI_EDGE_RET_CANT_START_BOT = -19,
  NEPI_EDGE_RET_BOT_EXEC_UNDETERMINED = -20,
  NEPI_EDGE_RET_CANT_KILL_BOT = -21,
} NEPI_EDGE_RET_t;

#endif //__NEPI_EDGE_ERRORS_H
