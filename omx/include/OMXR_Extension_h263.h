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
 * OMXR Extension header for H.263 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_h263.h
 * 
 */

#ifndef OMXR_EXTENSION_H263_H
#define OMXR_EXTENSION_H263_H

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
#define OMXR_MC_VendorBaseOffsetH263     (0x000001300)		/**< base offset for H.263 (internal use) */

#define OMXR_MC_IndexVendorBaseH263      (OMXR_MC_IndexVendorBaseVideo + OMXR_MC_VendorBaseOffsetH263)  /**< base value of extended OMX_INDEXTYPE for H.263  */
#define OMXR_MC_EventVendorBaseH263      (OMXR_MC_EventVendorBaseVideo + OMXR_MC_VendorBaseOffsetH263)  /**< base value of extended OMX_EVENTTYPE for H.263  */
#define OMXR_MC_ErrorVendorBaseH263      (OMXR_MC_ErrorVendorBaseVideo + OMXR_MC_VendorBaseOffsetH263)  /**< base value of extended OMX_ERRORTYPE for H.263  */

/** 
 * Enumeration of extended #OMX_VIDEO_H263PROFILETYPE 
 */
enum {
    OMXR_MC_VIDEO_H263ProfileUnknown = OMX_VIDEO_H263ProfileVendorStartUnused,		/**< unknown profile       */
    OMXR_MC_VIDEO_H263ProfileNone,													/**< no profile            */
    OMXR_MC_VIDEO_H263ProfileCustomizeStartUnused									/**< reserved to customize */
};

/** 
 * Enumeration of extended #OMX_VIDEO_H263LEVELTYPE 
 */
enum {
    OMXR_MC_VIDEO_H263LevelUnknown = OMX_VIDEO_H263LevelVendorStartUnused,			/**< unknown level         */
    OMXR_MC_VIDEO_H263LevelNone,                                          			/**< no level              */
    OMXR_MC_VIDEO_H263LevelCustomizeStartUnused                           			/**< reserved to customize */
};

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_H263_H */
