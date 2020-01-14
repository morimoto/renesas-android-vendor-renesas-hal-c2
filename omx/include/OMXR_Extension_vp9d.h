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
 * OMXR Extension header for VP9 decoder 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_vp9d.h
 * 
 */

#ifndef OMXR_EXTENSION_VP9D_H
#define OMXR_EXTENSION_VP9D_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_vdcmn.h"
#include "OMXR_Extension_vp9.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_IndexVendorBaseVP9Decoder     (OMXR_MC_IndexVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetVP9)		/**< base value of extended OMX_INDEXTYPE for VP9 decoder  */
#define OMXR_MC_EventVendorBaseVP9Decoder     (OMXR_MC_EventVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetVP9)		/**< base value of extended OMX_EVENTTYPE for VP9 decoder  */
#define OMXR_MC_ErrorVendorBaseVP9Decoder     (OMXR_MC_ErrorVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetVP9)		/**< base value of extended OMX_ERRORTYPE for VP9 decoder  */

#define OMXR_MC_VIDEO_NOEL_VP9_REF_IDX			(3u)
#define OMXR_MC_VIDEO_NOEL_VP9_REF				(4u)

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/**
 * extended #OMX_INDEXTYPE for VP9d decoder
 */
enum {
    OMXR_MC_IndexConfigVideoInfoVp9Syntax = (OMXR_MC_IndexVendorBaseVP9Decoder + 0x0000)
};

/**
* VP9 Uncompressed Header
*/
typedef struct tagOMXR_MC_VIDEO_INFO_VP9_UH_SYNTAX {
	OMX_U32	struct_size;
	OMXR_MC_VIDEO_SYNTAX_STATUS	status;
	OMX_U32	profile_low_bit;
	OMX_U32	profile_high_bit;
	OMX_U32	show_existing_frame;
	OMX_U32	frame_to_show_map_idx;
	OMX_U32	frame_type;
	OMX_U32	show_frame;
	OMX_U32	error_resilient_mode;
	OMX_U32	ten_or_twelve_bit;
	OMX_U32	color_space;
	OMX_U32	color_range;
	OMX_U32	subsampling_x;
	OMX_U32	subsampling_y;
	OMX_U32	frame_width_minus_1;
	OMX_U32	frame_height_minus_1;
	OMX_U32	render_and_frame_size_different;
	OMX_U32	render_width_minus_1;
	OMX_U32	render_height_minus_1;
	OMX_U32	intra_only;
	OMX_U32	reset_frame_context;
	OMX_U32	refresh_frame_flags; 
	OMX_U32	ref_frame_idx[OMXR_MC_VIDEO_NOEL_VP9_REF_IDX];
	OMX_U32	ref_frame_sign_bias[OMXR_MC_VIDEO_NOEL_VP9_REF];
	OMX_U32	found_ref;
	OMX_U32	allow_high_precision_mv;
	OMX_U32	is_filter_switchable;
	OMX_U32	raw_interpolation_filter;
	OMX_U32	refresh_frame_context;
	OMX_U32	frame_parallel_decoding_mode;
	OMX_U32	frame_context_idx;
} OMXR_MC_VIDEO_INFO_VP9_UH_SYNTAXTYPE;

/**
* VP9 syntax information
*/
typedef struct tagOMXR_MC_VIDEO_INFO_VP9_SYNTAX {
	OMXR_MC_VIDEO_INFO_VP9_UH_SYNTAXTYPE 	uh;
} OMXR_MC_VIDEO_INFO_VP9_SYNTAXTYPE;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_VP9D_H */
