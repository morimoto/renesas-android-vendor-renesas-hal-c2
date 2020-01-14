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
 * UVCS UDF(User Defined Function) Implementation
 *
 * \file
 * \attention
 */


/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "omxr_uvcs_udf.h"
#include "omxr_uvcs_udf_osal.h"
#include "omxr_module_common.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
typedef struct tagOMXR_UVCS_UDF_STRUCT {
	OMXR_UVCS_UDF_OSAL_HANDLE pvUvcsUdfOsalHandle;
	OMX_PTR pvUserContext;
	OMXR_UVCS_UDF_CALLBACK_FUNCTYPE fCallback;
	OMX_PTR pvThreadId;
	OMX_BOOL bTerminateRequest;
	OMX_U32 u32StructSize;
	OMX_PTR pvHwProcReadData;
} OMXR_UVCS_UDF_STRUCT;

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/
/**
 * UVCS UDF Thread Function
 *
 * This thread is listener thread that is to wait for the end of UVCS process
 *
 * \param[in]      u32ProcID
 * \param[in]      pvParam
 * \param[in]      pvContext
 * \return         OMX_ERROTTYPE
 * \attention
 */
OMXR_STATIC OMX_ERRORTYPE OmxrUvcsUdf_Thread(OMX_U32 u32ProcID, OMX_PTR pvParam, OMX_PTR pvContext);

/***************************************************************************/
/*    Variables                                                            */
/***************************************************************************/

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/
OMX_ERRORTYPE OmxrUvcsUdf_Open(OMXR_UVCS_UDF_HANDLE *hUdfHandle, OMX_PTR pvUserContext, OMXR_UVCS_UDF_OPEN_PARAM *psParam)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMXR_UVCS_UDF_STRUCT *psUvcsUdf = NULL;

	OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_DEBUG, "", "");

	if( (hUdfHandle == NULL) || (pvUserContext == NULL) || (psParam == NULL) ){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorBadParameter;
	}
	if(psParam->fCallback == NULL){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorBadParameter;
	}
	psUvcsUdf = (OMXR_UVCS_UDF_STRUCT *)OmxrMalloc(sizeof(OMXR_UVCS_UDF_STRUCT));
	if(psUvcsUdf == NULL){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "failed at malloc", "");
		return OMX_ErrorUndefined;
	}
	(void)OmxrMemset(psUvcsUdf, 0, sizeof(OMXR_UVCS_UDF_STRUCT));

	eError = OmxrUvcsUdfOsal_Open(&psUvcsUdf->pvUvcsUdfOsalHandle);
	if(eError != OMX_ErrorNone){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "OmxrUvcsUdfOsal_Open Failed(H'%08x)", (OMX_U32)eError);
		goto cleanup;
	}

	eError = OmxrCreateThread(&psUvcsUdf->pvThreadId, (OMXR_THREAD_FUNCTION)&OmxrUvcsUdf_Thread, (OMX_PTR)psUvcsUdf, "PRIORITY.HIGH.ME.UDF.UVCS", 0);
	if(eError != OMX_ErrorNone){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		goto cleanup;
	}

	psUvcsUdf->fCallback = psParam->fCallback;
	psUvcsUdf->pvUserContext = pvUserContext;
	psUvcsUdf->pvHwProcReadData = NULL;
	psUvcsUdf->u32StructSize = 0;

	*hUdfHandle = (OMXR_UVCS_UDF_HANDLE)psUvcsUdf;

	return OMX_ErrorNone;

cleanup:
	if(psUvcsUdf->pvUvcsUdfOsalHandle != NULL){
		(void)OmxrUvcsUdfOsal_Close(psUvcsUdf->pvUvcsUdfOsalHandle);
	}
	OmxrFree(psUvcsUdf);

	return OMX_ErrorUndefined;
}

OMX_ERRORTYPE OmxrUvcsUdf_Close(OMXR_UVCS_UDF_HANDLE hUdfHandle)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMXR_UVCS_UDF_STRUCT *psUvcsUdf;
	OMX_U32 u32Ret;

	OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_DEBUG, "", "");

	if(hUdfHandle == NULL){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorBadParameter;
	}
	psUvcsUdf = (OMXR_UVCS_UDF_STRUCT *)hUdfHandle;

	psUvcsUdf->bTerminateRequest = OMX_TRUE;

	(void)OmxrUvcsUdfOsal_CancelRead(hUdfHandle->pvUvcsUdfOsalHandle);

	eError = OmxrTerminateThread(psUvcsUdf->pvThreadId);
	if (OMX_ErrorNone != eError) {
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		eError = OMX_ErrorUndefined;
	}
	else {
		eError = OmxrJoinThread(psUvcsUdf->pvThreadId, NULL);
		if (OMX_ErrorNone != eError) {
			OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
			eError = OMX_ErrorUndefined;
		}
	}
	if(psUvcsUdf->pvUvcsUdfOsalHandle != NULL){
		u32Ret = OmxrUvcsUdfOsal_Close(psUvcsUdf->pvUvcsUdfOsalHandle);
		if(u32Ret != 0){
			OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		}
	}
	if(psUvcsUdf->pvHwProcReadData != NULL){
		OmxrFree(psUvcsUdf->pvHwProcReadData);
	}

	OmxrFree(psUvcsUdf);

	return eError;
}

OMX_ERRORTYPE OmxrUvcsUdf_ProcRequest(OMXR_UVCS_UDF_HANDLE hUdfHandle, OMX_PTR pvRequestData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMXR_UVCS_UDF_STRUCT *psUvcsUdf;
	OMX_U32 u32SizeOfStruct;

	if( (hUdfHandle == NULL) || (pvRequestData == NULL) ){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorBadParameter;
	}
	psUvcsUdf = (OMXR_UVCS_UDF_STRUCT *)hUdfHandle;

	u32SizeOfStruct = *((OMX_U32 *)pvRequestData); 

	if(psUvcsUdf->pvHwProcReadData == NULL){
		psUvcsUdf->u32StructSize = u32SizeOfStruct;
		psUvcsUdf->pvHwProcReadData = OmxrMalloc(psUvcsUdf->u32StructSize);
		if(psUvcsUdf->pvHwProcReadData == NULL){
			OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
			return OMX_ErrorUndefined;
		}
	}

	eError = OmxrUvcsUdfOsal_Write(psUvcsUdf->pvUvcsUdfOsalHandle, pvRequestData, u32SizeOfStruct);
	if(eError != OMX_ErrorNone){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorUndefined;
	}

	eError = OmxrSendEvent(psUvcsUdf->pvThreadId, 0, pvRequestData);
	if(eError != OMX_ErrorNone){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorUndefined;
	}
	return OMX_ErrorNone;
}

OMX_ERRORTYPE OmxrUvcsUdf_GetIpInfo(OMXR_UVCS_UDF_HANDLE hUdfHandle, OMX_PTR pvIpInfo)
{
	OMXR_UVCS_UDF_STRUCT *psUvcsUdf;

	OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_DEBUG, "", "");

	if( (hUdfHandle == NULL) || (pvIpInfo == NULL) ){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorBadParameter;
	}
	psUvcsUdf = (OMXR_UVCS_UDF_STRUCT *)hUdfHandle;

	return OmxrUvcsUdfOsal_GetIpInfo(psUvcsUdf->pvUvcsUdfOsalHandle, pvIpInfo);
}

OMXR_STATIC OMX_ERRORTYPE OmxrUvcsUdf_Thread(OMX_U32 u32ProcID, OMX_PTR pvParam, OMX_PTR pvContext)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMXR_UVCS_UDF_STRUCT *psUvcsUdf;

	if (u32ProcID >= 0xFFFFFFF0u) {
		if (u32ProcID == 0xFFFFFFFFu) {
			if (NULL == pvParam) {
				OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
				eError = OMX_ErrorBadParameter;
			} else {
				OMX_PTR pvThreadId;

				eError = OmxrGetThreadId(&pvThreadId);
				if (OMX_ErrorNone != eError) {
					OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
				} else {
					eError = OmxrSaveThreadContext(pvThreadId, pvParam);
					if (OMX_ErrorNone != eError) {
						OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
					}
				}
			}
		} else if (u32ProcID == 0xFFFFFFFEu) {
			eError = OMX_ErrorUndefined;
		} else if (u32ProcID == 0xFFFFFFFDu) {
		} else if (u32ProcID == 0xFFFFFFFCu) {
			eError = OMX_ErrorUndefined;
		} else {
			eError = OMX_ErrorUndefined;
		}
	} else {
		/* Check parameter. */
		if ((NULL == pvContext) /*|| (NULL == pvParam)*/) {
			OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
			return OMX_ErrorBadParameter;
		}
		psUvcsUdf = (OMXR_UVCS_UDF_STRUCT *)pvContext;

		if(psUvcsUdf->pvHwProcReadData == NULL){
			return OMX_ErrorUndefined;
		}

		if(psUvcsUdf->bTerminateRequest != OMX_TRUE){
			eError = OmxrUvcsUdfOsal_Read(psUvcsUdf->pvUvcsUdfOsalHandle, psUvcsUdf->pvHwProcReadData, psUvcsUdf->u32StructSize);
		}
		if(psUvcsUdf->bTerminateRequest != OMX_TRUE){
			psUvcsUdf->fCallback((OMXR_UVCS_UDF_HANDLE)psUvcsUdf, psUvcsUdf->pvUserContext, psUvcsUdf->pvHwProcReadData, eError);
		}
	}

	return eError;
}

OMX_ERRORTYPE OmxrUvcsUdf_SetUseIp(OMXR_UVCS_UDF_HANDLE hUdfHandle, OMX_U32 u32IpNum)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMXR_UVCS_UDF_STRUCT *psUvcsUdf;

	OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_DEBUG, "", "");

	if(hUdfHandle == NULL){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorBadParameter;
	}
	psUvcsUdf = (OMXR_UVCS_UDF_STRUCT *)hUdfHandle;

	eError = OmxrUvcsUdfOsal_SetUseIp(psUvcsUdf->pvUvcsUdfOsalHandle, u32IpNum);
	if(eError != OMX_ErrorNone){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "OmxrUvcsUdfOsal_SetUseIp Failed(H'%08x)", (OMX_U32)eError);
	}

	return eError;
}

OMX_ERRORTYPE OmxrUvcsUdf_ClearUseIp(OMXR_UVCS_UDF_HANDLE hUdfHandle, OMX_U32 u32IpNum)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMXR_UVCS_UDF_STRUCT *psUvcsUdf;

	OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_DEBUG, "", "");

	if(hUdfHandle == NULL){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorBadParameter;
	}
	psUvcsUdf = (OMXR_UVCS_UDF_STRUCT *)hUdfHandle;

	eError = OmxrUvcsUdfOsal_ClearUseIp(psUvcsUdf->pvUvcsUdfOsalHandle, u32IpNum);
	if(eError != OMX_ErrorNone){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "OmxrUvcsUdf_Close Failed(H'%08x)", (OMX_U32)eError);
	}

	return eError;
}


OMX_ERRORTYPE OmxrUvcsUdf_GetLsiInfo(OMXR_UVCS_UDF_HANDLE hUdfHandle, OMX_PTR pvLsiInfo){
	OMXR_UVCS_UDF_STRUCT *psUvcsUdf;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	if( ( hUdfHandle == NULL ) || ( pvLsiInfo == NULL)  ){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "", "");
		return OMX_ErrorBadParameter;
	}
	psUvcsUdf = (OMXR_UVCS_UDF_STRUCT *)hUdfHandle;

	eError = OmxrUvcsUdfOsal_GetLsiInfo(psUvcsUdf->pvUvcsUdfOsalHandle, pvLsiInfo);
	
	if(eError != OMX_ErrorNone){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "OmxrUvcsUdf_GetLsiInfo Failed(H'%08x)", (OMX_U32)eError);
	}

	return eError;
}

OMX_ERRORTYPE OmxrUvcsUdf_QuerySpecialLsi(OMX_U32 *pu32ProductId, OMX_U32 *pu32CutId, OMX_U32 u32Index){
	if( ( NULL == pu32ProductId ) || ( NULL == pu32CutId ) ){
		OMXR_LOGGER(OMXR_VIDEO_LOG_LEVEL_ERROR, "Runtime error","");
		return OMX_ErrorUndefined;
	}
	if( u32Index >= 1 ){
		return OMX_ErrorNoMore;
	}
	else{
		*pu32ProductId = 0x4Fu;
		*pu32CutId     = 0x00u;
	}

	return OMX_ErrorNone;
}

