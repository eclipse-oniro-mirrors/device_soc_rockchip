/*
 *
 * Copyright 2015 Rockchip Electronics Co., LTD.
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

#define MODULE_TAG "vpu_api"

#include "vpu_api.h"
#include <cstring>
#include <dlfcn.h>
#include <unistd.h>
#include "hdf_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"
#include "mpp_soc.h"
#include "vpu_api_legacy.h"
#include "vpu_mem_legacy.h"
#include "hdi_mpp_mpi.h"
#include "securec.h"

RKMppApi mRKMppApi;

static RK_S32
vpu_api_init(VpuCodecContext *ctx, RK_U8 *extraData, RK_U32 extra_size)
{
    HDF_LOGD("vpu_api_init in, extra_size: %d", extra_size);
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_init fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }
    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_init fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    return api->init(ctx, extraData, extra_size);
}

static RK_S32
vpu_api_decode(VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut)
{
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_decode fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_decode fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    return api->decode(ctx, pkt, aDecOut);
}
static RK_S32 vpu_api_sendstream(VpuCodecContext *ctx, VideoPacket_t *pkt)
{
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_decode fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_sendstream fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    return api->decode_sendstream(pkt);
}

static RK_S32 vpu_api_getframe(VpuCodecContext *ctx, DecoderOut_t *aDecOut)
{
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_decode fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_getframe fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    return api->decode_getoutframe(aDecOut);
}

static RK_S32
vpu_api_sendframe(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm)
{
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_decode fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_sendframe fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    return api->encoder_sendframe(ctx, aEncInStrm);
}

static RK_S32 vpu_api_getstream(VpuCodecContext *ctx, EncoderOut_t *aEncOut)
{
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_decode fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_getframe fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    return api->encoder_getstream(ctx, aEncOut);
}

static RK_S32
vpu_api_encode(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm,
               EncoderOut_t *aEncOut)
{
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_encode fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_encode fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    return api->encode(ctx, aEncInStrm, aEncOut);
}

static RK_S32 vpu_api_flush(VpuCodecContext *ctx)
{
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_encode fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_flush fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    return api->flush(ctx);
}

static RK_S32
vpu_api_control(VpuCodecContext *ctx, VPU_API_CMD cmdType, void *param)
{
    if (ctx == nullptr) {
        HDF_LOGE("%s vpu_api_decode fail, input invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    VpuApiLegacy* api = (VpuApiLegacy*)(ctx->vpuApiObj);
    if (api == nullptr) {
        HDF_LOGE("%s vpu_api_decode fail, vpu api invalid", __func__);
        return VPU_API_ERR_UNKNOW;
    }

    HDF_LOGD("enter");
    switch (cmdType) {
        case VPU_API_SET_VPUMEM_CONTEXT: {
            vpu_display_mem_pool_impl *p_mempool =
                (vpu_display_mem_pool_impl *)param;

        param = (void*)p_mempool->group;
        break;
        }
        default: {
            break;
        }
    }

    HDF_LOGD("pass to mpi");
    return api->control(ctx, cmdType, param);
}

static const char *codec_paths[] = {
    "/vendor/lib/librk_vpuapi.so",
    "/system/lib/librk_vpuapi.so",
    "/system/lib/librk_on2.so",
    "/usr/lib/librk_codec.so",
};

class VpulibDlsym {
public:
    void *rkapi_hdl;
    RK_S32 (*rkvpu_open_cxt)(VpuCodecContext **ctx);
    RK_S32 (*rkvpu_close_cxt)(VpuCodecContext **ctx);
    VpulibDlsym() :
    rkapi_hdl(nullptr),
    rkvpu_open_cxt(nullptr),
    rkvpu_close_cxt(nullptr) {
        RK_U32 i;
        for (i = 0; i < MPP_ARRAY_ELEMS(codec_paths); i++) {
            rkapi_hdl = dlopen(codec_paths[i], RTLD_LAZY | RTLD_GLOBAL);
            if (rkapi_hdl)
                break;
        }

        if (rkapi_hdl) {
            rkvpu_open_cxt  = (RK_S32 (*)(VpuCodecContext **ctx)) \
                                dlsym(rkapi_hdl, "vpu_open_context");
            rkvpu_close_cxt = (RK_S32 (*)(VpuCodecContext **ctx)) \
                                dlsym(rkapi_hdl, "vpu_close_context");
            HDF_LOGI("dlopen vpu lib %s success", codec_paths[i]);
        }
    }

    ~VpulibDlsym()
    {
        if (rkapi_hdl) {
            dlclose(rkapi_hdl);
            rkapi_hdl = nullptr;
        }
    }
};

static VpulibDlsym gVpulib;

static RK_S32 check_orign_vpu()
{
    return (gVpulib.rkapi_hdl) ? (MPP_OK) : (MPP_NOK);
}

static RK_S32 open_orign_vpu(VpuCodecContext **ctx)
{
    if (gVpulib.rkvpu_open_cxt && ctx) {
        return (gVpulib.rkvpu_open_cxt)(ctx);
    }
    return MPP_NOK;
}

static RK_S32 close_orign_vpu(VpuCodecContext **ctx)
{
    if (gVpulib.rkvpu_close_cxt && ctx) {
        return (gVpulib.rkvpu_close_cxt)(ctx);
    }
    return MPP_NOK;
}

/*
 * old libvpu path will input a nullptr pointer in *ctx
 * new libvpu path will input non-nullptr pointer in *ctx
 */
RK_S32 vpu_open_context(VpuCodecContext **ctx)
{
    GetMppAPI(&mRKMppApi);
    VpuCodecContext *s = *ctx;
    RK_S32 ret = -1;
    RK_U32 force_original = 0;
    RK_U32 force_mpp_mode = 0;
    RK_U32 use_mpp = 0;

    CODEC_TYPE codecType = CODEC_NONE;
    OMX_RK_VIDEO_CODINGTYPE videoCoding = OMX_RK_VIDEO_CodingUnused;
    RK_U32 width = 0;
    RK_U32 height = 0;
    void  *extradata = nullptr;
    RK_S32 extradata_size = 0;
    EXtraCfg_t extra_cfg;
    memset_s(&extra_cfg, sizeof(EXtraCfg_t), 0, sizeof(EXtraCfg_t));

    (*(mRKMppApi.HdiMppEnvGetU32))("vpu_api_debug", &vpu_api_debug, 0);
    HDF_LOGD("enter");

    (*(mRKMppApi.HdiMppEnvGetU32))("use_original", &force_original, 0);
    (*(mRKMppApi.HdiMppEnvGetU32))("use_mpp_mode", &force_mpp_mode, 0);

    /* if there is no original vpuapi library force to mpp path */
    if (check_orign_vpu())
        force_mpp_mode = 1;

    if (force_original) {
        /* force mpp mode here */
        use_mpp = 0;
    } else if (force_mpp_mode) {
        /* force mpp mode here */
        use_mpp = 1;
    } else if (!access("/dev/mpp_service", F_OK)) {
        /* if mpp_service exist, force mpp mode */
        use_mpp = 1;
    } else if (!!access("/dev/rkvdec", F_OK)) {
        /* if there is no rkvdec it means the platform must be vpu1 */
        if (s && s->videoCoding == OMX_RK_VIDEO_CodingHEVC &&
            (!access("/dev/hevc-service", F_OK)
             || !access("/dev/hevc_service", F_OK))) {
            /* if this is a hevc request and exist hevc_service for decoding use mpp */
            use_mpp = 1;
        } else {
            /* otherwise use original vpuapi path */
            use_mpp = 0;
        }
    } else if (nullptr == s) {
        /* caller is original vpuapi path. Force use original vpuapi path */
        use_mpp = 0;
    } else {
        if (s->videoCoding == OMX_RK_VIDEO_CodingAVC
            && s->codecType == CODEC_DECODER && s->width <= 1920 // width 1920
            && s->height <= 1088 && !s->extra_cfg.mpp_mode // height 1088
            && !strstr((*(mRKMppApi.Hdimpp_get_soc_name))(), "rk3399")) {
            /* H.264 smaller than 1080p use original vpuapi library for better error process */
            // NOTE: rk3399 need better performance
            use_mpp = 0;
        } else {
            MppCtxType type = (s->codecType == CODEC_DECODER) ? (MPP_CTX_DEC) :
                              (s->codecType == CODEC_ENCODER)
                              ? (MPP_CTX_ENC) : (MPP_CTX_BUTT);
            MppCodingType coding = (MppCodingType)s->videoCoding;

            if (MPP_OK == (*(mRKMppApi.HdiMppCheckSupportFormat))(type, coding)) {
                /* If current mpp can support this format use mpp */
                use_mpp = 1;
            } else {
                /* unsupport format use vpuapi library */
                use_mpp = 0;
            }
        }
    }

    /*
     * No matter what is the old context just release it.
     * But we need to save to pre-configured parameter
     */
    if (s) {
        codecType       = s->codecType;
        videoCoding     = s->videoCoding;
        width           = s->width;
        height          = s->height;
        extradata       = s->extradata;
        extradata_size  = s->extradata_size;
        extra_cfg       = s->extra_cfg;

        free(s);
        s = nullptr;
    }

    if (!use_mpp) {
        HDF_LOGI("use vpuapi path");

        ret = open_orign_vpu(&s);
        if (!ret && s) {
            // for safety
            s->extra_cfg.ori_vpu = 1;
            extra_cfg.ori_vpu = 1;
        }
    } else {
        HDF_LOGI("use mpp path");

        s = (VpuCodecContext*)(*(mRKMppApi.Hdimpp_osal_calloc))(__FUNCTION__, sizeof(VpuCodecContext));
        if (s) {
            s->enableparsing = 1;

            VpuApiLegacy* api = new VpuApiLegacy();

            if (api) {
                s->vpuApiObj = (void*)api;
                s->init = vpu_api_init;
                s->decode = vpu_api_decode;
                s->encode = vpu_api_encode;
                s->flush = vpu_api_flush;
                s->control = vpu_api_control;
                s->decode_sendstream = vpu_api_sendstream;
                s->decode_getframe = vpu_api_getframe;
                s->encoder_sendframe = vpu_api_sendframe;
                s->encoder_getstream = vpu_api_getstream;

                s->extra_cfg.ori_vpu = 0;
                extra_cfg.ori_vpu = 0;

                ret = 0;
            } else {
                HDF_LOGE("%s Vpu api object has not been properly allocated", __func__);
                mpp_free(s);
                s = nullptr;
            }
        } else {
            HDF_LOGE("%s Input context has not been properly allocated", __func__);
        }
    }

    if (s) {
        s->codecType        = codecType;
        s->videoCoding      = videoCoding;
        s->width            = width;
        s->height           = height;
        s->extradata        = extradata;
        s->extradata_size   = extradata_size;
        s->extra_cfg        = extra_cfg;
    }
    *ctx = s;

    HDF_LOGD("leave");
    return ret;
}

RK_S32 vpu_close_context(VpuCodecContext **ctx)
{
    HDF_LOGD("enter");
    VpuCodecContext *s = *ctx;
    RK_S32 ret = -1;
    RK_U32 force_original = 0;

    (*(mRKMppApi.HdiMppEnvGetU32))("force_original", &force_original, 0);

    if (s) {
        if (s->extra_cfg.ori_vpu) {
            ret = close_orign_vpu(ctx);
            HDF_LOGD("org vpu_close_context ok");
        } else {
            if (s->flush)
                s->flush(s);

            VpuApiLegacy* api = (VpuApiLegacy*)(s->vpuApiObj);
            if (s->vpuApiObj) {
                delete api;
                s->vpuApiObj = nullptr;
            }

            if (s->extradata_size > 0) {
                s->extradata_size = 0;
                s->extradata = nullptr;
            }

            if (s->private_data) {
                (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, s->private_data);
            }
            (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, s);
            ret = 0;
        }

        *ctx = s = nullptr;
    }
    HDF_LOGD("leave");

    return ret;
}
