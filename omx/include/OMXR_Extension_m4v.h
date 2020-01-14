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
 * OMXR Extension header for MPEG4 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_m4v.h
 * 
 */
#ifndef OMXR_EXTENSION_M4V_H
#define OMXR_EXTENSION_M4V_H

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
#define OMXR_MC_VendorBaseOffsetM4V     (0x000001100)		/**< base offset for MPEG4 (internal use) */

#define OMXR_MC_IndexVendorBaseM4V      (OMXR_MC_IndexVendorBaseVideo + OMXR_MC_VendorBaseOffsetM4V)  /**< base value of extended OMX_INDEXTYPE for MPEG4  */
#define OMXR_MC_EventVendorBaseM4V      (OMXR_MC_EventVendorBaseVideo + OMXR_MC_VendorBaseOffsetM4V)  /**< base value of extended OMX_EVENTTYPE for MPEG4  */
#define OMXR_MC_ErrorVendorBaseM4V      (OMXR_MC_ErrorVendorBaseVideo + OMXR_MC_VendorBaseOffsetM4V)  /**< base value of extended OMX_ERRORTYPE for MPEG4  */

/** 
 * Enumeration of extended #OMX_VIDEO_MPEG4PROFILETYPE 
 */
enum {
    OMXR_MC_VIDEO_MPEG4ProfileUnknown = OMX_VIDEO_MPEG4ProfileVendorStartUnused,		/**< unknown profile       */
    OMXR_MC_VIDEO_MPEG4ProfileNone,                                                 	/**< no profile            */
    OMXR_MC_VIDEO_MPEG4ProfileCustomizeStartUnused                                  	/**< reserved to customize */
};

/** 
 * Enumeration of extended #OMX_VIDEO_MPEG4LEVELTYPE 
 */
enum {
    OMXR_MC_VIDEO_MPEG4Level3b = OMX_VIDEO_MPEG4LevelVendorStartUnused,					/**< Level 3b              */
    OMXR_MC_VIDEO_MPEG4Level6,															/**< Level 6               */
    OMXR_MC_VIDEO_MPEG4LevelUnknown,                                                	/**< unknown level         */
    OMXR_MC_VIDEO_MPEG4LevelNone,                                                   	/**< no level              */
    OMXR_MC_VIDEO_MPEG4LevelCustomizeStartUnused                                    	/**< reserved to customize */
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

#endif /* OMXR_EXTENSION_M4V_H */
