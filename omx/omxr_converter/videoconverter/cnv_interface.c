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
 * OMX Converter Interface function
 *
 * \file cnv_interface.c
 * \attention 
 */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "cnv_public.h"
#include "cnv_plugin_cmn.h"
#include "cnv_config.h"
#include "cnv_private.h"

#include "cnv_osdep.h"

#include <string.h>
/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define PLUGIN_IF_FUNCTION_NAME "CNVP_GetPlugInFunc" 

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/
CNV_STATIC CNV_VOID Cnvlocal_SetState(
    CNV_HANDLE_T  *psCnvHandle, 
    CNV_STATETYPE eState
);

CNV_STATIC CNV_BOOL Cnvlocal_ConfirmCurrentState(
    CNV_HANDLE_T  *psCnvHandle,
    CNV_STATETYPE eState
);

CNV_STATIC CNV_VOID Cnvlocal_SetSubErrorCode(
    CNV_SUBERRORTYPE *pSubErrorCode,
    CNV_SUBERRORTYPE eErrorCode
);

CNV_STATIC CNV_VOID Cnvlocal_PluginEventCallback(
    CNV_HANDLE             hCnvHandle,
    CNV_CALLBACK_EVENTTYPE eEventId
);

CNV_STATIC CNV_VOID Cnvlocal_CompareParameter(
    CNV_U32 *nParamA,
    CNV_U32 nParamB
);

CNV_STATIC CNV_ERRORTYPE Cnvlocal_SetParameterList(
    CNV_PTR pList,
    CNV_PTR pParam,
    CNV_U32 nIndexType,
    CNV_PTR *pNewList
);

CNV_STATIC CNV_BOOL Cnvlocal_GetParameterList(
    CNV_PTR pList,
    CNV_U32 nIndexType,
    CNV_PTR pParam);

CNV_STATIC CNV_VOID Cnvlocal_ReleaseParameterList(
    CNV_PTR pList);

CNV_STATIC CNV_BOOL Cnvlocal_SearchParameterList(
    CNV_PTR pList,CNV_U32 nIndexType,
    CNV_PTR *pIndexList);

/***************************************************************************/
/*    Variables                                                            */
/***************************************************************************/

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/
/**
* \brief create the converter handle
*/
CNV_ERRORTYPE  CNV_Initialize(
  CNV_HANDLE           *pHandle,      /**< [in] converter handle */
  CNV_SUBERRORTYPE     *pSubErrorCode /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_U32 nIndex = 0;
    CNV_U32 nCnt   = 0;
    CNV_ERRORTYPE eResult = CNV_ERROR_INVALID_PARAMETER;
    CNV_HANDLE_T  *pCnvHandle;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)pHandle);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    /* Parameter check */
    if ( pHandle == NULL ) {
        CNV_LOGGER(CNV_LOG_ERROR,"Handle failed.= %p",(CNV_PTR)pHandle);
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    *pHandle = (CNV_HANDLE)Cnvdep_Malloc(sizeof(CNV_HANDLE_T));
    if( *pHandle == NULL ){
        CNV_LOGGER(CNV_LOG_ERROR,"Get Handle failed.= %p",(CNV_PTR)pHandle);
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_CREATEHANDLE);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    pCnvHandle = (CNV_HANDLE_T *)*pHandle;

    /* init */
    (void)Cnvdep_Memset(pCnvHandle,0,sizeof(CNV_HANDLE_T));

    /* Create Mutex */
    (void)Cnvdep_CreateMutex(&pCnvHandle->nMutexId);
    (void)Cnvdep_CreateMutex(&pCnvHandle->nStateMutex);

    /* Set State */
    Cnvlocal_SetState(pCnvHandle, CNV_STATE_DEINITIALIZED);
    
    /* Mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);
    /* plugin nIndex */
    eResult = CnvConfig_GetPuluginIndex(&pCnvHandle->nPluginMaxNum , pSubErrorCode);
    if( eResult == CNV_ERROR_NONE ){
        nIndex = pCnvHandle->nPluginMaxNum;
        /* DLL load Info */
        for( nCnt = 0;nCnt < nIndex;nCnt++)
        {        
            eResult = CnvConfig_GetPluginDllInfo(nCnt, &(pCnvHandle->sPluginConfig[nCnt]), pSubErrorCode);
            if( eResult != CNV_ERROR_NONE ){
                goto cleanup;
            }
            CNV_LOGGER(CNV_LOG_DEBUG,"CnvConfigGetPluginDllInfo. Load module = %s",pCnvHandle->sPluginConfig[nCnt].PluginPath);
            
            /* set plugin id*/
            pCnvHandle->sPluginList[nCnt].nPluginId = pCnvHandle->sPluginConfig[nCnt].PluginId;
        }
        /* DLL load */
        for( nCnt = 0;nCnt < nIndex;nCnt++)
        {
            eResult = Cnvdep_LoadDll( pCnvHandle->sPluginConfig[nCnt].PluginPath, &pCnvHandle->sPluginList[nCnt].hPlugin );
            if( eResult != CNV_ERROR_NONE ){
                Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_DLL_LOAD);
                goto cleanup;
            }
        }
        /* Get PlugIn Function */
        for( nCnt = 0;nCnt < nIndex;nCnt++ )
        {
            eResult = Cnvdep_GetDllEntry( pCnvHandle->sPluginList[nCnt].hPlugin , PLUGIN_IF_FUNCTION_NAME , (CNV_PTR*)&pCnvHandle->sPluginList[nCnt].CNVP_GetPlugInFunc);
            if( CNV_ERROR_NONE != eResult ){
                Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_DLL_Entry);
                goto cleanup;
            }
        }
        /* Get Function Table */
        for( nCnt = 0;nCnt < nIndex;nCnt++ )
        {
            eResult = pCnvHandle->sPluginList[nCnt].CNVP_GetPlugInFunc(&(pCnvHandle->sPluginIF[nCnt]),pSubErrorCode);
            if( CNV_ERROR_NONE != eResult ){
                goto cleanup;
            }
        }
    }
    /* Set State */
    Cnvlocal_SetState( pCnvHandle, CNV_STATE_CLOSED);

    /* set local data*/
    pCnvHandle->sEventFunc.hCnvHandle = (CNV_HANDLE)pCnvHandle;
    pCnvHandle->sEventFunc.PlugIn_CallBackEvent = &Cnvlocal_PluginEventCallback;
    pCnvHandle->bCloseFlag = CNV_FALSE;
    pCnvHandle->pParameterList = NULL;

    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;
cleanup:
    /* error release */
    for( nCnt = 0;nCnt < nIndex;nCnt++){
        if( pCnvHandle->sPluginList[nCnt].hPlugin != NULL ){
            eResult = Cnvdep_UnloadDll(pCnvHandle->sPluginList[nCnt].hPlugin);
        }
    }
    Cnvdep_DestroyMutex(pCnvHandle->nMutexId);
    Cnvdep_DestroyMutex(pCnvHandle->nStateMutex);
    Cnvdep_Free(pCnvHandle);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief get a extension index.
*/
CNV_ERRORTYPE CNV_GetExtensionIndex(
  CNV_HANDLE         hProcessing,    /**< [in] converter handle */
  CNV_STRING         cParameterName, /**< [in] name of extension index */
  CNV_INDEXTYPE      *pIndexType,    /**< [out] extension index */
  CNV_SUBERRORTYPE   *pSubErrorCode  /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_U32       nCnt                 = 0;
    CNV_BOOL      bCheck               = CNV_FALSE;
    CNV_ERRORTYPE eResult              = CNV_ERROR_INVALID_PARAMETER;

    CNV_HANDLE_T          *pCnvHandle;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    /* parameter check */
    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if ( cParameterName == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_PARAMETERNAME);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(cParameterName)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if ( pIndexType == NULL ) {
         Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_INDEXTYPE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pIndexType)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    
    pCnvHandle = (CNV_HANDLE_T *)hProcessing;

    /* mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);
    /* Check State */
    bCheck = Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_CLOSED);
    if( bCheck == CNV_FALSE){
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
        CNV_LOGGER(CNV_LOG_ERROR,"Invalid sequence",0);
        /* Unlock mutex */
        Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
        return CNV_ERROR_INVALID_SEQUENCE;
    }

    /* Get Exetentionindex */
    for( nCnt = 0 ;nCnt < pCnvHandle->nPluginMaxNum ;nCnt++ )
    {
        eResult = pCnvHandle->sPluginIF[nCnt].PlugIn_GetExtensionIndex(cParameterName, pIndexType, pSubErrorCode);
        if( eResult == CNV_ERROR_NONE ){
            CNV_LOGGER(CNV_LOG_DEBUG,"CNV_EXTENSIONINDEX = %p",(CNV_PTR)pIndexType);
            break;
        }
    }
    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief query format
*/
CNV_ERRORTYPE CNV_QueryFormat(
  CNV_HANDLE          hProcessing,   /**< [in] converter handle */
  CNV_U32             nImageFormat,  /**< [in] image format */
  CNV_U32             *pResponse,    /**< [out] response */
  CNV_CONSTRAINTS_T   *pConstraints, /**< [out] constraint */
  CNV_SUBERRORTYPE    *pSubErrorCode /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_U32  nCnt             = 0;
    CNV_BOOL bCheck           = CNV_FALSE;
    CNV_ERRORTYPE eResult     = CNV_ERROR_INVALID_PARAMETER;

    CNV_HANDLE_T          *pCnvHandle;
    CNV_CONSTRAINTS_T     sRtnConstaints;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    /* parameter check */
    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    if ( pResponse == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_RESPONSE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pResponse)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    if ( pConstraints == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_CONSTRAINTS);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pConstraints)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvHandle = (CNV_HANDLE_T *)hProcessing;
    /* mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);
    /* Check State */
    bCheck = Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_CLOSED);
    if( bCheck == CNV_FALSE ){
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
        CNV_LOGGER(CNV_LOG_ERROR,"Invalid sequence",0);
        /* Unlock mutex */
        Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
        return CNV_ERROR_INVALID_SEQUENCE;
    }

    /* Init Constraints */
    pConstraints->nImageCropPosXBoundary               = 0;
    pConstraints->nImageCropPosYBoundary               = 0;
    pConstraints->nImageCropWidthBoundary              = 0;
    pConstraints->nImageCropHeightBoundary             = 0;
    pConstraints->nBufferWidthMultiplesOf              = 0;
    pConstraints->nBufferHeightMultiplesOf             = 0;
    pConstraints->nBufferTopAddressBoundary            = 0;
    pConstraints->nMaxNumberOfBuffersPerProcessing     = 0;

    /* Get Exetentionindex */
    for( nCnt = 0 ;nCnt < pCnvHandle->nPluginMaxNum ;nCnt++ )
    {
        eResult = pCnvHandle->sPluginIF[nCnt].PlugIn_QueryFormat(nImageFormat, pResponse , &sRtnConstaints, pSubErrorCode);
        if( eResult == CNV_ERROR_NONE ){
            /* Compare data */
            Cnvlocal_CompareParameter(&pConstraints->nImageCropPosXBoundary , sRtnConstaints.nImageCropPosXBoundary );
            Cnvlocal_CompareParameter(&pConstraints->nImageCropPosYBoundary , sRtnConstaints.nImageCropPosYBoundary );
            Cnvlocal_CompareParameter(&pConstraints->nImageCropWidthBoundary , sRtnConstaints.nImageCropWidthBoundary );
            Cnvlocal_CompareParameter(&pConstraints->nImageCropHeightBoundary , sRtnConstaints.nImageCropHeightBoundary );
            Cnvlocal_CompareParameter(&pConstraints->nBufferWidthMultiplesOf , sRtnConstaints.nBufferWidthMultiplesOf );
            Cnvlocal_CompareParameter(&pConstraints->nBufferHeightMultiplesOf , sRtnConstaints.nBufferHeightMultiplesOf );
            Cnvlocal_CompareParameter(&pConstraints->nBufferTopAddressBoundary , sRtnConstaints.nBufferTopAddressBoundary );
            Cnvlocal_CompareParameter(&pConstraints->nMaxNumberOfBuffersPerProcessing , sRtnConstaints.nMaxNumberOfBuffersPerProcessing );
        }
    }
    /* CNV_QueryFormat */
    CNV_LOGGER(CNV_LOG_DEBUG,"nImageCropPosXBoundary = %d, nImageCropPosYBoundary = %d, nImageCropWidthBoundary = %d",
        pConstraints->nImageCropPosXBoundary,pConstraints->nImageCropPosYBoundary, pConstraints->nImageCropWidthBoundary);
    CNV_LOGGER(CNV_LOG_DEBUG,"nBufferWidthMultiplesOf = %d, BufferHeightMultiplesOf = %d, nBufferTopAddressBoundary = %d, nMaxNumberOfBuffersPerProcessing = %d",
        pConstraints->nBufferWidthMultiplesOf,pConstraints->nBufferHeightMultiplesOf,pConstraints->nBufferTopAddressBoundary,pConstraints->nMaxNumberOfBuffersPerProcessing);

    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* #brief get parameter
*/
CNV_ERRORTYPE CNV_GetParameter (
  CNV_HANDLE         hProcessing,                    /**< [in] converter handle */
  CNV_INDEXTYPE      nIndexType,                     /**< [in] extension index */
  CNV_PTR            pProcessingParameterStructrure, /**< [out] paramter */ 
  CNV_SUBERRORTYPE   *pSubErrorCode                  /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_U32          nCnt             = 0;
    CNV_BOOL         bCheck           = CNV_FALSE;
    CNV_BOOL         bGetCheck        = CNV_FALSE;
    CNV_ERRORTYPE    eResult          = CNV_ERROR_INVALID_PARAMETER;

    CNV_HANDLE_T      *pCnvHandle;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    /*parameter check*/
    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if ( pProcessingParameterStructrure == NULL) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_PROCESSINGPARAMETERSTRUCTURE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pProcessingParameterStructrure)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvHandle = (CNV_HANDLE_T *)hProcessing;

    /* mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);
    /* Check State */
    bCheck = Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_CLOSED);
    if( bCheck == CNV_FALSE ){
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
        CNV_LOGGER(CNV_LOG_ERROR,"Invalid sequence",0);
        /* Unlock mutex */
        Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
        return CNV_ERROR_INVALID_SEQUENCE;
    }    

    /* Get parameter */
    bGetCheck = Cnvlocal_GetParameterList(pCnvHandle->pParameterList,nIndexType,pProcessingParameterStructrure);
    if( bGetCheck == CNV_FALSE ){
        /* Get default parameter  */
        for( nCnt = 0 ;nCnt < pCnvHandle->nPluginMaxNum ;nCnt++ )
        {
            eResult = pCnvHandle->sPluginIF[nCnt].PlugIn_GetParameter(nIndexType, pProcessingParameterStructrure, pCnvHandle->pParameterList, pSubErrorCode);
            if( CNV_ERROR_NONE == eResult ){
                break;
            }
        }
    } else {
        eResult = CNV_ERROR_NONE;
    }

    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* #brief get parameter
*/
CNV_ERRORTYPE CNV_SetParameter (
  CNV_HANDLE         hProcessing,                    /**< [in] converter handle */
  CNV_INDEXTYPE      nIndexType,                     /**< [in] extension index */
  CNV_PTR            pProcessingParameterStructrure, /**< [in] paramter */
  CNV_SUBERRORTYPE   *pSubErrorCode                  /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_U32          nCnt            = 0;
    CNV_BOOL         bCheck          = CNV_FALSE;
    CNV_ERRORTYPE    eResult         = CNV_ERROR_INVALID_PARAMETER;

    CNV_PTR            pOldList;
    CNV_PTR            pNewList;

    CNV_HANDLE_T          *pCnvHandle;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if ( pProcessingParameterStructrure == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_PROCESSINGPARAMETERSTRUCTURE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pProcessingParameterStructrure)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvHandle = (CNV_HANDLE_T *)hProcessing;

    /* mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);   
    /* Check State */
    bCheck = Cnvlocal_ConfirmCurrentState( pCnvHandle,CNV_STATE_CLOSED );
    if( bCheck == CNV_FALSE ){
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
        CNV_LOGGER(CNV_LOG_ERROR,"Invalid sequence",0);
        /* Unlock mutex */
        Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
        return CNV_ERROR_INVALID_SEQUENCE;
    }

    for( nCnt = 0 ;nCnt < pCnvHandle->nPluginMaxNum ;nCnt++ ){
        /* Check Parameter */
        eResult = pCnvHandle->sPluginIF[nCnt].PlugIn_CheckParameter(nIndexType, pProcessingParameterStructrure,pSubErrorCode);
        if( eResult == CNV_ERROR_NONE ){
            /* Set Parameter */
            pOldList = pCnvHandle->pParameterList;
            eResult = Cnvlocal_SetParameterList(pOldList,pProcessingParameterStructrure,nIndexType,&pNewList);
            if( eResult != CNV_ERROR_NONE){
                CNV_LOGGER(CNV_LOG_ERROR,"malloc error",0);
                Cnvlocal_ReleaseParameterList(pOldList);
            } else {
                pCnvHandle->pParameterList = pNewList;
            }
            break;
        }
    }
    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief create converter main thread
*/
CNV_ERRORTYPE CNV_Open (
  CNV_HANDLE                      hProcessing,             /**< [in] converter handle */
  CNV_PTR                         pUserPointer,            /**< [in] user pointer */
  CNV_SETTING_IMAGECONVERSION_T   *pImageConversionConfig, /**< [in] configuration */
  CNV_CALLBACK_FUNCS_T            *pCallbackFuncs,         /**< [in] callback function */
  CNV_SUBERRORTYPE                *pSubErrorCode           /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_U32  nCnt             = 0;
    CNV_U32  u32PluginId      = 0;
    CNV_BOOL bCheck           = CNV_FALSE;
    CNV_BOOL bPluginCheck     = CNV_FALSE;
    CNV_ERRORTYPE eResult     = CNV_ERROR_INVALID_PARAMETER;

    CNV_HANDLE_T *pCnvHandle;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    /*parameter check*/
    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if ( pImageConversionConfig  == NULL) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pImageConversionConfig)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if ( pCallbackFuncs == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_CALLBACKFUNCS);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pCallbackFuncs)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    } else {
        /* Callback check */
        if ( pCallbackFuncs->CNV_EmptyBufferDone == NULL ) {
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_EMTPYBUFFERDONE);
            CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pCallbackFuncs:CNV_EmptyBufferDone)",0);
            return CNV_ERROR_INVALID_PARAMETER;
        }
         if ( pCallbackFuncs->CNV_FillBufferDone == NULL ) {
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_FILLBUFFERDONE);
            CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pCallbackFuncs:CNV_FillBufferDone)",0);
            return CNV_ERROR_INVALID_PARAMETER;
         }
         if ( pCallbackFuncs->CNV_EventDone == NULL ) {
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_FLUSHBUFFERDONE);
            CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pCallbackFuncs:CNV_EventDone)",0);
            return CNV_ERROR_INVALID_PARAMETER;
         }
    }

    pCnvHandle = (CNV_HANDLE_T *)hProcessing;

    /* lock mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);
    /* Check State */
    bCheck = Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_CLOSED);
    if( bCheck == CNV_FALSE ){
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
        CNV_LOGGER(CNV_LOG_ERROR,"invalid sequence",0);
        /* Unlock mutex */
        Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
        return CNV_ERROR_INVALID_SEQUENCE;
    }

    for( nCnt = 0 ;nCnt < pCnvHandle->nPluginMaxNum ;nCnt++ ){
        /* PlugIn_PlugInSelect */
        eResult = pCnvHandle->sPluginIF[nCnt].PlugIn_PlugInSelect( 
                                pImageConversionConfig, 
                                (CNV_EXTENSIONINDEX_T*)pCnvHandle->pParameterList,
                                pCnvHandle->nPluginMaxNum, 
                                &bPluginCheck, 
                                &pCnvHandle->nConvertMode,
                                pSubErrorCode);
    
        if( bPluginCheck == CNV_TRUE ){
            pCnvHandle->nPluginLocalId = nCnt;
            eResult = CNV_ERROR_NONE;
            break;
        }
    }

    if( bPluginCheck == CNV_FALSE ){
        /* Unspported */
        eResult = CNV_ERROR_INVALID_PARAMETER;
    }
    if( eResult == CNV_ERROR_NONE ){
        u32PluginId = pCnvHandle->nPluginLocalId;
        /* Create Plugin Handle */
        eResult = pCnvHandle->sPluginIF[u32PluginId].PlugIn_CreateHandle( &pCnvHandle->hPluginModule, pCnvHandle->nConvertMode ,pUserPointer, pSubErrorCode);
        if( eResult != CNV_ERROR_NONE ){
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
            CNV_LOGGER(CNV_LOG_ERROR,"plugin handle error ",0);
            /* Unlock mutex */
            Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
            return CNV_ERROR_INVALID_PARAMETER;
        }
        /* Plugin setup */
        eResult = pCnvHandle->sPluginIF[u32PluginId].PlugIn_Open(
                                pCnvHandle->hPluginModule,
                                pCnvHandle->pParameterList,
                                pImageConversionConfig,
                                pCallbackFuncs,
                                &pCnvHandle->sEventFunc, 
                                pSubErrorCode);

        if( eResult == CNV_ERROR_NONE ){
            Cnvlocal_SetState( pCnvHandle, CNV_STATE_OPENED);
            pCnvHandle->bCloseFlag = CNV_TRUE;
        }
    }
    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief input buffer function
*/
CNV_ERRORTYPE CNV_EmptyThisBuffer (
  CNV_HANDLE              hProcessing,   /**< [in] converter handle */
  CNV_BUFFERHEADER_T      *pBuffer,      /**< [in] input buffer */
  CNV_SUBERRORTYPE        *pSubErrorCode /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_BOOL               bCheck = CNV_FALSE;
    CNV_ERRORTYPE          eResult = CNV_ERROR_INVALID_PARAMETER;
    CNV_HANDLE_T           *pCnvHandle;
    CNV_BUFFERHEADER_T     *pEmptyBuffer;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);   

    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if ( pBuffer == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_BUFFER);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pBuffer)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvHandle = (CNV_HANDLE_T *)hProcessing;
    /* mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);
    /* Check State */
    bCheck = Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_OPENED);
    if( bCheck == CNV_FALSE ){
        if( Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_FLUSHING) == CNV_TRUE ){
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
            CNV_LOGGER(CNV_LOG_ERROR,"Already flushing state",0);
            eResult = CNV_ERROR_FLUSHING;
        } else {
            /* INVSLID SEQUENCE */
            eResult = CNV_ERROR_INVALID_SEQUENCE;
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
            CNV_LOGGER(CNV_LOG_ERROR,"invalid sequence",0);
        }
        /* Unlock mutex */
        Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
        return eResult;
    }

    pEmptyBuffer = (CNV_BUFFERHEADER_T *)Cnvdep_BufferMalloc(sizeof(CNV_BUFFERHEADER_T));
    if( pEmptyBuffer != NULL ){
        (void)Cnvdep_Memcpy(pEmptyBuffer,pBuffer,sizeof(CNV_BUFFERHEADER_T));
        if( ( pEmptyBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] != 0 ) ||
			( pEmptyBuffer->sBottomArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] != 0 ) ){
            /* CNVP_EmptyThisBuffer */
            eResult = pCnvHandle->sPluginIF[pCnvHandle->nPluginLocalId].PlugIn_EmptyThisBuffer( pCnvHandle->hPluginModule, pEmptyBuffer, pSubErrorCode);
        } else {
            if( CNV_TRUE == pEmptyBuffer->nFlag){
                /* NULL + EOS */
                eResult = pCnvHandle->sPluginIF[pCnvHandle->nPluginLocalId].PlugIn_EmptyThisBuffer( pCnvHandle->hPluginModule, pEmptyBuffer, pSubErrorCode);               
            } else {
                eResult = CNV_ERROR_INVALID_PARAMETER;
                Cnvdep_BufferFree((CNV_PTR)pEmptyBuffer);
                CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(buffer:NULL)",0);
            }
        }
    } else {
        eResult = CNV_ERROR_FATAL;
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_UNDEFINE);
        CNV_LOGGER(CNV_LOG_ERROR,"malloc error",0);
    }
    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief output buffer function
*/
CNV_ERRORTYPE CNV_FillThisBuffer(
    CNV_HANDLE             hProcessing,   /**< [in] converter handle */
    CNV_BUFFERHEADER_T     *pBuffer,      /**< [in] input buffer */
    CNV_SUBERRORTYPE       *pSubErrorCode /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_BOOL              bCheck = CNV_FALSE;
    CNV_ERRORTYPE         eResult = CNV_ERROR_INVALID_PARAMETER;

    CNV_HANDLE_T          *pCnvHandle;
    CNV_BUFFERHEADER_T    *pFillBuffer;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if ( pBuffer == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_BUFFER);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(pBuffer)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvHandle = (CNV_HANDLE_T *)hProcessing;

    /* Lock mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);
    /* Check State */
    bCheck = Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_OPENED);
    if( bCheck == CNV_FALSE ){
        if( Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_FLUSHING) == CNV_TRUE ){
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
            CNV_LOGGER(CNV_LOG_ERROR,"Already flushing state",0);
            eResult = CNV_ERROR_FLUSHING;
        } else {
            /* INVSLID SEQUENCE */
            eResult = CNV_ERROR_INVALID_SEQUENCE;
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
            CNV_LOGGER(CNV_LOG_ERROR,"invalid sequence",0);
        }
        /* Unlock mutex */
        Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
        return eResult;
    }

    pFillBuffer = (CNV_BUFFERHEADER_T *)Cnvdep_BufferMalloc(sizeof(CNV_BUFFERHEADER_T));
    if( pFillBuffer != NULL ){
        (void)Cnvdep_Memcpy(pFillBuffer,pBuffer,sizeof(CNV_BUFFERHEADER_T));
        if( ( pFillBuffer->sTopArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] != 0 ) ||
            ( pFillBuffer->sBottomArea.nTopAddress_Y_Area[CNV_BUFFER_AREA_IDX_FMEM] != 0 ) ){
            /* CNVP_FillThisBuffer */
            eResult = pCnvHandle->sPluginIF[pCnvHandle->nPluginLocalId].PlugIn_FillThisBuffer( pCnvHandle->hPluginModule, pFillBuffer, pSubErrorCode);
        } else {
            eResult = CNV_ERROR_INVALID_PARAMETER;
            Cnvdep_BufferFree((CNV_PTR)pFillBuffer);
            CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(buffer:NULL)",0);
        }
    } else {
        eResult = CNV_ERROR_FATAL;
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_UNDEFINE);
        CNV_LOGGER(CNV_LOG_ERROR,"malloc error",0);
    }

    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief request Flush process
*/
CNV_ERRORTYPE CNV_CommandFlush (
    CNV_HANDLE           hProcessing,   /**< [in] converter handle */
    CNV_SUBERRORTYPE     *pSubErrorCode /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_BOOL         bCheck = CNV_FALSE;
    CNV_ERRORTYPE    eResult = CNV_ERROR_INVALID_PARAMETER;

    CNV_HANDLE_T     *pCnvHandle;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvHandle = (CNV_HANDLE_T *)hProcessing;

    /* lock mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);

    /* Check State */
    bCheck = Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_OPENED);
    if( bCheck == CNV_TRUE ){
        Cnvlocal_SetState( pCnvHandle, CNV_STATE_FLUSHING);
    } else {
        /* INVSLID SEQUENCE */
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
        CNV_LOGGER(CNV_LOG_ERROR,"invalid sequence",0);
        /* Unlock mutex */
        Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
        return CNV_ERROR_INVALID_SEQUENCE;
    }

    /* CNVP_CommandFlush */
    eResult = pCnvHandle->sPluginIF[pCnvHandle->nPluginLocalId].PlugIn_CommandFlush( pCnvHandle->hPluginModule, pSubErrorCode);
    if( eResult != CNV_ERROR_NONE ){
        return CNV_ERROR_INVALID_PARAMETER;
    }
    /* Unlock mutex */
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return CNV_ERROR_NONE;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief close converter thread
*/
CNV_ERRORTYPE CNV_Close (
    CNV_HANDLE        hProcessing,   /**< [in] converter handle */
    CNV_SUBERRORTYPE  *pSubErrorCode /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_ERRORTYPE    eResult = CNV_ERROR_INVALID_PARAMETER;
    CNV_HANDLE_T     *pCnvHandle;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    /* Close ConverterThread */
    pCnvHandle = (CNV_HANDLE_T *)hProcessing;

    /* Lock mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);

    /* Check State */
    if( Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_OPENED) != CNV_TRUE ){
        if( Cnvlocal_ConfirmCurrentState(pCnvHandle,CNV_STATE_FATAL) != CNV_TRUE ){
            Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);
            CNV_LOGGER(CNV_LOG_ERROR,"invalid sequence",0);
            /* Unlock mutex */
            Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
            return CNV_ERROR_INVALID_SEQUENCE;
        }
    }
    eResult = pCnvHandle->sPluginIF[pCnvHandle->nPluginLocalId].PlugIn_Close( pCnvHandle->hPluginModule, pSubErrorCode);
    if( eResult != CNV_ERROR_NONE ){
        eResult = CNV_ERROR_INVALID_PARAMETER;
    } else {
        /* Close Plugin Handle */
        eResult = pCnvHandle->sPluginIF[pCnvHandle->nPluginLocalId].PlugIn_DeleteHandle( pCnvHandle->hPluginModule );
        if( eResult == CNV_ERROR_NONE ){
            pCnvHandle->bCloseFlag = CNV_FALSE;
            Cnvlocal_SetState( pCnvHandle, CNV_STATE_CLOSED);
        }
    }
    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief release converter handle.
*/
CNV_ERRORTYPE  CNV_Deinitialize(
  CNV_HANDLE       hProcessing,   /**< [in] converter handle */
  CNV_SUBERRORTYPE *pSubErrorCode /**< [out] sub error code */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_U32          nCnt = 0;
    CNV_ERRORTYPE eResult = CNV_ERROR_INVALID_PARAMETER;

    CNV_HANDLE_T  *pCnvHandle;

    CNV_LOGGER(CNV_LOG_INFO,"Handle.= %p",(CNV_PTR)hProcessing);
    Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_SUBERROR_NONE);

    if ( hProcessing == NULL ) {
        Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
         CNV_LOGGER(CNV_LOG_ERROR,"Parameter error(hProcessing)",0);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvHandle = (CNV_HANDLE_T *)hProcessing;

    /* lock mutex */
    Cnvdep_LockMutex(pCnvHandle->nMutexId);

    if( pCnvHandle->bCloseFlag == CNV_TRUE ){
        /* CNV_Close */
        eResult = pCnvHandle->sPluginIF[pCnvHandle->nPluginLocalId].PlugIn_Close( pCnvHandle->hPluginModule, pSubErrorCode);
        if( eResult != CNV_ERROR_NONE ){
            Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
            return CNV_ERROR_INVALID_PARAMETER;
        } else {
             pCnvHandle->bCloseFlag = CNV_FALSE;
            /* Close Plugin Handle */
            pCnvHandle->sPluginIF[pCnvHandle->nPluginLocalId].PlugIn_DeleteHandle( pCnvHandle->hPluginModule );
        }
    }

    /* PlugIn unload */
    for( nCnt = 0;nCnt < pCnvHandle->nPluginMaxNum;nCnt++)
    {
        eResult = Cnvdep_UnloadDll(pCnvHandle->sPluginList[nCnt].hPlugin);
        if( eResult != CNV_ERROR_NONE ){
                Cnvlocal_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_DLL_UNLOAD);
        }
    }

    Cnvlocal_ReleaseParameterList(pCnvHandle->pParameterList);

    Cnvdep_UnlockMutex(pCnvHandle->nMutexId);
    /* Delete Mutex */
	Cnvdep_DestroyMutex( pCnvHandle->nMutexId );
	Cnvdep_DestroyMutex( pCnvHandle->nStateMutex );

    /* Free Handle */
    Cnvdep_Free((CNV_PTR)pCnvHandle);

    return CNV_ERROR_NONE;

#else
    return CNV_ERROR_NONE;
#endif

}

/**
* \brief convert error string from error code
*/
CNV_ERRORTYPE CNV_ErrorCodeToString (
  CNV_S32         nErrorCode,  /**< [in] error code */
  CNV_STRING     *pErrorString /**< [out] error string */
)
{

#ifndef OMXR_DISABLE_VIDEO_CONVERTER

    CNV_ERRORTYPE eResult = CNV_ERROR_NONE;

    switch(nErrorCode)
    {
    /* Error */
    case(CNV_ERROR_NONE):
        *pErrorString = "CNV_ERROR_NONE";
        break;
    case(CNV_ERROR_INVALID_SEQUENCE):
        *pErrorString = "CNV_ERROR_INVALID_SEQUENCE";
        break;
    case(CNV_ERROR_INVALID_PARAMETER):
        *pErrorString = "CNV_ERROR_INVALID_PARAMETER";
        break;
    case(CNV_ERROR_FATAL):
        *pErrorString = "CNV_ERROR_FATAL";
        break;
    case(CNV_ERROR_FLUSHING):
        *pErrorString = "CNV_ERROR_FLUSHING";
        break;
    /* SBUERROR */
    case(CNV_SUBERROR_NONE):
        *pErrorString = "CNV_SUBERROR_NONE";
        break;
    case(CNV_ERROR_SEQUENCE_OUTOFORDER):
        *pErrorString = "CNV_ERROR_SEQUENCE_OUTOFORDER";
        break;
    case(CNV_ERROR_PARAM_CONFIG_PLUGINDATA):
        *pErrorString = "CNV_ERROR_PARAM_CONFIG_PLUGINDATA";
        break;
    case(CNV_ERROR_PARAM_CREATEHANDLE):
        *pErrorString = "CNV_ERROR_PARAM_CREATEHANDLE";
        break;
    case(CNV_ERROR_PARAM_DLL_LOAD):
        *pErrorString = "CNV_ERROR_PARAM_DLL_LOAD";
        break;
    case(CNV_ERROR_PARAM_DLL_UNLOAD):
        *pErrorString = "CNV_ERROR_PARAM_DLL_UNLOAD";
        break;
    case(CNV_ERROR_PARAM_DLL_Entry):
        *pErrorString = "CNV_ERROR_PARAM_DLL_Entry";
        break;
    case(CNV_ERROR_PARAM_GET_FUNCTION):
        *pErrorString = "CNV_ERROR_PARAM_GET_FUNCTION";
        break;
    case(CNV_ERROR_PARAM_PARAMETERNAME):
        *pErrorString = "CNV_ERROR_PARAM_PARAMETERNAME";
        break;
    case(CNV_ERROR_PARAM_INDEXTYPE):
        *pErrorString = "CNV_ERROR_PARAM_INDEXTYPE";
        break;
    case(CNV_ERROR_PARAM_RESPONSE):
        *pErrorString = "CNV_ERROR_PARAM_RESPONSE";
        break;
    case(CNV_ERROR_PARAM_CONSTRAINTS):
        *pErrorString = "CNV_ERROR_PARAM_CONSTRAINTS";
        break;
    case(CNV_ERROR_PARAM_PROCESSINGPARAMETERSTRUCTURE):
        *pErrorString = "CNV_ERROR_PARAM_PROCESSINGPARAMETERSTRUCTURE";
        break;
    case(CNV_ERROR_PARAM_NINPUTIMAGE_COLORFORMAT):
        *pErrorString = "CNV_ERROR_PARAM_NINPUTIMAGE_COLORFORMAT";
        break;
    case(CNV_ERROR_PARAM_UNSELECT_PLUGIN):
        *pErrorString = "CNV_ERROR_PARAM_UNSELECT_PLUGIN";
        break;
    case(CNV_ERROR_PARAM_CALLBACKFUNCS):
        *pErrorString = "CNV_ERROR_PARAM_CALLBACKFUNCS";
        break;
    case(CNV_ERROR_PARAM_EMTPYBUFFERDONE):
        *pErrorString = "CNV_ERROR_PARAM_EMTPYBUFFERDONE";
        break;
    case(CNV_ERROR_PARAM_FILLBUFFERDONE):
        *pErrorString = "CNV_ERROR_PARAM_FILLBUFFERDONE";
        break;
    case(CNV_ERROR_PARAM_FLUSHBUFFERDONE):
        *pErrorString = "CNV_ERROR_PARAM_FLUSHBUFFERDONE";
        break;
    case(CNV_ERROR_PARAM_BUFFER):
        *pErrorString = "CNV_ERROR_PARAM_BUFFER";
        break;
        case(CNV_ERROR_SUBMODULE_PARAM):
        *pErrorString = "CNV_ERROR_SUBMODULE_PARAM";
        break;
    case(CNV_ERROR_SUBMODULE_PARAM_EXTENSIONINDEX):
        *pErrorString = "CNV_ERROR_SUBMODULE_PARAM_EXTENSIONINDEX";
        break;
    case(CNV_ERROR_SUBMODULE_PARAM_CREATE_HANDLE):
        *pErrorString = "CNV_ERROR_SUBMODULE_PARAM_CREATE_HANDLE";
        break;
    case(CNV_ERRPR_SUBMODULE_PARAM_GET_FUNCTION):
        *pErrorString = "CNV_ERRPR_SUBMODULE_PARAM_GET_FUNCTION";
        break;
    case(CNV_ERRPR_SUBMODULE_PARAM_CALLBACKFUNCS):
        *pErrorString = "CNV_ERRPR_SUBMODULE_PARAM_CALLBACKFUNCS";
        break;
    case(CNV_ERROR_SUBMODULE_IP):
        *pErrorString = "CNV_ERROR_SUBMODULE_IP";
        break;
    case(CNV_ERROR_SUBMODULE_ABEND):
        *pErrorString = "CNV_ERROR_SUBMODULE_ABEND";
        break;
    case(CNV_ERROR_SUBMODULE_TIMEOUT):
        *pErrorString = "CNV_ERROR_SUBMODULE_TIMEOUT";
        break;
    case(CNV_ERROR_FLUSHING_RUN):
        *pErrorString = "CNV_ERROR_FLUSHING_RUN";
        break;
    default:
        eResult = CNV_ERROR_INVALID_PARAMETER;
        break;
    }

    return eResult;

#else
    return CNV_ERROR_NONE;
#endif

}

/***************************************************************************
 * local Funciton .
 ***************************************************************************/
/**
* \brief set converter state
*/
CNV_STATIC CNV_VOID Cnvlocal_SetState(
    CNV_HANDLE_T  *psCnvHandle, /**< [in] converter handle */
    CNV_STATETYPE eState        /**< [in] state */
)
{
    /* mutex */
    Cnvdep_LockMutex(psCnvHandle->nStateMutex);
    psCnvHandle->eState = eState;
    CNV_LOGGER(CNV_LOG_DEBUG," Converter Change State = %08x", (CNV_U32)psCnvHandle->eState);
    /* Unlock mutex */
    Cnvdep_UnlockMutex(psCnvHandle->nStateMutex);
}

/**
* \brief confirm current converter state
*/
CNV_STATIC CNV_BOOL Cnvlocal_ConfirmCurrentState(
    CNV_HANDLE_T  *psCnvHandle, /**< [in] converter handle */
    CNV_STATETYPE eState        /**< [in] state */
)
{
    CNV_BOOL bCheck;

    /* mutex */
    Cnvdep_LockMutex(psCnvHandle->nStateMutex);
    bCheck = CNV_FALSE;
    
    if( eState == psCnvHandle->eState ){
        bCheck = CNV_TRUE;
    }
    /* Unlock mutex */
    Cnvdep_UnlockMutex(psCnvHandle->nStateMutex);

    return bCheck;
}

/**
* \brief set sub error code.
*/
CNV_STATIC CNV_VOID Cnvlocal_SetSubErrorCode(
    CNV_SUBERRORTYPE *pSubErrorCode, /**< [out] sub error code */
    CNV_SUBERRORTYPE eErrorCode      /**< [in] error code */
)
{
    if( pSubErrorCode != NULL ){
        *pSubErrorCode = eErrorCode;
    }
}

/**
* \brief compare paramter
*/
CNV_STATIC CNV_VOID Cnvlocal_CompareParameter(
    CNV_U32 *nParamA, /**< [out] parameter */
    CNV_U32 nParamB   /**< [in] parameter */
)
{
    if( *nParamA < nParamB ){
        *nParamA = nParamB;
    }
}

/**
* \brief converter event callback
*/
CNV_STATIC CNV_VOID Cnvlocal_PluginEventCallback(
    CNV_HANDLE             hCnvHandle,  /**< [in] converter handle */
    CNV_CALLBACK_EVENTTYPE eEventId     /**< [in] event id */
)
{
    switch(eEventId){
    case(CNV_CALLBACK_EVENT_FLUSHDONE):
        Cnvlocal_SetState( (CNV_HANDLE_T*)hCnvHandle, CNV_STATE_OPENED );
        break;
    case(CNV_CALLBACK_EVENT_FATAL):
        Cnvlocal_SetState( (CNV_HANDLE_T*)hCnvHandle, CNV_STATE_FATAL );
        break;
    default:
        break;
    }
}

/**
* \brief search parameter list.
*/
CNV_STATIC CNV_BOOL Cnvlocal_SearchParameterList(
    CNV_PTR pList,      /**< [in] list */
    CNV_U32 nIndexType, /**< [in] extension index */
    CNV_PTR *pIndexList /**< [out] list */
)
{
    CNV_BOOL bResult = CNV_FALSE;
    CNV_EXTENSIONINDEX_T *pHead;
    CNV_EXTENSIONINDEX_T *pNextHead;

    pHead = (CNV_EXTENSIONINDEX_T*)pList;

    while(pHead != NULL){
        pNextHead = (CNV_EXTENSIONINDEX_T*)pHead->pNextParam;
        if( pHead->nIndexType == nIndexType ){
            *pIndexList = (CNV_PTR)pHead;
            bResult = CNV_TRUE;
            break;
        }
        pHead = pNextHead;
    }
    return bResult;
}

/**
* \breirf set parameter to list.
*/
CNV_STATIC CNV_ERRORTYPE Cnvlocal_SetParameterList(
    CNV_PTR pList,      /**< [in] list */
    CNV_PTR pParam,     /**< [in] parameter */
    CNV_U32 nIndexType, /**< [in] extension index */
    CNV_PTR *pNewList   /**< [out] new list */
)
{
    CNV_PTR                 *pData;
    CNV_BOOL             bResult;
    CNV_EXTENSIONINDEX_T *pListHead;
    CNV_STRUCT_HEADER_T  *pHeader;
    CNV_ERRORTYPE         eResult = CNV_ERROR_NONE;

    pHeader = (CNV_STRUCT_HEADER_T *)pParam;
    bResult = Cnvlocal_SearchParameterList(pList,nIndexType,(CNV_PTR)&pListHead);
    if( bResult == CNV_TRUE ){
        (void)Cnvdep_Memcpy((CNV_PTR)pListHead->pParameter, (CNV_PTR)pHeader, pHeader->nStructSize);
        *pNewList = (CNV_PTR)pList;
    } else {
        pListHead = (CNV_EXTENSIONINDEX_T*)Cnvdep_Malloc(sizeof(CNV_EXTENSIONINDEX_T));
        if(pListHead != NULL){
            if(pHeader != NULL){
                pData = (CNV_PTR*)Cnvdep_Malloc(pHeader->nStructSize);
                if(pData != NULL){
                    (void)Cnvdep_Memcpy((CNV_PTR)pData, (CNV_PTR)pHeader, pHeader->nStructSize);
                
                    pListHead->pParameter = pData;
                    pListHead->nIndexType = nIndexType;
                    pListHead->pNextParam = pList;
                
                    *pNewList = (CNV_PTR)pListHead;
                } else {
                    eResult = CNV_ERROR_FATAL;
                    Cnvdep_Free((CNV_PTR)pListHead);
                }
            } else {
                Cnvdep_Free((CNV_PTR)pListHead);
            }
        } else {
            eResult = CNV_ERROR_FATAL;
        }
    }
    return eResult;
}

/**
* brief get parameter from list
*/
CNV_STATIC CNV_BOOL Cnvlocal_GetParameterList(
    CNV_PTR pList,       /**< [in] list */
    CNV_U32 nIndexType,  /**< [in] extension index */
    CNV_PTR pParam       /**< [in] parameter */
)
{
    CNV_BOOL bResult = CNV_FALSE;
    CNV_EXTENSIONINDEX_T *pHead;
    CNV_EXTENSIONINDEX_T *pNextHead;
    CNV_STRUCT_HEADER_T  *pHeader;
    pHead = (CNV_EXTENSIONINDEX_T*)pList;

    while(pHead != NULL){
        pNextHead = (CNV_EXTENSIONINDEX_T*)pHead->pNextParam;
        if( pHead->nIndexType == nIndexType ){
            pHeader = (CNV_STRUCT_HEADER_T*)pHead->pParameter;
            (void)Cnvdep_Memcpy((CNV_PTR)pParam, (CNV_PTR)pHeader, pHeader->nStructSize);
            bResult = CNV_TRUE;
            break;
        }
        pHead = pNextHead;
    }
    return bResult;
}

/**
* \brief release a list.
*/
CNV_STATIC CNV_VOID Cnvlocal_ReleaseParameterList(
    CNV_PTR pList /**< [in] list */
)
{
    CNV_EXTENSIONINDEX_T *pHead;
    CNV_EXTENSIONINDEX_T *pNextHead;
    pHead = (CNV_EXTENSIONINDEX_T*)pList;

    while(pHead != NULL){
        pNextHead = pHead->pNextParam;
        Cnvdep_Free((CNV_PTR)pHead->pParameter);
        Cnvdep_Free((CNV_PTR)pHead);
        pHead = pNextHead;
    }
    return;
}
