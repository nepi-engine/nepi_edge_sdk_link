/*
 * NEPI Dual-Use License
 * Project: nepi_edge_sdk_link
 *
 * This license applies to any user of NEPI Engine software
 *
 * Copyright (C) 2023 Numurus, LLC <https://www.numurus.com>
 * see https://github.com/numurus-nepi/nepi_edge_sdk_link
 *
 * This software is dual-licensed under the terms of either a NEPI software developer license
 * or a NEPI software commercial license.
 *
 * The terms of both the NEPI software developer and commercial licenses
 * can be found at: www.numurus.com/licensing-nepi-engine
 *
 * Redistributions in source code must retain this top-level comment block.
 * Plagiarizing this software to sidestep the license obligations is illegal.
 *
 * Contact Information:
 * ====================
 * - https://www.numurus.com/licensing-nepi-engine
 * - mailto:nepi@numurus.com
 *
 */
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
