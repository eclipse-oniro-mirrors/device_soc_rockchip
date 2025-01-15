/*
 *
 * Copyright 2020 Rockchip Electronics Co., LTD.
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

#define MODULE_TAG "vpu_api_mlvec"

#include "vpu_api_mlvec.h"
#include <fcntl.h>
#include "cstring"
#include "hdf_log.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "vpu_api_legacy.h"
#include "vpu_mem_legacy.h"
#include "securec.h"

#define VPU_API_DBG_MLVEC_FUNC      (0x00010000)
#define VPU_API_DBG_MLVEC_FLOW      (0x00020000)

typedef struct VpuApiMlvecImpl_t {
    MppCtx      mpp;
    MppApi      *mpi;
    MppEncCfg   enc_cfg;

    VpuApiMlvecStaticCfg    st_cfg;
    VpuApiMlvecDynamicCfg   dy_cfg;
} VpuApiMlvecImpl;

MPP_RET vpu_api_mlvec_init(VpuApiMlvec *ctx)
{
    if (ctx == nullptr) {
        HDF_LOGE("invalid nullptr input\n");
        return MPP_ERR_NULL_PTR;
    }

    HDF_LOGE("enter %p\n", ctx);

    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl*)(*(mRKMppApi.Hdimpp_osal_calloc))(__FUNCTION__, sizeof(VpuApiMlvecImpl));
    if (impl == nullptr)
        HDF_LOGE("failed to create MLVEC context\n");

    /* default disable frame_qp setup */
    impl->dy_cfg.frame_qp = -1;

    *ctx = impl;

    HDF_LOGE("leave %p %p\n", ctx, impl);
    return (impl) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET vpu_api_mlvec_deinit(VpuApiMlvec ctx)
{
    HDF_LOGE("enter %p\n", ctx);
    if (ctx) {
        (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, ctx);
    }
    ctx = nullptr;
    HDF_LOGE("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET vpu_api_mlvec_setup(VpuApiMlvec ctx, MppCtx mpp, MppApi *mpi, MppEncCfg enc_cfg)
{
    if (ctx == nullptr || mpp == nullptr || mpi == nullptr || enc_cfg == nullptr) {
        HDF_LOGE("invalid nullptr input ctx %p mpp %p mpi %p cfg %p\n",
            ctx, mpp, mpi, enc_cfg);
        return MPP_ERR_NULL_PTR;
    }

    HDF_LOGE("enter %p\n", ctx);

    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl *)ctx;
    impl->mpp = mpp;
    impl->mpi = mpi;
    impl->enc_cfg = enc_cfg;

    HDF_LOGE("leave %p\n", ctx);

    return MPP_OK;
}

MPP_RET vpu_api_mlvec_check_cfg(void *p)
{
    if (p == nullptr) {
        HDF_LOGE("invalid nullptr input\n");
        return MPP_ERR_NULL_PTR;
    }

    VpuApiMlvecStaticCfg *cfg = (VpuApiMlvecStaticCfg *)p;
    RK_U32 magic = cfg->magic;
    MPP_RET ret = MPP_OK;

    if ((((magic >> 24) & 0xff) != MLVEC_MAGIC) || // (magic >> 24)
        (((magic >> 16) & 0xff) != MLVEC_VERSION)) // (magic >> 16)
        ret = MPP_NOK;

    HDF_LOGE("check mlvec cfg magic %08x %s\n", magic,
        (ret == MPP_OK) ? "success" : "failed");

    return ret;
}

MPP_RET vpu_api_mlvec_set_st_cfg(VpuApiMlvec ctx, VpuApiMlvecStaticCfg *cfg)
{
    if (ctx == nullptr || cfg == nullptr) {
        HDF_LOGE("invalid nullptr input ctx %p cfg %p\n");
        return MPP_ERR_NULL_PTR;
    }

    HDF_LOGE("enter ctx %p cfg %p\n", ctx, cfg);

    /* check mlvec magic word */
    if (vpu_api_mlvec_check_cfg(cfg))
        return MPP_NOK;

    MPP_RET ret = MPP_OK;
    /* update static configure */
    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl *)ctx;

    if (memcpy_s(&impl->st_cfg, sizeof(impl->st_cfg), cfg, sizeof(impl->st_cfg)) != EOK) {
        HDF_LOGE("memcpy_s no");
    }
    cfg = &impl->st_cfg;

    /* get mpp context and check */
    MppCtx mpp_ctx = impl->mpp;
    MppApi *mpi = impl->mpi;
    MppEncCfg enc_cfg = impl->enc_cfg;

    /* start control mpp */
    HDF_LOGE("hdr_on_idr %d\n", cfg->hdr_on_idr);
    MppEncHeaderMode mode = cfg->hdr_on_idr ?
                            MPP_ENC_HEADER_MODE_EACH_IDR :
                            MPP_ENC_HEADER_MODE_DEFAULT;

    ret = mpi->control(mpp_ctx, MPP_ENC_SET_HEADER_MODE, &mode);
    if (ret)
        HDF_LOGE("setup enc header mode %d failed ret %d\n", mode, ret);

    HDF_LOGE("add_prefix %d\n", cfg->add_prefix);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:prefix_mode", cfg->add_prefix);

    HDF_LOGE("slice_mbs  %d\n", cfg->slice_mbs);
    if (cfg->slice_mbs) {
        (*(mRKMppApi.HdiMppEncCfgSetU32))(enc_cfg, "split:mode", MPP_ENC_SPLIT_BY_CTU);
        (*(mRKMppApi.HdiMppEncCfgSetU32))(enc_cfg, "split:arg", cfg->slice_mbs);
    } else
        (*(mRKMppApi.HdiMppEncCfgSetU32))(enc_cfg, "split:mode", MPP_ENC_SPLIT_NONE);

    /* NOTE: ltr_frames is already configured */
    vpu_api_mlvec_set_dy_max_tid(ctx, cfg->max_tid);

    HDF_LOGE("leave ctx %p ret %d\n", ctx, ret);

    return ret;
}

MPP_RET vpu_api_mlvec_set_dy_cfg(VpuApiMlvec ctx, VpuApiMlvecDynamicCfg *cfg, MppMeta meta)
{
    if (ctx == nullptr || cfg == nullptr || meta == nullptr) {
        HDF_LOGE("invalid nullptr input ctx %p cfg %p meta %p\n",
            ctx, cfg, meta);
        return MPP_ERR_NULL_PTR;
    }

    HDF_LOGE("enter ctx %p cfg %p meta %p\n", ctx, cfg, meta);

    MPP_RET ret = MPP_OK;
    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl *)ctx;
    VpuApiMlvecDynamicCfg *dst = &impl->dy_cfg;

    /* clear non-sticky flag first */
    dst->mark_ltr   = -1;
    dst->use_ltr    = -1;
    /* frame qp and base layer pid is sticky flag */

    /* update flags */
    if (cfg->updated) {
        if (cfg->updated & VPU_API_ENC_MARK_LTR_UPDATED)
            dst->mark_ltr = cfg->mark_ltr;

        if (cfg->updated & VPU_API_ENC_USE_LTR_UPDATED)
            dst->use_ltr = cfg->use_ltr;

        if (cfg->updated & VPU_API_ENC_FRAME_QP_UPDATED)
            dst->frame_qp = cfg->frame_qp;

        if (cfg->updated & VPU_API_ENC_BASE_PID_UPDATED)
            dst->base_layer_pid = cfg->base_layer_pid;

        /* dynamic max temporal layer count updated go through mpp ref cfg */
        cfg->updated = 0;
    }

    HDF_LOGE("ltr mark %2d use %2d frm qp %2d blpid %d\n", dst->mark_ltr,
        dst->use_ltr, dst->frame_qp, dst->base_layer_pid);

    /* setup next frame configure */
    if (dst->mark_ltr >= 0)
        (*(mRKMppApi.Hdimpp_meta_set_s32))(meta, KEY_ENC_MARK_LTR, dst->mark_ltr);

    if (dst->use_ltr >= 0)
        (*(mRKMppApi.Hdimpp_meta_set_s32))(meta, KEY_ENC_USE_LTR, dst->use_ltr);

    if (dst->frame_qp >= 0)
        (*(mRKMppApi.Hdimpp_meta_set_s32))(meta, KEY_ENC_FRAME_QP, dst->frame_qp);

    if (dst->base_layer_pid >= 0)
        (*(mRKMppApi.Hdimpp_meta_set_s32))(meta, KEY_ENC_BASE_LAYER_PID, dst->base_layer_pid);

    HDF_LOGE("leave ctx %p ret %d\n", ctx, ret);

    return ret;
}

MPP_RET vpu_api_mlvec_set_dy_max_tid(VpuApiMlvec ctx, RK_S32 max_tid)
{
    if (ctx == nullptr) {
        HDF_LOGE("invalid nullptr input\n");
        return MPP_ERR_NULL_PTR;
    }

    HDF_LOGE("enter ctx %p max_tid %d\n", ctx, max_tid);

    MPP_RET ret = MPP_OK;
    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl *)ctx;
    MppCtx mpp_ctx = impl->mpp;
    MppApi *mpi = impl->mpi;

    MppEncRefLtFrmCfg lt_ref[16];
    MppEncRefStFrmCfg st_ref[16];
    RK_S32 lt_cfg_cnt = 0;
    RK_S32 st_cfg_cnt = 0;
    RK_S32 tid0_loop = 0;
    RK_S32 ltr_frames = impl->st_cfg.ltr_frames;

    memset_s(lt_ref, sizeof(lt_ref), 0, sizeof(lt_ref));
    memset_s(st_ref, sizeof(st_ref), 0, sizeof(st_ref));

    HDF_LOGE("ltr_frames %d\n", ltr_frames);
    HDF_LOGE("max_tid    %d\n", max_tid);

    switch (max_tid) {
        case 0 : {
            st_ref[0].is_non_ref    = 0;
            st_ref[0].temporal_id   = 0;
            st_ref[0].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[0].ref_arg       = 0;
            st_ref[0].repeat        = 0;

            st_cfg_cnt = 1;
            tid0_loop = 1;
            HDF_LOGE("no tsvc\n");
            } break;
        case 1 : {
            /* set tsvc2 st-ref struct */
            /* st 0 layer 0 - ref */
            st_ref[0].is_non_ref    = 0;
            st_ref[0].temporal_id   = 0;
            st_ref[0].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[0].ref_arg       = 0;
            st_ref[0].repeat        = 0;
            /* st 1 layer 1 - non-ref */
            st_ref[1].is_non_ref    = 1;
            st_ref[1].temporal_id   = 1;
            st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[1].ref_arg       = 0;
            st_ref[1].repeat        = 0;
            /* st 2 layer 0 - ref */
            st_ref[2].is_non_ref    = 0; // st 2 layer 0 - ref
            st_ref[2].temporal_id   = 0; // st 2 layer 0 - ref
            st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM; // st 2 layer 0 - ref
            st_ref[2].ref_arg       = 0; // st 2 layer 0 - ref
            st_ref[2].repeat        = 0; // st 2 layer 0 - ref

            st_cfg_cnt = 3; // st_cfg_cnt = 3
            tid0_loop = 2; // tid0_loop = 2
            HDF_LOGE("tsvc2\n");
            } break;
        case 2 : { // st 2 layer
            /* set tsvc3 st-ref struct */
            /* st 0 layer 0 - ref */
            st_ref[0].is_non_ref    = 0;
            st_ref[0].temporal_id   = 0;
            st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
            st_ref[0].ref_arg       = 0;
            st_ref[0].repeat        = 0;
            /* st 1 layer 2 - non-ref */
            st_ref[1].is_non_ref    = 0;
            st_ref[1].temporal_id   = 2; // layer 2
            st_ref[1].ref_mode      = REF_TO_TEMPORAL_LAYER;
            st_ref[1].ref_arg       = 0;
            st_ref[1].repeat        = 0;
            /* st 2 layer 1 - ref */
            st_ref[2].is_non_ref    = 0; // layer 2
            st_ref[2].temporal_id   = 1; // layer 2
            st_ref[2].ref_mode      = REF_TO_TEMPORAL_LAYER; // layer 2
            st_ref[2].ref_arg       = 0; // layer 2
            st_ref[2].repeat        = 0; // layer 2
            /* st 3 layer 2 - non-ref */
            st_ref[3].is_non_ref    = 0; // layer 3
            st_ref[3].temporal_id   = 2; // layer 2 3
            st_ref[3].ref_mode      = REF_TO_TEMPORAL_LAYER; // layer 3
            st_ref[3].ref_arg       = 1; // layer 3
            st_ref[3].repeat        = 0; // layer 3
            /* st 4 layer 0 - ref */
            st_ref[4].is_non_ref    = 0; // layer 4
            st_ref[4].temporal_id   = 0; // layer 4
            st_ref[4].ref_mode      = REF_TO_TEMPORAL_LAYER; // layer 4
            st_ref[4].ref_arg       = 0; // layer 4
            st_ref[4].repeat        = 0; // layer 4

            st_cfg_cnt = 5; // st_cfg_cnt = 5
            tid0_loop = 4; // tid0_loop = 4
            HDF_LOGE("tsvc3\n");
            } break;
        case 3 : { // set tsvc3
            /* set tsvc3 st-ref struct */
            /* st 0 layer 0 - ref */
            st_ref[0].is_non_ref    = 0;
            st_ref[0].temporal_id   = 0;
            st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
            st_ref[0].ref_arg       = 0;
            st_ref[0].repeat        = 0;
                /* st 1 layer 3 - non-ref */
            st_ref[1].is_non_ref    = 1;
            st_ref[1].temporal_id   = 3; // layer 3
            st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[1].ref_arg       = 0;
            st_ref[1].repeat        = 0;
            /* st 2 layer 2 - ref */
            st_ref[2].is_non_ref    = 0; // st 2
            st_ref[2].temporal_id   = 2; // st 2
            st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM; // st 2
            st_ref[2].ref_arg       = 0; // st 2
            st_ref[2].repeat        = 0; // st 2
            /* st 3 layer 3 - non-ref */
            st_ref[3].is_non_ref    = 1; // st 3
            st_ref[3].temporal_id   = 3; // st 3
            st_ref[3].ref_mode      = REF_TO_PREV_REF_FRM; // st 3
            st_ref[3].ref_arg       = 0; // st 3
            st_ref[3].repeat        = 0; // st 3
            /* st 4 layer 1 - ref */
            st_ref[4].is_non_ref    = 0; // st 4
            st_ref[4].temporal_id   = 1; // st 4
            st_ref[4].ref_mode      = REF_TO_TEMPORAL_LAYER; // st 4
            st_ref[4].ref_arg       = 0; // st 4
            st_ref[4].repeat        = 0; // st 4
            /* st 5 layer 3 - non-ref */
            st_ref[5].is_non_ref    = 1; // st 5
            st_ref[5].temporal_id   = 3; // st 5 layer 3
            st_ref[5].ref_mode      = REF_TO_PREV_REF_FRM; // st 5
            st_ref[5].ref_arg       = 0; // st 5
            st_ref[5].repeat        = 0; // st 5
            /* st 6 layer 2 - ref */
            st_ref[6].is_non_ref    = 0; // st 6
            st_ref[6].temporal_id   = 2; // st 6 layer 2
            st_ref[6].ref_mode      = REF_TO_PREV_REF_FRM; // st 6
            st_ref[6].ref_arg       = 0; // st 6
            st_ref[6].repeat        = 0; // st 6
            /* st 7 layer 3 - non-ref */
            st_ref[7].is_non_ref    = 1; // st 7
            st_ref[7].temporal_id   = 3; // st 7 layer 3
            st_ref[7].ref_mode      = REF_TO_PREV_REF_FRM; // st 7
            st_ref[7].ref_arg       = 0; // st 7
            st_ref[7].repeat        = 0; // st 7
            /* st 8 layer 0 - ref */
            st_ref[8].is_non_ref    = 0; // st 8
            st_ref[8].temporal_id   = 0; // st 8
            st_ref[8].ref_mode      = REF_TO_PREV_REF_FRM; // st 8 layer 0 - ref
            st_ref[8].ref_arg       = 0; // st 8
            st_ref[8].repeat        = 0; // st 8

            st_cfg_cnt = 9; // st_cfg_cnt = 9
            tid0_loop = 8; // tid0_loop = 8
            HDF_LOGE("tsvc4\n");
            } break;
        default : {
            HDF_LOGE("invalid max temporal layer id %d\n", max_tid);
            } break;
        }

    if (ltr_frames) {
        RK_S32 i;

        lt_cfg_cnt = ltr_frames;
        for (i = 0; i < ltr_frames; i++) {
            lt_ref[i].lt_idx        = i;
            lt_ref[i].temporal_id   = 0;
            lt_ref[i].ref_mode      = REF_TO_PREV_LT_REF;
            lt_ref[i].lt_gap        = 0;
            lt_ref[i].lt_delay      = tid0_loop * i;
        }
    }

    HDF_LOGE("lt_cfg_cnt %d st_cfg_cnt %d\n", lt_cfg_cnt, st_cfg_cnt);
    if (lt_cfg_cnt || st_cfg_cnt) {
        MppEncRefCfg ref = nullptr;

        (*(mRKMppApi.HdiMppEncRefCfgInit))(&ref);

        ret = (*(mRKMppApi.Hdimpp_enc_ref_cfg_set_cfg_cnt))(ref, lt_cfg_cnt, st_cfg_cnt);
        ret = (*(mRKMppApi.Hdimpp_enc_ref_cfg_add_lt_cfg))(ref, lt_cfg_cnt, lt_ref);
        ret = (*(mRKMppApi.Hdimpp_enc_ref_cfg_add_st_cfg))(ref, st_cfg_cnt, st_ref);
        ret = (*(mRKMppApi.Hdimpp_enc_ref_cfg_set_keep_cpb))(ref, 1);
        ret = (*(mRKMppApi.Hdimpp_enc_ref_cfg_check))(ref);

        ret = mpi->control(mpp_ctx, MPP_ENC_SET_REF_CFG, ref);
        if (ret)
            HDF_LOGE("mpi control enc set ref cfg failed ret %d\n", ret);

        (*(mRKMppApi.HdiMppEncRefCfgDeinit))(&ref);
    } else {
        ret = mpi->control(mpp_ctx, MPP_ENC_SET_REF_CFG, nullptr);
        if (ret)
            HDF_LOGE("mpi control enc set ref cfg failed ret %d\n", ret);
    }

    HDF_LOGE("leave ctx %p ret %d\n", ctx, ret);

    return ret;
}
