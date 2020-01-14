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
 * OMXR OS wrapper for Linux
 *
 * This file implements OS wrapper function.
 *
 * \file
 * \attention
 */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/

#include <sys/time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include <omxr_dep_common.h>
#include "omxr_os_wrapper.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

#define OMXR_PRIORITY_HIGH          (60)
#define OMXR_PRIORITY_MID           (50)
#define OMXR_PRIORITY_LOW           (10)

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/* Message data structure */
typedef struct TMessageData {
    OMX_PTR             Message;            /**< Message */
    struct TMessageData *Next;              /**< Pointer to next message */
} MESSAGE_DATA;


/* Message control structure */
typedef struct {
    pthread_cond_t  Semaphore;              /**< Semaphore to indicate message is reached */
    MESSAGE_DATA    *MessageTop;            /**< Pointer to first message */
    MESSAGE_DATA    *MessageLast;           /**< Pointer to last message */
    OMX_U32         Destroy_flg;            /**< Flag of Destroy message box */
    OMX_U32         Recvwait_flg;           /**< Flag of Receive message */
} MESSAGE_INFO_TABLE;


/* Signal Information */
struct SignalInf {
    OMX_U32         pendflg;
    pthread_cond_t  cond;
    pthread_mutex_t mutex;
};

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/

/**
 * OmxrMakeThreadAttribute
 *
 *Make pthread attribute
 *
 * \param[in/out] custom_sched_attr scheduling addtribute
 * \param[in/out] custom_sched_param scheduling paramter
 * \param[in] strPriority Priority string
 * \param[in] pvStackTop Stack top
 * \param[in] u32StackSize Stack size
 * \return OMX_ERRORTYPE
*/
OMXR_STATIC OMX_ERRORTYPE OmxrMakeThreadAttribute(
    pthread_attr_t *custom_sched_attr,
    struct sched_param *custom_sched_param,
    OMX_STRING strPriority,
    OMX_PTR pvStackTop,
    OMX_U32 u32StackSize);

/**
 * OmxrThreadEntry
 *
 * thread entry
 *
 * \param[in] pvInfo Thread handle
 * \return void*   status
*/
OMXR_STATIC void *OmxrThreadEntry(
    void *pvInfo);

/**
 * OmxrGetAbsTimeOut
 *
 * Get absolute time
 *
 * \param[in] s32Timeout Time out period (relative)
 * \param[out] psTimeSpec Time out period (absolute)
 * \return none
*/
OMXR_STATIC void OmxrGetAbsTimeOut(
    OMX_S32 s32Timeout,
    struct timespec *psTimeSpec);

/**
 * OmxrCreateMessageBox
 *
 * Create message box
 *
 * \param[out] pvMessageId Message box ID
 * \return OMX_ERRORTYPE
*/
OMXR_STATIC OMX_ERRORTYPE OmxrCreateMessageBox(
    OMX_PTR *ppvMessageId);

/**
 * OmxrDestroyMessageBox
 *
 * Destroy a  message box
 *
 * \param[in] pvMessageId Message box ID
 * \return OMX_ERRORTYPE
*/
OMXR_STATIC OMX_ERRORTYPE OmxrDestroyMessageBox(
    OMX_PTR pvMessageId);

/**
 * OmxrDoDestroyMessageBox
 *
 * Destroy a  message box main
 *
 * \param[in] pvMessageId Message box ID
 * \return OMX_ERRORTYPE
*/
OMXR_STATIC void OmxrDoDestroyMessageBox(
    MESSAGE_INFO_TABLE *psMessageInfoTable);

/**
 * OmxrPostMessage
 *
 * Post message
 *
 * \param[in] pvMessageId Message box ID
 * \param[in] pvData Message data
 * \return OMX_ERRORTYPE
*/
OMXR_STATIC OMX_ERRORTYPE OmxrPostMessage(
    OMX_PTR pvMessageId,
    OMX_PTR pvData);

/**
 * OmxrReceiveMessage
 *
 * Receive message (blocking)
 *
 * \param[out] ppvData Message data
 * \param[in] pvMessageId Message box ID
 * \return OMX_ERRORTYPE
*/
OMXR_STATIC OMX_ERRORTYPE OmxrReceiveMessage(
    OMX_PTR *ppvData,
    OMX_PTR pvMessageId);

/**
 * OmxrReceiveMessageCleanup
 *
 * OmxrReceiveMessage cleanup handler
 *
 * \param[in] pvArg mutex handle
 * \return none
 * \attention When the pthread_cond_wait function cancelled by the pthread_cancel
 *            function, the associated mutex is locked. This handler unlocks mutex.
*/
OMXR_STATIC void OmxrReceiveMessageCleanup(
    OMX_PTR pvArg);

/***************************************************************************/
/*    Variables                                                            */
/***************************************************************************/

OMXR_STATIC OMX_S32 WrapperClosed = 1;
OMXR_STATIC pthread_mutex_t TableAccessMutex;
OMXR_STATIC pthread_mutex_t ThreadTableAccessMutex;
OMXR_STATIC OMXR_THREAD_HANDLE *ThreadHandlrAnchor;  /* Anchor of the thread handle */

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/

/*============================================
* Thread
*=============================================*/


OMX_ERRORTYPE OmxrCreateThread(OMX_PTR *ppvThreadId, OMXR_THREAD_FUNCTION pFunction, OMX_PTR pvParam, OMX_STRING strPriority, OMX_U32 u32Attribute)
{
    return OmxrCreateThreadEx(ppvThreadId, pFunction, pvParam, strPriority, u32Attribute, NULL, 0);
}


OMX_ERRORTYPE OmxrCreateThreadEx(OMX_PTR *ppvThreadId, OMXR_THREAD_FUNCTION pFunction, OMX_PTR pvParam, OMX_STRING strPriority, OMX_U32 u32Attribute, OMX_PTR pvStackTop, OMX_U32 u32StackSize)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrCreateThreadEx";
    pthread_attr_t custom_sched_attr;
    struct sched_param custom_sched_param;
    OMXR_THREAD_HANDLE *psHandle = NULL;
    OMXR_THREAD_HANDLE *psLast;
    OMX_BOOL bShared;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        eError = (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    } else if ((ppvThreadId == NULL) || (pFunction == NULL)) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        eError = OMX_ErrorBadParameter;
    } else {
        (void)pthread_mutex_lock(&ThreadTableAccessMutex);

        psHandle = NULL;
        bShared = OMX_FALSE;
        if ((u32Attribute & OMXR_THREAD_ATTR_SHARED) != 0) {
            /* Search shared thread */
            psHandle = ThreadHandlrAnchor;
            while (psHandle != NULL) {
                if ((psHandle->pFunction == pFunction) &&
                        ((psHandle->u32Attribute & OMXR_THREAD_ATTR_SHARED) != 0)) {
                    psHandle->u32RefCount += 1;
                    bShared = OMX_TRUE;
                    break;
                }
                psHandle = psHandle->Next;
            }
        }

        if (psHandle == NULL) {
            /* Create new thread */
            psHandle = (OMXR_THREAD_HANDLE*)OmxrMalloc(sizeof(OMXR_THREAD_HANDLE));
            if (psHandle == NULL) {
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Cannot allocate a memory.", strFunction);
                eError = OMX_ErrorUndefined;
            } else {
                if (OmxrCreateMessageBox(&psHandle->pvMessageBox) != OMX_ErrorNone) {
                    OmxrFree(psHandle);
                    OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Cannot create a message box for smap wrapper.", strFunction);
                    eError = OMX_ErrorUndefined;
                }
            }
            if (eError == OMX_ErrorNone) {
                psHandle->pFunction = pFunction;
                psHandle->pvContext = NULL;
                psHandle->u32Attribute = u32Attribute;
                psHandle->Next = NULL;
                psHandle->u32RefCount = 1;
                psHandle->pvThreadReturn = NULL;

                eError = OmxrMakeThreadAttribute(&custom_sched_attr, &custom_sched_param, strPriority, pvStackTop, u32StackSize);
                if (eError != OMX_ErrorNone) {
                    OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OmxrMakeThreadAttribute failed. (error=0x%x)", strFunction, eError);
                    (void)OmxrDestroyMessageBox(psHandle->pvMessageBox);
                    OmxrFree(psHandle);
                } else {
#ifdef OMXR_BUILD_LINUX_NON_ROOT
                    if (pthread_create(&psHandle->hPhysHandle, NULL, &OmxrThreadEntry, psHandle) != 0) {
#else
                    if (pthread_create(&psHandle->hPhysHandle, &custom_sched_attr, &OmxrThreadEntry, psHandle) != 0) {
#endif
                        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_create failed.", strFunction);
                        (void)OmxrDestroyMessageBox(psHandle->pvMessageBox);
                        OmxrFree(psHandle);
                        eError = OMX_ErrorUndefined;
                    } else {
                        /* Add to linked list */
                        if (ThreadHandlrAnchor == NULL) {
                            ThreadHandlrAnchor = psHandle;
                        } else {
                            psLast = ThreadHandlrAnchor;
                            while (psLast->Next != NULL) {
                                psLast = psLast->Next;
                            }
                            psLast->Next = psHandle;
                        }
                        bShared = OMX_FALSE;
                    }
                }
            }
        }
        (void)pthread_mutex_unlock(&ThreadTableAccessMutex);
    }

    if (eError == OMX_ErrorNone) {
        *ppvThreadId = psHandle;
        if (bShared == OMX_TRUE) {
            eError = OmxrSendEvent(psHandle, OMXR_THREAD_PROCID_SHARED, pvParam);
        } else {
            eError = OmxrSendEvent(psHandle, OMXR_THREAD_PROCID_CREATED, pvParam);
        }
    }
    return eError;
}


OMXR_STATIC OMX_ERRORTYPE OmxrMakeThreadAttribute(pthread_attr_t *custom_sched_attr, struct sched_param *custom_sched_param, OMX_STRING strPriority, OMX_PTR pvStackTop, OMX_U32 u32StackSize)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrMakeThreadAttribute";

    if (pthread_attr_init(custom_sched_attr) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_attr_init failed.", strFunction);
        return OMX_ErrorUndefined;
#ifndef ANDROID
    } else if (pthread_attr_setscope(custom_sched_attr, PTHREAD_SCOPE_SYSTEM) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_attr_setscope failed.", strFunction);
        return OMX_ErrorUndefined;
    } else if (pthread_attr_setinheritsched(custom_sched_attr, PTHREAD_EXPLICIT_SCHED) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_attr_setinheritsched failed.", strFunction);
        return OMX_ErrorUndefined;
#endif
    } else if (pthread_attr_setdetachstate(custom_sched_attr, PTHREAD_CREATE_JOINABLE) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_attr_setdetachstate failed.", strFunction);
        return OMX_ErrorUndefined;
    } else if (pthread_attr_setschedpolicy(custom_sched_attr, SCHED_FIFO) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_attr_setschedpolicy failed.", strFunction);
        return OMX_ErrorUndefined;
    } else {
        /* Set priority */
        (void)memset(custom_sched_param, 0, sizeof(struct sched_param));

        if (strPriority == NULL) {
            custom_sched_param->sched_priority = OMXR_PRIORITY_MID;
        } else if (strcmp(strPriority, "PRIORITY.HIGH") == 0) {
            custom_sched_param->sched_priority = OMXR_PRIORITY_HIGH;
        } else if (strcmp(strPriority, "PRIORITY.LOW") == 0) {
            custom_sched_param->sched_priority = OMXR_PRIORITY_LOW;
        } else {
            custom_sched_param->sched_priority = OMXR_PRIORITY_MID;
        }

        if (pthread_attr_setschedparam(custom_sched_attr, custom_sched_param) != 0) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_attr_setschedparam failed.", strFunction);
            return OMX_ErrorUndefined;
        }

        /* Set stack */
        if ((pvStackTop != NULL) && (u32StackSize == 0)) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Stack top is specified but no size specified.", strFunction);
            return OMX_ErrorUndefined;
        } else if ((pvStackTop == NULL) && (u32StackSize != 0)) {
            if (pthread_attr_setstacksize(custom_sched_attr, u32StackSize) != 0) {
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_attr_setstacksize failed.", strFunction);
                return OMX_ErrorUndefined;
            }
        } else if ((pvStackTop != NULL) && (u32StackSize != 0)) {
            if (pthread_attr_setstack(custom_sched_attr, (void*)pvStackTop, u32StackSize) != 0) {
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_attr_setstack failed.", strFunction);
                return OMX_ErrorUndefined;
            }
        } else {
        }
    }
    return OMX_ErrorNone;
}


OMXR_STATIC void *OmxrThreadEntry(void *pvInfo)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrThreadEntry";
    OMX_ERRORTYPE eError;
    OMXR_THREAD_HANDLE *psHandle;
    OMXR_THREAD_COMMEND_MESSAGE *psCommandMessage;
    OMXR_THREAD_RESULT_MESSAGE *psResultMessage;
    OMX_BOOL bExit = OMX_FALSE;

    psHandle = (OMXR_THREAD_HANDLE*)pvInfo;
    /* Command processing loop */
    while (bExit == OMX_FALSE) {
        eError = OmxrReceiveMessage((void**)&psCommandMessage, (void*)psHandle->pvMessageBox);
        if (eError == OMX_ErrorNone) {
            if (psCommandMessage->u32ProcId != OMXR_THREAD_PROCID_RESERVED) {
                /* Call thread function */
                eError = (psHandle->pFunction)(psCommandMessage->u32ProcId, psCommandMessage->pvInfo, psHandle->pvContext);
            }

            if ((psCommandMessage->u32ProcId == OMXR_THREAD_PROCID_RESERVED) ||
                    (psCommandMessage->u32ProcId == OMXR_THREAD_PROCID_TERMINATE)) {
                bExit = OMX_TRUE;
                psHandle->pFunction = NULL;
                psHandle->pvContext = NULL;
            }

            if (0 != psCommandMessage->pvMessageBox) {
                /* Send back a result value */
                psResultMessage = OmxrMalloc(sizeof(OMXR_THREAD_RESULT_MESSAGE));
                if (psResultMessage == NULL) {
                    OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: malloc failed. ", strFunction);
                } else {
                    psResultMessage->eErrorType = eError;
                    eError = OmxrPostMessage(psCommandMessage->pvMessageBox, psResultMessage);
                    if (eError != OMX_ErrorNone) {
                        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OmxrPostMessage failed. ", strFunction);
                    }
                }
            }
            OmxrFree(psCommandMessage);
        } else {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OmxrReceiveMessage failed. (error=0x%x)", strFunction, eError);
            psHandle->pFunction = NULL;
            psHandle->pvContext = NULL;
            bExit = OMX_TRUE;
        }
    }
    pthread_exit(NULL);
    return 0;
}


OMX_ERRORTYPE OmxrSaveThreadContext(OMX_PTR pvThreadId, OMX_PTR pvContext)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrSaveThreadContext";
    OMXR_THREAD_HANDLE *psHandle = (OMXR_THREAD_HANDLE*)pvThreadId;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        eError = (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    } else if (psHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        eError = OMX_ErrorBadParameter;
    } else {
        psHandle->pvContext = pvContext;
    }
    return eError;
}


OMX_ERRORTYPE OmxrSendEvent(OMX_PTR pvThreadId, OMX_U32 u32ProcId, OMX_PTR pvParam)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrSendEvent";
    OMX_ERRORTYPE eError;
    OMXR_THREAD_HANDLE *psHandle = (OMXR_THREAD_HANDLE*)pvThreadId;
    OMXR_THREAD_COMMEND_MESSAGE *psCommandMessage;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        eError = (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    } else if (psHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        eError = OMX_ErrorBadParameter;
    } else {
        psCommandMessage = OmxrMalloc(sizeof(OMXR_THREAD_COMMEND_MESSAGE));
        if (psCommandMessage == NULL) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Cannot allocate a memory.", strFunction);
            eError = OMX_ErrorUndefined;
        } else {
            /* Post message to thread */
            psCommandMessage->u32ProcId = u32ProcId;
            psCommandMessage->pvInfo = pvParam;
            psCommandMessage->pvMessageBox = 0;
            eError = OmxrPostMessage(psHandle->pvMessageBox, psCommandMessage);
            if (eError != OMX_ErrorNone) {
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OmxrPostMessage failed. (error=0x%x)", strFunction, eError);
            }
        }
    }
    return eError;
}


OMX_ERRORTYPE OmxrGetThreadId(OMX_PTR *ppvThreadId)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrGetThreadId";
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    pthread_t hThread;
    OMXR_THREAD_HANDLE *psHandle;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        eError = (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    } else if (ppvThreadId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        eError = OMX_ErrorBadParameter;
    } else {
        hThread = pthread_self();

        (void)pthread_mutex_lock(&ThreadTableAccessMutex);

        psHandle = ThreadHandlrAnchor;
        while (psHandle != NULL) {
            if (psHandle->hPhysHandle == hThread) {
                *ppvThreadId = psHandle;
                eError = OMX_ErrorNone;
                break;
            }
            psHandle = psHandle->Next;
        }
        (void)pthread_mutex_unlock(&ThreadTableAccessMutex);
        if (eError != OMX_ErrorNone) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Current thread is not registerd.", strFunction);
        }
    }
    return eError;
}


OMX_ERRORTYPE OmxrExitThread(OMX_PTR pvReturn)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrExitThread";
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXR_THREAD_HANDLE *psHandle;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        eError = (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    } else {
        eError = OmxrGetThreadId((OMX_PTR)&psHandle);
        if (eError == OMX_ErrorNone) {
            (void)pthread_mutex_lock(&ThreadTableAccessMutex);

            if (0 < psHandle->u32RefCount) {
                psHandle->u32RefCount -= 1;
            }
            if (psHandle->u32RefCount == 0) {
                psHandle->pvThreadReturn = pvReturn;
                eError = OmxrSendEvent(psHandle, OMXR_THREAD_PROCID_RESERVED, NULL);
            } else {
                eError = (OMX_ERRORTYPE)OMXR_Util_ErrorThreadShared;
            }
            (void)pthread_mutex_unlock(&ThreadTableAccessMutex);
        }
    }
    return eError;
}


OMX_ERRORTYPE OmxrTerminateThread(OMX_PTR pvThreadId)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrTerminateThread";
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXR_THREAD_HANDLE *psHandle = (OMXR_THREAD_HANDLE*)pvThreadId;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        eError = (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    } else if (psHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        eError = OMX_ErrorBadParameter;
    } else {
        (void)pthread_mutex_lock(&ThreadTableAccessMutex);

        if (0 < psHandle->u32RefCount) {
            psHandle->u32RefCount -= 1;
        }
        if (psHandle->u32RefCount == 0) {
            eError = OmxrSendEvent(psHandle, OMXR_THREAD_PROCID_TERMINATE, NULL);
        } else {
            eError = OmxrSendEvent(psHandle, OMXR_THREAD_PROCID_REMAINED, NULL);
            if (eError == OMX_ErrorNone) {
                eError = (OMX_ERRORTYPE)OMXR_Util_ErrorThreadShared;
            }
        }
        (void)pthread_mutex_unlock(&ThreadTableAccessMutex);
    }
    return eError;
}


OMX_ERRORTYPE OmxrJoinThread(OMX_PTR pvThreadId, OMX_PTR *ppvThreadReturn)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrJoinThread";
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXR_THREAD_HANDLE *psHandle = (OMXR_THREAD_HANDLE*)pvThreadId;
    OMXR_THREAD_HANDLE *psLast;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        eError = (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    } else if (psHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        eError = OMX_ErrorBadParameter;
    } else {
        /* Wait until thread is terminated */
        while (psHandle->pFunction != NULL) {
            OmxrSleep(10);
        }

        /* Get thread return value */
        if (ppvThreadReturn != NULL) {
            *ppvThreadReturn = psHandle->pvThreadReturn;
        }

        (void)pthread_mutex_lock(&ThreadTableAccessMutex);

        /* Remove from linked list */
        if (psHandle == ThreadHandlrAnchor) {
            ThreadHandlrAnchor = psHandle->Next;
        } else {
            psLast = ThreadHandlrAnchor;
            while (psLast->Next != psHandle) {
                psLast = psLast->Next;
            }
            psLast->Next = psHandle->Next;
        }

        (void)pthread_mutex_unlock(&ThreadTableAccessMutex);

        /* Delete message box */
        eError = OmxrDestroyMessageBox(psHandle->pvMessageBox);
        if (eError != OMX_ErrorNone) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OmxrDestroyMessageBox failed. (error=0x%x)", strFunction, eError);
        }

        /* Wait until thread is terminated */
        (void)pthread_join(psHandle->hPhysHandle, NULL);

        /* Delete thread handle */
        OmxrFree(psHandle);

    }
    return eError;
}


void OmxrSleep(OMX_U32 u32msec)
{
    useconds_t usec = (useconds_t)(u32msec * 1000);
    (void)usleep(usec);

    return;
}

/*============================================
* Mutex
*=============================================*/


OMX_ERRORTYPE OmxrCreateMutex(OMX_PTR *ppvMutexId, OMX_BOOL bInitialOwner)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrCreateMutex";
    pthread_mutex_t *psMutex;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        return (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    }
    if (ppvMutexId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        return (OMX_ERRORTYPE)OMX_ErrorBadParameter;
    }

    psMutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (psMutex == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Cannot allocate a memory.", strFunction);
        return OMX_ErrorUndefined;
    }

    (void)pthread_mutex_init(psMutex, NULL);

    *ppvMutexId = psMutex;

    if (bInitialOwner == OMX_TRUE) {
        (void)pthread_mutex_lock(psMutex);
    }

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OmxrDestroyMutex(OMX_PTR pvMutexId)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrDestroyMutex";
    OMX_U32 u32RetVal;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        return (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    }
    if (pvMutexId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        return OMX_ErrorBadParameter;
    }

    u32RetVal = (OMX_U32)pthread_mutex_destroy((pthread_mutex_t*)pvMutexId);
    if (u32RetVal != 0) {
        if (u32RetVal == EBUSY) {
            return (OMX_ERRORTYPE)OMXR_Util_ErrorBusy;
        } else {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_mutex_destroy failed.", strFunction);
            return OMX_ErrorUndefined;
        }
    }
    free((void*)pvMutexId);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OmxrLockMutex(OMX_PTR pvMutexId, OMX_BOOL bBlocking)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrLockMutex";
    OMX_U32 u32RetVal;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        return (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    }
    if (pvMutexId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        return OMX_ErrorBadParameter;
    }

    if (bBlocking == OMX_TRUE) {
        if (pthread_mutex_lock((pthread_mutex_t*)pvMutexId) != 0) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_mutex_lock failed.", strFunction);
            return OMX_ErrorUndefined;
        }
    } else {
        u32RetVal = (OMX_U32)pthread_mutex_trylock((pthread_mutex_t*)pvMutexId);
        if (u32RetVal != 0) {
            if (u32RetVal == EBUSY) {
                return (OMX_ERRORTYPE)OMXR_Util_ErrorBusy;
            } else {
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_mutex_trylock failed.", strFunction);
                return OMX_ErrorUndefined;
            }
        }
    }

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OmxrUnlockMutex(OMX_PTR pvMutexId)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrUnlockMutex";

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        return (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    }
    if (pvMutexId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        return OMX_ErrorBadParameter;
    }

    if (pthread_mutex_unlock((pthread_mutex_t*)pvMutexId) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_mutex_unlock failed.", strFunction);
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}


/*============================================
* Signal
*=============================================*/


OMX_ERRORTYPE OmxrCreateSignal(OMX_PTR *ppvSignalId, OMX_BOOL bSignaled)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrCreateSignal";
    struct  SignalInf *psSigInf;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        return (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    }
    if (ppvSignalId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        return OMX_ErrorBadParameter;
    }

    psSigInf = (struct SignalInf*)malloc(sizeof(struct SignalInf));
    if (psSigInf == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Cannot allocate a memory.", strFunction);
        return OMX_ErrorUndefined;
    }

    psSigInf->pendflg = 0;    /* no signal */

    if (pthread_cond_init((pthread_cond_t*)&(psSigInf->cond), NULL) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_cond_init failed.", strFunction);
        return OMX_ErrorUndefined;
    }

    if (pthread_mutex_init((pthread_mutex_t*)&(psSigInf->mutex), NULL) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_mutex_init failed.", strFunction);
        return OMX_ErrorUndefined;
    }

    if (bSignaled == OMX_TRUE) {
        (void)pthread_mutex_lock((pthread_mutex_t*)&(psSigInf->mutex));

        psSigInf->pendflg = 1;

        if (pthread_cond_broadcast((pthread_cond_t*)&(psSigInf->cond)) != 0) {
            (void)pthread_mutex_unlock((pthread_mutex_t*)&(psSigInf->mutex));
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_cond_broadcast failed.", strFunction);
            return OMX_ErrorUndefined;
        }
        (void)pthread_mutex_unlock((pthread_mutex_t*)&(psSigInf->mutex));
    }

    *ppvSignalId = psSigInf;

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OmxrDestroySignal(OMX_PTR pvSignalId)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrDestroySignal";
    struct SignalInf *psSigInf;
    OMX_U32 u32RetVal;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        return (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    }
    if (pvSignalId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        return OMX_ErrorBadParameter;
    }

    psSigInf = (struct SignalInf*)pvSignalId;

    (void)pthread_mutex_lock((pthread_mutex_t*)&(psSigInf->mutex));

    u32RetVal = (OMX_U32)pthread_cond_destroy((pthread_cond_t*)&(psSigInf->cond));
    if (u32RetVal != 0) {
        if (u32RetVal == EBUSY) { /* othre thread is sigwait. */
            (void)pthread_mutex_unlock((pthread_mutex_t*)&(psSigInf->mutex));
            return (OMX_ERRORTYPE)OMXR_Util_ErrorBusy;
        } else {
            (void)pthread_mutex_unlock((pthread_mutex_t*)&(psSigInf->mutex));
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_cond_destroy failed.", strFunction);
            return OMX_ErrorUndefined;
        }
    }

    psSigInf->pendflg = 0;    /* no signal */
    (void)pthread_mutex_unlock((pthread_mutex_t*)&(psSigInf->mutex));

    if (pthread_mutex_destroy((pthread_mutex_t*)&(psSigInf->mutex)) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_mutex_destroy failed.", strFunction);
        return OMX_ErrorUndefined;
    }
    free((void*)pvSignalId);

    return OMX_ErrorNone;
}


OMX_ERRORTYPE OmxrWaitSignal(OMX_PTR pvSignalId, OMX_S32 s32Timeout)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrWaitSignal";
    struct timespec sTimeSpec;
    struct SignalInf *psSigInf;
    OMX_U32 u32RetVal;
    OMX_ERRORTYPE eError;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        return (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    }
    if (pvSignalId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        return OMX_ErrorBadParameter;
    }

    psSigInf = (struct SignalInf*)pvSignalId;

    (void)pthread_mutex_lock((pthread_mutex_t*)&(psSigInf->mutex));

    if (psSigInf->pendflg == 1) {
        psSigInf->pendflg = 0;
        eError = OMX_ErrorNone;
    } else {
        if (s32Timeout == 0) {
            eError = (OMX_ERRORTYPE)OMXR_Util_ErrorNoSignal;
        }
        /*-------------------------------------------
         * cond wait                                 
         *------------------------------------------*/
        else if (s32Timeout == -1) {
            (void)pthread_cond_wait((pthread_cond_t*)&(psSigInf->cond), (pthread_mutex_t*)&(psSigInf->mutex));
            psSigInf->pendflg = 0;
            eError = OMX_ErrorNone;
        }
        /*-------------------------------------------
         * timedout wait                             
         *------------------------------------------*/
        else if (s32Timeout > 0) {
            OmxrGetAbsTimeOut(s32Timeout, &sTimeSpec);
            u32RetVal = (OMX_U32)pthread_cond_timedwait((pthread_cond_t*)&(psSigInf->cond), (pthread_mutex_t*)&(psSigInf->mutex), &sTimeSpec);
            if (u32RetVal != 0) {
                if (u32RetVal == ETIMEDOUT) {
                    eError = OMX_ErrorTimeout;
                } else {
                    OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_cond_timedwait failed.", strFunction);
                    eError = OMX_ErrorUndefined;
                }
            } else {
                psSigInf->pendflg = 0;
                eError = OMX_ErrorNone;
            }
        }
        /*-------------------------------------------
         * error                                     
         *------------------------------------------*/
        else {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
            eError = OMX_ErrorBadParameter;
        }
    }

    (void)pthread_mutex_unlock((pthread_mutex_t*)&(psSigInf->mutex));

    return eError;
}


OMX_ERRORTYPE OmxrSignal(OMX_PTR pvSignalId)
{
    OMXR_STATIC OMXR_CONST_STRING strFunction = "OmxrSignal";
    struct SignalInf *psSigInf;

    if (WrapperClosed == 1) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: OS wrapper is closed.", strFunction);
        return (OMX_ERRORTYPE)OMXR_Util_ErrorOsWrapper;
    }
    if (pvSignalId == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: Bad parameter.", strFunction);
        return OMX_ErrorBadParameter;
    }

    psSigInf = (struct SignalInf*)pvSignalId;

    (void)pthread_mutex_lock((pthread_mutex_t*)&(psSigInf->mutex));

    psSigInf->pendflg = 1;

    if (pthread_cond_broadcast((pthread_cond_t*)&(psSigInf->cond)) != 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "%s: error: pthread_cond_broadcast failed.", strFunction);
        (void)pthread_mutex_unlock((pthread_mutex_t*)&(psSigInf->mutex));
        return OMX_ErrorUndefined;
    }

    (void)pthread_mutex_unlock((pthread_mutex_t*)&(psSigInf->mutex));

    return OMX_ErrorNone;
}


OMXR_STATIC void OmxrGetAbsTimeOut(OMX_S32 s32Timeout, struct timespec *psTimeSpec)
{
    struct timeval sTimeVal;

    (void)gettimeofday(&sTimeVal, NULL);

    psTimeSpec->tv_sec = sTimeVal.tv_sec + (time_t)(s32Timeout / 1000);
    psTimeSpec->tv_nsec = (sTimeVal.tv_usec + ((s32Timeout % 1000) * 1000)) * 1000;

    /* Check on nsec overflow */
    if (psTimeSpec->tv_nsec >= 1000000000) {
        psTimeSpec->tv_sec += psTimeSpec->tv_nsec / 1000000000;
        psTimeSpec->tv_nsec = psTimeSpec->tv_nsec % 1000000000;
    }
    return;
}


/*============================================
* Others
*=============================================*/


void OmxrOpenOsWrapper(void)
{
    OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_DEBUG, "Open OS Wrapper", "");

    if (WrapperClosed == 1) {
        WrapperClosed = 0;
        (void)pthread_mutex_init(&TableAccessMutex, NULL);
        (void)pthread_mutex_init(&ThreadTableAccessMutex, NULL);
        OmxrInitMalloc();
    }
    return;
}


void OmxrCloseOsWrapper(void)
{
    OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_DEBUG, "Close OS Wrapper", "");

    (void)pthread_mutex_lock(&ThreadTableAccessMutex);
    (void)pthread_mutex_destroy(&ThreadTableAccessMutex);

    (void)pthread_mutex_lock(&TableAccessMutex);
    (void)pthread_mutex_destroy(&TableAccessMutex);

    OmxrDeinitMalloc();
    WrapperClosed = 1;
    return;
}


/*============================================
* Message box
*=============================================*/


OMXR_STATIC OMX_ERRORTYPE OmxrCreateMessageBox(OMX_PTR *ppvMessageId)
{
    MESSAGE_INFO_TABLE *psMessageInfoTable;

    (void)pthread_mutex_lock(&TableAccessMutex);

    psMessageInfoTable = (MESSAGE_INFO_TABLE*)malloc(sizeof(MESSAGE_INFO_TABLE));

    if (psMessageInfoTable == NULL) {
        /* memory allocation error. */
        (void)pthread_mutex_unlock(&TableAccessMutex);
        return OMX_ErrorUndefined;
    } else {
        (void)pthread_cond_init(&psMessageInfoTable->Semaphore, NULL);
        psMessageInfoTable->MessageTop = NULL;
        psMessageInfoTable->MessageLast = NULL;
        psMessageInfoTable->Destroy_flg = 0;
        psMessageInfoTable->Recvwait_flg = 0;
    }

    *ppvMessageId = psMessageInfoTable;

    (void)pthread_mutex_unlock(&TableAccessMutex);

    return OMX_ErrorNone;
}


OMXR_STATIC OMX_ERRORTYPE OmxrDestroyMessageBox(OMX_PTR pvMessageId)
{
    MESSAGE_INFO_TABLE *psMessageInfoTable;

    (void)pthread_mutex_lock(&TableAccessMutex);

    psMessageInfoTable = (MESSAGE_INFO_TABLE*)pvMessageId;

    if (psMessageInfoTable->Recvwait_flg == 0) {
        OmxrDoDestroyMessageBox(psMessageInfoTable);
    } else {
        psMessageInfoTable->Destroy_flg = 1;
        /* Set semaphore */
        (void)pthread_cond_signal(&psMessageInfoTable->Semaphore);
    }

    (void)pthread_mutex_unlock(&TableAccessMutex);

    return OMX_ErrorNone;
}


OMXR_STATIC void OmxrDoDestroyMessageBox(MESSAGE_INFO_TABLE *psMessageInfoTable)
{
    MESSAGE_DATA *psMessageData;

    (void)pthread_cond_destroy(&psMessageInfoTable->Semaphore);

    /* Delete all messages held by this message box */
    while (psMessageInfoTable->MessageTop != NULL) {
        psMessageData = psMessageInfoTable->MessageTop;
        psMessageInfoTable->MessageTop = psMessageData->Next;

        OmxrFree(psMessageData->Message);

        free(psMessageData);
    }
    psMessageInfoTable->MessageTop = NULL;
    psMessageInfoTable->MessageLast = NULL;

    free((void*)psMessageInfoTable);

    return;
}


OMXR_STATIC OMX_ERRORTYPE OmxrPostMessage(OMX_PTR pvMessageId, OMX_PTR pvData)
{
    MESSAGE_INFO_TABLE *psMessageInfoTable;
    MESSAGE_DATA *psMessageData;
    OMX_ERRORTYPE eError;

    (void)pthread_mutex_lock(&TableAccessMutex);

    psMessageInfoTable = (MESSAGE_INFO_TABLE*)pvMessageId;

    eError = OMX_ErrorUndefined;

    if (pvMessageId != NULL) {
        /* Set message to queue */
        psMessageData = (MESSAGE_DATA*)malloc(sizeof(MESSAGE_DATA));

        if (psMessageData == NULL) {
            /* memory allocation error. */
            eError = OMX_ErrorUndefined;
        } else {
            psMessageData->Message = pvData;
            psMessageData->Next = NULL;
            if (psMessageInfoTable->MessageTop == NULL) {
                psMessageInfoTable->MessageTop = psMessageData;
            } else {
                psMessageInfoTable->MessageLast->Next = psMessageData;
            }
            psMessageInfoTable->MessageLast = psMessageData;

            /* Set semaphore */
            (void)pthread_cond_signal(&psMessageInfoTable->Semaphore);
            eError = OMX_ErrorNone;
        }
    }

    (void)pthread_mutex_unlock(&TableAccessMutex);

    return eError;
}


OMXR_STATIC OMX_ERRORTYPE OmxrReceiveMessage(OMX_PTR *ppvData, OMX_PTR pvMessageId)
{
    MESSAGE_INFO_TABLE *psMessageInfoTable;
    MESSAGE_DATA *psMessageData;
    OMX_ERRORTYPE eError;

    (void)pthread_mutex_lock(&TableAccessMutex);

    psMessageInfoTable = (MESSAGE_INFO_TABLE*)pvMessageId;

    eError = (OMX_ERRORTYPE)OMXR_Util_ErrorNothingToDo;

    if (pvMessageId != NULL) {
        do {
            if (psMessageInfoTable->MessageTop != NULL) {
                /* Get message data */
                psMessageData = psMessageInfoTable->MessageTop;
                *ppvData = psMessageData->Message;
                psMessageInfoTable->MessageTop = psMessageData->Next;

                free(psMessageData);
                eError = OMX_ErrorNone;
            } else {
                /* Recieve wait flag set */
                psMessageInfoTable->Recvwait_flg = 1;

                /* wait until message is reached */
                pthread_cleanup_push(&OmxrReceiveMessageCleanup, &TableAccessMutex);
                (void)pthread_cond_wait(&psMessageInfoTable->Semaphore, &TableAccessMutex);
                pthread_cleanup_pop(0);

                /* Recieve wait flag clear */
                psMessageInfoTable->Recvwait_flg = 0;

                if (psMessageInfoTable->Destroy_flg == 1) {
                    OmxrDoDestroyMessageBox(psMessageInfoTable);
                    eError = OMX_ErrorUndefined;      /* destroy message box error */
                }
            }
        } while ((OMX_ERRORTYPE)OMXR_Util_ErrorNothingToDo == eError);
    }

    (void)pthread_mutex_unlock(&TableAccessMutex);

    return eError;
}


OMXR_STATIC void OmxrReceiveMessageCleanup(OMX_PTR pvArg)
{
    pthread_mutex_t *psMutex;

    psMutex = (pthread_mutex_t*)pvArg;

    (void)pthread_mutex_unlock(psMutex);

    return;
}
