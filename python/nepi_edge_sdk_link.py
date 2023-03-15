import ctypes
import os

NEPI_EDGE_SDK_LIB_NAME = "libnepi_edge_sdk_link_shared.so"

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

NEPI_EDGE_HEADING_REF_TRUE_NORTH = 0
NEPI_EDGE_HEADING_REF_MAG_NORTH = 1

NEPI_EDGE_LB_PARAM_ID_TYPE_STRING = 0
NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER = 1
NEPI_EDGE_LB_PARAM_ID_TYPE_UNKNOWN = 2

NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL = 0
NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64 = 1
NEPI_EDGE_LB_PARAM_VALUE_TYPE_UINT64 = 2
NEPI_EDGE_LB_PARAM_VALUE_TYPE_FLOAT = 3
NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE = 4
NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING = 5
NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES = 6
NEPI_EDGE_LB_PARAM_VALUE_TYPE_UNKNOWN = 7

NEPI_EDGE_COMMS_STATUS_DISABLED         = 0
NEPI_EDGE_COMMS_STATUS_SUCCESS          = 1
NEPI_EDGE_COMMS_STATUS_CONN_FAILED      = 2
NEPI_EDGE_COMMS_STATUS_UNKNOWN          = 3

NEPI_EDGE_HB_DIRECTION_DO               = 0
NEPI_EDGE_HB_DIRECTION_DT               = 1
NEPI_EDGE_HB_DIRECTION_UNKNOWN          = 2

class NEPIEdgeSDKError(Exception):
    def __init__(self, err_val, message):
        self.err_val = err_val
        self.message = message

class NEPIEdgeLBParamId(ctypes.Union):
    _fields_ = [("id_string", ctypes.c_char_p),
                ("id_number", ctypes.c_uint)]

class NEPIEdgeLBParamBytes(ctypes.Structure):
    _fields_ = [("val", ctypes.POINTER(ctypes.c_uint8)),
              ("length", ctypes.c_size_t)]

class NEPIEdgeLBParamValue(ctypes.Union):
    _fields_ = [("bool_val", ctypes.c_bool),
                ("int64_val", ctypes.c_int64),
                ("uint64_val", ctypes.c_uint64),
                ("float_val", ctypes.c_float),
                ("double_val", ctypes.c_double),
                ("string_val", ctypes.c_char_p),
                ("bytes_val", NEPIEdgeLBParamBytes)]

class NEPIEdgeBase(object):
    c_lib = None

    def __init__(self):
        libname = NEPI_EDGE_SDK_LIB_NAME
        self.c_lib = ctypes.CDLL(libname)

    @staticmethod
    def exceptionIfError(err_val):
        if (err_val == NEPI_EDGE_RET_OK):
            pass
        else:
            print('NEPI Edge SDK Error Occurred: Error Value = ' + str(err_val))
            raise NEPIEdgeSDKError(err_val, "NEPI Edge SDK Error Occurred")

class NEPIEdgeSDK(NEPIEdgeBase):

    def initFunctionPrototypes(self):
        self.c_lib.NEPI_EDGE_SetBotBaseFilePath.argtypes = [ctypes.c_char_p]
        self.c_lib.NEPI_EDGE_GetBotBaseFilePath.restype = ctypes.c_char_p
        self.c_lib.NEPI_EDGE_GetBotNUID.restype = ctypes.c_char_p

        self.c_lib.NEPI_EDGE_StartBot.argtypes = [ctypes.c_ubyte, ctypes.c_uint32, ctypes.c_ubyte, ctypes.c_uint32]
        self.c_lib.NEPI_EDGE_CheckBotRunning.argtypes = [ctypes.POINTER(ctypes.c_ubyte)]
        self.c_lib.NEPI_EDGE_StopBot.argtypes = [ctypes.c_ubyte]

        self.c_lib.NEPI_EDGE_HBLinkDataFolder.argtypes = [ctypes.c_char_p]

    def __init__(self):
        super(NEPIEdgeSDK, self).__init__()
        self.initFunctionPrototypes()

    def setBotBaseFilePath(self, base_path):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_SetBotBaseFilePath(base_path.encode('utf-8')))

    def getBotBaseFilePath(self):
        return self.c_lib.NEPI_EDGE_GetBotBaseFilePath().decode("utf-8")

    def getBotNUID(self):
        return self.c_lib.NEPI_EDGE_GetBotNUID().decode("utf-8")

    def startBot(self, run_lb, lb_timeout_s, run_hb, hb_timeout_s):
        run_lb_ubyte = 1 if run_lb is True else 0
        run_hb_ubyte = 1 if run_hb is True else 0
        self.exceptionIfError(self.c_lib.NEPI_EDGE_StartBot(run_lb_ubyte, lb_timeout_s, run_hb_ubyte, hb_timeout_s))

    def checkBotRunning(self):
        running = ctypes.c_ubyte()
        self.exceptionIfError(self.c_lib.NEPI_EDGE_CheckBotRunning(ctypes.byref(running)))
        ret_val = True if running.value == 1 else False
        return ret_val

    def stopBot(self, force_kill):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_StopBot(force_kill))

    def linkHBDataFolder(self, data_folder_path):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_HBLinkDataFolder(data_folder_path.encode('utf-8')))

    def unlinkHBDataFolder(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_HBUnlinkDataFolder())

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
        super(NEPIEdgeLBStatus, self).__init__()
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
        super(NEPIEdgeLBDataSnippet, self).__init__()
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

class NEPIEdgeLBConfig(NEPIEdgeBase):

    def initFunctionPrototypes(self):
        self.c_lib.NEPI_EDGE_LBConfigCreate.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
        self.c_lib.NEPI_EDGE_LBConfigDestroy.argtypes = [ctypes.c_void_p]
        #self.c_lib.NEPI_EDGE_LBConfigDestroyArray.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_uint]

        self.c_lib.NEPI_EDGE_LBImportConfig.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        #self.c_lib.NEPI_EDGE_LBImportAllConfig.argtypes = [ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p), ctypes.POINTER(ctypes.c_uint]
        self.c_lib.NEPI_EDGE_LBConfigGetParamCount.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint)]
        self.c_lib.NEPI_EDGE_LBConfigGetParam.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                                                          ctypes.POINTER(ctypes.c_int), ctypes.POINTER(NEPIEdgeLBParamId),
                                                          ctypes.POINTER(ctypes.c_int), ctypes.POINTER(NEPIEdgeLBParamValue)]

    def __init__(self):
        super(NEPIEdgeLBConfig, self).__init__()
        self.c_ptr_self = ctypes.c_void_p()

        self.initFunctionPrototypes()

        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBConfigCreate(ctypes.byref(self.c_ptr_self)))

    def __del__(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBConfigDestroy(self.c_ptr_self))

    def importFromFile(self, filename):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBImportConfig(self.c_ptr_self, filename.encode('utf-8')))

    @staticmethod
    def importAll(nepi_edge_sdk_link):
        cfg_path = nepi_edge_sdk_link.getBotBaseFilePath() + "/lb/cfg"

        cfg_instances = list()
        for filename in os.listdir(cfg_path):
            if filename.endswith(".json"):
                new_instance = NEPIEdgeLBConfig()
                new_instance.importFromFile(filename)
                cfg_instances.append(new_instance)
        return cfg_instances

    def getParamCount(self):
        item_count = ctypes.c_uint()
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBConfigGetParamCount(self.c_ptr_self, ctypes.byref(item_count)))
        return item_count.value

    def getParam(self, param_index):
        id_type = ctypes.c_int()
        id = NEPIEdgeLBParamId()
        val_type = ctypes.c_int()
        val = NEPIEdgeLBParamValue()
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBConfigGetParam(self.c_ptr_self, param_index, ctypes.byref(id_type), ctypes.byref(id), ctypes.byref(val_type), ctypes.byref(val)))

        ret_id = None
        if (id_type.value == NEPI_EDGE_LB_PARAM_ID_TYPE_STRING):
            ret_id = id.id_string
        elif (id_type.value == NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER):
            ret_id = id.id_number

        ret_val = None
        if (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL):
            ret_val = val.bool_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64):
            ret_val = val.int64_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_UINT64):
            ret_val = val.uint64_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_FLOAT):
            ret_val = val.float_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE):
            ret_val = val.double_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING):
            ret_val = val.string_val
        elif(val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES):
            ret_val = val.bytes_val.val[:val.bytes_val.length]

        return (ret_id, ret_val)

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
        self.c_lib.NEPI_EDGE_LBGeneralGetParam.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(NEPIEdgeLBParamId),
                                                           ctypes.POINTER(ctypes.c_int), ctypes.POINTER(NEPIEdgeLBParamValue)]

    def __init__(self):
        super(NEPIEdgeLBGeneral, self).__init__()
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
                raise NEPIEdgeSDKError(NEPI_EDGE_RET_BAD_PARAM, "Invalid type for payload parameter (" + str(type(payload)) + ")")
        elif (isinstance(id, int)):
            if (isinstance(payload, bool)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntBool(self.c_ptr_self, id, payload))
            elif (isinstance(payload, int)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntInt64(self.c_ptr_self, id, payload))
            elif (isinstance(payload, float)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntDouble(self.c_ptr_self, id, payload))
            elif (isinstance(payload, str)):
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntStr(self.c_ptr_self, id, payload.encode('utf-8')))
            elif (isinstance(payload, bytearray)):
                payload_array = (ctypes.c_byte * len(payload))(*payload)
                self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralSetPayloadIntBytes(self.c_ptr_self, id, payload_array, len(payload)))
            else:
                raise NEPIEdgeSDKError(NEPI_EDGE_RET_BAD_PARAM, "Invalid type for payload parameter (" + str(type(payload)) + ")")
        else:
            raise NEPIEdgeSDKError(NEPI_EDGE_RET_BAD_PARAM, "Invalid type for id parameter (" + str(type(id)) + ")")

    def export(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBExportGeneral(self.c_ptr_self))

    def importFromFile(self, filename):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBImportGeneral(self.c_ptr_self, filename.encode('utf-8')))

    @staticmethod
    def importAll(nepi_edge_sdk_link):
        general_path = nepi_edge_sdk_link.getBotBaseFilePath() + "/lb/dt-msg"

        general_instances = list()
        for filename in os.listdir(general_path):
            if filename.endswith(".json"):
                new_instance = NEPIEdgeLBGeneral()
                new_instance.importFromFile(filename)
                general_instances.append(new_instance)
        return general_instances

    def getParam(self):
        id_type = ctypes.c_int()
        id = NEPIEdgeLBParamId()
        val_type = ctypes.c_int()
        val = NEPIEdgeLBParamValue()
        self.exceptionIfError(self.c_lib.NEPI_EDGE_LBGeneralGetParam(self.c_ptr_self, ctypes.byref(id_type), ctypes.byref(id), ctypes.byref(val_type), ctypes.byref(val)))

        ret_id = None
        if (id_type.value == NEPI_EDGE_LB_PARAM_ID_TYPE_STRING):
            ret_id = id.id_string
        elif (id_type.value == NEPI_EDGE_LB_PARAM_ID_TYPE_NUMBER):
            ret_id = id.id_number

        ret_val = None
        if (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_BOOL):
            ret_val = val.bool_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_INT64):
            ret_val = val.int64_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_UINT64):
            ret_val = val.uint64_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_FLOAT):
            ret_val = val.float_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_DOUBLE):
            ret_val = val.double_val
        elif (val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_STRING):
            ret_val = val.string_val
        elif(val_type.value == NEPI_EDGE_LB_PARAM_VALUE_TYPE_BYTES):
            ret_val = val.bytes_val.val[:val.bytes_val.length]

        return (ret_id, ret_val)

class NEPIEdgeExecStatus(NEPIEdgeBase):
    def initFunctionPrototypes(self):
        self.c_lib.NEPI_EDGE_ExecStatusCreate.argtypes = [ctypes.POINTER(ctypes.c_void_p)]
        self.c_lib.NEPI_EDGE_ExecStatusDestroy.argtypes = [ctypes.c_void_p]

        self.c_lib.NEPI_EDGE_ImportExecStatus.argtypes = [ctypes.c_void_p]
        self.c_lib.NEPI_EDGE_ExecStatusGetCounts.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t)]
        self.c_lib.NEPI_EDGE_SoftwareWasUpdated.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8)]

        self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsType.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char_p)]
        self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsStatus.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_int)]
        self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsTimestamps.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_char_p)]
        self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsGetWarnErrCount.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t)]
        self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsGetWarning.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char_p)]
        self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsGetError.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char_p)]
        self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsStatistics.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t)]

        self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsType.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char_p)]
        self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsStatus.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_int)]
        self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsTimestamps.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(ctypes.c_char_p)]
        self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsGetWarnErrCount.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t)]
        self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsGetWarning.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char_p)]
        self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsGetError.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, ctypes.POINTER(ctypes.c_char_p)]
        self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsDirection.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_int)]
        self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsStatistics.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(ctypes.c_size_t)]

    def __init__(self):
        super(NEPIEdgeExecStatus, self).__init__()
        self.c_ptr_self = ctypes.c_void_p()

        self.initFunctionPrototypes()

        self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusCreate(ctypes.byref(self.c_ptr_self)))

    def __del__(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusDestroy(self.c_ptr_self))

    def importStatus(self):
        self.exceptionIfError(self.c_lib.NEPI_EDGE_ImportExecStatus(self.c_ptr_self))

        lb_counts = ctypes.c_size_t()
        hb_counts = ctypes.c_size_t()
        self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetCounts(self.c_ptr_self, ctypes.byref(lb_counts), ctypes.byref(hb_counts)))

        lb_statuses = list()
        hb_statuses = list()
        for i in range(lb_counts.value):
            status_dict = {}

            comms_type = ctypes.c_char_p()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsType(self.c_ptr_self, i, ctypes.byref(comms_type)))
            status_dict['comms_type'] = comms_type.value

            comms_status = ctypes.c_int()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsStatus(self.c_ptr_self, i, ctypes.byref(comms_status)))
            status_dict['comms_status'] = comms_status.value

            start_time_rfc3339 = ctypes.c_char_p()
            stop_time_rfc3339 = ctypes.c_char_p()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsTimestamps(self.c_ptr_self, i, ctypes.byref(start_time_rfc3339), ctypes.byref(stop_time_rfc3339)))
            status_dict['start_time_rfc3339'] = start_time_rfc3339.value
            status_dict['stop_time_rfc3339'] = stop_time_rfc3339.value

            warning_count = ctypes.c_size_t()
            error_count = ctypes.c_size_t()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsGetWarnErrCount(self.c_ptr_self, i, ctypes.byref(warning_count), ctypes.byref(error_count)))

            warnings = list()
            for j in range(warning_count.value):
                warning_msg = ctypes.c_char_p()
                self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsGetWarning(self.c_ptr_self, i, j, ctypes.byref(warning_msg)))
                warnings.append(warning_msg.value)
            status_dict['warnings'] = warnings

            errors = list()
            for j in range(error_count.value):
                error_msg = ctypes.c_char_p()
                self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsGetError(self.c_ptr_self, i, j, ctypes.byref(error_msg)))
                errors.append(error_msg.value)
            status_dict['errors'] = errors

            msgs_sent = ctypes.c_size_t()
            pkts_sent = ctypes.c_size_t()
            msgs_rcvd = ctypes.c_size_t()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetLBCommsStatistics(self.c_ptr_self, i, ctypes.byref(msgs_sent), ctypes.byref(pkts_sent), ctypes.byref(msgs_rcvd)))
            status_dict['msgs_sent'] = msgs_sent.value
            status_dict['msgs_rcvd'] = msgs_rcvd.value
            status_dict['pkts_sent'] = pkts_sent.value

            lb_statuses.append(status_dict)

        for i in range(hb_counts.value):
            status_dict = {}

            comms_type = ctypes.c_char_p()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsType(self.c_ptr_self, i, ctypes.byref(comms_type)))
            status_dict['comms_type'] = comms_type.value

            comms_status = ctypes.c_int()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsStatus(self.c_ptr_self, i, ctypes.byref(comms_status)))
            status_dict['comms_status'] = comms_status.value

            start_time_rfc3339 = ctypes.c_char_p()
            stop_time_rfc3339 = ctypes.c_char_p()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsTimestamps(self.c_ptr_self, i, ctypes.byref(start_time_rfc3339), ctypes.byref(stop_time_rfc3339)))
            status_dict['start_time_rfc3339'] = start_time_rfc3339.value
            status_dict['stop_time_rfc3339'] = stop_time_rfc3339.value

            warning_count = ctypes.c_size_t()
            error_count = ctypes.c_size_t()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsGetWarnErrCount(self.c_ptr_self, i, ctypes.byref(warning_count), ctypes.byref(error_count)))
            warnings = list()
            for j in range(warning_count.value):
                warning_msg = ctypes.c_char_p()
                self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsGetWarning(self.c_ptr_self, i, j, ctypes.byref(warning_msg)))
                warnings.append(warning_msg.value)
            status_dict['warnings'] = warnings
            errors = list()
            for j in range(error_count.value):
                error_msg = ctypes.c_char_p()
                self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsGetError(self.c_ptr_self, i, j, ctypes.byref(error_msg)))
                errors.append(error_msg.value)
            status_dict['errors'] = errors

            direction = ctypes.c_int()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsDirection(self.c_ptr_self, i, ctypes.byref(direction)))
            status_dict['direction'] = direction.value

            datasent_kb = ctypes.c_size_t()
            datareceived_kb = ctypes.c_size_t()
            self.exceptionIfError(self.c_lib.NEPI_EDGE_ExecStatusGetHBCommsStatistics(self.c_ptr_self, i, ctypes.byref(datasent_kb), ctypes.byref(datareceived_kb)))
            status_dict['datasent_kB'] = datasent_kb.value
            status_dict['datareceived_kB'] = datareceived_kb.value

            hb_statuses.append(status_dict)

        software_was_updated = ctypes.c_uint8()
        self.exceptionIfError(self.c_lib.NEPI_EDGE_SoftwareWasUpdated(self.c_ptr_self, ctypes.byref(software_was_updated)))
        software_updated = True if (software_was_updated.value == 1) else False

        return lb_statuses, hb_statuses, software_updated
