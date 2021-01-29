#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nepi_edge_sdk_impl.h"

#include "frozen/frozen.h"

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
  if (-1 == access(path, F_OK)) // Not a regular file
  {
    struct stat sb;
    if (-1 == lstat(path, &sb)) // Not a symlink either (use lstat because it reports on the link, not the file it points to)
    {
      mkdir_recursive(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    else if (S_IFLNK == (sb.st_mode & S_IFMT)) // must be a broken link
    {
      // Try to delete the broken link and create a new one
      if (0 != remove(path))
      {
        return NEPI_EDGE_RET_FILE_DELETE_ERROR;
      }
      mkdir_recursive(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
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

  // hb/do/data
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_HB_DO_DATA_FOLDER_PATH);
  ret = NEPI_EDGE_SDKCheckPath(tmp_path);
  if (ret != NEPI_EDGE_RET_OK) return ret;

  // hb/dt
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_HB_DT_FOLDER_PATH);
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

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusCreate(NEPI_EDGE_Exec_Status_t *exec_status)
{
  *exec_status = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_Exec_Status));
  if (NULL == *exec_status) return NEPI_EDGE_RET_MALLOC_ERR;

  struct NEPI_EDGE_Exec_Status *p = (struct NEPI_EDGE_Exec_Status*)(*exec_status);
  p->opaque_helper.msg_id = NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS;

  p->lb_conn_status = NULL; // Linked list
  p->hb_conn_status = NULL; // Linked list

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusDestroy(NEPI_EDGE_Exec_Status_t exec_status)
{
  VALIDATE_OPAQUE_TYPE(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status)

  struct NEPI_EDGE_LB_Connection_Status *lb_conn_status = p->lb_conn_status;
  struct NEPI_EDGE_LB_Connection_Status *tmp_lb;
  while (lb_conn_status != NULL)
  {
    tmp_lb = lb_conn_status->next;
    NEPI_EDGE_FREE(lb_conn_status);
    lb_conn_status = tmp_lb;
  }

  struct NEPI_EDGE_HB_Connection_Status *hb_conn_status = p->hb_conn_status;
  struct NEPI_EDGE_HB_Connection_Status *tmp_hb;
  while (hb_conn_status != NULL)
  {
    tmp_hb = hb_conn_status->next;
    NEPI_EDGE_FREE(hb_conn_status);
    hb_conn_status = tmp_hb;
  }

  NEPI_EDGE_FREE(exec_status);
  exec_status = NULL;

  return NEPI_EDGE_RET_OK;
}

static void detectNewLBConnectionEntry(struct NEPI_EDGE_LB_Connection_Status **conn_status, uint32_t field_mask)
{
  // If comms_type is already set, this is a new entry so step the linked list and start fresh
  if ((*conn_status)->hdr.fields_set & field_mask)
  {
    (*conn_status)->next = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_LB_Connection_Status));
    (*conn_status) = (*conn_status)->next;
    (*conn_status)->next = NULL;
    (*conn_status)->hdr.fields_set = 0;
    (*conn_status)->hdr.currently_parsing_warnings = 0;
    (*conn_status)->hdr.currently_parsing_errors = 0;
  }
}

static void json_walk_exec_lb_status_callback(void *callback_data, const char *name, size_t name_len, const char *path, const struct json_token *token)
{
  if (token->type == JSON_TYPE_OBJECT_START || token->type == JSON_TYPE_OBJECT_END) return;

  char tmp_name[1024];
  size_t tmp_len = (name_len < 1024)? name_len : 1024;
  strncpy(tmp_name, name, tmp_len);
  tmp_name[tmp_len] = '\0';

  if (0 == strcmp(tmp_name, "connections")) return; // The start of the connections array, nothing to do

  struct NEPI_EDGE_Exec_Status *exec_status = *(struct NEPI_EDGE_Exec_Status**)(callback_data);
  if (exec_status->lb_conn_status == NULL) // First entry of linked list not allocated yet
  {
    exec_status->lb_conn_status = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_LB_Connection_Status));
    if (NULL == exec_status->lb_conn_status) return; // Not much we can do here -- probably totally hosed

    exec_status->lb_conn_status->next = NULL;
    exec_status->lb_conn_status->hdr.fields_set = 0;
    exec_status->lb_conn_status->hdr.currently_parsing_warnings = 0;
    exec_status->lb_conn_status->hdr.currently_parsing_errors = 0;
  }

  struct NEPI_EDGE_LB_Connection_Status *conn_status = exec_status->lb_conn_status;

  // Navigate to the last entry in the linked list -- that's the one we're working on right now
  while (conn_status->next != NULL)
  {
    conn_status = conn_status->next;
  }

  if (0 == strcmp(tmp_name, "comms_type"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Comms_Type);

    const size_t bytes_to_copy = (token->len < NEPI_EDGE_MAX_COMMS_TYPE_STRLENGTH)? token->len : NEPI_EDGE_MAX_COMMS_TYPE_STRLENGTH;
    memcpy(conn_status->hdr.comms_type, token->ptr, bytes_to_copy);
    conn_status->hdr.comms_type[bytes_to_copy] = '\0';
    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Comms_Type;
  }
  else if (0 == strcmp(tmp_name, "status"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Status);

    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Status;
    char tmp_val[1024];
    size_t tmp_len = (token->len < 1024)? token->len : 1024;
    memcpy(tmp_val, token->ptr, tmp_len);
    tmp_val[tmp_len] = '\0';

    if (0 == strcmp(tmp_val, NEPI_EDGE_COMMS_STATUS_STRING_SUCCESS)) conn_status->hdr.status = NEPI_EDGE_COMMS_STATUS_SUCCESS;
    else if (0 == strcmp(tmp_val, NEPI_EDGE_COMMS_STATUS_STRING_CONN_FAILED)) conn_status->hdr.status = NEPI_EDGE_COMMS_STATUS_CONN_FAILED;
    else if (0 == strcmp(tmp_val, NEPI_EDGE_COMMS_STATUS_STRING_DISABLED)) conn_status->hdr.status = NEPI_EDGE_COMMS_STATUS_DISABLED;
    else conn_status->hdr.status = NEPI_EDGE_COMMS_STATUS_UNKNOWN;
  }
  else if (0 == strcmp(tmp_name, "timestart"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Start_Time);

    const size_t bytes_to_copy = (token->len < NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH)? token->len : NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH;
    memcpy(conn_status->hdr.start_time_rfc3339, token->ptr, bytes_to_copy);
    conn_status->hdr.start_time_rfc3339[bytes_to_copy] = '\0';
    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Start_Time;
  }
  else if (0 == strcmp(tmp_name, "timestop"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Stop_Time);

    const size_t bytes_to_copy = (token->len < NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH)? token->len : NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH;
    memcpy(conn_status->hdr.stop_time_rfc3339, token->ptr, bytes_to_copy);
    conn_status->hdr.stop_time_rfc3339[bytes_to_copy] = '\0';
    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Stop_Time;
  }
  else if (0 == strcmp(tmp_name, "warnings"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Warnings);

    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Warnings;
    conn_status->hdr.warning_count = 0;
    conn_status->hdr.currently_parsing_warnings = 1;
  }
  else if (0 == strcmp(tmp_name, "errors"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Errors);

    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Errors;
    conn_status->hdr.error_count = 0;
    conn_status->hdr.currently_parsing_errors = 1;
  }
  else if (0 == strcmp(tmp_name, "msgsent"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_LB_Connection_Status_Fields_Messages_Sent);

    conn_status->messages_sent = strtol(token->ptr, NULL, 10);
    conn_status->hdr.fields_set |= NEPI_EDGE_LB_Connection_Status_Fields_Messages_Sent;
  }
  else if (0 == strcmp(tmp_name, "pktsent"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_LB_Connection_Status_Fields_Packets_Sent);

    conn_status->packets_sent = strtol(token->ptr, NULL, 10);
    conn_status->hdr.fields_set |= NEPI_EDGE_LB_Connection_Status_Fields_Packets_Sent;
  }
  else if (0 == strcmp(tmp_name, "msgrecv"))
  {
    detectNewLBConnectionEntry(&conn_status, NEPI_EDGE_LB_Connection_Status_Fields_Messages_Received);

    conn_status->messages_received = strtol(token->ptr, NULL, 10);
    conn_status->hdr.fields_set |= NEPI_EDGE_LB_Connection_Status_Fields_Messages_Received;
  }
  else if ((0 == strcmp(tmp_name, "statsent")) || (0 == strcmp(tmp_name, "datasent")) ||
           (0 == strcmp(tmp_name, "gensent")) || (0 == strcmp(tmp_name, "cfgrecv")) ||
           (0 == strcmp(tmp_name, "genrecv")))
  {
    // We aren't doing anything with these fields presently
    return;
  }

  // Otherwise, we're in the error or warning array
  else if ((token->type == JSON_TYPE_STRING) && (path[strlen(path) - 1] == ']'))
  {
    // This cannot be the start of a new connections entry, so skip detectNewLBConnectionEntry
    char *destination_buf = NULL;
    if ((1 == conn_status->hdr.currently_parsing_warnings) && (conn_status->hdr.warning_count < NEPI_EDGE_MAX_COMMS_ERROR_CNT))
    {
      destination_buf = conn_status->hdr.warnings[conn_status->hdr.warning_count];
      ++(conn_status->hdr.warning_count);
    }
    else if ((1 == conn_status->hdr.currently_parsing_errors) && (conn_status->hdr.error_count < NEPI_EDGE_MAX_COMMS_ERROR_CNT))
    {
      destination_buf = conn_status->hdr.errors[conn_status->hdr.error_count];
      ++(conn_status->hdr.error_count);
    }
    else // Either we're in an unknown array or the warning/error array is alradyat max capacity
    {
      return;
    }
    const int bytes_to_copy = (token->len < NEPI_EDGE_MAX_COMMS_ERROR_STRLENGTH)? token->len : NEPI_EDGE_MAX_COMMS_ERROR_STRLENGTH;
    memcpy(destination_buf, token->ptr, bytes_to_copy);
    destination_buf[bytes_to_copy] = '\0';
  }
  else if (token->type == JSON_TYPE_ARRAY_END) // Must catch this one to update the state
  {
    if (1 == conn_status->hdr.currently_parsing_warnings)
    {
      conn_status->hdr.currently_parsing_warnings = 0;
    }
    else if (1 == conn_status->hdr.currently_parsing_errors)
    {
      conn_status->hdr.currently_parsing_errors = 0;
    }
    else
    {
      return; // Probably just the end of the "connections" array
    }
  }
}

static void detectNewHBConnectionEntry(struct NEPI_EDGE_HB_Connection_Status **conn_status, uint32_t field_mask)
{
  // If comms_type is already set, this is a new entry so step the linked list and start fresh
  if ((*conn_status)->hdr.fields_set & field_mask)
  {
    (*conn_status)->next = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_HB_Connection_Status));
    (*conn_status) = (*conn_status)->next;
    (*conn_status)->next = NULL;
    (*conn_status)->hdr.fields_set = 0;
    (*conn_status)->hdr.currently_parsing_warnings = 0;
    (*conn_status)->hdr.currently_parsing_errors = 0;
  }
}

static void json_walk_exec_hb_status_callback(void *callback_data, const char *name, size_t name_len, const char *path, const struct json_token *token)
{
  if (token->type == JSON_TYPE_OBJECT_START || token->type == JSON_TYPE_OBJECT_END) return;

  char tmp_name[1024];
  size_t tmp_len = (name_len < 1024)? name_len : 1024;
  strncpy(tmp_name, name, tmp_len);
  tmp_name[tmp_len] = '\0';

  if (0 == strcmp(tmp_name, "connections")) return; // The start of the connections array, nothing to do

  struct NEPI_EDGE_Exec_Status *exec_status = *(struct NEPI_EDGE_Exec_Status**)(callback_data);
  if (exec_status->hb_conn_status == NULL) // First entry of linked list not allocated yet
  {
    exec_status->hb_conn_status = NEPI_EDGE_MALLOC(sizeof(struct NEPI_EDGE_HB_Connection_Status));
    if (NULL == exec_status->hb_conn_status) return; // Not much we can do here -- probably totally hosed

    exec_status->hb_conn_status->next = NULL;
    exec_status->hb_conn_status->hdr.fields_set = 0;
    exec_status->hb_conn_status->hdr.currently_parsing_warnings = 0;
    exec_status->hb_conn_status->hdr.currently_parsing_errors = 0;
  }

  struct NEPI_EDGE_HB_Connection_Status *conn_status = exec_status->hb_conn_status;

  // Navigate to the last entry in the linked list -- that's the one we're working on right now
  while (conn_status->next != NULL)
  {
    conn_status = conn_status->next;
  }

  if (0 == strcmp(tmp_name, "comms_type"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Comms_Type);

    const size_t bytes_to_copy = (token->len < NEPI_EDGE_MAX_COMMS_TYPE_STRLENGTH)? token->len : NEPI_EDGE_MAX_COMMS_TYPE_STRLENGTH;
    memcpy(conn_status->hdr.comms_type, token->ptr, bytes_to_copy);
    conn_status->hdr.comms_type[bytes_to_copy] = '\0';
    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Comms_Type;
  }
  else if (0 == strcmp(tmp_name, "status"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Status);

    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Status;
    char tmp_val[1024];
    size_t tmp_len = (token->len < 1024)? token->len : 1024;
    memcpy(tmp_val, token->ptr, tmp_len);
    tmp_val[tmp_len] = '\0';

    if (0 == strcmp(tmp_val, NEPI_EDGE_COMMS_STATUS_STRING_SUCCESS)) conn_status->hdr.status = NEPI_EDGE_COMMS_STATUS_SUCCESS;
    else if (0 == strcmp(tmp_val, NEPI_EDGE_COMMS_STATUS_STRING_CONN_FAILED)) conn_status->hdr.status = NEPI_EDGE_COMMS_STATUS_CONN_FAILED;
    else if (0 == strcmp(tmp_val, NEPI_EDGE_COMMS_STATUS_STRING_DISABLED)) conn_status->hdr.status = NEPI_EDGE_COMMS_STATUS_DISABLED;
    else conn_status->hdr.status = NEPI_EDGE_COMMS_STATUS_UNKNOWN;
  }
  else if (0 == strcmp(tmp_name, "timestart"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Start_Time);

    const size_t bytes_to_copy = (token->len < NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH)? token->len : NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH;
    memcpy(conn_status->hdr.start_time_rfc3339, token->ptr, bytes_to_copy);
    conn_status->hdr.start_time_rfc3339[bytes_to_copy] = '\0';
    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Start_Time;
  }
  else if (0 == strcmp(tmp_name, "timestop"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Stop_Time);

    const size_t bytes_to_copy = (token->len < NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH)? token->len : NEPI_EDGE_MAX_TSTAMP_STRING_LENGTH;
    memcpy(conn_status->hdr.stop_time_rfc3339, token->ptr, bytes_to_copy);
    conn_status->hdr.stop_time_rfc3339[bytes_to_copy] = '\0';
    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Stop_Time;
  }
  else if (0 == strcmp(tmp_name, "warnings"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Warnings);

    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Warnings;
    conn_status->hdr.warning_count = 0;
    conn_status->hdr.currently_parsing_warnings = 1;
  }
  else if (0 == strcmp(tmp_name, "errors"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Errors);

    conn_status->hdr.fields_set |= NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Errors;
    conn_status->hdr.error_count = 0;
    conn_status->hdr.currently_parsing_errors = 1;
  }
  else if (0 == strcmp(tmp_name, "dtype"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_HB_Connection_Status_Fields_Dtype);

    if (0 == memcmp("do", token->ptr, 2)) conn_status->dtype = NEPI_EDGE_HB_Connection_Dtype_DO;
    else if (0 == memcmp("dt", token->ptr, 2)) conn_status->dtype = NEPI_EDGE_HB_Connection_Dtype_DT;
    else conn_status->dtype = NEPI_EDGE_HB_Connection_Dtype_Unknown;

    conn_status->hdr.fields_set |= NEPI_EDGE_HB_Connection_Status_Fields_Dtype;
  }
  else if (0 == strcmp(tmp_name, "datasent_kb"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_HB_Connection_Status_Fields_Data_Sent);

    conn_status->datasent_kb = strtol(token->ptr, NULL, 10);
    conn_status->hdr.fields_set |= NEPI_EDGE_HB_Connection_Status_Fields_Data_Sent;
  }
  else if (0 == strcmp(tmp_name, "datarecv_kb"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_HB_Connection_Status_Fields_Data_Received);

    conn_status->datareceived_kb = strtol(token->ptr, NULL, 10);
    conn_status->hdr.fields_set |= NEPI_EDGE_HB_Connection_Status_Fields_Data_Received;
  }
  else if ((0 == strcmp(tmp_name, "numdirs")) || (0 == strcmp(tmp_name, "numfiles")))
  {
    // We aren't doing anything with these fields presently
    return;
  }

  // Otherwise, we're in the error or warning array
  else if ((token->type == JSON_TYPE_STRING) && (path[strlen(path) - 1] == ']'))
  {
    // This cannot be the start of a new connections entry, so skip detectNewHBConnectionEntry
    char *destination_buf = NULL;
    if ((1 == conn_status->hdr.currently_parsing_warnings) && (conn_status->hdr.warning_count < NEPI_EDGE_MAX_COMMS_ERROR_CNT))
    {
      destination_buf = conn_status->hdr.warnings[conn_status->hdr.warning_count];
      ++(conn_status->hdr.warning_count);
    }
    else if ((1 == conn_status->hdr.currently_parsing_errors) && (conn_status->hdr.error_count < NEPI_EDGE_MAX_COMMS_ERROR_CNT))
    {
      destination_buf = conn_status->hdr.errors[conn_status->hdr.error_count];
      ++(conn_status->hdr.error_count);
    }
    else // Either we're in an unknown array or the warning/error array is alradyat max capacity
    {
      return;
    }
    const int bytes_to_copy = (token->len < NEPI_EDGE_MAX_COMMS_ERROR_STRLENGTH)? token->len : NEPI_EDGE_MAX_COMMS_ERROR_STRLENGTH;
    memcpy(destination_buf, token->ptr, bytes_to_copy);
    destination_buf[bytes_to_copy] = '\0';
  }
  else if (token->type == JSON_TYPE_ARRAY_END) // Must catch this one to update the state
  {
    if (1 == conn_status->hdr.currently_parsing_warnings)
    {
      conn_status->hdr.currently_parsing_warnings = 0;
    }
    else if (1 == conn_status->hdr.currently_parsing_errors)
    {
      conn_status->hdr.currently_parsing_errors = 0;
    }
    else
    {
      return; // Probably just the end of the "connections" array
    }
  }
}

NEPI_EDGE_RET_t NEPI_EDGE_ImportExecStatus(NEPI_EDGE_Exec_Status_t exec_status)
{
  VALIDATE_OPAQUE_TYPE(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status)

  // Exec status comes in two files: lb_exec and hb_exec
  // Parse each individually
  char lb_exec_filename_with_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(lb_exec_filename_with_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s",
           NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_LB_EXEC_STAT_FILE_PATH);

  // Check if the lb exec status file exists, and if so, parse it
  if (access(lb_exec_filename_with_path, F_OK) == 0)
  {
    char *json_string = json_fread(lb_exec_filename_with_path);
    if (NULL == json_string) return NEPI_EDGE_RET_INVALID_FILE_FORMAT;
    //printf("%s\n", json_string); // Debugging
    json_walk(json_string, strlen(json_string), json_walk_exec_lb_status_callback, &p);
    free(json_string); // Must free the frozen-malloc'd string
  }

  char hb_exec_filename_with_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(hb_exec_filename_with_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s",
           NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_HB_EXEC_STAT_FILE_PATH);

  // Check if the hb exec status file exists, and if so, parse it
  if (access(hb_exec_filename_with_path, F_OK) == 0)
  {
    char *json_string = json_fread(hb_exec_filename_with_path);
    if (NULL == json_string) return NEPI_EDGE_RET_INVALID_FILE_FORMAT;
    //printf("%s\n", json_string); // Debugging
    json_walk(json_string, strlen(json_string), json_walk_exec_hb_status_callback, &p);
    free(json_string); // Must free the frozen-malloc'd string
  }

  return NEPI_EDGE_RET_OK;
}
