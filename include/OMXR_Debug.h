/*
 * Copyright (C) 2019 GlobalLogic
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OMXR_DEBUG_H
#define OMXR_DEBUG_H

#include <iostream>

#ifndef DEFINE_STREAM_OUT
#define DEFINE_STREAM_OUT(type)                                 \
    inline std::ostream& operator<<(std::ostream& os, type i) { \
        return os << AsString(i);                               \
    }

#ifdef OMX_Core_h

constexpr const char* AsString(OMX_COMMANDTYPE i) {
    switch (i) {
    case OMX_CommandStateSet:
        return "StateSet";
    case OMX_CommandFlush:
        return "Flush";
    case OMX_CommandPortDisable:
        return "PortDisable";
    case OMX_CommandPortEnable:
        return "PortEnable";
    case OMX_CommandMarkBuffer:
        return "MarkBuffer";
    default:
        return "?";
    }
}

constexpr const char* AsString(OMX_STATETYPE i) {
    switch (i) {
    case OMX_StateInvalid:
        return "Invalid";
    case OMX_StateLoaded:
        return "Loaded";
    case OMX_StateIdle:
        return "Idle";
    case OMX_StateExecuting:
        return "Executing";
    case OMX_StatePause:
        return "Pause";
    case OMX_StateWaitForResources:
        return "WaitForResources";
    default:
        return "?";
    }
}

constexpr const char* AsString(OMX_ERRORTYPE i) {
    switch (i) {
    case OMX_ErrorNone:
        return "None";
    case OMX_ErrorInsufficientResources:
        return "InsufficientResources";
    case OMX_ErrorUndefined:
        return "Undefined";
    case OMX_ErrorInvalidComponentName:
        return "InvalidComponentName";
    case OMX_ErrorComponentNotFound:
        return "ComponentNotFound";
    case OMX_ErrorInvalidComponent:
        return "InvalidComponent";
    case OMX_ErrorBadParameter:
        return "BadParameter";
    case OMX_ErrorNotImplemented:
        return "NotImplemented";
    case OMX_ErrorUnderflow:
        return "Underflow";
    case OMX_ErrorOverflow:
        return "Overflow";
    case OMX_ErrorHardware:
        return "Hardware";
    case OMX_ErrorInvalidState:
        return "InvalidState";
    case OMX_ErrorStreamCorrupt:
        return "StreamCorrupt";
    case OMX_ErrorPortsNotCompatible:
        return "PortsNotCompatible";
    case OMX_ErrorResourcesLost:
        return "ResourcesLost";
    case OMX_ErrorNoMore:
        return "NoMore";
    case OMX_ErrorVersionMismatch:
        return "VersionMismatch";
    case OMX_ErrorNotReady:
        return "NotReady";
    case OMX_ErrorTimeout:
        return "Timeout";
    case OMX_ErrorSameState:
        return "SameState";
    case OMX_ErrorResourcesPreempted:
        return "ResourcesPreempted";
    case OMX_ErrorPortUnresponsiveDuringAllocation:
        return "PortUnresponsiveDuringAllocation";
    case OMX_ErrorPortUnresponsiveDuringDeallocation:
        return "PortUnresponsiveDuringDeallocation";
    case OMX_ErrorPortUnresponsiveDuringStop:
        return "PortUnresponsiveDuringStop";
    case OMX_ErrorIncorrectStateTransition:
        return "IncorrectStateTransition";
    case OMX_ErrorIncorrectStateOperation:
        return "IncorrectStateOperation";
    case OMX_ErrorUnsupportedSetting:
        return "UnsupportedSetting";
    case OMX_ErrorUnsupportedIndex:
        return "UnsupportedIndex";
    case OMX_ErrorBadPortIndex:
        return "BadPortIndex";
    case OMX_ErrorPortUnpopulated:
        return "PortUnpopulated";
    case OMX_ErrorComponentSuspended:
        return "ComponentSuspended";
    case OMX_ErrorDynamicResourcesUnavailable:
        return "DynamicResourcesUnavailable";
    case OMX_ErrorMbErrorsInFrame:
        return "MbErrorsInFrame";
    case OMX_ErrorFormatNotDetected:
        return "FormatNotDetected";
    case OMX_ErrorContentPipeOpenFailed:
        return "ContentPipeOpenFailed";
    case OMX_ErrorContentPipeCreationFailed:
        return "ContentPipeCreationFailed";
    case OMX_ErrorSeperateTablesUsed:
        return "SeperateTablesUsed";
    case OMX_ErrorTunnelingUnsupported:
        return "TunnelingUnsupported";
    default:
        return "?";
    }
}

constexpr const char* AsString(OMX_EVENTTYPE i) {
    switch (i) {
    case OMX_EventCmdComplete:
        return "CmdComplete";
    case OMX_EventError:
        return "Error";
    case OMX_EventMark:
        return "Mark";
    case OMX_EventPortSettingsChanged:
        return "PortSettingsChanged";
    case OMX_EventBufferFlag:
        return "BufferFlag";
    case OMX_EventResourcesAcquired:
        return "ResourcesAcquired";
    case OMX_EventComponentResumed:
        return "ComponentResumed";
    case OMX_EventDynamicResourcesAvailable:
        return "DynamicResourcesAvailable";
    case OMX_EventPortFormatDetected:
        return "PortFormatDetected";
    case OMX_EventMax:
        return "EventMax";
    default:
        return "?";
    }
}

DEFINE_STREAM_OUT(OMX_COMMANDTYPE)
DEFINE_STREAM_OUT(OMX_STATETYPE)
DEFINE_STREAM_OUT(OMX_ERRORTYPE)
DEFINE_STREAM_OUT(OMX_EVENTTYPE)

#endif // OMX_Core_h

#ifdef OMX_Index_h

constexpr const char* AsString(OMX_INDEXTYPE i) {
    switch (i) {
    case OMX_IndexParamStandardComponentRole:
        return "ParamStandardComponentRole";
    case OMX_IndexParamPortDefinition:
        return "ParamPortDefinition";
    case OMX_IndexParamVideoPortFormat:
        return "ParamVideoPortFormat";
    case OMX_IndexParamVideoBitrate:
        return "ParamVideoBitrate";
    case OMX_IndexParamVideoIntraRefresh:
        return "ParamVideoIntraRefresh";
    case OMX_IndexParamVideoErrorCorrection:
        return "ParamVideoErrorCorrection";
    case OMX_IndexParamVideoMpeg4:
        return "ParamVideoMpeg4";
    case OMX_IndexParamVideoAvc:
        return "ParamVideoAvc";
    case OMX_IndexParamVideoH263:
        return "ParamVideoH263";
    case OMX_IndexParamVideoProfileLevelQuerySupported:
        return "ParamVideoProfileLevelQuerySupported";
    case OMX_IndexParamVideoProfileLevelCurrent:
        return "ParamVideoProfileLevelCurrent";
    case OMX_IndexConfigVideoBitrate:
        return "ConfigVideoBitrate";
    case OMX_IndexConfigVideoFramerate:
        return "ConfigVideoFramerate";
    case OMX_IndexConfigVideoIntraVOPRefresh:
        return "ConfigVideoIntraVOPRefresh";
    case OMX_IndexConfigCommonInputCrop:
        return "ConfigCommonInputCrop";
    case OMX_IndexConfigCommonOutputCrop:
        return "ConfigCommonOutputCrop";
    default:
        return "?";
    }
}

DEFINE_STREAM_OUT(OMX_INDEXTYPE)

#endif // OMX_Index_h

#ifdef OMX_IVCommon_h

constexpr const char* AsString(OMX_COLOR_FORMATTYPE i) {
    switch (i) {
    case OMX_COLOR_FormatUnused:
        return "COLOR_FormatUnused";
    case OMX_COLOR_FormatMonochrome:
        return "COLOR_FormatMonochrome";
    case OMX_COLOR_Format8bitRGB332:
        return "COLOR_Format8bitRGB332";
    case OMX_COLOR_Format12bitRGB444:
        return "COLOR_Format12bitRGB444";
    case OMX_COLOR_Format16bitARGB4444:
        return "COLOR_Format16bitARGB4444";
    case OMX_COLOR_Format16bitARGB1555:
        return "COLOR_Format16bitARGB1555";
    case OMX_COLOR_Format16bitRGB565:
        return "COLOR_Format16bitRGB565";
    case OMX_COLOR_Format16bitBGR565:
        return "COLOR_Format16bitBGR565";
    case OMX_COLOR_Format18bitRGB666:
        return "COLOR_Format18bitRGB666";
    case OMX_COLOR_Format18bitARGB1665:
        return "COLOR_Format18bitARGB1665";
    case OMX_COLOR_Format19bitARGB1666:
        return "COLOR_Format19bitARGB1666";
    case OMX_COLOR_Format24bitRGB888:
        return "COLOR_Format24bitRGB888";
    case OMX_COLOR_Format24bitBGR888:
        return "COLOR_Format24bitBGR888";
    case OMX_COLOR_Format24bitARGB1887:
        return "COLOR_Format24bitARGB1887";
    case OMX_COLOR_Format25bitARGB1888:
        return "COLOR_Format25bitARGB1888";
    case OMX_COLOR_Format32bitBGRA8888:
        return "COLOR_Format32bitBGRA8888";
    case OMX_COLOR_Format32bitARGB8888:
        return "COLOR_Format32bitARGB8888";
    case OMX_COLOR_FormatYUV411Planar:
        return "COLOR_FormatYUV411Planar";
    case OMX_COLOR_FormatYUV411PackedPlanar:
        return "COLOR_FormatYUV411PackedPlanar";
    case OMX_COLOR_FormatYUV420Planar:
        return "COLOR_FormatYUV420Planar";
    case OMX_COLOR_FormatYUV420PackedPlanar:
        return "COLOR_FormatYUV420PackedPlanar";
    case OMX_COLOR_FormatYUV420SemiPlanar:
        return "COLOR_FormatYUV420SemiPlanar";
    case OMX_COLOR_FormatYUV422Planar:
        return "COLOR_FormatYUV422Planar";
    case OMX_COLOR_FormatYUV422PackedPlanar:
        return "COLOR_FormatYUV422PackedPlanar";
    case OMX_COLOR_FormatYUV422SemiPlanar:
        return "COLOR_FormatYUV422SemiPlanar";
    case OMX_COLOR_FormatYCbYCr:
        return "COLOR_FormatYCbYCr";
    case OMX_COLOR_FormatYCrYCb:
        return "COLOR_FormatYCrYCb";
    case OMX_COLOR_FormatCbYCrY:
        return "COLOR_FormatCbYCrY";
    case OMX_COLOR_FormatCrYCbY:
        return "COLOR_FormatCrYCbY";
    case OMX_COLOR_FormatYUV444Interleaved:
        return "COLOR_FormatYUV444Interleaved";
    case OMX_COLOR_FormatRawBayer8bit:
        return "COLOR_FormatRawBayer8bit";
    case OMX_COLOR_FormatRawBayer10bit:
        return "COLOR_FormatRawBayer10bit";
    case OMX_COLOR_FormatRawBayer8bitcompressed:
        return "COLOR_FormatRawBayer8bitcompressed";
    case OMX_COLOR_FormatL2:
        return "COLOR_FormatL2";
    case OMX_COLOR_FormatL4:
        return "COLOR_FormatL4";
    case OMX_COLOR_FormatL8:
        return "COLOR_FormatL8";
    case OMX_COLOR_FormatL16:
        return "COLOR_FormatL16";
    case OMX_COLOR_FormatL24:
        return "COLOR_FormatL24";
    case OMX_COLOR_FormatL32:
        return "COLOR_FormatL32";
    case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        return "COLOR_FormatYUV420PackedSemiPlanar";
    case OMX_COLOR_FormatYUV422PackedSemiPlanar:
        return "COLOR_FormatYUV422PackedSemiPlanar";
    case OMX_COLOR_Format18BitBGR666:
        return "COLOR_Format18BitBGR666";
    case OMX_COLOR_Format24BitARGB6666:
        return "COLOR_Format24BitARGB6666";
    case OMX_COLOR_Format24BitABGR6666:
        return "COLOR_Format24BitABGR6666";
    default:
#ifdef OMXR_EXTENSION_VIDEO_H
        if (static_cast<std::underlying_type_t<OMX_COLOR_FORMATTYPE>>(i) ==
            OMX_COLOR_FormatYVU420Planar) {
            return "OMXR_COLOR_FormatYV12";
        }
#endif // OMXR_EXTENSION_VIDEO_H
        return "?";
    }
}

DEFINE_STREAM_OUT(OMX_COLOR_FORMATTYPE)

#endif // OMX_IVCommon_h

#ifdef __OMXR_EXTENSION_ANDROID_H__

constexpr const char* AsString(OMXR_VIDEO_CODINGTYPE i) {
    switch (i) {
    case OMXR_VIDEO_CodingUnused:
        return "OMXR_Unused";
    case OMXR_VIDEO_CodingAutoDetect:
        return "OMXR_AutoDetect";
    case OMXR_VIDEO_CodingH263:
        return "OMXR_H263";
    case OMXR_VIDEO_CodingMPEG4:
        return "OMXR_MPEG4";
    case OMXR_VIDEO_CodingAVC:
        return "OMXR_AVC";
    case OMXR_VIDEO_CodingVP8:
        return "OMXR_VP8";
    case OMXR_VIDEO_CodingHEVC:
        return "OMXR_HEVC";
    case OMXR_VIDEO_CodingVP9:
        return "OMXR_VP9";
    default:
        return "?";
    }
}

DEFINE_STREAM_OUT(OMXR_VIDEO_CODINGTYPE)

#endif // __OMXR_EXTENSION_ANDROID_H__

#ifdef OMX_Video_h

constexpr const char* AsString(OMX_VIDEO_CODINGTYPE i) {
    switch (i) {
    case OMX_VIDEO_CodingUnused:
        return "Unused";
    case OMX_VIDEO_CodingAutoDetect:
        return "AutoDetect";
    case OMX_VIDEO_CodingMPEG2:
        return "MPEG2";
    case OMX_VIDEO_CodingH263:
        return "H263";
    case OMX_VIDEO_CodingMPEG4:
        return "MPEG4";
    case OMX_VIDEO_CodingWMV:
        return "WMV";
    case OMX_VIDEO_CodingRV:
        return "RV";
    case OMX_VIDEO_CodingAVC:
        return "AVC";
    case OMX_VIDEO_CodingMJPEG:
        return "MJPEG";
    case OMX_VIDEO_CodingVP8:
        return "VP8";
    case OMX_VIDEO_CodingVP9:
        return "VP9";
    case OMX_VIDEO_CodingHEVC:
        return "HEVC";
    default:
#ifdef __OMXR_EXTENSION_ANDROID_H__
        return AsString(static_cast<OMXR_VIDEO_CODINGTYPE>(i));
#else
        return "?";
#endif // __OMXR_EXTENSION_ANDROID_H__
    }
}

constexpr const char* AsString(OMX_VIDEO_H263PROFILETYPE i) {
    switch (i) {
    case OMX_VIDEO_H263ProfileBaseline:
        return "Baseline";
    case OMX_VIDEO_H263ProfileH320Coding:
        return "H320Coding";
    case OMX_VIDEO_H263ProfileBackwardCompatible:
        return "BackwardCompatible";
    case OMX_VIDEO_H263ProfileISWV2:
        return "ISWV2";
    case OMX_VIDEO_H263ProfileISWV3:
        return "ISWV3";
    case OMX_VIDEO_H263ProfileHighCompression:
        return "HighCompression";
    case OMX_VIDEO_H263ProfileInternet:
        return "Internet";
    case OMX_VIDEO_H263ProfileInterlace:
        return "Interlace";
    case OMX_VIDEO_H263ProfileHighLatency:
        return "HighLatency";
    default:
        return "?";
    }
}

constexpr const char* AsString(OMX_VIDEO_H263LEVELTYPE i) {
    switch (i) {
    case OMX_VIDEO_H263Level10:
        return "Level10";
    case OMX_VIDEO_H263Level20:
        return "Level20";
    case OMX_VIDEO_H263Level30:
        return "Level30";
    case OMX_VIDEO_H263Level40:
        return "Level40";
    case OMX_VIDEO_H263Level45:
        return "Level45";
    case OMX_VIDEO_H263Level50:
        return "Level50";
    case OMX_VIDEO_H263Level60:
        return "Level60";
    case OMX_VIDEO_H263Level70:
        return "Level70";
    default:
        return "?";
    }
}

constexpr const char* AsString(OMX_VIDEO_MPEG4PROFILETYPE i) {
    switch (i) {
    case OMX_VIDEO_MPEG4ProfileSimple:
        return "Simple";
    case OMX_VIDEO_MPEG4ProfileSimpleScalable:
        return "SimpleScalable";
    case OMX_VIDEO_MPEG4ProfileCore:
        return "Core";
    case OMX_VIDEO_MPEG4ProfileMain:
        return "Main";
    case OMX_VIDEO_MPEG4ProfileNbit:
        return "Nbit";
    case OMX_VIDEO_MPEG4ProfileScalableTexture:
        return "ScalableTexture";
    case OMX_VIDEO_MPEG4ProfileSimpleFace:
        return "SimpleFace";
    case OMX_VIDEO_MPEG4ProfileSimpleFBA:
        return "SimpleFBA";
    case OMX_VIDEO_MPEG4ProfileBasicAnimated:
        return "BasicAnimated";
    case OMX_VIDEO_MPEG4ProfileHybrid:
        return "Hybrid";
    case OMX_VIDEO_MPEG4ProfileAdvancedRealTime:
        return "AdvancedRealTime";
    case OMX_VIDEO_MPEG4ProfileCoreScalable:
        return "CoreScalable";
    case OMX_VIDEO_MPEG4ProfileAdvancedCoding:
        return "AdvancedCoding";
    case OMX_VIDEO_MPEG4ProfileAdvancedCore:
        return "AdvancedCore";
    case OMX_VIDEO_MPEG4ProfileAdvancedScalable:
        return "AdvancedScalable";
    case OMX_VIDEO_MPEG4ProfileAdvancedSimple:
        return "AdvancedSimple";
    default:
        return "?";
    }
}

constexpr const char* AsString(OMX_VIDEO_MPEG4LEVELTYPE i) {
    switch (i) {
    case OMX_VIDEO_MPEG4Level0:
        return "Level0";
    case OMX_VIDEO_MPEG4Level0b:
        return "Level0b";
    case OMX_VIDEO_MPEG4Level1:
        return "Level1";
    case OMX_VIDEO_MPEG4Level2:
        return "Level2";
    case OMX_VIDEO_MPEG4Level3:
        return "Level3";
    case OMX_VIDEO_MPEG4Level4:
        return "Level4";
    case OMX_VIDEO_MPEG4Level4a:
        return "Level4a";
    case OMX_VIDEO_MPEG4Level5:
        return "Level5";
    case OMX_VIDEO_MPEG4Level6:
        return "Level6";
    default:
        return "?";
    }
}

constexpr const char* AsString(OMX_VIDEO_AVCPROFILETYPE i) {
    switch (i) {
    case OMX_VIDEO_AVCProfileBaseline:
        return "Baseline";
    case OMX_VIDEO_AVCProfileMain:
        return "Main";
    case OMX_VIDEO_AVCProfileExtended:
        return "Extended";
    case OMX_VIDEO_AVCProfileHigh:
        return "High";
    case OMX_VIDEO_AVCProfileHigh10:
        return "High10";
    case OMX_VIDEO_AVCProfileHigh422:
        return "High422";
    case OMX_VIDEO_AVCProfileHigh444:
        return "High444";
    default:
        return "?";
    }
}

constexpr const char* AsString(OMX_VIDEO_AVCLEVELTYPE i) {
    switch (i) {
    case OMX_VIDEO_AVCLevel1:
        return "Level1";
    case OMX_VIDEO_AVCLevel1b:
        return "Level1b";
    case OMX_VIDEO_AVCLevel11:
        return "Level11";
    case OMX_VIDEO_AVCLevel12:
        return "Level12";
    case OMX_VIDEO_AVCLevel13:
        return "Level13";
    case OMX_VIDEO_AVCLevel2:
        return "Level2";
    case OMX_VIDEO_AVCLevel21:
        return "Level21";
    case OMX_VIDEO_AVCLevel22:
        return "Level22";
    case OMX_VIDEO_AVCLevel3:
        return "Level3";
    case OMX_VIDEO_AVCLevel31:
        return "Level31";
    case OMX_VIDEO_AVCLevel32:
        return "Level32";
    case OMX_VIDEO_AVCLevel4:
        return "Level4";
    case OMX_VIDEO_AVCLevel41:
        return "Level41";
    case OMX_VIDEO_AVCLevel42:
        return "Level42";
    case OMX_VIDEO_AVCLevel5:
        return "Level5";
    case OMX_VIDEO_AVCLevel51:
        return "Level51";
    default:
        return "?";
    }
}

DEFINE_STREAM_OUT(OMX_VIDEO_CODINGTYPE)
DEFINE_STREAM_OUT(OMX_VIDEO_H263PROFILETYPE)
DEFINE_STREAM_OUT(OMX_VIDEO_H263LEVELTYPE)
DEFINE_STREAM_OUT(OMX_VIDEO_MPEG4PROFILETYPE)
DEFINE_STREAM_OUT(OMX_VIDEO_MPEG4LEVELTYPE)
DEFINE_STREAM_OUT(OMX_VIDEO_AVCPROFILETYPE)
DEFINE_STREAM_OUT(OMX_VIDEO_AVCLEVELTYPE)

#endif // OMX_Video_h

#undef DEFINE_STREAM_OUT
#endif // DEFINE_STREAM_OUT

#endif // OMXR_DEBUG_H
