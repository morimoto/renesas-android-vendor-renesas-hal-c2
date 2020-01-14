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
 * OMXR Extension header for VP8 decoder 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_vp8d.h
 * 
 */

#ifndef OMXR_EXTENSION_VP8D_H
#define OMXR_EXTENSION_VP8D_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_vdcmn.h"
#include "OMXR_Extension_vp8.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_IndexVendorBaseVP8Decoder     (OMXR_MC_IndexVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetVP8)		/**< base value of extended OMX_INDEXTYPE for VP8 decoder  */
#define OMXR_MC_EventVendorBaseVP8Decoder     (OMXR_MC_EventVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetVP8)		/**< base value of extended OMX_EVENTTYPE for VP8 decoder  */
#define OMXR_MC_ErrorVendorBaseVP8Decoder     (OMXR_MC_ErrorVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetVP8)		/**< base value of extended OMX_ERRORTYPE for VP8 decoder  */

#define OMXR_MC_SyntaxIndexBaseVP8Decoder     (OMXR_MC_VideoSyntaxIndexBase + OMXR_MC_VendorBaseOffsetVP8)
/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/**
 * extended VP8d syntax
 */
enum {
    OMXR_MC_IndexConfigVideoInfoVp8Syntax = (OMXR_MC_SyntaxIndexBaseVP8Decoder + 0x0000)
};

/**
* VP8 Uncompressed Data Chunk
*/
typedef struct tagOMXR_MC_VIDEO_INFO_VP8_UDC_SYNTAX {
	OMX_U32	struct_size;
	OMXR_MC_VIDEO_SYNTAX_STATUS	status;
	OMX_U32	frame_type;
	OMX_U32	version;
	OMX_U32	show_frame;
	OMX_U32	first_part_size;
	OMX_U32	width;
	OMX_U32	horizontal_scale;
	OMX_U32	height;
	OMX_U32	vertical_scale;
} OMXR_MC_VIDEO_INFO_VP8_UDC_SYNTAXTYPE;

/**
* VP8 Frame Header
*/
typedef struct OMXR_MC_VIDEO_INFO_VP8_FH_SYNTAX {
	OMX_U32	struct_size;
	OMXR_MC_VIDEO_SYNTAX_STATUS	status;
	OMX_U32	color_space;
	OMX_U32	clamping_type;
	OMX_U32	segmentation_enabled;
	OMX_U32	filter_type;
	OMX_U32	loop_filter_level;
	OMX_U32	sharpness_level;
	OMX_U32	log2_nbr_of_dct_partitions;
	OMX_U32	refresh_entropy_probs;
	OMX_U32	refresh_golden_frame;
	OMX_U32	refresh_alternate_frame;
	OMX_U32	copy_buffer_to_golden;
	OMX_U32	copy_buffer_to_alternate;
	OMX_U32	sign_bias_golden;
	OMX_U32	sign_bias_alternate;
	OMX_U32	refresh_last;
} OMXR_MC_VIDEO_INFO_VP8_FH_SYNTAXTYPE;

/**
* VP8 syntax information
*/
typedef struct tagOMXR_MC_VIDEO_INFO_VP8_SYNTAX {
	OMXR_MC_VIDEO_INFO_VP8_UDC_SYNTAXTYPE 	udc;
	OMXR_MC_VIDEO_INFO_VP8_FH_SYNTAXTYPE 	fh;
} OMXR_MC_VIDEO_INFO_VP8_SYNTAXTYPE;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_VP8D_H */
