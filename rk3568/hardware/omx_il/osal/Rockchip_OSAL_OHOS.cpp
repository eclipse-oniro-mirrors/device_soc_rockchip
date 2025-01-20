/*
 * Copyright (C) 2021 HiHope Open Source Organization .
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
 * @file        Rockchip_OSAL_OHOS.cpp
 * @brief
 * @author      csy(csy@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2013.11.26 : Create
 */
#undef  ROCKCHIP_LOG_TAG
#define ROCKCHIP_LOG_TAG    "omx_osal_OHOS"

#include <stdio.h>
#include <stdlib.h>
#include <memory>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <codec_omx_ext.h>
#ifdef OHOS_BUFFER_HANDLE
#include <display_type.h>
#include <buffer_handle.h>
#endif
#include "Rockchip_OSAL_Mutex.h"
#include "Rockchip_OSAL_Semaphore.h"
#include "Rockchip_OMX_Baseport.h"
#include "Rockchip_OMX_Basecomponent.h"
#include "Rockchip_OMX_Macros.h"

#include "Rkvpu_OMX_Vdec.h"
#include "Rkvpu_OMX_Venc.h"
#include "Rockchip_OSAL_Log.h"
#include "Rockchip_OSAL_Env.h"
#include "omx_video_global.h"
#include "vpu_api.h"
#include "Rockchip_OSAL_OHOS.h"
#include "VideoExt.h"
#include "IVCommonExt.h"

enum {
    kFenceTimeoutMs = 1000
};

#ifdef __cplusplus
extern "C" {
#endif

OMX_U32 Get_Video_HorAlign(OMX_VIDEO_CODINGTYPE codecId, OMX_U32 width, OMX_U32 height, OMX_U32 codecProfile)
{
    OMX_U32 stride = 0;
    if (codecId == (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC) {
        if (codecProfile == CODEC_HEVC_PROFILE_MAIN10 || codecProfile == CODEC_HEVC_PROFILE_MAIN10_HDR10) {
            // 10:byte alignment, 8:byte alignment, 255:byte alignment, 256:byte alignment
            stride = (((width * 10 / 8 + 255) & ~(255)) | (256));
        } else
            // 255:byte alignment, 256:byte alignment
            stride = ((width + 255) & (~255)) | (256);
    } else if (codecId == (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP9) {
#ifdef AVS100
        // 255:byte alignment, 256:byte alignment
        stride = ((width + 255) & (~255)) | (256);
#else
        // 127:byte alignment
        stride = (width + 127) & (~127);
#endif
    } else {
        if (codecProfile == OMX_VIDEO_AVCProfileHigh10 && codecId == OMX_VIDEO_CodingAVC) {
            stride = ((width * 10 / 8 + 15) & (~15)); // 10:byte alignment, 8:byte alignment, 15:byte alignment
        } else
            stride = ((width + 15) & (~15)); // 15:byte alignment
    }
#ifdef AVS100
    if (access("/d/mpp_service/rkvdec/aclk", F_OK) == 0 ||
        access("/proc/mpp_service/rkvdec/aclk", F_OK) == 0) {
#else
    if (access("/dev/rkvdec", 06) == 0) { // 06:operation code
#endif
        if (width > 1920 || height > 1088) { // 1920:Resolution size, 1088:Resolution size
            if (codecId == OMX_VIDEO_CodingAVC) {
                if (codecProfile == OMX_VIDEO_AVCProfileHigh10) {
                    // 10:byte alignment, 8:byte alignment, 255:byte alignment, 256:byte alignment
                    stride = (((width * 10 / 8 + 255) & ~(255)) | (256));
                } else
                    // 255:byte alignment, 256:byte alignment
                    stride = ((width + 255) & (~255)) | (256);
            }
        }
    }

    return stride;
}

OMX_U32 Get_Video_VerAlign(OMX_VIDEO_CODINGTYPE codecId, OMX_U32 height, OMX_U32 codecProfile)
{
    OMX_U32 stride = 0;;
    if (codecId == (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC) {
        if (codecProfile == CODEC_HEVC_PROFILE_MAIN10 || codecProfile == CODEC_HEVC_PROFILE_MAIN10_HDR10)
            stride = (height + 15) & (~15); // 15:byte alignment
        else
            stride = (height + 7) & (~7); // 7:byte alignment
    } else if (codecId == (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP9) {
        stride = (height + 63) & (~63); // 63:byte alignment
    } else {
        stride = ((height + 15) & (~15)); // 15:byte alignment
    }
    return stride;
}

OMX_BOOL Rockchip_OSAL_Check_Use_FBCMode(OMX_VIDEO_CODINGTYPE codecId, int32_t depth,
                                         ROCKCHIP_OMX_BASEPORT *pPort)
{
    OMX_BOOL fbcMode = OMX_FALSE;
    OMX_U32 pValue;
    OMX_U32 width, height;

    Rockchip_OSAL_GetEnvU32("omx_fbc_disable", &pValue, 0);
    if (pValue == 1) {
        return OMX_FALSE;
    }

    if (pPort->bufferProcessType != BUFFER_SHARE) {
        return OMX_FALSE;
    }

    width = pPort->portDefinition.format.video.nFrameWidth;
    height = pPort->portDefinition.format.video.nFrameHeight;

#ifdef SUPPORT_AFBC
    // 10bit force set fbc mode
    if ((depth ==  OMX_DEPTH_BIT_10) || ((width * height > 1920 * 1088) // 1920:Resolution size, 1088:Resolution size
        && (codecId == OMX_VIDEO_CodingAVC
        || codecId == (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC
        || codecId == (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP9))) {
            fbcMode = OMX_TRUE;
    }
#else
    (void)codecId;
    (void)width;
    (void)height;
    (void)depth;
#endif

    return fbcMode;
}
OMX_ERRORTYPE Rockchip_OSAL_GetInfoFromMetaData(OMX_IN OMX_BYTE pBuffer,
    OMX_OUT OMX_PTR *ppBuf)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    MetadataBufferType type;

    FunctionIn();

    /*
     * meta data contains the following data format.
     * payload depends on the MetadataBufferType
     * --------------------------------------------------------------
     * | MetadataBufferType                         |          payload                           |
     * --------------------------------------------------------------
     *
     * If MetadataBufferType is kMetadataBufferTypeCameraSource, then
     * --------------------------------------------------------------
     * | kMetadataBufferTypeCameraSource  | physical addr. of Y |physical addr. of CbCr |
     * --------------------------------------------------------------
     *
     * If MetadataBufferType is kMetadataBufferTypeGrallocSource, then
     * --------------------------------------------------------------
     * | kMetadataBufferTypeGrallocSource    | buffer_handle_t |
     * --------------------------------------------------------------
     */
    /* MetadataBufferType */
    Rockchip_OSAL_Memcpy(&type, pBuffer, sizeof(MetadataBufferType));
#ifdef USE_ANW
    if (type > kMetadataBufferTypeNativeHandleSource) {
        omx_err("Data passed in with metadata mode does not have type "
                "kMetadataBufferTypeGrallocSource (%d), has type %ld instead",
                kMetadataBufferTypeGrallocSource, type);
        return OMX_ErrorBadParameter;
    }
#else
    if ((type != kMetadataBufferTypeGrallocSource) && (type != kMetadataBufferTypeCameraSource)) {
        omx_err("Data passed in with metadata mode does not have type "
                "kMetadataBufferTypeGrallocSource (%d), has type %ld instead",
                kMetadataBufferTypeGrallocSource, type);
        return OMX_ErrorBadParameter;
    }
#endif
    if (type == kMetadataBufferTypeCameraSource) {

        void *pAddress = nullptr;

        /* Address. of Y */
        Rockchip_OSAL_Memcpy(&pAddress, pBuffer + sizeof(MetadataBufferType), sizeof(void *));
        ppBuf[0] = (void *)pAddress;

        /* Address. of CbCr */
        Rockchip_OSAL_Memcpy(&pAddress, pBuffer + sizeof(MetadataBufferType) + sizeof(void *), sizeof(void *));
        ppBuf[1] = (void *)pAddress;

    } else if (type == kMetadataBufferTypeGrallocSource) {
    }
#ifdef USE_ANW
    else if (type == kMetadataBufferTypeANWBuffer) {
        ANativeWindowBuffer *buffer = nativeMeta.pBuffer;
        if (buffer != nullptr)
            ppBuf[0] = (OMX_PTR)buffer->handle;
        if (nativeMeta.nFenceFd >= 0) {
            sp<Fence> fence = new Fence(nativeMeta.nFenceFd);
            nativeMeta.nFenceFd = -1;
            status_t err = fence->wait(kFenceTimeoutMs);
            if (err != OK) {
                omx_err("Timed out waiting on input fence");
                return OMX_ErrorBadParameter;
            }
        }
    } else if (type == kMetadataBufferTypeNativeHandleSource) {
        omx_trace("kMetadataBufferTypeNativeHandleSource process in");
        VideoNativeHandleMetadata &nativehandleMeta = *(VideoNativeHandleMetadata*)pBuffer;
        ppBuf[0] = (OMX_PTR)nativehandleMeta.pHandle;
    }
#endif

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_SetPrependSPSPPSToIDR(
    OMX_PTR pComponentParameterStructure,
    OMX_PTR pbPrependSpsPpsToIdr)
{
    OMX_ERRORTYPE                    ret        = OMX_ErrorNone;
    PrependSPSPPSToIDRFramesParams  *pANBParams = (PrependSPSPPSToIDRFramesParams *)pComponentParameterStructure;

    ret = Rockchip_OMX_Check_SizeVersion(pANBParams, sizeof(PrependSPSPPSToIDRFramesParams));
    if (ret != OMX_ErrorNone) {
        omx_err("%s: Rockchip_OMX_Check_SizeVersion(PrependSPSPPSToIDRFrames) is failed", __func__);
        goto EXIT;
    }

    (*((OMX_BOOL *)pbPrependSpsPpsToIdr)) = pANBParams->bEnable;

EXIT:
    return ret;
}

#ifdef OHOS_BUFFER_HANDLE
OMX_U32 CodecOmxExtColor2CodecFormat(enum CodecColorFormatExt codecExtColor)
{
    OMX_U32 codecFormat = PIXEL_FMT_BUTT;
    switch (codecExtColor) {
    case CODEC_COLOR_FORMAT_RGBA8888:
        codecFormat = PIXEL_FMT_RGBA_8888;
        break;
    default:
        omx_err("%s: undefined omxColorFormat[%d]", __func__, codecExtColor);
        break;
    }
    return codecFormat;
}
OMX_U32 Rockchip_OSAL_OmxColorFormat2CodecFormat(OMX_COLOR_FORMATTYPE omxColorFormat)
{
    OMX_U32 codecFormat = PIXEL_FMT_BUTT;
    switch (omxColorFormat) {
    case OMX_COLOR_Format16bitRGB565:
        codecFormat = PIXEL_FMT_RGB_565;
        break;
    case OMX_COLOR_Format12bitRGB444:
        codecFormat = PIXEL_FMT_RGB_444;
        break;
    case OMX_COLOR_Format24bitRGB888:
        codecFormat = PIXEL_FMT_RGB_888;
        break;
    case OMX_COLOR_Format16bitBGR565:
        codecFormat = PIXEL_FMT_BGR_565;
        break;
    case OMX_COLOR_Format32bitBGRA8888:
        codecFormat = PIXEL_FMT_BGRA_8888;
        break;
    case OMX_COLOR_FormatYUV422SemiPlanar:
        codecFormat = PIXEL_FMT_YCBCR_422_SP;
        break;
    case OMX_COLOR_FormatYUV420SemiPlanar:
        codecFormat = PIXEL_FMT_YCBCR_420_SP;
        break;
    case OMX_COLOR_FormatYUV422Planar:
        codecFormat = PIXEL_FMT_YCBCR_422_P;
        break;
    case OMX_COLOR_FormatYUV420Planar:
        codecFormat = PIXEL_FMT_YCBCR_420_P;
        break;
    case OMX_COLOR_FormatYCbYCr:
        codecFormat = PIXEL_FMT_YUYV_422_PKG;
        break;
    case OMX_COLOR_FormatCbYCrY:
        codecFormat = PIXEL_FMT_UYVY_422_PKG;
        break;
    case OMX_COLOR_FormatYCrYCb:
        codecFormat = PIXEL_FMT_YVYU_422_PKG;
        break;
    case OMX_COLOR_FormatCrYCbY:
        codecFormat = PIXEL_FMT_VYUY_422_PKG;
        break;
    default:
        codecFormat = CodecOmxExtColor2CodecFormat((enum CodecColorFormatExt)omxColorFormat);
        break;
    }
    return codecFormat;
}
OMX_COLOR_FORMATTYPE Rochip_OSAL_CodecFormat2OmxColorFormat(OMX_U32 codecFormat)
{
    OMX_COLOR_FORMATTYPE omxColorFormat = OMX_COLOR_FormatUnused;
    switch (codecFormat) {
    case PIXEL_FMT_RGB_565:
        omxColorFormat = OMX_COLOR_Format16bitRGB565;
        break;
    case PIXEL_FMT_RGB_444:
        omxColorFormat = OMX_COLOR_Format12bitRGB444;
        break;
    case PIXEL_FMT_RGB_888:
        omxColorFormat = OMX_COLOR_Format24bitRGB888;
        break;
    case PIXEL_FMT_BGR_565:
        omxColorFormat = OMX_COLOR_Format16bitBGR565;
        break;
    case PIXEL_FMT_BGRA_8888:
        omxColorFormat = OMX_COLOR_Format32bitBGRA8888;
        break;
    case PIXEL_FMT_YCBCR_422_SP:
        omxColorFormat = OMX_COLOR_FormatYUV422SemiPlanar;
        break;
    case PIXEL_FMT_YCBCR_420_SP:
        omxColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        break;
    case PIXEL_FMT_YCBCR_422_P:
        omxColorFormat = OMX_COLOR_FormatYUV422Planar;
        break;
    case PIXEL_FMT_YCBCR_420_P:
        omxColorFormat = OMX_COLOR_FormatYUV420Planar;
        break;
    case PIXEL_FMT_YUYV_422_PKG:
        omxColorFormat = OMX_COLOR_FormatYCbYCr;
        break;
    case PIXEL_FMT_UYVY_422_PKG:
        omxColorFormat = OMX_COLOR_FormatCbYCrY;
        break;
    case PIXEL_FMT_YVYU_422_PKG:
        omxColorFormat = OMX_COLOR_FormatYCrYCb;
        break;
    case PIXEL_FMT_VYUY_422_PKG:
        omxColorFormat = OMX_COLOR_FormatCrYCbY;
        break;
    case PIXEL_FMT_RGBA_8888:
        omxColorFormat = (OMX_COLOR_FORMATTYPE)CODEC_COLOR_FORMAT_RGBA8888;
        break;
    default:
        omx_err("%s: undefined codecColorFormat[%d]", __func__, omxColorFormat);
        break;
    }
    return omxColorFormat;
}
OMX_COLOR_FORMATTYPE Rockchip_OSAL_GetBufferHandleColorFormat(BufferHandle* bufferHandle)
{
    if (bufferHandle == NULL) {
        omx_err_f("bufferHandle is null");
        return OMX_COLOR_FormatUnused;
    }
    return Rochip_OSAL_CodecFormat2OmxColorFormat(bufferHandle->format);
}
#endif

OMX_COLOR_FORMATTYPE Rockchip_OSAL_Hal2OMXPixelFormat(
    unsigned int hal_format)
{
    OMX_COLOR_FORMATTYPE omx_format;
    switch (hal_format) {
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
            omx_format = OMX_COLOR_FormatYCbYCr;
            break;
        case HAL_PIXEL_FORMAT_YV12:
            omx_format = OMX_COLOR_FormatYUV420Planar;
            break;
        case HAL_PIXEL_FORMAT_YCrCb_NV12:
            omx_format = OMX_COLOR_FormatYUV420SemiPlanar;
            break;
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
            omx_format = OMX_COLOR_FormatYUV420SemiPlanar;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            omx_format = (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYUV420Flexible;
            break;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            omx_format = OMX_COLOR_Format32bitBGRA8888;
            break;
        case  HAL_PIXEL_FORMAT_RGBA_8888:
        case  HAL_PIXEL_FORMAT_RGBX_8888:
            omx_format = OMX_COLOR_Format32bitARGB8888;
            break;
        default:
            omx_format = OMX_COLOR_FormatYUV420Planar;
            break;
    }
    return omx_format;
}

unsigned int Rockchip_OSAL_OMX2HalPixelFormat(
    OMX_COLOR_FORMATTYPE omx_format)
{
    unsigned int hal_format;
    switch ((OMX_U32)omx_format) {
        case OMX_COLOR_FormatYCbYCr:
            hal_format = HAL_PIXEL_FORMAT_YCbCr_422_I;
            break;
        case OMX_COLOR_FormatYUV420Planar:
            hal_format = HAL_PIXEL_FORMAT_YV12;
            break;
        case OMX_COLOR_FormatYUV420SemiPlanar:
            hal_format = HAL_PIXEL_FORMAT_YCrCb_NV12;
            break;
        case OMX_COLOR_FormatYUV420Flexible:
            hal_format = HAL_PIXEL_FORMAT_YCbCr_420_888;
            break;
        case OMX_COLOR_Format32bitARGB8888:
            hal_format = HAL_PIXEL_FORMAT_RGBA_8888;
            break;
        case OMX_COLOR_Format32bitBGRA8888:
            hal_format = HAL_PIXEL_FORMAT_BGRA_8888;
            break;
        default:
            hal_format = HAL_PIXEL_FORMAT_YV12;
            break;
    }
    return hal_format;
}

OMX_ERRORTYPE Rockchip_OSAL_CommitBuffer(
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
    OMX_U32 index)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT        *pRockchipPort        = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    struct vpu_display_mem_pool *pMem_pool = (struct vpu_display_mem_pool*)pVideoDec->vpumem_handle;
    OMX_U32 width = pRockchipPort->portDefinition.format.video.nStride;
    OMX_U32 height = pRockchipPort->portDefinition.format.video.nSliceHeight;
#if 1 // LOW_VRESION
    OMX_U32 nBytesize = width * height * 2;
#else
    OMX_U32 nBytesize = width * height * 9 / 5;
#endif
    OMX_S32 dupshared_fd = -1;
    OMX_BUFFERHEADERTYPE* bufferHeader = pRockchipPort->extendBufferHeader[index].OMXBufferHeader;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Rockchip_OSAL_Fd2VpumemPool(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
    OMX_BUFFERHEADERTYPE* bufferHeader)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    ROCKCHIP_OMX_BASEPORT        *pRockchipPort        = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    OMX_U32 i = 0;

    for (i = 0; i < pRockchipPort->portDefinition.nBufferCountActual; i++) {
        if (pRockchipPort->extendBufferHeader[i].OMXBufferHeader == bufferHeader) {
            omx_trace("commit bufferHeader 0x%x", bufferHeader);
            break;
        }
    }

    if (!pRockchipPort->extendBufferHeader[i].pRegisterFlag) {
        ret = Rockchip_OSAL_CommitBuffer(pRockchipComponent, i);
        if (ret != OMX_ErrorNone) {
            omx_err("commit buffer error header: %p", bufferHeader);
        }
    } else {
        omx_trace(" free bufferHeader 0x%x", pRockchipPort->extendBufferHeader[i].OMXBufferHeader);
        if (pRockchipPort->extendBufferHeader[i].pPrivate != nullptr) {
            Rockchip_OSAL_FreeVpumem(pRockchipPort->extendBufferHeader[i].pPrivate);
            pRockchipPort->extendBufferHeader[i].pPrivate = nullptr;
        };
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Rockchip_OSAL_resetVpumemPool(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    struct vpu_display_mem_pool *pMem_pool = (struct vpu_display_mem_pool*)pVideoDec->vpumem_handle;
    pMem_pool->reset(pMem_pool);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  Rockchip_OSAL_FreeVpumem(OMX_IN OMX_PTR pVpuframe)
{
    omx_trace("Rockchip_OSAL_FreeVpumem");
    VPU_FRAME *pframe = (VPU_FRAME *)pVpuframe;
    VPUMemLink(&pframe->vpumem);
    VPUFreeLinear(&pframe->vpumem);
    Rockchip_OSAL_Free(pframe);
    return OMX_ErrorNone;
}

OMX_BUFFERHEADERTYPE *Rockchip_OSAL_Fd2OmxBufferHeader(ROCKCHIP_OMX_BASEPORT *pRockchipPort,
                                                       OMX_IN OMX_S32 fd, OMX_IN OMX_PTR pVpuframe)
{
    OMX_U32 i = 0;
    for (i = 0; i < pRockchipPort->portDefinition.nBufferCountActual; i++) {
        if (fd == pRockchipPort->extendBufferHeader[i].buf_fd[0]) {
            omx_trace(" current fd = 0x%x send to render current header 0x%x",
                fd, pRockchipPort->extendBufferHeader[i].OMXBufferHeader);
            if (pRockchipPort->extendBufferHeader[i].pPrivate != nullptr) {
                omx_trace("This buff alreay send to display ");
                return nullptr;
            }
            if (pVpuframe) {
                pRockchipPort->extendBufferHeader[i].pPrivate = pVpuframe;
            } else {
                omx_trace("vpu_mem point is nullptr may error");
            }
            return pRockchipPort->extendBufferHeader[i].OMXBufferHeader;
        }
    }
    return nullptr;
}

OMX_ERRORTYPE  Rockchip_OSAL_Openvpumempool(OMX_IN ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent, OMX_U32 portIndex)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT *pRockchipPort = &pRockchipComponent->pRockchipPort[portIndex];
    if (pRockchipPort->bufferProcessType == BUFFER_SHARE) {
        pVideoDec->vpumem_handle = (void*)open_vpu_memory_pool();
        if (pVideoDec->vpumem_handle != nullptr) {
            omx_trace("open_vpu_memory_pool success handle 0x%x", pVideoDec->vpumem_handle);
        }
    } else {
        vpu_display_mem_pool   *pool = nullptr;
        OMX_U32 hor_stride = Get_Video_HorAlign(pVideoDec->codecId,
            pRockchipPort->portDefinition.format.video.nFrameWidth,
            pRockchipPort->portDefinition.format.video.nFrameHeight, pVideoDec->codecProfile);

        OMX_U32 ver_stride = Get_Video_VerAlign(pVideoDec->codecId,
            pRockchipPort->portDefinition.format.video.nFrameHeight, pVideoDec->codecProfile);
        omx_err("hor_stride %d ver_stride %d", hor_stride, ver_stride);
        if (0 != create_vpu_memory_pool_allocator(&pool, 8, (hor_stride * ver_stride * 2))) {
            omx_err("create_vpu_memory_pool_allocator fail");
        }
        pVideoDec->vpumem_handle = (void*)(pool);
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE  Rockchip_OSAL_Closevpumempool(OMX_IN ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT *pRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    if (pRockchipPort->bufferProcessType == BUFFER_SHARE && pVideoDec->vpumem_handle != nullptr) {
        close_vpu_memory_pool((vpu_display_mem_pool *)pVideoDec->vpumem_handle);
        pVideoDec->vpumem_handle = nullptr;
    } else if (pVideoDec->vpumem_handle != nullptr) {
        release_vpu_memory_pool_allocator((vpu_display_mem_pool*)pVideoDec->vpumem_handle);
        pVideoDec->vpumem_handle  = nullptr;
    }
    return OMX_ErrorNone;
}

// DDR Frequency conversion
OMX_ERRORTYPE Rockchip_OSAL_PowerControl(
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
    int32_t width,
    int32_t height,
    int32_t mHevc,
    int32_t frameRate,
    OMX_BOOL mFlag,
    int bitDepth)
{
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    OMX_U32 nValue = 0;
    if (Rockchip_OSAL_GetEnvU32("sf.power.control", &nValue, 0) || nValue <= 0) {
        omx_info("power control is not set");
        return OMX_ErrorUndefined;
    }

    if (pVideoDec->power_fd == -1) {
        pVideoDec->power_fd = open("/dev/video_state", O_WRONLY);
        if (pVideoDec->power_fd == -1) {
            omx_err("power control open /dev/video_state fail!");
        }
    }

    if (pVideoDec->power_fd == -1) {
        pVideoDec->power_fd = open("/sys/class/devfreq/dmc/system_status", O_WRONLY);
        if (pVideoDec->power_fd == -1) {
            omx_err("power control open /sys/class/devfreq/dmc/system_status fail");
        }
    }

    if (bitDepth <= 0) {
        bitDepth = 8;
    }

    char para[200] = {0}; // 200:array length
    int paraLen = 0;
    paraLen = sprintf(para, "%d,width=%d,height=%d,ishevc=%d,videoFramerate=%d,streamBitrate=%d",
        mFlag, width, height, mHevc, frameRate, bitDepth);
    omx_info(" write: %s", para);
    if (pVideoDec->power_fd != -1) {
        write(pVideoDec->power_fd, para, paraLen);
        if (!mFlag) {
            close(pVideoDec->power_fd);
            pVideoDec->power_fd = -1;
        }
    }

    return OMX_ErrorNone;
}

OMX_COLOR_FORMATTYPE Rockchip_OSAL_CheckFormat(
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent,
    OMX_IN OMX_PTR pVpuframe)
{
    RKVPU_OMX_VIDEODEC_COMPONENT    *pVideoDec  = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT           *pInputPort         = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    ROCKCHIP_OMX_BASEPORT           *pOutputPort        = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    OMX_COLOR_FORMATTYPE             eColorFormat       = pOutputPort->portDefinition.format.video.eColorFormat;
    VPU_FRAME *pframe = (VPU_FRAME *)pVpuframe;

    if ((pVideoDec->codecId == (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC && (pframe->OutputWidth != 0x20))
        || (pframe->ColorType & VPU_OUTPUT_FORMAT_BIT_MASK) == VPU_OUTPUT_FORMAT_BIT_10) { // 10bit
        OMX_BOOL fbcMode = Rockchip_OSAL_Check_Use_FBCMode(pVideoDec->codecId,
                                                           (int32_t)OMX_DEPTH_BIT_10, pOutputPort);

        if ((pframe->ColorType & 0xf) == VPU_OUTPUT_FORMAT_YUV422) {
            if (fbcMode) {
                eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_Y210;
            } else {
                eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCbCr_422_SP_10;
            }
        } else {
            if (fbcMode) {
                eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YUV420_10BIT_I;
            } else {
                eColorFormat = (OMX_COLOR_FORMATTYPE)HAL_PIXEL_FORMAT_YCrCb_NV12_10;
            }
        }

        if ((pframe->ColorType & OMX_COLORSPACE_MASK) != 0) {
            OMX_RK_EXT_COLORSPACE colorSpace =
                (OMX_RK_EXT_COLORSPACE)((pframe->ColorType & OMX_COLORSPACE_MASK) >> 20); // 20:byte alignment
            pVideoDec->extColorSpace = colorSpace;
            omx_trace("extension color space = %d", colorSpace);
        }
        if ((pframe->ColorType & OMX_DYNCRANGE_MASK) != 0) {
            OMX_RK_EXT_DYNCRANGE dyncRange = (OMX_RK_EXT_DYNCRANGE)((pframe->ColorType & OMX_DYNCRANGE_MASK) >> 24);
            pVideoDec->extDyncRange = dyncRange;
        }

        if (pVideoDec->bIsPowerControl == OMX_TRUE && pVideoDec->bIs10bit == OMX_FALSE) {
            Rockchip_OSAL_PowerControl(pRockchipComponent, 3840, 2160, pVideoDec->bIsHevc, /* 3840:Resolution size,
                                                                                             2160:Resolution size */
                                       pInputPort->portDefinition.format.video.xFramerate,
                                       OMX_FALSE,
                                       8); // 8:point
            pVideoDec->bIsPowerControl = OMX_FALSE;
        }

        if (pframe->FrameWidth > 1920 && pframe->FrameHeight > 1088 // 1920:Resolution size, 1088:Resolution size
            && pVideoDec->bIsPowerControl == OMX_FALSE) {
            Rockchip_OSAL_PowerControl(pRockchipComponent, 3840, 2160, pVideoDec->bIsHevc, /* 3840:Resolution size,
                                                                                             2160:Resolution size */
                                       pInputPort->portDefinition.format.video.xFramerate,
                                       OMX_TRUE,
                                       10); // 10:point
            pVideoDec->bIsPowerControl = OMX_TRUE;
        }
        pVideoDec->bIs10bit = OMX_TRUE;
    }

    return eColorFormat;
}

#ifdef AVS80
OMX_U32 Rockchip_OSAL_GetVideoNativeMetaSize()
{
    return sizeof(VideoNativeMetadata);
}

OMX_U32 Rockchip_OSAL_GetVideoGrallocMetaSize()
{
    return sizeof(VideoGrallocMetadata);
}
#endif

OMX_ERRORTYPE Rkvpu_ComputeDecBufferCount(
    OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = nullptr;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = nullptr;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = nullptr;
    ROCKCHIP_OMX_BASEPORT      *pInputRockchipPort = nullptr;
    ROCKCHIP_OMX_BASEPORT      *pOutputRockchipPort = nullptr;
    OMX_U32                     nValue = 0;
    OMX_BOOL                    nLowMemMode = OMX_FALSE;
    OMX_U32                     nTotalMemSize = 0;
    OMX_U32                     nMaxBufferCount = 0;
    OMX_U32                     nMaxLowMemBufferCount = 0;
    OMX_U32                     nBufferSize = 0;
    OMX_U32                     nRefFrameNum = 0;

    FunctionIn();

    char  pValue[128 + 1]; // 128:array length

    if (hComponent == nullptr) {
        omx_err("omx component is nullptr");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err("omx component version check failed!");
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == nullptr) {
        omx_err("omx component private is nullptr!");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    if (pVideoDec == nullptr) {
        omx_err("video decode component is nullptr!");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pRockchipComponent->currentState == OMX_StateInvalid) {
        omx_err("current state is invalid!");
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pInputRockchipPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    pOutputRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    nBufferSize = pOutputRockchipPort->portDefinition.nBufferSize;

    if (!Rockchip_OSAL_GetEnvU32("sys.video.maxMemCapacity", &nValue, 0) && (nValue > 0)) {
        omx_info("use low memory mode, set low mem : %d MB", nValue);
        nTotalMemSize = nValue * 1024 * 1024;
        nLowMemMode = OMX_TRUE;
    }

    if (nLowMemMode) {
        pInputRockchipPort->portDefinition.nBufferCountActual = 3;
        pInputRockchipPort->portDefinition.nBufferCountMin = 3;
#ifdef AVS80
        pVideoDec->nMinUnDequeBufferCount = 3;
#else
        pVideoDec->nMinUnDequeBufferCount = 4;
#endif
    } else {
        pInputRockchipPort->portDefinition.nBufferCountActual = MAX_VIDEO_INPUTBUFFER_NUM;
        pInputRockchipPort->portDefinition.nBufferCountMin = MAX_VIDEO_INPUTBUFFER_NUM;
        pVideoDec->nMinUnDequeBufferCount = 4;
    }

    if (BUFFER_COPY == pOutputRockchipPort->bufferProcessType) {
        nMaxBufferCount = pInputRockchipPort->portDefinition.nBufferCountActual;
        pVideoDec->nMinUnDequeBufferCount = 0;
    } else {
        OMX_BOOL isSecure = OMX_FALSE;
        // for gts exo test
        Rockchip_OSAL_Memset(pValue, 0, sizeof(pValue));
        if (!Rockchip_OSAL_GetEnvStr("cts_gts.exo.gts", pValue, nullptr) && !strcasecmp(pValue, "true")) {
            omx_info("This is gts exo test. pValue: %s", pValue);
            nRefFrameNum = 7;
        } else {
            nRefFrameNum = Rockchip_OSAL_CalculateTotalRefFrames(
                pVideoDec->codecId,
                pInputRockchipPort->portDefinition.format.video.nFrameWidth,
                pInputRockchipPort->portDefinition.format.video.nFrameHeight,
                isSecure);
        }
        if (pVideoDec->nDpbSize > 0) {
            nRefFrameNum = pVideoDec->nDpbSize;
        }
        if ((pInputRockchipPort->portDefinition.format.video.nFrameWidth
             * pInputRockchipPort->portDefinition.format.video.nFrameHeight > 1920 * 1088)
            || isSecure || access("/dev/iep", F_OK) == -1) {
            nMaxBufferCount = nRefFrameNum + pVideoDec->nMinUnDequeBufferCount + 1;
        } else {
            /* if width * height < 2304 * 1088, need consider IEP */
            nMaxBufferCount = nRefFrameNum + pVideoDec->nMinUnDequeBufferCount + 1 + DEFAULT_IEP_OUTPUT_BUFFER_COUNT;
        }

        if (nLowMemMode) {
            nMaxLowMemBufferCount = nTotalMemSize / nBufferSize;
            if (nMaxLowMemBufferCount > 23) {
                nMaxLowMemBufferCount = 23;
            }
            nMaxBufferCount = (nMaxLowMemBufferCount < nMaxBufferCount) ? nMaxLowMemBufferCount : nMaxBufferCount;
        }
    }

    if (pOutputRockchipPort->portDefinition.nBufferCountActual < nMaxBufferCount)
        pOutputRockchipPort->portDefinition.nBufferCountActual = nMaxBufferCount;
    pOutputRockchipPort->portDefinition.nBufferCountMin = nMaxBufferCount - pVideoDec->nMinUnDequeBufferCount;

    omx_info("input nBufferSize: %d, width: %d, height: %d, minBufferCount: %d, bufferCount: %d",
             pInputRockchipPort->portDefinition.nBufferSize,
             pInputRockchipPort->portDefinition.format.video.nFrameWidth,
             pInputRockchipPort->portDefinition.format.video.nFrameHeight,
             pInputRockchipPort->portDefinition.nBufferCountMin,
             pInputRockchipPort->portDefinition.nBufferCountActual);

    omx_info("output nBufferSize: %d, width: %d, height: %d, minBufferCount: %d, bufferCount: %d buffer type: %d",
             pOutputRockchipPort->portDefinition.nBufferSize,
             pOutputRockchipPort->portDefinition.format.video.nFrameWidth,
             pOutputRockchipPort->portDefinition.format.video.nFrameHeight,
             pOutputRockchipPort->portDefinition.nBufferCountMin,
             pOutputRockchipPort->portDefinition.nBufferCountActual,
             pOutputRockchipPort->bufferProcessType);

EXIT:
    FunctionOut();

    return ret;
}

OMX_U32 Rockchip_OSAL_CalculateTotalRefFrames(
    OMX_VIDEO_CODINGTYPE codecId,
    OMX_U32 width,
    OMX_U32 height,
    OMX_BOOL isSecure)
{
    OMX_U32 nRefFramesNum = 0;
    switch (codecId) {
        case OMX_VIDEO_CodingAVC: {
            /* used level 5.1 MaxDpbMbs */
            nRefFramesNum = 184320 / ((width / 16) * (height / 16)); /* 184320:frame number,
                                                                        16:frame number */
            if (nRefFramesNum > 16) { // 16:frame number
                /* max refs frame number is 16 */
                nRefFramesNum = 16; // 16:frame number
            } else if (nRefFramesNum < 6) { // 6:frame number
                nRefFramesNum = 6; // 6:frame number
            }
        } break;
        case CODEC_OMX_VIDEO_CodingHEVC: {
            /* use 4K refs frame Num to computate others */
            nRefFramesNum = 4096 * 2160 * 6 / (width * height); /* 4096:Resolution size,
                                                                   2160:Resolution size, 6:frame number */
            if (nRefFramesNum > 16) {
                /* max refs frame number is 16 */
                nRefFramesNum = 16;
            } else if (nRefFramesNum < 6) {
                /* min refs frame number is 6 */
                nRefFramesNum = 6;
            }
        } break;
        case OMX_VIDEO_CodingVP9: {
            nRefFramesNum = 4096 * 2176 * 4 / (width * height);/* 4096:Resolution size,
                                                                   2176:Resolution size, 4:frame number */
            if (nRefFramesNum > 8) { // 8:frame number
                nRefFramesNum = 8; // 8:frame number
            } else if (nRefFramesNum < 4) { // 4:frame number
                nRefFramesNum = 4; // 4:frame number
            }
        } break;
        default: {
            nRefFramesNum = 8;
        } break;
    }

    /*
     * for SVP, Usually it is streaming video, and secure buffering
     * is smaller, so buffer allocation is less.
     */
    if (isSecure && ((width * height) > 1280 * 768)) { /* 1280:Resolution size, 768:Resolution size */
        nRefFramesNum = nRefFramesNum > 9 ? 9 : nRefFramesNum;
    }

    return nRefFramesNum;
}

#ifdef __cplusplus
}
#endif