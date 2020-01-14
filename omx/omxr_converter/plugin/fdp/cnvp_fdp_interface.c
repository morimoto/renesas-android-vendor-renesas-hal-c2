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
 * \file cnvp_fdp_interface.c
 * \attention 
 */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/

#include "cnv_type.h"
#include "cnv_plugin_cmn.h"

#include "cnvp_cmn.h"
#include "cnvp_fdp_interface.h"
#include "cnvp_fdp_core.h"

#include "cnv_osdep.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
/* FDP PLUGIN dependence info */
#define CnvPif_PLUGIN_ID                     (0x00000004)

/* Query format */    
#define CnvVspm_IMAGE_CROP_POSX_BOUNDARY       (0)
#define CnvVspm_IMAGE_CROP_POSY_BOUNDARY       (0)
#define CnvVspm_IMAGE_CROP_WIDTH_BOUNDARY      (0)
#define CnvVspm_IMAGE_CROP_HEIGHT_BOUNDARY     (0)
#define CnvVspm_BUFFER_WIDTH_MULTIPLES         (2)
#define CnvVspm_BUFFER_HEIGHT_MULTIPLES        (2)
#define CnvVspm_BUFFER_TOP_ADDRESS_BOUNDARY    (32)
#define CnvVspm_MAX_BUFFER_NUM                 (64)

#if defined(CNV_STATIC_LINK)
#define GET_PLUGIN_FUNCTION      CnvPif_CNVP_GetPlugInFunc
#else
#define GET_PLUGIN_FUNCTION      CNVP_GetPlugInFunc
#endif

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/* Extension Type*/
typedef enum {
    OMXR_MC_EXTENSION_NONE                    = (CNV_S32)(CNVP_EXTENSION_INDEX_BASE + 0x0000C000u),
    OMXR_MC_IndexParamVideoConverterType,
    OMXR_MC_IndexParamVideoDeinterlaceMode,
    OMXR_MC_IndexParamVideoSubModuleTimeout
}CnvPif_EXTETIONTYPE;

typedef struct{
    CNV_STRING           cIndexName;
    CnvPif_EXTETIONTYPE eExtensionIndex;
}CnvPif_EXTENSIONINDEX_T;

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/

CNV_STATIC CNV_BOOL CnvPif_GetExtensionParameter(CNV_PTR pParamList, CNV_STRING cIndexName,CNV_PTR *pParam);
/***************************************************************************/
/*    Variables                                                            */
/***************************************************************************/
static CnvPif_EXTENSIONINDEX_T sExtensionIndex[] = {
    {"OMX.RENESAS.INDEX.PARAM.VIDEO.CONVERTER.CONVERTERTYPE", OMXR_MC_IndexParamVideoConverterType},
    {"OMX.RENESAS.INDEX.PARAM.VIDEO.CONVERTER.DEINITERLACEMODE", OMXR_MC_IndexParamVideoDeinterlaceMode},
    {"OMX.RENESAS.INDEX.PARAM.VIDEO.CONVERTER.TIMEOUT", OMXR_MC_IndexParamVideoSubModuleTimeout},
};

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/

/**
* \brief get plugin interface functions
*/
CNV_ERRORTYPE GET_PLUGIN_FUNCTION(
    CNVP_PLUGIN_IF_FUNCS_T   *psPlugInFunc, /**< [out] plugin function */
    CNV_SUBERRORTYPE         *pSubErrorCode /**< [out] sub error code */
)
{
    /* Set SubError code */
    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );
    
    if( psPlugInFunc == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    /* Set Function */
    psPlugInFunc->PlugIn_Open              = &CnvPif_Open;
    psPlugInFunc->PlugIn_Close             = &CnvPif_Close;
    psPlugInFunc->PlugIn_CreateHandle      = &CnvPif_CreateHandle;
    psPlugInFunc->PlugIn_DeleteHandle      = &CnvPif_DeleteHandle;
    psPlugInFunc->PlugIn_CheckParameter    = &CnvPif_CheckParameter;
    psPlugInFunc->PlugIn_GetParameter      = &CnvPif_GetParameter;
    psPlugInFunc->PlugIn_GetExtensionIndex = &CnvPif_GetExtensionIndex;
    psPlugInFunc->PlugIn_QueryFormat       = &CnvPif_QueryFormat;
    psPlugInFunc->PlugIn_EmptyThisBuffer   = &CnvPif_EmptyThisBuffer;
    psPlugInFunc->PlugIn_FillThisBuffer    = &CnvPif_FillThisBuffer;
    psPlugInFunc->PlugIn_CommandFlush      = &CnvPif_CommandFlush;
    psPlugInFunc->PlugIn_PlugInSelect      = &CnvPif_PlugInSelect;
    
    return CNV_ERROR_NONE;
}

/**
* \brief create fdp plugin interface handle.
*/
CNV_ERRORTYPE CnvPif_CreateHandle(
    CNV_HANDLE       *hPlugInHandle, /**< [out] plugin hanlde */
    CNV_U32          nConvertMode,   /**< [in] convert mode */
    CNV_PTR          pUserPointer,   /**< [in] user pointer */
    CNV_SUBERRORTYPE *pSubErrorCode  /**< [out] fdp plugin function */
)
{
    CNVP_CONTEXT_T *pCnvpContext;
    
    CNV_ERRORTYPE eResult = CNV_ERROR_INVALID_PARAMETER;
    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );
    
    /* Parameter check */
    if( hPlugInHandle == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_HANDLE );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    /* Create Plugin Context */
    pCnvpContext = (CNVP_CONTEXT_T *)Cnvdep_Malloc(sizeof(CNVP_CONTEXT_T));
    if( pCnvpContext == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM_CREATE_HANDLE );
        return CNV_ERROR_INVALID_PARAMETER;
    }
    (void)Cnvdep_Memset(pCnvpContext,0,sizeof(CNVP_CONTEXT_T));
    
    /* Set UserPointer */
    pCnvpContext->pUserPointer =  pUserPointer;
    
    /* Create CnvpCmnHandle */    
    eResult = CnvPcmn_CreateHandle(&pCnvpContext->pCnvpCmnHandle);
    if( eResult != CNV_ERROR_NONE ){
        Cnvdep_Free((CNV_PTR)pCnvpContext);
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM_CREATE_HANDLE );
        return eResult;
    }
    
    /* Set Function Parameter */
    eResult = CnvpCore_GetPluginCoreFunction(&pCnvpContext->sPluginFunc);
    if( eResult != CNV_ERROR_NONE ){
        CnvPcmn_DeleteHandle((CNV_PTR)pCnvpContext->pCnvpCmnHandle);
        Cnvdep_Free((CNV_PTR)pCnvpContext);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    
    /* Set Mode Parameter */
    pCnvpContext->nConvertMode = nConvertMode;
    if( ( pCnvpContext->nConvertMode != CNV_PROG) &&
		( pCnvpContext->nConvertMode != CNV_DEINT_2D_HALFRATE) && 
		( pCnvpContext->nConvertMode != CNV_DEINT_2D_FULLRATE) &&
        ( pCnvpContext->nConvertMode != CNV_DEINT_3D_HALFRATE) && 
		( pCnvpContext->nConvertMode != CNV_DEINT_3D_FULLRATE) && 
		( pCnvpContext->nConvertMode != CNV_DEINT_NONE_FIELD ) )
    {
        CNV_LOGGER(CNV_LOG_DEBUG,",FDP Select: CONVERT AUTO MODE",CNVP_DEFAULT_CONVERT_MODE);
        pCnvpContext->nConvertMode = CNVP_DEFAULT_CONVERT_MODE;
    }
    
    /* Create handle plugin */
    eResult = pCnvpContext->sPluginFunc.CNVP_CreateHandle(&pCnvpContext->pCnvpDepHandle, pSubErrorCode);
    
    CNV_LOGGER(CNV_LOG_DEBUG,"pCnvpCmnHandle = %p, pCnvpDepHandle = %p",(CNV_PTR)pCnvpContext->pCnvpCmnHandle, (CNV_PTR)pCnvpContext->pCnvpDepHandle);
    
    if( eResult == CNV_ERROR_NONE ){
        *hPlugInHandle = (CNV_PTR)pCnvpContext;
    }
    return eResult;        
}

/**
* \brief destroy plugin interface handle
*/
CNV_ERRORTYPE CnvPif_DeleteHandle(
    CNV_HANDLE hPlugInHandle /**< [in] plugin hanlde */
)
{
    CNVP_CONTEXT_T *pCnvpContext;

    /* Parameter check */
    if( hPlugInHandle == NULL ){
        return CNV_ERROR_INVALID_PARAMETER;
    }    

    pCnvpContext = (CNVP_CONTEXT_T *)hPlugInHandle;

    /* CmnHandle Free */
    CnvPcmn_DeleteHandle((CNV_PTR)pCnvpContext->pCnvpCmnHandle);
    
    /* DepHandle Free */
    pCnvpContext->sPluginFunc.CNVP_DeleteHandle((CNV_PTR)pCnvpContext->pCnvpDepHandle);

    /* pCnvpContext Free */
    Cnvdep_Free((CNV_PTR)pCnvpContext);

    return CNV_ERROR_NONE;
}

/**
* \brief create converter thread
*/
CNV_ERRORTYPE CnvPif_Open(
    CNV_HANDLE                    hPlugInHandle,            /**< [in] plugin hanlde */
    CNV_PTR                       pParamList,               /**< [in] parameter list */
    CNV_SETTING_IMAGECONVERSION_T *pImageConversionConfig,  /**< [in] configuration */
    CNV_CALLBACK_FUNCS_T          *psConverterCallbackFunc, /**< [in] callback function */
    CNVP_EVENT_FUNCS_T            *psEventFunc,             /**< [in] local event callback */          
    CNV_SUBERRORTYPE              *pSubErrorCode            /**< [out] sub error code */
)
{
    CNVP_CONTEXT_T *pCnvpContext;

    CNV_ERRORTYPE eResult = CNV_ERROR_NONE;
    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );

    /* Parameter check */
    if( hPlugInHandle == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_HANDLE );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if( pImageConversionConfig == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_NINPUTIMAGE_COLORFORMAT );
        return CNV_ERROR_INVALID_PARAMETER;
    }    

    if( psConverterCallbackFunc == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_CALLBACKFUNCS );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if( psEventFunc == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERRPR_SUBMODULE_PARAM_CALLBACKFUNCS );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pCnvpContext = (CNVP_CONTEXT_T*)hPlugInHandle;

    pCnvpContext->sCallbackFunc.CNV_EmptyBufferDone = psConverterCallbackFunc->CNV_EmptyBufferDone;
    pCnvpContext->sCallbackFunc.CNV_FillBufferDone  = psConverterCallbackFunc->CNV_FillBufferDone;
    pCnvpContext->sCallbackFunc.CNV_EventDone        = psConverterCallbackFunc->CNV_EventDone;

    /* Set local Event Parameter */
    pCnvpContext->sPluginlocalEvent.hCnvHandle = psEventFunc->hCnvHandle;
    pCnvpContext->sPluginlocalEvent.PlugIn_CallBackEvent = psEventFunc->PlugIn_CallBackEvent;    
 
    /* plugin param */
    pCnvpContext->sPluginRule.bBottomFirst         = pImageConversionConfig->bBottomFirst;
    pCnvpContext->sPluginRule.bInterlacedSequence  = pImageConversionConfig->bInterlacedSequence;
    pCnvpContext->sPluginRule.nInputFormat         = pImageConversionConfig->nInputImage_ColorFormat;
    pCnvpContext->sPluginRule.nOutputFormat        = pImageConversionConfig->nOutputImage_ColorFormat;
    pCnvpContext->sPluginRule.bCropping            = pImageConversionConfig->bCropping;
    pCnvpContext->sPluginRule.bScaling             = pImageConversionConfig->bScaling;
    pCnvpContext->sPluginRule.bTileMode            = pImageConversionConfig->bTileMode;

    /* module param */
    pCnvpContext->sModuleParam.nMaxStride          = pImageConversionConfig->sModuleParam.nMaxStride;
    pCnvpContext->sModuleParam.nMaxSliceHeight     = pImageConversionConfig->sModuleParam.nMaxSliceHeight;
    pCnvpContext->sModuleParam.nModuleWorkSize     = pImageConversionConfig->sModuleParam.nModuleWorkSize;
    pCnvpContext->sModuleParam.nModuleWorkPhysAddr = pImageConversionConfig->sModuleParam.nModuleWorkPhysAddr;

    /* compression */
    pCnvpContext->bImageCompression                = pImageConversionConfig->bCompression;

    /* Extension Information */
    pCnvpContext->sExtensionInfo.pParameterList             = pParamList;
    pCnvpContext->sExtensionInfo.CNVP_GetExtensionParameter = &CnvPif_GetExtensionParameter;

    eResult = CnvPcmn_CreateCmnThread( (CNV_PTR)pCnvpContext , pSubErrorCode);

    return eResult;    
}

/**
* \brief destory converter thread
*/
CNV_ERRORTYPE CnvPif_Close (
  CNV_HANDLE       hPlugInHandle, /**< [in] plugin hanlde */
  CNV_SUBERRORTYPE *pSubErrorCode /**< [out] sub error code */
)
{
    CNV_ERRORTYPE eResult = CNV_ERROR_INVALID_PARAMETER;
    
    if( hPlugInHandle == NULL ) {
        CnvPcmn_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    
    /* Send Event */
    eResult = CnvPcmn_SendEvent( (CNV_PTR)hPlugInHandle, CNV_EVENT_CLOSE, NULL, pSubErrorCode);
    if( eResult != CNV_ERROR_NONE ){
        return CNV_ERROR_INVALID_PARAMETER;    
    }
    
    eResult = CnvPcmn_CmnThreadJoin( (CNV_PTR)hPlugInHandle ,  pSubErrorCode);
    
    /* plugin close */
    
    return eResult;    
}

/**
* \brief send a input buffer to converter thread
*/
CNV_ERRORTYPE CnvPif_EmptyThisBuffer (
  CNV_HANDLE         hPlugInHandle, /**< [in] plugin hanlde */
  CNV_BUFFERHEADER_T *pBuffer,      /**< [in] input buffer */
  CNV_SUBERRORTYPE   *pSubErrorCode /**< [out] sub error code */
)
{
    CNV_ERRORTYPE  eResult = CNV_ERROR_NONE;
    CNVP_CONTEXT_T *pCnvpContext;

    if( hPlugInHandle == NULL) {
        CnvPcmn_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    
    if( pBuffer == NULL) {
        CnvPcmn_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_NINPUTIMAGE_COLORFORMAT);
        return CNV_ERROR_INVALID_PARAMETER;
    }    
        
    pCnvpContext = (CNVP_CONTEXT_T*)hPlugInHandle;

    /* Send Message (Buffer Data) */
    eResult = CnvPcmn_SendEvent( (CNV_PTR)pCnvpContext, CNV_EVENT_EMPTYBUFFER, (CNV_PTR)pBuffer, pSubErrorCode);
    if( eResult == CNV_ERROR_NONE){
        pCnvpContext->nCountofInput++;
    }

    return eResult;
}

/**
* \brief send a output buffer to converter thread
*/
CNV_ERRORTYPE CnvPif_FillThisBuffer(
    CNV_HANDLE         hPlugInHandle, /**< [in] plugin hanlde */
    CNV_BUFFERHEADER_T *pBuffer,      /**< [in] output buffer */
    CNV_SUBERRORTYPE   *pSubErrorCode /**< [out] sub error code */
)
{
    CNV_ERRORTYPE  eResult = CNV_ERROR_NONE;
    CNVP_CONTEXT_T *pCnvpContext;
    
    if( hPlugInHandle == NULL) {
        CnvPcmn_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        return CNV_ERROR_INVALID_PARAMETER;
    }
    
    if( pBuffer == NULL ) {
        CnvPcmn_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_BUFFER);
        return CNV_ERROR_INVALID_PARAMETER;
    }    

    pCnvpContext = (CNVP_CONTEXT_T*)hPlugInHandle;

    /* Send Message (Buffer Data) */
    eResult = CnvPcmn_SendEvent( (CNV_PTR)pCnvpContext, CNV_EVENT_FILLBUFFER, (CNV_PTR)pBuffer, pSubErrorCode);
    if( eResult == CNV_ERROR_NONE ){
        pCnvpContext->nCountofOutput++;
    }

    return eResult;
}

/**
* \brief send a flush request to converter thread
*/
CNV_ERRORTYPE CnvPif_CommandFlush (
  CNV_HANDLE       hPlugInHandle, /**< [in] plugin hanlde */
  CNV_SUBERRORTYPE *pSubErrorCode /**< [out] sub error code */
)
{
    CNV_ERRORTYPE eResult = CNV_ERROR_INVALID_PARAMETER;

    if( hPlugInHandle == NULL ) {
        CnvPcmn_SetSubErrorCode(pSubErrorCode,CNV_ERROR_PARAM_HANDLE);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    /* Send Message (Buffer Data) */
    eResult = CnvPcmn_SendEvent( (CNV_PTR)hPlugInHandle, CNV_EVENT_FLUSHING, NULL, pSubErrorCode);

    return eResult;
}

/**
* \brief get extension index
*/
CNV_ERRORTYPE CnvPif_GetExtensionIndex(
    CNV_STRING       cParameterName, /**< [in] index name */
    CNV_INDEXTYPE    *pIndexType,    /**< [out] extension index */
    CNV_SUBERRORTYPE *pSubErrorCode  /**< [out] sub error code */
)
{
    CNV_U32       uCnt          = 0;
    CNV_U32       nMaxIndexNum  = 0;
    CNV_ERRORTYPE eResult       = CNV_ERROR_INVALID_PARAMETER;

    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );

    /* Parameter check */
    if( cParameterName == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_PARAMETERNAME );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if (pIndexType == NULL) {
        CnvPcmn_SetSubErrorCode(pSubErrorCode, CNV_ERROR_PARAM_PARAMETERNAME);
        return CNV_ERROR_INVALID_PARAMETER;
    }

    nMaxIndexNum = sizeof(sExtensionIndex) / sizeof(CnvPif_EXTENSIONINDEX_T);
    /* Set ExtensionIndex */
    for( uCnt = 0; uCnt < nMaxIndexNum; uCnt++)
    {
        /* Set ExtensionIndex */
        if( strcmp( sExtensionIndex[uCnt].cIndexName , cParameterName ) == 0 ){
            *pIndexType = sExtensionIndex[uCnt].eExtensionIndex;

            eResult = CNV_ERROR_NONE;
            CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );
            CNV_LOGGER(CNV_LOG_DEBUG,"ExtensionName = %s",cParameterName);
            break;
        } else {
            eResult = CNV_ERROR_INVALID_PARAMETER;
            CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_PARAMETERNAME );
        }
    }

    return eResult;
}

/**
* \brief get query information
*/
CNV_ERRORTYPE CnvPif_QueryFormat(
    CNV_U32           nImageFormat,  /**< [in] index name */
    CNV_U32           *pResponse,    /**< [out] response */
    CNV_CONSTRAINTS_T *pConstraints, /**< [out] constraints */
    CNV_SUBERRORTYPE  *pSubErrorCode /**< [out] sub error code */
)
{
    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );

    /* Parameter check */
    if( pResponse == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_RESPONSE );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    /* Parameter check */
    if( pConstraints == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_CONSTRAINTS );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    /* Check support format */

    switch( nImageFormat )
    {
    case CNVP_COLOR_FormatYUV420Planar:
        *pResponse = CNV_QUERY_IMAGE_SUPPOERTED_INPUT_OUTPUT;
        break;
    case CNVP_COLOR_FormatYUV420PackedPlanar:
        *pResponse = CNV_QUERY_IMAGE_SUPPOERTED_INPUT_OUTPUT;        
        break;
    case CNVP_COLOR_FormatYUV420SemiPlanar:
        *pResponse = CNV_QUERY_IMAGE_SUPPOERTED_INPUT_OUTPUT;        
        break;
    default:
        *pResponse = CNV_QUERY_IMAGE_NOT_SUPPORTED;
        break;
    }

    /* Set Query Format */
    pConstraints->nImageCropPosXBoundary           = CnvVspm_IMAGE_CROP_POSX_BOUNDARY;
    pConstraints->nImageCropPosYBoundary           = CnvVspm_IMAGE_CROP_POSY_BOUNDARY;
    pConstraints->nImageCropWidthBoundary          = CnvVspm_IMAGE_CROP_WIDTH_BOUNDARY;
    pConstraints->nImageCropHeightBoundary         = CnvVspm_IMAGE_CROP_HEIGHT_BOUNDARY;
    pConstraints->nBufferWidthMultiplesOf          = CnvVspm_BUFFER_WIDTH_MULTIPLES;
    pConstraints->nBufferHeightMultiplesOf         = CnvVspm_BUFFER_HEIGHT_MULTIPLES;
    pConstraints->nBufferTopAddressBoundary        = CnvVspm_BUFFER_TOP_ADDRESS_BOUNDARY;
    pConstraints->nMaxNumberOfBuffersPerProcessing = CnvVspm_MAX_BUFFER_NUM;

    return CNV_ERROR_NONE;    
}

/**
* \brief get parameter 
*/
CNV_ERRORTYPE CnvPif_GetParameter(
    CNV_INDEXTYPE    nIndexType,                     /**< [in] extension index */
    CNV_PTR          pProcessingParameterStructrure, /**< [out] parameter */
    CNV_PTR          pParameter,                     /**< [in] current parameter */
    CNV_SUBERRORTYPE *pSubErrorCode                  /**< [out] sub error code */
)
{
    CNV_U32                           uCnt = 0;
    CNV_U32                           nMaxIndexNum = 0;
    CNV_ERRORTYPE                     eResult = CNV_ERROR_INVALID_PARAMETER;
    OMXR_MC_VIDEO_CONVERTER_T         *psIdParam;
    OMXR_MC_VIDEO_DEINTERLACEMODE_T   *psModeParam;
    OMXR_MC_VIDEO_SUBMODULE_TIMEOUT_T *psTimeoutParam;

    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );

    /* check parameter */
    if( pProcessingParameterStructrure == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_PROCESSINGPARAMETERSTRUCTURE );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    nMaxIndexNum = sizeof(sExtensionIndex) / sizeof(CnvPif_EXTENSIONINDEX_T);
    for( uCnt = 0; uCnt < nMaxIndexNum; uCnt++)
    {
        if((CNV_U32)sExtensionIndex[uCnt].eExtensionIndex == nIndexType ){
            switch( nIndexType )
            {
            case OMXR_MC_IndexParamVideoConverterType:
                psIdParam = (OMXR_MC_VIDEO_CONVERTER_T *)pProcessingParameterStructrure;
                psIdParam->nPluginID =  (CNV_U32)CNV_PLUGIN_ID_FDP;
                CNV_LOGGER(CNV_LOG_DEBUG,"OMXR_MC_IndexParamVideoConverterType ",0);
                break;
            case OMXR_MC_IndexParamVideoDeinterlaceMode:
                psModeParam                               = (OMXR_MC_VIDEO_DEINTERLACEMODE_T *)pProcessingParameterStructrure;
                psModeParam->nDeinterlace                 = (CNV_U32)CNV_DEINT_3D_HALFRATE;
				psModeParam->bDisableInterpolateTimeStamp = CNV_FALSE;
				psModeParam->nTimeStampOffset             = 16666; /* micro sec unit */
                CNV_LOGGER(CNV_LOG_DEBUG,"OMXR_MC_IndexParamVideoDeinterlaceMode = %p", (CNV_PTR)pParameter);
                break;
            case OMXR_MC_IndexParamVideoSubModuleTimeout:
                psTimeoutParam                  = (OMXR_MC_VIDEO_SUBMODULE_TIMEOUT_T *)pProcessingParameterStructrure;
                psTimeoutParam->nWaitTime         =  (CNV_U32)CNVP_THREAD_WAIT_TIME;
                psTimeoutParam->nTimeout         =  (CNV_U32)CNVP_THREAD_TIMEOUT;
                CNV_LOGGER(CNV_LOG_DEBUG,"OMXR_MC_IndexParamVideoSubModuleTimeout = %p", (CNV_PTR)pParameter);
                break;
            default:
                break;
            }
            eResult = CNV_ERROR_NONE;
            CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );
            break;
        }
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM );
    }
    return eResult;
}


/**
* \brief check a setting parameter
*/
CNV_ERRORTYPE CnvPif_CheckParameter(
    CNV_INDEXTYPE    nIndexType,                     /**< [in] extension index */
    CNV_PTR          pProcessingParameterStructrure, /**< [in] parameter */
    CNV_SUBERRORTYPE *pSubErrorCode                  /**< [out] sub error code */
)
{
    CNV_U32       uCnt         = 0;
    CNV_U32       nMaxIndexNum = 0;
    CNV_ERRORTYPE eResult      = CNV_ERROR_INVALID_PARAMETER;
    
    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );
    
    /* check parameter */
    if( pProcessingParameterStructrure == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_PROCESSINGPARAMETERSTRUCTURE );
        return CNV_ERROR_INVALID_PARAMETER;
    }
    
    nMaxIndexNum = sizeof(sExtensionIndex) / sizeof(CnvPif_EXTENSIONINDEX_T);
    for( uCnt = 0; uCnt < nMaxIndexNum; uCnt++)
    {
        if((CNV_U32)sExtensionIndex[uCnt].eExtensionIndex == nIndexType ){
            switch( nIndexType )
            {
            case OMXR_MC_IndexParamVideoConverterType:
                eResult = CNV_ERROR_NONE;
                CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );
                break;
            case OMXR_MC_IndexParamVideoDeinterlaceMode:
                eResult = CNV_ERROR_NONE;
                CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );
                break;
            case OMXR_MC_IndexParamVideoSubModuleTimeout:
                eResult = CNV_ERROR_NONE;
                CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );
                break;
            default:
                break;
            }
            break;
        }
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM );
    }
    return eResult;    
}

/**
* \brief decide plugin
*/
CNV_ERRORTYPE CnvPif_PlugInSelect(
    CNV_SETTING_IMAGECONVERSION_T *pParam,               /**< [in] image parameter */
    CNV_EXTENSIONINDEX_T          *pExtensionData,       /**< [in] extension parameter */
    CNV_U32                       nPluginNum,            /**< [in] plugin number */
    CNV_BOOL                      *bResult,              /**< [out] result */
    CNV_U32                       *nMode,                /**< [out] deinterlace mode */
    CNV_SUBERRORTYPE              *pSubErrorCode         /**< [out] sub error code */
)
{
    CNV_BOOL        bExtensionMode  = CNV_FALSE;
    CNV_BOOL        bPluginId       = CNV_FALSE;
    CNV_ERRORTYPE     eResult       = CNV_ERROR_INVALID_PARAMETER;
	CNVP_PLUGIN_SPEC_T              sPluginSpec;
    OMXR_MC_VIDEO_CONVERTER_T       *psIdParam;
    OMXR_MC_VIDEO_DEINTERLACEMODE_T *psModeParam;

    CNV_EXTENSIONINDEX_T *pHead;
    CNV_EXTENSIONINDEX_T *pNextHead;

    CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_SUBERROR_NONE );

    /* check parameter */
    if( pParam == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_NINPUTIMAGE_COLORFORMAT );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    if( bResult == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM );
        return CNV_ERROR_INVALID_PARAMETER;
    }
    
    if( nMode == NULL ){
        CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM );
        return CNV_ERROR_INVALID_PARAMETER;
    }

    pHead = (CNV_EXTENSIONINDEX_T*)pExtensionData;

    (void)Cnvdep_Memset(&sPluginSpec, 0, sizeof(CNVP_PLUGIN_SPEC_T));

    while(pHead != NULL ){
        pNextHead = pHead->pNextParam;
        switch( pHead->nIndexType )
        {
        case OMXR_MC_IndexParamVideoConverterType:
            psIdParam  =  (OMXR_MC_VIDEO_CONVERTER_T *)pHead->pParameter;
			sPluginSpec.nPlugInId = psIdParam->nPluginID;
            if( sPluginSpec.nPlugInId == CNV_PLUGIN_ID_FDP ){
                *bResult = CNV_TRUE;
                eResult  = CNV_ERROR_NONE;
            }
            bPluginId = CNV_TRUE;
            CNV_LOGGER(CNV_LOG_DEBUG,"OMXR_MC_IndexParamVideoConverterType = %d", sPluginSpec.nPlugInId);
            break;
        case OMXR_MC_IndexParamVideoDeinterlaceMode:
            psModeParam = (OMXR_MC_VIDEO_DEINTERLACEMODE_T *)pHead->pParameter;
			sPluginSpec.nDeinterlace = psModeParam->nDeinterlace;
            CNV_LOGGER(CNV_LOG_DEBUG,"OMXR_MC_IndexParamVideoDeinterlaceMode = %d",psModeParam->nDeinterlace);
            bExtensionMode = CNV_TRUE;
            break;
        default:
            CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_SUBMODULE_PARAM_EXTENSIONINDEX );
            break;
        }
        pHead = pNextHead;
    }
    /* check parameter */
    /* plugIn dependence */
    if( bPluginId == CNV_FALSE ){
		sPluginSpec.bInterlacedSequence        = pParam->bInterlacedSequence;
		sPluginSpec.bCropping                  = pParam->bCropping;
		sPluginSpec.bScaling                   = pParam->bScaling;
            
		sPluginSpec.nInputFormat               = pParam->nInputImage_ColorFormat;
		sPluginSpec.nOutputFormat              = pParam->nOutputImage_ColorFormat;
		sPluginSpec.bBottomFirst               = pParam->bBottomFirst;

        if( pParam->nInputImage_ColorFormat != pParam->nOutputImage_ColorFormat ){
			sPluginSpec.bImageFormatConversion    = CNV_TRUE;
        } else {
			sPluginSpec.bImageFormatConversion    = CNV_FALSE;
        }

        eResult = CnvPvspmCore_ConfirmPluginSpec(&sPluginSpec,bResult,pSubErrorCode);
        if( CNV_ERROR_NONE != eResult ){
            CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_UNSELECT_PLUGIN );
        }
    } else {
        if(( pParam->bInterlacedSequence == CNV_FALSE) && ( pParam->bBottomFirst == CNV_TRUE ) ){
            *bResult = CNV_FALSE;
            eResult  = CNV_ERROR_INVALID_PARAMETER;
            CnvPcmn_SetSubErrorCode( pSubErrorCode, CNV_ERROR_PARAM_UNSELECT_PLUGIN );
        }
    }

    /* 1 plugin mode */
    if( nPluginNum == 1){
        if( ( bPluginId == CNV_TRUE ) && ( eResult != CNV_ERROR_INVALID_PARAMETER ) ){
            *bResult = CNV_TRUE;
            eResult  = CNV_ERROR_NONE;
        }
        
        if( CNV_FALSE == bPluginId ){
            *bResult = CNV_TRUE;
            eResult  = CNV_ERROR_NONE;
        }
    }

    if( bExtensionMode != CNV_TRUE){
		sPluginSpec.nDeinterlace = CNVP_DEFAULT_CONVERT_MODE;
    } else {
        if( sPluginSpec.bInterlacedSequence == CNV_FALSE){
            if( sPluginSpec.nDeinterlace != CNV_PROG ){
				sPluginSpec.nDeinterlace = CNV_PROG;
                CNV_LOGGER(CNV_LOG_DEBUG,"DeInterlase NONE",0);
            }
        }
    }

    *nMode = sPluginSpec.nDeinterlace;
    CNV_LOGGER(CNV_LOG_DEBUG,"Result = %d", (CNV_U32)eResult);
    return eResult;
}

/**
* \brief get extension parameter
*/
CNV_STATIC CNV_BOOL CnvPif_GetExtensionParameter(
    CNV_PTR pParamList,     /**< [in] list of parameter */
    CNV_STRING cIndexName,  /**< [in] index name */
    CNV_PTR *pParam         /**< [out] parameter */
)
{
    CNV_INDEXTYPE        nExtensionIndex;
    CNV_BOOL             bGetFlag = CNV_FALSE;
    CNV_ERRORTYPE        eResult  = CNV_ERROR_NONE;
    CNV_EXTENSIONINDEX_T *pHead;
    CNV_EXTENSIONINDEX_T *pNextHead;

    pHead = (CNV_EXTENSIONINDEX_T*)pParamList;

    eResult = CnvPif_GetExtensionIndex(cIndexName,&nExtensionIndex,NULL);
    if( eResult == CNV_ERROR_NONE ){
        while(pHead != NULL ){
            pNextHead = (CNV_EXTENSIONINDEX_T*)pHead->pNextParam;
            if( nExtensionIndex == pHead->nIndexType ){
                *pParam = pHead->pParameter;
                bGetFlag = CNV_TRUE;
                break;
            }
            pHead = pNextHead;
        }
    }
    return bGetFlag;
}
