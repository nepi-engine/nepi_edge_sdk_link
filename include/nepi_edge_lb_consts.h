#ifndef __NEPI_EDGE_LB_CONSTS_H
#define __NEPI_EDGE_LB_CONSTS_H

#define NEPI_EDGE_DATA_SNIPPET_TYPE_LENGTH      3

#define NEPI_EDGE_BYTE_ARRAY_BLOCK_SIZE   1024 // Bytes allocated at a time when importing a JSON file with a byte array

typedef enum NEPI_EDGE_Heading_Ref
{
  NEPI_EDGE_HEADING_REF_TRUE_NORTH = 0,
  NEPI_EDGE_HEADING_REF_MAG_NORTH = 1
} NEPI_EDGE_Heading_Ref_t;

#define NEPI_EDGE_LB_STATUS_FILENAME         "sys_status.json"

#define NEPI_EDGE_LB_DATA_FOLDER_PATH        "lb/data"
#define NEPI_EDGE_LB_CONFIG_FOLDER_PATH      "lb/cfg"
#define NEPI_EDGE_LB_GENERAL_DO_FOLDER_PATH  "lb/do-msg"
#define NEPI_EDGE_LB_GENERAL_DT_FOLDER_PATH  "lb/dt-msg"
#endif // __NEPI_EDGE_LB_CONSTS_H
