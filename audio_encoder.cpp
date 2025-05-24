#include "audio_encoder.h"

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
    containerVec.push_back("ad903d5702f24ac19dde8faca3488051");
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
}

AudioEncoder::~AudioEncoder()
{
}

StatusCode AudioEncoder::DoInit(HostPropertyCollectionRef* p_pProps)
{
    uint32_t bitDepth = 0;
    p_pProps->GetUINT32(pIOPropBitDepth, bitDepth);

    uint8_t isFloat = 0;
    p_pProps->GetUINT8(pIOPropIsFloat, isFloat);

    uint32_t samplingRate = 0;
    p_pProps->GetUINT32(pIOPropSamplingRate, samplingRate);

    uint32_t numChannels = 0;
    p_pProps->GetUINT32(pIOPropNumChannels, numChannels);

    uint32_t trackIdx = 0xFFFFFFFF;
    p_pProps->GetUINT32(pIOPropTrackIdx, trackIdx);

    g_Log(logLevelWarn, "Dummy Audio Plugin :: Init Bit Depth: %d, isFloat: %d, Sampling Rate: %d, Num Channels: %d, Track Index: %d", bitDepth, isFloat, samplingRate, numChannels, trackIdx);

    return errNone;
}

void AudioEncoder::DoFlush()
{
    // Flush codec if needed
    g_Log(logLevelWarn, "Dummy Audio Plugin :: Flush");
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

    g_Log(logLevelWarn, "Dummy Audio Plugin :: Open Bit Depth: %d, isFloat: %d, Sampling Rate: %d, Num Channels: %d, Track Index: %d, Bit Rate: %d", bitDepth, isFloat, samplingRate, numChannels, trackIdx, m_pSettings->GetBitRate());

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
    bool hasOutput = true;
    if (p_pBuff != NULL)
    {
        int64_t pts = -1;
        p_pBuff->GetINT64(pIOPropPTS, pts);

        int64_t duration = 0; // in sampling rate units
        p_pBuff->GetINT64(pIOPropDuration, duration);

        char* pBuf = NULL;
        size_t bufSize = 0;
        if (p_pBuff->LockBuffer(&pBuf, &bufSize))
        {
            // put the encoding code here
            g_Log(logLevelWarn, "Dummy Audio Plugin :: Process buffer PTS: %lld, Duration: %lld, size: %ld", pts, duration, bufSize);
            hasOutput = (pBuf != nullptr) && (bufSize > 0);
            p_pBuff->UnlockBuffer();
        }
        else
        {
            hasOutput = false;
        }
    }
    else
    {
        hasOutput = false;
        g_Log(logLevelWarn, "Dummy Audio Plugin :: Process flush buffer");
    }

    // Passthrough the buffer to the writer to be processed there
    return (hasOutput ? IPluginCodecRef::DoProcess(p_pBuff) : errNone);
}
