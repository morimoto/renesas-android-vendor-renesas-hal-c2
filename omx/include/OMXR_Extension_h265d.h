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
 * OMXR Extension header for H265 decoder 
 * 
 * This file contains vendor-defined extension definitions.
 *
 * \file OMXR_Extension_hevd.h
 * 
 */

#ifndef OMXR_EXTENSION_H265D_H
#define OMXR_EXTENSION_H265D_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMXR_Extension_vdcmn.h"
#include "OMXR_Extension_h265.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define OMXR_MC_IndexVendorBaseH265Decoder     (OMXR_MC_IndexVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH265)		/**< base value of extended OMX_INDEXTYPE for H265 decoder  */
#define OMXR_MC_EventVendorBaseH265Decoder     (OMXR_MC_EventVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH265)		/**< base value of extended OMX_EVENTTYPE for H265 decoder  */
#define OMXR_MC_ErrorVendorBaseH265Decoder     (OMXR_MC_ErrorVendorBaseVideoDecoder + OMXR_MC_VendorBaseOffsetH265)		/**< base value of extended OMX_ERRORTYPE for H265 decoder  */

#define OMXR_MC_SyntaxIndexBaseH265Decoder     (OMXR_MC_VideoSyntaxIndexBase + OMXR_MC_VendorBaseOffsetH265)

#define OMXR_MC_VIDEO_NOEL_HEV_PCFLAG  (32u)
#define OMXR_MC_VIDEO_NOEL_HEV_SUBLYR  (7u)
#define OMXR_MC_VIDEO_NOEL_HEV_LTRSPS  (64u)

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/**
 * extended h265 syntax
 */
enum {
    OMXR_MC_IndexConfigVideoInfoHevSyntax = (OMXR_MC_SyntaxIndexBaseH265Decoder + 0x0000)
};

/**
* The syntax information of NAL unit.
*/
typedef struct tagOMXR_MC_VIDEO_INFO_HEV_NALU_SYNTAX {
	OMX_U32	forbidden_zero_bit;
	OMX_U32	nal_unit_type;
	OMX_U32	nuh_layer_id;
	OMX_U32	nuh_temporal_id_plus1;
} OMXR_MC_VIDEO_INFO_HEV_NALU_SYNTAXTYPE;

/**
* The syntax information of Profile, tier and Level.
*/
typedef struct tagOMXR_MC_VIDEO_INFO_HEV_PTL_SYNTAX {
	OMX_U32	struct_size;
	OMX_U32	general_profile_space;
	OMX_U8	general_tier_flag;
	OMX_U32	general_profile_idc;
	OMX_U8	general_profile_compatibility_flag[ OMXR_MC_VIDEO_NOEL_HEV_PCFLAG ];
	OMX_U8	general_progressive_source_flag;
	OMX_U8	general_interlaced_source_flag;
	OMX_U8	general_non_packed_constraint_flag;
	OMX_U8	general_frame_only_constraint_flag;
	OMX_U8	general_max_12bit_constraint_flag;
	OMX_U8	general_max_10bit_constraint_flag;
	OMX_U8	general_max_8bit_constraint_flag;
	OMX_U8	general_max_422chroma_constraint_flag;
	OMX_U8	general_max_420chroma_constraint_flag;
	OMX_U8	general_max_monochrome_constraint_flag;
	OMX_U8	general_intra_constraint_flag;
	OMX_U8	general_one_picture_only_constraint_flag;
	OMX_U8	general_lower_bit_rate_constraint_flag;
	OMX_U8	general_inbld_flag;
	OMX_U32	general_level_idc;
	OMX_U8	sub_layer_profile_present_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_level_present_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U32	sub_layer_profile_space[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_tier_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U32	sub_layer_profile_idc[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_profile_compatibility_flag[OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ][ OMXR_MC_VIDEO_NOEL_HEV_PCFLAG ];
	OMX_U8	sub_layer_progressive_source_flag[OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_interlaced_source_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_non_packed_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_frame_only_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_max_12bit_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_max_10bit_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_max_8bit_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_max_422chroma_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_max_420chroma_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_max_monochrome_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_intra_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_one_picture_only_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_lower_bit_rate_constraint_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U8	sub_layer_inbld_flag[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U32	sub_layer_level_idc[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
} OMXR_MC_VIDEO_INFO_HEV_PTL_SYNTAXTYPE;

/**
* The syntax information of VUI paramters.
*/
typedef struct tagOMXR_MC_VIDEO_INFO_HEV_VUI_SYNTAX {
    OMX_U32	struct_size;
	OMX_U8	aspect_ratio_info_present_flag;
	OMX_U32	aspect_ratio_idc;
	OMX_U32	sar_width;
	OMX_U32	sar_height;
	OMX_U8	overscan_info_present_flag;
	OMX_U8	overscan_appropriate_flag;
	OMX_U8	video_signal_type_present_flag;
	OMX_U32	video_format;
	OMX_U8	video_full_range_flag;
	OMX_U8	colour_description_present_flag;
	OMX_U32	colour_primaries;
	OMX_U32	transfer_characteristics;
	OMX_U32	matrix_coeffs;
	OMX_U8	chroma_loc_info_present_flag;
	OMX_U32	chroma_sample_loc_type_top_field;
	OMX_U32	chroma_sample_loc_type_bottom_field;
	OMX_U8	neutral_chroma_indication_flag;
	OMX_U8	field_seq_flag;
	OMX_U8	frame_field_info_present_flag;
	OMX_U8	default_display_window_flag;
	OMX_U32	def_disp_win_left_offset;
	OMX_U32	def_disp_win_right_offset;
	OMX_U32	def_disp_win_top_offset;
	OMX_U32	def_disp_win_bottom_offset;
	OMX_U8	vui_timing_info_present_flag;
	OMX_U32	vui_num_units_in_tick;
	OMX_U32	vui_time_scale;
	OMX_U8	vui_poc_proportional_to_timing_flag;
	OMX_U32	vui_num_ticks_poc_diff_one_minus1;
	OMX_U8	vui_hrd_parameters_present_flag;
	OMX_U8	bitstream_restriction_flag;
	OMX_U8	tiles_fixed_structure_flag;
	OMX_U8	motion_vectors_over_pic_boundaries_flag;
	OMX_U8	restricted_ref_pic_lists_flag;
	OMX_U32	min_spatial_segmentation_idc;
	OMX_U32	max_bytes_per_pic_denom;
	OMX_U32	max_bits_per_min_cu_denom;
	OMX_U32	log2_max_mv_length_horizontal;
	OMX_U32	log2_max_mv_length_vertical;
} OMXR_MC_VIDEO_INFO_HEV_VUI_SYNTAXTYPE;

/**
* The syntax information of General sequence parameter set RBSP.
*/
typedef struct tagOMXR_MC_VIDEO_INFO_HEV_SPS_SYNTAX {
	OMX_U32	struct_size;
	OMXR_MC_VIDEO_SYNTAX_STATUS	status;
	OMXR_MC_VIDEO_INFO_HEV_NALU_SYNTAXTYPE nalu;
	OMX_U32	sps_video_parameter_set_id;
	OMX_U32	sps_max_sub_layers_minus1;
	OMX_U8	sps_temporal_id_nesting_flag;
	OMX_U32	sps_seq_parameter_set_id;
	OMX_U32	chroma_format_idc;
	OMX_U8	separate_colour_plane_flag;
	OMX_U32	pic_width_in_luma_samples;
	OMX_U32	pic_height_in_luma_samples;
	OMX_U8	conformance_window_flag;
	OMX_U32	conf_win_left_offset;
	OMX_U32	conf_win_right_offset;
	OMX_U32	conf_win_top_offset;
	OMX_U32	conf_win_bottom_offset;
	OMX_U32	bit_depth_luma_minus8;
	OMX_U32	bit_depth_chroma_minus8;
	OMX_U32	log2_max_pic_order_cnt_lsb_minus4;
	OMX_U8	sps_sub_layer_ordering_info_present_flag;
	OMX_U32	sps_max_dec_pic_buffering_minus1[OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U32	sps_max_num_reorder_pics[OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U32	sps_max_latency_increase_plus1[ OMXR_MC_VIDEO_NOEL_HEV_SUBLYR ];
	OMX_U32	log2_min_luma_coding_block_size_minus3;
	OMX_U32	log2_diff_max_min_luma_coding_block_size;
	OMX_U32	log2_min_luma_transform_block_size_minus2;
	OMX_U32	log2_diff_max_min_luma_transform_block_size;
	OMX_U32	max_transform_hierarchy_depth_inter;
	OMX_U32	max_transform_hierarchy_depth_intra;
	OMX_U8	scaling_list_enabled_flag;
	OMX_U8	sps_scaling_list_data_present_flag;
	OMX_U8	amp_enabled_flag;
	OMX_U8	sample_adaptive_offset_enabled_flag;
	OMX_U8	pcm_enabled_flag;
	OMX_U32	pcm_sample_bit_depth_luma_minus1;
	OMX_U32	pcm_sample_bit_depth_chroma_minus1;
	OMX_U32	log2_min_pcm_luma_coding_block_size_minus3;
	OMX_U32	log2_diff_max_min_pcm_luma_coding_block_size;
	OMX_U8	pcm_loop_filter_disabled_flag;
	OMX_U32	num_short_term_ref_pic_sets;
	OMX_U8	long_term_ref_pics_present_flag;
	OMX_U32	num_long_term_ref_pics_sps;
	OMX_U32	lt_ref_pic_poc_lsb_sps[ OMXR_MC_VIDEO_NOEL_HEV_LTRSPS ];
	OMX_U8	used_by_curr_pic_lt_sps_flag[ OMXR_MC_VIDEO_NOEL_HEV_LTRSPS ];
	OMX_U8	sps_temporal_mvp_enabled_flag;
	OMX_U8	strong_intra_smoothing_enabled_flag;
	OMX_U8	vui_parameters_present_flag;
	OMX_U8	sps_extension_present_flag;
	OMX_U8	sps_range_extension_flag;
	OMX_U8	sps_multilayer_extension_flag;
	OMX_U32	sps_extension_6bits;
} OMXR_MC_VIDEO_INFO_HEV_SPS_SYNTAXTYPE;

/**
* hev syntax information
*/
typedef struct tagOMXR_MC_VIDEO_INFO_HEV_SYNTAX {
	OMXR_MC_VIDEO_INFO_HEV_PTL_SYNTAXTYPE		ptl;
	OMXR_MC_VIDEO_INFO_HEV_VUI_SYNTAXTYPE		vui;
	OMXR_MC_VIDEO_INFO_HEV_SPS_SYNTAXTYPE		sps;
} OMXR_MC_VIDEO_INFO_HEV_SYNTAXTYPE;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_H265D_H */
