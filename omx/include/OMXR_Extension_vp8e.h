/*
 * Copyright(C) 2015-2018 Renesas Electronics Corporation. All Rights Reserved.
 * RENESAS ELECTRONICS CONFIDENTIAL AND PROPRIETARY
 * This program must be used solely for the purpose for which
 * it was furnished by Renesas Electronics Corporation.
 * No part of this program may be reproduced or disclosed to
 * others, in any form, without the prior written permission
 * of Renesas Electronics Corporation.
 */
/**
 * OMXR Extension header for VP8 encoder 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_vp8e.h
 */
#ifndef OMXR_EXTENSION_VP8E_H
#define OMXR_EXTENSION_VP8E_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_vecmn.h"
#include "OMXR_Extension_vp8.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_IndexVendorBaseVP8Encoder     (OMXR_MC_IndexVendorBaseVideoEncoder + OMXR_MC_VendorBaseOffsetVP8)		/**< base value of extended OMX_INDEXTYPE for VP8 encoder  */
#define OMXR_MC_EventVendorBaseVP8Encoder     (OMXR_MC_EventVendorBaseVideoEncoder + OMXR_MC_VendorBaseOffsetVP8)		/**< base value of extended OMX_EVENTTYPE for VP8 encoder  */
#define OMXR_MC_ErrorVendorBaseVP8Encoder     (OMXR_MC_ErrorVendorBaseVideoEncoder + OMXR_MC_VendorBaseOffsetVP8)		/**< base value of extended OMX_ERRORTYPE for VP8 encoder  */

#define OMXR_VIDEO_VP8_1DCT_RESIDUAL_PARTITIONS     0u
#define OMXR_VIDEO_VP8_2DCT_RESIDUAL_PARTITIONS     1u

/**
 * extended #OMX_INDEXTYPE for VP8 encoder
 */
enum {
    OMXR_MC_IndexParamVideoVP8Miscellaneous     = (OMXR_MC_IndexVendorBaseVP8Encoder + 0x0000)  /**< OMX.RENESAS.INDEX.PARAM.VIDEO.MISCELLANEOUS */
};

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
typedef struct tagOMXR_MC_VIDEO_PARAM_VP8_MISCELLANEOUS {
	OMX_U32				nSize;                    /**< size of the structure */
	OMX_VERSIONTYPE		nVersion;                 /**< OMX specification version info */
	OMX_U32				nPortIndex;               /**< target port index */
	OMX_U32				nPFrames;                 /**< Number of P frames between each I frame */ 
	OMX_U32				nSharpness;               /**< Sharpness level */
	OMX_U32				nLoopFilterLevel;         /**< Loop filter level */
	OMX_BOOL			bSegmentationEnable;      /**< Segmentation enable */
	OMX_BOOL			bColorSpace;              /**< Color Space */
} OMXR_MC_VIDEO_PARAM_VP8_MISCELLANEOUS;



/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_VP8E_H */
