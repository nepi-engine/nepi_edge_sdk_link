#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "nepi_edge_sdk.h"
#include "nepi_edge_sdk_impl.h"
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
