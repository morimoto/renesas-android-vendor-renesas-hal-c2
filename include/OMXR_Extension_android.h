/**********************************************************************
 *
 * PURPOSE
 *   Media Component Library Header File
 *
 * AUTHOR
 *   Renesas Electronics Corporation
 *
 * DATE
 *   2010/06/30
 *
 **********************************************************************/
/*
 * Copyright (C) 2015-2019 Renesas Electronics Corporation. All Rights Reserved.
 * RENESAS ELECTRONICS CONFIDENTIAL AND PROPRIETARY
 * This program must be used solely for the purpose for which
 * it was furnished by Renesas Electronics Corporation.
 * No part of this program may be reproduced or disclosed to
 * others, in any form, without the prior written permission
 * of Renesas Electronics Corporation.
 *
 **********************************************************************/

#ifndef __OMXR_EXTENSION_ANDROID_H__
#define __OMXR_EXTENSION_ANDROID_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Header Files                                                 */
/***************************************************************************/
#include <OMX_Audio.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_IVCommon.h>
#include <OMX_Image.h>
#include <OMX_Index.h>
#include <OMX_Other.h>
#include <OMX_Types.h>
#include <OMX_Video.h>

/*******************/
/* Extended Index. */
/*******************/
typedef enum OMXR_VIDEO_CODINGTYPE {
    OMXR_VIDEO_CodingUnused,
    OMXR_VIDEO_CodingAutoDetect,
    OMXR_VIDEO_CodingMPEG2,
    OMXR_VIDEO_CodingH263,
    OMXR_VIDEO_CodingMPEG4,
    OMXR_VIDEO_CodingWMV,
    OMXR_VIDEO_CodingRV,
    OMXR_VIDEO_CodingAVC,
    OMXR_VIDEO_CodingMJPEG,
    OMXR_VIDEO_CodingKhronosExtensions = 0x6F000000,
    OMXR_VIDEO_CodingVP8,
    OMXR_VIDEO_CodingVendorStartUnused = 0x7F000000,
    OMXR_VIDEO_CodingHEVC = OMX_VIDEO_CodingVendorStartUnused + 0x0005,
    OMXR_VIDEO_CodingVP9,
    OMXR_VIDEO_CodingMax = 0x7FFFFFFF
} OMXR_VIDEO_CODINGTYPE;

typedef enum OMXR_ANDROID_VIDEO_CODINGTYPE {
    OMXR_ANDROID_VIDEO_CodingUnused,
    OMXR_ANDROID_VIDEO_CodingAutoDetect,
    OMXR_ANDROID_VIDEO_CodingMPEG2,
    OMXR_ANDROID_VIDEO_CodingH263,
    OMXR_ANDROID_VIDEO_CodingMPEG4,
    OMXR_ANDROID_VIDEO_CodingWMV,
    OMXR_ANDROID_VIDEO_CodingRV,
    OMXR_ANDROID_VIDEO_CodingAVC,
    OMXR_ANDROID_VIDEO_CodingMJPEG,
    OMXR_ANDROID_VIDEO_CodingVP8,
    OMXR_ANDROID_VIDEO_CodingVP9,
    OMXR_ANDROID_VIDEO_CodingHEVC,
    OMXR_ANDROID_VIDEO_CodingDolbyVision,
    OMXR_ANDROID_VIDEO_CodingKhronosExtensions = 0x6F000000,
    OMXR_ANDROID_VIDEO_CodingVendorStartUnused = 0x7F000000,
    OMXR_ANDROID_VIDEO_CodingMax = 0x7FFFFFFF
} OMXR_ANDROID_VIDEO_CODINGTYPE;

enum {
    OMX_IndexCustomizeStartUnused = (OMX_IndexVendorStartUnused + 0x00100000),
    OMX_IndexEnableAndroidNativeBuffer,
    OMX_IndexAndroidNativeBufferUsage,
    OMX_IndexStoreMetaDataInBuffers,
    OMX_IndexPrependSPSPPSToIDRFrames,
    OMX_IndexPrepareForAdaptivePlayback,
    OMX_IndexDescribeColorAspects
};

typedef struct OMX_PARAM_ANDROID_NATIVEBUFFERUSAGE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nUsage;
} OMX_PARAM_ANDROID_NATIVEBUFFERUSAGE;

typedef struct OMX_PARAM_ANDROID_STOREMATADATAINBUFFERS {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStoreMetaData;
} OMX_PARAM_ANDROID_STOREMATADATAINBUFFERS;

typedef struct OMX_PARAM_ANDROID_ENABLENATIVEBUFFERS {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL enable;
} OMX_PARAM_ANDROID_ENABLENATIVEBUFFERS;

typedef struct OMX_PARAM_ANDROID_BUFFERMETADATA {
    OMX_U32 nSize;
    OMX_U32 u32BufferSize;
    OMX_U32 u32BufferAddress;
} OMX_PARAM_ANDROID_BUFFERMETADATA;

typedef struct OMX_PARAM_ANDROID_PREPENDSPSPPSTOIDRFRAMES {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_BOOL enable;
} OMX_PARAM_ANDROID_PREPENDSPSPPSTOIDRFRAMES;

typedef struct OMX_PARAM_ANDROID_PREPAREFORADAPTIVEPLAYBACK {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
    OMX_U32 nMaxFrameWidth;
    OMX_U32 nMaxFrameHeight;
} OMX_PARAM_ANDROID_PREPAREFORADAPTIVEPLAYBACK;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OMXR_EXTENSION_ANDROID_H__ */
