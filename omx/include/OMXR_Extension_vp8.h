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
 * OMXR Extension header for VP8
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_VP8.h
 * 
 */

#ifndef OMXR_EXTENSION_VP8_H
#define OMXR_EXTENSION_VP8_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_video.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_VendorBaseOffsetVP8     (0x000001900)		/**< base offset for VP8 (internal use) */

#define OMXR_MC_IndexVendorBaseVP8      (OMXR_MC_IndexVendorBaseVideo + OMXR_MC_VendorBaseOffsetVP8)  /**< base value of extended OMX_INDEXTYPE for VP8  */
#define OMXR_MC_EventVendorBaseVP8      (OMXR_MC_EventVendorBaseVideo + OMXR_MC_VendorBaseOffsetVP8)  /**< base value of extended OMX_EVENTTYPE for VP8  */
#define OMXR_MC_ErrorVendorBaseVP8      (OMXR_MC_ErrorVendorBaseVideo + OMXR_MC_VendorBaseOffsetVP8)  /**< base value of extended OMX_ERRORTYPE for VP8  */

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
typedef struct tagOMXR_MC_VP8D_DECODERESULTTYPE {
	OMX_U32						nSize;
	OMX_VERSIONTYPE				nVersion;
	OMX_U32						u32HorizontalScale;
	OMX_U32						u32VerticalScale;
}OMXR_MC_VP8D_DECODERESULTTYPE;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_VP8_H */
