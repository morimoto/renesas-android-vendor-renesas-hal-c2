/*
 * Copyright(C) 2017-2018 Renesas Electronics Corporation. All Rights Reserved.
 * RENESAS ELECTRONICS CONFIDENTIAL AND PROPRIETARY
 * This program must be used solely for the purpose for which
 * it was furnished by Renesas Electronics Corporation.
 * No part of this program may be reproduced or disclosed to
 * others, in any form, without the prior written permission
 * of Renesas Electronics Corporation.
 */
/**
 * OMXR Extension header for VP9 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_vp9.h
 * 
 */

#ifndef OMXR_EXTENSION_VP9_H
#define OMXR_EXTENSION_VP9_H

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
#define OMXR_MC_VendorBaseOffsetVP9     (0x000001B00)		/**< base offset for VP9 (internal use) */

#define OMXR_MC_IndexVendorBaseVP9      (OMXR_MC_IndexVendorBaseVideo + OMXR_MC_VendorBaseOffsetVP9)  /**< base value of extended OMX_INDEXTYPE for VP9  */
#define OMXR_MC_EventVendorBaseVP9      (OMXR_MC_EventVendorBaseVideo + OMXR_MC_VendorBaseOffsetVP9)  /**< base value of extended OMX_EVENTTYPE for VP9  */
#define OMXR_MC_ErrorVendorBaseVP9      (OMXR_MC_ErrorVendorBaseVideo + OMXR_MC_VendorBaseOffsetVP9)  /**< base value of extended OMX_ERRORTYPE for VP9  */

/**
 * extended #OMX_INDEXTYPE for VP9 decoder
 */
enum {
	OMXR_IndexParamVideoVp9              = (OMXR_MC_IndexVendorBaseVP9 + 0x0000)   /**< N parameter. parameter name:OMX.RENESAS.INDEX.PARAM.VIDEO.VP9 */
};

/** 
 * Enumeration of extended #OMX_VIDEO_CODINGTYPE 
 */
enum {
    OMXR_MC_VIDEO_CodingVP9 = OMX_VIDEO_CodingVendorStartUnused + 0x0006
};

/** 
 */
typedef enum OMXR_VIDEO_VP9PROFILETYPE{
    OMXR_VIDEO_VP9Profile0,                   	 /**< profile 0         */
    OMXR_VIDEO_VP9Profile1,                   	 /**< profile 1         */
    OMXR_VIDEO_VP9Profile2,                   	 /**< profile 2         */
    OMXR_VIDEO_VP9Profile3,                   	 /**< profile 3         */
    OMXR_VIDEO_VP9ProfileUnknown,                /**< unknown profile       */
    OMXR_MC_VIDEO_VP9ProfileCustomizeStartUnused /**< reserved to customize */
}OMXR_VIDEO_VP9PROFILETYPE;

/** 
 */
typedef enum OMXR_VIDEO_VP9LEVELTYPE{
    OMXR_VIDEO_VP9Level1,        	/**< level 1        */
    OMXR_VIDEO_VP9Level11,        	/**< level 11       */
    OMXR_VIDEO_VP9Level2,       	/**< level 2        */
    OMXR_VIDEO_VP9Level21,        	/**< level 21       */
    OMXR_VIDEO_VP9Level3,       	/**< level 3        */
    OMXR_VIDEO_VP9Level31,        	/**< level 31       */
    OMXR_VIDEO_VP9Level4,       	/**< level 4        */
    OMXR_VIDEO_VP9Level41,        	/**< level 41       */
    OMXR_VIDEO_VP9LevelUnknown,  	/**< unknown level  */
    OMXR_MC_VIDEO_VP9LevelCustomizeStartUnused  /**< reserved to customize */
}OMXR_VIDEO_VP9LEVELTYPE;

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/** 
 * VP9 params 
 */
typedef struct OMXR_VIDEO_PARAM_VP9TYPE {
    OMX_U32 nSize;                       /**< Size of the structure in bytes */
    OMX_VERSIONTYPE nVersion;            /**< OMX specification version information */
    OMX_U32 nPortIndex;                  /**< Port that this structure applies to */
    OMXR_VIDEO_VP9PROFILETYPE eProfile; /**< VP9 profile(s) to use */
    OMXR_VIDEO_VP9LEVELTYPE eLevel;     /**< VP9 level(s) to use */
} OMXR_VIDEO_PARAM_VP9TYPE;


/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_VP9_H */
