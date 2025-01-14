/*
 * Copyright (C) 2021-2023 HiHope Open Source Organization .
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        Rockchip_OSAL_OHOS.h
 * @brief
 * @author      csy(csy@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2013.11.26 : Create
 */
#ifndef Rockchip_OSAL_OHOS
#define Rockchip_OSAL_OHOS

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Index.h"
#include "IndexExt.h"
#ifdef OHOS_BUFFER_HANDLE
#include <buffer_handle.h>
#endif
typedef unsigned int uint32_t;
typedef signed int int32_t;

typedef struct _ROCKCHIP_OMX_SHARED_BUFFER {
    OMX_S32 BufferFd;
    OMX_S32 BufferFd1;
    OMX_S32 BufferFd2;
    unsigned long *pIonHandle;
    unsigned long *pIonHandle1;
    unsigned long *pIonHandle2;
    OMX_U32 cnt;
} ROCKCHIP_OMX_SHARED_BUFFER;

typedef struct _ROCKCHIP_OMX_REF_HANDLE {
    OMX_HANDLETYPE hMutex;
    ROCKCHIP_OMX_SHARED_BUFFER SharedBuffer[MAX_BUFFER_REF];
} ROCKCHIP_OMX_REF_HANDLE;

enum _ROCKCHIP_OMX_DEPTH {
    OMX_DEPTH_BIT_8,
    OMX_DEPTH_BIT_10,
    // add more
};

#ifdef __cplusplus
extern "C" {
#endif

OMX_ERRORTYPE Rockchip_OSAL_GetInfoFromMetaData(OMX_IN OMX_BYTE pBuffer,
                                                OMX_OUT OMX_PTR *pOutBuffer);
OMX_ERRORTYPE Rockchip_OSAL_GetInfoRkWfdMetaData(OMX_IN OMX_BOOL bRkWFD,
                                                 OMX_IN OMX_BYTE pBuffer,
                                                 OMX_OUT OMX_PTR *ppBuf);

OMX_ERRORTYPE Rockchip_OSAL_CheckANB(OMX_IN ROCKCHIP_OMX_DATA *pBuffer,
                                     OMX_OUT OMX_BOOL *bIsANBEnabled);

OMX_ERRORTYPE Rockchip_OSAL_SetPrependSPSPPSToIDR(OMX_PTR pComponentParameterStructure,
                                                  OMX_PTR pbPrependSpsPpsToIdr);

OMX_ERRORTYPE Rockchip_OSAL_CheckBuffType(OMX_U32 type);

#ifdef OHOS_BUFFER_HANDLE
OMX_COLOR_FORMATTYPE Rockchip_OSAL_GetBufferHandleColorFormat(BufferHandle* bufferHandle);
OMX_U32 Rockchip_OSAL_OmxColorFormat2CodecFormat(OMX_COLOR_FORMATTYPE omxColorFormat);
OMX_COLOR_FORMATTYPE Rochip_OSAL_CodecFormat2OmxColorFormat(OMX_U32 codecFormat);
#endif

OMX_COLOR_FORMATTYPE Rockchip_OSAL_Hal2OMXPixelFormat(unsigned int hal_format);

unsigned int Rockchip_OSAL_OMX2HalPixelFormat(OMX_COLOR_FORMATTYPE omx_format);

OMX_ERRORTYPE Rockchip_OSAL_Fd2VpumemPool(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
                                          OMX_BUFFERHEADERTYPE* bufferHeader);

OMX_ERRORTYPE Rockchip_OSAL_CommitBuffer(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
                                         OMX_U32 index);

OMX_BUFFERHEADERTYPE *Rockchip_OSAL_Fd2OmxBufferHeader(ROCKCHIP_OMX_BASEPORT *pRockchipPort,
                                                       OMX_IN OMX_S32 fd, OMX_IN OMX_PTR pVpumem);
OMX_ERRORTYPE  Rockchip_OSAL_FreeVpumem(OMX_IN OMX_PTR pVpumem);

OMX_ERRORTYPE  Rockchip_OSAL_Openvpumempool(OMX_IN ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent, OMX_U32 portIndex);

OMX_ERRORTYPE  Rockchip_OSAL_Closevpumempool(OMX_IN ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent);

OMX_ERRORTYPE Rockchip_OSAL_resetVpumemPool(OMX_IN ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent);

OMX_BOOL Rockchip_OSAL_Check_Use_FBCMode(OMX_VIDEO_CODINGTYPE codecId, int32_t depth, ROCKCHIP_OMX_BASEPORT *pPort);

OMX_COLOR_FORMATTYPE Rockchip_OSAL_CheckFormat(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
                                               OMX_IN OMX_PTR pVpuframe);

OMX_U32 Get_Video_HorAlign(OMX_VIDEO_CODINGTYPE codecId, OMX_U32 width, OMX_U32 height, OMX_U32 codecprofile);
OMX_U32 Get_Video_VerAlign(OMX_VIDEO_CODINGTYPE codecId, OMX_U32 height, OMX_U32 codecprofile);
OMX_ERRORTYPE Rockchip_OSAL_PowerControl(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
                                         int32_t width,
                                         int32_t height,
                                         int32_t mHevc,
                                         int32_t frameRate,
                                         OMX_BOOL mFlag,
                                         int bitDepth);
OMX_U32 GetDataSize(OMX_U32 width, OMX_U32 height, OMX_COLOR_FORMATTYPE);
OMX_ERRORTYPE Rkvpu_ComputeDecBufferCount(OMX_HANDLETYPE hComponent);
OMX_U32 Rockchip_OSAL_CalculateTotalRefFrames(OMX_VIDEO_CODINGTYPE codecId, OMX_U32 width,
                                              OMX_U32 height, OMX_BOOL isSecure);

#ifdef AVS80
#endif

#ifdef __cplusplus
}
#endif

#endif