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
 * OMX Converter os dependence file 
 *
 * \file cnv_osdep.h
 * \attention
 */
#ifndef CNV_OS_DEPENDENCE_H
#define CNV_OS_DEPENDENCE_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
extern void OmxrLogFormat(
    CNV_U32    u32Level,
    CNV_STRING strFunction,
    CNV_U32    u32Lineno,
    CNV_STRING strString,
    ...);

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/**
* \enum CNV_LOG_MODE
*/
typedef enum {
    CNV_LOG_DEBUG   = 0,
    CNV_LOG_WARN    = 1,
    CNV_LOG_INFO    = 2,
    CNV_LOG_ERROR   = 3,
    CNV_LOG_UNSUSED = 4
} CNV_LOG_MODE;

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/

CNV_PTR Cnvdep_Malloc(
    CNV_U32 nSize
);

CNV_PTR Cnvdep_BufferMalloc(
    CNV_U32 nSize
);

CNV_VOID Cnvdep_Free(
    CNV_PTR pvPtr
);

CNV_VOID Cnvdep_BufferFree(
    CNV_PTR pvPtr
);

CNV_PTR Cnvdep_Memcpy(
    CNV_PTR pvDest,
    CNV_PTR pvSrc,
    CNV_U32 nSize
);

CNV_PTR Cnvdep_Memset(
    CNV_PTR pvDest,
    CNV_U8  nCode,
    CNV_U32 nSize
);

CNV_ERRORTYPE Cnvdep_LoadDll(
    CNV_STRING path,
    CNV_HANDLE *handle
);

CNV_ERRORTYPE Cnvdep_GetDllEntry(
    CNV_HANDLE handle,
    CNV_STRING name,
    CNV_PTR    *entry
);

CNV_ERRORTYPE Cnvdep_UnloadDll(
    CNV_HANDLE handle
);

CNV_VOID Cnvdep_ThreadExit(
    CNV_VOID *retval
);

CNV_U32 Cnvdep_ThreadJoin(
    CNV_PTR nThreadID,
    CNV_PTR *retval
);

CNV_ERRORTYPE Cnvdep_CreateOsDepHandle(
    CNV_PTR *nThreadID
);

CNV_ERRORTYPE Cnvdep_CreateThread(
    CNV_PTR nThreadID,
    CNV_VOID (*thread_function)(
        CNV_PTR pHandle,
        CNV_U32 nEventId,
        CNV_PTR pData),
    CNV_PTR pParam 
);

CNV_ERRORTYPE Cnvdep_CreateMutex(
    CNV_PTR *nMutex
);

CNV_VOID Cnvdep_DestroyMutex(
    CNV_PTR nMutex
);

CNV_VOID Cnvdep_LockMutex(
    CNV_PTR nMutex
);

CNV_VOID Cnvdep_UnlockMutex(
    CNV_PTR nMutex
);

CNV_VOID Cnvdep_Sleep(
    CNV_U32 nSleepTime
);

CNV_ERRORTYPE Cnvdep_SaveTreadContext(
    CNV_PTR nThreadID,
    CNV_PTR pParam
);

CNV_ERRORTYPE Cnvdep_SendEvent(
    CNV_PTR nThreadId,
    CNV_U32 nEventId,
    CNV_PTR pvParam
);

CNV_U32 Cnvdep_GetOmxrLogLevel(
    CNV_U32 u32Level
);

#ifndef OMXR_BUILD_LOGGER_DISABLE
void Cnvdep_LogFormat(
    CNV_U32    u32Level,
    CNV_STRING strFunction,
    CNV_U32    u32Lineno,
    CNV_STRING strString,
    ...
);
#endif

CNV_VOID Cnvdep_Open(
    CNV_VOID
);

CNV_VOID Cnvdep_Close(
    CNV_VOID
);

#ifndef OMXR_BUILD_LOGGER_DISABLE
#define CNV_LOGGER(loglevel, fmt, ...)        Cnvdep_LogFormat(Cnvdep_GetOmxrLogLevel(loglevel), (CNV_STRING)__func__, __LINE__, fmt, __VA_ARGS__, "")
#else
#define CNV_LOGGER(loglevel, fmt, ...) 
#endif

#ifdef __cplusplus
}
#endif

#endif /* end of include guard */
