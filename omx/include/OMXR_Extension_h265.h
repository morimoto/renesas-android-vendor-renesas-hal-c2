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
 * OMXR Extension header for H.265 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_h265.h
 * 
 */

#ifndef OMXR_EXTENSION_H265_H
#define OMXR_EXTENSION_H265_H

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
#define OMXR_MC_VendorBaseOffsetH265     (0x000001A00)		/**< base offset for H.265 (internal use) */

#define OMXR_MC_IndexVendorBaseH265      (OMXR_MC_IndexVendorBaseVideo + OMXR_MC_VendorBaseOffsetH265)  /**< base value of extended OMX_INDEXTYPE for H.265  */
#define OMXR_MC_EventVendorBaseH265      (OMXR_MC_EventVendorBaseVideo + OMXR_MC_VendorBaseOffsetH265)  /**< base value of extended OMX_EVENTTYPE for H.265  */
#define OMXR_MC_ErrorVendorBaseH265      (OMXR_MC_ErrorVendorBaseVideo + OMXR_MC_VendorBaseOffsetH265)  /**< base value of extended OMX_ERRORTYPE for H.265  */

/**
 * extended #OMX_INDEXTYPE for h265 decoder
 */
enum {
	OMXR_IndexParamVideoHevc              = (OMXR_MC_IndexVendorBaseH265 + 0x0000)   /**< N parameter. parameter name:OMX.RENESAS.INDEX.PARAM.VIDEO.HEVC */
};

/** 
 * Enumeration of extended #OMX_VIDEO_CODINGTYPE 
 */
enum {
    OMXR_MC_VIDEO_CodingHEVC = OMX_VIDEO_CodingVendorStartUnused + 0x0005
};

/** 
 */
typedef enum OMXR_VIDEO_HEVCPROFILETYPE{
    OMXR_VIDEO_HEVCProfileMain,                   /**< main profile          */
    OMXR_VIDEO_HEVCProfileUnknown,                /**< unknown profile       */
    OMXR_MC_VIDEO_HEVCProfileCustomizeStartUnused /**< reserved to customize */
}OMXR_VIDEO_HEVCPROFILETYPE;

/** 
 */
typedef enum OMXR_VIDEO_HEVCLEVELTYPE{
    OMXR_VIDEO_HEVCLevel1,        /**< level 1       */
    OMXR_VIDEO_HEVCLevel2,        /**< level 2       */
    OMXR_VIDEO_HEVCLevel21,       /**< level 2.1     */
    OMXR_VIDEO_HEVCLevel3,        /**< level 3       */
    OMXR_VIDEO_HEVCLevel31,       /**< level 3.1     */
    OMXR_VIDEO_HEVCLevel4,        /**< level 4       */
    OMXR_VIDEO_HEVCLevel41,       /**< level 4.1     */
    OMXR_VIDEO_HEVCLevel5,        /**< level 5       */
    OMXR_VIDEO_HEVCLevelUnknown,  /**< unknown level */
    OMXR_MC_VIDEO_HEVCLevelCustomizeStartUnused  /**< reserved to customize */
}OMXR_VIDEO_HEVCLEVELTYPE;

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/** 
 * HEVC params 
 */
typedef struct OMXR_VIDEO_PARAM_HEVCTYPE {
    OMX_U32 nSize;                       /**< Size of the structure in bytes */
    OMX_VERSIONTYPE nVersion;            /**< OMX specification version information */
    OMX_U32 nPortIndex;                  /**< Port that this structure applies to */
    OMXR_VIDEO_HEVCPROFILETYPE eProfile; /**< HEVC profile(s) to use */
    OMXR_VIDEO_HEVCLEVELTYPE eLevel;     /**< HEVC level(s) to use */
    OMX_U32 nAllowedPictureTypes;        /**< Specifies the picture types allowed in the bitstream */  
} OMXR_VIDEO_PARAM_HEVCTYPE;


/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_H265_H */
