#!/usr/bin/env python3

from nepi_edge_sdk import *

if __name__ == "__main__":
    print("Testing NEPIEdgeSdk (python bindings)")
    sdk = NEPIEdgeSDK()

    # First, set the the NEPI-EDGE filesys interface path.
    sdk.setBotBaseFilePath("./nepi_sdk_example_filesys")

    # Now create the status
    status = NEPIEdgeLBStatus("2020-09-03 17:14:25.2-04:00")
    # Populate some of the optional fields
    status.setOptionalFields(navsat_fix_time_rfc3339="2020-08-21T09:49:00.0-04:00", latitude_deg=47.6062, longitude_deg=-122.3321,
                             heading_ref=NEPI_EDGE_HEADING_REF_MAG_NORTH, heading_deg=45.0,
                             roll_angle_deg=5.5, pitch_angle_deg=-17.6, temperature_c=40.0,
                             power_state_percentage=94.2, device_status=bytes([1,2,3,4,5]))

    # Next create a couple of data snippets
    data_snippet_1 = NEPIEdgeLBDataSnippet(b'cls', 0)
    # And populate some of the optional fields
    data_snippet_1.setOptionalFields(data_timestamp_rfc3339="2020-09-03 17:14:25.6-04:00", latitude_deg=47.607, longitude_deg=-122.33,
                                     heading_deg=22.5, roll_angle_deg=6.5, pitch_angle_deg=-18.2,
                                     quality_score=0.33, type_score=1.0, event_score=0.5, data_file="./example_files/left_img_detections.txt")

    # Now the second data snippet -- empty except for the required type and instance fields and optional data file field
    data_snippet_2 = NEPIEdgeLBDataSnippet(b'cls', 1)
    data_snippet_2.setOptionalFields(data_file="./example_files/left_img_small.jpeg")

    # And export the status+data
    status.export([data_snippet_1, data_snippet_2])
    print("Created status and two associated data snippet files");

    # Now create a general message
    general_1 = NEPIEdgeLBGeneral()
    # And populate its sole payload field -- set it up for a string-keyed, floating-point valued payload
    general_1.setPayload("WantSomePI?", 3.14159)
    # And export it to a file for NEPI-BOT to consume
    general_1.export()
    print("Created a DO General file")

    # Let's do another general message
    general_2 = NEPIEdgeLBGeneral()
    # Populate this one with a numerical key and an arbitrary byte array for its value
    general_2.setPayload(12345, bytes([0xDE, 0xAD, 0xBE, 0xEF]))
    # And export it to a file
    general_2.export()
    print("Created another DO General file")

    # Now we demonstrate import -- First a collection of Config messages */
    cfg_list = NEPIEdgeLBConfig.importAll(sdk)
    print("Imported " + str(len(cfg_list)) + " Config messages")

    general_dt_list = NEPIEdgeLBGeneral.importAll(sdk)
    print("Imported " + str(len(general_dt_list)) + " General messages")
