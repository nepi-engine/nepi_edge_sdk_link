#!/usr/bin/env python3

import os
from nepi_edge_sdk import *

# Helper for config and general message params
def printParam(param_id, param_val):
    id_string = None
    val_string = None

    if (isinstance(param_id, bytes) or isinstance(param_id, bytearray)):
        id_string = param_id.decode('utf-8')
    else:
        id_string = str(param_id)

    print("\t\tID (" + str(type(param_id)) + "): " + id_string)
    if (type(param_val) != list):
        print("\t\tVal (" + str(type(param_val)) + "): " + str(param_val))
    else:
        print("\t\tVal (" + str(type(param_val)) + "): " + str(len(param_val)) + " entries")
        print ('\t\t[{}]'.format(', '.join(hex(x) for x in param_val)))

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
                             power_state_percentage=94.2, device_status=bytearray([1,2,3,4,5]))

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
    general_2.setPayload(12345, bytearray([0xDE, 0xAD, 0xBE, 0xEF]))
    # And export it to a file
    general_2.export()
    print("Created another DO General file")

    # Now we demonstrate import -- First a collection of Config messages */
    cfg_list = NEPIEdgeLBConfig.importAll(sdk)
    print("Imported " + str(len(cfg_list)) + " Config messages")
    for i,cfg_msg in enumerate(cfg_list):
        param_count = cfg_msg.getParamCount()
        print("Config message " + str(i) + " has " + str(param_count) + " parameters")
        for j in range(param_count):
            print("\tParam " + str(j) + ":")
            (param_id, param_val) = cfg_msg.getParam(j)
            printParam(param_id, param_val)

    general_dt_list = NEPIEdgeLBGeneral.importAll(sdk)
    print("Imported " + str(len(general_dt_list)) + " General-DT messages")
    for i, general_dt_msg in enumerate(general_dt_list):
        print("\tGeneral-DT Msg " + str(i) + ' Param:')
        (param_id, param_val) = general_dt_msg.getParam()
        printParam(param_id, param_val)

    # Now some HB testing. Setup data export.
    # First, create a temporary/testing data folder
    test_directory = os.getcwd() + '/testing_data'
    if not os.path.exists(test_directory):
        os.makedirs(test_directory)
    with open(test_directory + '/testfile.txt', 'w') as f:
        f.write('This is a nepi_sdk_example_session testfile -- you can delete it and the parent directory')

    # Now link the test directory to NEPI for HB data export
    sdk.linkHBDataFolder(test_directory)
    print("Linked HB Data Offload folder to " + test_directory)

    # Here is how you actually launch the Bot and check the status afterwards
    # TODO: Launch bot and wait for it to terminate

    # Now import the execution status to get information about how it went
    exec_status = NEPIEdgeExecStatus()
    (lb_statuses, hb_statuses) = exec_status.importStatus()
    print('Imported the BOT Execution Status')

    for i,status in enumerate(lb_statuses):
        print('\tLB Connection Status ' + str(i) + ':')
        print('\t\tComms Type: ' + str(status['comms_type']))
        print('\t\tComms Status: ' + str(status['comms_status']))
        print('\t\tStart Time: ' + str(status['start_time_rfc3339']))
        print('\t\tStop Time: ' + str(status['stop_time_rfc3339']))
        print('\t\tWarnings:')
        for j,warning in enumerate(status['warnings']):
            print('\t\t  ' + str(j) + '. ' + str(warning))
        print('\t\tErrors:')
        for j,error in enumerate(status['errors']):
            print('\t\t  ' + str(j) + '. ' + str(error))
        print('\t\tMessages Sent: ' + str(status['msgs_sent']))
        print('\t\tPackets Sent: ' + str(status['pkts_sent']))
        print('\t\tMessages Received: ' + str(status['msgs_rcvd']))

    for i, status in enumerate(hb_statuses):
          print('\tHB Connection Status ' + str(i) + ':')
          print('\t\tComms Type: ' + str(status['comms_type']))
          print('\t\tComms Status: ' + str(status['comms_status']))
          print('\t\tStart Time: ' + str(status['start_time_rfc3339']))
          print('\t\tStop Time: ' + str(status['stop_time_rfc3339']))
          print('\t\tWarnings:')
          for j,warning in enumerate(status['warnings']):
              print('\t\t  ' + str(j) + '. ' + str(warning))
          print('\t\tErrors:')
          for j,error in enumerate(status['errors']):
              print('\t\t  ' + str(j) + '. ' + str(error))
          print('\t\tDirection: ' + str(status['direction']))
          print('\t\tData Sent: ' + str(status['datasent_kB']) + 'kB')
          print('\t\tData Received: ' + str(status['datareceived_kB']) + 'kB')
