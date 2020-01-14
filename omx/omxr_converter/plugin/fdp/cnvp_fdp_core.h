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
 * OMX Converter fdp plugin file 
 *
 * \file cnvp_fdp_core.h
 * \attention
 */
#ifndef CNVP_FDP_CORE_H
#define CNVP_FDP_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "vspm_public.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
typedef struct {
    CNV_U32                nConvertMode;              /**<Converter mode*/
    CNV_U32                nVspSeqMode;               /**<Sequence mode*/
    CNV_U32                nCurInputIndicator;        /**<Current input indicator */
    CNV_BOOL               bInitFlag;                 /**<Init flag*/
    CNV_BOOL               bEosFlag;                  /**<Eos flag*/
    CNV_BOOL               bFirstInput;               /**<First input flag*/
    CNV_BOOL               bField;                    /**<CNV_FALSE(TOP) CNV_TRUE(BOTTOM)*/
    CNV_BOOL               bReleaseFlag;              /**<Release flag*/
    CNV_BOOL               bTlen;                     /**<tl conversion*/
    CNV_BOOL               bDisableInterpolateTimeStamp;/**<Disable interpolate timestamp*/
    CNV_BOOL               nTimeStampOffset;          /**<Timestamp offset*/
    CNV_BOOL               bInterporateTimeStampFlag; /**< interpolate timestamp flag*/
    CNV_BUFFERHEADER_T     sLocalInputBuffer[2];      /**<Input buffers*/
    CNV_BUFFERHEADER_T     sLocalOutputBuffer;        /**<Output buffers*/
    CNVP_CONVERT_BUFFER_T  sConvertEmptyBuffer[2];    /**<Input information*/
    CNVP_CONVERT_BUFFER_T  sConvertFillBuffer;        /**<Output information*/
    void*                  pVspmModuleHandle;         /**<vpsm handle*/
    CNV_U32                nMaxStride;                /**<Max stride*/
    CNV_U32                nMaxSliceHeight;           /**<Max slice Height*/
    CNV_U32                nOpenWidth;                /**<Open width */
    CNV_U32                nOpenHeight;               /**<Open Height */
    CNV_BOOL               bFcnlCompression;          /**<Fcnl compression */
} CNVP_FDP_HANDLE_T;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

CNV_ERRORTYPE CnvpCore_GetPluginCoreFunction(
    CNVP_PLUGIN_FUNCS_T *psPlugInFunc
);

CNV_ERRORTYPE CnvPvspmCore_CreateHandle(
    CNV_PTR *hPluginHandle,
    CNV_SUBERRORTYPE *peSubErrorCode
);

CNV_ERRORTYPE CnvPvspmCore_DeleteHandle(
    CNV_PTR hPluginHandle
);

CNV_BOOL CnvPvspmCore_ConfirmAvailableBuffer(
    CNV_PTR pContext
);

CNV_ERRORTYPE CnvPvspmCore_Executing(
    CNV_PTR  pContext,
    CNV_SUBERRORTYPE *peSubErrorCode
);

CNV_ERRORTYPE CnvPvspmCore_InitPluginParam(
   CNV_PTR            pContext,
   CNV_BUFFERHEADER_T *psBuffer
);

CNV_ERRORTYPE CnvPvspmCore_ConfirmPluginSpec(
	CNVP_PLUGIN_SPEC_T     *psPluginSpec,
    CNV_BOOL               *pbResult,
    CNV_SUBERRORTYPE       *peSubErrorCode
);

CNV_ERRORTYPE CnvPvspmCore_GetReleaseBuffer(
    CNV_PTR pContext,
    CNV_PTR *ppEmptyBuffer,
    CNV_PTR *ppFillBuffer
);

CNV_ERRORTYPE CnvPvspmCore_GetUsingBuffer(
    CNV_PTR pContext,
    CNV_PTR *ppEmptyBuffer,
    CNV_PTR *ppFillBuffer
);

#ifdef __cplusplus
}
#endif

#endif

