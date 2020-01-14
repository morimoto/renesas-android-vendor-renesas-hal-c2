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
 *  OMX Converter os dependence function
 *
 * \file cnv_osdep.c
 * \attention 
 */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/

#include "cnv_type.h"
#include "cnv_osdep.h"

#include "omxr_dep_common.h"
#include "omxr_module_common.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/**
* \struct CNV_THREAD_HANDLE_T
*/
typedef struct {
    CNV_PTR nThreadID;
    CNV_VOID (*thread_function)(
        CNV_PTR pHandle,
        CNV_U32 nEventId,
        CNV_PTR pData);
    CNV_PTR pvParam;
    CNV_PTR pHandle;
    CNV_U32 nCommand;
} CNV_THREAD_HANDLE_T;

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/

static OMX_ERRORTYPE Cnvdep_ThreadEntry(
    OMX_U32 u32ProcId,
    OMX_PTR pvParam,
    OMX_PTR pvContext
);

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/
/**
* \brief debug function. (not use)
*/
CNV_VOID Cnvdep_Open(
    CNV_VOID
)
{
    OmxrOpenOsWrapper();
}

/**
* \brief debug function. (not use)
*/
CNV_VOID Cnvdep_Close(
    CNV_VOID
)
{
    OmxrCloseOsWrapper();
}

/**
* \brief allocate memory
*/
CNV_PTR Cnvdep_Malloc(
    CNV_U32 nSize /**< [in] size */
)
{
    return (CNV_PTR)OmxrMalloc((OMX_U32)nSize);
}

/**
* \brief release memory 
*/
CNV_VOID Cnvdep_Free(
    CNV_PTR pvPtr  /**< [in] pointer */
)
{
    OmxrFree((OMX_PTR)pvPtr);
    return;
}

/**
* \brief allocate memory area for buffer
*/
CNV_PTR Cnvdep_BufferMalloc(
    CNV_U32 nSize /**< [in] size */
)
{
    return (CNV_PTR)OmxrMalloc((OMX_U32)nSize);
}

/**
* \brief release memory for buffer
*/
CNV_VOID Cnvdep_BufferFree(
    CNV_PTR pvPtr /**< [in] pointer */
)
{
    OmxrFree((OMX_PTR)pvPtr);
    return;
}

/**
* \brief copy memory
*/
CNV_PTR Cnvdep_Memcpy(
    CNV_PTR pvDest, /**< [in] dest */
    CNV_PTR pvSrc,  /**< [in] src */ 
    CNV_U32 nSize   /**< [in] size */
)
{
    return OmxrMemcpy((OMX_PTR)pvDest,(OMX_PTR)pvSrc,nSize);
}

/**
* \brief set memory 
*/
CNV_PTR Cnvdep_Memset(
    CNV_PTR pvDest, /**< [in] dest */
    CNV_U8  nCode,  /**< [in] code */
    CNV_U32 nSize   /**< [in] size */
)
{
    return OmxrMemset((OMX_PTR)pvDest,(OMX_U8)nCode,(OMX_U32)nSize);
}

/**
* \brief load a dynamic link library
*/
CNV_ERRORTYPE Cnvdep_LoadDll(
    CNV_STRING path,   /**< [in] file path */
    CNV_HANDLE *handle /**< [in] handle */
)
{
    OMX_ERRORTYPE eError;

    eError = OmxrLoadDll((OMX_STRING)path, handle);
    if (eError != OMX_ErrorNone) {
           CNV_LOGGER(CNV_LOG_DEBUG,"DLL entry not found failed.= %s",path);
        return CNV_ERROR_FATAL;
    } else {
        return CNV_ERROR_NONE;
    }
}

/**
* \brief return the specified entry
*/
CNV_ERRORTYPE Cnvdep_GetDllEntry(
    CNV_HANDLE handle, /**< [in] handle */
    CNV_STRING name,   /**< [in] name */
    CNV_PTR    *entry  /**< [out] entry */
)
{
    OMX_ERRORTYPE eError;
    
    eError = OmxrGetDllEntry(handle, (OMX_STRING)name, (OMX_PTR *)entry);
    if (eError != OMX_ErrorNone) {
        CNV_LOGGER(CNV_LOG_ERROR,"DLL entry not found failed.= %s",name);
        return CNV_ERROR_FATAL;
    } else {
        return CNV_ERROR_NONE;
    }
}

/**
* \brief unload the dynamic link library
*/
CNV_ERRORTYPE Cnvdep_UnloadDll(
    CNV_HANDLE handle /**< [in] handle */
)
{
    OMX_ERRORTYPE eError;

    eError = OmxrUnloadDll(handle);
    if(eError != OMX_ErrorNone){
        CNV_LOGGER(CNV_LOG_DEBUG,"DLL unloaded.= %p",handle);
        return CNV_ERROR_FATAL;
    }else{
        return CNV_ERROR_NONE;
    }
}

/**
* \brief create mutex
*/
CNV_ERRORTYPE Cnvdep_CreateMutex(
    CNV_PTR *nMutex /**< [out] mutex */
)
{
    OMX_ERRORTYPE eError;
    
    eError = OmxrCreateMutex(nMutex, OMX_FALSE);

    if(eError != OMX_ErrorNone){
        return CNV_ERROR_FATAL;    
    }else{
        return CNV_ERROR_NONE;
    }
}    


/**
* \brief destroy mutex
*/
CNV_VOID Cnvdep_DestroyMutex(
    CNV_PTR nMutex /**< [in] mutex */
)
{
    OMX_ERRORTYPE eError;
    
    eError = OmxrDestroyMutex(nMutex);

    if(eError != OMX_ErrorNone){
        return;
    }else{
        return;
    }
}

/**
* \brief lock mutex
*/
CNV_VOID Cnvdep_LockMutex(
	CNV_PTR nMutex /**< [in] mutex */
)
{
    OMX_ERRORTYPE eError;
    
    eError = OmxrLockMutex(nMutex, OMX_TRUE);

    if(eError != OMX_ErrorNone){
        return;
    }else{
        return;
    }
}

/**
* \brief unlock mutex
*/
CNV_VOID Cnvdep_UnlockMutex(
    CNV_PTR nMutex /**< [in] mutex */
)
{
    OMX_ERRORTYPE eError;
    
    eError = OmxrUnlockMutex(nMutex);

    if(eError != OMX_ErrorNone){
        return;
    }else{
        return;
    }
}

/**
*  \brief create handle of osal( Operating system abstraction layer ).
*/
CNV_ERRORTYPE Cnvdep_CreateOsDepHandle(
    CNV_PTR *nThreadID /**< [in] thread id */
)
{
    CNV_THREAD_HANDLE_T *psHandle;
    CNV_ERRORTYPE        eResult = CNV_ERROR_NONE;
    psHandle = (CNV_THREAD_HANDLE_T *)Cnvdep_Malloc( sizeof(CNV_THREAD_HANDLE_T) );
    if ( psHandle == NULL ) {
        CNV_LOGGER(CNV_LOG_ERROR,"Handle failed.= %x", psHandle);
        return CNV_ERROR_FATAL;
    } else {
        (void)OmxrMemset(psHandle,0,sizeof(CNV_THREAD_HANDLE_T));
    }
    *nThreadID = psHandle;
    
    return eResult;
}

/**
* \brief thread entry function
*/
static OMX_ERRORTYPE Cnvdep_ThreadEntry(
	OMX_U32 u32ProcId, /**< [in] thread id */
	OMX_PTR pvParam,   /**< [in] paramter */
	OMX_PTR pvContext  /**< [in] common context */
)
{
    CNV_THREAD_HANDLE_T *psHandle;
    
    if( u32ProcId == 0xFFFFFFFFu ){
        psHandle = (CNV_THREAD_HANDLE_T *)pvParam;
        psHandle->thread_function(psHandle->pHandle,u32ProcId,psHandle->pvParam);
    } else {
        psHandle = (CNV_THREAD_HANDLE_T *)pvContext;
        psHandle->thread_function(psHandle->pHandle,u32ProcId,pvParam);
    }
    return OMX_ErrorNone;
}

/**
* \brief create thread
*/
CNV_ERRORTYPE Cnvdep_CreateThread(
    CNV_PTR nThreadID,            /**< [in] handle */
    CNV_VOID (*thread_function)(  /**< [in] thread function */
        CNV_PTR pHandle,
        CNV_U32 nEventId,
        CNV_PTR pData),
    CNV_PTR pParam                /**< [in] local parameter */
)
{
    CNV_THREAD_HANDLE_T *psHandle;
    OMX_ERRORTYPE         eError = OMX_ErrorNone;
    CNV_ERRORTYPE       eResult = CNV_ERROR_NONE;
    psHandle = (CNV_THREAD_HANDLE_T *)nThreadID;

    psHandle->thread_function = thread_function;
    psHandle->pHandle = pParam;
    
    eError = OmxrCreateThread(&psHandle->nThreadID, &Cnvdep_ThreadEntry, psHandle,"PRIORITY.HIGH.CNV", 0);
    if ( OMX_ErrorNone != eError ) {
        CNV_LOGGER(CNV_LOG_ERROR,"OmxrCreateThread failed.= %x",(CNV_U32)eError);
        eResult = CNV_ERROR_FATAL;
    }
    
    return eResult;
}

/**
* \brief save local context
*/
CNV_ERRORTYPE Cnvdep_SaveTreadContext(
    CNV_PTR nThreadID,  /**< [in] thread id */
    CNV_PTR pParam      /**< [in] local parameter */
)
{
    OMX_ERRORTYPE         eError = OMX_ErrorNone;
    CNV_THREAD_HANDLE_T *psHandle;
    
    psHandle = (CNV_THREAD_HANDLE_T *)nThreadID;
    
    eError = OmxrSaveThreadContext(psHandle->nThreadID,pParam);
    if( eError != OMX_ErrorNone ){
        return CNV_ERROR_FATAL;
    }
    
    return CNV_ERROR_NONE;
}

/**
* \breif exit tjread
*/
CNV_VOID Cnvdep_ThreadExit(
    CNV_VOID *retval /**< [out] retval */
)
{
    (void)OmxrExitThread(retval);
    return;
}

/**
* \brief terminated thread
*/
CNV_U32 Cnvdep_ThreadJoin(
    CNV_PTR nThreadID, /**< [in] thread id */
    CNV_PTR *retval    /**< [out] retval */
)
{    
    OMX_ERRORTYPE iret;
    CNV_U32    func_ret = 0;
    CNV_THREAD_HANDLE_T *psHandle;
    
    psHandle = (CNV_THREAD_HANDLE_T *)nThreadID;
    
    iret = OmxrJoinThread(psHandle->nThreadID, retval );
    if ( iret != OMX_ErrorNone ) {
        CNV_LOGGER(CNV_LOG_ERROR,"nThreadID.= %d",psHandle->nThreadID);
        func_ret = 0xFFFFFFFFu;
    }
    /* Free Handle */
    Cnvdep_Free((CNV_PTR)psHandle);
    
    return func_ret;
}

/**
* \brief send event to thread
*/
CNV_ERRORTYPE Cnvdep_SendEvent(
	CNV_PTR nThreadId, /**< [in] thread id */
    CNV_U32 nEventId,  /**< [in] eventid */
    CNV_PTR pvParam    /**< [in] parameter */
)
{
    CNV_ERRORTYPE eResult = CNV_ERROR_NONE;
    OMX_ERRORTYPE         eError = OMX_ErrorNone;
    CNV_THREAD_HANDLE_T *psHandle = (CNV_THREAD_HANDLE_T*)nThreadId;

    psHandle->pvParam  = pvParam;
    psHandle->nCommand = nEventId;

    eError = OmxrSendEvent(psHandle->nThreadID, (OMX_U32)nEventId, (OMX_PTR)pvParam);
    if( OMX_ErrorNone != eError ){
        CNV_LOGGER(CNV_LOG_ERROR,"SendEvent error.= %d", (OMX_U32)eError);
          eResult = CNV_ERROR_FATAL;
    }
   return eResult;
}

/**
* \brief sleep 
*/
CNV_VOID Cnvdep_Sleep(
    CNV_U32 nSleepTime /**< [in] sleep time */
)
{
    OmxrSleep(nSleepTime);
    return;
}

/**
 * \breif get log level
 */
CNV_U32 Cnvdep_GetOmxrLogLevel(
    CNV_U32 u32Level /**< [in] level */
)
{
    static const CNV_U32 au32Table[] = {OMXR_CNV_LOG_LEVEL_DEBUG, OMXR_CNV_LOG_LEVEL_WARN, OMXR_CNV_LOG_LEVEL_INFO, OMXR_CNV_LOG_LEVEL_ERROR};
    if( u32Level >= CNV_LOG_UNSUSED ){
        u32Level = OMXR_CNV_LOG_LEVEL_ERROR; /* fail safe */
        return u32Level;
    }
    return au32Table[u32Level];
}

#ifndef OMXR_BUILD_LOGGER_DISABLE
/**
 * \brief do output log messages
 */
void Cnvdep_LogFormat(
    CNV_U32    u32Level,    /**< [in] level */
    CNV_STRING strFunction, /**< [in] function name */
    CNV_U32    u32Lineno,   /**< [in] line number */
    CNV_STRING strString,   /**< [in] string */
    ...)
{
    va_list vaArgs;
    va_start(vaArgs, strString);
    OmxrLogFormatVa(u32Level, strFunction, u32Lineno, strString, vaArgs); 
    va_end(vaArgs);
}
#endif

