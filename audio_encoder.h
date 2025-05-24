#pragma once

#include "wrapper/plugin_api.h"

using namespace IOPlugin;
class UIAudioSettingsController;

class AudioEncoder : public IPluginCodecRef
{
public:
    static const uint8_t s_UUID[];

public:
    AudioEncoder();
    ~AudioEncoder();

    static StatusCode s_RegisterCodecs(HostListRef* p_pList);
    static StatusCode s_GetEncoderSettings(HostPropertyCollectionRef* p_pValues, HostListRef* p_pSettingsList);

protected:
    virtual void DoFlush() override;
    virtual StatusCode DoInit(HostPropertyCollectionRef* p_pProps) override;
    virtual StatusCode DoOpen(HostBufferRef* p_pBuff) override;
    virtual StatusCode DoProcess(HostBufferRef* p_pBuff) override;

private:
    std::unique_ptr<UIAudioSettingsController> m_pSettings;
    HostCodecConfigCommon m_CommonProps;
};