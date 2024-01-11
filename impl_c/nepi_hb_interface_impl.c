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
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "nepi_edge_sdk_link.h"
#include "nepi_edge_sdk_link_impl.h"
#include "nepi_edge_hb_interface.h"

static char targ_data_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH] = {'\0'};

NEPI_EDGE_RET_t NEPI_EDGE_HBLinkDataFolder(const char* data_folder_path)
{
  // First, check existence of the source data folder, and create it if necessary
  NEPI_EDGE_RET_t ret = NEPI_EDGE_SDKCheckPath(data_folder_path);
  if (NEPI_EDGE_RET_OK != ret) return ret;

  // Now, create/update the target data link
  snprintf(targ_data_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s",
           NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_HB_DO_DATA_FOLDER_PATH);

  // Check if it exists -- if so, we must delete it first
  if (0 == access( targ_data_path, F_OK))
  {
    if (0 != remove(targ_data_path))
    {
      return NEPI_EDGE_RET_FILE_DELETE_ERROR;
    }
  }

  // Now create the symlink
  if (0 != symlink(data_folder_path, targ_data_path))
  {
      return NEPI_EDGE_RET_SYMLINK_CREATE_ERROR;
  }

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_HBUnlinkDataFolder(void)
{
  if (targ_data_path[0] != '\0')
  {
    if (0 != remove(targ_data_path))
    {
      return NEPI_EDGE_RET_FILE_DELETE_ERROR;
    }
    targ_data_path[0] = '\0';
  }
  return NEPI_EDGE_RET_OK;
}
