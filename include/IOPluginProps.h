#pragma once

namespace IOPlugin
{
    typedef const char* PropertyID;

    enum PropertyType
    {
        propTypeNull = 0,
        propTypeInt8,
        propTypeUInt8,
        propTypeInt16,
        propTypeUInt16,
        propTypeInt32,
        propTypeUInt32,
        propTypeInt64,
        propTypeUInt64,
        propTypeDouble,
        propTypeString, // char utf-8 string
    };

    enum UIEntryType
    {
        uiEntryInvalid = 0,
        uiEntrySeparator,
        uiEntryLabel,
        uiEntrySlider,
        uiEntryButton,
        uiEntryCheckbox,
        uiEntryCombobox,
        uiEntryRadiobox,
        uiEntryTextbox,
        uiEntryMarkers,
    };

    enum ComponentOrder
    {
        clrUnknown = 0,
        clrRGB, // Interleaved RGB (8, 16 bit)
        clrRGBp, // Planar RGB (8, 16 bit)
        clrRGBA, // Interleaved RGBA (8, 16 bit)
        clrRGBAp, // Planar RGBA (8, 16 bit)
        clrV210, // Packed v210 YUV 10 bit
        clrR210, // Packed r210
        clrYUVp, /* Planar YUV,
                  * must be set along with hSubsampling and vSubsampling properties:
                  * 420 (8, 16 bit)
                  * 422 (8, 16 bit)
                  * 411 (8 bit)
                  * 444 (8, 10, 16 bit)
                  */
        clrUYVY, // Interleaved 422 YUV (8, 16 bit)
        clrNV12, // Planar 420 YUV, Y plane with interleaved U + V plane (8, 10, 16 bit)
        clrUYAVYA, // Interleaved 422 YUV with Alpha (8, 16 bit)
        clrAYUV, // Interleaved YUV with Alpha channel. Supported subsampling: 444 (8, 16 bit)
    };

    enum MediaType
    {
        mediaNone = 0,
        mediaAudio = 1,
        mediaVideo = 2,
    };

    enum CodecDirection
    {
        dirNone = 0,
        dirDecode = 1,
        dirEncode = 2
    };

    enum FieldOrder
    {
        fieldNone = 0,
        fieldProgressive = (1 << 0),
        fieldTop = (1 << 1),
        fieldBottom = (1 << 2),
    };

    enum AudioChannelLayout
    {
        audLayoutNone       = 0,
        audLayoutMono       = 1,
        audLayoutStereo     = 2,
        audLayoutGeneric5_1 = 3,
        audLayoutGeneric7_1 = 4,
    };

    static PropertyID pIOPropUUID = "uuid"; // 16 uint8_t
    static PropertyID pIOPropFourCC = "4cc"; // uint32_t
    static PropertyID pIOPropName = "name"; // String for short codec name, i.e. MPEG2, etc
    static PropertyID pIOPropGroup = "group"; // String for short codec group name, if missing will be put to "Plugins"
    static PropertyID pIOPropThreadSafe = "isThreadSafe"; // uint8_t for boolean if codec is thread safe and can be used concurrently and out of frame order
    static PropertyID pIOPropReel = "reel"; // String for reel ID
    static PropertyID pIOPropMagicCookie = "magicCookie"; // uint8 array
    static PropertyID pIOPropMagicCookieType = "magicCookieType"; // uint32_t fourCC ('avcC', 'esds', 'anxb' etc)
    static PropertyID pIOPropBitDepth = "bitDepth"; // uint32_t number of meaningful bits
    static PropertyID pIOPropIsFloat = "isFloat"; // uint8_t 1 - if audio is floating point
    static PropertyID pIOPropBitsPerSample = "sampleBits"; // uint32_t number of bits per sample
    static PropertyID pIOPropByteOrder = "byteOrder"; // uint8_t 0/absent - little-endian, 1 - big endian
    static PropertyID pIOPropPTS = "pts"; // int64_t
    static PropertyID pIOPropDTS = "dts"; // int64_t
    static PropertyID pIOPropDuration = "duration"; // int64 - duration in time base
    static PropertyID pIOPropTimeBase = "timeBase"; // two uint32 (numerator + denominator)
    static PropertyID pIOPropMediaType = "mediaType"; // uint32 MediaType flags
    static PropertyID pIOPropCodecDirection = "codecDirection"; // uint32 CodecDirection flags
    static PropertyID pIOPropWidth = "width"; // uint32
    static PropertyID pIOPropHeight = "height"; // uint32
    static PropertyID pIOPropStride = "stride"; // uint32 array of plane strides
    static PropertyID pIOPropFieldOrder = "fieldOrder"; // uint8: 0 - progressive, 1 - top field first, 2 - bottom field first
    static PropertyID pIOPropCropTopX = "cropX"; // uint32
    static PropertyID pIOPropCropTopY = "cropY"; // uint32
    static PropertyID pIOPropCropWidth = "cropW"; // uint32
    static PropertyID pIOPropCropHeight = "cropH"; // uint32
    static PropertyID pIOPropHWAcc = "hwAcc"; // uint8_t hw acceleration (1 - on, 0 - off)
    static PropertyID pIOPropTemporalReordering = "frameReordering"; // uint32_t: absent - intra frames, 0 - off (p-frames only), > 0 - number of B-frames
    static PropertyID pIOPropDataRange = "dataRange"; // uint8_t 0 - video range, 1 - full range, encoder codec may specify both as array of two, the first one will be default
                                                      // or only a single entry for single data levels option available for the codec
    static PropertyID pIOPropInitFrameBytes = "initFrameBytes"; // uint32_t: 0 - need full first frame, > 0 - number of bytes, absent - no need for init
    static PropertyID pIOPropColorModel = "colorModel"; // uint32 IOPluginColorModel
    static PropertyID pIOPropHSubsampling = "hSubsampling"; // uint8_t horizontal subsampling
    static PropertyID pIOPropVSubsampling = "vSubsampling"; // uint8_t vertical subsampling
    static PropertyID pIOPropNumChannels = "numChannels"; // uint32_t number of (audio) channels
    static PropertyID pIOPropDecoderDelay = "decoderDelay"; //uint32_t in/out decoder delay audio samples (will be discarded from the beginning of the stream)
    static PropertyID pIOPropDecoderPreroll = "decoderPreroll"; //uint32_t start this number of frames earlier to bootstrap the decoder state
    static PropertyID pIOPropFrameRate = "frameRate"; // double or two uint32 (numerator + denominator)
    static PropertyID pIOPropFrameRateIsDrop = "frameRateIsDrop"; // uint8_t 1 - dropframe, 0 - non-drop
    static PropertyID pIOPropSamplingRate = "samplingRate"; // uint32
    static PropertyID pIOPropIsKeyFrame = "isKeyFrame"; // uint8_t 0/absent - not a keyframe, 1 - a keyframe
    static PropertyID pIOPropHasAlpha = "hasAlpha"; // uint8_t 0 - alpha present but meaningless, absent - alpha channel is absent, 1 - present and used
    static PropertyID pIOPropMultiPass = "isMultipass"; // uint8_t 0/absent - single pass, 1 - multipass
    static PropertyID pIOPropColorPrimaries = "clrPrimaries"; // int16_t
    static PropertyID pIOTransferCharacteristics = "clrTransfer"; // int16_t
    static PropertyID pIOPropPath = "path"; //string, representing path (for encoder - destination file)
    static PropertyID pIOColorMatrix = "clrMtx"; // int16_t
    static PropertyID pIOPropSupportedFPS = "supportedFPS"; // double or two uint32 (numerator + denominator) array to limit the supported FPS
    static PropertyID pIOCustomErrorString = "customErrorString"; // string, custom error message for encode error
    static PropertyID pIOPropBitRate = "bitRate"; // uint64_t bits per second

    static PropertyID pIOPropHDRPrimaries = "hdrPrimaries"; // float array
    static PropertyID pIOPropHDRWhitePoint = "hdrWhite"; // float array
    static PropertyID pIOPropHDRMasterLum = "hdrMasterLum"; // float array
    static PropertyID pIOPropHDRMaxCLL = "hdrMaxCLL"; // uint16_t
    static PropertyID pIOPropHDRMaxFALL = "hdrMaxFALL"; // uint16_t

    static PropertyID pIOPropContainerList = "codecContainers"; // string list with UUIDs or mov/mp4 strings

    // common UI entries
    static PropertyID pIOPropUIType = "uiType"; // uint32
    static PropertyID pIOPropUIValue = "uiValue"; // variable type
    static PropertyID pIOPropUILabel = "uiLabel"; // variable type
    static PropertyID pIOPropUISuffix = "uiSuffix"; // variable type
    static PropertyID pIOPropUIDisabled = "uiDisabled"; // uint8_t
    static PropertyID pIOPropUIHidden = "uiHidden"; // uint8_t
    static PropertyID pIOPropUITriggerUpdate = "uiTrigger"; // uint8_t 0 - value change doesn't trigger update, 1 - does trigger update

    // slider
    static PropertyID pIOPropUIMinValue = "uiMinValue"; // variable type
    static PropertyID pIOPropUIMaxValue = "uiMaxValue"; // variable type
    static PropertyID pIOPropUIDefaultValue = "uiDefaultValue"; // variable type
    static PropertyID pIOPropUIStep = "uiStep"; // variable type

    // combobox
    static PropertyID pIOPropUITextsList = "uiTexts"; // variable type
    static PropertyID pIOPropUIValuesList = "uiValues"; // variable type

    // Container track info
    static PropertyID pIOPropTrackIdx = "trackIdx"; // uint32_t 0-based track index
    static PropertyID pIOPropPAR = "par"; // double, pixel aspect ratio
    static PropertyID pIOPropContainerExt = "contExt"; // string extension
    static PropertyID pIOPropAudioChannelLayout = "audLayout"; // uint32 AudioChannelLayout
    static PropertyID pIOBufferStride = "bufferStride"; // array of uint32_t bytes per line per component
    static PropertyID pIOPropStartTime = "startTime"; // double - start time in seconds (timespamp in seconds from the midnight, should be accompanied by fps)

    static PropertyID pIOPropMarkersBlob = "markersBlob"; // uint8_t array containing markers data
}
