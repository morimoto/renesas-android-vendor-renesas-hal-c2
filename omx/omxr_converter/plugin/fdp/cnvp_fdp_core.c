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
 *  OMX Converter plugin main function
 *
 * \file cnvp_fdp_core.c
 * \attention 
 */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "vspm_public.h"

#include "cnv_type.h"
#include "cnvp_cmn.h"
#include "cnvp_fdp_core.h"
#include "cnv_osdep.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
/* Release Index */
#define CNVP_BUFFER_CONTROL_KEEP                (0)
#define CNVP_BUFFER_CONTROL_RELEASE             (1)
#define CNVP_BUFFER_CONTROL_2D_FULLRATE_RELEASE (2)

/* Input case */
#define CNVP_BUFFER_DATA (0) 
#define CNVP_EOS_NULL    (1)
#define CNVP_EOS_DATA    (2)

#define CNVP_FDP_QUEUE   (2)
/* image size */
#define CNVP_FDP_MAX_IMAGE_WIDTH  (4096)
#define CNVP_FDP_MAX_IMAGE_HEIGHT (4096)
#define CNVP_FDP_MIN_IMAGE_WIDTH  (32)
#define CNVP_FDP_MIN_IMAGE_HEIGHT (32)

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/

CNV_STATIC CNV_VOID CnvPvspmCore_Callback(
    unsigned long job_id,
    long          result,
    void*         user_data
);

CNV_STATIC CNV_BOOL CnvPvspmCore_CheckBufferFlag(
    CNV_BUFFERHEADER_T *pEmptyBuffer,
    CNV_BUFFERHEADER_T *pFillBuffer
);

CNV_STATIC CNV_VOID CnvPvspmCore_SetUserData(
    CNVP_CONTEXT_T     *pCnvpContext,
    CNV_BUFFERHEADER_T *pFirstEmptyBuffer,
    CNV_BUFFERHEADER_T *pSecondEmptyBuffer
);

CNV_STATIC CNV_VOID CnvPvspmCore_CreatInputData(
    CNVP_FDP_HANDLE_T   *pFdpHandle,
    CNV_BUFFERHEADER_T  *pFirstEmptyBuffer,
    CNV_BUFFERHEADER_T  *pSecondEmptyBuffer,
    CNV_BOOL            bBottomFirst,
    CNV_U32             nInputFormat,
    struct fdp_imgbuf_t *pNextBuffer,
    struct fdp_imgbuf_t *pCurrentBuffer,
    struct fdp_imgbuf_t *pPrevBuffer,
    struct fcp_info_t   *pFcp_Info
);

CNV_STATIC CNV_VOID CnvPvspmCore_SetReleaseFlag(
    CNVP_FDP_HANDLE_T *pFdpHandle
);

CNV_STATIC CNV_VOID CnvPvspmCore_SetPictrueErrorFlag(
    CNVP_FDP_HANDLE_T  *pFdpHandle, 
    CNV_BUFFERHEADER_T *pCurrentBuffer,
    CNV_BUFFERHEADER_T *pPrevBuffer,
    CNV_BUFFERHEADER_T *pFillBuffer
);

CNV_STATIC CNV_VOID CnvPvspmCore_SetTimeStamp(
    CNVP_FDP_HANDLE_T  *pFdpHandle,
    CNV_BUFFERHEADER_T *pCurrentBuffer,
    CNV_BUFFERHEADER_T *pPrevBuffer, 
    CNV_BUFFERHEADER_T *pFillBuffer
);

CNV_STATIC CNV_ERRORTYPE CnvPvspmCore_CheckBufferInfo(
    CNV_BUFFERHEADER_T *pFirstInBuffer,
    CNV_BUFFERHEADER_T *pSecondInBuffer,
    CNV_BUFFERHEADER_T *pOutBuffer,
    CNV_U32            nConvertMode
);

CNV_STATIC CNV_VOID CnvPvspmCore_SetInputData(
    struct fdp_imgbuf_t *pConvertBuffer,
    CNV_BUFFERHEADER_T *pBuffer,
    CNV_BOOL bInputMode,
    CNV_BOOL bBottomFirst,
    CNV_U32  nInputFormat,
    CNV_U32  *pAncBufferY,
    CNV_U32  *pAncBufferC
);

CNV_STATIC CNV_VOID CnvPvspmCore_CreatOutputData(
    CNV_BUFFERHEADER_T  *pFillBuffer,
    CNV_U32             nOutputFormat,
    struct fdp_imgbuf_t *pOutputBuffer
);

CNV_STATIC CNV_VOID CnvPvspmCore_SetCurrentFieldIndicator(
    CNVP_FDP_HANDLE_T *pFdpHandle,
    CNV_BOOL bBottomFirst, 
    CNV_BOOL bUseFirstInput
);

CNV_STATIC CNV_BOOL CnvPvspmCore_CheckErrorFlag(
    CNV_BUFFERHEADER_T *pBuffer
);

CNV_STATIC CNV_BOOL CnvPvspmCore_SetErrorFlag(
    CNV_BUFFERHEADER_T *pInputBuffer,
    CNV_BUFFERHEADER_T *pOutputBuffer
);

CNV_STATIC CNV_TICKS CnvPvspmCore_GetInterpolateTimeStamp
(
	CNVP_FDP_HANDLE_T  *pFdpHandle,
	CNV_BUFFERHEADER_T *pBuffer
);

CNV_STATIC CNV_TICKS CnvPvspmCore_GetSecondFrameTimeStamp(
	CNV_BUFFERHEADER_T *pBuffer
);

CNV_STATIC CNV_BOOL CnvPvspmCore_ConfirmVspmStart(
    CNVP_FDP_HANDLE_T *pFdpHandle,
    CNV_U32 nCurrentAddr,
	CNV_U32 nPrevAddr,
	CNV_U32 nNextAddr,
    CNV_U32 nFlags
);

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/
/**
* \brief get fdp plugin function
*/
CNV_ERRORTYPE CnvpCore_GetPluginCoreFunction(
    CNVP_PLUGIN_FUNCS_T *psPlugInFunc /**< [out]plugin function */
)
{    
    if( psPlugInFunc == NULL ){
        return CNV_ERROR_FATAL;
    }

    psPlugInFunc->CNVP_CreateHandle             = &CnvPvspmCore_CreateHandle;
    psPlugInFunc->CNVP_DeleteHandle             = &CnvPvspmCore_DeleteHandle;
    psPlugInFunc->CNVP_ModuleInit               = &CnvPvspmCore_InitPluginParam;
    psPlugInFunc->CNVP_CheckConvertStart        = &CnvPvspmCore_ConfirmAvailableBuffer;
    psPlugInFunc->CNVP_GetFlushBuffer           = &CnvPvspmCore_GetUsingBuffer;
    psPlugInFunc->CNVP_ConverterExecuting       = &CnvPvspmCore_Executing;
    psPlugInFunc->CNVP_CheckReleaseBuffer       = &CnvPvspmCore_GetReleaseBuffer;
    psPlugInFunc->CNVP_PluginSelected           = &CnvPvspmCore_ConfirmPluginSpec;
    
    return CNV_ERROR_NONE;
}

/**
* \brief confirm plugin specification
*/
CNV_ERRORTYPE CnvPvspmCore_ConfirmPluginSpec(
    CNVP_PLUGIN_SPEC_T *psPluginSpec,  /**< [in] plugin specification */
    CNV_BOOL           *pbResult,      /**< [out] result */
    CNV_SUBERRORTYPE   *peSubErrorCode /**< [out] sub error code */
)
{
    CNV_ERRORTYPE eResult = CNV_ERROR_NONE;
    
    CnvPcmn_SetSubErrorCode( peSubErrorCode, CNV_ERROR_SUBMODULE_PARAM );

    if(( pbResult == NULL ) ||
       ( psPluginSpec == NULL ) ||
       (( psPluginSpec->bInterlacedSequence == CNV_FALSE ) && ( psPluginSpec->bBottomFirst == CNV_TRUE )) ||
       ( psPluginSpec->bCropping != CNV_FALSE ) ||
       ( psPluginSpec->bScaling != CNV_FALSE )) {
        return CNV_ERROR_INVALID_PARAMETER;
    }

    *pbResult = CNV_TRUE;
    CnvPcmn_SetSubErrorCode( peSubErrorCode, CNV_SUBERROR_NONE );

    return eResult;
}

/**
* \brief create fdp plugin handle
*/
CNV_ERRORTYPE CnvPvspmCore_CreateHandle(
    CNV_PTR          *hPluginHandle, /**< [in] plugin handle */
    CNV_SUBERRORTYPE *peSubErrorCode /**< [in] sub error code */
)
{
    CNVP_FDP_HANDLE_T  *hHandle;
    CNV_U32           nCnt;
    CNV_ERRORTYPE eResult = CNV_ERROR_NONE;

    if (hPluginHandle == NULL) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    hHandle = ( CNVP_FDP_HANDLE_T *)Cnvdep_Malloc(sizeof(CNVP_FDP_HANDLE_T));
    if( hHandle == NULL ){
        CnvPcmn_SetSubErrorCode( peSubErrorCode, CNV_ERROR_SUBMODULE_PARAM_CREATE_HANDLE );
        return CNV_ERROR_FATAL;
    }
    /* clear */
    (void)Cnvdep_Memset(hHandle,0,sizeof(CNVP_FDP_HANDLE_T));
    (void)Cnvdep_Memset(&hHandle->sLocalOutputBuffer,0,sizeof(CNV_BUFFERHEADER_T));    
    
    /* Init parameter */
    for( nCnt = 0; nCnt < CNVP_FDP_QUEUE; nCnt++ ){
        (void)Cnvdep_Memset(&hHandle->sLocalInputBuffer[nCnt],0,sizeof(CNV_BUFFERHEADER_T));
        hHandle->sConvertEmptyBuffer[nCnt].nReleaseFlag      = CNV_BUFFER_NONE; 
        hHandle->sConvertEmptyBuffer[nCnt].pConvertBuffer    = &hHandle->sLocalInputBuffer[nCnt];
    }

    hHandle->bEosFlag     = CNV_FALSE;
    hHandle->bFirstInput  = CNV_FALSE;
    hHandle->bReleaseFlag = CNV_FALSE;
    hHandle->sConvertFillBuffer.nReleaseFlag   = CNV_BUFFER_NONE;
    hHandle->sConvertFillBuffer.pConvertBuffer = &hHandle->sLocalOutputBuffer;
    hHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;

    *hPluginHandle = (CNV_PTR)hHandle;

    return eResult;
}

/**
* \brief delete fdp plugin handle
*/
CNV_ERRORTYPE CnvPvspmCore_DeleteHandle(
    CNV_PTR hPluginHandle  /**< [in] plugin handle */
)
{
    CNVP_FDP_HANDLE_T  *pFdpHandle;
    CNV_ERRORTYPE      eResult = CNV_ERROR_NONE;
    long               rt_code = 0; 

    if (hPluginHandle == NULL) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    pFdpHandle = (CNVP_FDP_HANDLE_T *)hPluginHandle;
    
    if( pFdpHandle->bInitFlag == CNV_TRUE ){
        rt_code = vspm_quit_driver(pFdpHandle->pVspmModuleHandle);
        if( rt_code != R_VSPM_OK ){
            CNV_LOGGER(CNV_LOG_ERROR,"vspm_quit_driver fatal %d", rt_code);
            eResult = CNV_ERROR_FATAL;
        }
    }
    Cnvdep_Free(hPluginHandle);

    return eResult;
}

/**
* \brief initialize modules and parameter
*/
CNV_ERRORTYPE CnvPvspmCore_InitPluginParam(
    CNV_PTR            pContext, /**< [in] context */
    CNV_BUFFERHEADER_T *psBuffer /**< [in] input buffer */
)
{
    long                   rt_code;
    CNVP_CONTEXT_T         *pCnvpContext;
    CNVP_FDP_HANDLE_T      *pCnvpFdpHandle;
    void*                  pVspmHandle;
    struct vspm_init_t     sVspmInitParam;
    struct vspm_init_fdp_t sVspmInitFdp;
    CNV_U32                nStlmskphys;
    CNV_U32                nOfset;

    CNV_ERRORTYPE          eResult = CNV_ERROR_NONE;

    if ((pContext == NULL) || (psBuffer == NULL)) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    pCnvpContext      = (CNVP_CONTEXT_T*)pContext;
    pCnvpFdpHandle    = (CNVP_FDP_HANDLE_T*)pCnvpContext->pCnvpDepHandle;

    /* Init Param */
    pCnvpFdpHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;
    pCnvpFdpHandle->bInterporateTimeStampFlag = CNV_FALSE;

    /* Input Info */
    pCnvpFdpHandle->nMaxStride = pCnvpContext->sModuleParam.nMaxStride;
    pCnvpFdpHandle->nMaxSliceHeight = pCnvpContext->sModuleParam.nMaxStride;
    pCnvpFdpHandle->nOpenWidth = psBuffer->sTopAreaImage.nWidth;
    pCnvpFdpHandle->nOpenHeight = psBuffer->sTopAreaImage.nHeight;
    pCnvpFdpHandle->bFcnlCompression = pCnvpContext->bImageCompression;
    pCnvpFdpHandle->bTlen = pCnvpContext->sPluginRule.bTileMode;
    pCnvpFdpHandle->bInitFlag = CNV_TRUE;
    pCnvpFdpHandle->bDisableInterpolateTimeStamp = pCnvpContext->bDisableInterpolateTimeStamp;
    pCnvpFdpHandle->nTimeStampOffset             = pCnvpContext->nTimeStampOffset;

    /* Converter default mode(Auto) */
    if (pCnvpContext->nConvertMode == CNVP_DEFAULT_CONVERT_MODE ){
        if (pCnvpContext->sPluginRule.bInterlacedSequence == CNV_TRUE) {
            pCnvpContext->nConvertMode = CNV_DEINT_3D_HALFRATE;
        }
        else {
            pCnvpContext->nConvertMode = CNV_PROG;
        }
    }

    pCnvpFdpHandle->nConvertMode = pCnvpContext->nConvertMode;
    CNV_LOGGER(CNV_LOG_DEBUG, "pCnvpFdpHandle->nConvertMode = %d", pCnvpFdpHandle->nConvertMode);
    switch(pCnvpFdpHandle->nConvertMode)
    {
    case CNV_PROG:
        pCnvpFdpHandle->nVspSeqMode  = FDP_SEQ_PROG;
        break;
    case CNV_DEINT_2D_HALFRATE:
    case CNV_DEINT_2D_FULLRATE:
        pCnvpFdpHandle->nVspSeqMode  = FDP_SEQ_INTER_2D;
        break;
    case CNV_DEINT_3D_HALFRATE:
    case CNV_DEINT_3D_FULLRATE:
        pCnvpFdpHandle->nVspSeqMode  = FDP_SEQ_INTER;
        break;
    default:
        CNV_LOGGER(CNV_LOG_ERROR, "Not support conversion mode. %d\n", pCnvpFdpHandle->nConvertMode);
        pCnvpFdpHandle->nConvertMode = CNV_PROG;
        pCnvpFdpHandle->nVspSeqMode = FDP_SEQ_PROG;
        break;
    }

    /* vspm initialize parameter */
    nStlmskphys = pCnvpContext->sModuleParam.nModuleWorkPhysAddr;
    nOfset = pCnvpContext->sModuleParam.nModuleWorkSize / 2;

    (void)Cnvdep_Memset(&sVspmInitFdp, 0, sizeof(struct vspm_init_fdp_t));
    sVspmInitFdp.hard_addr[0] = nStlmskphys;
    sVspmInitFdp.hard_addr[1] = nStlmskphys + nOfset;

    (void)Cnvdep_Memset(&sVspmInitParam, 0, sizeof(struct vspm_init_t));
    sVspmInitParam.use_ch = VSPM_EMPTY_CH;
    sVspmInitParam.mode = VSPM_MODE_MUTUAL;
    sVspmInitParam.type = VSPM_TYPE_FDP_AUTO;
    sVspmInitParam.par.fdp = &sVspmInitFdp;

    rt_code = vspm_init_driver(&pVspmHandle, &sVspmInitParam);
    if (rt_code != R_VSPM_OK) {
        CNV_LOGGER(CNV_LOG_ERROR, "vspm_init_driver() Failed!! ercd=%d\n", rt_code);
        eResult = CNV_ERROR_FATAL;
    }
    else {
        pCnvpFdpHandle->pVspmModuleHandle = pVspmHandle;
    }

    return eResult;
}

/**
* \brief confirm available buffer
*
* \return 
*   retval CNV_TRUE    can convert
*   retval CNV_FALSE   can't convert
*/
CNV_BOOL CnvPvspmCore_ConfirmAvailableBuffer(
    CNV_PTR pContext /**< [in] context */
)
{
    CNV_BOOL            bCanConvert = CNV_FALSE;

    CNVP_CONTEXT_T         *pCnvpContext;
    CNVP_CMN_HANDLE_T    *pCnvpCmnHdl;
    CNVP_FDP_HANDLE_T      *pFdpHandle;

    CNV_BUFFERHEADER_T *pEmptyBuffer = NULL;
    CNV_BUFFERHEADER_T *pFillBuffer  = NULL;

    if (pContext == NULL) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_FALSE;
    }

    /* cast */
    pCnvpContext   = (CNVP_CONTEXT_T*)pContext;
    pCnvpCmnHdl    = (CNVP_CMN_HANDLE_T*)pCnvpContext->pCnvpCmnHandle;
    pFdpHandle = (CNVP_FDP_HANDLE_T*)pCnvpContext->pCnvpDepHandle;

    if ((pCnvpCmnHdl == NULL) || (pFdpHandle == NULL)) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_FALSE;
    }

    /* Get Buffer */
    (void)CnvPcmn_RemoveBufferToQueue(pCnvpCmnHdl, CNV_EVENT_FILLBUFFER, &pFdpHandle->sConvertFillBuffer);
    pFillBuffer = pFdpHandle->sConvertFillBuffer.pConvertBuffer;
    if (pFillBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] != 0) {
        switch (pCnvpContext->nConvertMode)
        {
        case CNV_PROG:
        case CNV_DEINT_2D_HALFRATE:
        case CNV_DEINT_2D_FULLRATE:
            /* Get Buffer */
            (void)CnvPcmn_RemoveBufferToQueue(pCnvpCmnHdl, CNV_EVENT_EMPTYBUFFER, &pFdpHandle->sConvertEmptyBuffer[0]);
            pEmptyBuffer = pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer;
            bCanConvert  = CnvPvspmCore_CheckBufferFlag(pEmptyBuffer, pFillBuffer);
            break;
        case CNV_DEINT_3D_HALFRATE:
        case CNV_DEINT_3D_FULLRATE:
            /* Get Buffer */
            if (pFdpHandle->nCurInputIndicator == CNVP_BUFFER_CONTROL_RELEASE) {
                if (CNV_BUFFER_NONE == pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag) {
                    if (CNV_DEINT_3D_HALFRATE == pFdpHandle->nConvertMode) {
                        pFdpHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;
                    }
                    /* data copy */
                    (void)Cnvdep_Memcpy(pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer,
                        pFdpHandle->sConvertEmptyBuffer[1].pConvertBuffer, sizeof(CNV_BUFFERHEADER_T));

                    /* mem clear*/
                    (void)Cnvdep_Memset(pFdpHandle->sConvertEmptyBuffer[1].pConvertBuffer, 0, sizeof(CNV_BUFFERHEADER_T));
                    /* Set buffer Flag*/
                    pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag = pFdpHandle->sConvertEmptyBuffer[1].nReleaseFlag;
                    pFdpHandle->sConvertEmptyBuffer[1].nReleaseFlag = CNV_BUFFER_NONE;
                }
            }
            (void)CnvPcmn_RemoveBufferToQueue(pCnvpCmnHdl, CNV_EVENT_EMPTYBUFFER, &pFdpHandle->sConvertEmptyBuffer[1]);
            pEmptyBuffer = pFdpHandle->sConvertEmptyBuffer[1].pConvertBuffer;
            bCanConvert = CnvPvspmCore_CheckBufferFlag(pEmptyBuffer, pFillBuffer);
            if (CNV_FALSE == bCanConvert) {
                pEmptyBuffer = pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer;
                if (CNVP_EOS_FLAG == pEmptyBuffer->nFlag) {
                    bCanConvert = CnvPvspmCore_CheckBufferFlag(pEmptyBuffer, pFillBuffer);
                }
            }
            break;
        default:
            break;
        }
    }
    CNV_LOGGER(CNV_LOG_DEBUG, "bCanConvert = %d", (CNV_U32)bCanConvert);
    return bCanConvert;
}

/**
* \brief start converter process
*/
CNV_ERRORTYPE CnvPvspmCore_Executing (
    CNV_PTR          pContext,       /**< [in] context */
    CNV_SUBERRORTYPE *peSubErrorCode /**< [out] sub error code */
)
{
    CNV_ERRORTYPE       eResult = CNV_ERROR_NONE;
    long                rt_code;
    unsigned long       job_id;
    CNV_STATIC CNV_BOOL bVspmStart = CNV_FALSE;
    CNVP_CONTEXT_T      *pCnvpContext;
    CNVP_FDP_HANDLE_T   *pFdpHandle;

    CNV_BUFFERHEADER_T  *pFirstEmptyBuffer;
    CNV_BUFFERHEADER_T  *pSecondEmptyBuffer;
    CNV_BUFFERHEADER_T  *pFillBuffer;

    /* FDP Param */
    struct fdp_seq_t    sSeqParam;
    struct fdp_pic_t    sPicParam;
    struct fdp_start_t  sStartParam;
    struct fdp_fproc_t  sFprocParam;
    struct fdp_refbuf_t sRefBuffer;
    struct fdp_imgbuf_t sOutputBuffer;
    struct fdp_imgbuf_t sNextBuffer;
    struct fdp_imgbuf_t sCurrentBuffer;
    struct fdp_imgbuf_t sPrevBuffer;
    struct fcp_info_t   sFcpInfo;
    struct vspm_job_t   sVspmJob;

    if (pContext == NULL) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    /* cast */
    pCnvpContext = (CNVP_CONTEXT_T*)pContext;
    pFdpHandle   = (CNVP_FDP_HANDLE_T*)pCnvpContext->pCnvpDepHandle;

    if (pFdpHandle == NULL) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    /* Buffer Data */
    if( (CNV_PROG == pFdpHandle->nConvertMode) ||
        (CNV_DEINT_2D_FULLRATE == pFdpHandle->nConvertMode) ||
        (CNV_DEINT_2D_HALFRATE == pFdpHandle->nConvertMode)) {
        pFirstEmptyBuffer  = (CNV_BUFFERHEADER_T *)pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer;
        pSecondEmptyBuffer = NULL;
    } else {
        pFirstEmptyBuffer  = (CNV_BUFFERHEADER_T *)pFdpHandle->sConvertEmptyBuffer[1].pConvertBuffer;
        pSecondEmptyBuffer = (CNV_BUFFERHEADER_T *)pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer;
    }

    pFillBuffer = (CNV_BUFFERHEADER_T *)pFdpHandle->sConvertFillBuffer.pConvertBuffer;

    /* check buffer infomation */
    eResult = CnvPvspmCore_CheckBufferInfo( pFirstEmptyBuffer, pSecondEmptyBuffer ,pFillBuffer, pCnvpContext->nConvertMode );
    if (CNV_ERROR_NONE == eResult) {
        /* fdp_seq_t */
        (void)Cnvdep_Memset(&sSeqParam,0,sizeof(struct fdp_seq_t));
        sSeqParam.seq_mode              = (unsigned char)pFdpHandle->nVspSeqMode;
        sSeqParam.telecine_mode         = 0;/* FDP_TC_OFF */
        sSeqParam.in_width              = (CNV_U16)pFirstEmptyBuffer->sTopAreaImage.nWidth;
        sSeqParam.in_height             = (unsigned short)(pFirstEmptyBuffer->sTopAreaImage.nHeight);

        /* fdp_pic_t */
        (void)Cnvdep_Memset(&sPicParam,0,sizeof(struct fdp_pic_t));
        sPicParam.picid                = (unsigned long)pFdpHandle->pVspmModuleHandle;
        sPicParam.width                = (unsigned short)pFirstEmptyBuffer->sTopAreaImage.nWidth;
        sPicParam.height               = (unsigned short)pFirstEmptyBuffer->sTopAreaImage.nHeight;

        switch(pCnvpContext->sPluginRule.nInputFormat)
        {
        case CNV_YUV420SP:
            sPicParam.chroma_format   = FDP_YUV420;
            break;
        case CNV_YVU420SP:
            sPicParam.chroma_format   = FDP_YUV420_NV21;
            break;
        case CNV_YUV420P:
        case CNV_YVU420P:
            sPicParam.chroma_format   = FDP_YUV420_YV12;
            break;
        default:
            break;
        }
        /* FDP_TC_FORCED_PULL_DOWN disable */
        sPicParam.progressive_sequence = 0;
        sPicParam.progressive_frame    = 0;
        sPicParam.repeat_first_field   = 0;
        sPicParam.top_field_first      = 0;

        /* Create FDP Input Data */
        (void)Cnvdep_Memset(&sFcpInfo,0,sizeof(struct fcp_info_t));
        (void)Cnvdep_Memset(&sNextBuffer,0,sizeof(struct fdp_imgbuf_t));
        (void)Cnvdep_Memset(&sCurrentBuffer,0,sizeof(struct fdp_imgbuf_t));
        (void)Cnvdep_Memset(&sPrevBuffer,0,sizeof(struct fdp_imgbuf_t));
        CnvPvspmCore_CreatInputData(
                pFdpHandle,
                pFirstEmptyBuffer,
                pSecondEmptyBuffer,
                pCnvpContext->sPluginRule.bBottomFirst,
                pCnvpContext->sPluginRule.nInputFormat,
                &sNextBuffer,      /* output */
                &sCurrentBuffer,   /* output */
                &sPrevBuffer,      /* output */
                &sFcpInfo);        /* output */

        /* fdp_imgbuf_t */
        (void)Cnvdep_Memset(&sOutputBuffer,0,sizeof(struct fdp_imgbuf_t));
        CnvPvspmCore_CreatOutputData(
                pFillBuffer,
                pCnvpContext->sPluginRule.nOutputFormat,
                &sOutputBuffer);    /* output */

        /* fdp_refbuf_t */
        (void)Cnvdep_Memset(&sRefBuffer,0,sizeof(struct fdp_refbuf_t));
        sRefBuffer.next_buf   = &sNextBuffer;
        sRefBuffer.cur_buf    = &sCurrentBuffer;
        sRefBuffer.prev_buf   = &sPrevBuffer;

        if( (sPicParam.width != pFdpHandle->nOpenWidth) || (sPicParam.height != pFdpHandle->nOpenHeight) ){
            pFdpHandle->bFirstInput = CNV_FALSE;
            pFdpHandle->nOpenWidth = sPicParam.width;
            pFdpHandle->nOpenHeight= sPicParam.height;
        }

        /* fdp_fproc_t */
        (void)Cnvdep_Memset(&sFprocParam,0,sizeof(struct fdp_fproc_t));
        if( CNV_FALSE == pFdpHandle->bFirstInput){
            sFprocParam.seq_par         = &sSeqParam;
            pFdpHandle->bFirstInput = CNV_TRUE;
        } else {
            sFprocParam.seq_par         = NULL; 
        }
        sFprocParam.in_pic              = &sPicParam;
        sFprocParam.last_seq_indicator  = 0;
        sFprocParam.current_field       = (unsigned char)pFdpHandle->bField;
        sFprocParam.interpolated_line   = 0;
        sFprocParam.out_buf             = &sOutputBuffer;
        sFprocParam.ref_buf             = &sRefBuffer;
        sFprocParam.fcp_par             = &sFcpInfo;
        sFprocParam.ipc_par             = NULL;

        switch(pCnvpContext->sPluginRule.nOutputFormat)
        {
        case CNV_YUV420SP:
            sFprocParam.out_format   = FDP_YUV420;
            break;
        case CNV_YVU420SP:
            sFprocParam.out_format   = FDP_YUV420_NV21;
            break;
        case CNV_YUV420P:
        case CNV_YVU420P:
            sFprocParam.out_format   = FDP_YUV420_YV12;
            break;
        default:
            break;
        }

        /* fdp_start_t */
        (void)Cnvdep_Memset(&sStartParam,0,sizeof(struct fdp_start_t));
        sStartParam.fdpgo     = FDP_GO;
        sStartParam.fproc_par = &sFprocParam;

        /* vspm_job_t */
        (void)Cnvdep_Memset(&sVspmJob,0,sizeof(struct vspm_job_t));
        sVspmJob.type   = VSPM_TYPE_FDP_AUTO;
        sVspmJob.par.fdp = &sStartParam;
        
        /* Set TimeStamp */
        CnvPvspmCore_SetTimeStamp(pFdpHandle,pFirstEmptyBuffer,pSecondEmptyBuffer,pFillBuffer);

        /* Set Picture ErrorCode */
        CnvPvspmCore_SetPictrueErrorFlag(pFdpHandle, pFirstEmptyBuffer,pSecondEmptyBuffer,pFillBuffer);

        /* Set Release Flag */
        CnvPvspmCore_SetReleaseFlag(pFdpHandle);

        /* Set userpointer */
        if( ( CNV_DEINT_3D_FULLRATE == pCnvpContext->nConvertMode ) || ( CNV_DEINT_3D_HALFRATE == pCnvpContext->nConvertMode ) ){
            CnvPvspmCore_SetUserData(pCnvpContext, pFirstEmptyBuffer,pSecondEmptyBuffer);
        } else {
            CnvPvspmCore_SetUserData(pCnvpContext, pFirstEmptyBuffer,NULL);
        }

        /* VSPM Start */
        bVspmStart = CnvPvspmCore_ConfirmVspmStart(pFdpHandle,sCurrentBuffer.addr, sPrevBuffer.addr , sNextBuffer.addr ,pFirstEmptyBuffer->nFlag );
        if ( bVspmStart == CNV_TRUE ){
            rt_code = vspm_entry_job(pFdpHandle->pVspmModuleHandle, &job_id, VSPM_PRI_STD, &sVspmJob, (void *)pCnvpContext, CnvPvspmCore_Callback);
            if( 0 != rt_code){
                CNV_LOGGER(CNV_LOG_ERROR,"vspm_entry_job rt_code = %d", rt_code);
                eResult = CNV_ERROR_FATAL;
            }
        } else {
           if ((sCurrentBuffer.addr == 0) && (pFirstEmptyBuffer->nFlag == CNVP_EOS_FLAG )
                && ( pFdpHandle->sConvertFillBuffer.nReleaseFlag  == CNV_BUFFER_RELEASE)) {
                pFillBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM]  = 0;
                pFillBuffer->sTopArea.nTopAddress_C_Area0[CNV_BUFFER_AREA_IDX_FMEM] = 0;
                pFillBuffer->sTopArea.nTopAddress_C_Area1[CNV_BUFFER_AREA_IDX_FMEM] = 0;
                pFillBuffer->nTimeStamp = pFirstEmptyBuffer->nTimeStamp;
                pFillBuffer->nErrorFlag = pFirstEmptyBuffer->nErrorFlag;
                pFillBuffer->nFlag = CNVP_EOS_FLAG;
            }
            pFdpHandle->bFirstInput = CNV_FALSE;
            pFdpHandle->bEosFlag    = CNV_FALSE;

            eResult = CnvPcmn_SendEvent((CNV_PTR)pCnvpContext, CNV_EVENT_DONE, NULL, peSubErrorCode);
        }
    }
    return eResult;
}

/**
* \brief get release buffer
*/
CNV_ERRORTYPE CnvPvspmCore_GetReleaseBuffer(
  CNV_PTR pContext,       /**< [in] context */
  CNV_PTR *ppEmptyBuffer, /**< [out] input buffer can release */
  CNV_PTR *ppFillBuffer   /**< [out] output buffer can release */
)
{
    CNVP_CONTEXT_T    *pCnvpContext;
    CNVP_FDP_HANDLE_T *pFdpHandle;

    CNV_BUFFERHEADER_T *pEmptyBufferDone;
    CNV_BUFFERHEADER_T *pFillBufferDone;

    if ((pContext == NULL) || (ppEmptyBuffer == NULL) || (ppFillBuffer == NULL)) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    /* cast */
    pCnvpContext    = (CNVP_CONTEXT_T*)pContext;
    pFdpHandle  = (CNVP_FDP_HANDLE_T*)pCnvpContext->pCnvpDepHandle;

    if (pFdpHandle == NULL) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    pEmptyBufferDone = pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer;
    pFillBufferDone  = pFdpHandle->sConvertFillBuffer.pConvertBuffer;

    /* Check EmptyBuffer */
    if( pEmptyBufferDone != NULL ){
        if( pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag == CNV_BUFFER_RELEASE ){
            *ppEmptyBuffer = pEmptyBufferDone;
            /* Release Buffer */
            pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag   = CNV_BUFFER_NONE;
        } else {
            *ppEmptyBuffer = NULL;
        }
    }

    /* Check FillBuffer */
    if( pFillBufferDone != NULL ){
        if( pFdpHandle->sConvertFillBuffer.nReleaseFlag == CNV_BUFFER_RELEASE ){
            *ppFillBuffer = pFillBufferDone;

            if( pFillBufferDone->nFlag != CNVP_EOS_FLAG ){
                if( ( pFdpHandle->nConvertMode == CNV_DEINT_2D_FULLRATE ) ||
                    ( pFdpHandle->nConvertMode == CNV_DEINT_3D_FULLRATE ) ){
                    if( pFdpHandle->nCurInputIndicator == CNVP_BUFFER_CONTROL_RELEASE){
                        pCnvpContext->sOutputPicInfo.bFrameExtended    = CNV_FALSE;
                        pCnvpContext->sOutputPicInfo.nFrameExtendedNum = 0;
                    } else {
                        pCnvpContext->sOutputPicInfo.bFrameExtended    = CNV_TRUE;
                        pCnvpContext->sOutputPicInfo.nFrameExtendedNum = 1;
                    }
                } else {
                    pCnvpContext->sOutputPicInfo.bFrameExtended    = CNV_FALSE;
                    pCnvpContext->sOutputPicInfo.nFrameExtendedNum = 0;
                }
            } else {
                /* EOS */
                pCnvpContext->sOutputPicInfo.bFrameExtended    = CNV_FALSE;
                pCnvpContext->sOutputPicInfo.nFrameExtendedNum = 0;
                pCnvpContext->sOutputPicInfo.bEmptyKeepFlag    = CNV_FALSE;
            }
            /* Release Buffer */
            pFdpHandle->sConvertFillBuffer.nReleaseFlag = CNV_BUFFER_NONE;
        } else {
            *ppFillBuffer = NULL;
        }
    }
    return CNV_ERROR_NONE;
}


/**
* \brief get using buffer
*/
CNV_ERRORTYPE CnvPvspmCore_GetUsingBuffer(
    CNV_PTR pContext,       /**< [in] context */
    CNV_PTR *ppEmptyBuffer, /**< [out] using input buffer */
    CNV_PTR *ppFillBuffer   /**< [out] using output buffer */
)
{
    CNVP_CONTEXT_T         *pCnvpContext;
    CNVP_FDP_HANDLE_T      *pFdpHandle;
    CNV_U32                nCnt;

    if ((pContext == NULL) || (ppEmptyBuffer == NULL) || (ppFillBuffer == NULL)) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    /* cast */
    pCnvpContext   = (CNVP_CONTEXT_T*)pContext;
    pFdpHandle = (CNVP_FDP_HANDLE_T*)pCnvpContext->pCnvpDepHandle;

    if (pFdpHandle == NULL) {
        CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
        return CNV_ERROR_FATAL;
    }

    *ppEmptyBuffer = NULL;
    *ppFillBuffer  = NULL;

    /* Check FillBuffer */
    if( pFdpHandle->sConvertFillBuffer.nReleaseFlag != CNV_BUFFER_NONE ){
        *ppFillBuffer = pFdpHandle->sConvertFillBuffer.pConvertBuffer;
        pFdpHandle->sConvertFillBuffer.nReleaseFlag = CNV_BUFFER_NONE;
    }

    /* Check EmptyBuffer */
    for( nCnt = 0; nCnt < CNVP_FDP_QUEUE; nCnt++ ){
        if( pFdpHandle->sConvertEmptyBuffer[nCnt].nReleaseFlag != CNV_BUFFER_NONE ){
            *ppEmptyBuffer = pFdpHandle->sConvertEmptyBuffer[nCnt].pConvertBuffer;
            pFdpHandle->sConvertEmptyBuffer[nCnt].nReleaseFlag = CNV_BUFFER_NONE;
            break;
        }
     }

    pFdpHandle->bEosFlag    = CNV_FALSE;
    pFdpHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;
    pFdpHandle->bFirstInput = CNV_FALSE;
    pFdpHandle->bInterporateTimeStampFlag = CNV_FALSE;

    return CNV_ERROR_NONE;
}

/***************************************************************************
 * local Funciton .
 ***************************************************************************/
/**
* \brief vspm callback function
*/
CNV_STATIC CNV_VOID CnvPvspmCore_Callback(
    unsigned long job_id,   /**< [in] job id */
    long          result,   /**< [in] job result */
    void *        user_data /**< [in] user data */
)
{
    CNVP_CONTEXT_T      *pCnvpContext;
    CNVP_FDP_HANDLE_T   *pFdpHandle;

    (void)job_id; /* unused parameter */

    pCnvpContext    = (CNVP_CONTEXT_T*)user_data;
    if(pCnvpContext != NULL){
       pFdpHandle = (CNVP_FDP_HANDLE_T*)pCnvpContext->pCnvpDepHandle;
       if(pFdpHandle != NULL){
           if(result == R_VSPM_OK){
               (void)CnvPcmn_SendEvent( (CNV_PTR)pCnvpContext, CNV_EVENT_DONE, NULL, NULL);
           } else {
               CNV_LOGGER(CNV_LOG_ERROR,"CnvPvspmCore_Callback wResult = %d", result);
               (void)CnvPcmn_SendEvent( (CNV_PTR)pCnvpContext, CNV_EVENT_FATAL, NULL, NULL);
           }
       } else {
          CNV_LOGGER(CNV_LOG_ERROR, "Fatal error", "");
       }
    }
    return;
}

/**
* \brief check a intput buffer and a output buffer flag
*/
CNV_STATIC CNV_BOOL CnvPvspmCore_CheckBufferFlag(
    CNV_BUFFERHEADER_T *pEmptyBuffer, /**< [in] input buffer */
    CNV_BUFFERHEADER_T *pFillBuffer   /**< [in] output buffer */
)
{
    CNV_BOOL bBufferCheck = CNV_FALSE;
    
    if( ( pEmptyBuffer != NULL ) && ( pFillBuffer != NULL ) ){
        if( ( pEmptyBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] != 0 ) &&
            ( pFillBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] != 0 ) ){
            bBufferCheck = CNV_TRUE;
        } else {
            if( ( pEmptyBuffer->nFlag == CNVP_EOS_FLAG) && ( pFillBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] != 0 ) ){
                bBufferCheck = CNV_TRUE;
            }
        }
    }
    return bBufferCheck;
}

/**
* \breif check buffers information
*/
CNV_STATIC CNV_ERRORTYPE CnvPvspmCore_CheckBufferInfo(
    CNV_BUFFERHEADER_T *pFirstInBuffer, /**< [in] input buffer */
    CNV_BUFFERHEADER_T *pSecondInBuffer,/**< [in] input buffer */
    CNV_BUFFERHEADER_T *pOutBuffer,     /**< [in] output buffer */
    CNV_U32            nConvertMode     /**< [in] convert mode */
)
{
    CNV_ERRORTYPE eResult = CNV_ERROR_NONE;
    CNV_LOG_MODE eErrorLevel = CNV_LOG_DEBUG;
    CNV_IMAGE_INFO_T *pInTopImage;
    CNV_IMAGE_INFO_T *pInBotImage;
    CNV_IMAGE_INFO_T *pOutTopImage;
    CNV_BUFFER_AREA_INFO_T *pInTopArea;
    CNV_BUFFER_AREA_INFO_T *pOutTopArea;

    if ((pFirstInBuffer == NULL) || (pOutBuffer == NULL)) {
        eResult = CNV_ERROR_FATAL;
    }
    else if ((pSecondInBuffer == NULL) &&
        ((nConvertMode == CNV_DEINT_3D_HALFRATE) || (nConvertMode == CNV_DEINT_3D_FULLRATE))) {
        eResult = CNV_ERROR_FATAL;
    } else if ( pFirstInBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] == 0 ) {
        eResult  = CNV_ERROR_NONE;
    }
    else {

        pInTopImage = &(pFirstInBuffer->sTopAreaImage);
        pInBotImage = &(pFirstInBuffer->sBottomAreaImage);
        pInTopArea  = &(pFirstInBuffer->sTopArea);

        pOutTopImage = &(pOutBuffer->sTopAreaImage);
        pOutTopArea  = &(pOutBuffer->sTopArea);

        /* Input buffer */
        if ((pInTopImage->nWidth  > CNVP_FDP_MAX_IMAGE_WIDTH) ||
            (pInTopImage->nWidth  < CNVP_FDP_MIN_IMAGE_WIDTH) ||
            (pInTopImage->nWidth  > pInTopArea->nStride_Y_Area) ||
            (pInTopImage->nWidth  > pInTopArea->nStride_C_Area) ||
            (pInTopImage->nHeight > CNVP_FDP_MAX_IMAGE_HEIGHT) ||
            (pInTopImage->nHeight < (CNVP_FDP_MIN_IMAGE_HEIGHT/2)) ||
            (pInTopImage->nHeight > pInTopArea->nHeight_Y_Area) ||
            ((pInTopImage->nHeight/2) > pInTopArea->nHeight_C_Area)) {
            eResult = CNV_ERROR_FATAL;
        }
        if (nConvertMode != CNV_PROG) {
            if ((pInTopImage->nWidth != pInBotImage->nWidth) ||
                (pInTopImage->nHeight != pInBotImage->nHeight)) {
                eResult = CNV_ERROR_FATAL;
            }
        }
        /* Output buffer */
        if ((pOutTopImage->nWidth  > CNVP_FDP_MAX_IMAGE_WIDTH) ||
            (pOutTopImage->nWidth  < CNVP_FDP_MIN_IMAGE_WIDTH) ||
            (pOutTopImage->nWidth  > pOutTopArea->nStride_Y_Area) ||
            (pOutTopImage->nHeight > CNVP_FDP_MAX_IMAGE_HEIGHT) ||
            (pOutTopImage->nHeight < CNVP_FDP_MIN_IMAGE_HEIGHT) ||
            (pOutTopImage->nHeight > pOutTopArea->nHeight_Y_Area) ||
            ((pOutTopImage->nHeight/2) > pOutTopArea->nHeight_C_Area)) {
            eResult = CNV_ERROR_FATAL;
        }
        if ((pOutTopArea->nStride_Y_Area != pOutTopArea->nStride_C_Area) &&
            ((pOutTopImage->nWidth / 2) > pOutTopArea->nStride_C_Area)) {
            eResult = CNV_ERROR_FATAL;
        }

        if (eResult == CNV_ERROR_FATAL) {
            eErrorLevel = CNV_LOG_ERROR;
        }
        CNV_LOGGER(eErrorLevel, "Input buffer nWidth = %d, nHeigh = %d, nStride_Y = %d, nSliceHeigt_Y = %d, nStride_C = %d, nSliceHeigt_C = %d",
            pInTopImage->nWidth, pInTopImage->nHeight, pInTopArea->nStride_Y_Area, pInTopArea->nHeight_Y_Area, pInTopArea->nStride_C_Area, pInTopArea->nHeight_C_Area);
        CNV_LOGGER(eErrorLevel, "Output buffer nWidth = %d, nHeigh = %d, nStride_Y = %d, nSliceHeigt_Y = %d, nStride_C = %d, nSliceHeigt_C = %d",
            pOutTopImage->nWidth, pOutTopImage->nHeight, pOutTopArea->nStride_Y_Area, pOutTopArea->nHeight_Y_Area, pOutTopArea->nStride_C_Area, pOutTopArea->nHeight_C_Area);
    }
    return eResult;
}


/**
* \brief set input data
*/
CNV_STATIC CNV_VOID CnvPvspmCore_SetInputData(
    struct fdp_imgbuf_t *pConvertBuffer, /**< [out] fdp imgbuf */
    CNV_BUFFERHEADER_T  *pBuffer,        /**< [in] input buffer */
    CNV_BOOL            bInputMode,      /**< [in] set buffer type */
    CNV_BOOL            bBottomFirst,    /**< [in] current field */
    CNV_U32             nInputFormat,    /**< [in] input color format */
    CNV_U32             *pAncBufferY,    /**< [out] anc buffer y */
    CNV_U32             *pAncBufferC     /**< [out] anc buffer c */
)
{
    CNV_BUFFER_AREA_INFO_T *pBufferInfo;

    if (bBottomFirst == CNV_FALSE) {
        pBufferInfo = &pBuffer->sTopArea;
    } else {
        pBufferInfo = &pBuffer->sBottomArea;
    }

    if (bInputMode == CNV_TRUE) {
        pConvertBuffer->addr = pBufferInfo->nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM];
        if (nInputFormat == CNV_YVU420P) {
            pConvertBuffer->addr_c0 = pBufferInfo->nTopAddress_C_Area1[CNV_BUFFER_AREA_IDX_FMEM];
            pConvertBuffer->addr_c1 = pBufferInfo->nTopAddress_C_Area0[CNV_BUFFER_AREA_IDX_FMEM];
        } else {
            pConvertBuffer->addr_c0 = pBufferInfo->nTopAddress_C_Area0[CNV_BUFFER_AREA_IDX_FMEM];
            pConvertBuffer->addr_c1 = pBufferInfo->nTopAddress_C_Area1[CNV_BUFFER_AREA_IDX_FMEM];
        }
        if (pAncBufferY != NULL) {
            *pAncBufferY = pBufferInfo->nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_ANC];
        }
        if (pAncBufferC != NULL) {
            *pAncBufferC = pBufferInfo->nTopAddress_C_Area0[CNV_BUFFER_AREA_IDX_ANC];
        }
        pConvertBuffer->stride = (CNV_U16)pBuffer->sTopArea.nStride_Y_Area;
        pConvertBuffer->stride_c = (CNV_U16)pBuffer->sTopArea.nStride_C_Area;
    } else {
        /* NULL Set*/
        pConvertBuffer->addr     = 0;
        pConvertBuffer->addr_c0  = 0;
        pConvertBuffer->addr_c1  = 0;
        pConvertBuffer->stride   = 0;
        pConvertBuffer->stride_c = 0;
    }

    return;
}


/**
* \brief indicate current field
*/
CNV_STATIC CNV_VOID CnvPvspmCore_SetCurrentFieldIndicator(
    CNVP_FDP_HANDLE_T *pFdpHandle, /**< [in] plugin handle */
    CNV_BOOL          bBottomFirst,    /**< [in] filed type */ 
    CNV_BOOL          bUseFirstInput  /**< [in] input type */
)
{
    if( bUseFirstInput == CNV_TRUE ){
        if( bBottomFirst == CNV_TRUE ){
            pFdpHandle->bField = CNV_TRUE;
        } else {
            pFdpHandle->bField = CNV_FALSE;
        }
    } else {
        if( bBottomFirst == CNV_TRUE){
            pFdpHandle->bField = CNV_FALSE;
        } else {
            pFdpHandle->bField = CNV_TRUE;
        }
    }
    return;
}


/**
* \brief create fdp input bufffer
*/
CNV_STATIC CNV_VOID CnvPvspmCore_CreatInputData(
    CNVP_FDP_HANDLE_T   *pFdpHandle,     /**< [in] plugin handle */
    CNV_BUFFERHEADER_T  *pFirstEmptyBuffer,  /**< [in] input buffer */
    CNV_BUFFERHEADER_T  *pSecondEmptyBuffer, /**< [in] input buffer */
    CNV_BOOL            bBottomFirst,        /**< [in] field type */
    CNV_U32             nInputFormat,        /**< [in] color format */
    struct fdp_imgbuf_t *pNextBuffer,        /**< [out] next buffer */
    struct fdp_imgbuf_t *pCurrentBuffer,     /**< [out] current buffer */
    struct fdp_imgbuf_t *pPrevBuffer,        /**< [out] previous buffer */
    struct fcp_info_t   *pFcp_Info           /**< [out] fcp information */
)
{
    switch(pFdpHandle->nConvertMode)
    {
    case CNV_PROG:
    case CNV_DEINT_2D_HALFRATE:
        CnvPvspmCore_SetInputData(pCurrentBuffer, pFirstEmptyBuffer,CNV_TRUE,  bBottomFirst, nInputFormat, &pFcp_Info->ba_anc_cur_y, &pFcp_Info->ba_anc_cur_c);
        CnvPvspmCore_SetInputData(pPrevBuffer,    pFirstEmptyBuffer,CNV_FALSE, bBottomFirst, nInputFormat, NULL, NULL);
        CnvPvspmCore_SetInputData(pNextBuffer,    pFirstEmptyBuffer,CNV_FALSE, bBottomFirst, nInputFormat, NULL, NULL);
        CnvPvspmCore_SetCurrentFieldIndicator(pFdpHandle,bBottomFirst,CNV_TRUE);
        break;
    case CNV_DEINT_2D_FULLRATE:
        if( pFdpHandle->nCurInputIndicator == CNVP_BUFFER_CONTROL_KEEP ){
            CnvPvspmCore_SetInputData(pCurrentBuffer, pFirstEmptyBuffer,CNV_TRUE,  bBottomFirst, nInputFormat, &pFcp_Info->ba_anc_cur_y, &pFcp_Info->ba_anc_cur_c);
            CnvPvspmCore_SetInputData(pPrevBuffer,    pFirstEmptyBuffer,CNV_FALSE, bBottomFirst, nInputFormat, NULL, NULL);
            CnvPvspmCore_SetInputData(pNextBuffer,    pFirstEmptyBuffer,CNV_FALSE, bBottomFirst, nInputFormat, NULL, NULL);
            CnvPvspmCore_SetCurrentFieldIndicator(pFdpHandle,bBottomFirst,CNV_TRUE);
        } else {
            CnvPvspmCore_SetInputData(pCurrentBuffer, pFirstEmptyBuffer,CNV_TRUE,  !bBottomFirst, nInputFormat, &pFcp_Info->ba_anc_cur_y, &pFcp_Info->ba_anc_cur_c);
            CnvPvspmCore_SetInputData(pPrevBuffer,    pFirstEmptyBuffer,CNV_FALSE, !bBottomFirst, nInputFormat, NULL, NULL);
            CnvPvspmCore_SetInputData(pNextBuffer,    pFirstEmptyBuffer,CNV_FALSE, !bBottomFirst, nInputFormat, NULL, NULL);
            CnvPvspmCore_SetCurrentFieldIndicator(pFdpHandle,bBottomFirst,CNV_FALSE);
        }
        pFdpHandle->nCurInputIndicator++;
        break;
    case CNV_DEINT_3D_HALFRATE:
        CnvPvspmCore_SetInputData(pPrevBuffer,    pSecondEmptyBuffer,CNV_TRUE, !bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_prev_y, NULL);
        CnvPvspmCore_SetInputData(pCurrentBuffer, pFirstEmptyBuffer ,CNV_TRUE , bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_cur_y, &pFcp_Info->ba_anc_cur_c);
        CnvPvspmCore_SetInputData(pNextBuffer,    pFirstEmptyBuffer,CNV_TRUE,  !bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_next_y, NULL);
        CnvPvspmCore_SetCurrentFieldIndicator(pFdpHandle,bBottomFirst,CNV_TRUE);
        pFdpHandle->nCurInputIndicator++;
        if( pPrevBuffer->addr == 0 ){
            CnvPvspmCore_SetInputData(pPrevBuffer,    pFirstEmptyBuffer,CNV_TRUE, !bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_prev_y, NULL);
        }
        break;
    case CNV_DEINT_3D_FULLRATE:
        if( pFdpHandle->nCurInputIndicator == CNVP_BUFFER_CONTROL_KEEP ){
            CnvPvspmCore_SetInputData(pPrevBuffer,    pSecondEmptyBuffer,CNV_TRUE,  !bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_prev_y, NULL);
            CnvPvspmCore_SetInputData(pCurrentBuffer, pFirstEmptyBuffer, CNV_TRUE,  bBottomFirst,nInputFormat,  &pFcp_Info->ba_anc_cur_y, &pFcp_Info->ba_anc_cur_c); 
            CnvPvspmCore_SetInputData(pNextBuffer,    pFirstEmptyBuffer, CNV_TRUE,  !bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_next_y, NULL);
            CnvPvspmCore_SetCurrentFieldIndicator(pFdpHandle,bBottomFirst,CNV_TRUE);
            if( pPrevBuffer->addr == 0 ){
                CnvPvspmCore_SetInputData(pPrevBuffer,    pFirstEmptyBuffer,CNV_TRUE, !bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_prev_y, NULL);
            } else {
                pFdpHandle->bReleaseFlag = CNV_TRUE;
            }
            pFdpHandle->nCurInputIndicator++;
        } else {
            CnvPvspmCore_SetInputData(pPrevBuffer,    pSecondEmptyBuffer    ,CNV_TRUE,  bBottomFirst,nInputFormat,  &pFcp_Info->ba_anc_prev_y, NULL);
            CnvPvspmCore_SetInputData(pCurrentBuffer, pSecondEmptyBuffer    ,CNV_TRUE,  !bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_cur_y, &pFcp_Info->ba_anc_cur_c);
            CnvPvspmCore_SetInputData(pNextBuffer,    pFirstEmptyBuffer     ,CNV_TRUE,  bBottomFirst,nInputFormat,  &pFcp_Info->ba_anc_next_y, NULL);
            CnvPvspmCore_SetCurrentFieldIndicator(pFdpHandle,bBottomFirst,CNV_FALSE);
            pFdpHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;
            if( pNextBuffer->addr == 0 ){
                CnvPvspmCore_SetInputData(pNextBuffer,    pSecondEmptyBuffer,CNV_TRUE,  bBottomFirst,nInputFormat, &pFcp_Info->ba_anc_next_y, NULL);
            }
        }
        break;
    default:
        break;
    }
    /* FCP setting */
    if( pFdpHandle->bFcnlCompression == CNV_TRUE ){
        pFcp_Info->fcnl      = FCP_FCNL_ENABLE;
    } else {
        pFcp_Info->fcnl      = FCP_FCNL_DISABLE;
    }
    pFcp_Info->pos_y         = 0;
    pFcp_Info->pos_c         = 0;
    pFcp_Info->ba_ref_prev_y = pPrevBuffer->addr;
    pFcp_Info->ba_ref_cur_y  = pCurrentBuffer->addr;
    pFcp_Info->ba_ref_next_y = pNextBuffer->addr;
    pFcp_Info->ba_ref_cur_c  = pCurrentBuffer->addr_c0;
    pFcp_Info->stride_div16  = pCurrentBuffer->stride/16;
    /* DEBUG */
    if( pFdpHandle->bTlen == CNV_TRUE ){
        pFcp_Info->tlen      = FCP_TL_ENABLE;
    } else {
        pFcp_Info->tlen      = FCP_TL_DISABLE;
    }
    return;
}

/**
* \brief create fdp output buffer
*/
CNV_STATIC CNV_VOID CnvPvspmCore_CreatOutputData(
    CNV_BUFFERHEADER_T  *pFillBuffer,    /**< [in] input buffer */
    CNV_U32             nOutputFormat,   /**< [in] color format */
    struct fdp_imgbuf_t *pOutputBuffer   /**< [out] output handle */
)
{
    pOutputBuffer->addr = pFillBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM];
    if (nOutputFormat == CNV_YVU420P) {
        pOutputBuffer->addr_c0 = pFillBuffer->sTopArea.nTopAddress_C_Area1[CNV_BUFFER_AREA_IDX_FMEM];
        pOutputBuffer->addr_c1 = pFillBuffer->sTopArea.nTopAddress_C_Area0[CNV_BUFFER_AREA_IDX_FMEM];
    }
    else {
        pOutputBuffer->addr_c0 = pFillBuffer->sTopArea.nTopAddress_C_Area0[CNV_BUFFER_AREA_IDX_FMEM];
        pOutputBuffer->addr_c1 = pFillBuffer->sTopArea.nTopAddress_C_Area1[CNV_BUFFER_AREA_IDX_FMEM];
    }
    pOutputBuffer->stride = (CNV_U16)pFillBuffer->sTopArea.nStride_Y_Area;
    pOutputBuffer->stride_c = (CNV_U16)pFillBuffer->sTopArea.nStride_C_Area;

    return;
}

/**
* \brief confirm vspm start
*/
CNV_STATIC CNV_BOOL CnvPvspmCore_ConfirmVspmStart(
    CNVP_FDP_HANDLE_T *pFdpHandle, /**< [in] plugin handle */
    CNV_U32 nCurrentAddr,          /**< [in] current address */
    CNV_U32 nPrevAddr,             /**< [in] previous address */
    CNV_U32 nNextAddr,             /**< [in] next address */
    CNV_U32 nFlags                 /**< [in] flags */
)
{
    CNV_BOOL bVspmStart;

    if ((pFdpHandle->nConvertMode == CNV_DEINT_3D_HALFRATE) || (pFdpHandle->nConvertMode == CNV_DEINT_3D_FULLRATE)) {
        /* 3DIP conversion */
        if (( nCurrentAddr !=  0 ) && ( nPrevAddr != 0 ) && ( nNextAddr != 0 )) {
            if (pFdpHandle->bEosFlag == CNV_FALSE) {
                bVspmStart = CNV_TRUE;
            }
            else {
                bVspmStart = CNV_FALSE;
            }
        }
        else {
            bVspmStart = CNV_FALSE;
        }
    } else {
        /* progressive or 2DIP conversion */
        if (( nCurrentAddr != 0 ) && ( nFlags != CNVP_EOS_FLAG )) {
            bVspmStart = CNV_TRUE;
        }
        else {
            bVspmStart = CNV_FALSE;
        }
    }
    CNV_LOGGER(CNV_LOG_DEBUG, "bVspmStart=%d: pPrevAddr = %x, pCurrentAddr = %x, pNextAddr = %x",bVspmStart, nPrevAddr, nCurrentAddr, nNextAddr);
    return bVspmStart;
}


/**
* \brief set to released flag in local buffer
*/
CNV_STATIC CNV_VOID CnvPvspmCore_SetReleaseFlag(
    CNVP_FDP_HANDLE_T *pFdpHandle /**< [in] plugin handle */
)
{
    CNV_U32 nTopAddr    = 0;
    CNV_U32 nBottomAddr = 0;

    nTopAddr    = pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM];
    nBottomAddr = pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer->sBottomArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM];
    
    switch(pFdpHandle->nConvertMode)
    {
    case CNV_PROG:
    case CNV_DEINT_2D_HALFRATE:
        pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag = CNV_BUFFER_RELEASE; 
        pFdpHandle->sConvertFillBuffer.nReleaseFlag     = CNV_BUFFER_RELEASE;
        break;
    case CNV_DEINT_2D_FULLRATE:
        if( pFdpHandle->nCurInputIndicator == CNVP_BUFFER_CONTROL_2D_FULLRATE_RELEASE ){
            pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag = CNV_BUFFER_RELEASE; 
            pFdpHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;
        } else {
            /* NULL + EOS */
            if( pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer->nFlag == CNVP_EOS_FLAG ){
                if( ( nTopAddr == 0 ) && ( nBottomAddr == 0 ) ){
                    pFdpHandle->bEosFlag = CNV_TRUE;
                    pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag = CNV_BUFFER_RELEASE; 
                    pFdpHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;
                }
            }
        }
        pFdpHandle->sConvertFillBuffer.nReleaseFlag     = CNV_BUFFER_RELEASE;
        break;
    case CNV_DEINT_3D_HALFRATE:
        if( ( nTopAddr != 0 ) && ( nBottomAddr !=  0 ) ){
            pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag = CNV_BUFFER_RELEASE;
            if( pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer->nFlag != CNVP_EOS_FLAG ){
                pFdpHandle->sConvertFillBuffer.nReleaseFlag     = CNV_BUFFER_RELEASE;                
            }
        } else {
            if( ( pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer->nFlag == CNVP_EOS_FLAG ) && ( nTopAddr == 0 ) ){
                /* NULL + EOS only input */
                pFdpHandle->bEosFlag = CNV_TRUE;
                pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag = CNV_BUFFER_RELEASE;
                pFdpHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;
            } else {
                pFdpHandle->sConvertFillBuffer.nReleaseFlag     = CNV_BUFFER_RELEASE;
            }
        }
        break;
    case CNV_DEINT_3D_FULLRATE:
        if( ( nTopAddr != 0 ) && ( nBottomAddr != 0 ) ){
            if( CNV_TRUE == pFdpHandle->bReleaseFlag ){
                pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag = CNV_BUFFER_RELEASE;
                pFdpHandle->bReleaseFlag = CNV_FALSE;
            }
            
            if( pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer->nFlag == CNVP_EOS_FLAG ){
                pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag  = CNV_BUFFER_RELEASE;
            }    
            pFdpHandle->sConvertFillBuffer.nReleaseFlag     = CNV_BUFFER_RELEASE;
        } else {
            if( ( pFdpHandle->sConvertEmptyBuffer[0].pConvertBuffer->nFlag == CNVP_EOS_FLAG ) && ( nTopAddr == 0 ) ){
                /* NULL + EOS only input */
                pFdpHandle->bEosFlag = CNV_TRUE;
                pFdpHandle->sConvertEmptyBuffer[0].nReleaseFlag = CNV_BUFFER_RELEASE;
                pFdpHandle->nCurInputIndicator = CNVP_BUFFER_CONTROL_KEEP;
            } else {
                pFdpHandle->sConvertFillBuffer.nReleaseFlag     = CNV_BUFFER_RELEASE;
            }
        }
        
        break;
    default:
        break;
    }
}

/**
* \brief set a user data to common context
*/
CNV_STATIC CNV_VOID CnvPvspmCore_SetUserData(
    CNVP_CONTEXT_T     *pCnvpContext,      /**< [in] context */
    CNV_BUFFERHEADER_T *pFirstEmptyBuffer, /**< [in] first buffer */
    CNV_BUFFERHEADER_T *pSecondEmptyBuffer /**< [in] second buffer */
)
{
    CNVP_FDP_HANDLE_T  *pFdpHandle;
    CNV_BUFFERHEADER_T *pFillBuffer;
    /* cast */
    pFdpHandle = (CNVP_FDP_HANDLE_T*)pCnvpContext->pCnvpDepHandle;    
    pFillBuffer    = pFdpHandle->sConvertFillBuffer.pConvertBuffer;
    /* Mode dep */
    switch(pCnvpContext->nConvertMode)
    {
    case CNV_PROG:
    case CNV_DEINT_2D_HALFRATE:
    case CNV_DEINT_3D_HALFRATE:
        pCnvpContext->sOutputPicInfo.pEmptyUserPointer = pFirstEmptyBuffer->pUserPointer;
        pCnvpContext->sOutputPicInfo.bEmptyKeepFlag    = CNV_FALSE;
        pFillBuffer->pExtendedInfo = pFirstEmptyBuffer->pExtendedInfo;
        break;
    case CNV_DEINT_2D_FULLRATE:
        pCnvpContext->sOutputPicInfo.pEmptyUserPointer = pFirstEmptyBuffer->pUserPointer;
        pFillBuffer->pExtendedInfo = pFirstEmptyBuffer->pExtendedInfo;
        if( pFdpHandle->nCurInputIndicator == CNVP_BUFFER_CONTROL_RELEASE ){
            pCnvpContext->sOutputPicInfo.bEmptyKeepFlag    = CNV_TRUE;
        } else {
            pCnvpContext->sOutputPicInfo.bEmptyKeepFlag    = CNV_FALSE;
        }
        break;
    case CNV_DEINT_3D_FULLRATE:
        if( pFdpHandle->nCurInputIndicator == CNVP_BUFFER_CONTROL_RELEASE ){
            pCnvpContext->sOutputPicInfo.pEmptyUserPointer = pFirstEmptyBuffer->pUserPointer;
            pCnvpContext->sOutputPicInfo.bEmptyKeepFlag    = CNV_TRUE;
            pFillBuffer->pExtendedInfo = pFirstEmptyBuffer->pExtendedInfo;
        } else {
            pCnvpContext->sOutputPicInfo.pEmptyUserPointer = pSecondEmptyBuffer->pUserPointer;
            pCnvpContext->sOutputPicInfo.bEmptyKeepFlag    = CNV_FALSE;
            pFillBuffer->pExtendedInfo = pSecondEmptyBuffer->pExtendedInfo;
        }
        break;
    default:
        break;
    }
}

/**
* \brief check error flags
*/
CNV_STATIC CNV_BOOL CnvPvspmCore_CheckErrorFlag(
    CNV_BUFFERHEADER_T *pBuffer /**< [in] input buffer */
)
{
    CNV_BOOL bResult = CNV_FALSE;
    if( ( pBuffer->nErrorFlag & CNVP_MASK_FLAG ) != 0 ){
        bResult = CNV_TRUE;
    } 
    return bResult;
}

/**
* \brief set error flags
*/
CNV_STATIC CNV_BOOL CnvPvspmCore_SetErrorFlag(
    CNV_BUFFERHEADER_T *pInputBuffer, /**< [in] input buffer */
    CNV_BUFFERHEADER_T *pOutputBuffer /**< [in] output buffer */
)
{
    CNV_BOOL bResult = CNV_FALSE;
    
    pOutputBuffer->nErrorFlag = 0;
    if( CnvPvspmCore_CheckErrorFlag(pInputBuffer) == CNV_TRUE ){
        pOutputBuffer->nErrorFlag = pInputBuffer->nErrorFlag;
        bResult = CNV_TRUE;
    }
    return bResult;
}


/**
* \brief set picture flag
*/
CNV_STATIC CNV_VOID CnvPvspmCore_SetPictrueErrorFlag(
    CNVP_FDP_HANDLE_T  *pFdpHandle, /**< [in] input buffer */
    CNV_BUFFERHEADER_T *pCurrentBuffer,  /**< [in] cyrrebt buffer */
    CNV_BUFFERHEADER_T *pPrevBuffer,  /**< [in] prev buffer */
    CNV_BUFFERHEADER_T *pFillBuffer   /**< [in] output buffer */
)
{
    switch(pFdpHandle->nConvertMode)
    {
    case CNV_PROG:
    case CNV_DEINT_2D_HALFRATE:
    case CNV_DEINT_2D_FULLRATE:
        if( CnvPvspmCore_SetErrorFlag(pCurrentBuffer, pFillBuffer) == CNV_TRUE ){
            CNV_LOGGER(CNV_LOG_WARN,"%x", pCurrentBuffer->nErrorFlag);
        }
        break;
    case CNV_DEINT_3D_HALFRATE:
    case CNV_DEINT_3D_FULLRATE:
        if( CnvPvspmCore_SetErrorFlag(pCurrentBuffer, pFillBuffer) == CNV_TRUE ){
            CNV_LOGGER(CNV_LOG_WARN,"%x", pCurrentBuffer->nErrorFlag);
        } else {
            if( CnvPvspmCore_SetErrorFlag(pPrevBuffer, pFillBuffer) == CNV_TRUE ){
                CNV_LOGGER(CNV_LOG_WARN,"%x", pPrevBuffer->nErrorFlag);
            }
        }
        break;
    default:
        break;
    }
    return;
}

/**
* get second frame timestamp
*/
CNV_STATIC CNV_TICKS CnvPvspmCore_GetSecondFrameTimeStamp(
	CNV_BUFFERHEADER_T *pBuffer /**< [in] buffer */
)
{
	CNV_U32 nAddTimeStamp;
    CNVP_EXTEND_INFORMATION_T *pExtendInfo;
    pExtendInfo = (CNVP_EXTEND_INFORMATION_T*)pBuffer->pExtendedInfo;
    
    if( pExtendInfo != NULL ){
        if( pExtendInfo->struct_size != sizeof(CNVP_EXTEND_INFORMATION_T) ){
            /* Set default Time Rate(Full Rate) */
            nAddTimeStamp = CNVP_TIME_DEFAULT;
        } else {
            switch(pExtendInfo->frame_rate_code){
            case CNVP_FRAME_RATE_CODE_FORBIDDEN:
                /* Set default Frame Rate(Full Rate) */
                nAddTimeStamp = CNVP_TIME_DEFAULT;
                break;
            case CNVP_FRAME_RATE_CODE_23_976:
                nAddTimeStamp = CNVP_TIME_RATE_23_976;
                break;
            case CNVP_FRAME_RATE_CODE_24:
                nAddTimeStamp = CNVP_TIME_RATE_24;
                break;
            case CNVP_FRAME_RATE_CODE_25:
                nAddTimeStamp = CNVP_TIME_RATE_25;
                break;
            case CNVP_FRAME_RATE_CODE_29_97:
                nAddTimeStamp = CNVP_TIME_RATE_29_97;
                break;
            case CNVP_FRAME_RATE_CODE_30:
                nAddTimeStamp = CNVP_TIME_RATE_30;
                break;
            case CNVP_FRAME_RATE_CODE_50:
                nAddTimeStamp = CNVP_TIME_RATE_50;
                break;
            case CNVP_FRAME_RATE_CODE_59_94:
                nAddTimeStamp = CNVP_TIME_RATE_59_94;
                break;
            case CNVP_FRAME_RATE_CODE_60:
                nAddTimeStamp = CNVP_TIME_RATE_60;
                break;
            case CNVP_FRAME_RATE_CODE_EXTENDED:
                if( ( pExtendInfo->frame_rate_n == 0 ) || ( pExtendInfo->frame_rate_d == 0 ) ){
                    nAddTimeStamp = CNVP_TIME_DEFAULT;
                }else{
                    nAddTimeStamp = (pExtendInfo->frame_rate_d*CNVP_TIME_OFFSET)/pExtendInfo->frame_rate_n/2;
                }
                break;
            default:
                nAddTimeStamp = CNVP_TIME_DEFAULT;
                break;
            }
        }
    } else {
        nAddTimeStamp = CNVP_TIME_DEFAULT;
    }
    return (pBuffer->nTimeStamp + (CNV_TICKS)nAddTimeStamp);
}

/**
* \brief get interpolate time stamp
*/
CNV_STATIC CNV_TICKS CnvPvspmCore_GetInterpolateTimeStamp
(
    CNVP_FDP_HANDLE_T  *pFdpHandle,  /**< [in] plugin handle */
    CNV_BUFFERHEADER_T *pBuffer      /**< [in] buffer */
)
{
    CNV_TICKS nInterpolateTimeStamp;

    if (pFdpHandle->bDisableInterpolateTimeStamp == CNV_TRUE) {
        nInterpolateTimeStamp = pBuffer->nTimeStamp + pFdpHandle->nTimeStampOffset;
    } else {
        nInterpolateTimeStamp = CnvPvspmCore_GetSecondFrameTimeStamp(pBuffer);
    }
    return nInterpolateTimeStamp;
}

/**
* \brief set time stamp
*/
CNV_STATIC CNV_VOID CnvPvspmCore_SetTimeStamp(
    CNVP_FDP_HANDLE_T  *pFdpHandle,     /**< [in] plugin handle */
    CNV_BUFFERHEADER_T *pCurrentBuffer, /**< [in] input buffer */
    CNV_BUFFERHEADER_T *pPrevBuffer,    /**< [in] input buffer */
    CNV_BUFFERHEADER_T *pFillBuffer     /**< [in] output buffer */
)
{
    switch(pFdpHandle->nConvertMode)
    {
    case CNV_PROG:
    case CNV_DEINT_2D_HALFRATE:
    case CNV_DEINT_3D_HALFRATE:
        pFillBuffer->nTimeStamp = pCurrentBuffer->nTimeStamp;
        break;
    case CNV_DEINT_2D_FULLRATE:
    case CNV_DEINT_3D_FULLRATE:
        if( pFdpHandle->bInterporateTimeStampFlag == CNV_FALSE ){
            pFillBuffer->nTimeStamp = pCurrentBuffer->nTimeStamp;
            pFdpHandle->bInterporateTimeStampFlag = CNV_TRUE;
        }else{
            if (pFdpHandle->nConvertMode == CNV_DEINT_3D_FULLRATE) {
                pFillBuffer->nTimeStamp = CnvPvspmCore_GetInterpolateTimeStamp(pFdpHandle, pPrevBuffer);
            } else {
                pFillBuffer->nTimeStamp = CnvPvspmCore_GetInterpolateTimeStamp(pFdpHandle, pCurrentBuffer);
            }
            pFdpHandle->bInterporateTimeStampFlag = CNV_FALSE;
        }
        break;
    default:
        break;
    }
    return;
}
