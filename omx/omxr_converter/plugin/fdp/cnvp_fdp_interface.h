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
 * \file cnvp_fdp_interface.h
 * \attention
 */
#ifndef CNVP_FDP_INTERFACE_H
#define CNVP_FDP_INTERFACE_H

#ifdef _cplusplus
extern "C" {
#endif

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

CNV_ERRORTYPE CNVP_GetPlugInFunc(
    CNVP_PLUGIN_IF_FUNCS_T *psPlugInFunc,
    CNV_SUBERRORTYPE       *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_CreateHandle(
    CNV_HANDLE       *hPlugInHandle,
    CNV_U32          nConvertMode,
    CNV_PTR          pUserPointer,
    CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_DeleteHandle(
    CNV_HANDLE hPlugInHandle
);

CNV_ERRORTYPE CnvPif_Open(
    CNV_HANDLE                    hPlugInHandle,
    CNV_PTR                       pParamList,
    CNV_SETTING_IMAGECONVERSION_T *pImageConversionConfig,
    CNV_CALLBACK_FUNCS_T          *psConverterCallbackFunc,
    CNVP_EVENT_FUNCS_T            *psEventFunc,
    CNV_SUBERRORTYPE              *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_Close(
  CNV_HANDLE       hPlugInHandle,
  CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_EmptyThisBuffer(
  CNV_HANDLE         hPlugInHandle,
  CNV_BUFFERHEADER_T *pBuffer,
  CNV_SUBERRORTYPE   *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_FillThisBuffer(
  CNV_HANDLE         hPlugInHandle,
  CNV_BUFFERHEADER_T *pBuffer,
  CNV_SUBERRORTYPE   *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_CommandFlush(
  CNV_HANDLE       hPlugInHandle,
  CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_GetExtensionIndex(
  CNV_STRING       cParameterName,
  CNV_INDEXTYPE    *pIndexType,
  CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_QueryFormat(
  CNV_U32           nImageFormat,
  CNV_U32           *pResponse,
  CNV_CONSTRAINTS_T *pConstraints,
  CNV_SUBERRORTYPE  *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_GetParameter(
  CNV_INDEXTYPE    nIndexType,
  CNV_PTR          pProcessingParameterStructrure,
  CNV_PTR          pParameter,
  CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_CheckParameter(
  CNV_INDEXTYPE    nIndexType,
  CNV_PTR          pProcessingParameterStructrure,
  CNV_SUBERRORTYPE *pSubErrorCode
);

CNV_ERRORTYPE CnvPif_PlugInSelect(
    CNV_SETTING_IMAGECONVERSION_T *pParam,
    CNV_EXTENSIONINDEX_T          *pExtensionData,
    CNV_U32                       nPluginNum,
    CNV_BOOL                      *bResult,
    CNV_U32                       *nMode,
    CNV_SUBERRORTYPE              *pSubErrorCode
);

#ifdef _cplusplus
}
#endif

#endif
