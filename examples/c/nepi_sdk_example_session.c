#include <stdio.h>

#include "nepi_edge_sdk.h"
#include "nepi_edge_lb_interface.h"

/* Return codes not checked for brevity; you should check them */

int main(int argc, char **argv)
{
  printf("Testing NEPIEdgeSdk (C-language bindings)\n");
  /* First, set the the NEPI-EDGE filesys interface path */
  NEPI_EDGE_SetBotBaseFilePath("./nepi_sdk_example_filesys");

  /* Now create the status */
  NEPI_EDGE_LB_Status_t status;
  NEPI_EDGE_LBStatusCreate(&status, "2020-09-03 17:14:25.2-04:00"); // Timestamp is required; hence included in Create method
  /* Populate some of the optional fields */
  NEPI_EDGE_LBStatusSetNavSatFixTime(status, "2020-08-21T09:49:00.0-04:00"); // Either RFC3339 time/date separator is allowed.
  NEPI_EDGE_LBStatusSetLatitude(status, 47.6062f); // Seattle
  NEPI_EDGE_LBStatusSetLongitude(status, -122.3321f); // Seattle
  NEPI_EDGE_LBStatusSetHeading(status, NEPI_EDGE_HEADING_REF_MAG_NORTH, 45.0f); // North-east
  NEPI_EDGE_LBStatusSetRollAngle(status, 5.5f);
  NEPI_EDGE_LBStatusSetPitchAngle(status, -17.6f); // Diving
  NEPI_EDGE_LBStatusSetTemperature(status, 40.0f); // Balmy!
  NEPI_EDGE_LBStatusSetPowerState(status, 94.2f); // Mostly charged
  const uint8_t device_status[5] = {1,2,3,4,5}; // Will be copied into allocated memory, so this can be a temporary variable
  NEPI_EDGE_LBStatusSetDeviceStatus(status, device_status, 5);

  /* Next create a couple of data snippets */
  NEPI_EDGE_LB_Data_Snippet_t data_snippets[2];
  NEPI_EDGE_LBDataSnippetCreate(&data_snippets[0], "cls", 0); // Type and instance are required fields; hence in the Create method
  /* And populate some of the optional fields */
  NEPI_EDGE_LBDataSnippetSetDataTimestamp(data_snippets[0], "2020-09-03 17:14:25.6-04:00"); // 169ms after the status timestamp
  NEPI_EDGE_LBDataSnippetSetLatitude(data_snippets[0], 47.607f); // Small motion northward from position at status timestamp
  NEPI_EDGE_LBDataSnippetSetLongitude(data_snippets[0], -122.33f); // Small motion eastward from position at status timestamp
  NEPI_EDGE_LBDataSnippetSetHeading(data_snippets[0], 22.5f); // North-north east
  NEPI_EDGE_LBDataSnippetSetRollAngle(data_snippets[0], 6.5f); // Another degree of roll from attitude at status timestamp
  NEPI_EDGE_LBDataSnippetSetPitchAngle(data_snippets[0], -18.2f); // 0.6 degrees more downward pitch
  NEPI_EDGE_LBDataSnippetSetScores(data_snippets[0], 0.33f, 1.0f, 0.5f);
  const char *simulated_file_path = "left_img_detections.txt"; // Will be copied into allocated memory, so this can be a temporary variable
  NEPI_EDGE_LBDataSnippetSetDataFile(data_snippets[0], simulated_file_path);

  /* Now the second data snippet -- empty except for the required type and instance fields and optional data file field */
  NEPI_EDGE_LBDataSnippetCreate(&data_snippets[1], "cls", 1);
  const char *simulated_file_path_2 = "left_img_small.jpeg"; // Will be copied into allocated memory, so this can be a temporary variable
  NEPI_EDGE_LBDataSnippetSetDataFile(data_snippets[1], simulated_file_path_2);

  /* And export the status+data */
  NEPI_EDGE_LBExportData(status, data_snippets, 2);

  /* Now create a general message */
  NEPI_EDGE_LB_General_t general_1;
  NEPI_EDGE_LBGeneralCreate(&general_1);
  /* And populate its sole payload field -- set it up for a string-keyed, floating-point valued payload*/
  const char *general_1_payload_key = "WantSomePI?";
  NEPI_EDGE_LBGeneralSetPayloadStrFloat(general_1, general_1_payload_key, 3.14159f);
  /* And export it to a file for NEPI-BOT to consume */
  NEPI_EDGE_LBExportGeneral(general_1);

  /* Let's do another general message */
  NEPI_EDGE_LB_General_t general_2;
  NEPI_EDGE_LBGeneralCreate(&general_2);
  /* Populate this one with a numerical key and an arbitrary byte array for its value */
  uint8_t byte_array[4] = {0xDE,0xAD,0xBE,0xEF};
  NEPI_EDGE_LBGeneralSetPayloadIntBytes(general_2, 12345, byte_array, 4);
  /* And export it to a file */
  NEPI_EDGE_LBExportGeneral(general_2);

  /* Now we demonstrate import */
  NEPI_EDGE_LB_Config_t config_1;
  NEPI_EDGE_LBConfigCreate(&config_1);
  NEPI_EDGE_LBImportConfig(config_1, "dev_cfg_1.json"); // Will find this in the lb/config folder

  NEPI_EDGE_LB_General_t general_dt_1;
  NEPI_EDGE_LBGeneralCreate(&general_dt_1);
  NEPI_EDGE_LBImportGeneral(general_dt_1, "dev_dt_msg_1.json"); // Will find this in the lb/dt-msg folder

  /* Always destroy what you create */
  NEPI_EDGE_LBStatusDestroy(status);
  NEPI_EDGE_LBDataSnippetDestroy(data_snippets[0]);
  NEPI_EDGE_LBDataSnippetDestroy(data_snippets[1]);
  NEPI_EDGE_LBGeneralDestroy(general_1);
  NEPI_EDGE_LBGeneralDestroy(general_2);
  NEPI_EDGE_LBConfigDestroy(config_1);
  NEPI_EDGE_LBGeneralDestroy(general_dt_1);

  return 0;
}
