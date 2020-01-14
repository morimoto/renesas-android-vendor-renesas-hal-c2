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
 * OMX Converter public file 
 *
 * \file cnv_public.h
 * \attention
 */
#ifndef CNV_PUBLIC_H
#define CNV_PUBLIC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "cnv_type.h"
/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/
CNV_ERRORTYPE  CNV_Initialize(
    CNV_HANDLE       *pHandle,
    CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE  CNV_Deinitialize(
    CNV_HANDLE       hProcessing,
    CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CNV_GetExtensionIndex(
    CNV_HANDLE       hProcessing,
    CNV_STRING       cParameterName,
    CNV_INDEXTYPE    *pIndexType,
    CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CNV_QueryFormat(
    CNV_HANDLE        hProcessing,
    CNV_U32           nImageFormat,
    CNV_U32           *pResponse,
    CNV_CONSTRAINTS_T *pConstraints,
    CNV_SUBERRORTYPE  *pSubErrorCode
);

CNV_ERRORTYPE CNV_GetParameter(
    CNV_HANDLE       hProcessing,
    CNV_INDEXTYPE    nIndexType,
    CNV_PTR          pProcessingParameterStructrure,
    CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CNV_SetParameter (
    CNV_HANDLE       hProcessing,
    CNV_INDEXTYPE    nIndexType,
    CNV_PTR          pProcessingParameterStructrure,
    CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CNV_Open (
    CNV_HANDLE                    hProcessing,
    CNV_PTR                       pUserPointer,
    CNV_SETTING_IMAGECONVERSION_T *pImageConversionConfig,
    CNV_CALLBACK_FUNCS_T          *pCallbackFuncs,
    CNV_SUBERRORTYPE              *pSubErrorCode
);

CNV_ERRORTYPE CNV_EmptyThisBuffer (
    CNV_HANDLE         hProcessing,
    CNV_BUFFERHEADER_T *pBuffer,
    CNV_SUBERRORTYPE   *pSubErrorCode
);

CNV_ERRORTYPE CNV_FillThisBuffer(
    CNV_HANDLE         hProcessing,
    CNV_BUFFERHEADER_T *pBuffer,
    CNV_SUBERRORTYPE   *pSubErrorCode
);

CNV_ERRORTYPE CNV_CommandFlush (
    CNV_HANDLE        hProcessing,
    CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CNV_Close (
    CNV_HANDLE       hProcessing,
    CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CNV_ErrorCodeToString (
    CNV_S32    nErrorCode,
    CNV_STRING *pErrorString
);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CNV_PUBLIC_h */
