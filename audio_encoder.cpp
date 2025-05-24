#include "audio_encoder.h"

// FFMpeg includes
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
}

// NOTE: When creating a plugin for release, please generate a new Codec UUID in order to prevent conflicts with other third-party plugins.
const uint8_t AudioEncoder::s_UUID[] = { 0x6a, 0x88, 0xe8, 0x41, 0xd8, 0xe4, 0x41, 0x4b, 0x87, 0x9e, 0xa4, 0x80, 0xfc, 0x90, 0xda, 0xb5 };

class UIAudioSettingsController
{
public:
    UIAudioSettingsController()
    {
        InitDefaults();
    }

    ~UIAudioSettingsController()
    {
    }

    void Load(IPropertyProvider* p_pValues)
    {
        p_pValues->GetINT32("aud_enc_bitrate", m_BitRate);
    }

    StatusCode Render(HostListRef* p_pSettingsList)
    {
        HostUIConfigEntryRef item("aud_enc_bitrate");
        item.MakeSlider("Bit Rate", "kbps", m_BitRate, 128, 512, 128, 128);

        item.SetTriggersUpdate(true);
        if (!item.IsSuccess() || !p_pSettingsList->Append(&item))
        {
            g_Log(logLevelError, "Audio Plugin :: Failed to populate bitrate slider UI entry");
            return errFail;
        }

        return errNone;
    }

    int32_t GetBitRate() const
    {
        return m_BitRate;
    }

private:
    void InitDefaults()
    {
        m_BitRate = 128;
    }

private:
    int32_t m_BitRate;
};

// --- FFMpeg fields for AudioEncoder ---
struct AudioEncoderFFmpegContext {
    AVCodecContext* codecCtx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    int frame_size = 0;
    int64_t pts = 0;
};

StatusCode AudioEncoder::s_RegisterCodecs(HostListRef* p_pList)
{
    // add audio encoder
    HostPropertyCollectionRef codecInfo;
    if (!codecInfo.IsValid())
    {
        return errAlloc;
    }

    codecInfo.SetProperty(pIOPropUUID, propTypeUInt8, AudioEncoder::s_UUID, 16);

    const char* pCodecName = "AAC (FFMpeg Plugin)";
    codecInfo.SetProperty(pIOPropName, propTypeString, pCodecName, strlen(pCodecName));

    uint32_t val = 'aac ';
    codecInfo.SetProperty(pIOPropFourCC, propTypeUInt32, &val, 1);

    val = mediaAudio;
    codecInfo.SetProperty(pIOPropMediaType, propTypeUInt32, &val, 1);

    val = dirEncode;
    codecInfo.SetProperty(pIOPropCodecDirection, propTypeUInt32, &val, 1);

    // if need ieeefloat, set pIOPropIsFloat to 1 with bitdepth 32, supports only single bitdepth option of 32
    std::vector<uint32_t> bitDepths({16, 24});
    codecInfo.SetProperty(pIOPropBitDepth, propTypeUInt32, bitDepths.data(), bitDepths.size());
    codecInfo.SetProperty(pIOPropBitsPerSample, propTypeUInt32, &val, 1);

    // supported sampling rates, or empty
    std::vector<uint32_t> samplingRates({44100, 48000});
    codecInfo.SetProperty(pIOPropSamplingRate, propTypeUInt32, samplingRates.data(), samplingRates.size());

    std::vector<std::string> containerVec;

    containerVec.push_back("mp4");
    containerVec.push_back("mov");
    containerVec.push_back("mkv");
    std::string valStrings;
    for (size_t i = 0; i < containerVec.size(); ++i)
    {
        valStrings.append(containerVec[i]);
        if (i < (containerVec.size() - 1))
        {
            valStrings.append(1, '\0');
        }
    }

    codecInfo.SetProperty(pIOPropContainerList, propTypeString, valStrings.c_str(), valStrings.size());

    if (!p_pList->Append(&codecInfo))
    {
        return errFail;
    }

    return errNone;
}

StatusCode AudioEncoder::s_GetEncoderSettings(HostPropertyCollectionRef* p_pValues, HostListRef* p_pSettingsList)
{
    UIAudioSettingsController settings;
    settings.Load(p_pValues);

    return settings.Render(p_pSettingsList);
}

AudioEncoder::AudioEncoder()
{
    m_ffmpegCtx = nullptr;
}

AudioEncoder::~AudioEncoder()
{
    if (m_ffmpegCtx) {
        if (m_ffmpegCtx->frame) av_frame_free(&m_ffmpegCtx->frame);
        if (m_ffmpegCtx->pkt) av_packet_free(&m_ffmpegCtx->pkt);
        if (m_ffmpegCtx->codecCtx) avcodec_free_context(&m_ffmpegCtx->codecCtx);
        m_ffmpegCtx.reset();
    }
}

StatusCode AudioEncoder::DoInit(HostPropertyCollectionRef* p_pProps)
{
    uint32_t m_outputBitDepth = 0;
    p_pProps->GetUINT32(pIOPropBitDepth, m_outputBitDepth);
    if (m_outputBitDepth != 16 && m_outputBitDepth != 24) {
        g_Log(logLevelError, "AAC Audio Plugin :: Only 16-bit or 24-bit PCM is supported, got %d", m_outputBitDepth);
        return errFail;
    }
    uint8_t isFloat = 0;
    p_pProps->GetUINT8(pIOPropIsFloat, isFloat);
    uint32_t samplingRate = 0;
    p_pProps->GetUINT32(pIOPropSamplingRate, samplingRate);
    uint32_t numChannels = 0;
    p_pProps->GetUINT32(pIOPropNumChannels, numChannels);
    uint32_t trackIdx = 0xFFFFFFFF;
    p_pProps->GetUINT32(pIOPropTrackIdx, trackIdx);
    int bitRate = 128000; // default
    if (m_pSettings) bitRate = m_pSettings->GetBitRate() * 1000;

    // --- FFMpeg AAC encoder init ---
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        g_Log(logLevelError, "AAC encoder not found");
        return errFail;
    }
    m_ffmpegCtx.reset(new AudioEncoderFFmpegContext());
    m_ffmpegCtx->codecCtx = avcodec_alloc_context3(codec);
    if (!m_ffmpegCtx->codecCtx) {
        g_Log(logLevelError, "Could not allocate AVCodecContext");
        return errFail;
    }
    m_ffmpegCtx->codecCtx->bit_rate = bitRate;
    m_ffmpegCtx->codecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP; // AAC prefers planar float
    m_ffmpegCtx->codecCtx->sample_rate = samplingRate;
    av_channel_layout_default(&m_ffmpegCtx->codecCtx->ch_layout, numChannels);
    m_ffmpegCtx->codecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    if (avcodec_open2(m_ffmpegCtx->codecCtx, codec, nullptr) < 0) {
        g_Log(logLevelError, "Could not open AAC encoder");
        return errFail;
    }
    m_ffmpegCtx->frame = av_frame_alloc();
    m_ffmpegCtx->frame->nb_samples = m_ffmpegCtx->codecCtx->frame_size;
    m_ffmpegCtx->frame->format = m_ffmpegCtx->codecCtx->sample_fmt;
    av_channel_layout_copy(&m_ffmpegCtx->frame->ch_layout, &m_ffmpegCtx->codecCtx->ch_layout);
    av_frame_get_buffer(m_ffmpegCtx->frame, 0);
    m_ffmpegCtx->pkt = av_packet_alloc();
    m_ffmpegCtx->frame_size = m_ffmpegCtx->codecCtx->frame_size;
    m_ffmpegCtx->pts = 0;
    g_Log(logLevelWarn, "AAC Audio Plugin :: Init Bit Depth: %d, isFloat: %d, Sampling Rate: %d, Num Channels: %d, Track Index: %d", m_outputBitDepth, isFloat, samplingRate, numChannels, trackIdx);
    // Сохраняем желаемый выходной битдпет для использования в DoProcess
    m_ffmpegCtx->frame_size = m_ffmpegCtx->codecCtx->frame_size;
    m_ffmpegCtx->pts = 0;
    m_outputBitDepth_ = m_outputBitDepth;

    // --- RINGBUFFER INIT ---
    m_channels = numChannels;
    m_frameSize = m_ffmpegCtx->frame_size;
    m_pcmRingBuffer.clear();
    m_pcmRingBuffer.resize(m_channels);
    for (size_t ch = 0; ch < m_channels; ++ch) {
        m_pcmRingBuffer[ch].resize(m_frameSize, 0.0f);
    }
    m_ringBufferFill = 0;

    return errNone;
}

void AudioEncoder::DoFlush()
{
    // Flush FFMpeg encoder
    if (m_ffmpegCtx && m_ffmpegCtx->codecCtx) {
        avcodec_flush_buffers(m_ffmpegCtx->codecCtx);
    }
    g_Log(logLevelWarn, "AAC Audio Plugin :: Flush");
}

StatusCode AudioEncoder::DoOpen(HostBufferRef* p_pBuff)
{
    m_pSettings.reset(new UIAudioSettingsController());
    m_pSettings->Load(p_pBuff);

    // optionally fill bitrate info hint for Resolve
    if (m_pSettings->GetBitRate() > 0)
    {
        const uint64_t bitRate = static_cast<uint64_t>(m_pSettings->GetBitRate()) * 1000;
        StatusCode sts = p_pBuff->SetProperty(pIOPropBitRate, propTypeUInt32, &bitRate, 1);
        if (sts != errNone)
        {
            return sts;
        }
    }

    uint32_t bitDepth = 0;
    p_pBuff->GetUINT32(pIOPropBitDepth, bitDepth);
    uint8_t isFloat = 0;
    p_pBuff->GetUINT8(pIOPropIsFloat, isFloat);
    uint32_t samplingRate = 0;
    p_pBuff->GetUINT32(pIOPropSamplingRate, samplingRate);
    uint32_t numChannels = 0;
    p_pBuff->GetUINT32(pIOPropNumChannels, numChannels);
    uint32_t trackIdx = 0xFFFFFFFF;
    p_pBuff->GetUINT32(pIOPropTrackIdx, trackIdx);
    g_Log(logLevelWarn, "AAC Audio Plugin :: DoOpen params: bitDepth=%u, isFloat=%u, samplingRate=%u, numChannels=%u, trackIdx=%u, bitrate=%d", bitDepth, isFloat, samplingRate, numChannels, trackIdx, m_pSettings->GetBitRate());

    // fill magic cookie here if needed
    std::vector<uint8_t> cookie;
    if (!cookie.empty())
    {
        p_pBuff->SetProperty(pIOPropMagicCookie, propTypeUInt8, &cookie[0], cookie.size());
        uint32_t fourCC = 0;
        p_pBuff->SetProperty(pIOPropMagicCookieType, propTypeUInt32, &fourCC, 1);
    }

    return errNone;
}

StatusCode AudioEncoder::DoProcess(HostBufferRef* p_pBuff)
{
    if (!m_ffmpegCtx || !m_ffmpegCtx->codecCtx) return errFail;
    uint32_t inputBitDepth = 0;
    if (p_pBuff && !p_pBuff->GetUINT32(pIOPropBitDepth, inputBitDepth)) {
        inputBitDepth = m_outputBitDepth_;
    }
    if (inputBitDepth != 16 && inputBitDepth != 24) {
        inputBitDepth = m_outputBitDepth_;
    }
    int numChannels = m_channels;
    int frame_size = m_frameSize;
    if (p_pBuff != NULL)
    {
        char* pBuf = NULL;
        size_t bufSize = 0;
        if (!p_pBuff->LockBuffer(&pBuf, &bufSize) || bufSize == 0)
        {
            return errNone;
        }
        int bytesPerSample = (inputBitDepth == 16) ? 2 : 3;
        int totalSamples = bufSize / (numChannels * bytesPerSample);
        std::vector<std::vector<float>> planarPCM(numChannels, std::vector<float>(totalSamples, 0.0f));
        if (inputBitDepth == 16) {
            int16_t* src = (int16_t*)pBuf;
            for (int i = 0; i < totalSamples; ++i) {
                for (int ch = 0; ch < numChannels; ++ch) {
                    planarPCM[ch][i] = src[i * numChannels + ch] / 32768.0f;
                }
            }
        } else if (inputBitDepth == 24) {
            unsigned char* src = (unsigned char*)pBuf;
            for (int i = 0; i < totalSamples; ++i) {
                for (int ch = 0; ch < numChannels; ++ch) {
                    int idx = (i * numChannels + ch) * 3;
                    int32_t sample = (src[idx + 2] << 24) | (src[idx + 1] << 16) | (src[idx] << 8);
                    sample >>= 8;
                    planarPCM[ch][i] = sample / 8388608.0f;
                }
            }
        }
        int sampleIdx = 0;
        while (sampleIdx < totalSamples) {
            int chunk = std::min((int)(frame_size - m_ringBufferFill), totalSamples - sampleIdx);
            std::vector<const float*> chunkPtrs(numChannels);
            for (int ch = 0; ch < numChannels; ++ch) {
                chunkPtrs[ch] = &planarPCM[ch][sampleIdx];
            }
            AddPCMToRingBuffer(chunkPtrs.data(), chunk);
            if (IsRingBufferFull()) {
                AVFrame* tempFrame = av_frame_alloc();
                tempFrame->format = m_ffmpegCtx->codecCtx->sample_fmt;
                tempFrame->nb_samples = frame_size;
                av_channel_layout_copy(&tempFrame->ch_layout, &m_ffmpegCtx->codecCtx->ch_layout);
                av_frame_get_buffer(tempFrame, 0);
                float** dst = (float**)tempFrame->extended_data;
                GetFrameFromRingBuffer(dst, frame_size);
                tempFrame->pts = m_ffmpegCtx->pts;
                m_ffmpegCtx->pts += frame_size;
                int ret = avcodec_send_frame(m_ffmpegCtx->codecCtx, tempFrame);
                av_frame_free(&tempFrame);
                if (ret < 0) {
                    return errFail;
                }
                do {
                    ret = avcodec_receive_packet(m_ffmpegCtx->codecCtx, m_ffmpegCtx->pkt);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    else if (ret < 0) {
                        break;
                    }
                    HostBufferRef outBuf;
                    outBuf.Resize(m_ffmpegCtx->pkt->size);
                    char* outData = nullptr;
                    size_t outSize = 0;
                    if (outBuf.LockBuffer(&outData, &outSize) && outSize >= (size_t)m_ffmpegCtx->pkt->size) {
                        memcpy(outData, m_ffmpegCtx->pkt->data, m_ffmpegCtx->pkt->size);
                        outBuf.UnlockBuffer();
                        outBuf.SetProperty(pIOPropBitDepth, propTypeUInt32, &m_outputBitDepth_, 1);
                        outBuf.SetProperty(pIOPropSamplingRate, propTypeUInt32, &m_ffmpegCtx->codecCtx->sample_rate, 1);
                        uint32_t numChannels_ = m_ffmpegCtx->codecCtx->ch_layout.nb_channels;
                        outBuf.SetProperty(pIOPropNumChannels, propTypeUInt32, &numChannels_, 1);
                        uint8_t isKey = 0;
                        outBuf.SetProperty(pIOPropIsKeyFrame, propTypeUInt8, &isKey, 1);
                        int64_t pkt_pts = m_ffmpegCtx->pkt->pts;
                        outBuf.SetProperty(pIOPropPTS, propTypeInt64, &pkt_pts, 1);
                        int64_t pkt_dur = m_ffmpegCtx->pkt->duration;
                        outBuf.SetProperty(pIOPropDuration, propTypeInt64, &pkt_dur, 1);
                        IPluginCodecRef::DoProcess(&outBuf);
                    }
                    av_packet_unref(m_ffmpegCtx->pkt);
                } while (ret >= 0);
                ResetRingBuffer();
            }
            sampleIdx += chunk;
        }
        p_pBuff->UnlockBuffer();
    } else {
        if (m_ringBufferFill > 0) {
            PadAndFlushRingBuffer();
            AVFrame* tempFrame = av_frame_alloc();
            tempFrame->format = m_ffmpegCtx->codecCtx->sample_fmt;
            tempFrame->nb_samples = frame_size;
            av_channel_layout_copy(&tempFrame->ch_layout, &m_ffmpegCtx->codecCtx->ch_layout);
            av_frame_get_buffer(tempFrame, 0);
            float** dst = (float**)tempFrame->extended_data;
            GetFrameFromRingBuffer(dst, frame_size);
            tempFrame->pts = m_ffmpegCtx->pts;
            m_ffmpegCtx->pts += frame_size;
            int ret = avcodec_send_frame(m_ffmpegCtx->codecCtx, tempFrame);
            av_frame_free(&tempFrame);
            if (ret < 0) {
                return errFail;
            }
            do {
                ret = avcodec_receive_packet(m_ffmpegCtx->codecCtx, m_ffmpegCtx->pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    break;
                }
                HostBufferRef outBuf;
                outBuf.Resize(m_ffmpegCtx->pkt->size);
                char* outData = nullptr;
                size_t outSize = 0;
                if (outBuf.LockBuffer(&outData, &outSize) && outSize >= (size_t)m_ffmpegCtx->pkt->size) {
                    memcpy(outData, m_ffmpegCtx->pkt->data, m_ffmpegCtx->pkt->size);
                    outBuf.UnlockBuffer();
                    outBuf.SetProperty(pIOPropBitDepth, propTypeUInt32, &m_outputBitDepth_, 1);
                    outBuf.SetProperty(pIOPropSamplingRate, propTypeUInt32, &m_ffmpegCtx->codecCtx->sample_rate, 1);
                    uint32_t numChannels_ = m_ffmpegCtx->codecCtx->ch_layout.nb_channels;
                    outBuf.SetProperty(pIOPropNumChannels, propTypeUInt32, &numChannels_, 1);
                    uint8_t isKey = 0;
                    outBuf.SetProperty(pIOPropIsKeyFrame, propTypeUInt8, &isKey, 1);
                    int64_t pkt_pts = m_ffmpegCtx->pkt->pts;
                    outBuf.SetProperty(pIOPropPTS, propTypeInt64, &pkt_pts, 1);
                    int64_t pkt_dur = m_ffmpegCtx->pkt->duration;
                    outBuf.SetProperty(pIOPropDuration, propTypeInt64, &pkt_dur, 1);
                    IPluginCodecRef::DoProcess(&outBuf);
                }
                av_packet_unref(m_ffmpegCtx->pkt);
            } while (ret >= 0);
            ResetRingBuffer();
        }
        // --- FLUSH ---
        int ret = avcodec_send_frame(m_ffmpegCtx->codecCtx, nullptr);
        if (ret < 0 && ret != AVERROR_EOF) {
            return errFail;
        }
        do {
            ret = avcodec_receive_packet(m_ffmpegCtx->codecCtx, m_ffmpegCtx->pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0) {
                break;
            }
            HostBufferRef outBuf;
            outBuf.Resize(m_ffmpegCtx->pkt->size);
            char* outData = nullptr;
            size_t outSize = 0;
            if (outBuf.LockBuffer(&outData, &outSize) && outSize >= (size_t)m_ffmpegCtx->pkt->size) {
                memcpy(outData, m_ffmpegCtx->pkt->data, m_ffmpegCtx->pkt->size);
                outBuf.UnlockBuffer();
                outBuf.SetProperty(pIOPropBitDepth, propTypeUInt32, &m_outputBitDepth_, 1);
                outBuf.SetProperty(pIOPropSamplingRate, propTypeUInt32, &m_ffmpegCtx->codecCtx->sample_rate, 1);
                uint32_t numChannels_ = m_ffmpegCtx->codecCtx->ch_layout.nb_channels;
                outBuf.SetProperty(pIOPropNumChannels, propTypeUInt32, &numChannels_, 1);
                uint8_t isKey = 0;
                outBuf.SetProperty(pIOPropIsKeyFrame, propTypeUInt8, &isKey, 1);
                int64_t pkt_pts = m_ffmpegCtx->pkt->pts;
                outBuf.SetProperty(pIOPropPTS, propTypeInt64, &pkt_pts, 1);
                int64_t pkt_dur = m_ffmpegCtx->pkt->duration;
                outBuf.SetProperty(pIOPropDuration, propTypeInt64, &pkt_dur, 1);
                IPluginCodecRef::DoProcess(&outBuf);
            }
            av_packet_unref(m_ffmpegCtx->pkt);
        } while (ret >= 0);
    }
    return errNone;
}

// --- RINGBUFFER HELPERS ---
void AudioEncoder::AddPCMToRingBuffer(const float** planarPCM, size_t samples) {
    for (size_t i = 0; i < samples; ++i) {
        for (size_t ch = 0; ch < m_channels; ++ch) {
            if (m_ringBufferFill < m_frameSize)
                m_pcmRingBuffer[ch][m_ringBufferFill] = planarPCM[ch][i];
        }
        m_ringBufferFill++;
    }
}

bool AudioEncoder::IsRingBufferFull() const {
    return m_ringBufferFill >= m_frameSize;
}

void AudioEncoder::GetFrameFromRingBuffer(float** out, size_t samples) {
    for (size_t ch = 0; ch < m_channels; ++ch) {
        memcpy(out[ch], m_pcmRingBuffer[ch].data(), samples * sizeof(float));
    }
}

void AudioEncoder::PadAndFlushRingBuffer() {
    if (m_ringBufferFill == 0) return;
    for (size_t ch = 0; ch < m_channels; ++ch) {
        for (size_t i = m_ringBufferFill; i < m_frameSize; ++i) {
            m_pcmRingBuffer[ch][i] = 0.0f;
        }
    }
}

void AudioEncoder::ResetRingBuffer() {
    m_ringBufferFill = 0;
    for (size_t ch = 0; ch < m_channels; ++ch) {
        std::fill(m_pcmRingBuffer[ch].begin(), m_pcmRingBuffer[ch].end(), 0.0f);
    }
}
