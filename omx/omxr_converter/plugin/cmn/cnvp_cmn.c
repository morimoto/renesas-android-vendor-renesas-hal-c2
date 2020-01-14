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
 * OMX Converter plugin common function
 *
 * \file cnvp_cmn.c
 * \attention
 */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "cnv_type.h"

#include "cnvp_cmn.h"
#include "cnv_osdep.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/

CNV_STATIC CNV_BOOL CnvPcmn_ConfirmAvailableCmd(
    CNVP_CONTEXT_T* pCnvpCmnContext,
	CNVP_THREAD_COMMAND eCommand
);

CNV_STATIC CNV_VOID CnvPcmn_SetCmnThreadState(
    CNVP_CONTEXT_T* pCnvpCmnContext,
    CNV_U32 eState
);

CNV_STATIC CNV_VOID CnvPcmn_EventDone(
    CNVP_CONTEXT_T* pCnvpContext,
    CNV_CALLBACK_EVENTTYPE eEventId,
    CNV_ERRORTYPE eResult,
    CNV_SUBERRORTYPE nSubErrorCode
);

CNV_STATIC CNV_VOID CnvPcmn_SetCmnThreadTimeout(
    CNVP_CONTEXT_T *pCnvpContext
);

CNV_STATIC CNV_VOID CnvPcmn_SetCmnThreadTimeStampOffset(
    CNVP_CONTEXT_T *pCnvpContext
);

CNV_STATIC CNV_VOID CnvPcmn_ClearQueParameter(
    CNVP_CMN_HANDLE_T* pCnvpCmnHdl
);

CNV_STATIC CNV_VOID CnvPcmn_ConvertMainThreadInit(
    CNVP_CONTEXT_T *pCnvpContext
);

CNV_STATIC CNV_VOID CnvPcmn_InitConvertModule(
	CNVP_CONTEXT_T *pCnvpContext, /**< [in] common context */
	CNV_PTR pData
);

CNV_STATIC CNV_BOOL CnvPcmn_GetAContinueEventId(
	CNVP_CONTEXT_T*pCnvpContext,
	CNV_U32 *nEventId
);

CNV_STATIC CNV_VOID CnvPcmn_ConvertMainThread(
    CNV_PTR pHandle,
    CNV_U32 nEventId,
    CNV_PTR pData
);

CNV_STATIC CNV_ERRORTYPE CnvPcmn_StartApiEvent(
    CNVP_CONTEXT_T *pCnvpContext,
    CNVP_EVENTTYPE eEventId,
    CNV_PTR        pData,
    CNVP_THREAD_COMMAND *pCommand
);

CNV_STATIC CNV_ERRORTYPE CnvPcmn_StartThreadCommand(
    CNVP_CONTEXT_T *pCnvpContext,
    CNVP_THREAD_COMMAND eCommand
);

/***************************************************************************/
/*    Variables                                                            */
/***************************************************************************/

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/

/**
* \brief create common handle
*/
CNV_ERRORTYPE CnvPcmn_CreateHandle(
    CNV_PTR *pCnvpCmnHandle  /**< [out] created handle */
)
{
    CNVP_CMN_HANDLE_T        *pHandle;
    
    pHandle = (CNVP_CMN_HANDLE_T *)Cnvdep_Malloc(sizeof(CNVP_CMN_HANDLE_T));
    if( pHandle == NULL ){
        return CNV_ERROR_INVALID_PARAMETER;
    }
    
    /* Set handle */
    *pCnvpCmnHandle = (CNV_PTR)pHandle;
    
    return CNV_ERROR_NONE;
}

/**
* \brief release common handle
*/
CNV_VOID CnvPcmn_DeleteHandle(
    CNV_PTR pCnvpCmnHandle /**< [in] release handle */
)
{
    Cnvdep_Free(pCnvpCmnHandle);
}

/**
* \brief create common thread
*/
CNV_ERRORTYPE CnvPcmn_CreateCmnThread(
    CNV_PTR          pContext,       /**< [in] common context */
    CNV_SUBERRORTYPE *pSubErrorCode  /**< [out] sub error code */
)
{
    CNV_ERRORTYPE eResult = CNV_ERROR_NONE; 
    CNVP_CONTEXT_T *pCnvpContext;
    
    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE);
    if( pContext == NULL){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_HANDLE);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvpContext = (CNVP_CONTEXT_T*)pContext;

    eResult = Cnvdep_CreateOsDepHandle( &pCnvpContext->nThreadId);
    if( eResult != CNV_ERROR_NONE){
        CNV_LOGGER(CNV_LOG_ERROR,"Cnvdep_CreateOsDepHandle ID = %p",pCnvpContext->nThreadId);
    } else {
        /* Convert Thread */
        eResult = Cnvdep_CreateThread( pCnvpContext->nThreadId, &CnvPcmn_ConvertMainThread, pContext);
        CNV_LOGGER(CNV_LOG_DEBUG,"CnvpCmnCreateCnvertThread ID = %p",pCnvpContext->nThreadId);
        if( eResult != CNV_ERROR_NONE ){
            CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_ABEND);
        }
    }
    return eResult;
}

/**
* \brief terminated common thread
*/
CNV_ERRORTYPE CnvPcmn_CmnThreadJoin(
    CNV_PTR pContext,                /**< [in] common context */
    CNV_SUBERRORTYPE *pSubErrorCode  /**< [out] sub error code */
)
{
    CNV_U32        iret = 0;
    CNVP_CONTEXT_T *pCnvpContext;
    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE);
    if( pContext == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    pCnvpContext = (CNVP_CONTEXT_T*)pContext;

    iret = Cnvdep_ThreadJoin(pCnvpContext->nThreadId, NULL);
    if( iret != 0 ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_ABEND);
    }
    return CNV_ERROR_NONE;
}

/**
* \brief send event to common thread
*/
CNV_ERRORTYPE CnvPcmn_SendEvent(
    CNV_PTR pContext,                /**< [in] common context */
    CNV_U32 eEventId,                /**< [in] event id */
    CNV_PTR pSendParam,              /**< [in] send parameter */
    CNV_SUBERRORTYPE *pSubErrorCode
)
{
    CNV_ERRORTYPE eResult;
    CNVP_CONTEXT_T *pCnvpContext;

    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE);
    if( pContext == NULL ){
        return CNV_ERROR_INVALID_PARAMETER;
    }
    /* Send Event */
    pCnvpContext = (CNVP_CONTEXT_T*)pContext;
    eResult = Cnvdep_SendEvent(pCnvpContext->nThreadId, eEventId, pSendParam);
    if( eResult != CNV_ERROR_NONE ){
        CNV_LOGGER(CNV_LOG_ERROR,"eResult = %d ", (CNV_U32)eResult);
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_ABEND);
    }

    return eResult;
}

/**
* \brief set sub error code
*/
CNV_VOID CnvPcmn_SetSubErrorCode(
    CNV_SUBERRORTYPE *pSubErrorCode, /**< [out] sub error code  */
    CNV_SUBERRORTYPE eErrorCode      /**< [in] error code  */
)
{
    if( pSubErrorCode != NULL ){
        *pSubErrorCode = eErrorCode;
    }
}

/**
* \brief add a buffer to converter thread.
*/
CNV_ERRORTYPE CnvPcmn_AddBufferToQueue(
    CNVP_CMN_HANDLE_T  *pHandle, /**< [in] common handle */
    CNVP_EVENTTYPE     eEventId, /**< [in] event id */
    CNV_BUFFERHEADER_T *pBuffer  /**< [in] input buffer */
)
{
    CNV_U32 nIndex = 0;
    CNV_ERRORTYPE eResult = CNV_ERROR_NONE;
    CNV_BUFFER_QUEUE_T *pInputQueue = NULL;

    if ((pHandle == NULL) || (pBuffer ==NULL)) {
        return CNV_ERROR_FATAL;
    }

    switch( eEventId ){
    case CNV_EVENT_EMPTYBUFFER:
        pInputQueue = &pHandle->sEmptyBufferQueue;
        break;
    case CNV_EVENT_FILLBUFFER:
        pInputQueue = &pHandle->sFillBufferQueue;
        (void)Cnvdep_Memset(&pBuffer->sOutputConvertInfo,0,sizeof(CNV_OUTPUT_PIC_INFO_T));
        break;
    default:
        eResult = CNV_ERROR_FATAL;
        break;
    }

    /* Enqueue */
    if (eResult != CNV_ERROR_FATAL) {
        if(pInputQueue->nCountOfBuffers < CNV_COUNT_QUEUE_ELEMENTS){
            nIndex = pInputQueue->nLastElementIndex;
            /* Set Input Data */
            (void)Cnvdep_Memcpy(&pInputQueue->sBuffer[nIndex], pBuffer,sizeof(CNV_BUFFERHEADER_T));
            pInputQueue->nCountOfBuffers++;
            nIndex++;
            if( nIndex < (CNV_COUNT_QUEUE_ELEMENTS-1) ){
                pInputQueue->nLastElementIndex++;
            } else {
                pInputQueue->nLastElementIndex = 0;
            }
        } else {
            eResult = CNV_ERROR_FATAL;
        }
    }

    Cnvdep_BufferFree((CNV_PTR)pBuffer);

    return eResult;
}


/**
* \brief remove a buffer to converter thread
*/
CNV_ERRORTYPE CnvPcmn_RemoveBufferToQueue(
    CNVP_CMN_HANDLE_T     *pHandle,         /**< [in] common handle */
    CNV_U32               eEventId,         /**< [in] event id */
    CNVP_CONVERT_BUFFER_T *pConvertBuffer   /**< [in] buffer */
)
{
    CNV_U32 nIndex = 0;
    CNV_BUFFER_QUEUE_T *pOutputQuele = NULL;
    CNV_ERRORTYPE eResult = CNV_ERROR_NONE;

    if ((pHandle == NULL) || (pConvertBuffer == NULL)) {
        return CNV_ERROR_FATAL;
    }

    if(pConvertBuffer->nReleaseFlag == CNV_BUFFER_KEEP ){
        return CNV_ERROR_NONE;
    }

    switch( eEventId ){
    case CNV_EVENT_EMPTYBUFFER:
        pOutputQuele = &pHandle->sEmptyBufferQueue;
        break;
    case CNV_EVENT_FILLBUFFER:
        pOutputQuele = &pHandle->sFillBufferQueue;
        break;
    default:
        eResult = CNV_ERROR_FATAL;
        break;
    }

    /* dequeue */
    if (eResult != CNV_ERROR_FATAL) {
        if (pOutputQuele->nCountOfBuffers != 0){
            nIndex = pOutputQuele->nFirstElementIndex;
            (void)Cnvdep_Memcpy(pConvertBuffer->pConvertBuffer ,&pOutputQuele->sBuffer[nIndex],sizeof(CNV_BUFFERHEADER_T));
            (void)Cnvdep_Memset(&pOutputQuele->sBuffer[nIndex],0,sizeof(CNV_BUFFERHEADER_T));
            pConvertBuffer->nReleaseFlag   = CNV_BUFFER_KEEP;

            pOutputQuele->nCountOfBuffers--;
            nIndex++;
            if( nIndex < (CNV_COUNT_QUEUE_ELEMENTS-1) ){
                pOutputQuele->nFirstElementIndex++;
            } else {
                pOutputQuele->nFirstElementIndex = 0;
            }
        }
    }

    return eResult;
}
/***************************************************************************
* local Funciton .
***************************************************************************/
/**
* \brief confirm available command
*/
CNV_STATIC CNV_BOOL CnvPcmn_ConfirmAvailableCmd(
    CNVP_CONTEXT_T *pCnvpCmnContext,  /**< [in] common context */
	CNVP_THREAD_COMMAND eCommand           /**< [in] command */
)
{
    CNV_U32  nCurrentState;
    CNV_BOOL bResult = CNV_FALSE;

    nCurrentState = pCnvpCmnContext->nThreadState;
    switch( eCommand )
    {
    case CNV_COMMAND_NONE:
        /* Thread Fatal */
        break;
    case CNV_COMMAND_CONVERT:
        if(( nCurrentState != CNVP_STATE_FATAL ) && ( nCurrentState == CNVP_STATE_IDLE )  ){
            bResult = CNV_TRUE;
        }
        break;
    case CNV_COMMAND_DONE:
        if(( nCurrentState != CNVP_STATE_FATAL ) &&  ( ( nCurrentState == CNVP_STATE_EXECUTING ) || ( nCurrentState == CNVP_STATE_WAITING ) ) )
        {
            bResult = CNV_TRUE;
        }
        break;
    case CNV_COMMAND_FLUSHING:
        if(( CNVP_STATE_FATAL != nCurrentState ) && ( nCurrentState == CNVP_STATE_IDLE )){
            bResult = CNV_TRUE;
        }
        break;
    case CNV_COMMAND_CLOSE:
        if(( nCurrentState == CNVP_STATE_IDLE ) || ( nCurrentState == CNVP_STATE_FATAL ) ){
            bResult = CNV_TRUE;
        }
        break;
    case CNV_COMMAND_WAITING:
        if( nCurrentState == CNVP_STATE_WAITING ){
            bResult = CNV_TRUE;
        }
        break;
    case CNV_COMMAND_TIMEOUT:
        if( nCurrentState == CNVP_STATE_WAITING ){
            bResult = CNV_TRUE;
        }
        break;
    default:
        break;
    }
    return bResult;
}

/**
* \brief set common thread state.
*/
CNV_STATIC CNV_VOID CnvPcmn_SetCmnThreadState(
    CNVP_CONTEXT_T *pCnvpCmnContext, /**< [in] common context */
    CNV_U32        eState            /**< [in] state */
)
{
    pCnvpCmnContext->nThreadState = eState;
    return;
}

/**
* \brief event callback
*/
CNV_STATIC CNV_VOID CnvPcmn_EventDone(
    CNVP_CONTEXT_T         *pCnvpContext, /**< [in] common context */
    CNV_CALLBACK_EVENTTYPE eEventId,      /**< [in] event id */
    CNV_ERRORTYPE          eResult,       /**< [in] function result */
    CNV_SUBERRORTYPE       nSubErrorCode  /**< [in] sub error code */
)
{
    CNV_PTR pUserPtr;
    CNV_PTR pLocalParam;

    /* Callback parameter */
    pLocalParam = pCnvpContext->sPluginlocalEvent.hCnvHandle;
    pUserPtr    = pCnvpContext->pUserPointer;

    if( pCnvpContext->bFatalFlag == CNV_FALSE ){
        switch(eEventId)
        {
        case CNV_CALLBACK_EVENT_FATAL:
            pCnvpContext->bFatalFlag = CNV_TRUE;
            CnvPcmn_SetCmnThreadState( pCnvpContext, CNVP_STATE_FATAL);
            CNV_LOGGER(CNV_LOG_ERROR,"CNV_EVENT_FATAL eResult = %x nSubErrorCode = %x", eResult,(CNV_U32)nSubErrorCode);
            break;
        default:
            break;
        }
        /* local Event */
        pCnvpContext->sPluginlocalEvent.PlugIn_CallBackEvent(pLocalParam,eEventId);
        /* callback Event */
        pCnvpContext->sCallbackFunc.CNV_EventDone(pUserPtr,(CNV_U32)eEventId,eResult,nSubErrorCode);
    }
    return;
}

/**
* \brief set timeout
*/
CNV_STATIC CNV_VOID CnvPcmn_SetCmnThreadTimeout(
    CNVP_CONTEXT_T *pCnvpContext  /**< [in] common context */
)
{
    CNV_PTR  pExtParam;
    CNV_BOOL bResult;

    OMXR_MC_VIDEO_SUBMODULE_TIMEOUT_T *pTimeParam;

    /* Default */
    pCnvpContext->sTimeoutInfo.nWaitTime  = CNVP_THREAD_WAIT_TIME;
    pCnvpContext->sTimeoutInfo.nWaitCount = 0;
    pCnvpContext->sTimeoutInfo.nTimeout   = CNVP_THREAD_TIMEOUT;

    pCnvpContext->bDisableTimeout = CNV_FALSE;

    /* check set parameter */
    bResult = pCnvpContext->sExtensionInfo.CNVP_GetExtensionParameter(
        pCnvpContext->sExtensionInfo.pParameterList,
        "OMX.RENESAS.INDEX.PARAM.VIDEO.CONVERTER.TIMEOUT",
        &pExtParam);

    if( bResult == CNV_TRUE){
        pTimeParam = (OMXR_MC_VIDEO_SUBMODULE_TIMEOUT_T*)pExtParam;
        if( pTimeParam->sHeader.nStructSize == sizeof(OMXR_MC_VIDEO_SUBMODULE_TIMEOUT_T) ){
            if( ( pTimeParam->nWaitTime != 0 ) && ( pTimeParam->nWaitTime < CNVP_THREAD_WAIT_TIME ) ){
                pCnvpContext->sTimeoutInfo.nWaitTime = pTimeParam->nWaitTime;
            }
            if( pTimeParam->nTimeout == 0 ){
                pCnvpContext->bDisableTimeout = CNV_TRUE;
            } else {
                if( (pTimeParam->nTimeout - CNVP_MODULE_TIMEOUT) > 0 ){
                    pCnvpContext->sTimeoutInfo.nTimeout   =  pTimeParam->nTimeout - CNVP_MODULE_TIMEOUT;
                } else {
                    pCnvpContext->sTimeoutInfo.nTimeout   =  CNVP_MODULE_TIMEOUT;
                }
            }
        }
    }
    CNV_LOGGER(CNV_LOG_DEBUG,"Timeout disable = %x",(CNV_U32)pCnvpContext->bDisableTimeout);
    return;
}

/**
* \brief set timestamp offset
*/
CNV_STATIC CNV_VOID CnvPcmn_SetCmnThreadTimeStampOffset(
    CNVP_CONTEXT_T *pCnvpContext  /**< [in] common context */
)
{
    CNV_PTR  pExtParam;
    CNV_BOOL bResult;

    OMXR_MC_VIDEO_DEINTERLACEMODE_T *pDeinterlaceParam;

    /* Default */
    pCnvpContext->bDisableInterpolateTimeStamp = CNV_FALSE;
    pCnvpContext->nTimeStampOffset             = 16666;

    /* check set parameter */
    bResult = pCnvpContext->sExtensionInfo.CNVP_GetExtensionParameter(
        pCnvpContext->sExtensionInfo.pParameterList,
        "OMX.RENESAS.INDEX.PARAM.VIDEO.CONVERTER.DEINITERLACEMODE",
        &pExtParam);

    if (bResult == CNV_TRUE) {
        pDeinterlaceParam = (OMXR_MC_VIDEO_DEINTERLACEMODE_T*)pExtParam;
        if ((pDeinterlaceParam->nDeinterlace == CNV_DEINT_2D_FULLRATE) ||
            (pDeinterlaceParam->nDeinterlace == CNV_DEINT_3D_FULLRATE)) {
            pCnvpContext->bDisableInterpolateTimeStamp = pDeinterlaceParam->bDisableInterpolateTimeStamp;
            pCnvpContext->nTimeStampOffset             = pDeinterlaceParam->nTimeStampOffset;
        } else {
            pCnvpContext->bDisableInterpolateTimeStamp = CNV_FALSE;
            pCnvpContext->nTimeStampOffset             = 0;
        }
    }
    CNV_LOGGER(CNV_LOG_DEBUG,"bDisableInterpolateTimeStamp = %d, nTimeStampOffset = %d", pCnvpContext->bDisableInterpolateTimeStamp,pCnvpContext->nTimeStampOffset);
    return;
}

/**
* \brief clear que paramter
*/
CNV_STATIC CNV_VOID CnvPcmn_ClearQueParameter(
    CNVP_CMN_HANDLE_T* pCnvpCmnHdl /**< [in] que handle */
)
{
    CNV_U32 nCnt;

    /* EmptyBuffer */
	pCnvpCmnHdl->sEmptyBufferQueue.nQueueSize = CNV_COUNT_QUEUE_ELEMENTS;
	pCnvpCmnHdl->sEmptyBufferQueue.nFirstElementIndex = 0;
	pCnvpCmnHdl->sEmptyBufferQueue.nLastElementIndex = 0;
	pCnvpCmnHdl->sEmptyBufferQueue.nCountOfBuffers = 0;
    /* FillBuffer */
	pCnvpCmnHdl->sFillBufferQueue.nQueueSize = CNV_COUNT_QUEUE_ELEMENTS;
	pCnvpCmnHdl->sFillBufferQueue.nFirstElementIndex = 0;
	pCnvpCmnHdl->sFillBufferQueue.nLastElementIndex = 0;
	pCnvpCmnHdl->sFillBufferQueue.nCountOfBuffers = 0;

    for (nCnt = 0; nCnt < (CNV_COUNT_QUEUE_ELEMENTS - 1); nCnt++) {
		pCnvpCmnHdl->sEmptyBufferQueue.pBuffer[nCnt] = NULL;
		pCnvpCmnHdl->sFillBufferQueue.pBuffer[nCnt] = NULL;
    }
    return;
}

/**
* \brief initialize main thread
*/
CNV_STATIC CNV_VOID CnvPcmn_ConvertMainThreadInit(
    CNVP_CONTEXT_T *pCnvpContext /**< [in] common context */
)
{
    CNVP_CMN_HANDLE_T *pQueHdl;

    pQueHdl = (CNVP_CMN_HANDLE_T*)pCnvpContext->pCnvpCmnHandle;
    /* Context Init */
    pCnvpContext->nThreadState = CNVP_STATE_IDLE;
    pCnvpContext->bFlushFlag = CNV_FALSE;
    pCnvpContext->bCloseFlag = CNV_FALSE;
    pCnvpContext->bFatalFlag = CNV_FALSE;
    pCnvpContext->bModuleInit = CNV_FALSE;
    pCnvpContext->bThreadExit = CNV_FALSE;
    pCnvpContext->bContinueConversion = CNV_FALSE;

    /* Convert Info */
    pCnvpContext->sOutputPicInfo.bFrameExtended = CNV_FALSE;
    pCnvpContext->sOutputPicInfo.nFrameExtendedNum = 0;
    pCnvpContext->sOutputPicInfo.pEmptyUserPointer = 0;
    pCnvpContext->sOutputPicInfo.bEmptyKeepFlag = CNV_FALSE;

    /* Clear param */
    CnvPcmn_ClearQueParameter(pQueHdl);

    /* Timeout Init*/
    CnvPcmn_SetCmnThreadTimeout(pCnvpContext);

    /* Set timestamp offset */
    CnvPcmn_SetCmnThreadTimeStampOffset(pCnvpContext);

    (void)Cnvdep_SaveTreadContext(pCnvpContext->nThreadId, pCnvpContext->nThreadId);

    return;
}

/**
* \brief initialize module
*/
CNV_STATIC CNV_VOID CnvPcmn_InitConvertModule(
    CNVP_CONTEXT_T *pCnvpContext, /**< [in] common context */
    CNV_PTR pData
)
{
    CNV_ERRORTYPE eResult;

    /* Module Init */
    if ( pCnvpContext->bModuleInit == CNV_FALSE ) {
        eResult = pCnvpContext->sPluginFunc.CNVP_ModuleInit(pCnvpContext, (CNV_BUFFERHEADER_T*)pData);
        if ( eResult != CNV_ERROR_NONE) {
            CnvPcmn_EventDone(pCnvpContext, CNV_CALLBACK_EVENT_FATAL, CNV_ERROR_FATAL, CNV_ERROR_SUBMODULE_IP);
        }
        pCnvpContext->bModuleInit = CNV_TRUE;
    }
    return;
}

/**
* \brief Get local event id
*/
CNV_STATIC CNV_BOOL CnvPcmn_GetAContinueEventId(
    CNVP_CONTEXT_T*pCnvpContext,
    CNV_U32 *nEventId
)
{
    CNV_BOOL bContinueEvent = CNV_FALSE;

    if ( pCnvpContext->bContinueConversion == CNV_TRUE) {
        /* Convert continue */
        if ( pCnvpContext->bFlushFlag == CNV_TRUE) {
            *nEventId = CNV_EVENT_FLUSHING;
        } else {
            if ( pCnvpContext->bCloseFlag == CNV_TRUE) {
                *nEventId = CNV_EVENT_CLOSE;
            }
            else {
                *nEventId = CNV_EVENT_NONE;
            }
        }
        bContinueEvent = CNV_TRUE;
    }
    return bContinueEvent;
}

/**
* \brief converter main thread
*/
CNV_STATIC CNV_VOID CnvPcmn_ConvertMainThread(
    CNV_PTR pHandle,  /**< [in] handle */
    CNV_U32 nEventId, /**< [in] event id */
    CNV_PTR pData     /**< [in] input data */
)
{
    CNV_ERRORTYPE     eResult = CNV_ERROR_NONE;
    CNVP_CONTEXT_T    *pCnvpContext;
    CNV_BOOL          bContinueEvent;
    CNV_U32           nContinueEventId;
    CNV_BOOL          bEnableCommnad = CNV_FALSE;
	CNVP_THREAD_COMMAND eCommand       = CNV_COMMAND_NONE;

    pCnvpContext = (CNVP_CONTEXT_T*)pHandle;

    if( nEventId == 0xFFFFFFFFu ){
        CnvPcmn_ConvertMainThreadInit(pCnvpContext);
		nEventId = CNV_EVENT_INIT;
    } else {
        pCnvpContext->bThreadExit = CNV_FALSE;
    }

    while( pCnvpContext->bThreadExit != CNV_TRUE ){

        bContinueEvent = CnvPcmn_GetAContinueEventId(pCnvpContext, &nContinueEventId);
        if (bContinueEvent == CNV_TRUE) {
            nEventId = nContinueEventId;
        }

        eResult = CnvPcmn_StartApiEvent(pCnvpContext, (CNVP_EVENTTYPE)nEventId, pData, &eCommand);
        if (eResult != CNV_ERROR_NONE) {
            bEnableCommnad = CNV_FALSE;
        } else {
            /* Check command */
            bEnableCommnad = CnvPcmn_ConfirmAvailableCmd(pCnvpContext, eCommand);
        }

        if( bEnableCommnad == CNV_TRUE ){
            (void)CnvPcmn_StartThreadCommand(pCnvpContext, eCommand);
        } else {
            pCnvpContext->bContinueConversion = CNV_FALSE;
            eCommand                          = CNV_COMMAND_NONE;
        }

        if( CNV_TRUE != pCnvpContext->bThreadExit ){
            if( pCnvpContext->bContinueConversion == CNV_FALSE ){
                pCnvpContext->bThreadExit = CNV_TRUE;
            } else {
                pCnvpContext->bThreadExit = CNV_FALSE;
            }
        }
    }
    return;
}

/**
* \brief start event
*/
CNV_STATIC CNV_ERRORTYPE CnvPcmn_StartApiEvent(
    CNVP_CONTEXT_T *pCnvpContext,  /**< [in] common context */
    CNVP_EVENTTYPE eEventId,       /**< [in] event id */
    CNV_PTR        pData,          /**< [in] input data */
    CNVP_THREAD_COMMAND *pCommand
)
{
    CNV_ERRORTYPE     eResult = CNV_ERROR_NONE;
    CNVP_CMN_HANDLE_T *pCnvpCmnHdl;

    pCnvpCmnHdl = (CNVP_CMN_HANDLE_T*)pCnvpContext->pCnvpCmnHandle;

    *pCommand = CNV_COMMAND_NONE;

    switch (eEventId)
    {
    case CNV_EVENT_EMPTYBUFFER:
    case CNV_EVENT_FILLBUFFER:
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_BUFFER EVENT = %x", (CNV_U32)eEventId);
        if (eEventId == CNV_EVENT_EMPTYBUFFER) {
            CnvPcmn_InitConvertModule( pCnvpContext, pData );
        }
        /* Add Queue */
        eResult = CnvPcmn_AddBufferToQueue(pCnvpCmnHdl, eEventId, (CNV_BUFFERHEADER_T*)pData);
        if (eResult != CNV_ERROR_NONE) {
            CnvPcmn_EventDone(pCnvpContext, CNV_CALLBACK_EVENT_FATAL, eResult, CNV_ERROR_SUBMODULE_ABEND);
        } else {
            *pCommand = CNV_COMMAND_CONVERT;
        }
        break;
    case CNV_EVENT_DONE:
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_EVENT_DONE EVENT = %x", (CNV_U32)eEventId);
        *pCommand = CNV_COMMAND_DONE;
        break;
    case CNV_EVENT_CLOSE:
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_EVENT_CLOSE EVENT = %x", (CNV_U32)eEventId);
        pCnvpContext->bCloseFlag = CNV_TRUE;
        *pCommand = CNV_COMMAND_CLOSE;
        break;
    case CNV_EVENT_FLUSHING:
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_EVENT_FLUSHING EVENT = %x", (CNV_U32)eEventId);
        pCnvpContext->bFlushFlag = CNV_TRUE;
        *pCommand = CNV_COMMAND_FLUSHING;
        break;
    case CNV_EVENT_FATAL:
        *pCommand = CNV_COMMAND_NONE;
        CNV_LOGGER(CNV_LOG_ERROR, "FATAL EVENT eCommand EVENT = %x", (CNV_U32)eEventId);
        CnvPcmn_EventDone(pCnvpContext, CNV_CALLBACK_EVENT_FATAL, eResult, CNV_ERROR_SUBMODULE_ABEND);
        break;
    case CNV_EVENT_WAITING:
        CNV_LOGGER(CNV_LOG_WARN, "EVENT_WAITING eCommand EVENT = %x", (CNV_U32)eEventId);
        *pCommand = CNV_COMMAND_NONE;
        if (pCnvpContext->nThreadState == CNVP_STATE_WAITING) {
            if (pCnvpContext->sTimeoutInfo.nTimeout == pCnvpContext->sTimeoutInfo.nWaitCount) {
                *pCommand = CNV_COMMAND_TIMEOUT;
            } else {
                if (pCnvpContext->sTimeoutInfo.nWaitCount != 0) {
                    *pCommand = CNV_COMMAND_WAITING;
                }
            }
        }
        break;
    case CNV_EVENT_TIMEOUT:
        /* TIMEOUT Event */
        if (pCnvpContext->nThreadState == CNVP_STATE_EXECUTING) {
            *pCommand = CNV_COMMAND_WAITING;
            CNV_LOGGER(CNV_LOG_WARN, "TIMEOUT EVENT eCommand EVENT = %x", (CNV_U32)eEventId);
            CnvPcmn_SetCmnThreadState(pCnvpContext, CNVP_STATE_WAITING);
        } else {
            *pCommand = CNV_COMMAND_NONE;
        }
        break;
    case CNV_EVENT_NONE:
        *pCommand = CNV_COMMAND_CONVERT;
        break;
    case CNV_EVENT_INIT:
        *pCommand = CNV_COMMAND_NONE;
        break;
    default:
        break;
    }
    return eResult;
}

/**
* \brief Get local event id
*/
CNV_STATIC CNV_ERRORTYPE CnvPcmn_StartThreadCommand(
    CNVP_CONTEXT_T *pCnvpContext,  /**< [in] common context */
    CNVP_THREAD_COMMAND eCommand   /**< [in] command */
)
{
    CNV_ERRORTYPE     eResult = CNV_ERROR_NONE;
    CNV_BOOL          bConfirmConvert;
    CNV_U32           nCnt;
    CNVP_CMN_HANDLE_T *pQueHdl;

    CNV_BUFFERHEADER_T *pEmptyDoneBuffer = NULL;
    CNV_BUFFERHEADER_T *pFillDoneBuffer = NULL;

    CNV_BUFFERHEADER_T sInputDoneBuffer;
    CNV_BUFFERHEADER_T sOutputDoneBuffer;

    CNVP_CONVERT_BUFFER_T sQueueEmptyBufferData;
    CNVP_CONVERT_BUFFER_T sQueueFillBufferData;

    pQueHdl = (CNVP_CMN_HANDLE_T*)pCnvpContext->pCnvpCmnHandle;

    pCnvpContext->bContinueConversion = CNV_FALSE;

    switch (eCommand)
    {
    case CNV_COMMAND_CONVERT:
        /* Converter Executing */
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_COMMAND_CONVERT START thread state = %x", (CNV_U32)pCnvpContext->nThreadState);
        bConfirmConvert = pCnvpContext->sPluginFunc.CNVP_CheckConvertStart((CNV_PTR)pCnvpContext);
        if ((bConfirmConvert == CNV_TRUE) && (pCnvpContext->nThreadState == CNVP_STATE_IDLE)) {
            CnvPcmn_SetCmnThreadState(pCnvpContext, CNVP_STATE_EXECUTING);
            eResult = pCnvpContext->sPluginFunc.CNVP_ConverterExecuting((CNV_PTR)pCnvpContext, NULL);
            if (eResult == CNV_ERROR_FATAL) {
                CNV_LOGGER(CNV_LOG_ERROR, "FATAL COMMAND ID = %x", (CNV_U32)CNV_COMMAND_CONVERT);
                CnvPcmn_EventDone(pCnvpContext, CNV_CALLBACK_EVENT_FATAL, eResult, CNV_ERROR_SUBMODULE_ABEND);
            }
        }
        break;
    case CNV_COMMAND_DONE:
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_COMMAND_DONE START Thread state = %x", (CNV_U32)pCnvpContext->nThreadState);
        pCnvpContext->sPluginFunc.CNVP_CheckReleaseBuffer((CNV_PTR)pCnvpContext, (CNV_PTR*)&pEmptyDoneBuffer, (CNV_PTR*)&pFillDoneBuffer);

        /* Callback */
        if (pEmptyDoneBuffer != NULL) {
            CNV_LOGGER(CNV_LOG_INFO, "CNV_EmptyBufferDone nFlag = %x", pEmptyDoneBuffer->nFlag);
            pCnvpContext->sCallbackFunc.CNV_EmptyBufferDone(pEmptyDoneBuffer, pCnvpContext->pUserPointer, CNV_ERROR_NONE, CNV_SUBERROR_NONE);
            (void)Cnvdep_Memset(pEmptyDoneBuffer, 0, sizeof(CNV_BUFFERHEADER_T));
        }

        if (pFillDoneBuffer != NULL) {
            /* Set to context info */
            CNV_LOGGER(CNV_LOG_INFO, "CNV_FillBufferDone nFlag = %x", pFillDoneBuffer->nFlag);
            pFillDoneBuffer->sOutputConvertInfo = pCnvpContext->sOutputPicInfo;
            pCnvpContext->sCallbackFunc.CNV_FillBufferDone(pFillDoneBuffer, pCnvpContext->pUserPointer, CNV_ERROR_NONE, CNV_SUBERROR_NONE);
            (void)Cnvdep_Memset(pFillDoneBuffer, 0, sizeof(CNV_BUFFERHEADER_T));
        }
        /* Set state */
        CnvPcmn_SetCmnThreadState(pCnvpContext, CNVP_STATE_IDLE);

        /* Check to continue */
        bConfirmConvert = pCnvpContext->sPluginFunc.CNVP_CheckConvertStart((CNV_PTR)pCnvpContext);
        if (bConfirmConvert == CNV_TRUE) {
            pCnvpContext->bContinueConversion = CNV_TRUE;
        } else {
            if ((pCnvpContext->bFlushFlag == CNV_TRUE) || (pCnvpContext->bCloseFlag == CNV_TRUE)) {
                pCnvpContext->bContinueConversion = CNV_TRUE;
            }
        }
        /* reset */
        pCnvpContext->sTimeoutInfo.nWaitCount = 0;
        break;
    case CNV_COMMAND_FLUSHING:
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_EVENT_FLUSHING START thread state = %x", (CNV_U32)pCnvpContext->nThreadState);
        /* local data */
        for (nCnt = 0; nCnt < (CNV_COUNT_QUEUE_ELEMENTS - 1); nCnt++)
        {
            pCnvpContext->sPluginFunc.CNVP_GetFlushBuffer((CNV_PTR)pCnvpContext, (CNV_PTR)&pEmptyDoneBuffer, (CNV_PTR)&pFillDoneBuffer);
            /* EmptyBuffer Done */
            if (pEmptyDoneBuffer != NULL) {
                CNV_LOGGER(CNV_LOG_INFO, "FLUSH CNV_EmptyBufferDone", 0);
                pCnvpContext->sCallbackFunc.CNV_EmptyBufferDone(pEmptyDoneBuffer, pCnvpContext->pUserPointer, CNV_ERROR_NONE, CNV_SUBERROR_NONE);
                (void)Cnvdep_Memset(pEmptyDoneBuffer, 0, sizeof(CNV_BUFFERHEADER_T));
            }
            /* FillBuffer Done */
            if (pFillDoneBuffer != NULL) {
                CNV_LOGGER(CNV_LOG_INFO, "FLUSH CNV_FillBufferDone", 0);
                pCnvpContext->sCallbackFunc.CNV_FillBufferDone(pFillDoneBuffer, pCnvpContext->pUserPointer, CNV_ERROR_NONE, CNV_SUBERROR_NONE);
                (void)Cnvdep_Memset(pFillDoneBuffer, 0, sizeof(CNV_BUFFERHEADER_T));
            }
            if ((pEmptyDoneBuffer == NULL) && (pFillDoneBuffer == NULL)) {
                break;
            }
        }
        /* Queue data */
        for (nCnt = 0; nCnt < (CNV_COUNT_QUEUE_ELEMENTS - 1); nCnt++)
        {
            sQueueEmptyBufferData.pConvertBuffer = &sInputDoneBuffer;
            sQueueEmptyBufferData.nReleaseFlag = CNV_BUFFER_NONE;
            sQueueFillBufferData.pConvertBuffer = &sOutputDoneBuffer;
            sQueueFillBufferData.nReleaseFlag = CNV_BUFFER_NONE;
            /* Get Queue Data */
            (void)CnvPcmn_RemoveBufferToQueue(pQueHdl, CNV_EVENT_EMPTYBUFFER, &sQueueEmptyBufferData);
            (void)CnvPcmn_RemoveBufferToQueue(pQueHdl, CNV_EVENT_FILLBUFFER, &sQueueFillBufferData);

            /* EmptyBuffer Done */
            if (sQueueEmptyBufferData.nReleaseFlag != CNV_BUFFER_NONE) {
                CNV_LOGGER(CNV_LOG_INFO, "FLUSH CNV_EmptyBufferDone", 0);
                pCnvpContext->sCallbackFunc.CNV_EmptyBufferDone(sQueueEmptyBufferData.pConvertBuffer, pCnvpContext->pUserPointer, CNV_ERROR_NONE, CNV_SUBERROR_NONE);
            }
            /* FillBuffer Done */
            if (sQueueFillBufferData.nReleaseFlag != CNV_BUFFER_NONE) {
                CNV_LOGGER(CNV_LOG_INFO, "FLUSH CNV_FillBufferDone", 0);
                pCnvpContext->sCallbackFunc.CNV_FillBufferDone(sQueueFillBufferData.pConvertBuffer, pCnvpContext->pUserPointer, CNV_ERROR_NONE, CNV_SUBERROR_NONE);
            }
            if ((sQueueEmptyBufferData.nReleaseFlag == CNV_BUFFER_NONE) && (sQueueFillBufferData.nReleaseFlag == CNV_BUFFER_NONE)) {
                break;
            }
        }
        /* Flush Event */
        CnvPcmn_EventDone(pCnvpContext, CNV_CALLBACK_EVENT_FLUSHDONE, CNV_ERROR_NONE, CNV_SUBERROR_NONE);

        /* Clear paramerter */
        CnvPcmn_ClearQueParameter(pQueHdl);

        pCnvpContext->bFlushFlag = CNV_FALSE;
        if (pCnvpContext->bCloseFlag == CNV_TRUE) {
            pCnvpContext->bContinueConversion = CNV_TRUE;
        }
        break;
    case CNV_COMMAND_CLOSE:
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_EVENT_CLOSE START thread state = %x", (CNV_U32)pCnvpContext->nThreadState);
        pCnvpContext->bThreadExit = CNV_TRUE;
        Cnvdep_ThreadExit(0);
        break;
    case CNV_COMMAND_WAITING:
        if (0 < pCnvpContext->sTimeoutInfo.nWaitCount) {
            CNV_LOGGER(CNV_LOG_DEBUG, "CNV_EVENT_WAITING START thread state = %x", (CNV_U32)pCnvpContext->nThreadState);
            Cnvdep_Sleep(pCnvpContext->sTimeoutInfo.nWaitTime);
        }
        (void)CnvPcmn_SendEvent((CNV_PTR)pCnvpContext, CNV_EVENT_WAITING, NULL, NULL);
        pCnvpContext->sTimeoutInfo.nWaitCount++;
        break;
    case CNV_COMMAND_TIMEOUT:
        CNV_LOGGER(CNV_LOG_ERROR, "CNV_EVENT_TIMEOUT thread state = %x", (CNV_U32)pCnvpContext->nThreadState);
        (void)CnvPcmn_SendEvent((CNV_PTR)pCnvpContext, CNV_EVENT_FATAL, NULL, NULL);
        break;
    case CNV_COMMAND_NONE:
        CNV_LOGGER(CNV_LOG_DEBUG, "CNV_EVENT_NONE START thread state = %x", (CNV_U32)pCnvpContext->nThreadState);
        pCnvpContext->bContinueConversion = CNV_FALSE;
        break;
    default:
        break;
    }
    return eResult;
}
