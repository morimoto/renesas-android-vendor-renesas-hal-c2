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
 * OMXR Extension header for H.263 decoder 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_h263d.h
 * 
 */

#ifndef OMXR_EXTENSION_H263D_H
#define OMXR_EXTENSION_H263D_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_vdcmn.h"
#include "OMXR_Extension_h263.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_IndexVendorBaseH263Decoder     (OMXR_MC_IndexVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH263)		/**< base value of extended OMX_INDEXTYPE for H.263 decoder  */
#define OMXR_MC_EventVendorBaseH263Decoder     (OMXR_MC_EventVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH263)		/**< base value of extended OMX_EVENTTYPE for H.263 decoder  */
#define OMXR_MC_ErrorVendorBaseH263Decoder     (OMXR_MC_ErrorVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH263)		/**< base value of extended OMX_ERRORTYPE for H.263 decoder  */

#define OMXR_MC_SyntaxIndexBaseH263Decoder     (OMXR_MC_VideoSyntaxIndexBase + OMXR_MC_VendorBaseOffsetH263)

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/**
 * extended H.263 Syntax
 */
enum {
    OMXR_MC_IndexConfigVideoInfoH263Syntax = (OMXR_MC_SyntaxIndexBaseH263Decoder + 0x0000)
};

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_H263D_H */
