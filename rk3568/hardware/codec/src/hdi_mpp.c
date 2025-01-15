/*
 * Copyright (c) 2022 Shenzhen Kaihong DID Co., Ltd.
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

#include "hdi_mpp.h"
#include <dlfcn.h>
#include <securec.h>
#include <hdf_base.h>
#include <hdf_log.h>
#include "hdi_mpp_config.h"
#include "im2d.h"
#include "rga.h"
#include "rk_vdec_cfg.h"

#define HDF_LOG_TAG codec_hdi_mpp
#define BITWISE_LEFT_SHIFT_WITH_ONE     (1 << 20)
#define SLEEP_INTERVAL_MICROSECONDS     1000
#define BUFFER_GROUP_LIMIT_NUM          24
#define DFAULT_ENC_FPS_NUM              24
#define DFAULT_ENC_DROP_THD             20
#define DFAULT_ENC_GOP_OPERATOR         2

RKHdiBaseComponent *g_pBaseComponent = NULL;

void InitComponentSetup(RKHdiEncodeSetup *setup)
{
    setup->fmt = PIXEL_FORMAT_NONE;

    setup->fps.fpsInNum = DFAULT_ENC_FPS_NUM;
    setup->fps.fpsInDen = 1;
    setup->fps.fpsOutNum = DFAULT_ENC_FPS_NUM;
    setup->fps.fpsOutDen = 1;

    setup->drop.dropMode = MPP_ENC_RC_DROP_FRM_DISABLED;
    setup->drop.dropThd = DFAULT_ENC_DROP_THD;
    setup->drop.dropGap = 1;

    setup->rc.rcMode = MPP_ENC_RC_MODE_BUTT;

    setup->gop.gopMode = VID_CODEC_GOPMODE_NORMALP;
    setup->gop.gopLen = 0;
    setup->gop.viLen = 0;
    setup->gop.gop = setup->fps.fpsOutNum * DFAULT_ENC_GOP_OPERATOR;
}

int32_t CodecInit(void)
{
    g_pBaseComponent = (RKHdiBaseComponent *)malloc(sizeof(RKHdiBaseComponent));
    if (g_pBaseComponent == NULL) {
        HDF_LOGE("%{public}s: malloc failed!", __func__);
        return HDF_FAILURE;
    }
    int32_t ret = memset_s(g_pBaseComponent, sizeof(RKHdiBaseComponent), 0, sizeof(RKHdiBaseComponent));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: memset failed, error code: %{public}d", __func__, ret);
    }
    InitComponentSetup(&g_pBaseComponent->setup);

    ret = GetMppApi(&g_pBaseComponent->mppApi);
    if ((ret != HDF_SUCCESS) || (g_pBaseComponent->mppApi == NULL)) {
        HDF_LOGE("%{public}s: GetMppAPI failed!", __func__);
        ReleaseMppApi(g_pBaseComponent->mppApi);
        g_pBaseComponent->mppApi = NULL;
        free(g_pBaseComponent);
        g_pBaseComponent = NULL;
        return HDF_FAILURE;
    }

    g_pBaseComponent->ctxType = MPP_CTX_BUTT;
    g_pBaseComponent->codingType = MPP_VIDEO_CodingMax;
    g_pBaseComponent->frameNum = 0;
    ret = g_pBaseComponent->mppApi->HdiMppEncCfgInit(&g_pBaseComponent->cfg);
    if (ret != 0) {
        HDF_LOGE("%{public}s: config mpp cfg init failed!", __func__);
        ReleaseMppApi(g_pBaseComponent->mppApi);
        g_pBaseComponent->mppApi = NULL;
        free(g_pBaseComponent);
        g_pBaseComponent = NULL;
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CodecDeinit(void)
{
    ReleaseMppApi(g_pBaseComponent->mppApi);
    g_pBaseComponent->mppApi = NULL;
    if (g_pBaseComponent != NULL) {
        free(g_pBaseComponent);
        g_pBaseComponent = NULL;
    }
    return HDF_SUCCESS;
}

int32_t CodecSetCallback(CODEC_HANDLETYPE handle, CodecCallback *cb, UINTPTR instance)
{
    if (cb == NULL) {
        HDF_LOGE("%{public}s: call back is NULL", __func__);
        return HDF_FAILURE;
    }

    g_pBaseComponent->pCallbacks = cb;

    return HDF_SUCCESS;
}

MppCtxType GetMppCtxType(const char* name)
{
    char *pos = strstr(name, "decoder");
    if (pos != NULL) {
        return MPP_CTX_DEC;
    }

    pos = strstr(name, "encoder");
    if (pos != NULL) {
        return MPP_CTX_ENC;
    }

    HDF_LOGE("%{public}s: CtxType undefined!", __func__);
    return MPP_CTX_BUTT;
}

MppCodingType GetMppCodingType(const char* name)
{
    char *pos = strstr(name, "avc");
    if (pos != NULL) {
        return MPP_VIDEO_CodingAVC;
    }

    pos = strstr(name, "hevc");
    if (pos != NULL) {
        return MPP_VIDEO_CodingHEVC;
    }

    pos = strstr(name, "mpeg4");
    if (pos != NULL) {
        return MPP_VIDEO_CodingMPEG4;
    }

    pos = strstr(name, "mpeg2");
    if (pos != NULL) {
        return MPP_VIDEO_CodingMPEG2;
    }

    HDF_LOGE("%{public}s: CodingType unsupported!", __func__);
    return MPP_VIDEO_CodingMax;
}

int32_t CodecCreate(const char* name, CODEC_HANDLETYPE *handle)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = NULL;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    if (name == NULL || mppApi == NULL || handle == NULL) {
        return HDF_FAILURE;
    }

    MppCtxType ctxType = GetMppCtxType(name);
    if (ctxType == MPP_CTX_BUTT) {
        return HDF_ERR_NOT_SUPPORT;
    }
    g_pBaseComponent->ctxType = ctxType;
    MppCodingType codingType = GetMppCodingType(name);
    if (codingType == MPP_VIDEO_CodingMax) {
        return HDF_ERR_NOT_SUPPORT;
    }
    g_pBaseComponent->codingType = codingType;

    ret = mppApi->HdiMppCreate(&ctx, &(g_pBaseComponent->mpi));
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp create failed", __func__);
        return HDF_FAILURE;
    }
    *handle = ctx;
    g_pBaseComponent->ctx = ctx;
    g_pBaseComponent->componentName = name;
    if (g_pBaseComponent->ctxType == MPP_CTX_ENC) {
        MppPollType timeout = MPP_POLL_BLOCK;
        ret = g_pBaseComponent->mpi->control(ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: mpi control set output timeout failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    }

    ret = mppApi->HdiMppInit(ctx, ctxType, codingType);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp init failed", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t CodecDestroy(CODEC_HANDLETYPE handle)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = handle;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;

    if (g_pBaseComponent == NULL) {
        HDF_LOGE("%{public}s: g_pBaseComponent is NULL", __func__);
        return HDF_FAILURE;
    }

    if (g_pBaseComponent->cfg != NULL) {
        mppApi->HdiMppDecCfgDeinit(g_pBaseComponent->cfg);
        g_pBaseComponent->cfg = NULL;
    }

    if (g_pBaseComponent->packet != NULL) {
        mppApi->HdiMppPacketDeinit(&g_pBaseComponent->packet);
        g_pBaseComponent->packet = NULL;
    }

    if (g_pBaseComponent->frame != NULL) {
        mppApi->HdiMppFrameDeinit(&g_pBaseComponent->frame);
        g_pBaseComponent->frame = NULL;
    }

    if (g_pBaseComponent->frmBuf != NULL) {
        mppApi->HdiMppBufferPutWithCaller(g_pBaseComponent->frmBuf, __func__);
        g_pBaseComponent->frmBuf = NULL;
    }

    if (g_pBaseComponent->pktBuf != NULL) {
        mppApi->HdiMppBufferPutWithCaller(g_pBaseComponent->pktBuf, __func__);
        g_pBaseComponent->pktBuf = NULL;
    }

    if (g_pBaseComponent->frmGrp != NULL) {
        mppApi->HdiMppBufferGroupPut(g_pBaseComponent->frmGrp);
        g_pBaseComponent->frmGrp = NULL;
    }

    ret = mppApi->HdiMppDestroy(ctx);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp destroy failed", __func__);
        return HDF_FAILURE;
    }

    if (g_pBaseComponent->ctxType == MPP_CTX_DEC) {
        HDF_LOGI("%{public}s: dec frame count : %{public}d, error count : %{public}d", __func__,
            g_pBaseComponent->frameCount, g_pBaseComponent->frameErr);
        HDF_LOGI("%{public}s: dec max memory %{public}.2f MB", __func__,
            g_pBaseComponent->maxUsage / (float)BITWISE_LEFT_SHIFT_WITH_ONE);
    } else if (g_pBaseComponent->ctxType == MPP_CTX_ENC) {
        HDF_LOGI("%{public}s: enc frame count : %{public}d", __func__, g_pBaseComponent->frameCount);
    } else {
        HDF_LOGE("%{public}s: CtxType undefined!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t SetExtMppParam(Param *param)
{
    int32_t ret = HDF_SUCCESS;
    int32_t paramKey = param->key;

    switch (paramKey) {
        case KEY_EXT_SPLIT_PARSE_RK:
            ret = SetParamSplitParse(g_pBaseComponent, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config set split parse failed", __func__);
            }
            break;
        case KEY_EXT_DEC_FRAME_NUM_RK:
        case KEY_EXT_ENC_FRAME_NUM_RK:
            ret = SetParamCodecFrameNum(g_pBaseComponent, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config set frame number failed", __func__);
            }
            break;
        case KEY_EXT_SETUP_DROP_MODE_RK:
            ret = SetParamDrop(g_pBaseComponent, param);
            break;
        case KEY_EXT_ENC_VALIDATE_SETUP_RK:
            ret = ValidateEncSetup(g_pBaseComponent, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config validata setup failed", __func__);
            }
            break;
        case KEY_EXT_ENC_SETUP_AVC_RK:
            ret = SetParamEncSetupAVC(g_pBaseComponent, param);
            break;
        default:
            HDF_LOGE("%{public}s: param key unsupport, key:%{public}d", __func__, paramKey);
            return HDF_FAILURE;
    }
    return ret;
}

int32_t SetMppParam(Param *param)
{
    int32_t ret = HDF_SUCCESS;
    int32_t paramKey = param->key;

    switch (paramKey) {
        case KEY_VIDEO_WIDTH:
            ret = SetParamWidth(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_HEIGHT:
            ret = SetParamHeight(g_pBaseComponent, param);
            break;
        case KEY_PIXEL_FORMAT:
            ret = SetParamPixleFmt(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_STRIDE:
            ret = SetParamStride(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_FRAME_RATE:
            ret = SetParamFps(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_RC_MODE:
            ret = SetParamRateControl(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_GOP_MODE:
            ret = SetParamGop(g_pBaseComponent, param);
            break;
        case KEY_MIMETYPE:
            ret = SetParamMimeCodecType(g_pBaseComponent, param);
            break;
        case KEY_CODEC_TYPE:
            ret = SetParamCodecType(g_pBaseComponent, param);
            break;
        default:
            ret = SetExtMppParam(param);
    }
    return ret;
}

int32_t CodecSetParameter(CODEC_HANDLETYPE handle, Param *params, int32_t paramCnt)
{
    MppCtx ctx = handle;
    int32_t ret = HDF_SUCCESS;
    if (ctx != g_pBaseComponent->ctx) {
        HDF_LOGE("%{public}s: ctx not match %{public}d", __func__, ctx);
        return HDF_FAILURE;
    }

    for (int32_t i = 0; i < paramCnt; i++) {
        ret = SetMppParam(params + i);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%{public}s: SetMppParam faild, param key:%{public}d", __func__, params[i].key);
            return ret;
        }
    }

    return HDF_SUCCESS;
}

int32_t GetExtMppParam(Param *param)
{
    int32_t ret = HDF_SUCCESS;
    int32_t paramKey = param->key;

    switch (paramKey) {
        case KEY_EXT_DEFAULT_CFG_RK:
            ret = GetDefaultConfig(g_pBaseComponent);
            if (ret != 0) {
                HDF_LOGE("%{public}s: config get default config failed", __func__);
            }
            break;
        case KEY_EXT_SPLIT_PARSE_RK:
            ret = GetParamSplitParse(g_pBaseComponent, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config set split parse failed", __func__);
            }
            break;
        case KEY_EXT_DEC_FRAME_NUM_RK:
        case KEY_EXT_ENC_FRAME_NUM_RK:
            ret = GetParamCodecFrameNum(g_pBaseComponent, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config set frame number failed", __func__);
            }
            break;
        case KEY_EXT_SETUP_DROP_MODE_RK:
            ret = GetParamDrop(g_pBaseComponent, param);
            break;
        default:
            HDF_LOGE("%{public}s: param key unsupport, key:%{public}d", __func__, paramKey);
            return HDF_FAILURE;
    }
    return ret;
}

int32_t GetMppParam(Param *param)
{
    int32_t ret = HDF_SUCCESS;
    int32_t paramKey = param->key;

    switch (paramKey) {
        case KEY_BUFFERSIZE:
            ret = GetParamBufferSize(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_WIDTH:
            ret = GetParamWidth(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_HEIGHT:
            ret = GetParamHeight(g_pBaseComponent, param);
            break;
        case KEY_PIXEL_FORMAT:
            ret = GetParamPixleFmt(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_STRIDE:
            ret = GetParamStride(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_FRAME_RATE:
            ret = GetParamFps(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_RC_MODE:
            ret = GetParamRateControl(g_pBaseComponent, param);
            break;
        case KEY_VIDEO_GOP_MODE:
            ret = GetParamGop(g_pBaseComponent, param);
            break;
        case KEY_MIMETYPE:
            ret = GetParamMimeCodecType(g_pBaseComponent, param);
            break;
        case KEY_CODEC_TYPE:
            ret = GetParamCodecType(g_pBaseComponent, param);
            break;
        default:
            ret = GetExtMppParam(param);
    }
    return ret;
}

int32_t CodecGetParameter(CODEC_HANDLETYPE handle, Param *params, int32_t paramCnt)
{
    int32_t ret = HDF_SUCCESS;
    MppCtx ctx = handle;
    if (ctx != g_pBaseComponent->ctx) {
        HDF_LOGE("%{public}s: ctx not match %{public}d", __func__, ctx);
        return HDF_FAILURE;
    }

    for (int32_t i = 0; i < paramCnt; i++) {
        ret = GetMppParam(params + i);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%{public}s: GetMppParam faild, param key:%{public}d", __func__, params[i].key);
            return ret;
        }
    }

    return HDF_SUCCESS;
}

int32_t CodecStart(CODEC_HANDLETYPE handle)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = handle;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;

    ret = mppApi->HdiMppStart(ctx);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp start failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CodecStop(CODEC_HANDLETYPE handle)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = handle;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;

    ret = mppApi->HdiMppStop(ctx);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp stop failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CodecFlush(CODEC_HANDLETYPE handle, DirectionType directType)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = handle;
    switch (directType) {
        case INPUT_TYPE:
        case OUTPUT_TYPE:
        case ALL_TYPE:
            ret = g_pBaseComponent->mpi->reset(ctx);
            if (ret != 0) {
                HDF_LOGE("%{public}s: reset failed", __func__);
                return HDF_FAILURE;
            }
            break;
        default:
            HDF_LOGE("%{public}s: directType failed", __func__);
            return HDF_FAILURE;
    }

    UINTPTR userData = NULL;
    EventType event = EVENT_FLUSH_COMPLETE;
    uint32_t length = 0;
    int32_t *eventData = NULL;
    g_pBaseComponent->pCallbacks->OnEvent(userData, event, length, eventData);

    return HDF_SUCCESS;
}

int32_t DecodeInitPacket(MppPacket *pPacket, CodecBuffer *inputData, RK_U32 pkt_eos)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    uint8_t *inBuffer = (uint8_t *)inputData->buffer[0].buf;
    uint32_t inBufferSize = inputData->buffer[0].length;

    ret = mppApi->HdiMppPacketInit(pPacket, inBuffer, inBufferSize);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp packet_init failed", __func__);
        return HDF_FAILURE;
    }

    mppApi->HdiMppPacketSetData(*pPacket, inBuffer);
    mppApi->HdiMppPacketSetSize(*pPacket, inBufferSize);
    mppApi->HdiMppPacketSetPos(*pPacket, inBuffer);
    mppApi->HdiMppPacketSetLength(*pPacket, inBufferSize);
    // setup eos flag
    if (pkt_eos != 0) {
        mppApi->HdiMppPacketSetEos(*pPacket);
    }
    return HDF_SUCCESS;
}

MPP_RET DecodeGetFrame(MppCtx ctx, MppFrame *frame)
{
    MppApi *mpi = g_pBaseComponent->mpi;
    MPP_RET ret = MPP_OK;
    RK_S32 retryTimes = 10;
    while (true) {
        ret = mpi->decode_get_frame(ctx, frame);
        if (ret != MPP_ERR_TIMEOUT) {
            break;
        }
        if (retryTimes > 0) {
            retryTimes--;
            usleep(SLEEP_INTERVAL_MICROSECONDS);
        } else {
            HDF_LOGE("%{public}s: decode_get_frame failed too much time", __func__);
            break;
        }
    }
    return ret;
}

int32_t HandleDecodeFrameInfoChange(MppFrame frame, MppCtx ctx)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    MppApi *mpi = g_pBaseComponent->mpi;
    RK_U32 width = mppApi->HdiMppFrameGetWidth(frame);
    RK_U32 height = mppApi->HdiMppFrameGetHeight(frame);
    RK_U32 hor_stride = mppApi->HdiMppFrameGetHorStride(frame);
    RK_U32 ver_stride = mppApi->HdiMppFrameGetVerStride(frame);
    RK_U32 buf_size = mppApi->HdiMppFrameGetBufferSize(frame);

    HDF_LOGI("%{public}s: decode_get_frame get info changed found", __func__);
    HDF_LOGI("%{public}s: decoder require buffer w:h [%{public}d:%{public}d]", __func__, width, height);
    HDF_LOGI("%{public}s: decoder require stride [%{public}d:%{public}d]", __func__, hor_stride, ver_stride);
    HDF_LOGI("%{public}s: decoder require buf_size %{public}d", __func__, buf_size);

    if (g_pBaseComponent->frmGrp == NULL) {
        ret = mppApi->HdiMppBufferGroupGet(&g_pBaseComponent->frmGrp,
            MPP_BUFFER_TYPE_DRM, MPP_BUFFER_INTERNAL, NULL, __func__);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: get mpp buffer group failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
        ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, g_pBaseComponent->frmGrp);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: set buffer group failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    } else {
        ret = mppApi->HdiMppBufferGroupClear(g_pBaseComponent->frmGrp);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: clear buffer group failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    }
    
    ret = mppApi->HdiMppBufferGroupLimitConfig(g_pBaseComponent->frmGrp, buf_size, BUFFER_GROUP_LIMIT_NUM);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: limit buffer group failed ret %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    ret = mpi->control(ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: info change ready failed ret %{public}d", __func__, ret);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static IM_STATUS PutDecodeFrameToOutput(MppFrame frame, CodecBuffer *outInfo)
{
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    MppBuffer mppBuffer = mppApi->HdiMppFrameGetBuffer(frame);
    rga_buffer_t src;
    rga_buffer_t dst;
    im_rect rect;
    int32_t width = mppApi->HdiMppFrameGetWidth(frame);
    int32_t height = mppApi->HdiMppFrameGetHeight(frame);
    
    int32_t err = memset_s(&src, sizeof(src), 0, sizeof(src));
    if (err != EOK) {
        HDF_LOGE("%{public}s: memset_s src failed, error code: %{public}d", __func__, err);
        return IM_STATUS_FAILED;
    }
    memset_s(&dst, sizeof(dst), 0, sizeof(dst));
    if (err != EOK) {
        HDF_LOGE("%{public}s: memset_s dst failed, error code: %{public}d", __func__, err);
        return IM_STATUS_FAILED;
    }

    src.fd = mppApi->HdiMppBufferGetFdWithCaller(mppBuffer, __func__);
    src.wstride  = mppApi->HdiMppFrameGetHorStride(frame);
    src.hstride  = mppApi->HdiMppFrameGetVerStride(frame);
    src.width    = width;
    src.height   = height;
    src.format   = RK_FORMAT_YCbCr_420_SP;
    dst.fd = ((BufferHandle *)outInfo->buffer[0].buf)->fd;
    dst.wstride  = mppApi->HdiMppFrameGetHorStride(frame);
    dst.hstride  = mppApi->HdiMppFrameGetVerStride(frame);
    dst.width    = width;
    dst.height   = height;
    dst.format   = RK_FORMAT_YCbCr_420_SP;
    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height;
    return imcrop(src, dst, rect);
}

void HandleDecodeFrameOutput(MppFrame frame, int32_t frm_eos, CodecBuffer *outInfo)
{
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    RK_U32 err_info = mppApi->HdiMppFrameGetErrinfo(frame);
    RK_U32 discard = mppApi->HdiMppFrameGetDiscard(frame);
    g_pBaseComponent->frameCount++;
    
    if ((err_info | discard) != 0) {
        g_pBaseComponent->frameErr++;
        HDF_LOGE("%{public}s: bad output data, err_info: %{public}d", __func__, err_info);
        return;
    }
    if (outInfo == NULL || outInfo->bufferCnt == 0) {
        HDF_LOGE("%{public}s: outInfo param invalid!", __func__);
        return;
    }
    // have output data
    if (outInfo->buffer[0].type == BUFFER_TYPE_HANDLE) {
        IM_STATUS ret = PutDecodeFrameToOutput(frame, outInfo);
        if (ret != IM_STATUS_SUCCESS) {
            HDF_LOGE("%{public}s: copy decode output data failed, error code: %{public}d", __func__, ret);
        }
    } else {
        MppBuffer buffer = mppApi->HdiMppFrameGetBuffer(frame);
        RK_U8 *base = (RK_U8 *)mppApi->HdiMppBufferGetPtrWithCaller(buffer, __func__);
        RK_U32 bufferSize = mppApi->HdiMppFrameGetBufferSize(frame);
        uint8_t *outBuffer = (uint8_t *)outInfo->buffer[0].buf;
        uint32_t outBufferSize = outInfo->buffer[0].capacity;
        if (outBuffer != NULL && outBufferSize != 0 && base != NULL && bufferSize != 0) {
            int32_t ret = memcpy_s(outBuffer, outBufferSize, base, outBufferSize);
            if (ret == EOK) {
                outInfo->buffer[0].length = bufferSize;
            } else {
                HDF_LOGE("%{public}s: copy output data failed, error code: %{public}d", __func__, ret);
                HDF_LOGE("%{public}s: dst bufferSize:%{public}d, src data len: %{public}d", __func__,
                    outBufferSize, bufferSize);
            }
        } else {
            if (frm_eos == 0) {
                HDF_LOGE("%{public}s: output data not copy, buffer incorrect!", __func__);
            }
        }
    }

    if (frm_eos != 0) {
        outInfo->flag |= STREAM_FLAG_EOS;
        HDF_LOGI("%{public}s: dec reach STREAM_FLAG_EOS, frame count : %{public}d, error count : %{public}d",
            __func__, g_pBaseComponent->frameCount, g_pBaseComponent->frameErr);
    }
    int32_t acquireFd = 1;
    g_pBaseComponent->pCallbacks->OutputBufferAvailable((UINTPTR)NULL, outInfo, &acquireFd);
}

int32_t HandleDecodedFrame(MppFrame frame, MppCtx ctx, int32_t frm_eos, CodecBuffer *outInfo)
{
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    if (frame) {
        if (mppApi->HdiMppFrameGetInfoChange(frame)) {
            if (HandleDecodeFrameInfoChange(frame, ctx) != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: func failed!", __func__);
                return HDF_FAILURE;
            }
        } else {
            HandleDecodeFrameOutput(frame, frm_eos, outInfo);
        }
        mppApi->HdiMppFrameDeinit(&frame);
    }

    // try get runtime frame memory usage
    if (g_pBaseComponent->frmGrp) {
        size_t usage = mppApi->HdiMppBufferGroupUsage(g_pBaseComponent->frmGrp);
        if (usage > g_pBaseComponent->maxUsage)
            g_pBaseComponent->maxUsage = usage;
    }
    return HDF_SUCCESS;
}

RK_U32 CodecDecodeGetFrameLoop(MppCtx ctx, RK_U32 pkt_done, RK_U32 pkt_eos, CodecBuffer *outInfo)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    RK_U32 frm_eos = 0;
    MppFrame frame = NULL;

    do {
        RK_S32 get_frm = 0;
        ret = DecodeGetFrame(ctx, &frame);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: decode_get_frame failed, ret:%{public}d", __func__, ret);
            break;
        }
        if (frame) {
            frm_eos = mppApi->HdiMppFrameGetEos(frame);
            get_frm = 1;
            if (HandleDecodedFrame(frame, ctx, frm_eos, outInfo) != HDF_SUCCESS) {
                break;
            }
        }
        if (pkt_eos != 0 && pkt_done != 0 && frm_eos == 0) {
            usleep(SLEEP_INTERVAL_MICROSECONDS);
            continue;
        }
        if (frm_eos) {
            break;
        }

        if ((g_pBaseComponent->frameNum > 0 && (g_pBaseComponent->frameCount >= g_pBaseComponent->frameNum)) ||
            ((g_pBaseComponent->frameNum == 0) && frm_eos != 0)) {
            break;
        }
        if (get_frm) {
            continue;
        }
        break;
    } while (1);

    return frm_eos;
}

int32_t CodecDecode(CODEC_HANDLETYPE handle, CodecBuffer* inputData, CodecBuffer* outInfo, uint32_t timeoutMs)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = handle;
    MppApi *mpi = g_pBaseComponent->mpi;
    MppPacket packet = g_pBaseComponent->packet;
    RK_U32 loop_end = 0;
    RK_U32 frm_eos;
    RK_U32 pkt_done = 0;
    RK_U32 pkt_eos = (inputData->flag == STREAM_FLAG_EOS) ? 1 : 0;

    if (DecodeInitPacket(&packet, inputData, pkt_eos) != HDF_SUCCESS) {
        HDF_LOGE("%{public}s: Init packet failed!", __func__);
        return HDF_FAILURE;
    }

    do {
        if (!pkt_done) {
            ret = mpi->decode_put_packet(ctx, packet);
            if (MPP_OK == ret) {
                pkt_done = 1;
            }
        }

        frm_eos = CodecDecodeGetFrameLoop(ctx, pkt_done, pkt_eos, outInfo);
        if ((g_pBaseComponent->frameNum > 0 && (g_pBaseComponent->frameCount >= g_pBaseComponent->frameNum)) ||
            ((g_pBaseComponent->frameNum == 0) && frm_eos != 0)) {
            loop_end = 1;
            break;
        }
        if (pkt_done) {
            break;
        }
        usleep(SLEEP_INTERVAL_MICROSECONDS);
    } while (1);
    int32_t acquireFd = 1;
    g_pBaseComponent->pCallbacks->InputBufferAvailable((UINTPTR)NULL, inputData, &acquireFd);

    if (loop_end != 1) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static IM_STATUS GetEncodeFrameFromInput(MppFrame frame, MppBuffer mppBuffer, CodecBuffer *inputInfo)
{
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    rga_buffer_t src;
    rga_buffer_t dst;
    im_rect rect;
    int32_t width = mppApi->HdiMppFrameGetWidth(frame);
    int32_t height = mppApi->HdiMppFrameGetHeight(frame);
    
    int32_t err = memset_s(&src, sizeof(src), 0, sizeof(src));
    if (err != EOK) {
        HDF_LOGE("%{public}s: memset_s src failed, error code: %{public}d", __func__, err);
        return IM_STATUS_FAILED;
    }
    err = memset_s(&dst, sizeof(dst), 0, sizeof(dst));
    if (err != EOK) {
        HDF_LOGE("%{public}s: memset_s dst failed, error code: %{public}d", __func__, err);
        return IM_STATUS_FAILED;
    }

    src.fd = ((BufferHandle *)inputInfo->buffer[0].buf)->fd;
    src.wstride  = mppApi->HdiMppFrameGetHorStride(frame);
    src.hstride  = mppApi->HdiMppFrameGetVerStride(frame);
    src.width    = width;
    src.height   = height;
    src.format   = RK_FORMAT_YCbCr_420_SP;
    dst.fd = mppApi->HdiMppBufferGetFdWithCaller(mppBuffer, __func__);
    dst.wstride  = mppApi->HdiMppFrameGetHorStride(frame);
    dst.hstride  = mppApi->HdiMppFrameGetVerStride(frame);
    dst.width    = width;
    dst.height   = height;
    dst.format   = RK_FORMAT_YCbCr_420_SP;
    rect.x = 0;
    rect.y = 0;
    rect.width = width;
    rect.height = height;
    return imcrop(src, dst, rect);
}

int32_t EncodeInitFrame(MppFrame *pFrame, RK_U32 frm_eos, CodecBuffer *inputData)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;

    if (frm_eos != 0) {
        HDF_LOGI("%{public}s: receive eos frame", __func__);
    }
    ret = mppApi->HdiMppFrameInit(pFrame);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp_frame_init failed", __func__);
        return HDF_FAILURE;
    }
    mppApi->HdiMppFrameSetWidth(*pFrame, g_pBaseComponent->setup.width);
    mppApi->HdiMppFrameSetHeight(*pFrame, g_pBaseComponent->setup.height);
    mppApi->HdiMppFrameSetHorStride(*pFrame, g_pBaseComponent->setup.stride.horStride);
    mppApi->HdiMppFrameSetVerStride(*pFrame, g_pBaseComponent->setup.stride.verStride);
    mppApi->HdiMppFrameSetFormat(*pFrame, g_pBaseComponent->fmt);
    mppApi->HdiMppFrameSetEos(*pFrame, frm_eos);

    if (inputData->buffer[0].type == BUFFER_TYPE_HANDLE) {
        IM_STATUS status = GetEncodeFrameFromInput(*pFrame, g_pBaseComponent->frmBuf, inputData);
        if (status == IM_STATUS_SUCCESS) {
            mppApi->HdiMppFrameSetBuffer(*pFrame, g_pBaseComponent->frmBuf);
        } else {
            mppApi->HdiMppFrameDeinit(&pFrame);
            HDF_LOGE("%{public}s: copy encode input data failed, error code: %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    } else {
        uint8_t *buf = NULL;
        if ((uint8_t *)inputData->buffer[0].buf != NULL && inputData->buffer[0].length > 0) {
            buf = (uint8_t *)mppApi->HdiMppBufferGetPtrWithCaller(g_pBaseComponent->frmBuf, __func__);
            if (buf == NULL) {
                HDF_LOGE("%{public}s: mpp buffer get ptr with caller failed", __func__);
                mppApi->HdiMppFrameDeinit(&pFrame);
                return HDF_FAILURE;
            }
            uint8_t *inBuffer = (uint8_t *)inputData->buffer[0].buf;
            uint32_t inBufferSize = inputData->buffer[0].length;
            int32_t ret = memcpy_s(buf, inBufferSize, inBuffer, inBufferSize);
            if (ret != EOK) {
                HDF_LOGE("%{public}s: copy input data failed, error code: %{public}d", __func__, ret);
                mppApi->HdiMppFrameDeinit(&pFrame);
                return HDF_FAILURE;
            }
            mppApi->HdiMppFrameSetBuffer(*pFrame, g_pBaseComponent->frmBuf);
        } else {
            mppApi->HdiMppFrameSetBuffer(*pFrame, NULL);
            HDF_LOGI("%{public}s: receive empty frame", __func__);
        }
    }

    return HDF_SUCCESS;
}

int32_t HandleEncodedPacket(MppPacket packet, RK_U32 pkt_eos, CodecBuffer *outInfo)
{
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    void *ptr   = mppApi->HdiMppPacketGetPos(packet);
    size_t len  = mppApi->HdiMppPacketGetLength(packet);
    pkt_eos = mppApi->HdiMppPacketGetEos(packet);

    // call back have out data
    UINTPTR userData = NULL;
    int32_t acquireFd = 1;
    uint8_t *outBuffer = (uint8_t *)outInfo->buffer[0].buf;
    uint32_t outBufferSize = outInfo->buffer[0].capacity;
    if (outBuffer != NULL && outBufferSize != 0 && ptr != NULL && len != 0) {
        int32_t ret = memcpy_s(outBuffer, outBufferSize, ptr, len);
        if (ret == EOK) {
            outInfo->buffer[0].length = len;
        } else {
            HDF_LOGE("%{public}s: copy output data failed, error code: %{public}d", __func__, ret);
            HDF_LOGE("%{public}s: dst bufferSize:%{public}d, src data len: %{public}d", __func__, outBufferSize, len);
        }
    } else {
        HDF_LOGE("%{public}s: output data not copy, buffer incorrect!", __func__);
    }
    if (pkt_eos != 0) {
        outInfo->flag |= STREAM_FLAG_EOS;
        HDF_LOGI("%{public}s: enc reach STREAM_FLAG_EOS, frame count : %{public}d",
            __func__, g_pBaseComponent->frameCount);
    }
    g_pBaseComponent->pCallbacks->OutputBufferAvailable(userData, outInfo, &acquireFd);

    return HDF_SUCCESS;
}

RK_U32 CodecEncodeGetPacketLoop(MppCtx ctx, CodecBuffer *outInfo)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi = g_pBaseComponent->mpi;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    MppPacket packet = g_pBaseComponent->packet;
    RK_U32 eoi = 1;
    RK_U32 pkt_eos = 0;

    do {
        ret = mpi->encode_get_packet(ctx, &packet);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: mpp encode get packet failed", __func__);
            return HDF_FAILURE;
        }

        if (packet) {
            pkt_eos = mppApi->HdiMppPacketGetEos(packet);
            HandleEncodedPacket(packet, pkt_eos, outInfo);

            /* for low delay partition encoding */
            if (mppApi->HdiMppPacketIsPartition(packet)) {
                eoi = mppApi->HdiMppPacketIsEoi(packet);
            }

            mppApi->HdiMppPacketDeinit(&packet);
            g_pBaseComponent->frameCount += eoi;

            if (pkt_eos != 0) {
                HDF_LOGI("%{public}s: find eos packet", __func__);
                break;
            }
        }
    } while (!eoi);

    return pkt_eos;
}

int32_t CodecEncode(CODEC_HANDLETYPE handle, CodecBuffer *inputData, CodecBuffer *outInfo, uint32_t timeoutMs)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi = g_pBaseComponent->mpi;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;
    MppFrame frame = NULL;
    MppCtx ctx = handle;
    RK_U32 pkt_eos = 0;
    RK_U32 loop_end = 0;
    RK_U32 frm_eos = 0;
    UINTPTR userData = NULL;
    int32_t acquireFd = 1;

    if (inputData == NULL || inputData->bufferCnt == 0) {
        HDF_LOGE("%{public}s: inputData param invalid!", __func__);
        return HDF_FAILURE;
    }
    frm_eos = (inputData->flag == STREAM_FLAG_EOS) ? 1 : 0;

    if (EncodeInitFrame(&frame, frm_eos, inputData) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    ret = mpi->encode_put_frame(ctx, frame);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp encode put frame failed", __func__);
        mppApi->HdiMppFrameDeinit(&frame);
        return HDF_FAILURE;
    }
    mppApi->HdiMppFrameDeinit(&frame);
    pkt_eos = CodecEncodeGetPacketLoop(ctx, outInfo);

    g_pBaseComponent->pCallbacks->InputBufferAvailable(userData, inputData, &acquireFd);
    if (g_pBaseComponent->frameNum > 0 && g_pBaseComponent->frameCount >= g_pBaseComponent->frameNum) {
        loop_end = 1;
    }

    if (frm_eos != 0 && pkt_eos != 0) {
        loop_end = 1;
    }

    if (loop_end != 1) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CodecEncodeHeader(CODEC_HANDLETYPE handle, CodecBuffer outInfo, uint32_t timeoutMs)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = handle;
    RKMppApi *mppApi = g_pBaseComponent->mppApi;

    MppPacket packet = NULL;

    mppApi->HdiMppPacketInitWithBuffer(&packet, g_pBaseComponent->pktBuf);
    // NOTE: It is important to clear output packet length!!
    mppApi->HdiMppPacketSetLength(packet, 0);

    ret = g_pBaseComponent->mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpi control enc get extra info failed", __func__);
        return HDF_FAILURE;
    } else {
        void *ptr   = mppApi->HdiMppPacketGetPos(packet);
        size_t len  = mppApi->HdiMppPacketGetLength(packet);

        // call back have out data
        UINTPTR userData = NULL;
        int32_t acquireFd = 1;
        uint8_t *outBuffer = (uint8_t *)outInfo.buffer[0].buf;
        uint32_t outBufferSize = outInfo.buffer[0].capacity;
        outInfo.buffer[0].length = len;
        if (outBuffer != NULL && outBufferSize != 0 && ptr != NULL && len != 0) {
            int32_t ret = memcpy_s(outBuffer, len, ptr, len);
            if (ret != EOK) {
                HDF_LOGE("%{public}s: copy output data failed, error code: %{public}d", __func__, ret);
            }
        }
        g_pBaseComponent->pCallbacks->OutputBufferAvailable(userData, &outInfo, &acquireFd);
    }

    mppApi->HdiMppPacketDeinit(&packet);
    return HDF_SUCCESS;
}
