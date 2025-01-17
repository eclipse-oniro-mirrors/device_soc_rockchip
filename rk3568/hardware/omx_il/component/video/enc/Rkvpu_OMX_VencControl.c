/*
 *
 * Copyright 2013-2023 Rockchip Electronics Co., LTD.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        Rkon2_OMX_VdecControl.c
 * @brief
 * @author      csy (csy@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2013.11.28 : Create
 */
#undef  ROCKCHIP_LOG_TAG
#define ROCKCHIP_LOG_TAG    "omx_venc_ctl"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include "Rockchip_OMX_Macros.h"
#include "Rockchip_OSAL_Event.h"
#include "Rockchip_OMX_Basecomponent.h"
#include "Rockchip_OSAL_Thread.h"
#include "Rockchip_OSAL_Semaphore.h"
#include "Rockchip_OSAL_Mutex.h"
#include "Rockchip_OSAL_ETC.h"
#include "Rockchip_OSAL_SharedMemory.h"
#include "Rockchip_OSAL_Queue.h"
#ifdef OHOS_BUFFER_HANDLE
#include <buffer_handle.h>
#include <display_type.h>
#include <codec_omx_ext.h>
#endif
#include "Rockchip_OSAL_OHOS.h"
#include "Rkvpu_OMX_Venc.h"
#include "Rkvpu_OMX_VencControl.h"

#ifdef USE_ANB

#endif

#include "Rockchip_OSAL_Log.h"
#include "IVCommonExt.h"
#include "IndexExt.h"

typedef struct {
    OMX_U32 mProfile;
    OMX_U32 mLevel;
} CodecProfileLevel;

static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5},
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5},
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51},
};

static const CodecProfileLevel kH265ProfileLevels[] = {
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL1  },
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL2  },
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL21 },
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL3  },
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL31 },
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL4  },
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL41 },
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL5  },
    { CODEC_HEVC_PROFILE_MAIN, CODEC_HEVC_MAIN_TIER_LEVEL51 },
};

OMX_ERRORTYPE Rkvpu_OMX_ComponentTunnelRequest(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_U32 nPort,
    OMX_IN OMX_HANDLETYPE hTunneledComp, OMX_IN OMX_U32 nTunneledPort,
    OMX_INOUT OMX_TUNNELSETUPTYPE *pTunnelSetup)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    omx_err_f("This is not implemented");
    ret = OMX_ErrorTunnelingUnsupported;
    goto EXIT;
EXIT:
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_FreeTunnelBuffer(ROCKCHIP_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    omx_err_f("This is not implemented");
    ret = OMX_ErrorTunnelingUnsupported;
    goto EXIT;
EXIT:
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_AllocateTunnelBuffer(ROCKCHIP_OMX_BASEPORT *pOMXBasePort, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                 ret = OMX_ErrorNone;
    omx_err_f("This is not implemented");
    ret = OMX_ErrorTunnelingUnsupported;
    goto EXIT;
EXIT:
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr, OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes, OMX_IN OMX_U8 *pBuffer)
{
    FunctionIn();
    OMX_U32 i = 0;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    ROCKCHIP_OMX_BASEPORT *pRockchipPort = NULL;
    
    if (hComponent == NULL) {
        omx_err_f("hComponent is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err_f("check version ret err");
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        omx_err_f("pComponentPrivate is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pRockchipPort = &pRockchipComponent->pRockchipPort[nPortIndex];
    if (nPortIndex >= pRockchipComponent->portParam.nPorts) {
        omx_err_f("nPortIndex is %d", nPortIndex);
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    if (pRockchipPort->portState != OMX_StateIdle) {
        omx_err_f("portState is %d", pRockchipPort->portState);
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if (CHECK_PORT_TUNNELED(pRockchipPort) && CHECK_PORT_BUFFER_SUPPLIER(pRockchipPort)) {
        omx_err_f("port status is incorrect");
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)Rockchip_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        omx_err_f("temp_bufferHeader is null");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Rockchip_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pRockchipPort->portDefinition.nBufferCountActual; i++) {
        if (pRockchipPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pRockchipPort->extendBufferHeader[i].OMXBufferHeader = temp_bufferHeader;
            pRockchipPort->bufferStateAllocate[i] = (BUFFER_STATE_ASSIGNED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            temp_bufferHeader->pBuffer        = pBuffer;
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX) {
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            } else {
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;
            }
            pRockchipPort->assignedBufferNum++;
            if (pRockchipPort->assignedBufferNum == pRockchipPort->portDefinition.nBufferCountActual) {
                pRockchipPort->portDefinition.bPopulated = OMX_TRUE;
                Rockchip_OSAL_SemaphorePost(pRockchipPort->loadedResource);
            }
            *ppBufferHdr = temp_bufferHeader;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }

    Rockchip_OSAL_Free(temp_bufferHeader);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer, OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate, OMX_IN OMX_U32 nSizeBytes)
{
    FunctionIn();
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    ROCKCHIP_OMX_BASEPORT      *pRockchipPort = NULL;
    OMX_BUFFERHEADERTYPE  *temp_bufferHeader = NULL;
    OMX_U8                *temp_buffer = NULL;
    int                    temp_buffer_fd = -1;
    OMX_U32                i = 0;
    MEMORY_TYPE            mem_type = NORMAL_MEMORY;
    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        omx_err_f("hComponent is null");
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err_f("check version ret err");
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        omx_err_f("pComponentPrivate is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;

    pRockchipPort = &pRockchipComponent->pRockchipPort[nPortIndex];
    if (nPortIndex >= pRockchipComponent->portParam.nPorts) {
        omx_err_f("nPortIndex is %d", nPortIndex);
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }
    if (CHECK_PORT_TUNNELED(pRockchipPort) && CHECK_PORT_BUFFER_SUPPLIER(pRockchipPort)) {
        omx_err_f("port status is incorrect");
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_buffer = (OMX_U8 *)Rockchip_OSAL_Malloc(nSizeBytes);
    if (temp_buffer == NULL) {
        omx_err_f("temp_buffer is null");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)Rockchip_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        omx_err_f("temp_bufferHeader is null");
        Rockchip_OSAL_Free(temp_buffer);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Rockchip_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pRockchipPort->portDefinition.nBufferCountActual; i++) {
        if (pRockchipPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pRockchipPort->extendBufferHeader[i].OMXBufferHeader = temp_bufferHeader;
            pRockchipPort->extendBufferHeader[i].buf_fd[0] = temp_buffer_fd;
            pRockchipPort->bufferStateAllocate[i] = (BUFFER_STATE_ALLOCATED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            if (mem_type != SECURE_MEMORY) {
                temp_bufferHeader->pBuffer = temp_buffer;
            }
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX) {
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            } else {
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;
            }
            pRockchipPort->assignedBufferNum++;
            if (pRockchipPort->assignedBufferNum == pRockchipPort->portDefinition.nBufferCountActual) {
                pRockchipPort->portDefinition.bPopulated = OMX_TRUE;
                Rockchip_OSAL_SemaphorePost(pRockchipPort->loadedResource);
            }
            *ppBuffer = temp_bufferHeader;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }
    Rockchip_OSAL_Free(temp_bufferHeader);
    Rockchip_OSAL_Free(temp_buffer);
    ret = OMX_ErrorInsufficientResources;
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_FreeBuffer(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_U32 nPortIndex, OMX_IN OMX_BUFFERHEADERTYPE *pBufferHdr)
{
    FunctionIn();
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    ROCKCHIP_OMX_BASEPORT      *pRockchipPort = NULL;
    OMX_U32                i = 0;
    if (hComponent == NULL) {
        omx_err_f("hComponent is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err_f("check version ret err");
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        omx_err_f("pComponentPrivate is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
    pRockchipPort = &pRockchipComponent->pRockchipPort[nPortIndex];

    if (CHECK_PORT_TUNNELED(pRockchipPort) && CHECK_PORT_BUFFER_SUPPLIER(pRockchipPort)) {
        omx_err_f("port status is incorrect");
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((pRockchipPort->portState != OMX_StateLoaded) && (pRockchipPort->portState != OMX_StateInvalid)) {
        (*(pRockchipComponent->pCallbacks->EventHandler)) (pOMXComponent, pRockchipComponent->callbackData,
                                                           (OMX_U32)OMX_EventError, (OMX_U32)OMX_ErrorPortUnpopulated,
                                                           nPortIndex, NULL);
    }
    for (i = 0; i < MAX_BUFFER_NUM; i++) {
        if (((pRockchipPort->bufferStateAllocate[i] | BUFFER_STATE_FREE) != 0) &&
            (pRockchipPort->extendBufferHeader[i].OMXBufferHeader != NULL)) {
            if (pRockchipPort->extendBufferHeader[i].OMXBufferHeader->pBuffer == pBufferHdr->pBuffer) {
                if (pRockchipPort->bufferStateAllocate[i] & BUFFER_STATE_ALLOCATED) {
                    Rockchip_OSAL_Free(pRockchipPort->extendBufferHeader[i].OMXBufferHeader->pBuffer);
                    pRockchipPort->extendBufferHeader[i].OMXBufferHeader->pBuffer = NULL;
                    pBufferHdr->pBuffer = NULL;
                    omx_trace("pBufferHdr->pBuffer == pRockchipPort->extendBufferHeader[i].OMXBufferHeader->pBuffer");
                } else if (pRockchipPort->bufferStateAllocate[i] & BUFFER_STATE_ASSIGNED) {
                    omx_trace("donothing");
                }
                pRockchipPort->assignedBufferNum--;
                if (pRockchipPort->bufferStateAllocate[i] & HEADER_STATE_ALLOCATED) {
                    Rockchip_OSAL_Free(pRockchipPort->extendBufferHeader[i].OMXBufferHeader);
                    pRockchipPort->extendBufferHeader[i].OMXBufferHeader = NULL;
                    pBufferHdr = NULL;
                }
                pRockchipPort->bufferStateAllocate[i] = BUFFER_STATE_FREE;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }
    omx_trace("pRockchipPort->assignedBufferNum = %d", pRockchipPort->assignedBufferNum);
EXIT:
    if (ret == OMX_ErrorNone) {
        if (pRockchipPort->assignedBufferNum == 0) {
            omx_trace("pRockchipPort->unloadedResource signal set");
            Rockchip_OSAL_SemaphorePost(pRockchipPort->unloadedResource);
            pRockchipPort->portDefinition.bPopulated = OMX_FALSE;
        }
    }
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_GetFlushBuffer(ROCKCHIP_OMX_BASEPORT *pRockchipPort, ROCKCHIP_OMX_DATABUFFER *pDataBuffer[])
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    FunctionIn();
    *pDataBuffer = NULL;
    if (pRockchipPort->portWayType == WAY1_PORT) {
        omx_trace("portWayType == WAY1_PORT");
        *pDataBuffer = &pRockchipPort->way.port1WayDataBuffer.dataBuffer;
    } else if (pRockchipPort->portWayType == WAY2_PORT) {
        omx_trace("portWayType == WAY2_PORT");
        pDataBuffer[0] = &(pRockchipPort->way.port2WayDataBuffer.inputDataBuffer);
        pDataBuffer[1] = &(pRockchipPort->way.port2WayDataBuffer.outputDataBuffer);
    }
    goto EXIT;
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_FlushPort(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 portIndex)
{
    FunctionIn();
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    ROCKCHIP_OMX_BASEPORT      *pRockchipPort = NULL;
    OMX_BUFFERHEADERTYPE     *bufferHeader = NULL;
    ROCKCHIP_OMX_DATABUFFER    *pDataPortBuffer[2] = {NULL, NULL};
    ROCKCHIP_OMX_MESSAGE       *message = NULL;
    OMX_S32                semValue = 0;
    int i = 0, maxBufferNum = 0;
    pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
    while (Rockchip_OSAL_GetElemNum(&pRockchipPort->bufferQ) > 0) {
        Rockchip_OSAL_Get_SemaphoreCount(pRockchipComponent->pRockchipPort[portIndex].bufferSemID, &semValue);
        if (semValue == 0) {
            Rockchip_OSAL_SemaphorePost(pRockchipComponent->pRockchipPort[portIndex].bufferSemID);
        }
        Rockchip_OSAL_SemaphoreWait(pRockchipComponent->pRockchipPort[portIndex].bufferSemID);
        message = (ROCKCHIP_OMX_MESSAGE *)Rockchip_OSAL_Dequeue(&pRockchipPort->bufferQ);
        if ((message != NULL) && (message->messageType != ROCKCHIP_OMX_CommandFakeBuffer)) {
            bufferHeader = (OMX_BUFFERHEADERTYPE *)message->pCmdData;
            bufferHeader->nFilledLen = 0;
            if (portIndex == OUTPUT_PORT_INDEX) {
                Rockchip_OMX_OutputBufferReturn(pOMXComponent, bufferHeader);
            } else if (portIndex == INPUT_PORT_INDEX) {
                Rkvpu_OMX_InputBufferReturn(pOMXComponent, bufferHeader);
            }
        }
        Rockchip_OSAL_Free(message);
        message = NULL;
    }

    Rkvpu_OMX_GetFlushBuffer(pRockchipPort, pDataPortBuffer);
    if (portIndex == INPUT_PORT_INDEX) {
        if (pDataPortBuffer[0]->dataValid == OMX_TRUE)
            Rkvpu_InputBufferReturn(pOMXComponent, pDataPortBuffer[0]);
        if (pDataPortBuffer[1]->dataValid == OMX_TRUE)
            Rkvpu_InputBufferReturn(pOMXComponent, pDataPortBuffer[1]);
    } else if (portIndex == OUTPUT_PORT_INDEX) {
        if (pDataPortBuffer[0]->dataValid == OMX_TRUE)
            Rkvpu_OutputBufferReturn(pOMXComponent, pDataPortBuffer[0]);
        if (pDataPortBuffer[1]->dataValid == OMX_TRUE)
            Rkvpu_OutputBufferReturn(pOMXComponent, pDataPortBuffer[1]);
    }

    if (pRockchipComponent->bMultiThreadProcess == OMX_TRUE) {
        if (pRockchipPort->bufferProcessType == BUFFER_SHARE) {
            omx_trace("pRockchipPort->bufferProcessType is BUFFER_SHARE");
            if (pRockchipPort->processData.bufferHeader != NULL) {
                if (portIndex == INPUT_PORT_INDEX) {
                    Rkvpu_OMX_InputBufferReturn(pOMXComponent, pRockchipPort->processData.bufferHeader);
                } else if (portIndex == OUTPUT_PORT_INDEX) {
                    Rockchip_OMX_OutputBufferReturn(pOMXComponent, pRockchipPort->processData.bufferHeader);
                }
            }
            Rockchip_ResetCodecData(&pRockchipPort->processData);
            maxBufferNum = pRockchipPort->portDefinition.nBufferCountActual;
            omx_trace("maxBufferNum is %d", maxBufferNum);
            for (i = 0; i < maxBufferNum; i++) {
                if (pRockchipPort->extendBufferHeader[i].bBufferInOMX == OMX_TRUE) {
                    if (portIndex == OUTPUT_PORT_INDEX) {
                        Rockchip_OMX_OutputBufferReturn(pOMXComponent,
                            pRockchipPort->extendBufferHeader[i].OMXBufferHeader);
                    } else if (portIndex == INPUT_PORT_INDEX) {
                        Rkvpu_OMX_InputBufferReturn(pOMXComponent,
                            pRockchipPort->extendBufferHeader[i].OMXBufferHeader);
                    }
                }
            }
        }
    } else {
        Rockchip_ResetCodecData(&pRockchipPort->processData);
    }

    if ((pRockchipPort->bufferProcessType == BUFFER_SHARE) &&
        (portIndex == OUTPUT_PORT_INDEX)) {
        RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;

        if (pOMXComponent->pComponentPrivate == NULL) {
            omx_err_f("pComponentPrivate is null");
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
        pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
        pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
    }

    while (1) {
        OMX_S32 cnt = 0;
        Rockchip_OSAL_Get_SemaphoreCount(pRockchipComponent->pRockchipPort[portIndex].bufferSemID, &cnt);
        if (cnt <= 0) {
            omx_trace("cnt <= 0, don't wait");
            break;
        }
        Rockchip_OSAL_SemaphoreWait(pRockchipComponent->pRockchipPort[portIndex].bufferSemID);
    }
    Rockchip_OSAL_ResetQueue(&pRockchipPort->bufferQ);
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_BufferFlush(OMX_COMPONENTTYPE *pOMXComponent, OMX_S32 nPortIndex, OMX_BOOL bEvent)
{
    FunctionIn();
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    ROCKCHIP_OMX_BASEPORT      *pRockchipPort = NULL;
    ROCKCHIP_OMX_DATABUFFER    *flushPortBuffer[2] = {NULL, NULL};
    if (pOMXComponent == NULL) {
        omx_err_f("pOMXComponent is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err_f("version check err");
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        omx_err_f("pComponentPrivate is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;

    omx_trace("OMX_CommandFlush start, port:%ld", nPortIndex);

    pRockchipComponent->pRockchipPort[nPortIndex].bIsPortFlushed = OMX_TRUE;

    if (pRockchipComponent->bMultiThreadProcess == OMX_FALSE) {
        Rockchip_OSAL_SignalSet(pRockchipComponent->pauseEvent);
    } else {
        Rockchip_OSAL_SignalSet(pRockchipComponent->pRockchipPort[nPortIndex].pauseEvent);
    }

    pRockchipPort = &pRockchipComponent->pRockchipPort[nPortIndex];
    Rkvpu_OMX_GetFlushBuffer(pRockchipPort, flushPortBuffer);

    Rockchip_OSAL_SemaphorePost(pRockchipPort->bufferSemID);

    Rockchip_OSAL_MutexLock(flushPortBuffer[0]->bufferMutex);
    Rockchip_OSAL_MutexLock(flushPortBuffer[1]->bufferMutex);

    ret = Rkvpu_OMX_FlushPort(pOMXComponent, nPortIndex);
    VpuCodecContext_t *p_vpu_ctx = pVideoEnc->vpu_ctx;
    if (pRockchipComponent->nRkFlags & RK_VPU_NEED_FLUSH_ON_SEEK) {
        p_vpu_ctx->flush(p_vpu_ctx);
        pRockchipComponent->nRkFlags &= ~RK_VPU_NEED_FLUSH_ON_SEEK;
    }
    Rockchip_ResetCodecData(&pRockchipPort->processData);

    if (ret == OMX_ErrorNone) {
        if (nPortIndex == INPUT_PORT_INDEX) {
            pRockchipComponent->checkTimeStamp.needSetStartTimeStamp = OMX_TRUE;
            pRockchipComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            Rockchip_OSAL_Memset(pRockchipComponent->timeStamp, -19771003, // -19771003:byte alignment
                sizeof(OMX_TICKS) * MAX_TIMESTAMP);
            Rockchip_OSAL_Memset(pRockchipComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
            pRockchipComponent->getAllDelayBuffer = OMX_FALSE;
            pRockchipComponent->bSaveFlagEOS = OMX_FALSE;
            pRockchipComponent->bBehaviorEOS = OMX_FALSE;
            pRockchipComponent->reInputData = OMX_FALSE;
            if (pVideoEnc) {
                pVideoEnc->bEncSendEos = OMX_FALSE;
            }
        }

        pRockchipComponent->pRockchipPort[nPortIndex].bIsPortFlushed = OMX_FALSE;
        omx_trace("OMX_CommandFlush EventCmdComplete, port:%ld", nPortIndex);
        if (bEvent == OMX_TRUE)
            pRockchipComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                         pRockchipComponent->callbackData,
                                                         OMX_EventCmdComplete,
                                                         OMX_CommandFlush, nPortIndex, NULL);
    }
    Rockchip_OSAL_MutexUnlock(flushPortBuffer[1]->bufferMutex);
    Rockchip_OSAL_MutexUnlock(flushPortBuffer[0]->bufferMutex);

EXIT:
    if ((ret != OMX_ErrorNone) && (pOMXComponent != NULL) && (pRockchipComponent != NULL)) {
        omx_err("ERROR");
        pRockchipComponent->pCallbacks->EventHandler(pOMXComponent,
                                                     pRockchipComponent->callbackData,
                                                     OMX_EventError,
                                                     ret, 0, NULL);
    }

    FunctionOut();
    return ret;
}

void Rkvpu_RectSet(OMX_CONFIG_RECTTYPE* dstRect, OMX_CONFIG_RECTTYPE* src)
{
    if (dstRect == NULL || src == NULL) {
        omx_err_f("dstRect or sr is null");
        return;
    }
    
    dstRect->nTop     = src->nTop;
    dstRect->nLeft    = src->nLeft;
    dstRect->nWidth   = src->nWidth;
    dstRect->nHeight  = src->nHeight;
}

void Rkvpu_PortFormatSet(OMX_PARAM_PORTDEFINITIONTYPE* dst, OMX_PARAM_PORTDEFINITIONTYPE* src)
{
    if (dst == NULL || src == NULL) {
        omx_err_f("dstRect or sr is null");
        return;
    }
    dst->format.video.nFrameWidth     = src->format.video.nFrameWidth;
    dst->format.video.nFrameHeight    = src->format.video.nFrameHeight;
    dst->format.video.nStride         = src->format.video.nStride;
    dst->format.video.nSliceHeight    = src->format.video.nSliceHeight;
}
OMX_ERRORTYPE Rkvpu_ResolutionUpdate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    ROCKCHIP_OMX_BASEPORT           *pInputPort         = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    ROCKCHIP_OMX_BASEPORT           *pOutputPort        = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    // reset output crop rect
    Rkvpu_RectSet(&pOutputPort->cropRectangle, &pOutputPort->newCropRectangle);

    // reset input port video
    Rkvpu_PortFormatSet(&pInputPort->portDefinition, &pInputPort->newPortDefinition);

    pOutputPort->portDefinition.nBufferCountActual  = pOutputPort->newPortDefinition.nBufferCountActual;
    pOutputPort->portDefinition.nBufferCountMin     = pOutputPort->newPortDefinition.nBufferCountMin;

    UpdateFrameSize(pOMXComponent);
    return ret;
}

OMX_ERRORTYPE Rkvpu_OutputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent, ROCKCHIP_OMX_DATABUFFER *dataBuffer)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    ROCKCHIP_OMX_BASEPORT      *rockchipOMXOutputPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    OMX_BUFFERHEADERTYPE     *bufferHeader = NULL;
    FunctionIn();
    bufferHeader = dataBuffer->bufferHeader;
    if (bufferHeader != NULL) {
        bufferHeader->nFilledLen = dataBuffer->remainDataLen;
        bufferHeader->nOffset    = 0;
        bufferHeader->nFlags     = dataBuffer->nFlags;
        bufferHeader->nTimeStamp = dataBuffer->timeStamp;

        if ((rockchipOMXOutputPort->bStoreMetaData == OMX_TRUE) && (bufferHeader->nFilledLen > 0))
            bufferHeader->nFilledLen = bufferHeader->nAllocLen;

        if (pRockchipComponent->propagateMarkType.hMarkTargetComponent != NULL) {
            bufferHeader->hMarkTargetComponent = pRockchipComponent->propagateMarkType.hMarkTargetComponent;
            bufferHeader->pMarkData = pRockchipComponent->propagateMarkType.pMarkData;
            pRockchipComponent->propagateMarkType.hMarkTargetComponent = NULL;
            pRockchipComponent->propagateMarkType.pMarkData = NULL;
        }

        if ((bufferHeader->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            omx_info("event OMX_BUFFERFLAG_EOS!!!");
            pRockchipComponent->pCallbacks->EventHandler(pOMXComponent,
                                                         pRockchipComponent->callbackData,
                                                         OMX_EventBufferFlag,
                                                         OUTPUT_PORT_INDEX,
                                                         bufferHeader->nFlags, NULL);
        }

        Rockchip_OMX_OutputBufferReturn(pOMXComponent, bufferHeader);
    }

    /* reset dataBuffer */
    Rockchip_ResetDataBuffer(dataBuffer);

    goto EXIT;
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_InputBufferReturn(OMX_COMPONENTTYPE *pOMXComponent, ROCKCHIP_OMX_DATABUFFER *dataBuffer)
{
    FunctionIn();
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    ROCKCHIP_OMX_BASEPORT      *rockchipOMXInputPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    OMX_BUFFERHEADERTYPE     *bufferHeader = NULL;
    bufferHeader = dataBuffer->bufferHeader;
    if (bufferHeader != NULL) {
        omx_trace_f("bufferHeader is not null");
        if (rockchipOMXInputPort->markType.hMarkTargetComponent != NULL) {
            omx_trace_f("hMarkTargetComponent is not null");
            bufferHeader->hMarkTargetComponent      = rockchipOMXInputPort->markType.hMarkTargetComponent;
            bufferHeader->pMarkData                 = rockchipOMXInputPort->markType.pMarkData;
            rockchipOMXInputPort->markType.hMarkTargetComponent = NULL;
            rockchipOMXInputPort->markType.pMarkData = NULL;
        }
        if (bufferHeader->hMarkTargetComponent != NULL) {
            omx_trace_f("bufferHeader->hMarkTargetComponent is not null");
            if (bufferHeader->hMarkTargetComponent == pOMXComponent) {
                omx_trace("hMarkTargetComponent == pOMXComponent, send OMX_EventMark");
                pRockchipComponent->pCallbacks->EventHandler(pOMXComponent, pRockchipComponent->callbackData,
                                                             OMX_EventMark, 0, 0, bufferHeader->pMarkData);
            } else {
                pRockchipComponent->propagateMarkType.hMarkTargetComponent = bufferHeader->hMarkTargetComponent;
                pRockchipComponent->propagateMarkType.pMarkData = bufferHeader->pMarkData;
            }
        }
        bufferHeader->nFilledLen = 0;
        bufferHeader->nOffset = 0;
        omx_trace("input buffer return");
        Rkvpu_OMX_InputBufferReturn(pOMXComponent, bufferHeader);
    }
    /* reset dataBuffer */
    Rockchip_ResetDataBuffer(dataBuffer);
    goto EXIT;
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_OutputBufferGetQueue(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{
    OMX_U32       ret = OMX_ErrorUndefined;
    ROCKCHIP_OMX_BASEPORT   *pRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    ROCKCHIP_OMX_MESSAGE    *message = NULL;
    ROCKCHIP_OMX_DATABUFFER *outputUseBuffer = NULL;
    FunctionIn();
    outputUseBuffer = &(pRockchipPort->way.port2WayDataBuffer.outputDataBuffer);

    if (pRockchipComponent->currentState != OMX_StateExecuting) {
        omx_err_f("currentState is %d", pRockchipComponent->currentState);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else if ((pRockchipComponent->transientState != ROCKCHIP_OMX_TransStateExecutingToIdle) &&
               (!CHECK_PORT_BEING_FLUSHED(pRockchipPort))) {
        Rockchip_OSAL_SemaphoreWait(pRockchipPort->bufferSemID);
        if (outputUseBuffer->dataValid != OMX_TRUE) {
            message = (ROCKCHIP_OMX_MESSAGE *)Rockchip_OSAL_Dequeue(&pRockchipPort->bufferQ);
            if (message == NULL) {
                omx_err_f("message is null");
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }
            if (message->messageType == ROCKCHIP_OMX_CommandFakeBuffer) {
                Rockchip_OSAL_Free(message);
                ret = OMX_ErrorCodecFlush;
                omx_err_f("messageType == ROCKCHIP_OMX_CommandFakeBuffer");
                goto EXIT;
            }
            outputUseBuffer->dataLen       = 0;
            outputUseBuffer->usedDataLen   = 0;
            outputUseBuffer->dataValid     = OMX_TRUE;
            outputUseBuffer->bufferHeader  = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            outputUseBuffer->allocSize     = outputUseBuffer->bufferHeader->nAllocLen;
            outputUseBuffer->remainDataLen = outputUseBuffer->dataLen;
            Rockchip_OSAL_Free(message);
        }
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_InputBufferGetQueue(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{
    OMX_U32 ret = OMX_ErrorUndefined;
    ROCKCHIP_OMX_BASEPORT   *pRockchipPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    ROCKCHIP_OMX_MESSAGE    *message = NULL;
    ROCKCHIP_OMX_DATABUFFER *inputUseBuffer = NULL;

    FunctionIn();
    inputUseBuffer = &(pRockchipPort->way.port2WayDataBuffer.inputDataBuffer);
    if (pRockchipComponent->currentState != OMX_StateExecuting) {
        omx_err_f("currentState is %d", pRockchipComponent->currentState);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    } else if ((pRockchipComponent->transientState != ROCKCHIP_OMX_TransStateExecutingToIdle) &&
               (!CHECK_PORT_BEING_FLUSHED(pRockchipPort))) {
        Rockchip_OSAL_SemaphoreWait(pRockchipPort->bufferSemID);
        if (inputUseBuffer->dataValid != OMX_TRUE) {
            message = (ROCKCHIP_OMX_MESSAGE *)Rockchip_OSAL_Dequeue(&pRockchipPort->bufferQ);
            if (message == NULL) {
                omx_err_f("messageType is null");
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }
            if (message->messageType == ROCKCHIP_OMX_CommandFakeBuffer) {
                Rockchip_OSAL_Free(message);
                ret = OMX_ErrorCodecFlush;
                omx_err_f("messageType is ROCKCHIP_OMX_CommandFakeBuffer");
                goto EXIT;
            }
            inputUseBuffer->usedDataLen   = 0;
            inputUseBuffer->dataValid     = OMX_TRUE;
            inputUseBuffer->bufferHeader  = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
            OMX_BUFFERHEADERTYPE* bufferHead = inputUseBuffer->bufferHeader;
            if (bufferHead != NULL) {
                inputUseBuffer->allocSize     = bufferHead->nAllocLen;
                inputUseBuffer->dataLen       = bufferHead->nFilledLen;
                inputUseBuffer->nFlags        = bufferHead->nFlags;
                inputUseBuffer->timeStamp     = bufferHead->nTimeStamp;
            }
            inputUseBuffer->remainDataLen = inputUseBuffer->dataLen;
            Rockchip_OSAL_Free(message);
            if (inputUseBuffer->allocSize <= inputUseBuffer->dataLen)
                omx_trace("Input Buffer Full, Check input buffer size! allocSize:%lu, dataLen:%lu",
                    inputUseBuffer->allocSize, inputUseBuffer->dataLen);
        }
        ret = OMX_ErrorNone;
    }
EXIT:
    FunctionOut();
    return ret;
}

OMX_BUFFERHEADERTYPE *Rkvpu_OutputBufferGetQueue_Direct(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{
    OMX_BUFFERHEADERTYPE  *retBuffer = NULL;
    ROCKCHIP_OMX_BASEPORT   *pRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    ROCKCHIP_OMX_MESSAGE    *message = NULL;

    FunctionIn();

    if (pRockchipComponent->currentState != OMX_StateExecuting) {
        omx_err_f("currentState is %d", pRockchipComponent->currentState);
        retBuffer = NULL;
        goto EXIT;
    } else if ((pRockchipComponent->transientState != ROCKCHIP_OMX_TransStateExecutingToIdle) &&
               (!CHECK_PORT_BEING_FLUSHED(pRockchipPort))) {
        Rockchip_OSAL_SemaphoreWait(pRockchipPort->bufferSemID);

        message = (ROCKCHIP_OMX_MESSAGE *)Rockchip_OSAL_Dequeue(&pRockchipPort->bufferQ);
        if (message == NULL) {
            retBuffer = NULL;
            goto EXIT;
        }
        if (message->messageType == ROCKCHIP_OMX_CommandFakeBuffer) {
            omx_err_f("messageType == ROCKCHIP_OMX_CommandFakeBuffer");
            Rockchip_OSAL_Free(message);
            retBuffer = NULL;
            goto EXIT;
        }

        retBuffer  = (OMX_BUFFERHEADERTYPE *)(message->pCmdData);
        Rockchip_OSAL_Free(message);
    }

EXIT:
    FunctionOut();

    return retBuffer;
}

OMX_ERRORTYPE Rkvpu_CodecBufferReset(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent, OMX_U32 PortIndex)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    ROCKCHIP_OMX_BASEPORT   *pRockchipPort = NULL;
    FunctionIn();
    pRockchipPort = &pRockchipComponent->pRockchipPort[PortIndex];
    ret = Rockchip_OSAL_ResetQueue(&pRockchipPort->codecBufferQ);
    if (ret != 0) {
        omx_err_f("Rockchip_OSAL_ResetQueue ret err");
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    while (1) {
        OMX_S32 cnt = 0;
        Rockchip_OSAL_Get_SemaphoreCount(pRockchipPort->codecSemID, &cnt);
        if (cnt > 0) {
            omx_info_f("cnt > 0, need wait");
            Rockchip_OSAL_SemaphoreWait(pRockchipPort->codecSemID);
        } else {
            break;
        }
    }
    ret = OMX_ErrorNone;
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_GetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex, OMX_INOUT OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    FunctionIn();

    if (hComponent == NULL) {
        omx_err_f("hComponent is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err_f("check size err OMX_COMPONENTTYPE ");
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        omx_err_f("pComponentPrivate is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pRockchipComponent->currentState == OMX_StateInvalid) {
        omx_err_f("currentState == OMX_StateInvalid");
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        omx_err_f("ComponentParameterStructure is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch ((OMX_U32)nParamIndex) {
        case OMX_IndexParamVideoInit: {
            OMX_PORT_PARAM_TYPE *portParam = (OMX_PORT_PARAM_TYPE *)ComponentParameterStructure;
            ret = Rockchip_OMX_Check_SizeVersion(portParam, sizeof(OMX_PORT_PARAM_TYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }
            portParam->nPorts           = pRockchipComponent->portParam.nPorts;
            portParam->nStartPortNumber = pRockchipComponent->portParam.nStartPortNumber;
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamVideoPortFormat: {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
            OMX_U32 portIndex = portFormat->nPortIndex, index = portFormat->nIndex, supportFormatNum = 0;
            ROCKCHIP_OMX_BASEPORT            *pRockchipPort = NULL;
            OMX_PARAM_PORTDEFINITIONTYPE     *portDefinition = NULL;
            ret = Rockchip_OMX_Check_SizeVersion(portFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }
            if ((portIndex >= pRockchipComponent->portParam.nPorts)) {
                ret = OMX_ErrorBadPortIndex;
                omx_err_f("portIndex is %d", portIndex);
                goto EXIT;
            }
            if (portIndex == INPUT_PORT_INDEX) {
                pRockchipPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
                portDefinition = &pRockchipPort->portDefinition;
                switch (index) {
                    case supportFormat_0: {
                        portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                        portFormat->eColorFormat       = OMX_COLOR_FormatYUV420SemiPlanar;
                        portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                        break;
                    }
                    case supportFormat_1: {
                        portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                        portFormat->eColorFormat       = (OMX_COLOR_FORMATTYPE)CODEC_COLOR_FORMAT_RGBA8888;
                        portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                        break;
                    }
                    case supportFormat_2: {
                        portFormat->eCompressionFormat = OMX_VIDEO_CodingUnused;
                        portFormat->eColorFormat       = OMX_COLOR_Format32bitBGRA8888;
                        portFormat->xFramerate         = portDefinition->format.video.xFramerate;
                        break;
                    }
                    default: {
                        omx_err_f("index is large than supportFormat_0");
                        ret = OMX_ErrorNoMore;
                        goto EXIT;
                    }
                }
            } else if (portIndex == OUTPUT_PORT_INDEX) {
                supportFormatNum = OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX - 1;
                if (index > supportFormatNum) {
                    omx_err_f("index is too large");
                    ret = OMX_ErrorNoMore;
                    goto EXIT;
                }

                pRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
                portDefinition = &pRockchipPort->portDefinition;

                portFormat->eCompressionFormat = portDefinition->format.video.eCompressionFormat;
                portFormat->eColorFormat       = portDefinition->format.video.eColorFormat;
                portFormat->xFramerate         = portDefinition->format.video.xFramerate;
            }
            ret = OMX_ErrorNone;
        }
        break;
#ifdef OHOS_BUFFER_HANDLE
    case OMX_IndexParamSupportBufferType: {
        struct SupportBufferType *bufferTyps = (struct SupportBufferType *)ComponentParameterStructure;
        if (bufferTyps == NULL) {
            omx_err_f("bufferTyps is null");
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
        OMX_U32                       portIndex = bufferTyps->portIndex;
        if (portIndex >= pRockchipComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Rockchip_OMX_Check_SizeVersion(bufferTyps, sizeof(struct SupportBufferType));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
        bufferTyps->bufferTypes = CODEC_BUFFER_TYPE_AVSHARE_MEM_FD;
        if (portIndex == INPUT_PORT_INDEX) {
            bufferTyps->bufferTypes  |= CODEC_BUFFER_TYPE_DYNAMIC_HANDLE;
        }
        break;
    }
    case OMX_IndexParamGetBufferHandleUsage: {
        struct GetBufferHandleUsageParams *usage = (struct GetBufferHandleUsageParams *)ComponentParameterStructure;
        if (usage == NULL) {
            omx_err_f("usage is null");
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
        OMX_U32                       portIndex = usage->portIndex;

        if (portIndex >= pRockchipComponent->portParam.nPorts || portIndex == OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        
        ret = Rockchip_OMX_Check_SizeVersion(usage, sizeof(struct GetBufferHandleUsageParams));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
        usage->usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA;
        break;
    }
    case OMX_IndexCodecVideoPortFormat: {
            struct CodecVideoPortFormatParam *videoFormat =
                (struct CodecVideoPortFormatParam *)ComponentParameterStructure;
            if (videoFormat == NULL) {
                omx_err_f("videoFormat is null");
                ret = OMX_ErrorBadParameter;
                goto EXIT;
            }
            OMX_U32 portIndex = videoFormat->portIndex;
            if (portIndex >= pRockchipComponent->portParam.nPorts) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }

            ret = Rockchip_OMX_Check_SizeVersion(videoFormat, sizeof(struct CodecVideoPortFormatParam));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
            ROCKCHIP_OMX_BASEPORT *pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
            OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = &pRockchipPort->portDefinition;
        uint32_t index = videoFormat->codecColorIndex;
            if (portIndex == INPUT_PORT_INDEX) {
                switch (index) {
                    case supportFormat_0: {
                        videoFormat->codecColorFormat =
                            Rockchip_OSAL_OmxColorFormat2CodecFormat(OMX_COLOR_FormatYUV420SemiPlanar);
                        break;
                    }
                    case supportFormat_1: {
                        videoFormat->codecColorFormat =
                            Rockchip_OSAL_OmxColorFormat2CodecFormat((OMX_COLOR_FORMATTYPE)CODEC_COLOR_FORMAT_RGBA8888);
                        break;
                    }
                    case supportFormat_2: {
                        videoFormat->codecColorFormat =
                            Rockchip_OSAL_OmxColorFormat2CodecFormat(OMX_COLOR_Format32bitBGRA8888);
                        break;
                    }
                    default: {
                        omx_err_f("index is large than supportFormat_0");
                        ret = OMX_ErrorNoMore;
                        goto EXIT;
                    }
                }
                videoFormat->codecCompressFormat = OMX_VIDEO_CodingUnused;
                videoFormat->framerate = portDefinition->format.video.xFramerate;
            } else {
                if (index > supportFormat_0) {
                    omx_err_f("index is too large");
                    ret = OMX_ErrorNoMore;
                    goto EXIT;
                }
                videoFormat->codecColorFormat =
                    Rockchip_OSAL_OmxColorFormat2CodecFormat(portDefinition->format.video.eColorFormat);
                videoFormat->framerate = portDefinition->format.video.xFramerate;
                videoFormat->codecCompressFormat = portDefinition->format.video.eCompressionFormat;
            }
            ret = OMX_ErrorNone;
        } break;
#endif // OHOS_BUFFER_HANDLE
    case OMX_IndexParamVideoBitrate: {
        OMX_VIDEO_PARAM_BITRATETYPE     *videoRateControl = (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
        OMX_U32                          portIndex = videoRateControl->nPortIndex;
        ROCKCHIP_OMX_BASEPORT             *pRockchipPort = NULL;
        RKVPU_OMX_VIDEOENC_COMPONENT   *pVideoEnc = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE    *portDefinition = NULL;

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                portDefinition = &pRockchipPort->portDefinition;
                videoRateControl->eControlRate = pVideoEnc->eControlRate[portIndex];
                videoRateControl->nTargetBitrate = portDefinition->format.video.nBitrate;
            }
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamVideoQuantization: {
            OMX_VIDEO_PARAM_QUANTIZATIONTYPE  *videoQuantizationControl =
                (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)ComponentParameterStructure;
            OMX_U32                            portIndex = videoQuantizationControl->nPortIndex;
            ROCKCHIP_OMX_BASEPORT               *pRockchipPort = NULL;
            RKVPU_OMX_VIDEOENC_COMPONENT     *pVideoEnc = NULL;
            OMX_PARAM_PORTDEFINITIONTYPE      *portDefinition = NULL;

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                omx_err_f("portIndex is %d", portIndex);
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
                if (pVideoEnc != NULL) {
                    videoQuantizationControl->nQpI = pVideoEnc->quantization.nQpI;
                    videoQuantizationControl->nQpP = pVideoEnc->quantization.nQpP;
                    videoQuantizationControl->nQpB = pVideoEnc->quantization.nQpB;
                }
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                portDefinition = &pRockchipPort->portDefinition;
            }
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamPortDefinition: {
            OMX_PARAM_PORTDEFINITIONTYPE *portDefinition = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
            OMX_U32                       portIndex = portDefinition->nPortIndex;
            ROCKCHIP_OMX_BASEPORT          *pRockchipPort;
            if (portIndex >= pRockchipComponent->portParam.nPorts) {
                omx_err_f("portIndex is %d", portIndex);
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            ret = Rockchip_OMX_Check_SizeVersion(portDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("check version ret err");
                goto EXIT;
            }

            pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
            Rockchip_OSAL_Memcpy(portDefinition, &pRockchipPort->portDefinition, portDefinition->nSize);
            break;
        }
        case OMX_IndexParamVideoIntraRefresh: {
            OMX_VIDEO_PARAM_INTRAREFRESHTYPE *pIntraRefresh =
                (OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)ComponentParameterStructure;
            OMX_U32                           portIndex = pIntraRefresh->nPortIndex;
            RKVPU_OMX_VIDEOENC_COMPONENT    *pVideoEnc = NULL;

            if (portIndex != OUTPUT_PORT_INDEX) {
                omx_err_f("portIndex is %d", portIndex);
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
                pIntraRefresh->eRefreshMode = pVideoEnc->intraRefresh.eRefreshMode;
                pIntraRefresh->nAirMBs = pVideoEnc->intraRefresh.nAirMBs;
                pIntraRefresh->nAirRef = pVideoEnc->intraRefresh.nAirRef;
                pIntraRefresh->nCirMBs = pVideoEnc->intraRefresh.nCirMBs;
            }
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamStandardComponentRole: {
            OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParameterStructure;
            ret = Rockchip_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
            if (ret != OMX_ErrorNone) {
                omx_err_f("check version ret err");
                goto EXIT;
            }
            if (pVideoEnc->codecId == OMX_VIDEO_CodingAVC) {
                Rockchip_OSAL_Strcpy((char *)pComponentRole->cRole, RK_OMX_COMPONENT_H264_ENC_ROLE);
            } else if (pVideoEnc->codecId == OMX_VIDEO_CodingVP8EXT) {
                Rockchip_OSAL_Strcpy((char *)pComponentRole->cRole, RK_OMX_COMPONENT_VP8_ENC_ROLE);
            } else if (pVideoEnc->codecId == CODEC_OMX_VIDEO_CodingHEVC) {
                Rockchip_OSAL_Strcpy((char *)pComponentRole->cRole, RK_OMX_COMPONENT_HEVC_ENC_ROLE);
            }
        }
        break;
        case OMX_IndexParamVideoAvc: {
            OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)ComponentParameterStructure;
            OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;

            ret = Rockchip_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }

            if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }

            pSrcAVCComponent = &pVideoEnc->AVCComponent[pDstAVCComponent->nPortIndex];
            Rockchip_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        }
        break;
        case OMX_IndexParamVideoHevc: {
            struct CodecVideoParamHevc *pDstHEVCComponent = (struct CodecVideoParamHevc *)ComponentParameterStructure;
            struct CodecVideoParamHevc *pSrcHEVCComponent = NULL;
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;

            ret = Rockchip_OMX_Check_SizeVersion(pDstHEVCComponent, sizeof(struct CodecVideoParamHevc));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }

            if (pDstHEVCComponent->portIndex >= ALL_PORT_NUM) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }

            pSrcHEVCComponent = &pVideoEnc->HEVCComponent[pDstHEVCComponent->portIndex];
            Rockchip_OSAL_Memcpy(pDstHEVCComponent, pSrcHEVCComponent, sizeof(struct CodecVideoParamHevc));
        }
        break;
        case OMX_IndexParamVideoProfileLevelQuerySupported: {
            OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
                (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) ComponentParameterStructure;
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;

            OMX_U32 index = profileLevel->nProfileIndex;
            OMX_U32 nProfileLevels = 0;
            if (profileLevel->nPortIndex  >= ALL_PORT_NUM) {
                omx_err("Invalid port index: %ld", profileLevel->nPortIndex);
                ret = OMX_ErrorUnsupportedIndex;
                goto EXIT;
            }
            if (pVideoEnc->codecId == OMX_VIDEO_CodingAVC) {
                nProfileLevels = ARRAY_SIZE(kProfileLevels);
                if (index >= nProfileLevels) {
                    ret = OMX_ErrorNoMore;
                    goto EXIT;
                }
                profileLevel->eProfile = kProfileLevels[index].mProfile;
                profileLevel->eLevel = kProfileLevels[index].mLevel;
            } else if (pVideoEnc->codecId == CODEC_OMX_VIDEO_CodingHEVC) {
                nProfileLevels = ARRAY_SIZE(kH265ProfileLevels);
                if (index >= nProfileLevels) {
                    ret = OMX_ErrorNoMore;
                    goto EXIT;
                }
                profileLevel->eProfile = kH265ProfileLevels[index].mProfile;
                profileLevel->eLevel = kH265ProfileLevels[index].mLevel;
            } else {
                ret = OMX_ErrorNoMore;
                goto EXIT;
            }
            ret = OMX_ErrorNone;
        }
        break;
        case OMX_IndexParamRkEncExtendedVideo: {
            OMX_VIDEO_PARAMS_EXTENDED  *params_extend = (OMX_VIDEO_PARAMS_EXTENDED *)ComponentParameterStructure;
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
            omx_trace("get OMX_IndexParamRkEncExtendedVideo in ");
            Rockchip_OSAL_MutexLock(pVideoEnc->bScale_Mutex);
            Rockchip_OSAL_Memcpy(params_extend, &pVideoEnc->params_extend, sizeof(OMX_VIDEO_PARAMS_EXTENDED));
            Rockchip_OSAL_MutexUnlock(pVideoEnc->bScale_Mutex);
        }
        break;
        default: {
            ret = Rockchip_OMX_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
            break;
        }
    }
EXIT:
    FunctionOut();
    return ret;
}
OMX_ERRORTYPE Rkvpu_OMX_SetParameter(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex, OMX_IN OMX_PTR ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    FunctionIn();
    if (hComponent == NULL) {
        omx_err_f("hComponent is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err_f("check version ret err");
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        omx_err_f("pComponentPrivate is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pRockchipComponent->currentState == OMX_StateInvalid) {
        omx_err_f("currentState == OMX_StateInvalid");
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        omx_err_f("ComponentParameterStructure is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch ((OMX_U32)nIndex) {
        case OMX_IndexParamVideoPortFormat: {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *portFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)ComponentParameterStructure;
            OMX_U32                         portIndex = portFormat->nPortIndex;
            ROCKCHIP_OMX_BASEPORT            *pRockchipPort = NULL;
            OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;

            ret = Rockchip_OMX_Check_SizeVersion(portFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("check version ret err");
                goto EXIT;
            }

            if ((portIndex >= pRockchipComponent->portParam.nPorts)) {
                omx_err_f("portIndex is %d", portIndex);
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                portDefinition = &pRockchipPort->portDefinition;
                portDefinition->format.video.eColorFormat       = portFormat->eColorFormat;
                portDefinition->format.video.eCompressionFormat = portFormat->eCompressionFormat;
                portDefinition->format.video.xFramerate         = portFormat->xFramerate;
            }
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamVideoBitrate: {
            OMX_VIDEO_PARAM_BITRATETYPE     *videoRateControl =
                (OMX_VIDEO_PARAM_BITRATETYPE *)ComponentParameterStructure;
            OMX_U32                          portIndex = videoRateControl->nPortIndex;
            ROCKCHIP_OMX_BASEPORT             *pRockchipPort = NULL;
            RKVPU_OMX_VIDEOENC_COMPONENT   *pVideoEnc = NULL;
            OMX_PARAM_PORTDEFINITIONTYPE    *portDefinition = NULL;
            if ((portIndex != OUTPUT_PORT_INDEX)) {
                omx_err_f("portIndex is %d", portIndex);
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                portDefinition = &pRockchipPort->portDefinition;
                pVideoEnc->eControlRate[portIndex] = videoRateControl->eControlRate;
                portDefinition->format.video.nBitrate = videoRateControl->nTargetBitrate;
            }
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamVideoQuantization: {
            OMX_VIDEO_PARAM_QUANTIZATIONTYPE *videoQuantizationControl =
                (OMX_VIDEO_PARAM_QUANTIZATIONTYPE *)ComponentParameterStructure;
            OMX_U32                           portIndex = videoQuantizationControl->nPortIndex;
            ROCKCHIP_OMX_BASEPORT              *pRockchipPort = NULL;
            RKVPU_OMX_VIDEOENC_COMPONENT    *pVideoEnc = NULL;
            OMX_PARAM_PORTDEFINITIONTYPE     *portDefinition = NULL;

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                omx_err_f("portIndex is %d", portIndex);
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
                if (pVideoEnc != NULL) {
                    pVideoEnc->quantization.nQpI = videoQuantizationControl->nQpI;
                    pVideoEnc->quantization.nQpP = videoQuantizationControl->nQpP;
                    pVideoEnc->quantization.nQpB = videoQuantizationControl->nQpB;
                }
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                portDefinition = &pRockchipPort->portDefinition;
            }
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamPortDefinition: {
            OMX_PARAM_PORTDEFINITIONTYPE *pPortDefinition =
                (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
            OMX_U32 portIndex = pPortDefinition->nPortIndex;
            ROCKCHIP_OMX_BASEPORT          *pRockchipPort;
            if (portIndex >= pRockchipComponent->portParam.nPorts) {
                omx_err_f("portIndex is %d", portIndex);
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }
            ret = Rockchip_OMX_Check_SizeVersion(pPortDefinition, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("check version ret err");
                goto EXIT;
            }

            pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];

            if ((pRockchipComponent->currentState != OMX_StateLoaded) &&
                (pRockchipComponent->currentState != OMX_StateWaitForResources)) {
                if (pRockchipPort->portDefinition.bEnabled == OMX_TRUE) {
                    omx_err_f("portDefinition.bEnabled is OMX_TRUE");
                    ret = OMX_ErrorIncorrectStateOperation;
                    goto EXIT;
                }
            }
            if (pPortDefinition->nBufferCountActual < pRockchipPort->portDefinition.nBufferCountMin) {
                omx_err_f("nBufferCountActual < nBufferCountMin");
                ret = OMX_ErrorBadParameter;
                goto EXIT;
            }

            Rockchip_OSAL_Memcpy(&pRockchipPort->portDefinition, pPortDefinition, pPortDefinition->nSize);
            if (portIndex == INPUT_PORT_INDEX) {
                ROCKCHIP_OMX_BASEPORT *pRockchipOutputPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
                UpdateFrameSize(pOMXComponent);
                omx_trace("pRockchipOutputPort->portDefinition.nBufferSize: %lu",
                        pRockchipOutputPort->portDefinition.nBufferSize);
            }
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamVideoIntraRefresh: {
            OMX_VIDEO_PARAM_INTRAREFRESHTYPE *pIntraRefresh =
                (OMX_VIDEO_PARAM_INTRAREFRESHTYPE *)ComponentParameterStructure;
            OMX_U32                           portIndex = pIntraRefresh->nPortIndex;
            RKVPU_OMX_VIDEOENC_COMPONENT    *pVideoEnc = NULL;

            if (portIndex != OUTPUT_PORT_INDEX) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
                if (pIntraRefresh->eRefreshMode == OMX_VIDEO_IntraRefreshCyclic) {
                    pVideoEnc->intraRefresh.eRefreshMode = pIntraRefresh->eRefreshMode;
                    pVideoEnc->intraRefresh.nCirMBs = pIntraRefresh->nCirMBs;
                    omx_trace("OMX_VIDEO_IntraRefreshCyclic Enable, nCirMBs: %lu",
                            pVideoEnc->intraRefresh.nCirMBs);
                } else {
                    ret = OMX_ErrorUnsupportedSetting;
                    goto EXIT;
                }
            }
            ret = OMX_ErrorNone;
            break;
        }
#ifdef OHOS_BUFFER_HANDLE
    case OMX_IndexParamUseBufferType: {
        omx_info("OMX_IndexParamUseBufferType");
        struct UseBufferType *bufferType = (struct UseBufferType *)ComponentParameterStructure;
        if (bufferType == NULL) {
            omx_err_f("bufferType is null");
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
        OMX_U32  portIndex = bufferType->portIndex;
        if (portIndex >= pRockchipComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        ret = Rockchip_OMX_Check_SizeVersion(bufferType, sizeof(struct UseBufferType));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }
        if (((bufferType->bufferType & CODEC_BUFFER_TYPE_DYNAMIC_HANDLE) == CODEC_BUFFER_TYPE_DYNAMIC_HANDLE) &&
            portIndex == INPUT_PORT_INDEX) {
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
            pVideoEnc->bOhosDynamicBuffer = OMX_TRUE;
        }
        break;
    }
    case OMX_IndexCodecVideoPortFormat: {
        struct CodecVideoPortFormatParam *videoFormat =
                (struct CodecVideoPortFormatParam *)ComponentParameterStructure;
        if (videoFormat == NULL) {
            omx_err_f("videoFormat is null");
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
        OMX_U32                       portIndex = videoFormat->portIndex;
        ROCKCHIP_OMX_BASEPORT            *pRockchipPort = NULL;
        OMX_PARAM_PORTDEFINITIONTYPE   *portDefinition = NULL;

        ret = Rockchip_OMX_Check_SizeVersion(videoFormat, sizeof(struct CodecVideoPortFormatParam));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((portIndex >= pRockchipComponent->portParam.nPorts)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
            portDefinition = &pRockchipPort->portDefinition;
                portDefinition->format.video.eColorFormat =
                    Rochip_OSAL_CodecFormat2OmxColorFormat(videoFormat->codecColorFormat);
            portDefinition->format.video.eCompressionFormat = videoFormat->codecCompressFormat;
            portDefinition->format.video.xFramerate         = videoFormat->framerate;
        }
        ret = OMX_ErrorNone;
        break;
    }
#endif
#ifdef USE_STOREMETADATA
        case OMX_IndexParamStoreANWBuffer:
        case OMX_IndexParamStoreMetaDataBuffer: {
            ret = Rockchip_OSAL_SetANBParameter(hComponent, nIndex, ComponentParameterStructure);
        }
        break;
#endif
        case OMX_IndexParamPrependSPSPPSToIDR: {
            RKVPU_OMX_VIDEOENC_COMPONENT    *pVideoEnc = NULL;
            pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
            ret = Rockchip_OSAL_SetPrependSPSPPSToIDR(ComponentParameterStructure, &pVideoEnc->bPrependSpsPpsToIdr);
            break;
        }
        case OMX_IndexRkEncExtendedWfdState: {
            RKVPU_OMX_VIDEOENC_COMPONENT    *pVideoEnc = NULL;
            pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
            ROCKCHIP_OMX_WFD *pRkWFD = (ROCKCHIP_OMX_WFD*)ComponentParameterStructure;
            pVideoEnc->bRkWFD = pRkWFD->bEnable;
            omx_trace("OMX_IndexRkEncExtendedWfdState set as:%d", pRkWFD->bEnable);
            ret = OMX_ErrorNone;
            break;
        }
        case OMX_IndexParamStandardComponentRole: {
            OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;

            ret = Rockchip_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }

            if ((pRockchipComponent->currentState != OMX_StateLoaded) && (pRockchipComponent->currentState !=
                OMX_StateWaitForResources)) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }

            if (!Rockchip_OSAL_Strcmp((char*)pComponentRole->cRole, RK_OMX_COMPONENT_H264_ENC_ROLE)) {
                pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat =
                    OMX_VIDEO_CodingAVC;
            } else if (!Rockchip_OSAL_Strcmp((char*)pComponentRole->cRole, RK_OMX_COMPONENT_VP8_ENC_ROLE)) {
                pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat =
                    (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP8EXT;
            } else if (!Rockchip_OSAL_Strcmp((char*)pComponentRole->cRole, RK_OMX_COMPONENT_HEVC_ENC_ROLE)) {
                pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat =
                    (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC;
            } else {
                ret = OMX_ErrorInvalidComponentName;
                goto EXIT;
            }
            break;
        }
        case OMX_IndexParamVideoAvc: {
            OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
            OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)ComponentParameterStructure;
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
            ret = Rockchip_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
            if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }

            pDstAVCComponent = &pVideoEnc->AVCComponent[pSrcAVCComponent->nPortIndex];

            Rockchip_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
            break;
        }
        case OMX_IndexParamVideoHevc: {
            struct CodecVideoParamHevc *pDstHEVCComponent = NULL;
            struct CodecVideoParamHevc *pSrcHEVCComponent = (struct CodecVideoParamHevc *)ComponentParameterStructure;
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;
            ret = Rockchip_OMX_Check_SizeVersion(pSrcHEVCComponent, sizeof(struct CodecVideoParamHevc));
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
            if (pSrcHEVCComponent->portIndex >= ALL_PORT_NUM) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            }

            pDstHEVCComponent = &pVideoEnc->HEVCComponent[pSrcHEVCComponent->portIndex];
            Rockchip_OSAL_Memcpy(pDstHEVCComponent, pSrcHEVCComponent, sizeof(struct CodecVideoParamHevc));
            break;
        }
        case OMX_IndexParamRkEncExtendedVideo: {
            OMX_VIDEO_PARAMS_EXTENDED  *params_extend = (OMX_VIDEO_PARAMS_EXTENDED *)ComponentParameterStructure;
            RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc =
                (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;

            omx_trace("OMX_IndexParamRkEncExtendedVideo in ");
            if (ret != OMX_ErrorNone) {
                goto EXIT;
            }
            Rockchip_OSAL_MutexLock(pVideoEnc->bScale_Mutex);
            Rockchip_OSAL_Memcpy(&pVideoEnc->params_extend, params_extend, sizeof(OMX_VIDEO_PARAMS_EXTENDED));
            omx_trace("OMX_IndexParamRkEncExtendedVideo in flags %lu bEableCrop %d,cl %d cr %d ct %d cb %d, \
                      bScaling %d ScaleW %d ScaleH %d",
                    pVideoEnc->params_extend.ui32Flags, pVideoEnc->params_extend.bEnableCropping,
                    pVideoEnc->params_extend.ui16CropLeft, pVideoEnc->params_extend.ui16CropRight,
                    pVideoEnc->params_extend.ui16CropTop, pVideoEnc->params_extend.ui16CropBottom,
                    pVideoEnc->params_extend.bEnableScaling,
                    pVideoEnc->params_extend.ui16ScaledWidth, pVideoEnc->params_extend.ui16ScaledHeight);
            Rockchip_OSAL_MutexUnlock(pVideoEnc->bScale_Mutex);
            break;
        }
        default: {
            ret = Rockchip_OMX_SetParameter(hComponent, nIndex, ComponentParameterStructure);
            break;
        }
    }
EXIT:
    FunctionOut();
    return ret;
}
static OMX_ERRORTYPE Rkvpu_OMX_GetExConfig(RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc, OMX_HANDLETYPE hComponent,
    OMX_INDEXEXEXTTYPE nIndex, OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    switch (nIndex) {
#ifdef AVS80
        case OMX_IndexParamRkDescribeColorAspects: {
            OMX_CONFIG_DESCRIBECOLORASPECTSPARAMS *pParam =
                (OMX_CONFIG_DESCRIBECOLORASPECTSPARAMS *)pComponentConfigStructure;
            if (pParam->bRequestingDataSpace) {
                pParam->sAspects.mPrimaries = PrimariesUnspecified;
                pParam->sAspects.mRange = RangeUnspecified;
                pParam->sAspects.mTransfer = TransferUnspecified;
                pParam->sAspects.mMatrixCoeffs = MatrixUnspecified;
                return ret; // OMX_ErrorUnsupportedSetting;
            }
            if (pParam->bDataSpaceChanged == OMX_TRUE) {
                // If the dataspace says RGB, recommend 601-limited;
                // since that is the destination colorspace that C2D or Venus will convert to.
                if (pParam->nPixelFormat == HAL_PIXEL_FORMAT_RGBA_8888) {
                    Rockchip_OSAL_Memcpy(pParam, &pVideoEnc->ConfigColorAspects,
                        sizeof(OMX_CONFIG_DESCRIBECOLORASPECTSPARAMS));
                    pParam->sAspects.mPrimaries = PrimariesUnspecified;
                    pParam->sAspects.mRange = RangeUnspecified;
                    pParam->sAspects.mTransfer = TransferUnspecified;
                    pParam->sAspects.mMatrixCoeffs = MatrixUnspecified;
                } else {
                    Rockchip_OSAL_Memcpy(pParam,
                        &pVideoEnc->ConfigColorAspects, sizeof(OMX_CONFIG_DESCRIBECOLORASPECTSPARAMS));
                }
            } else {
                Rockchip_OSAL_Memcpy(pParam,
                    &pVideoEnc->ConfigColorAspects, sizeof(OMX_CONFIG_DESCRIBECOLORASPECTSPARAMS));
            }
            break;
        }
#endif
        default:
            ret = Rockchip_OMX_GetConfig(hComponent, (OMX_INDEXTYPE)nIndex, pComponentConfigStructure);
            break;
    }
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    FunctionIn();
    if (hComponent == NULL) {
        omx_err_f("hComponent is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err_f("check version ret err");
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        omx_err_f("pComponentPrivate is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pComponentConfigStructure == NULL) {
        omx_err_f("pComponentConfigStructure is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pRockchipComponent->currentState == OMX_StateInvalid) {
        omx_err_f("currentState is OMX_StateInvalid");
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;

    switch (nIndex) {
        case OMX_IndexConfigVideoAVCIntraPeriod: {
            OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod =
                (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
            OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

            ret = Rockchip_OMX_Check_SizeVersion(pAVCIntraPeriod, sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pAVCIntraPeriod->nIDRPeriod = pVideoEnc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
                pAVCIntraPeriod->nPFrames = pVideoEnc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames;
            }
        }
            break;
        case OMX_IndexConfigVideoBitrate: {
            OMX_VIDEO_CONFIG_BITRATETYPE *pEncodeBitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)pComponentConfigStructure;
            OMX_U32                       portIndex = pEncodeBitrate->nPortIndex;
            ROCKCHIP_OMX_BASEPORT          *pRockchipPort = NULL;

            ret = Rockchip_OMX_Check_SizeVersion(pEncodeBitrate, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                pEncodeBitrate->nEncodeBitrate = pRockchipPort->portDefinition.format.video.nBitrate;
            }
        }
            break;
        case OMX_IndexConfigVideoFramerate: {
            OMX_CONFIG_FRAMERATETYPE *pFramerate = (OMX_CONFIG_FRAMERATETYPE *)pComponentConfigStructure;
            OMX_U32                   portIndex = pFramerate->nPortIndex;
            ROCKCHIP_OMX_BASEPORT      *pRockchipPort = NULL;

            ret = Rockchip_OMX_Check_SizeVersion(pFramerate, sizeof(OMX_CONFIG_FRAMERATETYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                pFramerate->xEncodeFramerate = pRockchipPort->portDefinition.format.video.xFramerate;
            }
            break;
        }
        default:
            ret = Rkvpu_OMX_GetExConfig(pVideoEnc, hComponent, (OMX_INDEXEXEXTTYPE)nIndex, pComponentConfigStructure);
            break;
    }
EXIT:
    FunctionOut();
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_SetConfig(OMX_HANDLETYPE hComponent, OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    FunctionIn();
    if (hComponent == NULL) {
        omx_err_f("hComponent is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err_f("check size ret err");
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        omx_err_f("pComponentPrivate is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pComponentConfigStructure == NULL) {
        omx_err_f("pComponentConfigStructure is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pRockchipComponent->currentState == OMX_StateInvalid) {
        omx_err_f("currentState is OMX_StateInvalid");
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    RKVPU_OMX_VIDEOENC_COMPONENT *pVideoEnc = (RKVPU_OMX_VIDEOENC_COMPONENT *)pRockchipComponent->hComponentHandle;

    switch ((OMX_U32)nIndex) {
        case OMX_IndexConfigVideoIntraPeriod: {
            OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

            pVideoEnc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = nPFrames;

            ret = OMX_ErrorNone;
        }
            break;
        case OMX_IndexConfigVideoAVCIntraPeriod: {
            OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod =
                (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
            OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

            ret = Rockchip_OMX_Check_SizeVersion(pAVCIntraPeriod, sizeof(OMX_VIDEO_CONFIG_AVCINTRAPERIOD));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                if (pAVCIntraPeriod->nIDRPeriod == (pAVCIntraPeriod->nPFrames + 1))
                    pVideoEnc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = pAVCIntraPeriod->nPFrames;
                else {
                    ret = OMX_ErrorBadParameter;
                    goto EXIT;
                }
            }
        }
            break;
        case OMX_IndexConfigVideoBitrate: {
            OMX_VIDEO_CONFIG_BITRATETYPE *pEncodeBitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)pComponentConfigStructure;
            OMX_U32                       portIndex = pEncodeBitrate->nPortIndex;
            ROCKCHIP_OMX_BASEPORT          *pRockchipPort = NULL;
            VpuCodecContext_t *p_vpu_ctx = pVideoEnc->vpu_ctx;

            ret = Rockchip_OMX_Check_SizeVersion(pEncodeBitrate, sizeof(OMX_VIDEO_CONFIG_BITRATETYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                pRockchipPort->portDefinition.format.video.nBitrate = pEncodeBitrate->nEncodeBitrate;
                if (p_vpu_ctx !=  NULL) {
                    EncParameter_t vpug;
                    p_vpu_ctx->control(p_vpu_ctx, VPU_API_ENC_GETCFG, (void*)&vpug);
                    vpug.bitRate = pEncodeBitrate->nEncodeBitrate;
                    omx_err("set bitRate %lu", pEncodeBitrate->nEncodeBitrate);
                    p_vpu_ctx->control(p_vpu_ctx, VPU_API_ENC_SETCFG, (void*)&vpug);
                }
            }
        }
            break;
        case OMX_IndexConfigVideoFramerate: {
            OMX_CONFIG_FRAMERATETYPE *pFramerate = (OMX_CONFIG_FRAMERATETYPE *)pComponentConfigStructure;
            OMX_U32                   portIndex = pFramerate->nPortIndex;
            ROCKCHIP_OMX_BASEPORT      *pRockchipPort = NULL;
            VpuCodecContext_t *p_vpu_ctx = pVideoEnc->vpu_ctx;

            ret = Rockchip_OMX_Check_SizeVersion(pFramerate, sizeof(OMX_CONFIG_FRAMERATETYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
                pRockchipPort->portDefinition.format.video.xFramerate = pFramerate->xEncodeFramerate;
            }

            if (p_vpu_ctx !=  NULL) {
                EncParameter_t vpug;
                p_vpu_ctx->control(p_vpu_ctx, VPU_API_ENC_GETCFG, (void*)&vpug);
                vpug.framerate = (pFramerate->xEncodeFramerate >> 16); // 16:byte alignment
                p_vpu_ctx->control(p_vpu_ctx, VPU_API_ENC_SETCFG, (void*)&vpug);
            }
        }
            break;
        case OMX_IndexConfigVideoIntraVOPRefresh: {
            OMX_CONFIG_INTRAREFRESHVOPTYPE *pIntraRefreshVOP =
                (OMX_CONFIG_INTRAREFRESHVOPTYPE *)pComponentConfigStructure;
            OMX_U32 portIndex = pIntraRefreshVOP->nPortIndex;

            VpuCodecContext_t *p_vpu_ctx = pVideoEnc->vpu_ctx;

            ret = Rockchip_OMX_Check_SizeVersion(pIntraRefreshVOP, sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE));
            if (ret != OMX_ErrorNone) {
                omx_err_f("size check err");
                goto EXIT;
            }

            if ((portIndex != OUTPUT_PORT_INDEX)) {
                ret = OMX_ErrorBadPortIndex;
                goto EXIT;
            } else {
                pVideoEnc->IntraRefreshVOP = pIntraRefreshVOP->IntraRefreshVOP;
            }

            if (p_vpu_ctx !=  NULL && pVideoEnc->IntraRefreshVOP) {
                p_vpu_ctx->control(p_vpu_ctx, VPU_API_ENC_SETIDRFRAME, NULL);
            }
        }
            break;
#ifdef AVS80
        case OMX_IndexParamRkDescribeColorAspects: {
            Rockchip_OSAL_Memcpy(&pVideoEnc->ConfigColorAspects,
                pComponentConfigStructure, sizeof(OMX_CONFIG_DESCRIBECOLORASPECTSPARAMS));
            pVideoEnc->bIsCfgColorAsp = OMX_TRUE;
        }
            break;
#endif
        default:
            ret = Rockchip_OMX_SetConfig(hComponent, nIndex, pComponentConfigStructure);
            break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_ComponentRoleEnum(
    OMX_HANDLETYPE hComponent,
    OMX_U8        *cRole,
    OMX_U32        nIndex)
{
    OMX_ERRORTYPE             ret               = OMX_ErrorNone;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (nIndex == 0) {
        Rockchip_OSAL_Strcpy((char *)cRole, RK_OMX_COMPONENT_H264_ENC_ROLE);
        ret = OMX_ErrorNone;
    } else if (nIndex == 1) {
        Rockchip_OSAL_Strcpy((char *)cRole, RK_OMX_COMPONENT_VP8_ENC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }
EXIT:
    FunctionOut();

    return ret;
}


OMX_ERRORTYPE Rkvpu_OMX_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;

    FunctionIn();
    omx_trace("cParameterName:%s", cParameterName);

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pRockchipComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (Rockchip_OSAL_Strcmp(cParameterName, ROCKCHIP_INDEX_CONFIG_VIDEO_INTRAPERIOD) == 0) {
        *pIndexType = (OMX_INDEXTYPE)OMX_IndexConfigVideoIntraPeriod;
        ret = OMX_ErrorNone;
        goto EXIT;
    } else if (Rockchip_OSAL_Strcmp(cParameterName, ROCKCHIP_INDEX_PARAM_PREPEND_SPSPPS_TO_IDR) == 0) {
        *pIndexType = (OMX_INDEXTYPE)OMX_IndexParamPrependSPSPPSToIDR;
        ret = OMX_ErrorNone;
        goto EXIT;
    } else if (!Rockchip_OSAL_Strcmp(cParameterName, ROCKCHIP_INDEX_PARAM_RKWFD)) {
        *pIndexType = (OMX_INDEXTYPE)OMX_IndexRkEncExtendedWfdState;
        ret = OMX_ErrorNone;
        goto EXIT;
    } else if (Rockchip_OSAL_Strcmp(cParameterName, ROCKCHIP_INDEX_PARAM_EXTENDED_VIDEO) == 0) {

        *pIndexType = (OMX_INDEXTYPE)OMX_IndexParamRkEncExtendedVideo;
        ret = OMX_ErrorNone;
    }
#ifdef AVS80
    else if (Rockchip_OSAL_Strcmp(cParameterName, ROCKCHIP_INDEX_PARAM_DSECRIBECOLORASPECTS) == 0) {
        *pIndexType = (OMX_INDEXTYPE)OMX_IndexParamRkDescribeColorAspects;
        goto EXIT;
    }
#endif
#ifdef USE_STOREMETADATA
    else if (Rockchip_OSAL_Strcmp(cParameterName, ROCKCHIP_INDEX_PARAM_STORE_ANW_BUFFER) == 0) {
        *pIndexType = (OMX_INDEXTYPE)OMX_IndexParamStoreANWBuffer;
        ret = OMX_ErrorNone;
    } else if (Rockchip_OSAL_Strcmp(cParameterName, ROCKCHIP_INDEX_PARAM_STORE_METADATA_BUFFER) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexParamStoreMetaDataBuffer;
        goto EXIT;
    } else {
        ret = Rockchip_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
    }
#else
    else {
        ret = Rockchip_OMX_GetExtensionIndex(hComponent, cParameterName, pIndexType);
    }
#endif

EXIT:
    FunctionOut();
    return ret;
}