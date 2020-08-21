#ifndef __NEPI_EDGE_LB_CONSTS_H
#define __NEPI_EDGE_LB_CONSTS_H

#define NEPI_EDGE_MAX_FILE_PATH_LENGTH  1024

#define NEPI_EDGE_DATA_SNIPPET_TYPE_LENGTH      3

#define NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH  64 // Plenty big enough for RTC3339

#define NEPI_EDGE_MAX_DATA_SNIPPET_FILE_NAME_LENGTH   256 // Doesn't include path, just the filename with extension

typedef enum NEPI_EDGE_Heading_Ref
{
  NEPI_EDGE_HEADING_REF_TRUE_NORTH,
  NEPI_EDGE_HEADING_REF_MAG_NORTH
} NEPI_EDGE_Heading_Ref_t;

#define NEPI_EDGE_LB_STATUS_FILENAME         "sys_status.json"

#define NEPI_EDGE_LB_DATA_FOLDER_PATH        "lb/data"
#define NEPI_EDGE_LB_CONFIG_FOLDER_PATH      "lb/cfg"
#define NEPI_EDGE_LB_GENERAL_DO_FOLDER_PATH  "lb/do-msg"
#define NEPI_EDGE_LB_GENERAL_DT_FOLDER_PATH  "lb/dt-msg"
#define NEPI_EDGE_HB_CLONE_DO_FOLDER_PATH    "hb/clone-do"
#define NEPI_EDGE_HB_CLONE_DT_FOLDER_PATH    "hb/clone-dt"
#endif // __NEPI_EDGE_LB_CONSTS_H
