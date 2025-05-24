#include "plugin_api.h"

#include <assert.h>

#if defined(__APPLE__)
#include <libkern/OSAtomic.h>
#elif defined(_WIN64)
#include <windows.h>
#endif

static StatusCode s_HandleMessage(MessageID p_MsgID, ...)
{
    va_list args;
    va_start(args, p_MsgID);

    StatusCode err = errUnsupported;
    switch (p_MsgID)
    {
        case msgCreate:
        {
            unsigned char* pUUID = va_arg(args, unsigned char*);
            ObjectRef* ppObj = va_arg(args, ObjectRef*);
            err = g_HandleCreateObj(pUUID, ppObj);
            break;
        }
        case msgRetain:
        case msgRelease:
        {
            IPluginObjRef* pObj = static_cast<IPluginObjRef*>(va_arg(args, ObjectRef));
            int* pNewRef = va_arg(args, int*);
            if ((pObj == NULL) || (pNewRef == NULL))
            {
                err = errInvalidParam;
            }
            else
            {
                *pNewRef = ((p_MsgID == msgRetain) ? pObj->Retain() : pObj->Release());
                err = errNone;
            }
            break;
        }
        case msgPluginStart:
            err = g_HandlePluginStart();
            break;
        case msgPluginTerminate:
            err = g_HandlePluginTerminate();
            break;
        case msgPluginGetInfo:
        {
            HostPropertyCollectionRef props(va_arg(args, ObjectRef));
            if (!props.IsValid())
            {
                err = errInvalidParam;
            }
            else
            {
                err = g_HandleGetInfo(&props);
            }
            break;
        }
        case msgCodecSettings:
        {
            unsigned char* pUUID = va_arg(args, unsigned char*);
            HostPropertyCollectionRef props(va_arg(args, ObjectRef));
            if (!props.IsValid())
            {
                err = errInvalidParam;
            }
            else
            {
                HostListRef listObj(va_arg(args, ObjectRef));
                err = g_GetEncoderSettings(pUUID, &props, &listObj);
            }
            break;
        }
        case msgCodecInit:
        case msgCodecOpen:
        case msgCodecFlush:
        case msgCodecSetCallback:
        case msgCodecProcessData:
        case msgCodecNeedNextPass:
        case msgCodecAcceptFramePTS:
        case msgContainerInit:
        case msgContainerOpen:
        case msgContainerAddTrack:
        case msgTrackWrite:
        case msgContainerClose:
        {
            IPluginObjRef* pObj = static_cast<IPluginObjRef*>(va_arg(args, ObjectRef));
            if (pObj == NULL)
            {
                err = errInvalidParam;
            }
            else
            {
                err = pObj->HandleMessage(p_MsgID, args);
            }
            break;
        }
        case msgPluginListCodecs:
        {
            HostListRef listObj(va_arg(args, ObjectRef));
            err = g_ListCodecs(&listObj);
            break;
        }
        case msgPluginListContainers:
        {
            HostListRef listObj(va_arg(args, ObjectRef));
            err = g_ListContainers(&listObj);
            break;
        }
        default:
            break;
    }

    va_end(args);
    return err;
}

StatusCode pluginInit(const APIContext* p_pHostAPI, APIContext* p_PluginAPI)
{
    p_PluginAPI->version = IOPlugin::version;
    // Check for the minimum version of Host that the plugin will work with.
    if (p_pHostAPI->version < 0x00000001)
    {
        return errVersion;
    }

    SetHostAPI(p_pHostAPI);

    p_PluginAPI->pHandleMessage = s_HandleMessage;

    return errNone;
}

namespace IOPlugin
{
    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// IPluginObjRef
    ///
    ////////////////////////////////////////////////////////////////////////////////
    int32_t IPluginObjRef::Retain()
    {
        return (m_RefCount.fetch_add(1) + 1);
    }

    int32_t IPluginObjRef::Release()
    {
         const int32_t newCount = (m_RefCount.fetch_add(-1) - 1);
         if (newCount == 0)
         {
             delete this;
         }

         return newCount;
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// IPluginCodecRef
    ///
    ////////////////////////////////////////////////////////////////////////////////
    IPluginCodecRef::~IPluginCodecRef()
    {
        if (m_pCallback != NULL)
        {
            delete m_pCallback;
            m_pCallback = NULL;
        }
    }

    StatusCode IPluginCodecRef::SetCallback(ObjectRef p_pCallback)
    {
        assert(m_pCallback == NULL);

        m_pCallback = new HostCodecCallbackRef(p_pCallback);

        return errNone;
    }

    StatusCode IPluginCodecRef::SendOutput(HostBufferRef* p_pBuff)
    {
        if (m_pCallback == NULL)
        {
            return errInvalidParam;
        }

        return GetHostAPI()->pHandleMessage(msgCodecProcessData, m_pCallback, p_pBuff->GetOpaque());
    }

    bool IPluginCodecRef::IsHostAcceptingFrame(int64_t p_PTS)
    {
        if (m_pCallback == NULL)
        {
            return true;
        }

        bool isAccepting = true;
        GetHostAPI()->pHandleMessage(msgCodecAcceptFramePTS, m_pCallback, p_PTS, &isAccepting);

        return isAccepting;
    }

    StatusCode IPluginCodecRef::HandleMessage(MessageID p_MsgID, va_list& p_Args)
    {
        StatusCode err = errUnsupported;
        va_list args;
        va_copy(args, p_Args);
        switch (p_MsgID)
        {
            case msgRetain:
                *va_arg(args, int*) = Retain();
                err = errNone;
                break;
            case msgRelease:
                *va_arg(args, int*) = Release();
                err = errNone;
                break;
            case msgCodecInit:
            {
                HostPropertyCollectionRef props(va_arg(args, ObjectRef));
                if (!props.IsValid())
                {
                    err = errInvalidParam;
                }
                else
                {
                    err = DoInit(&props);
                }
                break;
            }
            case msgCodecOpen:
            {
                HostBufferRef buf(va_arg(args, ObjectRef));
                err = DoOpen(&buf);
                break;
            }
            case msgCodecFlush:
            {
                DoFlush();
                err = errNone;
                break;
            }
            case msgCodecSetCallback:
            {
                err = SetCallback(va_arg(args, ObjectRef));
                break;
            }
            case msgCodecProcessData:
            {
                HostBufferRef buf(va_arg(args, ObjectRef));
                err = DoProcess(&buf);
                break;
            }
            case msgCodecNeedNextPass:
            {
                *va_arg(args, uint8_t*) = IsNeedNextPass() ? 1 : 0;
                err = errNone;
                break;
            }
            case msgCodecAcceptFramePTS:
            {
                const int64_t pts = va_arg(args, int64_t);
                *va_arg(args, uint8_t*) = IsAcceptingFrame(pts) ? 1 : 0;
                err = errNone;
                break;
            }
            default:
                break;
        }

        return err;
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// IPluginContainerRef
    ///
    ////////////////////////////////////////////////////////////////////////////////
    StatusCode IPluginContainerRef::HandleMessage(MessageID p_MsgID, va_list& p_Args)
    {
        StatusCode err = errUnsupported;
        va_list args;
        va_copy(args, p_Args);
        switch (p_MsgID)
        {
            case msgContainerInit:
            {
                HostPropertyCollectionRef props(va_arg(args, ObjectRef));
                err = props.IsValid() ? DoInit(&props) : errInvalidParam;
                break;
            }
            case msgContainerOpen:
            {
                HostPropertyCollectionRef props(va_arg(args, ObjectRef));
                err = props.IsValid() ? DoOpen(&props) : errInvalidParam;
                break;
            }
            case msgContainerClose:
            {
                err = DoClose();
                break;
            }
            case msgContainerAddTrack:
            {
                HostPropertyCollectionRef props(va_arg(args, ObjectRef));
                HostPropertyCollectionRef codecProps(va_arg(args, ObjectRef));
                if (!props.IsValid())
                {
                    err = errInvalidParam;
                }
                else
                {
                    ObjectRef* pOutObj = va_arg(args, ObjectRef*);
                    *pOutObj = NULL;
                    IPluginTrackBase* pTrack = NULL;
                    err = DoAddTrack(&props, &codecProps, &pTrack);
                    if ((err == errNone) && (pTrack != NULL))
                    {
                        IPluginObjRef* pTrackObj = dynamic_cast<IPluginObjRef*>(pTrack);
                        assert(pTrackObj != NULL);
                        *pOutObj = pTrackObj;
                    }
                }
                break;
            }
            default:
                break;
        }

        return err;
    }

    ////////////////////////////////////////////////////////////////////////////////
    ///
    /// IPluginTrackBase
    ///
    ////////////////////////////////////////////////////////////////////////////////
    IPluginTrackBase::IPluginTrackBase(IPluginContainerRef* p_pContainer)
        : m_pContainer(p_pContainer)
    {
        if (m_pContainer != NULL)
        {
            m_pContainer->Retain();
        }
    }

    IPluginTrackBase::~IPluginTrackBase()
    {
        if (m_pContainer != NULL)
        {
            m_pContainer->Release();
            m_pContainer = NULL;
        }
    }

    StatusCode IPluginTrackBase::HandleMessage(MessageID p_MsgID, va_list& p_Args)
    {
        StatusCode err = errUnsupported;
        va_list args;
        va_copy(args, p_Args);
        switch (p_MsgID)
        {
            case msgTrackWrite:
            {
                HostBufferRef buf(va_arg(args, ObjectRef));
                IPluginTrackWriter* pWriter = dynamic_cast<IPluginTrackWriter*>(this);
                err = (pWriter == NULL) ? errUnsupported : pWriter->DoWrite(buf.IsValid() ? &buf : NULL);
                break;
            }
            default:
                break;
        }

        return err;
    }
}
