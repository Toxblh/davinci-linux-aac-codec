#pragma once

namespace IOPlugin
{
    const unsigned int version = (0 << 24) | (0 << 16) | (0 << 8) | 1; // 0.0.0.1

    typedef void* ObjectRef;
    typedef unsigned int MessageID;

    const unsigned char UUID_UnpinnedBuffer[]        = /* 54CFC086-F5E2-11E7-8C3F-9A214CF093AE */ {0x54,0xCF,0xC0,0x86,0x5F,0xE2,0x11,0xE7,0x8C,0x3F,0x9A,0x21,0x4C,0xF0,0x93,0xAE}; // NOLINT
    const unsigned char UUID_PinnedBuffer[]          = /* 6DDF9084-AB93-4A2C-8580-B53E59D6BD15 */ {0x6D,0xDF,0x90,0x84,0xAB,0x93,0x4A,0x2C,0x85,0x80,0xB5,0x3E,0x59,0xD6,0xBD,0x15}; // NOLINT
    const unsigned char UUID_PropertyCollection[]    = /* 35973526-5C1B-4177-AC79-E5CA72E080D3 */ {0x35,0x97,0x35,0x26,0x5C,0x1B,0x41,0x77,0xAC,0x79,0xE5,0xCA,0x72,0xE0,0x80,0xD3}; // NOLINT

    enum LogLevel
    {
        logLevelError       = 0,
        logLevelWarn        = 1,
        logLevelInfo        = 2
    };

    enum StatusCode
    {
        // status
        errNone             = 0,
        errMoreData         = 1,

        // errors
        errFail             = -1,
        errVersion          = -2,
        errInvalidParam     = -3,
        errAlloc            = -4,
        errNoCodec          = -5,
        errNoParam          = -6,
        errUnsupported      = -7,
        errInvalidOperation = -8,
    };

    // common messages
    enum APIMessage
    {
        /* uuids could be Databuffer, VideoBuffer, PropertyCollection in Resolve API, and Plugin(s) uuids in Plugin API */
        msgCreate               = 0x00000001, // params: unsigned char* p_pUUID, ObjectRef* p_ppObj
        msgRetain               = 0x00000002, // params: ObjectRef p_pObj, int* p_NewRefCnt
        msgRelease              = 0x00000003, // params: ObjectRef p_pObj, int* p_NewRefCnt
        msgCodecSettings        = 0x00000004, // params: unsigned char* p_pUUID, [in] ObjectRef p_pPropsObj, [out] ObjectRef p_pList
    };

    // handled by Resolve
    enum ResolveMessage
    {
        msgResolveLog           = 0x00010001, // params: LogLevel logLevel, const char* p_Msg
    };

    // handled by Propery object and a buffer object
    enum PropMessage
    {
        msgPropSet              = 0x00020001, // params: ObjectRef p_pPropsObj, PropertyID p_PropID, PropertyType p_PropType, const void* p_pValue, int p_NumValues
        msgPropGet              = 0x00020002, // params: ObjectRef p_pPropsObj, PropertyID p_PropID, PropertyType* p_pPropType, const void** p_ppValue, int* p_pNumValues
        msgPropClear            = 0x00020003, // params: ObjectRef p_pPropsObj
    };

    enum ListMessage
    {
        msgListAppend           = 0x00030001, // params ObjectRef p_pList, ObjectRef p_pEntry
    };

    // handled by Buffer object
    enum BufferMessage
    {
        msgBufferResize         = 0x00040001, // params: ObjectRef p_pBuf, size_t p_NewSize
        msgBufferLock           = 0x00040002, // params: ObjectRef p_pBuf, unsigned char** p_ppBuf, size_t* p_pBufSize
        msgBufferUnlock         = 0x00040003, // params: ObjectRef p_pBuf
    };

    // handled by the plugin
    enum PluginMessage
    {
        msgPluginStart          = 0x10000001, // params: void
        msgPluginTerminate      = 0x10000002, // params: void
        msgPluginListCodecs     = 0x10000003, // params: ObjectRef p_pList
        msgPluginListContainers = 0x10000004, // params: ObjectRef p_pList
        msgPluginGetInfo        = 0x10000005, // params: ObjectRef p_pProps
    };

    // Codec Object Scope message
    enum CodecMessage
    {
        msgCodecInit            = 0x10010001, // params: ObjectRef p_pCodecObj, ObjectRef p_pInitProps
        msgCodecOpen            = 0x10010002, // params: ObjectRef p_pCodecObj, ObjectRef p_pInitBuffer (buffer with properties, may be of 0 size when no init data is needed)
        msgCodecFlush           = 0x10010003, // params: ObjectRef p_pCodecObj
        msgCodecSetCallback     = 0x10010004, // params: ObjectRef p_pCodecObj, ObjectRef p_pCallback (will be receiving output via msgCodecProcessData, may respond to msgCodecAcceptFrame)
        msgCodecProcessData     = 0x10010005, // params: ObjectRef p_pCodecObj, ObjectRef p_pDataBuffer
        msgCodecAcceptFramePTS  = 0x10010006, // params: ObjectRef p_pCodecObj, int64_t p_PTS, uint8_t* p_IsAccepting
        msgCodecNeedNextPass    = 0x10010007, // params: ObjectRef p_pCodecObj, uint8_t* p_IsAccepting
    };

    enum ContainerMessage
    {
        msgContainerInit        = 0x10020001, // params: ObjectRef p_pContainerObj, ObjectRef p_pPathProps
        msgContainerOpen        = 0x10020002, // params: ObjectRef p_pContainerObj, ObjectRef p_pPathProps
        msgContainerAddTrack    = 0x100200A1, // params: ObjectRef p_pContainerObj, ObjectRef p_pProps, ObjectRef p_pCodecProps, ObjectRef* p_pTrack
        msgContainerClose       = 0x100200F1, // params: ObjectRef p_pContainerObj
    };

    enum ContainerTrackMessage
    {
        /**
         * Encodes/writes data
         *
         * params: ObjectRef p_pTrackObj, ObjectRef p_pBuffer
         */
        msgTrackWrite           = 0x10021001,
    };

#pragma pack(push, 1)
    /* Main structure which initializes the api
     * This structure is filled by Resolve and passed into a plugin
     * version should be checked for compatibility and fail when incompatible
     */
    struct APIContext
    {
        unsigned int version;
        // must return errUnsupported when no such selector
        StatusCode (*pHandleMessage)(MessageID p_MsgID, ...);
    };
#pragma pack(pop)

    // public function signatures exported by the plugin

    /* signature name: pluginInit
     * Called when the plugin is loaded, passes const ResolvePluginAPI value to the plugin to be used
     * Plugin should check version for compatibility
     * Plugin fills the p_pPluginAPI members
     */
    typedef StatusCode (*pluginInitFunc)(const APIContext* p_pResolveAPI, APIContext* p_pPluginAPI);
}
