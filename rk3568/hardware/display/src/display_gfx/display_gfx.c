/*
 * Copyright (c) 2021-2023 Rockchip Electronics Co., Ltd.
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

#include <stdio.h>
#include <string.h>
#include "im2d.h"
#include "rga.h"
#include "RgaUtils.h"
#include "display_log.h"
#include "securec.h"
#include "display_gfx.h"

#define ALIGN_UP(x, a) ((((x) + ((a)-1)) / (a)) * (a))
int32_t rkInitGfx()
{
    DISPLAY_LOGE("%s\n", querystring(RGA_ALL));
    return DISPLAY_SUCCESS;
}

int32_t rkDeinitGfx()
{
    return DISPLAY_SUCCESS;
}

RgaSURF_FORMAT colorSpaceModeChange(PixelFormat color, uint8_t *isYuv)
{
    RgaSURF_FORMAT rkFormat;
    switch (color) {
        case PIXEL_FMT_RGB_565:          /**< RGB565 format */
            rkFormat = RK_FORMAT_RGB_565;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGBA_4444:        /**< RGBA4444 format */
            rkFormat = RK_FORMAT_RGBA_4444;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGBA_5551:        /**< RGBA5551 format */
            rkFormat = RK_FORMAT_RGBA_5551;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGBX_8888:        /**< RGBX8888 format */
            rkFormat = RK_FORMAT_RGBX_8888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGBA_8888:        /**< RGBA8888 format */
            rkFormat = RK_FORMAT_RGBA_8888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGB_888:          /**< RGB888 format */
            rkFormat = RK_FORMAT_RGB_888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGR_565:          /**< BGR565 format */
            rkFormat = RK_FORMAT_BGR_565;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGRA_4444:        /**< BGRA4444 format */
            rkFormat = RK_FORMAT_BGRA_4444;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGRA_5551:        /**< BGRA5551 format */
            rkFormat = RK_FORMAT_BGRA_5551;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGRX_8888:        /**< BGRX8888 format */
            rkFormat = RK_FORMAT_BGRX_8888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGRA_8888:        /**< BGRA8888 format */
            rkFormat = RK_FORMAT_BGRA_8888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_YCBCR_422_SP:     /**< YCBCR422 semi-planar format */
            rkFormat = RK_FORMAT_YCbCr_420_SP;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCRCB_422_SP:     /**< YCRCB422 semi-planar format */
            rkFormat = RK_FORMAT_YCrCb_422_SP;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCBCR_420_SP:     /**< YCBCR420 semi-planar format */
            rkFormat = RK_FORMAT_YCbCr_420_SP;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCRCB_420_SP:     /**< YCRCB420 semi-planar format */
            rkFormat = RK_FORMAT_YCrCb_420_SP;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCBCR_422_P:      /**< YCBCR422 planar format */
            rkFormat = RK_FORMAT_YCbCr_422_P;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCRCB_422_P:      /**< YCRCB422 planar format */
            rkFormat = RK_FORMAT_YCrCb_422_P;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCBCR_420_P:      /**< YCBCR420 planar format */
            rkFormat = RK_FORMAT_YCbCr_420_P;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCRCB_420_P:      /**< YCRCB420 planar format */
            rkFormat = RK_FORMAT_YCrCb_420_P;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YUYV_422_PKG:     /**< YUYV422 packed format */
            rkFormat = RK_FORMAT_YUYV_422;
            *isYuv = 1;
            break;
        case PIXEL_FMT_UYVY_422_PKG:     /**< UYVY422 packed format */
            rkFormat = RK_FORMAT_UYVY_422;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YVYU_422_PKG:     /**< YVYU422 packed format */
            rkFormat = RK_FORMAT_YUYV_422;
            *isYuv = 1;
            break;
        case PIXEL_FMT_VYUY_422_PKG:     /**< VYUY422 packed format */
            rkFormat = RK_FORMAT_VYUY_422;
            *isYuv = 1;
            break;
        default:
//          PIXEL_FMT_CLUT8:        /**< CLUT8 format */
//          PIXEL_FMT_CLUT1,            /**< CLUT1 format */
//          PIXEL_FMT_CLUT4,            /**< CLUT4 format */
//          PIXEL_FMT_RGBA_5658,        /**< RGBA5658 format */
//          PIXEL_FMT_RGBX_4444,        /**< RGBX4444 format */
//          PIXEL_FMT_RGB_444,          /**< RGB444 format */
//          PIXEL_FMT_RGBX_5551,        /**< RGBX5551 format */
//          PIXEL_FMT_RGB_555,          /**< RGB555 format */
//          PIXEL_FMT_BGRX_4444,        /**< BGRX4444 format */
//          PIXEL_FMT_BGRX_5551,        /**< BGRX5551 format */
//          PIXEL_FMT_YUV_422_I,        /**< YUV422 interleaved format */
            rkFormat = RK_FORMAT_UNKNOWN;
            break;
    }
    return rkFormat;
}

int32_t rkFillRect(ISurface *iSurface, IRect *rect, uint32_t color, GfxOpt *opt)
{
    rga_buffer_t dst;
    rga_buffer_handle_t dst_handle = 0;
    im_handle_param_t dst_param;
    im_rect imRect;
    IM_STATUS ret;
    uint8_t isYuv;
    int32_t ret_val = DISPLAY_SUCCESS;

    errno_t eok = memset_s((void *)&imRect, sizeof(imRect), 0, sizeof(imRect));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        ret_val = false;
        goto EXIT;
    }
    imRect.x = rect->x;
    imRect.y = rect->y;
    imRect.width = rect->w;
    imRect.height = rect->h;

    eok = memset_s((void *)&dst, sizeof(dst), 0, sizeof(dst));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        ret_val = false;
        goto EXIT;
    }

    dst_param.width = (uint32_t)iSurface->width;
    dst_param.height = (uint32_t)iSurface->height;
    dst_param.format = (uint32_t)colorSpaceModeChange(iSurface->enColorFmt, &isYuv);

    dst_handle = importbuffer_fd(iSurface->phyAddr, &dst_param);
    if (dst_handle == 0) {
        DISPLAY_LOGE("import dst_handle error!");
        ret_val = DISPLAY_FAILURE;
        goto EXIT;
    }
    dst = wrapbuffer_handle(dst_handle, iSurface->width, iSurface->height, dst_param.format);

    ret = imfill(dst, imRect, color);
    if (ret != IM_STATUS_SUCCESS) {
        DISPLAY_LOGE("gfx imfill %{public}s", imStrError(ret));
        ret_val = DISPLAY_FAILURE;
    }

EXIT:
    if (dst_handle) {
        releasebuffer_handle(dst_handle);
    }
    return ret_val;
}

int32_t blendTypeChange(BlendType blendType)
{
    int32_t rkBlendType;
    switch (blendType) {
        case BLEND_SRC:              /**< SRC blending */
            rkBlendType = IM_ALPHA_BLEND_SRC;
            break;
        case BLEND_DST:              /**< SRC blending */
            rkBlendType = IM_ALPHA_BLEND_DST;
            break;
        case BLEND_SRCOVER:          /**< SRC_OVER blending */
            rkBlendType = IM_ALPHA_BLEND_SRC_OVER;
            break;
        case BLEND_DSTOVER:          /**< DST_OVER blending */
            rkBlendType = IM_ALPHA_BLEND_DST_OVER;
            break;
        default:
            /* Fix up later */
//        BLEND_NONE         /**< No blending */
//        BLEND_CLEAR:            /**< CLEAR blending */
//        BLEND_SRCIN:            /**< SRC_IN blending */
//        BLEND_DSTIN:            /**< DST_IN blending */
//        BLEND_SRCOUT:           /**< SRC_OUT blending */
//        BLEND_DSTOUT:           /**< DST_OUT blending */
//        BLEND_SRCATOP:          /**< SRC_ATOP blending */
//        BLEND_DSTATOP:          /**< DST_ATOP blending */
//        BLEND_ADD:              /**< ADD blending */
//        BLEND_XOR:              /**< XOR blending */
//        BLEND_DST:              /**< DST blending */
//        BLEND_AKS:              /**< AKS blending */
//        BLEND_AKD:              /**< AKD blending */
//        BLEND_BUTT:              /**< Null operation */
            rkBlendType = IM_STATUS_NOT_SUPPORTED;
            break;
    }
    return rkBlendType;
}

int32_t TransformTypeChange(TransformType type)
{
    int32_t rkTransformType;
    switch (type) {
        case ROTATE_90:            /**< Rotation by 90 degrees */
            rkTransformType = IM_HAL_TRANSFORM_ROT_90;
            break;
        case ROTATE_180:           /**< Rotation by 180 degrees */
            rkTransformType = IM_HAL_TRANSFORM_ROT_180;
            break;
        case ROTATE_270:           /**< Rotation by 270 degrees */
            rkTransformType = IM_HAL_TRANSFORM_ROT_270;
            break;
        case MIRROR_H:             /**< Mirror transform horizontally */
            rkTransformType = IM_HAL_TRANSFORM_FLIP_H;
            break;
        case MIRROR_V:             /**< Mirror transform vertically */
            rkTransformType = IM_HAL_TRANSFORM_FLIP_V;
            break;
        case MIRROR_H_ROTATE_90:   /**< Mirror transform horizontally, rotation by 90 degrees */
            rkTransformType = IM_HAL_TRANSFORM_ROT_90 | IM_HAL_TRANSFORM_FLIP_V;
            break;
        case MIRROR_V_ROTATE_90:   /**< Mirror transform vertically, rotation by 90 degrees */
            rkTransformType = IM_HAL_TRANSFORM_ROT_90 | IM_HAL_TRANSFORM_FLIP_H;
            break;
        default:
            rkTransformType = 0;   /**< No rotation */
            break;
    }
    return rkTransformType;
}

int32_t doFlit(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    int32_t usage = 0;
    uint8_t isYuv = 0;
    rga_buffer_t dstRgaBuffer, srcRgaBuffer, bRgbBuffer;
    rga_buffer_handle_t src_handle = 0, dst_handle = 0, brgb_handle = 0;
    im_handle_param_t src_param, dst_param, brgb_param;
    IM_STATUS ret = 0;
    im_rect srect;
    im_rect drect;
    im_rect prect;
    int32_t rkBlendType = 0;
    int32_t rkTransformType = 0;
    int32_t ret_val = DISPLAY_SUCCESS;

    errno_t eok = memset_s(&dstRgaBuffer, sizeof(dstRgaBuffer), 0, sizeof(dstRgaBuffer));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        ret_val = false;
        goto EXIT;
    }
    eok = memset_s(&srcRgaBuffer, sizeof(srcRgaBuffer), 0, sizeof(srcRgaBuffer));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        ret_val = false;
        goto EXIT;
    }
    eok = memset_s(&bRgbBuffer, sizeof(bRgbBuffer), 0, sizeof(bRgbBuffer));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        ret_val = false;
        goto EXIT;
    }
    eok = memset_s(&srect, sizeof(srect), 0, sizeof(srect));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        ret_val = false;
        goto EXIT;
    }
    eok = memset_s(&drect, sizeof(drect), 0, sizeof(drect));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        ret_val = false;
        goto EXIT;
    }
    eok = memset_s(&prect, sizeof(prect), 0, sizeof(prect));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        ret_val = false;
        goto EXIT;
    }

    DISPLAY_LOGD("gfx src fd %{public}d, w %{public}d, h %{publuc}d, sw %{public}d sh %{public}d",
        (int32_t)srcSurface->phyAddr, srcSurface->width, srcSurface->height, ALIGN_UP(srcSurface->width, 16),
        ALIGN_UP(srcSurface->height, 2));
    DISPLAY_LOGD("gfx dst fd %{public}d, w %{public}d, h %{public}d, sw %{public}d sh %{public}d",
        (int32_t)dstSurface->phyAddr, dstSurface->width, dstSurface->height, ALIGN_UP(dstSurface->width, 16),
        ALIGN_UP(dstSurface->height, 2));

    dst_param.width = (uint32_t)dstSurface->width;
    dst_param.height = (uint32_t)dstSurface->height;
    dst_param.format = (uint32_t)colorSpaceModeChange(dstSurface->enColorFmt, &isYuv);
    if (isYuv == 1) {
        DISPLAY_LOGE("rk gfx do not support dst buffer is yuv format");
        ret_val = DISPLAY_PARAM_ERR;
        goto EXIT;
    }

    src_param.width = (uint32_t)srcSurface->width;
    src_param.height = (uint32_t)srcSurface->height;
    src_param.format = (uint32_t)colorSpaceModeChange(srcSurface->enColorFmt, &isYuv);

    dst_handle = importbuffer_fd(dstSurface->phyAddr, &dst_param);
    if (dst_handle == 0) {
        DISPLAY_LOGE("import dst_handle error!");
        ret_val = DISPLAY_FAILURE;
        goto EXIT;
    }
    if (srcSurface->phyAddr == 0) {
        DISPLAY_LOGE("source fd is invalid");
        ret_val = DISPLAY_PARAM_ERR;
        goto EXIT;
    }

    src_handle = importbuffer_fd(srcSurface->phyAddr, &src_param);
    if (src_handle == 0) {
        DISPLAY_LOGE("import src_handle error!");
        ret_val = DISPLAY_FAILURE;
        goto EXIT;
    }
    srcRgaBuffer = wrapbuffer_handle(src_handle, srcSurface->width, srcSurface->height, src_param.format);
    dstRgaBuffer = wrapbuffer_handle(dst_handle, dstSurface->width, dstSurface->height, dst_param.format);

    srcRgaBuffer.wstride = ALIGN_UP(srcSurface->width, 16);
    srcRgaBuffer.hstride = ALIGN_UP(srcSurface->height, 2);
    dstRgaBuffer.wstride = ALIGN_UP(dstSurface->width, 16);
    dstRgaBuffer.hstride = ALIGN_UP(dstSurface->height, 2);

    srect.x = srcRect->x;
    srect.x = (srect.x % 2 == 1) ? (srect.x - 1) : srect.x; // 2: Is it odd?
    srect.y = srcRect->y;
    srect.y = (srect.y % 2 == 1) ? (srect.y - 1) : srect.y; // 2: Is it odd?
    srect.height = srcRect->h;
    srect.height = (srect.height % 2 == 1) ? (srect.height + 1) : srect.height; // 2: Is it odd?
    srect.width = srcRect->w;
    srect.width = (srect.width % 2 == 1) ? (srect.width + 1) : srect.width; // 2: Is it odd?
    drect.x = dstRect->x;
    drect.x = (drect.x % 2 == 1) ? (drect.x - 1) : drect.x; // 2: Is it odd?
    drect.y = dstRect->y;
    drect.y = (drect.y % 2 == 1) ? (drect.y - 1) : drect.y; // 2: Is it odd?
    drect.height = dstRect->h;
    drect.height = (drect.height % 2 == 1) ? (drect.height + 1) : drect.height; // 2: Is it odd?
    drect.width = dstRect->w;
    drect.width = (drect.width % 2 == 1) ? (drect.width + 1) : drect.width; // 2: Is it odd?

    if (opt->blendType) {
        rkBlendType = blendTypeChange(opt->blendType);
        if (rkBlendType > 0) {
            usage |= rkBlendType;
        } else if (rkBlendType == IM_STATUS_NOT_SUPPORTED) {
            ret_val = DISPLAY_NOT_SUPPORT;
            goto EXIT;
        }
    }
    if (opt->rotateType) {
        rkTransformType = TransformTypeChange(opt->rotateType);
        if (rkTransformType != 0)
            usage |= rkTransformType;
    }
    if (opt->enableScale) {
        DISPLAY_LOGE("gfx scale from (%{puhblic}d, %{public}d) to (%{public}d, %{public}d)", \
            srcRgaBuffer.width, srcRgaBuffer.height, dstRgaBuffer.width, dstRgaBuffer.height);
    }
    usage |= IM_SYNC;
    if (isYuv == 1) {
        if (rkBlendType == IM_ALPHA_BLEND_SRC_OVER || rkBlendType == IM_ALPHA_BLEND_SRC) {
            usage = 0;
            if (opt->enableScale == 0) {
                eok = memset_s(&srect, sizeof(srect), 0, sizeof(srect));
                if (eok != EOK) {
                    DISPLAY_LOGE("memset_s failed");
                    ret_val = false;
                    goto EXIT;
                }
                srect.width = srcRgaBuffer.width;
                srect.height = srcRgaBuffer.height;

                eok = memset_s(&drect, sizeof(drect), 0, sizeof(drect));
                if (eok != EOK) {
                    DISPLAY_LOGE("memset_s failed");
                    ret_val = false;
                    goto EXIT;
                }
                drect.x = dstRgaBuffer.width - srcRgaBuffer.width;
                drect.y = dstRgaBuffer.height - srcRgaBuffer.height;
                drect.width = srcRgaBuffer.width;
                drect.height = srcRgaBuffer.height;
            }
            srcRgaBuffer.wstride = srcSurface->stride;
            usage = rkTransformType | IM_SYNC | IM_ALPHA_BLEND_PRE_MUL;
            srcRgaBuffer.color_space_mode = IM_YUV_TO_RGB_BT601_LIMIT;
            ret = improcess(srcRgaBuffer, dstRgaBuffer, bRgbBuffer, srect, drect, prect, usage);
            if (ret != IM_STATUS_SUCCESS) {
                DISPLAY_LOGE("gfx improcess %{public}s", imStrError(ret));
                ret_val = DISPLAY_FAILURE;
            }
        } else if (rkBlendType == IM_ALPHA_BLEND_DST_OVER) {
            brgb_param.width = dstRgaBuffer.width;
            brgb_param.height = dstRgaBuffer.height;
            brgb_param.format = RK_FORMAT_RGBA_8888;

            brgb_handle = importbuffer_fd(-1, &brgb_param);
            if (dst_handle == 0) {
                DISPLAY_LOGE("import dst_handle error!");
                ret_val = DISPLAY_FAILURE;
                goto EXIT;
            }
            bRgbBuffer = wrapbuffer_handle(brgb_handle, dstRgaBuffer.width, dstRgaBuffer.height, RK_FORMAT_RGBA_8888);
            int ret = memcpy_s(&prect, sizeof(drect), &drect, sizeof(drect));
            if (!ret) {
                printf("memcpy_s failed!\n");
            }
            ret = improcess(srcRgaBuffer, bRgbBuffer, dstRgaBuffer, srect, prect, drect, usage);
            if (ret != IM_STATUS_SUCCESS) {
                DISPLAY_LOGE("gfx improcess %{public}s", imStrError(ret));
                ret_val = DISPLAY_FAILURE;
            } else {
                ret = imcopy(bRgbBuffer, dstRgaBuffer);
                if (ret != IM_STATUS_SUCCESS) {
                    DISPLAY_LOGE("gfx imcopy %{public}s", imStrError(ret));
                    ret_val = DISPLAY_FAILURE;
                }
            }
        }
    } else {
        ret = improcess(srcRgaBuffer, dstRgaBuffer, bRgbBuffer, srect, drect, prect, usage);
        if (ret != IM_STATUS_SUCCESS) {
            DISPLAY_LOGE("gfx improcess %{public}s", imStrError(ret));
            ret_val = DISPLAY_FAILURE;
        }
    }

EXIT:
    if (brgb_handle) {
        releasebuffer_handle(brgb_handle);
    }
    if (src_handle) {
        releasebuffer_handle(src_handle);
    }
    if (dst_handle) {
        releasebuffer_handle(dst_handle);
    }
    return ret_val;
}

int32_t rkBlit(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    CHECK_NULLPOINTER_RETURN_VALUE(srcSurface, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(srcRect, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(dstSurface, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(dstRect, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(opt, DISPLAY_NULL_PTR);

    if (doFlit(srcSurface, srcRect, dstSurface, dstRect, opt) < 0)
        return DISPLAY_FAILURE;
    else
        return DISPLAY_SUCCESS;
}

int32_t rkSync(int32_t timeOut)
{
    return DISPLAY_SUCCESS;
}

int32_t GfxInitialize(GfxFuncs **funcs)
{
    DISPLAY_CHK_RETURN((funcs == NULL), DISPLAY_PARAM_ERR, DISPLAY_LOGE("info is null"));
    GfxFuncs *gfxFuncs = (GfxFuncs *)malloc(sizeof(GfxFuncs));
    DISPLAY_CHK_RETURN((gfxFuncs == NULL), DISPLAY_NULL_PTR, DISPLAY_LOGE("gfxFuncs is nullptr"));
    errno_t eok = memset_s((void *)gfxFuncs, sizeof(GfxFuncs), 0, sizeof(GfxFuncs));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        free(gfxFuncs);
        return DISPLAY_FAILURE;
    }
    gfxFuncs->InitGfx = rkInitGfx;
    gfxFuncs->DeinitGfx = rkDeinitGfx;
    gfxFuncs->FillRect = rkFillRect;
    gfxFuncs->Blit = rkBlit;
    gfxFuncs->Sync = rkSync;
    *funcs = gfxFuncs;

    return DISPLAY_SUCCESS;
}

int32_t GfxUninitialize(GfxFuncs *funcs)
{
    CHECK_NULLPOINTER_RETURN_VALUE(funcs, DISPLAY_NULL_PTR);
    free(funcs);
    DISPLAY_LOGD("%s: gfx uninitialize success", __func__);
    return DISPLAY_SUCCESS;
}
