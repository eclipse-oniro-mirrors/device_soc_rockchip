/*
 * Copyright (c) 2022-2023 HiHope Open Source Organization .
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

#include "hdi_mpp_config.h"
#include <hdf_base.h>
#include <hdf_log.h>
#include <securec.h>
#include "mpp_common.h"

#define HDF_LOG_TAG codec_hdi_mpp_config
#define MPP_ALIGN_STRIDE_WITH_EIGHT         8
#define MPP_ALIGN_STRIDE_WITH_SIXTEEN       16
#define MPP_ALIGN_STRIDE_WITH_SIXTY_FOUR    64
#define MPP_ALIGN_MULTIPLE_WITH_TWO         2
#define MPP_ALIGN_MULTIPLE_WITH_THREE       3
#define MPP_ALIGN_MULTIPLE_WITH_FOUR        4
#define MPP_ALIGN_DIVISOR_WITH_TWO          2
#define MPP_ALIGN_DIVISOR_WITH_THREE        3
#define MPP_ALIGN_DIVISOR_WITH_FOUR         4
#define MPP_ALIGN_DIVISOR_WITH_SIXTEEN      16
#define GOP_MODE_THRESHOLD_VALUE            4

#define BPS_BASE                            16
#define BPS_MAX                             17
#define BPS_MEDIUM                          15
#define BPS_MIN                             1
#define BPS_TARGET                          2
#define FIXQP_INIT_VALUE                    20
#define FIXQP_MAX_VALUE                     20
#define FIXQP_MIN_VALUE                     20
#define FIXQP_MAX_I_VALUE                   20
#define FIXQP_MIN_I_VALUE                   20
#define FIXQP_IP_VALUE                      2
#define OTHER_QP_INIT_VALUE                 26
#define OTHER_QP_MAX_VALUE                  51
#define OTHER_QP_MIN_VALUE                  10
#define OTHER_QP_MAX_I_VALUE                51
#define OTHER_QP_MIN_I_VALUE                10
#define OTHER_QP_IP_VALUE                   2
#define AVC_SETUP_LEVEL_DEFAULT             40
#define AVC_SETUP_CABAC_EN_DEFAULT          1
#define AVC_SETUP_CABAC_IDC_DEFAULT         0
#define AVC_SETUP_TRANS_DEFAULT             1
#define AVC_SETUP_PROFILE_DEFAULT           100
#define FPS_SETUP_DEFAULT                   24
#define DFAULT_ENC_DROP_THD                 20
#define DFAULT_ENC_GOP_OPERATOR             2
#define DFAULT_BUF_SIZE_OPERATOR            2
#define FOUR_BYTE_PIX_BUF_SIZE_OPERATOR     4
#define PACKET_SIZE_MULTI                   3
#define PACKET_SIZE_OPERATOR                2
#define DEFAULT_RESOLUTION_SIZE             (1920 * 1088)
#define DEFAULT_PACKET_BUFFER_NUM           4
#define DEFAULT_FRAME_BUFFER_NUM            2

#define DEFAULT_REF_FRAME_NUM               8
#define LEVEL_5P1_MAX_DPB_MBS               184320
#define REF_FRAME_DIVISOR                   16
#define MAX_REF_FRAME_NUM                   16
#define MIN_REF_FRAME_NUM                   6
#define WIDTH_OF_4K_VIDEO                   4096
#define HEIGHT_OF_4K_HEVC                   2160
#define HEIGHT_OF_4K_VP9                    2176
#define MAX_REF_FRAME_NUM_VP9               8
#define MIN_REF_FRAME_NUM_VP9               4
#define BASIC_DEC_FRAME_BUFFER_NUM          5

static const CodecPixelFormatConvertTbl pixelFormatSupportted[] = {
    {PIXEL_FMT_RGBA_8888, MPP_FMT_RGBA8888, RK_FORMAT_RGBA_8888},
    {PIXEL_FMT_BGRA_8888, MPP_FMT_BGRA8888, RK_FORMAT_BGRA_8888},
    {PIXEL_FMT_YCBCR_420_SP, MPP_FMT_YUV420SP, RK_FORMAT_YCbCr_420_SP },
    {PIXEL_FMT_YCBCR_420_P, MPP_FMT_YUV420P, RK_FORMAT_YCbCr_420_P},
};

int32_t InitMppConfig(RKHdiBaseComponent *pBaseComponent)
{
    int32_t ret = 0;
    if (pBaseComponent->ctxType == MPP_CTX_ENC) {
        ret = pBaseComponent->mppApi->HdiMppEncCfgInit(&pBaseComponent->cfg);
    } else if (pBaseComponent->ctxType == MPP_CTX_DEC) {
        ret = pBaseComponent->mppApi->HdiMppDecCfgInit(&pBaseComponent->cfg);
    }
    return ret;
}

int32_t DeinitMppConfig(RKHdiBaseComponent *pBaseComponent)
{
    int32_t ret = 0;
    if (pBaseComponent->cfg == NULL) {
        return ret;
    }
    
    if (pBaseComponent->ctxType == MPP_CTX_ENC) {
        ret = pBaseComponent->mppApi->HdiMppEncCfgDeinit(pBaseComponent->cfg);
    } else if (pBaseComponent->ctxType == MPP_CTX_DEC) {
        ret = pBaseComponent->mppApi->HdiMppDecCfgDeinit(pBaseComponent->cfg);
    }
    pBaseComponent->cfg = NULL;
    return ret;
}

void SetDefaultFps(RKHdiEncodeSetup *setup)
{
    setup->fps.fpsInFlex = 0;
    setup->fps.fpsInNum = FPS_SETUP_DEFAULT;
    setup->fps.fpsOutNum = FPS_SETUP_DEFAULT;
    setup->fps.fpsInDen = 1;
    setup->fps.fpsOutDen = 1;
    setup->fps.fpsOutFlex = 0;
}

void SetDefaultDropMode(RKHdiEncodeSetup *setup)
{
    setup->drop.dropMode = MPP_ENC_RC_DROP_FRM_DISABLED;
    setup->drop.dropThd = DFAULT_ENC_DROP_THD;      /* 20% of max bps */
    setup->drop.dropGap = 1;                        /* Do not continuous drop frame */
}

void SetDefaultGopMode(RKHdiEncodeSetup *setup)
{
    setup->gop.gopMode = VID_CODEC_GOPMODE_INVALID;
    setup->gop.gopLen = 0;
    setup->gop.viLen = 0;
    setup->gop.gop = setup->fps.fpsOutNum * DFAULT_ENC_GOP_OPERATOR;
}

static int32_t GetDefaultConfig(RKHdiBaseComponent *pBaseComponent)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = pBaseComponent->ctx;

    if (pBaseComponent->ctxType == MPP_CTX_ENC) {
        ret = pBaseComponent->mpi->control(ctx, MPP_ENC_GET_CFG, pBaseComponent->cfg);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: config MPP_ENC_GET_CFG failed, ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    } else if (pBaseComponent->ctxType == MPP_CTX_ENC) {
        ret = pBaseComponent->mpi->control(ctx, MPP_DEC_GET_CFG, pBaseComponent->cfg);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: config MPP_DEC_GET_CFG failed, ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static PixelFormat ConvertRKFormat2HdiFormat(MppFrameFormat fmtRK)
{
    if (fmtRK == MPP_FMT_YUV420SP) {
        return PIXEL_FMT_YCBCR_420_SP;
    } else {
        return PIXEL_FMT_BUTT;
    }
}

MppFrameFormat ConvertHdiFormat2RKFormat(PixelFormat fmtHDI)
{
    MppFrameFormat mppFmt = MPP_FMT_BUTT;
    int32_t count = sizeof(pixelFormatSupportted) / sizeof(CodecPixelFormatConvertTbl);
    for (int i = 0; i < count; i++) {
        if (pixelFormatSupportted[i].pixFormat == fmtHDI) {
            mppFmt = pixelFormatSupportted[i].mppFormat;
            break;
        }
    }
    if (mppFmt == MPP_FMT_BUTT) {
        HDF_LOGE("%{public}s: do not support type %{public}d", __func__, fmtHDI);
    }
    return mppFmt;
}

RgaSURF_FORMAT ConvertHdiFormat2RgaFormat(PixelFormat fmtHDI)
{
    RgaSURF_FORMAT rgaFmt = RK_FORMAT_UNKNOWN;
    int32_t count = sizeof(pixelFormatSupportted) / sizeof(CodecPixelFormatConvertTbl);
    for (int i = 0; i < count; i++) {
        if (pixelFormatSupportted[i].pixFormat == fmtHDI) {
            rgaFmt = pixelFormatSupportted[i].rgaFormat;
            break;
        }
    }
    if (rgaFmt == RK_FORMAT_UNKNOWN) {
        HDF_LOGE("%{public}s: do not support type %{public}d", __func__, fmtHDI);
    }
    return rgaFmt;
}

RgaSURF_FORMAT ConvertMppFormat2RgaFormat(MppFrameFormat mppFmt)
{
    RgaSURF_FORMAT rgaFmt = RK_FORMAT_UNKNOWN;
    int32_t count = sizeof(pixelFormatSupportted) / sizeof(CodecPixelFormatConvertTbl);
    for (int i = 0; i < count; i++) {
        if (pixelFormatSupportted[i].mppFormat == mppFmt) {
            rgaFmt = pixelFormatSupportted[i].rgaFormat;
            break;
        }
    }
    if (rgaFmt == RK_FORMAT_UNKNOWN) {
        HDF_LOGE("%{public}s: do not support type %{public}d", __func__, mppFmt);
    }
    return rgaFmt;
}

static MppEncRcMode ConvertHdiRcMode2RKRcMode(VideoCodecRcMode hdiRcMode)
{
    MppEncRcMode rkRcMode = MPP_ENC_RC_MODE_VBR;
    switch (hdiRcMode) {
        case VID_CODEC_RC_CBR:
            rkRcMode = MPP_ENC_RC_MODE_CBR;
            break;
        case VID_CODEC_RC_VBR:
            rkRcMode = MPP_ENC_RC_MODE_VBR;
            break;
        case VID_CODEC_RC_AVBR:
            rkRcMode = MPP_ENC_RC_MODE_AVBR;
            break;
        case VID_CODEC_RC_FIXQP:
            rkRcMode = MPP_ENC_RC_MODE_FIXQP;
            break;
        default:
            break;
    }
    return rkRcMode;
}

static MppEncRcGopMode ConvertHdiGopMode2RKGopMode(VideoCodecGopMode hdiGopMode)
{
    MppEncRcGopMode rkGopMode = MPP_ENC_RC_NORMAL_P;
    switch (hdiGopMode) {
        case VID_CODEC_GOPMODE_NORMALP:
            rkGopMode = MPP_ENC_RC_NORMAL_P;
            break;
        case VID_CODEC_GOPMODE_SMARTP:
            rkGopMode = MPP_ENC_RC_SMART_P;
            break;
        default:
            break;
    }
    return rkGopMode;
}

static MppCodingType ConvertHdiMimeCodecType2RKCodecType(AvCodecMime mime)
{
    MppCodingType type = MPP_VIDEO_CodingMax;
    switch (mime) {
        case MEDIA_MIMETYPE_IMAGE_JPEG:
            type = MPP_VIDEO_CodingMJPEG;
            break;
        case MEDIA_MIMETYPE_VIDEO_AVC:
            type = MPP_VIDEO_CodingAVC;
            break;
        case MEDIA_MIMETYPE_VIDEO_HEVC:
            type = MPP_VIDEO_CodingHEVC;
            break;
        default:
            break;
    }
    if (type == MPP_VIDEO_CodingMax) {
        HDF_LOGE("%{public}s: do not support type %{public}d", __func__, type);
    }
    return type;
}

static int32_t GetStrideByWidth(int32_t width, MppFrameFormat fmt)
{
    int32_t stride = 0;

    switch (fmt & MPP_FRAME_FMT_MASK) {
        case MPP_FMT_YUV420SP:
        case MPP_FMT_YUV420SP_VU:
            stride = MPP_ALIGN(width, MPP_ALIGN_STRIDE_WITH_EIGHT);
            break;
        case MPP_FMT_YUV420P:
            /* NOTE: 420P need to align to 16 so chroma can align to 8 */
            stride = MPP_ALIGN(width, MPP_ALIGN_STRIDE_WITH_SIXTEEN);
            break;
        case MPP_FMT_YUV422P:
        case MPP_FMT_YUV422SP:
        case MPP_FMT_YUV422SP_VU:
            /* NOTE: 422 need to align to 8 so chroma can align to 16 */
            stride = MPP_ALIGN(width, MPP_ALIGN_STRIDE_WITH_EIGHT);
            break;
        case MPP_FMT_RGB565:
        case MPP_FMT_BGR565:
        case MPP_FMT_RGB555:
        case MPP_FMT_BGR555:
        case MPP_FMT_RGB444:
        case MPP_FMT_BGR444:
        case MPP_FMT_YUV422_YUYV:
        case MPP_FMT_YUV422_YVYU:
        case MPP_FMT_YUV422_UYVY:
        case MPP_FMT_YUV422_VYUY:
            /* NOTE: for vepu limitation */
            stride = MPP_ALIGN(width, MPP_ALIGN_STRIDE_WITH_EIGHT) * MPP_ALIGN_MULTIPLE_WITH_TWO;
            break;
        case MPP_FMT_RGB888:
        case MPP_FMT_BGR888:
            /* NOTE: for vepu limitation */
            stride = MPP_ALIGN(width, MPP_ALIGN_STRIDE_WITH_EIGHT) * MPP_ALIGN_MULTIPLE_WITH_THREE;
            break;
        case MPP_FMT_RGB101010:
        case MPP_FMT_BGR101010:
        case MPP_FMT_ARGB8888:
        case MPP_FMT_ABGR8888:
        case MPP_FMT_BGRA8888:
        case MPP_FMT_RGBA8888:
            /* NOTE: for vepu limitation */
            stride = MPP_ALIGN(width, MPP_ALIGN_STRIDE_WITH_EIGHT) * MPP_ALIGN_MULTIPLE_WITH_FOUR;
            break;
        default :
            break;
    }

    return stride;
}

int32_t GetDefaultHorStride(int32_t width, PixelFormat fmtHDI)
{
    MppFrameFormat fmt = ConvertHdiFormat2RKFormat(fmtHDI);
    return GetStrideByWidth(width, fmt);
}

static uint32_t GetMppFrameBufferSize(RKHdiBaseComponent *pBaseComponent)
{
    uint32_t frameSize = 0;
    switch (pBaseComponent->fmt) {
        case MPP_FMT_YUV420SP:
        case MPP_FMT_YUV420P:
            frameSize = MPP_ALIGN(pBaseComponent->horStride, MPP_ALIGN_STRIDE_WITH_SIXTY_FOUR)
                * MPP_ALIGN(pBaseComponent->verStride, MPP_ALIGN_STRIDE_WITH_SIXTY_FOUR)
                * MPP_ALIGN_MULTIPLE_WITH_THREE / MPP_ALIGN_DIVISOR_WITH_TWO;
            break;

        case MPP_FMT_YUV422_YUYV:
        case MPP_FMT_YUV422_YVYU:
        case MPP_FMT_YUV422_UYVY:
        case MPP_FMT_YUV422_VYUY:
        case MPP_FMT_YUV422P:
        case MPP_FMT_YUV422SP:
        case MPP_FMT_RGB444:
        case MPP_FMT_BGR444:
        case MPP_FMT_RGB555:
        case MPP_FMT_BGR555:
        case MPP_FMT_RGB565:
        case MPP_FMT_BGR565:
            frameSize = MPP_ALIGN(pBaseComponent->horStride, MPP_ALIGN_STRIDE_WITH_SIXTY_FOUR)
                * MPP_ALIGN(pBaseComponent->verStride, MPP_ALIGN_STRIDE_WITH_SIXTY_FOUR)
                * MPP_ALIGN_MULTIPLE_WITH_TWO;
            break;

        default:
            frameSize = MPP_ALIGN(pBaseComponent->horStride, MPP_ALIGN_STRIDE_WITH_SIXTY_FOUR)
                * MPP_ALIGN(pBaseComponent->verStride, MPP_ALIGN_STRIDE_WITH_SIXTY_FOUR)
                * MPP_ALIGN_MULTIPLE_WITH_FOUR;
            break;
    }
    return frameSize;
}

static uint32_t GetMppFrameHeaderSize(RKHdiBaseComponent *pBaseComponent)
{
    uint32_t headerSize = 0;
    if (MPP_FRAME_FMT_IS_FBC(pBaseComponent->fmt)) {
        headerSize = MPP_ALIGN(MPP_ALIGN(pBaseComponent->setup.width, MPP_ALIGN_STRIDE_WITH_SIXTEEN)
            * MPP_ALIGN(pBaseComponent->setup.height, MPP_ALIGN_STRIDE_WITH_SIXTEEN)
            / MPP_ALIGN_DIVISOR_WITH_SIXTEEN, SZ_4K);
    } else {
        headerSize = 0;
    }
    return headerSize;
}

static int32_t GetMppBuffers(RKHdiBaseComponent *pBaseComponent)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = pBaseComponent->mppApi;
    ret = mppApi->HdiMppBufferGroupGet(&pBaseComponent->frmGrp,
        MPP_BUFFER_TYPE_DRM, MPP_BUFFER_INTERNAL, NULL, __func__);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: failed to get mpp buffer group ret %{public}d", __func__, ret);
        return HDF_FAILURE;
    }

    ret = mppApi->HdiMppBufferGetWithTag(pBaseComponent->frmGrp, &pBaseComponent->frmBuf,
        pBaseComponent->frameSize + pBaseComponent->headerSize, MODULE_TAG, __func__);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: failed to get buffer for input frame ret %{public}d", __func__, ret);
        return HDF_FAILURE;
    }

    ret = mppApi->HdiMppBufferGetWithTag(pBaseComponent->frmGrp, &pBaseComponent->pktBuf,
        pBaseComponent->frameSize, MODULE_TAG, __func__);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: failed to get buffer for output packet ret %{public}d", __func__, ret);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void SetQpValue(RKHdiEncodeSetup *encSetup, MppEncRcMode mppRcMode)
{
    switch (mppRcMode) {
        case MPP_ENC_RC_MODE_FIXQP: {
            encSetup->rc.qpInit = FIXQP_INIT_VALUE;
            encSetup->rc.qpMax = FIXQP_MAX_VALUE;
            encSetup->rc.qpMin = FIXQP_MIN_VALUE;
            encSetup->rc.qpMaxI = FIXQP_MAX_I_VALUE;
            encSetup->rc.qpMinI = FIXQP_MIN_I_VALUE;
            encSetup->rc.qpIp = FIXQP_IP_VALUE;
            break;
        }
        case MPP_ENC_RC_MODE_CBR:
        case MPP_ENC_RC_MODE_VBR:
        case MPP_ENC_RC_MODE_AVBR: {
            encSetup->rc.qpInit = OTHER_QP_INIT_VALUE;
            encSetup->rc.qpMax = OTHER_QP_MAX_VALUE;
            encSetup->rc.qpMin = OTHER_QP_MIN_VALUE;
            encSetup->rc.qpMaxI = OTHER_QP_MAX_I_VALUE;
            encSetup->rc.qpMinI = OTHER_QP_MIN_I_VALUE;
            encSetup->rc.qpIp = OTHER_QP_IP_VALUE;
            break;
        }
        default: {
            HDF_LOGE("%{public}s: not handled or unsupported encoder rc mode %{public}d", __func__, mppRcMode);
            break;
        }
    }
}

static void CalcBpsValue(RKHdiBaseComponent *pBaseComponent, MppEncRcMode mppRcMode)
{
    RKHdiEncodeSetup *encSetup = &pBaseComponent->setup;
    encSetup->rc.bpsTarget = encSetup->width * encSetup->height * BPS_TARGET / BPS_BASE *
        (encSetup->fps.fpsOutNum / encSetup->fps.fpsOutDen);

    switch (mppRcMode) {
        case MPP_ENC_RC_MODE_FIXQP: {
            /* do not setup bitrate on FIXQP mode */
            break;
        }
        case MPP_ENC_RC_MODE_CBR: {
            /* CBR mode has narrow bound */
            encSetup->rc.bpsMax = encSetup->rc.bpsTarget * BPS_MAX / BPS_BASE;
            encSetup->rc.bpsMin = encSetup->rc.bpsTarget * BPS_MEDIUM / BPS_BASE;
            break;
        }
        case MPP_ENC_RC_MODE_VBR:
        case MPP_ENC_RC_MODE_AVBR: {
            /* VBR mode has wide bound */
            encSetup->rc.bpsMax = encSetup->rc.bpsTarget * BPS_MAX / BPS_BASE;
            encSetup->rc.bpsMin = encSetup->rc.bpsTarget * BPS_MIN / BPS_BASE;
            break;
        }
        default: {
            /* default use CBR mode */
            encSetup->rc.bpsMax = encSetup->rc.bpsTarget * BPS_MAX / BPS_BASE;
            encSetup->rc.bpsMin = encSetup->rc.bpsTarget * BPS_MEDIUM / BPS_BASE;
            break;
        }
    }
    /* setup qp for different codec and rc_mode */
    switch (pBaseComponent->codingType) {
        case MPP_VIDEO_CodingAVC:
        case MPP_VIDEO_CodingHEVC: {
            SetQpValue(encSetup, mppRcMode);
            break;
        }
        default: {
            break;
        }
    }
}

static void SetMimeSettings(RKHdiCodecMimeSetup *codecMimeSet, MppCodingType codingType)
{
    switch (codingType) {
        case MPP_VIDEO_CodingAVC: {
            /*
            * H.264 profile_idc parameter
            * 66  - Baseline profile
            * 77  - Main profile
            * 100 - High profile
            */
            codecMimeSet->avcSetup.profile = AVC_SETUP_PROFILE_DEFAULT;
            /*
            * H.264 level_idc parameter
            * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
            * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
            * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
            * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
            * 50 / 51 / 52         - 4K@30fps
            */
            codecMimeSet->avcSetup.level = AVC_SETUP_LEVEL_DEFAULT;
            codecMimeSet->avcSetup.cabacEn = AVC_SETUP_CABAC_EN_DEFAULT;
            codecMimeSet->avcSetup.cabacIdc = AVC_SETUP_CABAC_IDC_DEFAULT;
            codecMimeSet->avcSetup.trans8x8 = AVC_SETUP_TRANS_DEFAULT;
            break;
        }
        case MPP_VIDEO_CodingHEVC: {
            break;
        }
        default: {
            break;
        }
    }
}

static int32_t SetGop(RKHdiBaseComponent *pBaseComponent)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = pBaseComponent->mppApi;
    RKHdiGopSetup *gopSet = &pBaseComponent->setup.gop;

    gopSet->gop = gopSet->gopLen ? gopSet->gopLen : pBaseComponent->setup.fps.fpsOutNum * DFAULT_ENC_GOP_OPERATOR;
    ret = mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:gop", gopSet->gop);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: config mpp set gop failed! ret: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }

    RK_U32 gopMode = 0;
    RK_U32 defaultGopMode = ConvertHdiGopMode2RKGopMode(gopSet->gopMode);
    mppApi->HdiMppEnvGetU32("gop_mode", &gopMode, defaultGopMode);
    if (gopMode) {
        MppEncRefCfg ref = NULL;
        mppApi->HdiMppEncRefCfgInit(&ref);
        if (gopMode < GOP_MODE_THRESHOLD_VALUE) {
            mppApi->HdiMppEncGenRefCfg(ref, gopMode);
        } else {
            mppApi->HdiMppEncGenSmartGopRefCfg(ref, gopSet->gopLen, gopSet->viLen);
        }
        ret = pBaseComponent->mpi->control(pBaseComponent->ctx, MPP_ENC_SET_REF_CFG, ref);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: mpi control enc set ref cfg failed, ret %{public}d", __func__, ret);
            mppApi->HdiMppEncRefCfgDeinit(&ref);
            return HDF_FAILURE;
        }
        mppApi->HdiMppEncRefCfgDeinit(&ref);
    }
    return HDF_SUCCESS;
}

int32_t SetEncCfg(RKHdiBaseComponent *pBaseComponent)
{
    RKMppApi *mppApi = pBaseComponent->mppApi;
    RKHdiEncodeSetup *setup = &pBaseComponent->setup;
    GetDefaultConfig(pBaseComponent);
    
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "prep:width", setup->width);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "prep:height", setup->height);
    
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "prep:format", pBaseComponent->fmt);

    pBaseComponent->horStride = GetStrideByWidth(setup->width, pBaseComponent->fmt);
    pBaseComponent->verStride = setup->height;
    if (pBaseComponent->horStride == 0 || pBaseComponent->verStride == 0) {
        HDF_LOGE("%{public}s: size or stride incorrect!", __func__);
        return HDF_FAILURE;
    }
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "prep:hor_stride", pBaseComponent->horStride);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "prep:ver_stride", pBaseComponent->verStride);
    
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:fps_in_flex", setup->fps.fpsInFlex);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:fps_in_num", setup->fps.fpsInNum);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:fps_in_denorm", setup->fps.fpsInDen);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:fps_out_flex", setup->fps.fpsOutFlex);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:fps_out_num", setup->fps.fpsOutNum);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:fps_out_denorm", setup->fps.fpsOutDen);
    
    MppEncRcMode mppRcMode = ConvertHdiRcMode2RKRcMode(setup->rc.rcMode);
    CalcBpsValue(pBaseComponent, mppRcMode);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:mode", mppRcMode);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:bps_target", setup->rc.bpsTarget);
    if (mppRcMode != MPP_ENC_RC_MODE_FIXQP) {
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:bps_max", setup->rc.bpsMax);
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:bps_min", setup->rc.bpsMin);
    }

    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:qp_init", setup->rc.qpInit);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:qp_max", setup->rc.qpMax);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:qp_min", setup->rc.qpMin);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:qp_max_i", setup->rc.qpMaxI);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:qp_min_i", setup->rc.qpMinI);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "rc:qp_ip", setup->rc.qpIp);

    mppApi->HdiMppEncCfgSetU32(pBaseComponent->cfg, "rc:drop_mode", setup->drop.dropMode);
    mppApi->HdiMppEncCfgSetU32(pBaseComponent->cfg, "rc:drop_thd", setup->drop.dropThd);
    mppApi->HdiMppEncCfgSetU32(pBaseComponent->cfg, "rc:drop_gap", setup->drop.dropGap);
    
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "codec:type", pBaseComponent->codingType);
    SetMimeSettings(&setup->codecMime, pBaseComponent->codingType);
    if (pBaseComponent->codingType == MPP_VIDEO_CodingAVC) {
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "h264:profile", setup->codecMime.avcSetup.profile);
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "h264:level", setup->codecMime.avcSetup.level);
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "h264:cabac_en", setup->codecMime.avcSetup.cabacEn);
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "h264:cabac_idc", setup->codecMime.avcSetup.cabacIdc);
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "h264:trans8x8", setup->codecMime.avcSetup.trans8x8);
    }
    SetGop(pBaseComponent);
    ValidateEncSetup(pBaseComponent, NULL);
    return HDF_SUCCESS;
}

int32_t SetDecCfg(RKHdiBaseComponent *pBaseComponent)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = pBaseComponent->mppApi;
    MppCtx ctx = pBaseComponent->ctx;

    GetDefaultConfig(pBaseComponent);
    mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "codec:type", pBaseComponent->codingType);
    mppApi->HdiMppDecCfgSetU32(pBaseComponent->cfg, "base:split_parse", pBaseComponent->setup.split);
    ret = pBaseComponent->mpi->control(ctx, MPP_DEC_SET_CFG, pBaseComponent->cfg);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: config MPP_DEC_SET_CFG failed, ret %{public}d", __func__, ret);
        UINTPTR userData = NULL;
        EventType event = EVENT_ERROR;
        uint32_t length = 0;
        int32_t *eventData = NULL;

        pBaseComponent->pCallbacks->OnEvent(userData, event, length, eventData);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t SetParamWidth(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.width = *(RK_S32 *)param->val;
    if (pBaseComponent->setup.width <= 0) {
        HDF_LOGE("%{public}s: invalid width value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t SetParamHeight(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.height = *(RK_S32 *)param->val;
    if (pBaseComponent->setup.height <= 0) {
        HDF_LOGE("%{public}s: invalid height value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t SetParamPixelFmt(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    RK_S32 *hdiPixFmt = (RK_S32 *)param->val;
    MppFrameFormat mppPixFmt = ConvertHdiFormat2RKFormat(*hdiPixFmt);
    if (mppPixFmt == MPP_FMT_BUTT) {
        HDF_LOGE("%{public}s: invalid pixel format value!", __func__);
        return HDF_FAILURE;
    }
    pBaseComponent->setup.fmt = *hdiPixFmt;
    pBaseComponent->fmt = MPP_FMT_YUV420SP;
    return HDF_SUCCESS;
}

int32_t SetParamStride(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.stride.horStride = *(RK_S32 *)param->val;
    if (pBaseComponent->setup.stride.horStride <= 0) {
        HDF_LOGE("%{public}s: invalid stride value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t SetParamFps(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.fps.fpsInNum = *(RK_S32 *)param->val;
    pBaseComponent->setup.fps.fpsOutNum = *(RK_S32 *)param->val;
    if (pBaseComponent->setup.fps.fpsInNum <= 0) {
        HDF_LOGE("%{public}s: invalid fps value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t SetParamDrop(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.drop.dropMode = *(RK_U32 *)param->val;
    if (pBaseComponent->setup.drop.dropMode < 0 || pBaseComponent->setup.drop.dropMode >= MPP_ENC_RC_DROP_FRM_BUTT) {
        HDF_LOGE("%{public}s: drop stride value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t SetParamRateControl(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.rc.rcMode = *(RK_S32 *)param->val;
    if (pBaseComponent->setup.rc.rcMode < 0) {
        HDF_LOGE("%{public}s: invalid rcMode value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t SetParamGop(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.gop.gopMode = *(VideoCodecGopMode *)param->val;
    if (pBaseComponent->setup.gop.gopMode < 0 || pBaseComponent->setup.gop.gopMode >= VID_CODEC_GOPMODE_INVALID) {
        HDF_LOGE("%{public}s: invalid gop value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t SetParamMimeCodecType(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    RKHdiCodecMimeSetup *mimeCodecTypeSet = &pBaseComponent->setup.codecMime;
    mimeCodecTypeSet->mimeCodecType = *(RK_S32 *)param->val;
    if (mimeCodecTypeSet->mimeCodecType < 0 || mimeCodecTypeSet->mimeCodecType >= MEDIA_MIMETYPE_INVALID) {
        HDF_LOGE("%{public}s: invalid mime type value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    MppCodingType codingType = ConvertHdiMimeCodecType2RKCodecType(mimeCodecTypeSet->mimeCodecType);
    if (codingType == MPP_VIDEO_CodingMax) {
        return HDF_ERR_INVALID_PARAM;
    }
    pBaseComponent->codingType = codingType;
    return HDF_SUCCESS;
}

int32_t SetParamCodecType(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.codecType = *(RK_S32 *)param->val;
    if (pBaseComponent->setup.codecType < 0 || pBaseComponent->setup.codecType >= INVALID_TYPE) {
        HDF_LOGE("%{public}s: invalid codec type value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t SetParamSplitParse(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->setup.split = *(RK_U32 *)param->val;
    return HDF_SUCCESS;
}

int32_t SetParamCodecFrameNum(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    pBaseComponent->frameNum = *(RK_S32 *)param->val;
    if (pBaseComponent->frameNum < 0) {
        HDF_LOGE("%{public}s: invalid frame num value!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    return HDF_SUCCESS;
}

int32_t CheckSetupStride(RKHdiBaseComponent *pBaseComponent)
{
    if (pBaseComponent->setup.width <= 0 || pBaseComponent->setup.height <= 0) {
        HDF_LOGE("%{public}s: width or height invalid!", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (pBaseComponent->setup.stride.horStride == 0) {
        pBaseComponent->setup.stride.horStride = MPP_ALIGN(pBaseComponent->setup.width, MPP_ALIGN_STRIDE_WITH_SIXTEEN);
    }
    if (pBaseComponent->setup.stride.verStride == 0) {
        pBaseComponent->setup.stride.verStride = pBaseComponent->setup.height;
    }
    return HDF_SUCCESS;
}

int32_t ValidateEncSetup(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = pBaseComponent->mppApi;
    RK_U32 splitMode = 0;
    RK_U32 splitArg = 0;

    int32_t checkResult = CheckSetupStride(pBaseComponent);
    if (checkResult != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: CheckSetupStride failed!", __func__);
        return checkResult;
    }

    mppApi->HdiMppEnvGetU32("split_mode", &splitMode, MPP_ENC_SPLIT_NONE);
    mppApi->HdiMppEnvGetU32("split_arg", &splitArg, 0);
    if (splitMode) {
        HDF_LOGI("%{public}s: %{public}p splitMode %{public}d splitArg %{public}d",
            __func__, pBaseComponent->ctx, splitMode, splitArg);
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "split:mode", splitMode);
        mppApi->HdiMppEncCfgSetS32(pBaseComponent->cfg, "split:arg", splitArg);
    }

    ret = pBaseComponent->mpi->control(pBaseComponent->ctx, MPP_ENC_SET_CFG, pBaseComponent->cfg);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpi control enc set cfg failed ret %{public}d", __func__, ret);
        return HDF_FAILURE;
    }

    /* optional */
    MppEncSeiMode seiMode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = pBaseComponent->mpi->control(pBaseComponent->ctx, MPP_ENC_SET_SEI_CFG, &seiMode);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpi control enc set sei cfg failed ret %{public}d", __func__, ret);
        return HDF_FAILURE;
    }

    if ((pBaseComponent->codingType == MPP_VIDEO_CodingAVC) || (pBaseComponent->codingType == MPP_VIDEO_CodingHEVC)) {
        MppEncHeaderMode headerMode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = pBaseComponent->mpi->control(pBaseComponent->ctx, MPP_ENC_SET_HEADER_MODE, &headerMode);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: mpi control enc set header mode failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    }

    pBaseComponent->frameSize = GetMppFrameBufferSize(pBaseComponent);
    pBaseComponent->headerSize = GetMppFrameHeaderSize(pBaseComponent);
    ret = GetMppBuffers(pBaseComponent);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: GetMppBuffers failed ret %{public}d", __func__, ret);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

uint32_t CalculateTotalDecRefFrames(RKHdiBaseComponent *pBaseComponent)
{
    uint32_t refFramesNum = DEFAULT_REF_FRAME_NUM;
    int32_t width = pBaseComponent->setup.width;
    int32_t height = pBaseComponent->setup.height;

    if (width == 0 || height == 0 || pBaseComponent->codingType < 0 ||
        pBaseComponent->codingType >= MPP_VIDEO_CodingMax) {
        HDF_LOGE("%{public}s: invalid parameters", __func__);
        return refFramesNum;
    }
    switch (pBaseComponent->codingType) {
        case MPP_VIDEO_CodingAVC:
            /* used level 5.1 MaxDpbMbs to compute ref num */
            refFramesNum = LEVEL_5P1_MAX_DPB_MBS / ((width / REF_FRAME_DIVISOR) * (height / REF_FRAME_DIVISOR));
            if (refFramesNum > MAX_REF_FRAME_NUM) {
                /* max ref frame number is 16 */
                refFramesNum = MAX_REF_FRAME_NUM;
            } else if (refFramesNum < MIN_REF_FRAME_NUM) {
                /* min ref frame number is 6 */
                refFramesNum = MIN_REF_FRAME_NUM;
            }
            break;
        case MPP_VIDEO_CodingHEVC:
            /* use 4K resolution to compute ref num */
            refFramesNum = WIDTH_OF_4K_VIDEO * HEIGHT_OF_4K_HEVC * MIN_REF_FRAME_NUM / (width * height);
            if (refFramesNum > MAX_REF_FRAME_NUM) {
                /* max ref frame number is 16 */
                refFramesNum = MAX_REF_FRAME_NUM;
            } else if (refFramesNum < MIN_REF_FRAME_NUM) {
                /* min ref frame number is 6 */
                refFramesNum = MIN_REF_FRAME_NUM;
            }
            break;
        case MPP_VIDEO_CodingVP9:
            refFramesNum = WIDTH_OF_4K_VIDEO * HEIGHT_OF_4K_VP9 * MIN_REF_FRAME_NUM_VP9 / (width * height);
            if (refFramesNum > MAX_REF_FRAME_NUM_VP9) {
                /* max ref frame number is 8 */
                refFramesNum = MAX_REF_FRAME_NUM_VP9;
            } else if (refFramesNum < MIN_REF_FRAME_NUM_VP9) {
                /* min ref frame number is 4 */
                refFramesNum = MIN_REF_FRAME_NUM_VP9;
            }
            break;
        default:
            refFramesNum = DEFAULT_REF_FRAME_NUM;
            break;
    }

    return refFramesNum;
}

int32_t GetParamInputBufferCount(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_U32);
        param->val = malloc(param->size);
    }

    RK_U32 inputBufferCount = 0;
    if (pBaseComponent->ctxType == MPP_CTX_ENC) {
        inputBufferCount = DEFAULT_FRAME_BUFFER_NUM;
    } else {
        inputBufferCount = DEFAULT_PACKET_BUFFER_NUM;
    }
    int32_t ret = memcpy_s(param->val, param->size, &inputBufferCount, sizeof(RK_U32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamOutputBufferCount(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_U32);
        param->val = malloc(param->size);
    }

    RK_U32 outputBufferCount = 0;
    if (pBaseComponent->ctxType == MPP_CTX_ENC) {
        outputBufferCount = DEFAULT_PACKET_BUFFER_NUM;
    } else {
        RK_U32 refFramesNum = CalculateTotalDecRefFrames(pBaseComponent);
        outputBufferCount = refFramesNum + BASIC_DEC_FRAME_BUFFER_NUM;
    }
    int32_t ret = memcpy_s(param->val, param->size, &outputBufferCount, sizeof(RK_U32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamBufferSize(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_U32);
        param->val = malloc(param->size);
    }

    PixelFormat format = pBaseComponent->setup.fmt;
    if (format == PIXEL_FMT_BUTT) {
        HDF_LOGE("%{public}s: pixel format not set!", __func__);
        return HDF_FAILURE;
    }
    
    RK_U32 bufferSize = pBaseComponent->setup.width * pBaseComponent->setup.height;
    if (bufferSize == 0) {
        bufferSize = DEFAULT_RESOLUTION_SIZE;
    }
    if (format == PIXEL_FMT_YCBCR_420_SP || format == PIXEL_FMT_YCBCR_420_P) {
        bufferSize = (bufferSize * PACKET_SIZE_MULTI) / PACKET_SIZE_OPERATOR;
    } else if (format == PIXEL_FMT_BGRA_8888 || format == PIXEL_FMT_RGBA_8888) {
        bufferSize = bufferSize * FOUR_BYTE_PIX_BUF_SIZE_OPERATOR;
    } else {
        bufferSize = bufferSize * DFAULT_BUF_SIZE_OPERATOR;
    }
    int32_t ret = memcpy_s(param->val, param->size, &bufferSize, sizeof(RK_U32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamWidth(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.width, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamHeight(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.height, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamDecOutputPixelFmt(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(PixelFormat);
        param->val = malloc(param->size);
    }

    PixelFormat format = pBaseComponent->setup.fmt;
    if (format == PIXEL_FMT_BUTT) {
        // Rk mpp only support NV12 output (rga can be used for other formats)
        format = PIXEL_FMT_YCBCR_420_SP;
        pBaseComponent->setup.fmt = PIXEL_FMT_YCBCR_420_SP;
        pBaseComponent->fmt = MPP_FMT_YUV420SP;
    }
    int32_t ret = memcpy_s(param->val, param->size, &format, sizeof(PixelFormat));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamEncInputPixleFmt(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(PixelFormat);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.fmt, sizeof(PixelFormat));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamPixleFmt(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (pBaseComponent->setup.codecType == VIDEO_DECODER) {
        return GetParamDecOutputPixelFmt(pBaseComponent, param);
    } else if (pBaseComponent->setup.codecType == VIDEO_ENCODER) {
        return GetParamEncInputPixleFmt(pBaseComponent, param);
    } else {
        HDF_LOGE("%{public}s: codec type invalid!", __func__);
        return HDF_FAILURE;
    }
}

int32_t GetParamStride(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.stride.horStride, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamFps(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.fps.fpsInNum, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamRateControl(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.rc.rcMode, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamGop(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.gop.gopMode, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamMimeCodecType(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.codecMime.mimeCodecType, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamCodecType(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.codecType, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamSplitParse(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_U32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.split, sizeof(RK_U32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamCodecFrameNum(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->frameNum, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t GetParamDrop(RKHdiBaseComponent *pBaseComponent, Param *param)
{
    if (param->val == NULL) {
        param->size = sizeof(RK_S32);
        param->val = malloc(param->size);
    }
    int32_t ret = memcpy_s(param->val, param->size, &pBaseComponent->setup.drop.dropMode, sizeof(RK_S32));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: copy data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}
