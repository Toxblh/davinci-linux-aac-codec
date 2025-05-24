#pragma once

#include "wrapper/plugin_api.h"
#include <memory>

using namespace IOPlugin;
class UIAudioSettingsController;

struct AudioEncoderFFmpegContext;

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
    std::unique_ptr<AudioEncoderFFmpegContext> m_ffmpegCtx;
    uint32_t m_outputBitDepth_ = 16; // Желаемый bitDepth (выбранный пользователем)

    // Ringbuffer для PCM (float planar)
    std::vector<std::vector<float>> m_pcmRingBuffer; // [channel][sample]
    size_t m_ringBufferFill = 0;
    size_t m_channels = 0;
    size_t m_frameSize = 0;

    // Ringbuffer helpers
    void AddPCMToRingBuffer(const float** planarPCM, size_t samples);
    bool IsRingBufferFull() const;
    void GetFrameFromRingBuffer(float** out, size_t samples);
    void PadAndFlushRingBuffer();
    void ResetRingBuffer();
};