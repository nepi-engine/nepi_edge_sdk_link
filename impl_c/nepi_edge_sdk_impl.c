#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nepi_edge_sdk.h"

static char nepi_edge_bot_base_file_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH] = {'\0'};

static void mkdir_recursive(const char *dir, mode_t mode)
{
  char tmp[256];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp),"%s",dir);
  len = strlen(tmp);
  if(tmp[len - 1] == '/')
  {
    tmp[len - 1] = 0;
  }
  for(p = tmp + 1; *p; p++)
  {
    if(*p == '/')
    {
      *p = 0;
      mkdir(tmp, mode);
      *p = '/';
    }
  }
  mkdir(tmp, mode);
}

NEPI_EDGE_RET_t NEPI_EDGE_SDKCheckPath(const char* path)
{
  // First, check that the path exists, and try to create it if not
  if (-1 == access(path, F_OK))
  {
    mkdir_recursive(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  }

  // And ensure we have proper permissions
  if (-1 == access(path, R_OK | W_OK))
  {
    return NEPI_EDGE_RET_FILE_PERMISSION_ERR;
  }

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_SetBotBaseFilePath(const char* path)
{
  // First create and/or check permissions on subfolder paths
  char tmp_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];

  // lb/data
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_LB_DATA_FOLDER_PATH);
  NEPI_EDGE_RET_t ret = NEPI_EDGE_SDKCheckPath(tmp_path);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // lb/cfg
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_LB_CONFIG_FOLDER_PATH);
  ret = NEPI_EDGE_SDKCheckPath(tmp_path);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // lb/do-msg
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_LB_GENERAL_DO_FOLDER_PATH);
  ret = NEPI_EDGE_SDKCheckPath(tmp_path);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // lb/dt-msg
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_LB_GENERAL_DT_FOLDER_PATH);
  ret = NEPI_EDGE_SDKCheckPath(tmp_path);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // hb/clone-do
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_HB_CLONE_DO_FOLDER_PATH);
  ret = NEPI_EDGE_SDKCheckPath(tmp_path);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // hb/clone-dt
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_HB_CLONE_DT_FOLDER_PATH);
  ret = NEPI_EDGE_SDKCheckPath(tmp_path);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // Everything checks out, so update the global variable and return success
  strncpy(nepi_edge_bot_base_file_path, path, NEPI_EDGE_MAX_FILE_PATH_LENGTH);
  return NEPI_EDGE_RET_OK;
}

const char* NEPI_EDGE_GetBotBaseFilePath(void)
{
  return nepi_edge_bot_base_file_path;
}
