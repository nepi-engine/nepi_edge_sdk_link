import ctypes
import pathlib
import errno

NEPI_EDGE_SDK_LIB_NAME = "libnepi_edge_sdk_shared.so"

# Copied in from nepi_edge_errors.h
NEPI_EDGE_RET_OK = 0
NEPI_EDGE_RET_UNINIT_OBJ = -1
NEPI_EDGE_RET_MALLOC_ERR = -2
NEPI_EDGE_RET_WRONG_OBJ_TYPE = -3
NEPI_EDGE_RET_ARG_OUT_OF_RANGE = -4
NEPI_EDGE_RET_ARG_TOO_LONG = -5
NEPI_EDGE_RET_FILE_OPEN_ERR = -6
NEPI_EDGE_RET_INCOMPLETE_OBJ = -7
NEPI_EDGE_RET_REQUIRED_FIELD_MISSING = -8
NEPI_EDGE_RET_FILE_PERMISSION_ERR = -9
NEPI_EDGE_RET_FILE_MISSING = -10
NEPI_EDGE_RET_INVALID_FILE_FORMAT = -11
NEPI_EDGE_RET_BAD_PARAM = -12

NEPI_EDGE_HEADING_REF_TRUE_NORTH = 0,
NEPI_EDGE_HEADING_REF_MAG_NORTH = 1

class NEPIEdgeSDKError(Exception):
    def __init__(self, err_val, message):
        self.err_val = err_val
        self.message = message


class NEPIEdgeBase:
    c_lib = None

    def __init__(self):
        libname = NEPI_EDGE_SDK_LIB_NAME
        self.c_lib = ctypes.CDLL(libname)

    @staticmethod
    def exceptionIfError(err_val):
        if (err_val == NEPI_EDGE_RET_OK):
            pass
        else:
            raise NEPIEdgeSDKError(err_val, "NEPI Edge SDK Error Occurred")

class NEPIEdgeSDK(NEPIEdgeBase):

    def initFunctionPrototypes(self):
        self.c_lib.NEPI_EDGE_SetBotBaseFilePath.argtypes = [ctypes.c_char_p]

        self.c_lib.NEPI_EDGE_GetBotBaseFilePath.restype = ctypes.c_char_p

    def __init__(self):
        super().__init__()
        self.initFunctionPrototypes()

    def setBotBaseFilePath(self, base_path):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_SetBotBaseFilePath(base_path.encode('utf-8')))

    def getBotBaseFilePath(self):
        return self.c_lib.NEPI_EDGE_GetBotBaseFilePath().decode("utf-8")

class NEPIEdgeLBStatus(NEPIEdgeBase):

    def initFunctionPrototypes(self):
        self.c_lib.NEPI_EDGE_LBStatusCreate.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_char_p]
        self.c_lib.NEPI_EDGE_LBStatusDestroy.argtypes = [ctypes.c_void_p]

        self.c_lib.NEPI_EDGE_LBStatusSetNavSatFixTime.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        self.c_lib.NEPI_EDGE_LBStatusSetLatitude.argtypes = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBStatusSetLongitude.argtypes = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBStatusSetHeading.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBStatusSetRollAngle.argtypes = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBStatusSetPitchAngle.argtypes = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBStatusSetTemperature.argtypes = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBStatusSetPowerState.argtypes = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBStatusSetDeviceStatus.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_byte), ctypes.c_uint]

        self.c_lib.NEPI_EDGE_LBExportData.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_void_p), ctypes.c_uint]

    def __init__(self, timestamp_rfc3339):
        super().__init__()
        self.c_ptr_self = ctypes.c_void_p()

        self.initFunctionPrototypes()
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusCreate(ctypes.byref(self.c_ptr_self), timestamp_rfc3339.encode('utf-8')))

    def __del__(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusDestroy(self.c_ptr_self))

    def setOptionalFields(self, navsat_fix_time_rfc3339=None, latitude_deg=None, longitude_deg=None, heading_ref=None, heading_deg=None,
                          roll_angle_deg=None, pitch_angle_deg=None, temperature_c=None, power_state_percentage=None, device_status=None):
        if (navsat_fix_time_rfc3339 is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetNavSatFixTime(self.c_ptr_self, navsat_fix_time_rfc3339.encode('utf-8')))
        if (latitude_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetLatitude(self.c_ptr_self, latitude_deg))
        if (longitude_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetLongitude(self.c_ptr_self, longitude_deg))
        if (heading_ref is not None and heading_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetHeading(self.c_ptr_self, heading_ref, heading_deg))
        elif (heading_ref is not None or heading_deg is not None):
            raise NEPIEdgeSDKError(NEPI_EDGE_RET_REQUIRED_FIELD_MISSING, "Heading and heading ref must both be defined or neither")
        if (roll_angle_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetRollAngle(self.c_ptr_self, roll_angle_deg))
        if (pitch_angle_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetPitchAngle(self.c_ptr_self, pitch_angle_deg))
        if (temperature_c is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetTemperature(self.c_ptr_self, temperature_c))
        if (power_state_percentage is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetPowerState(self.c_ptr_self, power_state_percentage))
        if (device_status is not None):
            device_status_array = (ctypes.c_byte * len(device_status))(*device_status)
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetDeviceStatus(self.c_ptr_self, device_status_array, len(device_status)))
            #self.exceptionIfError(self.c_lib.NEPI_EDGE_LBStatusSetDeviceStatus(self.c_ptr_self, device_status, len(device_status)))

    def export(self, data_snippets):
        data_snippets_c_ptrs_list = [snippet.c_ptr_self for snippet in data_snippets]
        data_snippets_c_ptrs_array = (ctypes.c_void_p * len(data_snippets_c_ptrs_list))(*data_snippets_c_ptrs_list)
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBExportData(self.c_ptr_self, data_snippets_c_ptrs_array, len(data_snippets_c_ptrs_list)))

class NEPIEdgeLBDataSnippet(NEPIEdgeBase):

    def initFunctionPrototypes(self):
        self.c_lib.NEPI_EDGE_LBDataSnippetCreate.argtype = [ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_char), ctypes.c_uint32]
        self.c_lib.NEPI_EDGE_LBDataSnippetDestroy.argtypes = [ctypes.c_void_p]

        self.c_lib.NEPI_EDGE_LBDataSnippetSetDataTimestamp.argtype = [ctypes.c_void_p, ctypes.c_char_p]
        self.c_lib.NEPI_EDGE_LBDataSnippetSetLatitude.argtype = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBDataSnippetSetLongitude.argtype = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBDataSnippetSetHeading.argtype = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBDataSnippetSetRollAngle.argtype = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBDataSnippetSetPitchAngle.argtype = [ctypes.c_void_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBDataSnippetSetScores.argtype = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBDataSnippetSetDataFile.argtype = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_ubyte]

    def __init__(self, type, instance):
        super().__init__()
        self.c_ptr_self = ctypes.c_void_p()

        self.initFunctionPrototypes()

        type_array = (ctypes.c_char * 3)(*type)
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetCreate(ctypes.byref(self.c_ptr_self), type, instance))

    def __del__(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetDestroy(self.c_ptr_self))

    def setOptionalFields(self, data_timestamp_rfc3339=None, latitude_deg=None, longitude_deg=None, heading_deg=None,
                          roll_angle_deg=None, pitch_angle_deg=None, quality_score=None, type_score=None, event_score=None,
                          data_file=None, delete_data_file_after_export=False):
        if (data_timestamp_rfc3339 is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetSetDataTimestamp(self.c_ptr_self, data_timestamp_rfc3339.encode('utf-8')))
        if (latitude_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetSetLatitude(self.c_ptr_self, ctypes.c_float(latitude_deg)))
        if (longitude_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetSetLongitude(self.c_ptr_self, ctypes.c_float(longitude_deg)))
        if (heading_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetSetHeading(self.c_ptr_self, ctypes.c_float(heading_deg)))
        if (roll_angle_deg is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetSetRollAngle(self.c_ptr_self, ctypes.c_float(roll_angle_deg)))
        if (pitch_angle_deg):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetSetPitchAngle(self.c_ptr_self, ctypes.c_float(pitch_angle_deg)))
        if (quality_score is not None and type_score  is not None and event_score  is not None):
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetSetScores(self.c_ptr_self, ctypes.c_float(quality_score),
                                  ctypes.c_float(type_score), ctypes.c_float(event_score)))
        elif (quality_score is not None or type_score is not None or event_score is not None):
            raise NEPIEdgeSDKError(NEPI_EDGE_RET_REQUIRED_FIELD_MISSING, "If any data snippet score component is set, they must all be set")
        if (data_file is not None):
            delete_flag = 1 if (delete_data_file_after_export is True) else 0
            self.exceptionIfError(self.c_lib.NEPI_EDGE_LBDataSnippetSetDataFile(self.c_ptr_self, data_file.encode('utf-8'), delete_flag))

class NEPIEdgeLBGeneral(NEPIEdgeBase):

    def initFunctionPrototypes(self):
        self.c_lib.NEPI_EDGE_LBGeneralCreate.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
        self.c_lib.NEPI_EDGE_LBGeneralDestroy.argtypes = [ctypes.c_void_p]

        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrBool.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_ubyte]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrInt64.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_longlong]
        #self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrUInt64.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_ulonglong]
        #self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrFloat.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrDouble.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_double]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrStr.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrBytes.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(ctypes.c_byte), ctypes.c_uint]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntBool.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_ubyte]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntInt64.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_longlong]
        #self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntUInt64.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_ulonglong]
        #self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntFloat.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_float]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntDouble.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_double]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntStr.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_char_p]
        self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntBytes.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_byte), ctypes.c_uint]

        self.c_lib.NEPI_EDGE_LBExportGeneral.argtypes = [ctypes.c_void_p]

        self.c_lib.NEPI_EDGE_LBImportGeneral.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

    def __init__(self):
        super().__init__()
        self.c_ptr_self = ctypes.c_void_p()

        self.initFunctionPrototypes()

        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralCreate(ctypes.byref(self.c_ptr_self)))

    def __del__(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralDestroy(self.c_ptr_self))

    def setPayload(self, id, payload):
        if (isinstance(id, str)):
            if (isinstance(payload, bool)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrBool(self.c_ptr_self, id.encode('utf-8'), payload))
            elif (isinstance(payload, int)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrInt64(self.c_ptr_self, id.encode('utf-8'), payload))
            elif (isinstance(payload, float)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrDouble(self.c_ptr_self, id.encode('utf-8'), payload))
            elif (isinstance(payload, str)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrStr(self.c_ptr_self, id.encode('utf-8'), payload.encode('utf-8')))
            elif (isinstance(payload, bytes)):
                payload_array = (ctypes.c_byte * len(payload))(*payload)
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadStrBytes(self.c_ptr_self, id.encode('utf-8'), payload_array, len(payload)))
            else:
                raise NEPIEdgeSDKError(NEPI_EDGE_RET_BAD_PARAM, "Invalid type for payload parameter (" + type(payload) + ")")
        elif (isinstance(id, int)):
            if (isinstance(payload, bool)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntBool(self.c_ptr_self, id, payload))
            elif (isinstance(payload, int)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntInt64(self.c_ptr_self, id, payload))
            elif (isinstance(payload, float)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntDouble(self.c_ptr_self, id, payload))
            elif (isinstance(payload, str)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntStr(self.c_ptr_self, id, payload.encode('utf-8')))
            elif (isinstance(payload, bytes)):
                payload_array = (ctypes.c_byte * len(payload))(*payload)
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntBytes(self.c_ptr_self, id, payload_array, len(payload)))
            else:
                raise NEPIEdgeSDKError(NEPI_EDGE_RET_BAD_PARAM, "Invalid type for payload parameter (" + type(payload) + ")")
        else:
            raise NEPIEdgeSDKError(NEPI_EDGE_RET_BAD_PARAM, "Invalid type for id parameter (" + type(id) + ")")

    def export(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBExportGeneral(self.c_ptr_self))

    def importFromFile(self, filename):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBImportGeneral(self.c_ptr_self, filename.encode('utf-8')))
