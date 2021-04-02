#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include "nepi_edge_sdk_impl.h"

#include "frozen/frozen.h"

#define NEPI_EDGE_DEVNUID_FILE_PATH     "devinfo/devnuid.txt"
#define NEPI_EDGE_NUID_STRLENGTH    16

#define EXTRACT_LB_CONNECTION_STATUS(x,t,s,i) \
  VALIDATE_OPAQUE_TYPE(x,t,s) \
  struct NEPI_EDGE_LB_Connection_Status *lb_conn_status; \
  NEPI_EDGE_RET_t ret = getLBStatusByIndex(p, &lb_conn_status, (i)); \
  if (ret != NEPI_EDGE_RET_OK) return ret;

#define EXTRACT_HB_CONNECTION_STATUS(x,t,s,i) \
  VALIDATE_OPAQUE_TYPE(x,t,s) \
  struct NEPI_EDGE_HB_Connection_Status *hb_conn_status; \
  NEPI_EDGE_RET_t ret = getHBStatusByIndex(p, &hb_conn_status, (i)); \
  if (ret != NEPI_EDGE_RET_OK) return ret;

static char nepi_edge_bot_base_file_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH] = {'\0'};
static char nepi_edge_bot_nuid[NEPI_EDGE_NUID_STRLENGTH] = {'\0'};

static pid_t bot_pid = -1;

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

  // Get the NUID
  snprintf(tmp_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s", path, NEPI_EDGE_DEVNUID_FILE_PATH);
  FILE *nuid_file = fopen(tmp_path, "r");
  if ((NULL == tmp_path) ||
      (NULL == fgets(nepi_edge_bot_nuid, NEPI_EDGE_NUID_STRLENGTH, nuid_file)))
  {
    return NEPI_EDGE_RET_INVALID_BOT_PATH;
  }
  // Chomp the newline if there is one
  nepi_edge_bot_nuid[strcspn(nepi_edge_bot_nuid, "\n")] = '\0';
  fclose(nuid_file);

  // Everything checks out, so update the global variable and return success
  strncpy(nepi_edge_bot_base_file_path, path, NEPI_EDGE_MAX_FILE_PATH_LENGTH);
  return NEPI_EDGE_RET_OK;
}

const char* NEPI_EDGE_GetBotBaseFilePath(void)
{
  return nepi_edge_bot_base_file_path;
}

const char* NEPI_EDGE_GetBotNUID(void)
{
  return nepi_edge_bot_nuid;
}

NEPI_EDGE_RET_t NEPI_EDGE_StartBot(uint8_t run_lb, uint32_t lb_timeout_s, uint8_t run_hb, uint32_t hb_timeout_s)
{
  uint8_t bot_already_running = 0;
  const NEPI_EDGE_RET_t check_bot_running_ret = NEPI_EDGE_CheckBotRunning(&bot_already_running);
  if (NEPI_EDGE_RET_OK != check_bot_running_ret)
  {
    return check_bot_running_ret;
  }

  if (1 == bot_already_running)
  {
    return NEPI_EDGE_RET_BOT_ALREADY_RUNNING;
  }

  const pid_t fork_ret = fork();
  if (-1 == fork_ret)
  {
    return NEPI_EDGE_RET_CANT_START_BOT;
  }
  else if (0 == fork_ret) // Child process
  {
    //sleep(30); // Just for attaching a debugger to this child process

    // Avoid a race case on bot_pid by delaying here for a moment
    usleep(50000); // 50ms

    char executable_dir[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
    snprintf(executable_dir, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/bin/botmain", nepi_edge_bot_base_file_path);
    if (-1 == chdir(executable_dir))
    {
      return NEPI_EDGE_RET_INVALID_BOT_PATH;
    }

    // Set up the args for execve -- includes command line args and environment
    char *executable_argv[2];
    executable_argv[0] = "botmain"; // just argv[0] -- the application name, last one must be null-terminated
    executable_argv[1] = NULL; // Indicates the end of the array
    char run_lb_link_env_var[32];
    snprintf(run_lb_link_env_var, 32, "RUN_LB_LINK=%u", run_lb);
    char lb_proc_timeout_env_var[32];
    snprintf(lb_proc_timeout_env_var, 32, "LB_PROC_TIMEOUT=%u", lb_timeout_s);
    char run_hb_link_env_var[32];
    snprintf(run_hb_link_env_var, 32, "RUN_HB_LINK=%u", run_hb);
    char hb_proc_timeout_env_var[32];
    snprintf(hb_proc_timeout_env_var, 32, "HB_PROC_TIMEOUT=%u", hb_timeout_s);
    char *executable_env[5] = {run_lb_link_env_var, lb_proc_timeout_env_var, run_hb_link_env_var, hb_proc_timeout_env_var, NULL};

    if (-1 == execve("botmain", executable_argv, executable_env))
    {
      return NEPI_EDGE_RET_CANT_START_BOT; // Nobody will get this return value
    }
    // Otherwise, execve never returns
  }

  // else // Parent process
  // Just set the bot_pid and return
  bot_pid = fork_ret;
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_CheckBotRunning(uint8_t *bot_running)
{
  if (-1 == bot_pid) // not started
  {
    *bot_running = 0;
  }
  else
  {
    int status = 0;
    int waitpid_ret = waitpid(bot_pid, &status, WNOHANG);
    if (-1 == waitpid_ret)
    {
      return NEPI_EDGE_RET_BOT_EXEC_UNDETERMINED;
    }
    else if (0 == waitpid_ret)
    {
      *bot_running = 1;
    }
    else // Must have terminated
    {
      *bot_running = 0;
      // This is the only appropriate place to reset the bot_pid to the not-running sentinel value
      bot_pid = -1;
    }
  }
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_StopBot(uint8_t force_kill)
{
  if (-1 == bot_pid)
  {
    return NEPI_EDGE_RET_BOT_NOT_RUNNING;
  }

  int kill_ret;
  if (0 == force_kill)
  {
    kill_ret = kill(bot_pid, SIGINT);
  }
  else
  {
    kill_ret = kill(bot_pid, SIGKILL);
  }

  if (0 != kill_ret)
  {
    return NEPI_EDGE_RET_CANT_KILL_BOT;
  }
  return NEPI_EDGE_RET_OK;
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
    else // Either we're in an unknown array or the warning/error array is already at max capacity
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

    if (0 == memcmp("do", token->ptr, 2)) conn_status->direction = NEPI_EDGE_HB_DIRECTION_DO;
    //else if (0 == memcmp("dt", token->ptr, 2)) conn_status->direction = NEPI_EDGE_HB_DIRECTION_DT;
    // Currently only 'sw' entry is generated for DT
    else if (0 == memcmp("sw", token->ptr, 2)) conn_status->direction = NEPI_EDGE_HB_DIRECTION_DT;
    else conn_status->direction = NEPI_EDGE_HB_DIRECTION_UNKNOWN;

    conn_status->hdr.fields_set |= NEPI_EDGE_HB_Connection_Status_Fields_Dtype;
  }
  else if (0 == strcmp(tmp_name, "datasent_kB"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_HB_Connection_Status_Fields_Data_Sent);

    conn_status->datasent_kB = strtol(token->ptr, NULL, 10);
    conn_status->hdr.fields_set |= NEPI_EDGE_HB_Connection_Status_Fields_Data_Sent;
  }
  else if (0 == strcmp(tmp_name, "datarecv_kB"))
  {
    detectNewHBConnectionEntry(&conn_status, NEPI_EDGE_HB_Connection_Status_Fields_Data_Received);

    conn_status->datareceived_kB = strtol(token->ptr, NULL, 10);
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

  // Check if the sw update status file exists to inform caller if software has been updated... existence
  // of this file is the indicator
  char sw_status_filename_with_path[NEPI_EDGE_MAX_FILE_PATH_LENGTH];
  snprintf(sw_status_filename_with_path, NEPI_EDGE_MAX_FILE_PATH_LENGTH, "%s/%s",
           NEPI_EDGE_GetBotBaseFilePath(), NEPI_EDGE_SW_UPDATE_STAT_FILE_PATH);
  if (access(sw_status_filename_with_path, F_OK) == 0)
  {
    p->software_updated = 1;
  }
  else
  {
    p->software_updated = 0;
  }

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetCounts(NEPI_EDGE_Exec_Status_t exec_status, size_t *lb_counts, size_t *hb_counts)
{
  VALIDATE_OPAQUE_TYPE(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status)

  struct NEPI_EDGE_LB_Connection_Status *lb_conn_status = p->lb_conn_status;
  *lb_counts = 0;
  while (lb_conn_status != NULL)
  {
    ++(*lb_counts);
    lb_conn_status = lb_conn_status->next;
  }

  struct NEPI_EDGE_HB_Connection_Status *hb_conn_status = p->hb_conn_status;
  *hb_counts = 0;
  while (hb_conn_status != NULL)
  {
    ++(*hb_counts);
    hb_conn_status = hb_conn_status->next;
  }

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_SoftwareWasUpdated(NEPI_EDGE_Exec_Status_t exec_status, uint8_t *software_was_updated)
{
  VALIDATE_OPAQUE_TYPE(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status)

  *software_was_updated = p->software_updated;

  return NEPI_EDGE_RET_OK;
}

static NEPI_EDGE_RET_t getLBStatusByIndex(struct NEPI_EDGE_Exec_Status *exec_status, struct NEPI_EDGE_LB_Connection_Status **lb_conn_status, size_t lb_index)
{
  *lb_conn_status = exec_status->lb_conn_status;
  if (lb_conn_status == NULL) return NEPI_EDGE_RET_ARG_OUT_OF_RANGE;

  for (size_t i = 0; i < lb_index; ++i)
  {
    if ((*lb_conn_status)->next != NULL)
    {
      *lb_conn_status = (*lb_conn_status)->next;
    }
    else
    {
      return NEPI_EDGE_RET_ARG_OUT_OF_RANGE;
    }
  }
  return NEPI_EDGE_RET_OK;
}

static void getCommsType(struct NEPI_EDGE_Connection_Status_Common_Hdr *hdr, char **comms_type)
{
  if (0 == (hdr->fields_set & NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Comms_Type))
  {
    *comms_type = NULL;
  }
  else
  {
    *comms_type = hdr->comms_type;
  }
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsType(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, char **comms_type)
{
  EXTRACT_LB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, lb_index)

  getCommsType(&(lb_conn_status->hdr), comms_type);
  return NEPI_EDGE_RET_OK;
}

static void getCommsStatus(struct NEPI_EDGE_Connection_Status_Common_Hdr *hdr, NEPI_EDGE_COMMS_STATUS_t *comms_status)
{
  if (0 == (hdr->fields_set & NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Status))
  {
    *comms_status = NEPI_EDGE_COMMS_STATUS_UNKNOWN;
  }
  else
  {
    *comms_status = hdr->status;
  }
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsStatus(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, NEPI_EDGE_COMMS_STATUS_t *comms_status)
{
  EXTRACT_LB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, lb_index)

  getCommsStatus(&(lb_conn_status->hdr), comms_status);
  return NEPI_EDGE_RET_OK;
}

static void getCommsTimestamps(struct NEPI_EDGE_Connection_Status_Common_Hdr *hdr, char **start_time, char **stop_time)
{
  if (0 == (hdr->fields_set & NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Start_Time))
  {
    *start_time = NULL;
  }
  else
  {
    *start_time = hdr->start_time_rfc3339;
  }

  if (0 == (hdr->fields_set & NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Stop_Time))
  {
    *stop_time = NULL;
  }
  else
  {
    *stop_time = hdr->stop_time_rfc3339;
  }
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsTimestamps(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index,
                                                         char **start_time, char **stop_time)
{
  EXTRACT_LB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, lb_index)

  getCommsTimestamps(&(lb_conn_status->hdr), start_time, stop_time);
  return NEPI_EDGE_RET_OK;
}

static void getWarnErrCount(struct NEPI_EDGE_Connection_Status_Common_Hdr *hdr, size_t *warning_count, size_t *error_count)
{
  if (0 == (hdr->fields_set & NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Warnings))
  {
    *warning_count = 0;
  }
  else
  {
    *warning_count = hdr->warning_count;
  }

  if (0 == (hdr->fields_set & NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Errors))
  {
    *error_count = 0;
  }
  else
  {
    *error_count = hdr->error_count;
  }
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsGetWarnErrCount(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, size_t *warning_count, size_t *error_count)
{
  EXTRACT_LB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, lb_index)

  getWarnErrCount(&(lb_conn_status->hdr), warning_count, error_count);
  return NEPI_EDGE_RET_OK;
}

static NEPI_EDGE_RET_t getWarning(struct NEPI_EDGE_Connection_Status_Common_Hdr *hdr, size_t warning_index, char **warning_msg)
{
  if ((0 == (hdr->fields_set & NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Warnings)) ||
      (warning_index >= hdr->warning_count))
  {
    return NEPI_EDGE_RET_ARG_OUT_OF_RANGE;
  }

  *warning_msg = hdr->warnings[warning_index];
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsGetWarning(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, size_t warning_index, char **warning_msg)
{
  EXTRACT_LB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, lb_index)

  return getWarning(&(lb_conn_status->hdr), warning_index, warning_msg);
}

static NEPI_EDGE_RET_t getError(struct NEPI_EDGE_Connection_Status_Common_Hdr *hdr, size_t error_index, char **error_msg)
{
  if ((0 == (hdr->fields_set & NEPI_EDGE_Connection_Status_Common_Hdr_Fields_Errors)) ||
      (error_index >= hdr->error_count))
  {
    return NEPI_EDGE_RET_ARG_OUT_OF_RANGE;
  }

  *error_msg = hdr->errors[error_index];
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsGetError(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, size_t error_index, char **error_msg)
{
  EXTRACT_LB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, lb_index)

  return getError(&(lb_conn_status->hdr), error_index, error_msg);
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetLBCommsStatistics(NEPI_EDGE_Exec_Status_t exec_status, size_t lb_index, size_t *msgs_sent, size_t *pkts_sent, size_t *msgs_rcvd)
{
  EXTRACT_LB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, lb_index)

  // Initialize all to zero
  *msgs_sent = 0;
  *pkts_sent = 0;
  *msgs_rcvd = 0;

  if (0 != (lb_conn_status->hdr.fields_set & NEPI_EDGE_LB_Connection_Status_Fields_Messages_Sent))
  {
    *msgs_sent = lb_conn_status->messages_sent;
  }
  if (0 != (lb_conn_status->hdr.fields_set & NEPI_EDGE_LB_Connection_Status_Fields_Packets_Sent))
  {
    *pkts_sent = lb_conn_status->packets_sent;
  }
  if (0 != (lb_conn_status->hdr.fields_set & NEPI_EDGE_LB_Connection_Status_Fields_Messages_Received))
  {
    *msgs_rcvd = lb_conn_status->messages_received;
  }

  return NEPI_EDGE_RET_OK;
}

static NEPI_EDGE_RET_t getHBStatusByIndex(struct NEPI_EDGE_Exec_Status *exec_status, struct NEPI_EDGE_HB_Connection_Status **hb_conn_status, size_t hb_index)
{
  *hb_conn_status = exec_status->hb_conn_status;
  if (hb_conn_status == NULL) return NEPI_EDGE_RET_ARG_OUT_OF_RANGE;

  for (size_t i = 0; i < hb_index; ++i)
  {
    if ((*hb_conn_status)->next != NULL)
    {
      *hb_conn_status = (*hb_conn_status)->next;
    }
    else
    {
      return NEPI_EDGE_RET_ARG_OUT_OF_RANGE;
    }
  }
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsType(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, char **comms_type)
{
  EXTRACT_HB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, hb_index)

  getCommsType(&(hb_conn_status->hdr), comms_type);
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsStatus(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, NEPI_EDGE_COMMS_STATUS_t *comms_status)
{
  EXTRACT_HB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, hb_index)

  getCommsStatus(&(hb_conn_status->hdr), comms_status);
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsTimestamps(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index,
                                                         char **start_time, char **stop_time)
{
  EXTRACT_HB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, hb_index)

  getCommsTimestamps(&(hb_conn_status->hdr), start_time, stop_time);
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsGetWarnErrCount(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, size_t *warning_count, size_t *error_count)
{
  EXTRACT_HB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, hb_index)

  getWarnErrCount(&(hb_conn_status->hdr), warning_count, error_count);
  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsGetWarning(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, size_t warning_index, char **warning_msg)
{
  EXTRACT_HB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, hb_index)

  return getWarning(&(hb_conn_status->hdr), warning_index, warning_msg);
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsGetError(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, size_t error_index, char **error_msg)
{
  EXTRACT_HB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, hb_index)

  return getError(&(hb_conn_status->hdr), error_index, error_msg);
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsStatistics(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, size_t *datasent_kB, size_t *datareceived_kB)
{
  EXTRACT_HB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, hb_index)

  *datasent_kB = 0;
  *datareceived_kB = 0;

  if (0 != (hb_conn_status->hdr.fields_set & NEPI_EDGE_HB_Connection_Status_Fields_Data_Sent))
  {
    *datasent_kB = hb_conn_status->datasent_kB;
  }

  if (0 != (hb_conn_status->hdr.fields_set & NEPI_EDGE_HB_Connection_Status_Fields_Data_Received))
  {
    *datareceived_kB = hb_conn_status->datareceived_kB;
  }

  return NEPI_EDGE_RET_OK;
}

NEPI_EDGE_RET_t NEPI_EDGE_ExecStatusGetHBCommsDirection(NEPI_EDGE_Exec_Status_t exec_status, size_t hb_index, NEPI_EDGE_HB_DIRECTION_t *direction)
{
  EXTRACT_HB_CONNECTION_STATUS(exec_status, NEPI_EDGE_OPAQUE_TYPE_ID_EXEC_STATUS, NEPI_EDGE_Exec_Status, hb_index)

  if (0 != (hb_conn_status->hdr.fields_set & NEPI_EDGE_HB_Connection_Status_Fields_Dtype))
  {
    *direction = hb_conn_status->direction;
  }

  return NEPI_EDGE_RET_OK;
}
