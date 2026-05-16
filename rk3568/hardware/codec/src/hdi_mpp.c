/*
 * Copyright (c) 2022-2023 Shenzhen Kaihong DID Co., Ltd.
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
#include <hdf_base.h>
#include <hdf_log.h>
#include <securec.h>
#include "hdi_mpp_component_manager.h"
#include "hdi_mpp_config.h"
#include "im2d.h"
#include "mpp_common.h"
#include "rga.h"
#include "rk_vdec_cfg.h"

#define HDF_LOG_TAG codec_hdi_mpp
#define BITWISE_LEFT_SHIFT_WITH_ONE     (1 << 20)
#define SLEEP_INTERVAL_MICROSECONDS     1000
#define BUFFER_GROUP_LIMIT_NUM          24
#define FRAME_STRIDE_ALIGNMENT          16

static void InitComponentSetup(RKHdiBaseComponent *component)
{
    component->setup.fmt = PIXEL_FMT_BUTT;
    component->fmt = MPP_FMT_BUTT;
    SetDefaultFps(&component->setup);
    SetDefaultDropMode(&component->setup);
    component->setup.rc.rcMode = MPP_ENC_RC_MODE_VBR;
    SetDefaultGopMode(&component->setup);
}

static RKHdiBaseComponent* CreateMppComponent(MppCtxType ctxType, MppCodingType codingType)
{
    RKHdiBaseComponent* component = (RKHdiBaseComponent *)malloc(sizeof(RKHdiBaseComponent));
    if (component == NULL) {
        HDF_LOGE("%{public}s: malloc failed!", __func__);
        return NULL;
    }
    int32_t ret = memset_s(component, sizeof(RKHdiBaseComponent), 0, sizeof(RKHdiBaseComponent));
    if (ret != EOK) {
        HDF_LOGE("%{public}s: memset failed, error code: %{public}d", __func__, ret);
    }
    InitComponentSetup(component);

    ret = GetMppApi(&component->mppApi);
    if ((ret != HDF_SUCCESS) || (component->mppApi == NULL)) {
        HDF_LOGE("%{public}s: GetMppAPI failed!", __func__);
        component->mppApi = NULL;
        free(component);
        return NULL;
    }

    component->ctxType = ctxType;
    component->codingType = codingType;
    component->frameNum = 0;
    ret = InitMppConfig(component);
    if (ret != 0) {
        HDF_LOGE("%{public}s: config mpp cfg init failed!", __func__);
        ReleaseMppApi(component->mppApi);
        component->mppApi = NULL;
        free(component);
        return NULL;
    }

    return component;
}

static void DestroyMppComponent(RKHdiBaseComponent *component)
{
    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
    }
    DeinitMppConfig(component);
    ReleaseMppApi(component->mppApi);
    component->mppApi = NULL;
    free(component);
    component = NULL;
}

int32_t CodecInit(void)
{
    return HDF_SUCCESS;
}

int32_t CodecDeinit(void)
{
    return HDF_SUCCESS;
}

int32_t CodecSetCallback(CODEC_HANDLETYPE handle, CodecCallback *cb, UINTPTR instance)
{
    RKHdiBaseComponent* component = FindInMppComponentManager(handle);
    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    if (cb == NULL) {
        HDF_LOGE("%{public}s: call back is NULL", __func__);
        return HDF_FAILURE;
    }

    component->pCallbacks = cb;

    return HDF_SUCCESS;
}

static MppCtxType GetMppCtxType(const char* name)
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

static MppCodingType GetMppCodingType(const char* name)
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

    pos = strstr(name, "vp8");
    if (pos != NULL) {
        return MPP_VIDEO_CodingVP8;
    }

    pos = strstr(name, "vp9");
    if (pos != NULL) {
        return MPP_VIDEO_CodingVP9;
    }

    pos = strstr(name, "flv1");
    if (pos != NULL) {
        return MPP_VIDEO_CodingFLV1;
    }

    pos = strstr(name, "mjpeg");
    if (pos != NULL) {
        return MPP_VIDEO_CodingMJPEG;
    }

    HDF_LOGE("%{public}s: CodingType unsupported! name:%{public}s", __func__, name);
    return MPP_VIDEO_CodingMax;
}

int32_t CodecCreate(const char* name, CODEC_HANDLETYPE *handle)
{
    if (name == NULL || handle == NULL) {
        HDF_LOGE("%{public}s: invalid params!", __func__);
        return HDF_FAILURE;
    }
    MppCtxType ctxType = GetMppCtxType(name);
    if (ctxType == MPP_CTX_BUTT) {
        HDF_LOGE("%{public}s: MppCtxType:%{public}d not support!", __func__, ctxType);
        return HDF_ERR_NOT_SUPPORT;
    }
    MppCodingType codingType = GetMppCodingType(name);
    if (codingType == MPP_VIDEO_CodingMax) {
        HDF_LOGE("%{public}s: MppCodingType:%{public}d not support!", __func__, codingType);
        return HDF_ERR_NOT_SUPPORT;
    }

    MPP_RET ret = MPP_OK;
    MppCtx ctx = NULL;
    RKHdiBaseComponent* component = CreateMppComponent(ctxType, codingType);
    if (component == NULL) {
        return HDF_FAILURE;
    }

    ret = component->mppApi->HdiMppCreate(&ctx, &(component->mpi));
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp create failed", __func__);
        return HDF_FAILURE;
    }
    *handle = ctx;
    component->ctx = ctx;
    component->componentName = name;
    if (!AddToMppComponentManager(*handle, component)) {
        HDF_LOGE("%{public}s: AddToMppComponentManager failed!", __func__);
        return HDF_FAILURE;
    }
    if (component->ctxType == MPP_CTX_ENC) {
        MppPollType timeout = MPP_POLL_BLOCK;
        ret = component->mpi->control(ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: mpi control set output timeout failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    }

    ret = component->mppApi->HdiMppInit(ctx, ctxType, codingType);
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

    RKHdiBaseComponent* component = FindInMppComponentManager(handle);
    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }

    RKMppApi *mppApi = component->mppApi;
    if (component->packet != NULL) {
        mppApi->HdiMppPacketDeinit(&component->packet);
        component->packet = NULL;
    }

    if (component->frame != NULL) {
        mppApi->HdiMppFrameDeinit(&component->frame);
        component->frame = NULL;
    }

    if (component->frmBuf != NULL) {
        mppApi->HdiMppBufferPutWithCaller(component->frmBuf, __func__);
        component->frmBuf = NULL;
    }

    if (component->pktBuf != NULL) {
        mppApi->HdiMppBufferPutWithCaller(component->pktBuf, __func__);
        component->pktBuf = NULL;
    }

    if (component->frmGrp != NULL) {
        mppApi->HdiMppBufferGroupPut(component->frmGrp);
        component->frmGrp = NULL;
    }

    if (component->ctxType == MPP_CTX_DEC) {
        HDF_LOGI("%{public}s: dec frame count : %{public}d, error count : %{public}d", __func__,
            component->frameCount, component->frameErr);
        HDF_LOGI("%{public}s: dec max memory %{public}.2f MB", __func__,
            component->maxUsage / (float)BITWISE_LEFT_SHIFT_WITH_ONE);
    } else if (component->ctxType == MPP_CTX_ENC) {
        HDF_LOGI("%{public}s: enc frame count : %{public}d", __func__, component->frameCount);
    } else {
        HDF_LOGE("%{public}s: CtxType undefined!", __func__);
    }

    ret = mppApi->HdiMppDestroy(ctx);
    RemoveFromMppComponentManager(handle);
    DestroyMppComponent(component);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp destroy failed", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t SetExtMppParam(RKHdiBaseComponent* component, Param *param)
{
    int32_t ret = HDF_SUCCESS;
    int32_t paramKey = param->key;

    switch (paramKey) {
        case KEY_EXT_SPLIT_PARSE_RK:
            ret = SetParamSplitParse(component, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config set split parse failed", __func__);
            }
            break;
        case KEY_EXT_DEC_FRAME_NUM_RK:
        case KEY_EXT_ENC_FRAME_NUM_RK:
            ret = SetParamCodecFrameNum(component, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config set frame number failed", __func__);
            }
            break;
        case KEY_EXT_SETUP_DROP_MODE_RK:
            ret = SetParamDrop(component, param);
            break;
        default:
            HDF_LOGE("%{public}s: param key unsupport, key:%{public}d", __func__, paramKey);
            return HDF_FAILURE;
    }
    return ret;
}

int32_t SetMppParam(RKHdiBaseComponent* component, Param *param)
{
    int32_t ret = HDF_SUCCESS;
    int32_t paramKey = param->key;

    switch (paramKey) {
        case KEY_VIDEO_WIDTH:
            ret = SetParamWidth(component, param);
            break;
        case KEY_VIDEO_HEIGHT:
            ret = SetParamHeight(component, param);
            break;
        case KEY_PIXEL_FORMAT:
            ret = SetParamPixelFmt(component, param);
            break;
        case KEY_VIDEO_STRIDE:
            ret = SetParamStride(component, param);
            break;
        case KEY_VIDEO_FRAME_RATE:
            ret = SetParamFps(component, param);
            break;
        case KEY_VIDEO_RC_MODE:
            ret = SetParamRateControl(component, param);
            break;
        case KEY_VIDEO_GOP_MODE:
            ret = SetParamGop(component, param);
            break;
        case KEY_MIMETYPE:
            ret = SetParamMimeCodecType(component, param);
            break;
        case KEY_CODEC_TYPE:
            ret = SetParamCodecType(component, param);
            break;
        default:
            ret = SetExtMppParam(component, param);
    }
    return ret;
}

int32_t CodecSetParameter(CODEC_HANDLETYPE handle, Param *params, int32_t paramCnt)
{
    MppCtx ctx = handle;
    int32_t ret = HDF_SUCCESS;
    RKHdiBaseComponent* component = FindInMppComponentManager(handle);

    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    if (ctx != component->ctx) {
        HDF_LOGE("%{public}s: ctx not match %{public}d", __func__, ctx);
        return HDF_FAILURE;
    }

    for (int32_t i = 0; i < paramCnt; i++) {
        ret = SetMppParam(component, params + i);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%{public}s: SetMppParam faild, param key:%{public}d", __func__, params[i].key);
            return ret;
        }
    }

    return HDF_SUCCESS;
}

int32_t GetExtMppParam(RKHdiBaseComponent* component, Param *param)
{
    int32_t ret = HDF_SUCCESS;
    int32_t paramKey = param->key;

    switch (paramKey) {
        case KEY_EXT_SPLIT_PARSE_RK:
            ret = GetParamSplitParse(component, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config set split parse failed", __func__);
            }
            break;
        case KEY_EXT_DEC_FRAME_NUM_RK:
        case KEY_EXT_ENC_FRAME_NUM_RK:
            ret = GetParamCodecFrameNum(component, param);
            if (ret != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: config set frame number failed", __func__);
            }
            break;
        case KEY_EXT_SETUP_DROP_MODE_RK:
            ret = GetParamDrop(component, param);
            break;
        default:
            HDF_LOGE("%{public}s: param key unsupport, key:%{public}d", __func__, paramKey);
            return HDF_FAILURE;
    }
    return ret;
}

int32_t GetMppParam(RKHdiBaseComponent* component, Param *param)
{
    int32_t ret = HDF_SUCCESS;
    int32_t paramKey = param->key;

    switch (paramKey) {
        case KEY_INPUT_BUFFER_COUNT:
            ret = GetParamInputBufferCount(component, param);
            break;
        case KEY_OUTPUT_BUFFER_COUNT:
            ret = GetParamOutputBufferCount(component, param);
            break;
        case KEY_BUFFERSIZE:
            ret = GetParamBufferSize(component, param);
            break;
        case KEY_VIDEO_WIDTH:
            ret = GetParamWidth(component, param);
            break;
        case KEY_VIDEO_HEIGHT:
            ret = GetParamHeight(component, param);
            break;
        case KEY_PIXEL_FORMAT:
            ret = GetParamPixleFmt(component, param);
            break;
        case KEY_VIDEO_STRIDE:
            ret = GetParamStride(component, param);
            break;
        case KEY_VIDEO_FRAME_RATE:
            ret = GetParamFps(component, param);
            break;
        case KEY_VIDEO_RC_MODE:
            ret = GetParamRateControl(component, param);
            break;
        case KEY_VIDEO_GOP_MODE:
            ret = GetParamGop(component, param);
            break;
        case KEY_MIMETYPE:
            ret = GetParamMimeCodecType(component, param);
            break;
        case KEY_CODEC_TYPE:
            ret = GetParamCodecType(component, param);
            break;
        default:
            ret = GetExtMppParam(component, param);
    }
    return ret;
}

int32_t CodecGetParameter(CODEC_HANDLETYPE handle, Param *params, int32_t paramCnt)
{
    int32_t ret = HDF_SUCCESS;
    MppCtx ctx = handle;
    RKHdiBaseComponent* component = FindInMppComponentManager(handle);

    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    if (ctx != component->ctx) {
        HDF_LOGE("%{public}s: ctx not match %{public}d", __func__, ctx);
        return HDF_FAILURE;
    }

    for (int32_t i = 0; i < paramCnt; i++) {
        ret = GetMppParam(component, params + i);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%{public}s: GetMppParam faild, param key:%{public}d", __func__, params[i].key);
            return ret;
        }
    }

    return HDF_SUCCESS;
}

int32_t CodecStart(CODEC_HANDLETYPE handle)
{
    RKHdiBaseComponent* component = FindInMppComponentManager(handle);

    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    int32_t ret = HDF_FAILURE;
    if (component->ctxType == MPP_CTX_ENC) {
        ret = SetEncCfg(component);
    } else if (component->ctxType == MPP_CTX_DEC) {
        ret = SetDecCfg(component);
    }
    return ret;
}

int32_t CodecStop(CODEC_HANDLETYPE handle)
{
    RKHdiBaseComponent* component = FindInMppComponentManager(handle);

    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t CodecFlush(CODEC_HANDLETYPE handle, DirectionType directType)
{
    MPP_RET ret = MPP_OK;
    MppCtx ctx = handle;
    RKHdiBaseComponent* component = FindInMppComponentManager(handle);
    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    switch (directType) {
        case INPUT_TYPE:
        case OUTPUT_TYPE:
        case ALL_TYPE:
            ret = component->mpi->reset(ctx);
            if (ret != 0) {
                HDF_LOGE("%{public}s: reset failed", __func__);
                return HDF_FAILURE;
            }
            break;
        default:
            HDF_LOGE("%{public}s: directType failed", __func__);
            return HDF_FAILURE;
    }

    if (component->pCallbacks != NULL) {
        UINTPTR userData = (UINTPTR)component->ctx;
        EventType event = EVENT_FLUSH_COMPLETE;
        uint32_t length = 0;
        int32_t *eventData = NULL;
        component->pCallbacks->OnEvent(userData, event, length, eventData);
    }

    return HDF_SUCCESS;
}

int32_t DecodeInitPacket(RKHdiBaseComponent* component, MppPacket *pPacket, CodecBuffer *inputData, RK_U32 pkt_eos)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = component->mppApi;
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

MPP_RET DecodeGetFrame(RKHdiBaseComponent* component, MppCtx ctx, MppFrame *frame)
{
    MppApi *mpi = component->mpi;
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

int32_t HandleDecodeFrameInfoChange(RKHdiBaseComponent* component, MppFrame frame, MppCtx ctx)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = component->mppApi;
    MppApi *mpi = component->mpi;
    RK_U32 width = mppApi->HdiMppFrameGetWidth(frame);
    RK_U32 height = mppApi->HdiMppFrameGetHeight(frame);
    RK_U32 hor_stride = mppApi->HdiMppFrameGetHorStride(frame);
    RK_U32 ver_stride = mppApi->HdiMppFrameGetVerStride(frame);
    RK_U32 buf_size = mppApi->HdiMppFrameGetBufferSize(frame);

    component->setup.width = width;
    component->setup.height = height;
    CheckSetupStride(component);

    HDF_LOGI("%{public}s: decode_get_frame get info changed found", __func__);
    HDF_LOGI("%{public}s: decoder require buffer w:h [%{public}d:%{public}d]", __func__, width, height);
    HDF_LOGI("%{public}s: decoder require stride [%{public}d:%{public}d]", __func__, hor_stride, ver_stride);
    HDF_LOGI("%{public}s: decoder require buf_size %{public}d", __func__, buf_size);

    if (component->frmGrp == NULL) {
        ret = mppApi->HdiMppBufferGroupGet(&component->frmGrp,
            MPP_BUFFER_TYPE_DRM, MPP_BUFFER_INTERNAL, NULL, __func__);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: get mpp buffer group failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
        ret = mpi->control(ctx, MPP_DEC_SET_EXT_BUF_GROUP, component->frmGrp);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: set buffer group failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    } else {
        ret = mppApi->HdiMppBufferGroupClear(component->frmGrp);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: clear buffer group failed ret %{public}d", __func__, ret);
            return HDF_FAILURE;
        }
    }
    
    ret = mppApi->HdiMppBufferGroupLimitConfig(component->frmGrp, buf_size, BUFFER_GROUP_LIMIT_NUM);
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

static IM_STATUS PutDecodeFrameToOutput(RKHdiBaseComponent* component, MppFrame frame, CodecBuffer *outInfo)
{
    RKMppApi *mppApi = component->mppApi;
    MppBuffer mppBuffer = mppApi->HdiMppFrameGetBuffer(frame);
    rga_buffer_t src;
    rga_buffer_t dst;
    im_rect rect;
    
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
    if (outInfo->buffer[0].buf == 0) {
        HDF_LOGE("%{public}s: output buf invalid", __func__);
        return IM_STATUS_INVALID_PARAM;
    }

    src.fd = mppApi->HdiMppBufferGetFdWithCaller(mppBuffer, __func__);
    src.width    = mppApi->HdiMppFrameGetWidth(frame);
    src.height   = mppApi->HdiMppFrameGetHeight(frame);
    src.wstride  = mppApi->HdiMppFrameGetHorStride(frame);
    src.hstride  = mppApi->HdiMppFrameGetVerStride(frame);
    src.format   = RK_FORMAT_YCbCr_420_SP;

    if (outInfo->buffer[0].type == BUFFER_TYPE_HANDLE) {
        BufferHandle *bufferHandle = (BufferHandle *)outInfo->buffer[0].buf;
        dst.fd = bufferHandle->fd;
    } else {
        dst.vir_addr = (void *)outInfo->buffer[0].buf;
    }
    dst.width    = src.width;
    dst.height   = src.height;
    dst.wstride  = component->setup.stride.horStride;
    dst.hstride  = component->setup.stride.verStride;
    dst.format   = ConvertHdiFormat2RgaFormat(component->setup.fmt);

    rect.x = 0;
    rect.y = 0;
    rect.width = dst.width;
    rect.height = dst.height;
    return imcrop(src, dst, rect);
}

void HandleDecodeFrameOutput(RKHdiBaseComponent* component, MppFrame frame, int32_t frm_eos, CodecBuffer *outInfo)
{
    RKMppApi *mppApi = component->mppApi;
    RK_U32 err_info = mppApi->HdiMppFrameGetErrinfo(frame);
    RK_U32 discard = mppApi->HdiMppFrameGetDiscard(frame);
    component->frameCount++;
    
    if ((err_info | discard) != 0) {
        component->frameErr++;
        HDF_LOGE("%{public}s: bad output data, err_info: %{public}d", __func__, err_info);
        return;
    }
    if (outInfo == NULL || outInfo->bufferCnt == 0) {
        HDF_LOGE("%{public}s: outInfo param invalid!", __func__);
        return;
    }
    // have output data
    IM_STATUS ret = PutDecodeFrameToOutput(component, frame, outInfo);
    if (ret != IM_STATUS_SUCCESS) {
        HDF_LOGE("%{public}s: copy decode output data failed, error code: %{public}d", __func__, ret);
    }

    if (frm_eos != 0) {
        outInfo->flag |= STREAM_FLAG_EOS;
        HDF_LOGI("%{public}s: dec reach STREAM_FLAG_EOS, frame count : %{public}d, error count : %{public}d",
            __func__, component->frameCount, component->frameErr);
    }
    UINTPTR userData = (UINTPTR)component->ctx;
    int32_t acquireFd = 1;
    component->pCallbacks->OutputBufferAvailable(userData, outInfo, &acquireFd);
}

int32_t HandleDecodedFrame(RKHdiBaseComponent* component, MppFrame frame, MppCtx ctx,
    int32_t frm_eos, CodecBuffer *outInfo)
{
    RKMppApi *mppApi = component->mppApi;
    if (frame) {
        if (mppApi->HdiMppFrameGetInfoChange(frame)) {
            if (HandleDecodeFrameInfoChange(component, frame, ctx) != HDF_SUCCESS) {
                HDF_LOGE("%{public}s: func failed!", __func__);
                mppApi->HdiMppFrameDeinit(&frame);
                return HDF_FAILURE;
            }
        } else {
            HandleDecodeFrameOutput(component, frame, frm_eos, outInfo);
        }
        mppApi->HdiMppFrameDeinit(&frame);
    }

    // try get runtime frame memory usage
    if (component->frmGrp) {
        size_t usage = mppApi->HdiMppBufferGroupUsage(component->frmGrp);
        if (usage > component->maxUsage)
            component->maxUsage = usage;
    }
    return HDF_SUCCESS;
}

RK_U32 CodecDecodeGetFrameLoop(RKHdiBaseComponent* component, MppCtx ctx, RK_U32 pkt_done,
    RK_U32 pkt_eos, CodecBuffer *outInfo)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = component->mppApi;
    RK_U32 frm_eos = 0;
    MppFrame frame = NULL;

    do {
        RK_S32 get_frm = 0;
        ret = DecodeGetFrame(component, ctx, &frame);
        if (ret != MPP_OK) {
            HDF_LOGE("%{public}s: decode_get_frame failed, ret:%{public}d", __func__, ret);
            break;
        }
        if (frame) {
            frm_eos = mppApi->HdiMppFrameGetEos(frame);
            get_frm = 1;
            if (HandleDecodedFrame(component, frame, ctx, frm_eos, outInfo) != HDF_SUCCESS) {
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

        if ((component->frameNum > 0 && (component->frameCount >= component->frameNum)) ||
            ((component->frameNum == 0) && frm_eos != 0)) {
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
    RK_U32 loop_end = 0;
    RK_U32 frm_eos;
    RK_U32 pkt_done = 0;
    RK_U32 pkt_eos = (inputData->flag == STREAM_FLAG_EOS) ? 1 : 0;

    RKHdiBaseComponent* component = FindInMppComponentManager(handle);
    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    MppApi *mpi = component->mpi;
    MppPacket packet = component->packet;

    if (DecodeInitPacket(component, &packet, inputData, pkt_eos) != HDF_SUCCESS) {
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

        frm_eos = CodecDecodeGetFrameLoop(component, ctx, pkt_done, pkt_eos, outInfo);
        if ((component->frameNum > 0 && (component->frameCount >= component->frameNum)) ||
            ((component->frameNum == 0) && frm_eos != 0)) {
            loop_end = 1;
            break;
        }
        if (pkt_done) {
            break;
        }
        usleep(SLEEP_INTERVAL_MICROSECONDS);
    } while (1);
    UINTPTR userData = (UINTPTR)component->ctx;
    int32_t acquireFd = 1;
    component->pCallbacks->InputBufferAvailable(userData, inputData, &acquireFd);

    if (loop_end != 1) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static IM_STATUS GetEncodeFrameFromInput(RKHdiBaseComponent* component, MppFrame frame,
    MppBuffer mppBuffer, CodecBuffer *inputInfo)
{
    RKMppApi *mppApi = component->mppApi;
    rga_buffer_t src;
    rga_buffer_t dst;
    im_rect rect;
    
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
    if (inputInfo->buffer[0].buf == 0) {
        HDF_LOGE("%{public}s: output buf invalid", __func__);
        return IM_STATUS_INVALID_PARAM;
    }

    dst.fd = mppApi->HdiMppBufferGetFdWithCaller(mppBuffer, __func__);
    dst.wstride  = mppApi->HdiMppFrameGetHorStride(frame);
    dst.hstride  = mppApi->HdiMppFrameGetVerStride(frame);
    dst.width    = mppApi->HdiMppFrameGetWidth(frame);
    dst.height   = mppApi->HdiMppFrameGetHeight(frame);
    dst.format   = ConvertMppFormat2RgaFormat(component->fmt);

    if (inputInfo->buffer[0].type == BUFFER_TYPE_HANDLE) {
        BufferHandle *bufferHandle = (BufferHandle *)inputInfo->buffer[0].buf;
        src.fd = bufferHandle->fd;
    } else {
        src.vir_addr = (void *)inputInfo->buffer[0].buf;
    }
    src.width    = dst.width;
    src.height   = dst.height;
    src.wstride  = component->setup.stride.horStride;
    src.hstride  = component->setup.stride.verStride;
    src.format   = ConvertHdiFormat2RgaFormat(component->setup.fmt);

    rect.x = 0;
    rect.y = 0;
    rect.width = dst.width;
    rect.height = dst.height;
    return imcrop(src, dst, rect);
}

int32_t EncodeInitFrame(RKHdiBaseComponent* component, MppFrame *pFrame, RK_U32 frm_eos, CodecBuffer *inputData)
{
    MPP_RET ret = MPP_OK;
    RKMppApi *mppApi = component->mppApi;

    if (frm_eos != 0) {
        HDF_LOGI("%{public}s: receive eos frame", __func__);
    }
    ret = mppApi->HdiMppFrameInit(pFrame);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp_frame_init failed", __func__);
        return HDF_FAILURE;
    }
    mppApi->HdiMppFrameSetWidth(*pFrame, component->setup.width);
    mppApi->HdiMppFrameSetHeight(*pFrame, component->setup.height);
    mppApi->HdiMppFrameSetHorStride(*pFrame, component->horStride);
    mppApi->HdiMppFrameSetVerStride(*pFrame, component->verStride);
    mppApi->HdiMppFrameSetFormat(*pFrame, component->fmt);
    mppApi->HdiMppFrameSetEos(*pFrame, frm_eos);

    IM_STATUS status = GetEncodeFrameFromInput(component, *pFrame, component->frmBuf, inputData);
    if (status == IM_STATUS_SUCCESS) {
        mppApi->HdiMppFrameSetBuffer(*pFrame, component->frmBuf);
    } else {
        mppApi->HdiMppFrameDeinit(&pFrame);
        HDF_LOGE("%{public}s: copy encode input data failed, error code: %{public}d", __func__, ret);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t HandleEncodedPacket(RKHdiBaseComponent* component, MppPacket packet, RK_U32 pkt_eos, CodecBuffer *outInfo)
{
    RKMppApi *mppApi = component->mppApi;
    void *ptr   = mppApi->HdiMppPacketGetPos(packet);
    size_t len  = mppApi->HdiMppPacketGetLength(packet);
    pkt_eos = mppApi->HdiMppPacketGetEos(packet);

    // call back have out data
    UINTPTR userData = (UINTPTR)component->ctx;
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
            __func__, component->frameCount);
    }
    component->pCallbacks->OutputBufferAvailable(userData, outInfo, &acquireFd);

    return HDF_SUCCESS;
}

RK_U32 CodecEncodeGetPacketLoop(RKHdiBaseComponent* component, MppCtx ctx, CodecBuffer *outInfo)
{
    MPP_RET ret = MPP_OK;
    MppApi *mpi = component->mpi;
    RKMppApi *mppApi = component->mppApi;
    MppPacket packet = component->packet;
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
            HandleEncodedPacket(component, packet, pkt_eos, outInfo);

            /* for low delay partition encoding */
            if (mppApi->HdiMppPacketIsPartition(packet)) {
                eoi = mppApi->HdiMppPacketIsEoi(packet);
            }

            mppApi->HdiMppPacketDeinit(&packet);
            component->frameCount += eoi;

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

    RKHdiBaseComponent* component = FindInMppComponentManager(handle);
    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    MppApi *mpi = component->mpi;
    RKMppApi *mppApi = component->mppApi;

    if (EncodeInitFrame(component, &frame, frm_eos, inputData) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    ret = mpi->encode_put_frame(ctx, frame);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpp encode put frame failed", __func__);
        mppApi->HdiMppFrameDeinit(&frame);
        return HDF_FAILURE;
    }
    mppApi->HdiMppFrameDeinit(&frame);
    pkt_eos = CodecEncodeGetPacketLoop(component, ctx, outInfo);

    userData = (UINTPTR)component->ctx;
    component->pCallbacks->InputBufferAvailable(userData, inputData, &acquireFd);
    if (component->frameNum > 0 && component->frameCount >= component->frameNum) {
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
    MppPacket packet = NULL;

    RKHdiBaseComponent* component = FindInMppComponentManager(handle);
    if (component == NULL) {
        HDF_LOGE("%{public}s: component is NULL", __func__);
        return HDF_FAILURE;
    }
    RKMppApi *mppApi = component->mppApi;

    mppApi->HdiMppPacketInitWithBuffer(&packet, component->pktBuf);
    // NOTE: It is important to clear output packet length!!
    mppApi->HdiMppPacketSetLength(packet, 0);

    ret = component->mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
    if (ret != MPP_OK) {
        HDF_LOGE("%{public}s: mpi control enc get extra info failed", __func__);
        return HDF_FAILURE;
    } else {
        void *ptr   = mppApi->HdiMppPacketGetPos(packet);
        size_t len  = mppApi->HdiMppPacketGetLength(packet);

        // call back have out data
        UINTPTR userData = (UINTPTR)component->ctx;
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
        component->pCallbacks->OutputBufferAvailable(userData, &outInfo, &acquireFd);
    }

    mppApi->HdiMppPacketDeinit(&packet);
    return HDF_SUCCESS;
}
