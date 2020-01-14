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
 * OMXR Extension header for MPEG4 decoder 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_m4vd.h
 * 
 */

#ifndef OMXR_EXTENSION_M4VD_H
#define OMXR_EXTENSION_M4VD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_vdcmn.h"
#include "OMXR_Extension_m4v.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_IndexVendorBaseM4VDecoder     (OMXR_MC_IndexVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetM4V)		/**< extended definition of OMX_INDEXTYPE for MPEG4 decoder  */
#define OMXR_MC_EventVendorBaseM4VDecoder     (OMXR_MC_EventVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetM4V)		/**< extended definition of OMX_EVENTTYPE for MPEG4 decoder  */
#define OMXR_MC_ErrorVendorBaseM4VDecoder     (OMXR_MC_ErrorVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetM4V)		/**< extended definition of OMX_ERRORTYPE for MPEG4 decoder  */

#define OMXR_MC_SyntaxIndexBaseM4VDecoder     (OMXR_MC_VideoSyntaxIndexBase + OMXR_MC_VendorBaseOffsetM4V)

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/**
 * extended m4v syntax
 */
enum {
    OMXR_MC_IndexConfigVideoInfoM4vSyntax = (OMXR_MC_SyntaxIndexBaseM4VDecoder + 0x0000)
};

/**
* Visual Object Sequence information
*/
typedef struct tagOMXR_MC_VIDEO_INFO_M4V_VOS_SYNTAX {
	OMX_U32				struct_size;
	OMXR_MC_VIDEO_SYNTAX_STATUS	status;
	OMX_U32				profile_and_level_indication;
} OMXR_MC_VIDEO_INFO_M4V_VOS_SYNTAXTYPE;

/**
* Visual Object information
*/
typedef struct tagOMXR_MC_VIDEO_INFO_M4V_VO_SYNTAX {
	OMX_U32	struct_size;
	OMXR_MC_VIDEO_SYNTAX_STATUS	status;
	OMX_U32	is_visual_object_identifier;
	OMX_U32	visual_object_verid;
	OMX_U32	visual_object_priority;
	OMX_U32	visual_object_type;
	OMX_U32	video_signal_type;
	OMX_U32	video_format;
	OMX_U32	video_range;
	OMX_U32	colour_description;
	OMX_U32	colour_primaries;
	OMX_U32	transfer_characteristics;
	OMX_U32	matrix_coefficients;
} OMXR_MC_VIDEO_INFO_M4V_VO_SYNTAXTYPE;

/**
* Visual Object Layer information
*/
typedef struct tagOMXR_MC_VIDEO_INFO_M4V_VOL_SYNTAX {
	OMX_U32	struct_size;
	OMXR_MC_VIDEO_SYNTAX_STATUS	status;
	OMX_U32	random_accessible_vol;
	OMX_U32	video_object_type_indication;
	OMX_U32	is_object_layer_identifier;
	OMX_U32	video_object_layer_verid;
	OMX_U32	video_object_layer_priority;
	OMX_U32	aspect_ratio_info;
	OMX_U32	par_width;
	OMX_U32	par_height;
	OMX_U32	vol_control_parameters;
	OMX_U32	chroma_format;
	OMX_U32	low_delay;
	OMX_U32	vbv_parameters;
	OMX_U32	first_half_bit_rate;
	OMX_U32	latter_half_bit_rate;
	OMX_U32	first_half_vbv_buffer_size;
	OMX_U32	latter_half_vbv_buffer_size;
	OMX_U32	first_half_vbv_occupancy;
	OMX_U32	latter_half_vbv_occupancy;
	OMX_U32	video_object_layer_shape;
	OMX_U32	vop_time_increment_resolution;
	OMX_U32	fixed_vop_rate;
	OMX_U32	fixed_vop_time_increment;
	OMX_U32	video_object_layer_width;
	OMX_U32	video_object_layer_height;
	OMX_U32	interlaced;
	OMX_U32	obmc_disable;
	OMX_U32	sprite_enable;
	OMX_U32	no_of_sprite_warping_points;
	OMX_U32	sprite_warping_accuracy;
	OMX_U32	sprite_brightness_change;
	OMX_U32	not_8_bit;
	OMX_U32	quant_type;
	OMX_U32	quarter_sample;
	OMX_U32	complexity_estimation_disable;
	OMX_U32	resync_marker_disable;
	OMX_U32	data_partitioned;
	OMX_U32	reversible_vlc;
	OMX_U32	newpred_enable;
	OMX_U32	reduced_resolution_vop_enable;
	OMX_U32	scalability;
} OMXR_MC_VIDEO_INFO_M4V_VOL_SYNTAXTYPE;

/**
 * m4v syntax information
 */
typedef struct tagOMXR_MC_VIDEO_INFO_M4V_SYNTAX {
	OMXR_MC_VIDEO_INFO_M4V_VOS_SYNTAXTYPE	vos;
	OMXR_MC_VIDEO_INFO_M4V_VO_SYNTAXTYPE	vo;
	OMXR_MC_VIDEO_INFO_M4V_VOL_SYNTAXTYPE	vol;
} OMXR_MC_VIDEO_INFO_M4V_SYNTAXTYPE;


/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_M4VD_H */
