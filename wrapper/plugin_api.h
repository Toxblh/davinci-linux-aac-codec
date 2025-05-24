#pragma once

#include <stdarg.h>

#include "host_api.h"

#include <atomic>

#if defined _WIN64 || defined __CYGWIN__
    #ifdef __GNUC__
        #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
        #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
    #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif

using namespace IOPlugin;

extern "C"
{
    DLL_PUBLIC StatusCode pluginInit(const APIContext* p_pHostAPI, APIContext* p_pPluginAPI);
}

// handlers stubs
StatusCode g_HandleCreateObj(unsigned char* p_pUUID, ObjectRef* p_ppObj);
StatusCode g_HandlePluginStart();
StatusCode g_HandlePluginTerminate();
StatusCode g_HandleGetInfo(HostPropertyCollectionRef* p_pBag);
StatusCode g_ListCodecs(HostListRef* p_pList);
StatusCode g_ListContainers(HostListRef* p_pList);
StatusCode g_GetEncoderSettings(unsigned char* p_pUUID, HostPropertyCollectionRef* p_pValues, HostListRef* p_pSettingsList);

namespace IOPlugin
{
    class IPluginObjRef
    {
    public:
        IPluginObjRef()
            : m_RefCount(1)
        {
        }

        virtual int32_t Retain();
        virtual int32_t Release();

        virtual StatusCode HandleMessage(MessageID p_MsgID, va_list& p_Args) = 0;

    protected:
        virtual ~IPluginObjRef()
        {
        }

    private:
        // disable assignment and copy constructor
        IPluginObjRef(const IPluginObjRef& p_Other);
        IPluginObjRef& operator=(const IPluginObjRef& p_Other);

    private:
        std::atomic<int32_t> m_RefCount;
    };

    class IPluginCodecRef : public IPluginObjRef
    {
    public:
        IPluginCodecRef()
            : m_pCallback(NULL)
        {
        }

        virtual StatusCode HandleMessage(MessageID p_MsgID, va_list& p_Args);

        virtual StatusCode SetCallback(ObjectRef p_pCallback);
        virtual StatusCode SendOutput(HostBufferRef* p_pBuff);

        // In temporal decoder check if need to return this data of GOP
        virtual bool IsHostAcceptingFrame(int64_t p_PTS);

        // in Multipass encoding if this frame is needed in the pass
        virtual bool IsAcceptingFrame(int64_t p_PTS)
        {
            return true;
        }

        virtual bool IsNeedNextPass()
        {
            return false;
        }

    protected:
        virtual void DoFlush() = 0;
        virtual StatusCode DoInit(HostPropertyCollectionRef* p_pProps) = 0;
        virtual StatusCode DoOpen(HostBufferRef* p_pBuff) = 0;
        virtual StatusCode DoProcess(HostBufferRef* p_pBuff)
        {
            // default to a passthrough for the codec instance, to pass buffer untouched to container write
            if (!m_pCallback)
            {
                return errInvalidOperation;
            }

            return m_pCallback->SendOutput(p_pBuff);
        }

    protected:
        virtual ~IPluginCodecRef();

    protected:
        HostCodecCallbackRef* m_pCallback;
    };

    class IPluginTrackWriter
    {
    public:
        virtual StatusCode DoWrite(HostBufferRef* p_pBuf) = 0;
    };

    class IPluginContainerRef;
    class IPluginTrackBase : public IPluginObjRef
    {
    public:
        explicit IPluginTrackBase(IPluginContainerRef* p_pContainer);
        virtual StatusCode HandleMessage(MessageID p_MsgID, va_list& p_Args);

    protected:
        virtual ~IPluginTrackBase();

    protected:
        IPluginContainerRef* m_pContainer;
    };

    class IPluginContainerRef : public IPluginObjRef
    {
    public:
        IPluginContainerRef()
        {
        }

        virtual StatusCode HandleMessage(MessageID p_MsgID, va_list& p_Args);

    protected:
        // interface
        virtual StatusCode DoInit(HostPropertyCollectionRef* p_pProps) = 0;
        virtual StatusCode DoOpen(HostPropertyCollectionRef* p_pProps) = 0;
        virtual StatusCode DoAddTrack(HostPropertyCollectionRef* p_pProps, HostPropertyCollectionRef* p_pCodecProps, IPluginTrackBase** p_pTrack) = 0;
        virtual StatusCode DoClose() = 0;

    protected:
        ~IPluginContainerRef()
        {
        }
    };
};

