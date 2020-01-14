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
 * OMXR workbuffer allocate/free function
 *
 * This file implements the buffer allocation function.
 *
 * \file
 * \attention
 */
 
/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/

#include <stdlib.h>
#include <omxr_dep_common.h>

#ifndef OMXR_WORKBUFFER_FUNC_DISABLE_USING_MMNGR_LIB
#include "mmngr_user_public.h"
#else
typedef OMX_S32 MMNGR_ID;
#define MMNGR_VA_SUPPORT (0)
#define MMNGR_PA_SUPPORT (1)
#define R_MM_OK          (0)
#define R_MM_FATAL       (-1)
#define R_MM_SEQE        (-2)
#define R_MM_PARE        (-3)
#define R_MM_NOMEM       (-4)

#define mmngr_alloc_in_user_ext(...)    ( OmxrWorkbufferfuncFail("mmngr_alloc_in_user_ext") )
#define mmngr_share_in_user_ext(...)    ( OmxrWorkbufferfuncFail("mmngr_share_in_user_ext") )
#define mmngr_release_in_user_ext(...)  ( OmxrWorkbufferfuncFail("mmngr_release_in_user_ext") )
#define mmngr_free_in_user_ext(...)     ( OmxrWorkbufferfuncFail("mmngr_free_in_user_ext") )

OMXR_STATIC OMX_S32 OmxrWorkbufferfuncFail(OMX_STRING str);
OMXR_STATIC OMX_S32 OmxrWorkbufferfuncFail(OMX_STRING str)
{
	OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "OMXR_WORKBUFFER_FUNC_DISABLE_USING_MMNGR_LIB is defined, but %s() is called.", str);
	return R_MM_FATAL;
}
#endif

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
#define ALLOCATE_ALIGNE_FRAMEMEM_SIZE   (0x00001000) /* 4096 */

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

typedef struct tagOMXR_WORKBUFFER_HANDLE_INFO {
    MMNGR_ID hMmHandle;          /**< Memory Manager Handle (integer value)        */
    MMNGR_ID hMapSpaceHandle;    /**< Memory Mapping Space Handle (integer value)  */
    OMX_PTR  pvBaseVirtAddr;     /**< Address that was mapped to the CPU */
    OMX_U32  u32BaseHardIpAddr;  /**< Address that was mapped to the Hardware IP */
    OMX_U32  u32AddrOffset;      /**< Offset */
    OMX_U32  u32ValidSize;       /**< Valied size */
    OMX_U32  u32AllocSize;       /**< Allocated size */
} OMXR_WORKBUFFER_HANDLE_INFO;

/***************************************************************************/
/*    Function Prototypes (private)                                        */
/***************************************************************************/

/**
 * OmxrAllocateWorkBuffer_allocfunc
 *
 * This function allocates the work buffer.
 *
 * \param[in,out] psInfo Buffer handle
 * \param[in] u32ReqSize Require size
 * \param[in] u32ReqAlign Align byte
 * \param[in] strBufferDiscri Buffer discriptor
 * \param[in] u32Flags Mapping flag
 * \return OMX_ERROTTYPE
 */
OMXR_STATIC OMX_ERRORTYPE OmxrAllocateWorkBuffer_allocfunc(OMXR_WORKBUFFER_HANDLE_INFO *psInfo, OMX_U32 u32ReqSize, OMX_U32 u32ReqAlign, OMX_STRING strBufferDiscri, OMX_U32 u32Flags);

/**
 * OmxrAllocateWorkBuffer_freefunc
 *
 * This function free the work buffer.
 *
 * \param[in,out] psInfo Buffer handle
 * \return OMX_ERROTTYPE
 */
OMXR_STATIC OMX_ERRORTYPE OmxrAllocateWorkBuffer_freefunc(OMXR_WORKBUFFER_HANDLE_INFO *psInfo);

/***************************************************************************/
/*    Variables                                                            */
/***************************************************************************/

/***************************************************************************/
/*    Functions                                                            */
/***************************************************************************/

OMX_ERRORTYPE OmxrAllocateWorkBuffer(OMXR_WORKBUFFER_HANDLE* pBufferHandle, OMX_U32 u32Size, OMX_U32 u32Align, OMX_STRING strBufferDiscri, OMX_U32 u32Flags)
{
    OMX_U32 u32CalcAlign, u32CalcSize;
    OMXR_WORKBUFFER_HANDLE_INFO *psInfo;
    OMX_ERRORTYPE eError;

    /* Check handle pointer */
    if (pBufferHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Bad parameter.", "");
        return OMX_ErrorBadParameter;
    }
    
    /* Check allocation size */
    if (u32Size == 0) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Bad parameter.", "");
        return OMX_ErrorBadParameter;
    }

    /* Allocate information area */
    psInfo = (OMXR_WORKBUFFER_HANDLE_INFO *)OmxrMalloc(sizeof(OMXR_WORKBUFFER_HANDLE_INFO));
    if (psInfo == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: malloc error.", "");
        return OMX_ErrorUndefined;
    }
    
    OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_INFO, "OmxrAllocateWorkBuffer (pBufferHandle=%p, u32Size=%u, u32Align=%u, strBufferDiscri=%s, u32Flags=%u)", pBufferHandle, u32Size, u32Align, strBufferDiscri, u32Flags);
    /* Check buffer type - strBufferDiscri Strings*/
    if (!OmxrStrcmp("OMXBUF.VIDEO.WORKBUFFER.DEC.FRAMEMEM", strBufferDiscri)) {
    /* XY Addressing workbuffer case*/
        u32CalcSize  = u32Size;
        u32CalcAlign = 1;

        eError = OmxrAllocateWorkBuffer_allocfunc(psInfo, u32CalcSize, u32CalcAlign, strBufferDiscri, u32Flags);
        if (eError == OMX_ErrorNone) {
            /* check aligment */
            if ((psInfo->u32BaseHardIpAddr & (ALLOCATE_ALIGNE_FRAMEMEM_SIZE - 1u)) != 0) {
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_DEBUG, "bad aligment(0x%x).", psInfo->u32BaseHardIpAddr);
                /* If it can not allocate the alignment as intended, it is retried by adding an alignment area. */
                (void)OmxrAllocateWorkBuffer_freefunc(psInfo);
                u32CalcAlign = ALLOCATE_ALIGNE_FRAMEMEM_SIZE;
                eError = OmxrAllocateWorkBuffer_allocfunc(psInfo, u32CalcSize, u32CalcAlign, strBufferDiscri, u32Flags);
            }
        }
    } else {
    /* Other case */
        u32CalcSize  = u32Size;
        u32CalcAlign = u32Align;

        eError = OmxrAllocateWorkBuffer_allocfunc(psInfo, u32CalcSize, u32CalcAlign, strBufferDiscri, u32Flags);
    }
    
    if (eError != OMX_ErrorNone) {
        OmxrFree(psInfo);
    }
    else{
      *pBufferHandle = psInfo;
      OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_INFO, "[Allocated Buffer Info] strBufferDiscri=%s, pvBaseVirtAddr=%p, u32BaseHardIpAddr=0x%08x, u32AddrOffset=%u, u32ValidSize=%u, u32AllocSize=%u, u32Flags=%u",
          strBufferDiscri,
          psInfo->pvBaseVirtAddr,
          psInfo->u32BaseHardIpAddr,
          psInfo->u32AddrOffset,
          psInfo->u32ValidSize,
          psInfo->u32AllocSize,
          u32Flags
      );
    }
    
    return eError;
}

OMXR_STATIC OMX_ERRORTYPE OmxrAllocateWorkBuffer_allocfunc(OMXR_WORKBUFFER_HANDLE_INFO *psInfo, OMX_U32 u32ReqSize, OMX_U32 u32ReqAlign, OMX_STRING strBufferDiscri, OMX_U32 u32Flags)
{
    MMNGR_ID pid;
    void *mem_param;
    OMX_U32 flags;
    OMX_S32 ret;
    OMX_PTR pvVirt;
    OMX_U32 u32hard;
    OMX_U32 u32AllocReqSize;

    u32AllocReqSize = (u32ReqSize + u32ReqAlign) - 1u;

#ifdef OMXR_WORKBUFFER_FUNC_DISABLE_USING_VA_SUPPORT
	if ((u32Flags & OMXR_WORKBUFFER_HA_SUPPORT) == OMXR_WORKBUFFER_HA_SUPPORT) {
		u32Flags  &= ~OMXR_WORKBUFFER_VA_SUPPORT;
		u32Flags |= OMXR_WORKBUFFER_TA_SUPPORT;
		OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_INFO, "Use MMNGR_PA_SUPPORT instead of MMNGR_VA_SUPPORT u32Flags=0x%x", u32Flags);
	}
#endif

    /* Branch allocation area */
    if (((u32Flags & OMXR_WORKBUFFER_VA_SUPPORT) == OMXR_WORKBUFFER_VA_SUPPORT) &&
        ((u32Flags & OMXR_WORKBUFFER_TA_SUPPORT) == OMXR_WORKBUFFER_TA_SUPPORT)) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Not support flag.", "");
        return OMX_ErrorBadParameter;
    } else if (u32Flags == OMXR_WORKBUFFER_VA_SUPPORT) {
        /* Allocation heap */
        pvVirt = (OMX_PTR)malloc(u32AllocReqSize);
        if (pvVirt == NULL) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Cannot allocate a work buffer. (size=%d)", u32AllocReqSize);
            return OMX_ErrorInsufficientResources;
        }
        psInfo->hMmHandle         = 0;
        psInfo->hMapSpaceHandle   = 0;
        psInfo->pvBaseVirtAddr     = pvVirt;
        psInfo->u32BaseHardIpAddr   = 0;
        {
            OMX_U64 u64Top    = (OMX_U64)pvVirt;
            OMX_U64 u64Offset = ((((u64Top + u32ReqAlign) - 1u) / u32ReqAlign) * u32ReqAlign) - u64Top;
            if( u64Offset > 0x00000000FFFFFFFFu ){ /* exceed 32-bit, which is the maximum of u32BaseHardIpAddr */
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "Runtime error","");
                return OMX_ErrorUndefined;
            }
            psInfo->u32AddrOffset = (OMX_U32)u64Offset;
        }
        psInfo->u32ValidSize        = u32ReqSize;
        psInfo->u32AllocSize        = u32AllocReqSize;
    } else {
        /* Allocation via MMngr */
        if ((u32Flags & OMXR_WORKBUFFER_TA_SUPPORT) == OMXR_WORKBUFFER_TA_SUPPORT) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_DEBUG, "MMNGR_PA_SUPPORT.", "");
            flags = MMNGR_PA_SUPPORT;
        } else {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_DEBUG, "MMNGR_VA_SUPPORT.", "");
            flags = MMNGR_VA_SUPPORT;
        }
       
#ifndef OMXR_WORKBUFFER_FUNC_DISABLE_USING_MMNGR_LIB
        if (!OmxrStrcmp("OMXBUF.VIDEO.WORKBUFFER.DEC.OUTPUT.LOSSY.YUV420PLANAR", strBufferDiscri)) {
            OMXR_STATIC struct MM_FUNC sMmFunc;
            OMXR_STATIC unsigned int conf = 0;
            sMmFunc.func = MM_FUNC_LOSSY_ENABLE;
            sMmFunc.type = MM_FUNC_TYPE_LOSSY_AREA;
            sMmFunc.attr = MM_FUNC_FMT_LOSSY_YUVPLANAR;
            sMmFunc.conf = &conf;

            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_INFO, "MM_FUNC .func=0x%x .type=0x%x .attr=0x%x .conf=%p ", sMmFunc.func, sMmFunc.type, sMmFunc.attr, sMmFunc.conf );
            flags     = MMNGR_PA_SUPPORT_LOSSY;
            mem_param = &sMmFunc;

            u32Flags = OMXR_WORKBUFFER_HA_SUPPORT|OMXR_WORKBUFFER_TA_SUPPORT;
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_INFO, "u32Flags=0x%x", u32Flags );
        }
        else{
            mem_param = NULL;
        }
#else
        mem_param = NULL;
#endif
        

        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_INFO, "mmngr_alloc_in_user_ext size=%d flag=0x%x mem_param=%p", u32AllocReqSize, flags, mem_param);
        ret = mmngr_alloc_in_user_ext(&pid, u32AllocReqSize, &u32hard, &pvVirt, flags, mem_param);
        if (ret != R_MM_OK) {
            if (ret == R_MM_NOMEM) {
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: mmngr no mem.", "");
                return OMX_ErrorInsufficientResources;
            } else {
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: mmngr fatal. %d", ret);
                return OMX_ErrorUndefined;
            }
        } else {
            psInfo->hMmHandle = pid;
            if ((u32Flags & OMXR_WORKBUFFER_VA_SUPPORT) == OMXR_WORKBUFFER_VA_SUPPORT) {
                psInfo->pvBaseVirtAddr = pvVirt;
            } else {
                psInfo->pvBaseVirtAddr = NULL;
            }
            if ((u32Flags & OMXR_WORKBUFFER_HA_SUPPORT) == OMXR_WORKBUFFER_HA_SUPPORT) {
                psInfo->u32BaseHardIpAddr = u32hard;
            } else {
                psInfo->u32BaseHardIpAddr = 0;
            }
            psInfo->u32AddrOffset = ((((u32hard + u32ReqAlign) - 1u) / u32ReqAlign) * u32ReqAlign) - u32hard;
            psInfo->u32ValidSize  = u32ReqSize;
            psInfo->u32AllocSize  = u32AllocReqSize;

            /* map the PA address area to virtual address space. This area is to be accessed by CPU. */
            if ((u32Flags & OMXR_WORKBUFFER_TA_SUPPORT) == OMXR_WORKBUFFER_TA_SUPPORT) {
                MMNGR_ID pid2;
                OMX_S32 ret2;
                OMX_PTR pvVirt2;

                ret2 = mmngr_share_in_user_ext(&pid2, psInfo->u32AllocSize, psInfo->u32BaseHardIpAddr, &pvVirt2, NULL);
                OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_INFO, "[mmngr_share_in_user_ext] ret=%d pid=%ld, size=%ld, hard_addr=0x%x, puser_virt_addr=0x%x", ret2, pid2, psInfo->u32AllocSize, psInfo->u32BaseHardIpAddr, pvVirt2);
                if( ret2 != R_MM_OK ){
                  return OMX_ErrorUndefined;
                }

                psInfo->hMapSpaceHandle = pid2;
                psInfo->pvBaseVirtAddr   = pvVirt2;
            } else {
                psInfo->hMapSpaceHandle   = 0;
            }
        }
    }
    return OMX_ErrorNone;
}

OMXR_STATIC OMX_ERRORTYPE OmxrAllocateWorkBuffer_freefunc(OMXR_WORKBUFFER_HANDLE_INFO *psInfo)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_S32 ret;


    /* unmap from virtual address space. */
    if( psInfo->hMapSpaceHandle != 0 ){
        OMX_S32 ret2;
        ret2 = mmngr_release_in_user_ext((MMNGR_ID)psInfo->hMapSpaceHandle);
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_INFO, "[mmngr_release_in_user_ext] ret=%d pid=%ld", ret2, psInfo->hMapSpaceHandle);
        psInfo->hMapSpaceHandle = 0;
    }

    /* Free memory block */
    if (psInfo->hMmHandle != 0) {
        ret = mmngr_free_in_user_ext((MMNGR_ID)psInfo->hMmHandle);
        if (ret != R_MM_OK) {
            OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: mmngr fatal.", "");
            eError = OMX_ErrorUndefined;
        }
        psInfo->hMmHandle = 0;
    } else {
        free((OMX_PTR)psInfo->pvBaseVirtAddr);
        psInfo->pvBaseVirtAddr = NULL;
    }
    
    return eError;
}

OMX_ERRORTYPE OmxrFreeWorkBuffer(OMXR_WORKBUFFER_HANDLE BufferHandle)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if (BufferHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Bad parameter.", "");
        eError = OMX_ErrorBadParameter;
    } else {
        eError = OmxrAllocateWorkBuffer_freefunc((OMXR_WORKBUFFER_HANDLE_INFO *)BufferHandle);
        OmxrFree((OMX_PTR)BufferHandle);
    }
    
    return eError;
}


OMX_PTR OmxrGetVirtualAddress(OMXR_WORKBUFFER_HANDLE BufferHandle)
{
    OMXR_WORKBUFFER_HANDLE_INFO *psInfo;
    OMX_PTR pvaddr = NULL;

    if (BufferHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Bad parameter.", "");
        pvaddr = NULL;
    } else {
        psInfo = (OMXR_WORKBUFFER_HANDLE_INFO *)BufferHandle;
        if (NULL != psInfo->pvBaseVirtAddr) {
            pvaddr = (OMX_PTR)((OMXR_UINTPTR)psInfo->pvBaseVirtAddr + psInfo->u32AddrOffset);
        }
    }
    return (OMX_PTR)pvaddr;
}


OMX_U32 OmxrGetHardwareIPAddress(OMXR_WORKBUFFER_HANDLE BufferHandle)
{
    OMXR_WORKBUFFER_HANDLE_INFO *psInfo;
    OMX_U32 u32addr = 0;

    if (BufferHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Bad parameter.", "");
        u32addr = 0;
    } else {
        psInfo = (OMXR_WORKBUFFER_HANDLE_INFO *)BufferHandle;
        if (psInfo->u32BaseHardIpAddr != 0u) {
            u32addr = psInfo->u32BaseHardIpAddr + psInfo->u32AddrOffset;
        }
    }
    return u32addr;
}


OMX_PTR OmxrGetMemoryManagerHandle(OMXR_WORKBUFFER_HANDLE BufferHandle)
{
    OMXR_WORKBUFFER_HANDLE_INFO *psInfo;
    OMX_PTR pvHandle;

    if (BufferHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Bad parameter.", "");
        pvHandle = 0;
    } else {
        psInfo = (OMXR_WORKBUFFER_HANDLE_INFO *)BufferHandle;
        pvHandle = (OMX_PTR)(OMX_U64)psInfo->hMmHandle;
    }
    return pvHandle;
}


OMX_U32 OmxrGetWorkBufferValidSizeBytes(OMXR_WORKBUFFER_HANDLE BufferHandle)
{
    OMXR_WORKBUFFER_HANDLE_INFO *psInfo;
    OMX_U32 u32ValidSizeBytes;

    if (BufferHandle == NULL) {
        OMXR_LOGGER(OMXR_UTIL_LOG_LEVEL_ERROR, "error: Bad parameter.", "");
        u32ValidSizeBytes = 0;
    } else {
        psInfo = (OMXR_WORKBUFFER_HANDLE_INFO *)BufferHandle;
        u32ValidSizeBytes = psInfo->u32ValidSize;
    }
    return u32ValidSizeBytes;
}

