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

#define MODULE_TAG "vpu_api_legacy"

#include "vpu_api_legacy.h"
#include <fcntl.h>
#include "cstring"
#include "hdf_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer_impl.h"
#include "mpp_frame.h"
#include "vpu_mem_legacy.h"
#include "securec.h"

#define VPU_API_ENC_INPUT_TIMEOUT 100

RK_U32 vpu_api_debug = 0;

static MppFrameFormat vpu_pic_type_remap_to_mpp(EncInputPictureType type)
{
    MppFrameFormat ret = MPP_FMT_BUTT;
    switch (type) {
        case ENC_INPUT_YUV420_PLANAR : {
            ret = MPP_FMT_YUV420P;
            } break;
        case ENC_INPUT_YUV420_SEMIPLANAR : {
            ret = MPP_FMT_YUV420SP;
            } break;
        case ENC_INPUT_YUV422_INTERLEAVED_YUYV : {
            ret = MPP_FMT_YUV422_YUYV;
            } break;
        case ENC_INPUT_YUV422_INTERLEAVED_UYVY : {
            ret = MPP_FMT_YUV422_UYVY;
            } break;
        case ENC_INPUT_RGB565 : {
            ret = MPP_FMT_RGB565;
            } break;
        case ENC_INPUT_BGR565 : {
            ret = MPP_FMT_BGR565;
            } break;
        case ENC_INPUT_RGB555 : {
            ret = MPP_FMT_RGB555;
            } break;
        case ENC_INPUT_BGR555 : {
            ret = MPP_FMT_BGR555;
            } break;
        case ENC_INPUT_RGB444 : {
            ret = MPP_FMT_RGB444;
            } break;
        case ENC_INPUT_BGR444 : {
            ret = MPP_FMT_BGR444;
            } break;
        case ENC_INPUT_RGB888 : {
            ret = MPP_FMT_RGBA8888;
            } break;
        case ENC_INPUT_BGR888 : {
            ret = MPP_FMT_BGRA8888;
            } break;
        case ENC_INPUT_RGB101010 : {
            ret = MPP_FMT_RGB101010;
            } break;
        case ENC_INPUT_BGR101010 : {
            ret = MPP_FMT_BGR101010;
            } break;
        default : {
            HDF_LOGE("There is no match format, err!!!!!!");
            } break;
    }
    return ret;
}

static MPP_RET vpu_api_set_enc_cfg(MppCtx mpp_ctx, MppApi *mpi, MppEncCfg enc_cfg,
                                   MppCodingType coding, MppFrameFormat fmt,
                                   EncParameter_t *cfg)
{
    MPP_RET ret = MPP_OK;
    RK_S32 width    = cfg->width;
    RK_S32 height   = cfg->height;
    RK_S32 bps      = cfg->bitRate;
    RK_S32 fps_in   = cfg->framerate;
    RK_S32 fps_out  = (cfg->framerateout) ? (cfg->framerateout) : (fps_in);
    RK_S32 gop      = (cfg->intraPicRate) ? (cfg->intraPicRate) : (fps_out);
    RK_S32 qp_init  = (coding == MPP_VIDEO_CodingAVC) ? (26) : // qp_init 26
                      (coding == MPP_VIDEO_CodingMJPEG) ? (10) : // qp_init 10
                      (coding == MPP_VIDEO_CodingVP8) ? (56) : // qp_init 56
                      (coding == MPP_VIDEO_CodingHEVC) ? (26) : (0); // qp_init 26
    RK_S32 qp       = (cfg->qp) ? (cfg->qp) : (qp_init);
    RK_S32 profile  = cfg->profileIdc;
    RK_S32 level    = cfg->levelIdc;
    RK_S32 cabac_en = cfg->enableCabac;
    RK_S32 rc_mode  = cfg->rc_mode;
    RK_U32 is_fix_qp = (rc_mode == MPP_ENC_RC_MODE_FIXQP) ? 1 : 0;

    HDF_LOGE("setup encoder rate control config:\n");
    HDF_LOGE("width %4d height %4d format %d:%x\n", width, height, cfg->format, fmt);
    HDF_LOGE("rc_mode %s qp %d bps %d\n", (rc_mode) ? ("CBR") : ("CQP"), qp, bps);
    HDF_LOGE("fps in %d fps out %d gop %d\n", fps_in, fps_out, gop);
    HDF_LOGE("setup encoder stream feature config:\n");
    HDF_LOGE("profile %d level %d cabac %d\n", profile, level, cabac_en);

    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "prep:width", width);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "prep:height", height);
    switch (fmt & MPP_FRAME_FMT_MASK) {
        case MPP_FMT_YUV420SP :
        case MPP_FMT_YUV420SP_VU : {
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, \
                "prep:hor_stride", MPP_ALIGN(width, 16)); // hor_stride 16
            } break;
        case MPP_FMT_RGB565:
        case MPP_FMT_BGR565:
        case MPP_FMT_RGB555:
        case MPP_FMT_BGR555: {
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "prep:hor_stride", \
                2 * MPP_ALIGN(width, 16)); // hor_stride 2 * 16
            } break;
        case MPP_FMT_ARGB8888 :
        case MPP_FMT_ABGR8888 :
        case MPP_FMT_BGRA8888 :
        case MPP_FMT_RGBA8888 : {
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "prep:hor_stride", 4 * MPP_ALIGN(width, 16)); // hor_stride 4 * 16
            } break;
        default: {
            HDF_LOGE("unsupport format 0x%x\n", fmt & MPP_FRAME_FMT_MASK);
            } break;
    }
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "prep:ver_stride", MPP_ALIGN(height, 8)); // height 8 bit
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "prep:format", fmt);

    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:mode", is_fix_qp ? MPP_ENC_RC_MODE_FIXQP :
                        (rc_mode ? MPP_ENC_RC_MODE_CBR : MPP_ENC_RC_MODE_VBR));
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:bps_target", bps);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:bps_max", bps * 17 / 16); // bps_max  17 / 16
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:bps_min", rc_mode ? bps * 15 / 16 : bps * 1 / 16); // bps_min 15 / 16
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:fps_in_flex", 0);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:fps_in_num", fps_in);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:fps_in_denorm", 1);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:fps_out_flex", 0);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:fps_out_num", fps_out);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:fps_out_denorm", 1);
    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "rc:gop", gop);

    (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "codec:type", coding);
    switch (coding) {
        case MPP_VIDEO_CodingAVC : {
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:profile", profile);
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:level", level);
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:cabac_en", cabac_en);
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:cabac_idc", 0);
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:qp_init", is_fix_qp ? qp : -1);
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:qp_min", is_fix_qp  ? qp : 10); // qp_min 10
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:qp_max", is_fix_qp ? qp : 51); // h264:qp_max 51
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:qp_min_i", 10); // qp_min_i 10
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:qp_max_i", 51); // qp_max_i 51
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:qp_step", 4); // qp_step 4
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "h264:qp_delta_ip", 3); // qp_delta_ip 3
            } break;
        case MPP_VIDEO_CodingVP8 : {
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "vp8:qp_init", -1);
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "vp8:qp_min", 0); // qp_min 0
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "vp8:qp_max", 127); // qp_max 127
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "vp8:qp_min_i", 0); // qp_min_i 0
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "vp8:qp_max_i", 127); // qp_max_i 127
            } break;
        case MPP_VIDEO_CodingMJPEG : {
            (*(mRKMppApi.HdiMppEncCfgSetS32))(enc_cfg, "jpeg:quant", qp);
            } break;
        default : {
            HDF_LOGE("support encoder coding type %d\n", coding);
            } break;
    }

    ret = mpi->control(mpp_ctx, MPP_ENC_SET_CFG, enc_cfg);
    if (ret) {
        HDF_LOGE("setup enc config failed ret %d\n", ret);
        goto RET;
    }
RET:
    return ret;
}

static int is_valid_dma_fd(int fd)
{
    int ret = 1;
    /* detect input file handle */
    int fs_flag = fcntl(fd, F_GETFL, nullptr);
    int fd_flag = fcntl(fd, F_GETFD, nullptr);
    if (fs_flag == -1 || fd_flag == -1) {
        ret = 0;
    }

    return ret;
}

static int copy_align_raw_buffer_to_dest(RK_U8 *dst, RK_U8 *src, RK_U32 width,
                                         RK_U32 height, MppFrameFormat fmt)
{
    int ret = 1;
    RK_U32 index = 0;
    RK_U8 *dst_buf = dst;
    RK_U8 *src_buf = src;
    RK_U32 row = 0;
    RK_U32 hor_stride = MPP_ALIGN(width, 16); // width 16
    RK_U32 ver_stride = MPP_ALIGN(height, 8); // height 8
    RK_U8 *dst_u = dst_buf + hor_stride * ver_stride;
    RK_U8 *dst_v = dst_u + hor_stride * ver_stride / 4;

    switch (fmt) {
        case MPP_FMT_YUV420SP : {
            for (row = 0; row < height; row++) {
                if (memcpy_s(dst_buf + row * hor_stride, width, src_buf + index, width) != EOK) {
                    HDF_LOGE("memcpy_s no");
                }
                index += width;
            }
            for (row = 0; row < height / 2; row++) { // height : / 2
                if (memcpy_s(dst_u + row * hor_stride, width, src_buf + index, width) != EOK) {
                    HDF_LOGE("memcpy_s no");
                }
                index += width;
            }
            } break;
        case MPP_FMT_YUV420P : {
            for (row = 0; row < height; row++) {
                if (memcpy_s(dst_buf + row * hor_stride, width, src_buf + index, width) != EOK) {
                    HDF_LOGE("memcpy_s no");
                }
                index += width;
            }
            for (row = 0; row < height / 2; row++) { // height : / 2
                if (memcpy_s(dst_u + row * hor_stride / 2, // height : / 2
                    width / 2, src_buf + index, width / 2) != EOK) { // height : / 2
                    HDF_LOGE("memcpy_s no");
                }
                index += width / 2; // width / 2
            }
            for (row = 0; row < height / 2; row++) { // height : / 2
                if (memcpy_s(dst_v + row * hor_stride / 2, // height : / 2
                    width / 2, src_buf + index, width / 2) != EOK) { // height : / 2
                    HDF_LOGE("memcpy_s no");
                }
                index += width / 2; // width / 2
            }
            } break;
        case MPP_FMT_RGBA8888 :
        case MPP_FMT_BGRA8888 :
        case MPP_FMT_ABGR8888 :
        case MPP_FMT_ARGB8888 : {
            for (row = 0; row < height; row++) {
                if (memcpy_s(dst_buf + row * hor_stride * 4, // width 4
                    width * 4, src_buf + row * width * 4, width * 4) != EOK) { // width 4
                    HDF_LOGE("memcpy_s no");
                }
                }
            } break;
        default : {
            HDF_LOGE("unsupport align fmt:%d now\n", fmt);
            } break;
        }

    return ret;
}

VpuApiLegacy::VpuApiLegacy() : mpp_ctx(nullptr),
    mpi(nullptr),
    init_ok(0),
    frame_count(0),
    set_eos(0),
    memGroup(nullptr),
    format(MPP_FMT_YUV420P),
    fd_input(-1),
    fd_output(-1),
    mEosSet(0),
    enc_cfg(nullptr),
    enc_hdr_pkt(nullptr),
    enc_hdr_buf(nullptr),
    enc_hdr_buf_size(0)
{
    HDF_LOGE("enter\n");

    (*(mRKMppApi.HdiMppCreate))(&mpp_ctx, &mpi);

    memset_s(&enc_param, sizeof(enc_param), 0, sizeof(enc_param));

    mlvec = nullptr;
    memset_s(&mlvec_dy_cfg, sizeof(mlvec_dy_cfg), 0, sizeof(mlvec_dy_cfg));

    HDF_LOGE("leave\n");
}

VpuApiLegacy::~VpuApiLegacy()
{
    HDF_LOGE("enter\n");

    (*(mRKMppApi.HdiMppDestroy))(mpp_ctx);

    if (memGroup) {
        (*(mRKMppApi.HdiMppBufferGroupPut))(memGroup);
        memGroup = nullptr;
    }

    if (enc_cfg) {
        (*(mRKMppApi.HdiMppEncCfgDeinit))(enc_cfg);
        enc_cfg = nullptr;
    }

    if (mlvec) {
        vpu_api_mlvec_deinit(mlvec);
        mlvec = nullptr;
    }

    if (enc_hdr_pkt) {
        (*(mRKMppApi.HdiMppPacketDeinit))(&enc_hdr_pkt);
        enc_hdr_pkt = nullptr;
    }
    if (enc_hdr_buf) {
        (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, enc_hdr_buf);
    }
    enc_hdr_buf = nullptr;

    enc_hdr_buf_size = 0;

    HDF_LOGE("leave\n");
}

static RK_S32 init_frame_info(VpuCodecContext *ctx,
                              MppCtx mpp_ctx, MppApi *mpi, VPU_GENERIC *p)
{
    RK_S32 ret = -1;
    MppFrame frame = nullptr;
    RK_U32 fbcOutFmt = 0;

    if (ctx->private_data)
        fbcOutFmt = *(RK_U32 *)ctx->private_data;

    if (ctx->extra_cfg.bit_depth
        || ctx->extra_cfg.yuv_format) {
        if (ctx->extra_cfg.bit_depth == 10) // extra_cfg.bit_depth == 10
            p->CodecType = (ctx->extra_cfg.yuv_format == 1)
                           ? MPP_FMT_YUV422SP_10BIT : MPP_FMT_YUV420SP_10BIT;
        else
            p->CodecType = (ctx->extra_cfg.yuv_format == 1)
                           ? MPP_FMT_YUV422SP : MPP_FMT_YUV420SP;
    } else {
        /**hightest of p->ImgWidth bit show current dec bitdepth
          * 0 - 8bit
          * 1 - 10bit
          **/
        if (p->ImgWidth & 0x80000000)
            p->CodecType = (p->ImgWidth & 0x40000000)
                           ? MPP_FMT_YUV422SP_10BIT : MPP_FMT_YUV420SP_10BIT;
        else
            p->CodecType = (p->ImgWidth & 0x40000000)
                           ? MPP_FMT_YUV422SP : MPP_FMT_YUV420SP;
    }
    p->ImgWidth = (p->ImgWidth & 0xFFFF);

    (*(mRKMppApi.HdiMppFrameInit))(&frame);

    (*(mRKMppApi.HdiMppFrameSetWidth))(frame, p->ImgWidth);
    (*(mRKMppApi.HdiMppFrameSetHeight))(frame, p->ImgHeight);
    (*(mRKMppApi.HdiMppFrameSetFormat))(frame, (MppFrameFormat)(p->CodecType | fbcOutFmt));

    ret = mpi->control(mpp_ctx, MPP_DEC_SET_FRAME_INFO, (MppParam)frame);
    /* output the parameters used */
    p->ImgHorStride = (*(mRKMppApi.HdiMppFrameGetHorStride))(frame);
    p->ImgVerStride = (*(mRKMppApi.HdiMppFrameGetVerStride))(frame);
    p->BufSize = (*(mRKMppApi.HdiMppFrameGetBufferSize))(frame);

    (*(mRKMppApi.HdiMppFrameDeinit))(&frame);

    return ret;
}


RK_S32 VpuApiLegacy::init(VpuCodecContext *ctx, RK_U8 *extraData, RK_U32 extra_size)
{
    HDF_LOGE("enter\n");

    MPP_RET ret = MPP_OK;
    MppCtxType type;

    if (mpp_ctx == nullptr || mpi == nullptr) {
        HDF_LOGE("found invalid context input");
        return MPP_ERR_NULL_PTR;
    }

    if (CODEC_DECODER == ctx->codecType) {
        type = MPP_CTX_DEC;
    } else if (CODEC_ENCODER == ctx->codecType) {
        type = MPP_CTX_ENC;
    } else {
        HDF_LOGE("found invalid codec type %d\n", ctx->codecType);
        return MPP_ERR_VPU_CODEC_INIT;
    }

    ret = (*(mRKMppApi.HdiMppInit))(mpp_ctx, type, (MppCodingType)ctx->videoCoding);
    if (ret) {
        HDF_LOGE(" init error. \n");
        return ret;
    }

    if (MPP_CTX_ENC == type) {
        EncParameter_t *param = (EncParameter_t*)ctx->private_data;
        MppCodingType coding = (MppCodingType)ctx->videoCoding;
        MppPollType block = (MppPollType)VPU_API_ENC_INPUT_TIMEOUT;
        MppEncSeiMode sei_mode = MPP_ENC_SEI_MODE_DISABLE;

        /* setup input / output block mode */
        ret = mpi->control(mpp_ctx, MPP_SET_INPUT_TIMEOUT, (MppParam)&block);
        if (MPP_OK != ret)
            HDF_LOGE("mpi control MPP_SET_INPUT_TIMEOUT failed\n");

        /* disable sei by default */
        ret = mpi->control(mpp_ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
        if (ret)
            HDF_LOGE("mpi control MPP_ENC_SET_SEI_CFG failed ret %d\n", ret);

        if (memGroup == nullptr) {
            ret = (*(mRKMppApi.HdiMppBufferGroupGet)) \
                (&memGroup, MPP_BUFFER_TYPE_ION, MPP_BUFFER_INTERNAL, MODULE_TAG, __FUNCTION__);
            if (ret) {
                HDF_LOGE("memGroup (*(mRKMppApi.HdiMppBufferGroupGet)) failed %d\n", ret);
                return ret;
            }
        }

        ret = (*(mRKMppApi.HdiMppEncCfgInit))(&enc_cfg);
        if (ret) {
            HDF_LOGE("(*(mRKMppApi.HdiMppEncCfgInit)) failed %d\n", ret);
            (*(mRKMppApi.HdiMppBufferGroupPut))(memGroup);
            memGroup = nullptr;
            return ret;
        }

        format = vpu_pic_type_remap_to_mpp((EncInputPictureType)param->format);
        if (memcpy_s(&enc_param, sizeof(enc_param), param, sizeof(enc_param)) != EOK) {
            HDF_LOGE("memcpy_s no");
        }

        if (MPP_OK == vpu_api_mlvec_check_cfg(param)) {
            if (mlvec == nullptr) {
                vpu_api_mlvec_init(&mlvec);
                vpu_api_mlvec_setup(mlvec, mpp_ctx, mpi, enc_cfg);
            }
        }

        if (mlvec)
            vpu_api_mlvec_set_st_cfg(mlvec, (VpuApiMlvecStaticCfg *)param);

        vpu_api_set_enc_cfg(mpp_ctx, mpi, enc_cfg, coding, format, param);

        if (!mlvec) {
            if (enc_hdr_pkt == nullptr) {
                if (enc_hdr_buf == nullptr) {
                    enc_hdr_buf_size = SZ_1K;
                    enc_hdr_buf = (*(mRKMppApi.Hdimpp_osal_calloc))(__FUNCTION__, enc_hdr_buf_size);
                }

                if (enc_hdr_buf)
                    (*(mRKMppApi.HdiMppPacketInit))(&enc_hdr_pkt, enc_hdr_buf, enc_hdr_buf_size);
            }
            if (enc_hdr_pkt) {
                ret = mpi->control(mpp_ctx, MPP_ENC_GET_HDR_SYNC, enc_hdr_pkt);
                ctx->extradata_size = (*(mRKMppApi.HdiMppPacketGetLength))(enc_hdr_pkt);
                ctx->extradata      = (*(mRKMppApi.Hdimpp_packet_get_data))(enc_hdr_pkt);
            }
        }
    } else { /* MPP_CTX_DEC */
        vpug.CodecType  = ctx->codecType;
        vpug.ImgWidth   = ctx->width;
        vpug.ImgHeight  = ctx->height;

        init_frame_info(ctx, mpp_ctx, mpi, &vpug);

        if (extraData != nullptr) {
            MppPacket pkt = nullptr;

            (*(mRKMppApi.HdiMppPacketInit))(&pkt, extraData, extra_size);
            (*(mRKMppApi.Hdimpp_packet_set_extra_data))(pkt);
            mpi->decode_put_packet(mpp_ctx, pkt);
            (*(mRKMppApi.HdiMppPacketDeinit))(&pkt);
        }

        RK_U32 flag = 0;
        ret = mpi->control(mpp_ctx, MPP_DEC_SET_ENABLE_DEINTERLACE, &flag);
        if (ret)
            HDF_LOGE("disable mpp deinterlace failed ret %d\n", ret);
    }

    init_ok = 1;

    HDF_LOGE("leave\n");
    return ret;
}

RK_S32 VpuApiLegacy::flush(VpuCodecContext *ctx)
{
    (void)ctx;
    HDF_LOGE("enter\n");
    if (mpi && mpi->reset && init_ok) {
        mpi->reset(mpp_ctx);
        set_eos = 0;
        mEosSet = 0;
    }
    HDF_LOGE("leave\n");
    return 0;
}

static void setup_VPU_FRAME_from_mpp_frame(VPU_FRAME *vframe, MppFrame mframe)
{
    MppBuffer buf = (*(mRKMppApi.HdiMppFrameGetBuffer))(mframe);
    RK_U64 pts  = (*(mRKMppApi.Hdimpp_frame_get_pts))(mframe);
    RK_U32 mode = (*(mRKMppApi.Hdimpp_frame_get_mode))(mframe);

    MppFrameColorRange colorRan = (*(mRKMppApi.Hdimpp_frame_get_color_range))(mframe);
    MppFrameColorTransferCharacteristic colorTrc = (*(mRKMppApi.Hdimpp_frame_get_color_trc))(mframe);
    MppFrameColorPrimaries colorPri = (*(mRKMppApi.Hdimpp_frame_get_color_primaries))(mframe);
    MppFrameColorSpace colorSpa = (*(mRKMppApi.Hdimpp_frame_get_colorspace))(mframe);

    if (buf)
        (*(mRKMppApi.Hdimpp_buffer_inc_ref_with_caller))(buf, __FUNCTION__);

    vframe->DisplayWidth = (*(mRKMppApi.HdiMppFrameGetWidth))(mframe);
    vframe->DisplayHeight = (*(mRKMppApi.HdiMppFrameGetHeight))(mframe);
    vframe->FrameWidth = (*(mRKMppApi.HdiMppFrameGetHorStride))(mframe);
    vframe->FrameHeight = (*(mRKMppApi.HdiMppFrameGetVerStride))(mframe);

    vframe->ColorRange = (colorRan == MPP_FRAME_RANGE_JPEG);
    vframe->ColorPrimaries = colorPri;
    vframe->ColorTransfer = colorTrc;
    vframe->ColorCoeffs = colorSpa;

    if (mode == MPP_FRAME_FLAG_FRAME)
        vframe->FrameType = 0;
    else {
        RK_U32 field_order = mode & MPP_FRAME_FLAG_FIELD_ORDER_MASK;
        if (field_order == MPP_FRAME_FLAG_TOP_FIRST)
            vframe->FrameType = 1; // vframe->FrameType = 1
        else if (field_order == MPP_FRAME_FLAG_BOT_FIRST)
            vframe->FrameType = 2; // vframe->FrameType = 2
        else if (field_order == MPP_FRAME_FLAG_DEINTERLACED)
            vframe->FrameType = 4; // vframe->FrameType = 4
    }
    vframe->ErrorInfo = (*(mRKMppApi.HdiMppFrameGetErrinfo))(mframe) | (*(mRKMppApi.HdiMppFrameGetDiscard))(mframe);
    vframe->ShowTime.TimeHigh = (RK_U32)(pts >> 32); // (pts >> 32)
    vframe->ShowTime.TimeLow = (RK_U32)pts;
    switch ((*(mRKMppApi.HdiMppFrameGetFormat))(mframe) & MPP_FRAME_FMT_MASK) {
        case MPP_FMT_YUV420SP: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV420_SEMIPLANAR;
            vframe->OutputWidth = 0x20;
            } break;
        case MPP_FMT_YUV420SP_10BIT: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV420_SEMIPLANAR;
            vframe->ColorType |= VPU_OUTPUT_FORMAT_BIT_10;
            vframe->OutputWidth = 0x22;
            } break;
        case MPP_FMT_YUV422SP: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV422;
            vframe->OutputWidth = 0x10;
            } break;
        case MPP_FMT_YUV422SP_10BIT: {
            vframe->ColorType = VPU_OUTPUT_FORMAT_YUV422;
            vframe->ColorType |= VPU_OUTPUT_FORMAT_BIT_10;
            vframe->OutputWidth = 0x23;
            } break;
        default: {
            } break;
        }

    switch (colorPri) {
        case MPP_FRAME_PRI_BT2020: {
            vframe->ColorType |= VPU_OUTPUT_FORMAT_COLORSPACE_BT2020;
            } break;
        case MPP_FRAME_PRI_BT709: {
            vframe->ColorType |= VPU_OUTPUT_FORMAT_COLORSPACE_BT709;
            } break;
        default: {
            } break;
        }

    switch (colorTrc) {
        case MPP_FRAME_TRC_SMPTEST2084: {
            vframe->ColorType |= VPU_OUTPUT_FORMAT_DYNCRANGE_HDR10; // HDR10
            } break;
        case MPP_FRAME_TRC_ARIB_STD_B67: {
            vframe->ColorType |= VPU_OUTPUT_FORMAT_DYNCRANGE_HDR_HLG; // HDR_HLG
            } break;
        case MPP_FRAME_TRC_BT2020_10: {
            vframe->ColorType |= VPU_OUTPUT_FORMAT_COLORSPACE_BT2020; // BT2020
            } break;
        default: {
            } break;
        }

    if (buf) {
        MppBufferImpl *p = (MppBufferImpl*)buf;
        void *ptr = (p->mode == MPP_BUFFER_INTERNAL) ?
                    (*(mRKMppApi.HdiMppBufferGetPtrWithCaller))(buf, __FUNCTION__) : nullptr;
        RK_S32 fd = (*(mRKMppApi.HdiMppBufferGetFdWithCaller))(buf, __FUNCTION__);

        vframe->FrameBusAddr[0] = fd;
        vframe->FrameBusAddr[1] = fd;
        vframe->vpumem.vir_addr = (RK_U32*)ptr;
        vframe->vpumem.phy_addr = fd;

        vframe->vpumem.size = vframe->FrameWidth * vframe->FrameHeight * 3 / 2; // FrameHeight * 3 / 2
        vframe->vpumem.offset = (RK_U32*)buf;
    }
}

RK_S32 VpuApiLegacy::decode(VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut)
{
    MPP_RET ret = MPP_OK;
    MppFrame mframe = nullptr;
    MppPacket packet = nullptr;

    HDF_LOGE("enter\n");

    if (ctx->videoCoding == OMX_RK_VIDEO_CodingMJPEG) {
        MppTask task = nullptr;

        if (!init_ok) {
            HDF_LOGE("init failed!\n");
            return VPU_API_ERR_VPU_CODEC_INIT;
        }

        /* check input param */
        if (!pkt || !aDecOut) {
            HDF_LOGE("invalid input %p and output %p\n", pkt, aDecOut);
            return VPU_API_ERR_UNKNOW;
        }

        if (pkt->size <= 0) {
            HDF_LOGE("invalid input size %d\n", pkt->size);
            return VPU_API_ERR_UNKNOW;
        }

        /* try import input buffer and output buffer */
        RK_S32 fd           = -1;
        RK_U32 width        = ctx->width;
        RK_U32 height       = ctx->height;
        RK_U32 hor_stride   = MPP_ALIGN(width,  16); // (width,  16)
        RK_U32 ver_stride   = MPP_ALIGN(height, 16); // (height, 16)
        MppBuffer   str_buf = nullptr; /* input */
        MppBuffer   pic_buf = nullptr; /* output */

        ret = (*(mRKMppApi.HdiMppFrameInit))(&mframe);
        if (MPP_OK != ret) {
            HDF_LOGE("(*(mRKMppApi.HdiMppFrameInit)) failed\n");
            goto DECODE_OUT;
        }

        fd = (RK_S32)(pkt->pts & 0xffffffff);
        if (fd_input < 0) {
            fd_input = is_valid_dma_fd(fd);
        }

        if (fd_input) {
            MppBufferInfo   inputCommit;

            memset_s(&inputCommit, sizeof(inputCommit), 0, sizeof(inputCommit));
            inputCommit.type = MPP_BUFFER_TYPE_ION;
            inputCommit.size = pkt->size;
            inputCommit.fd = fd;

            ret = (*(mRKMppApi.Hdimpp_buffer_import_with_tag))(nullptr, \
                &inputCommit, &str_buf, MODULE_TAG, __FUNCTION__);
            if (ret) {
                HDF_LOGE("import input picture buffer failed\n");
                goto DECODE_OUT;
            }
        } else {
            if (pkt->data == nullptr) {
                ret = MPP_ERR_NULL_PTR;
                goto DECODE_OUT;
            }

            ret = (*(mRKMppApi.HdiMppBufferGetWithTag))(memGroup, &str_buf, pkt->size, MODULE_TAG, __FUNCTION__);
            if (ret) {
                HDF_LOGE("allocate input picture buffer failed\n");
                goto DECODE_OUT;
            }
            if (memcpy_s((RK_U8*) (*(mRKMppApi.HdiMppBufferGetPtrWithCaller))(str_buf, __FUNCTION__), \
                pkt->size, pkt->data, pkt->size) != EOK) {
                HDF_LOGE("memcpy_s no");
            }
        }

        fd = (RK_S32)(aDecOut->timeUs & 0xffffffff);
        if (fd_output < 0) {
            fd_output = is_valid_dma_fd(fd);
        }

        if (fd_output) {
            MppBufferInfo outputCommit;

            memset_s(&outputCommit, sizeof(outputCommit), 0, sizeof(outputCommit));
            /* in order to avoid interface change use space in output to transmit information */
            outputCommit.type = MPP_BUFFER_TYPE_ION;
            outputCommit.fd = fd;
            outputCommit.size = width * height * 3 / 2; // size 3 / 2 bit
            outputCommit.ptr = (void*)aDecOut->data;

            ret = (*(mRKMppApi.Hdimpp_buffer_import_with_tag)) \
                (nullptr, &outputCommit, &pic_buf, MODULE_TAG, __FUNCTION__);
            if (ret) {
                HDF_LOGE("import output stream buffer failed\n");
                goto DECODE_OUT;
            }
        } else {
            ret = (*(mRKMppApi.HdiMppBufferGetWithTag))
                (memGroup, &pic_buf, hor_stride * ver_stride * 3 / 2, MODULE_TAG, __FUNCTION__); // size 3 / 2 bit
            if (ret) {
                HDF_LOGE("allocate output stream buffer failed\n");
                goto DECODE_OUT;
            }
        }

        (*(mRKMppApi.HdiMppPacketInitWithBuffer))(&packet, str_buf); /* input */
        (*(mRKMppApi.HdiMppFrameSetBuffer))(mframe, pic_buf); /* output */

        HDF_LOGE("mpp import input fd %d output fd %d", \
            (*(mRKMppApi.HdiMppBufferGetFdWithCaller))(str_buf, __FUNCTION__), \
            (*(mRKMppApi.HdiMppBufferGetFdWithCaller))(pic_buf, __FUNCTION__));

        ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
        if (ret) {
            HDF_LOGE("mpp input poll failed\n");
            goto DECODE_OUT;
        }

        ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
        if (ret) {
            HDF_LOGE("mpp task input dequeue failed\n");
            goto DECODE_OUT;
        }

        (*(mRKMppApi.Hdimpp_task_meta_set_packet))(task, KEY_INPUT_PACKET, packet);
        (*(mRKMppApi.Hdimpp_task_meta_set_frame))(task, KEY_OUTPUT_FRAME, mframe);

        ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
        if (ret) {
            HDF_LOGE("mpp task input enqueue failed\n");
            goto DECODE_OUT;
        }

        pkt->size = 0;
        task = nullptr;

        ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
        if (ret) {
            HDF_LOGE("mpp output poll failed\n");
            goto DECODE_OUT;
        }

        ret = mpi->dequeue(mpp_ctx, MPP_PORT_OUTPUT, &task);
        if (ret) {
            HDF_LOGE("ret %d mpp task output dequeue failed\n", ret);
            goto DECODE_OUT;
        }

        if (task) {
            MppFrame frame_out = nullptr;

            (*(mRKMppApi.Hdimpp_task_meta_get_frame))(task, KEY_OUTPUT_FRAME, &frame_out);
            HDF_LOGE("decoded frame %d\n", frame_count);
            frame_count++;

            ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
            if (ret) {
                HDF_LOGE("mpp task output enqueue failed\n");
                goto DECODE_OUT;
            }
            task = nullptr;
        }

        // copy decoded frame into output buffer, and set outpub frame size
        if (mframe != nullptr) {
            MppBuffer buf_out = (*(mRKMppApi.HdiMppFrameGetBuffer))(mframe);
            size_t len  = (*(mRKMppApi.Hdimpp_buffer_get_size_with_caller))(buf_out, __FUNCTION__);
            aDecOut->size = len;

            if (fd_output) {
                HDF_LOGE("fd for output is invalid!\n");
                aDecOut->data = (RK_U8*)(*(mRKMppApi.Hdimpp_osal_malloc))
                    (__FUNCTION__, sizeof(RK_U8) * (width * height * 3 / 2)); // size 3 / 2 bit
                if ( memcpy_s(aDecOut->data,  aDecOut->size, \
                    (RK_U8*) (*(mRKMppApi.HdiMppBufferGetPtrWithCaller))(pic_buf, __FUNCTION__), \
                    aDecOut->size) != EOK) {
                    HDF_LOGE("memcpy_s no");
                }
            }

            HDF_LOGE("get frame %p size %d\n", mframe, len);

            (*(mRKMppApi.HdiMppFrameDeinit))(&mframe);
        } else {
            HDF_LOGE("outputPacket is nullptr!");
        }

    DECODE_OUT:
        if (str_buf) {
            (*(mRKMppApi.HdiMppBufferPutWithCaller))(str_buf, __FUNCTION__);
            str_buf = nullptr;
        }

        if (pic_buf) {
            (*(mRKMppApi.HdiMppBufferPutWithCaller))(pic_buf, __FUNCTION__);
            pic_buf = nullptr;
        }
    } else {
        (*(mRKMppApi.HdiMppPacketInit))(&packet, pkt->data, pkt->size);
        (*(mRKMppApi.HdiMppPacketSetPts))(packet, pkt->pts);
        if (pkt->nFlags & OMX_BUFFERFLAG_EOS) {
            (*(mRKMppApi.HdiMppPacketSetEos))(packet);
        }

        HDF_LOGE("input size %-6d flag %x pts %lld\n", \
            pkt->size, pkt->nFlags, pkt->pts);

        ret = mpi->decode(mpp_ctx, packet, &mframe);
        if (MPP_OK == ret) {
            pkt->size = 0;
        }
        if (ret || mframe == nullptr) {
            aDecOut->size = 0;
        } else {
            VPU_FRAME *vframe = (VPU_FRAME *)aDecOut->data;
            MppBuffer buf = (*(mRKMppApi.HdiMppFrameGetBuffer))(mframe);

            setup_VPU_FRAME_from_mpp_frame(vframe, mframe);

            aDecOut->size = sizeof(VPU_FRAME);
            aDecOut->timeUs = (*(mRKMppApi.Hdimpp_frame_get_pts))(mframe);
            frame_count++;

            if ((*(mRKMppApi.HdiMppFrameGetEos))(mframe)) {
                set_eos = 1;
                if (buf == nullptr) {
                    aDecOut->size = 0;
                }
            }
            if (vpu_api_debug & VPU_API_DBG_OUTPUT) {
                HDF_LOGE("get one frame pts %lld, fd 0x%x, poc %d, errinfo %x, discard %d, eos %d, verr %d", \
                    aDecOut->timeUs, \
                    ((buf) ? ((*(mRKMppApi.HdiMppBufferGetFdWithCaller))(buf, __FUNCTION__)) : (-1)), \
                    (*(mRKMppApi.Hdimpp_frame_get_poc))(mframe), \
                    (*(mRKMppApi.HdiMppFrameGetErrinfo))(mframe), \
                    (*(mRKMppApi.HdiMppFrameGetDiscard))(mframe), \
                    (*(mRKMppApi.HdiMppFrameGetEos))(mframe), vframe->ErrorInfo);
            }

            /*
             * IMPORTANT: mframe is malloced from mpi->decode_get_frame
             * So we need to deinit mframe here. But the buffer in the frame should not be free with mframe.
             * Because buffer need to be set to vframe->vpumem.offset and send to display.
             * The we have to clear the buffer pointer in mframe then release mframe.
             */
            (*(mRKMppApi.HdiMppFrameDeinit))(&mframe);
        }
    }

    if (packet)
        (*(mRKMppApi.HdiMppPacketDeinit))(&packet);

    if (mframe)
        (*(mRKMppApi.HdiMppFrameDeinit))(&mframe);

    HDF_LOGE("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy::decode_sendstream(VideoPacket_t *pkt)
{
    HDF_LOGE("enter\n");

    RK_S32 ret = MPP_OK;
    MppPacket mpkt = nullptr;

    if (!init_ok) {
        return VPU_API_ERR_VPU_CODEC_INIT;
    }

    (*(mRKMppApi.HdiMppPacketInit))(&mpkt, pkt->data, pkt->size);
    (*(mRKMppApi.HdiMppPacketSetPts))(mpkt, pkt->pts);
    if (pkt->nFlags & OMX_BUFFERFLAG_EOS) {
        (*(mRKMppApi.HdiMppPacketSetEos))(mpkt);
    }

    HDF_LOGE("input size %-6d flag %x pts %lld\n", \
        pkt->size, pkt->nFlags, pkt->pts);

    ret = mpi->decode_put_packet(mpp_ctx, mpkt);
    if (ret == MPP_OK) {
        pkt->size = 0;
    } else {
        /* reduce cpu overhead here */
        msleep(1);
    }

    (*(mRKMppApi.HdiMppPacketDeinit))(&mpkt);

    HDF_LOGE("leave ret %d\n", ret);
    /* NOTE: always return success for old player compatibility */
    return MPP_OK;
}

RK_S32 VpuApiLegacy::decode_getoutframe(DecoderOut_t *aDecOut)
{
    RK_S32 ret = 0;
    VPU_FRAME *vframe = (VPU_FRAME *)aDecOut->data;
    MppFrame  mframe = nullptr;

    HDF_LOGE("enter\n");

    if (!init_ok) {
        return VPU_API_ERR_VPU_CODEC_INIT;
    }

    memset_s(vframe, sizeof(VPU_FRAME), 0, sizeof(VPU_FRAME));

    if (mpi == nullptr) {
        aDecOut->size = 0;
        return 0;
    }

    if (set_eos) {
        aDecOut->size = 0;
        mEosSet = 1;
        return VPU_API_EOS_STREAM_REACHED;
    }

    ret = mpi->decode_get_frame(mpp_ctx, &mframe);
    if (ret || mframe == nullptr) {
        aDecOut->size = 0;
    } else {
        MppBuffer buf = (*(mRKMppApi.HdiMppFrameGetBuffer))(mframe);

        setup_VPU_FRAME_from_mpp_frame(vframe, mframe);

        aDecOut->size = sizeof(VPU_FRAME);
        aDecOut->timeUs = (*(mRKMppApi.Hdimpp_frame_get_pts))(mframe);
        frame_count++;

        if ((*(mRKMppApi.HdiMppFrameGetEos))(mframe) && !(*(mRKMppApi.HdiMppFrameGetInfoChange))(mframe)) {
            set_eos = 1;
            if (buf == nullptr) {
                aDecOut->size = 0;
                mEosSet = 1;
                ret = VPU_API_EOS_STREAM_REACHED;
            } else {
                aDecOut->nFlags |= VPU_API_EOS_STREAM_REACHED;
            }
        }
        if (vpu_api_debug & VPU_API_DBG_OUTPUT) {
            HDF_LOGE("get one frame pts %lld, fd 0x%x, poc %d, errinfo %x, discard %d, eos %d, verr %d", \
                aDecOut->timeUs, \
                ((buf) ? ((*(mRKMppApi.HdiMppBufferGetFdWithCaller))(buf, __FUNCTION__)) : (-1)), \
                (*(mRKMppApi.Hdimpp_frame_get_poc))(mframe), \
                (*(mRKMppApi.HdiMppFrameGetErrinfo))(mframe), \
                (*(mRKMppApi.HdiMppFrameGetDiscard))(mframe), \
                (*(mRKMppApi.HdiMppFrameGetEos))(mframe), vframe->ErrorInfo);
        }

        /*
         * IMPORTANT: mframe is malloced from mpi->decode_get_frame
         * So we need to deinit mframe here. But the buffer in the frame should not be free with mframe.
         * Because buffer need to be set to vframe->vpumem.offset and send to display.
         * The we have to clear the buffer pointer in mframe then release mframe.
         */
        (*(mRKMppApi.HdiMppFrameDeinit))(&mframe);
    }

    HDF_LOGE("leave ret %d\n", ret);

    return ret;
}

RK_S32 VpuApiLegacy::encode(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm, EncoderOut_t *aEncOut)
{
    MPP_RET ret = MPP_OK;
    MppTask task = nullptr;
    HDF_LOGE("enter\n");

    if (!init_ok)
        return VPU_API_ERR_VPU_CODEC_INIT;

    /* check input param */
    if (!aEncInStrm || !aEncOut) {
        HDF_LOGE("invalid input %p and output %p\n", aEncInStrm, aEncOut);
        return VPU_API_ERR_UNKNOW;
    }

    if (aEncInStrm->buf == nullptr || aEncInStrm->size == 0) {
        HDF_LOGE("invalid input buffer %p size %d\n", aEncInStrm->buf, aEncInStrm->size);
        return VPU_API_ERR_UNKNOW;
    }

    /* try import input buffer and output buffer */
    RK_S32 fd           = -1;
    RK_U32 width        = ctx->width;
    RK_U32 height       = ctx->height;
    RK_U32 hor_stride   = MPP_ALIGN(width,  16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 16);
    MppFrame    frame   = nullptr;
    MppPacket   packet  = nullptr;
    MppBuffer   pic_buf = nullptr;
    MppBuffer   str_buf = nullptr;

    ret = (*(mRKMppApi.HdiMppFrameInit))(&frame);
    if (MPP_OK != ret) {
        HDF_LOGE("(*(mRKMppApi.HdiMppFrameInit)) failed\n");
        goto ENCODE_OUT;
    }

    (*(mRKMppApi.HdiMppFrameSetWidth))(frame, width);
    (*(mRKMppApi.HdiMppFrameSetHeight))(frame, height);
    (*(mRKMppApi.HdiMppFrameSetHorStride))(frame, hor_stride);
    (*(mRKMppApi.HdiMppFrameSetVerStride))(frame, ver_stride);

    fd = aEncInStrm->bufPhyAddr;
    if (fd_input < 0) {
        fd_input = is_valid_dma_fd(fd);
    }
    if (fd_input) {
        MppBufferInfo   inputCommit;

        memset_s(&inputCommit, sizeof(inputCommit), 0, sizeof(inputCommit));
        inputCommit.type = MPP_BUFFER_TYPE_ION;
        inputCommit.size = aEncInStrm->size;
        inputCommit.fd = fd;

        ret = (*(mRKMppApi.Hdimpp_buffer_import_with_tag))(nullptr, &inputCommit, \
            &pic_buf, MODULE_TAG, __FUNCTION__);
        if (ret) {
            HDF_LOGE("import input picture buffer failed\n");
            goto ENCODE_OUT;
        }
    } else {
        if (aEncInStrm->buf == nullptr) {
            ret = MPP_ERR_NULL_PTR;
            goto ENCODE_OUT;
        }

        ret = (*(mRKMppApi.HdiMppBufferGetWithTag))(memGroup, &pic_buf, aEncInStrm->size, MODULE_TAG, __FUNCTION__);
        if (ret) {
            HDF_LOGE("allocate input picture buffer failed\n");
            goto ENCODE_OUT;
        }
        if (memcpy_s((RK_U8*) (*(mRKMppApi.HdiMppBufferGetPtrWithCaller))(pic_buf, __FUNCTION__), \
            aEncInStrm->size, aEncInStrm->buf, aEncInStrm->size) != EOK) {
            HDF_LOGE("memcpy_s no");
        }
    }

    fd = (RK_S32)(aEncOut->timeUs & 0xffffffff);

    if (fd_output < 0) {
        fd_output = is_valid_dma_fd(fd);
    }
    if (fd_output) {
        RK_S32 *tmp = (RK_S32*)(&aEncOut->timeUs);
        MppBufferInfo outputCommit;

        memset_s(&outputCommit, sizeof(outputCommit), 0, sizeof(outputCommit));
        /* in order to avoid interface change use space in output to transmit information */
        outputCommit.type = MPP_BUFFER_TYPE_ION;
        outputCommit.fd = fd;
        outputCommit.size = tmp[1];
        outputCommit.ptr = (void*)aEncOut->data;

        ret = (*(mRKMppApi.Hdimpp_buffer_import_with_tag))(nullptr, &outputCommit, &str_buf, MODULE_TAG, __FUNCTION__);
        if (ret) {
            HDF_LOGE("import output stream buffer failed\n");
            goto ENCODE_OUT;
        }
    } else {
        ret = (*(mRKMppApi.HdiMppBufferGetWithTag))(memGroup, \
            &str_buf, hor_stride * ver_stride, MODULE_TAG, __FUNCTION__);
        if (ret) {
            HDF_LOGE("allocate output stream buffer failed\n");
            goto ENCODE_OUT;
        }
    }

    (*(mRKMppApi.HdiMppFrameSetBuffer))(frame, pic_buf);
    (*(mRKMppApi.HdiMppPacketInitWithBuffer))(&packet, str_buf);
    (*(mRKMppApi.HdiMppPacketSetLength))(packet, 0);

    HDF_LOGE("mpp import input fd %d output fd %d", \
        (*(mRKMppApi.HdiMppBufferGetFdWithCaller))(pic_buf, __FUNCTION__), \
        (*(mRKMppApi.HdiMppBufferGetFdWithCaller))(str_buf, __FUNCTION__));

    ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
    if (ret) {
        HDF_LOGE("mpp input poll failed\n");
        goto ENCODE_OUT;
    }

    ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
    if (ret) {
        HDF_LOGE("mpp task input dequeue failed\n");
        goto ENCODE_OUT;
    }
    if (task == nullptr) {
        HDF_LOGE("mpi dequeue from MPP_PORT_INPUT fail, task equal with nullptr!");
        goto ENCODE_OUT;
    }

    (*(mRKMppApi.Hdimpp_task_meta_set_frame)) (task, KEY_INPUT_FRAME,  frame);
    (*(mRKMppApi.Hdimpp_task_meta_set_packet))(task, KEY_OUTPUT_PACKET, packet);

    ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
    if (ret) {
        HDF_LOGE("mpp task input enqueue failed\n");
        goto ENCODE_OUT;
    }
    task = nullptr;

    ret = mpi->poll(mpp_ctx, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
    if (ret) {
        HDF_LOGE("mpp output poll failed\n");
        goto ENCODE_OUT;
    }

    ret = mpi->dequeue(mpp_ctx, MPP_PORT_OUTPUT, &task);
    if (ret) {
        HDF_LOGE("ret %d mpp task output dequeue failed\n", ret);
        goto ENCODE_OUT;
    }

    if (task) {
        MppFrame frame_out = nullptr;
        MppFrame packet_out = nullptr;

        (*(mRKMppApi.HdiMppTaskMetaGetPacket))(task, KEY_OUTPUT_PACKET, &packet_out);

        HDF_LOGE("encoded frame %d\n", frame_count);
        frame_count++;

        ret = mpi->enqueue(mpp_ctx, MPP_PORT_OUTPUT, task);
        if (ret) {
            HDF_LOGE("mpp task output enqueue failed\n");
            goto ENCODE_OUT;
        }
        task = nullptr;

        ret = mpi->poll(mpp_ctx, MPP_PORT_INPUT, MPP_POLL_BLOCK);
        if (ret) {
            HDF_LOGE("mpp input poll failed\n");
            goto ENCODE_OUT;
        }

        // dequeue task from MPP_PORT_INPUT
        ret = mpi->dequeue(mpp_ctx, MPP_PORT_INPUT, &task);
        if (ret) {
            HDF_LOGE("failed to dequeue from input port ret %d\n", ret);
            goto ENCODE_OUT;
        }
        ret = (*(mRKMppApi.Hdimpp_task_meta_get_frame))(task, KEY_INPUT_FRAME, &frame_out);
        ret = mpi->enqueue(mpp_ctx, MPP_PORT_INPUT, task);
        if (ret) {
            HDF_LOGE("mpp task output enqueue failed\n");
            goto ENCODE_OUT;
        }
        task = nullptr;
    }

    // copy encoded stream into output buffer, and set output stream size
    if (packet) {
        RK_U32 eos = (*(mRKMppApi.HdiMppPacketGetEos))(packet);
        RK_S64 pts = (*(mRKMppApi.HdiMppPacketGetPts))(packet);
        size_t length = (*(mRKMppApi.HdiMppPacketGetLength))(packet);
        MppMeta meta = (*(mRKMppApi.Hdimpp_packet_get_meta))(packet);
        RK_S32 is_intra = 0;

        if (!fd_output) {
            RK_U8 *src = (RK_U8 *)(*(mRKMppApi.Hdimpp_packet_get_data))(packet);
            size_t buffer = MPP_ALIGN(length, SZ_4K);

            aEncOut->data = (RK_U8*)(*(mRKMppApi.Hdimpp_osal_malloc))(__FUNCTION__, sizeof(RK_U8)*buffer);

            if (ctx->videoCoding == OMX_RK_VIDEO_CodingAVC) {
                // remove first 00 00 00 01
                length -= 4; // length -= 4
                if (memcpy_s(aEncOut->data, length, src + 4, length) != EOK) { // length -= 4
                    HDF_LOGE("memcpy_s no");
                }
            } else {
                if (memcpy_s(aEncOut->data, length, src, length) != EOK) {
                    HDF_LOGE("memcpy_s no");
                }
            }
        }

        (*(mRKMppApi.Hdimpp_meta_get_s32))(meta, KEY_OUTPUT_INTRA, &is_intra);

        aEncOut->size = (RK_S32)length;
        aEncOut->timeUs = pts;
        aEncOut->keyFrame = is_intra;

        HDF_LOGE("get packet %p size %d pts %lld keyframe %d eos %d\n", \
            packet, length, pts, aEncOut->keyFrame, eos);

        (*(mRKMppApi.HdiMppPacketDeinit))(&packet);
    } else {
        HDF_LOGE("outputPacket is nullptr!");
    }

ENCODE_OUT:
    if (pic_buf) {
        (*(mRKMppApi.HdiMppBufferPutWithCaller))(pic_buf, __FUNCTION__);
        pic_buf = nullptr;
    }

    if (str_buf) {
        (*(mRKMppApi.HdiMppBufferPutWithCaller))(str_buf, __FUNCTION__);
        str_buf = nullptr;
    }

    if (frame)
        (*(mRKMppApi.HdiMppFrameDeinit))(&frame);

    if (packet)
        (*(mRKMppApi.HdiMppPacketDeinit))(&packet);

    HDF_LOGE("leave ret %d\n", ret);

    return ret;
}

RK_S32 VpuApiLegacy::encoder_sendframe(VpuCodecContext *ctx, EncInputStream_t *aEncInStrm)
{
    RK_S32 ret = 0;
    HDF_LOGE("enter\n");

    RK_U32 width        = ctx->width;
    RK_U32 height       = ctx->height;
    RK_U32 hor_stride   = MPP_ALIGN(width,  16);
    RK_U32 ver_stride   = MPP_ALIGN(height, 8);
    RK_S64 pts          = aEncInStrm->timeUs;
    RK_S32 fd           = aEncInStrm->bufPhyAddr;
    RK_U32 size         = aEncInStrm->size;

    /* try import input buffer and output buffer */
    MppFrame frame = nullptr;

    ret = (*(mRKMppApi.HdiMppFrameInit))(&frame);
    if (MPP_OK != ret) {
        HDF_LOGE("(*(mRKMppApi.HdiMppFrameInit)) failed\n");
        goto FUNC_RET;
    }

    (*(mRKMppApi.HdiMppFrameSetWidth))(frame, width);
    (*(mRKMppApi.HdiMppFrameSetHeight))(frame, height);
    (*(mRKMppApi.HdiMppFrameSetHorStride))(frame, hor_stride);
    (*(mRKMppApi.HdiMppFrameSetVerStride))(frame, ver_stride);
    (*(mRKMppApi.Hdimpp_frame_set_pts))(frame, pts);

    if (aEncInStrm->nFlags) {
        HDF_LOGE("found eos true\n");
        (*(mRKMppApi.HdiMppFrameSetEos))(frame, 1);
    }

    if (size <= 0) {
        (*(mRKMppApi.HdiMppFrameSetBuffer))(frame, nullptr);
        if (!aEncInStrm->nFlags)
            HDF_LOGE("found empty frame without eos flag set!\n");
        goto PUT_FRAME;
    }

    if (fd_input < 0) {
        fd_input = is_valid_dma_fd(fd);
    }
    if (fd_input) {
        MppBufferInfo   inputCommit;

        memset_s(&inputCommit, sizeof(inputCommit), 0, sizeof(inputCommit));
        inputCommit.type = MPP_BUFFER_TYPE_ION;
        inputCommit.size = size;
        inputCommit.fd = fd;
        if (size > 0) {
            MppBuffer buffer = nullptr;
            ret = (*(mRKMppApi.Hdimpp_buffer_import_with_tag))(nullptr, \
                &inputCommit, &buffer, MODULE_TAG, __FUNCTION__);
            if (ret) {
                HDF_LOGE("import input picture buffer failed\n");
                goto FUNC_RET;
            }
            (*(mRKMppApi.HdiMppFrameSetBuffer))(frame, buffer);
            (*(mRKMppApi.HdiMppBufferPutWithCaller))(buffer, __FUNCTION__);
            buffer = nullptr;
        }
    } else {
        RK_U32 align_size   = 0;

        if (aEncInStrm->buf == nullptr) {
            ret = MPP_ERR_NULL_PTR;
            goto FUNC_RET;
        }
        if (format >= MPP_FMT_YUV420SP && format < MPP_FMT_YUV_BUTT) {
            align_size = hor_stride * MPP_ALIGN(ver_stride, 16) * 3 / 2; // (ver_stride, 16) * 3 / 2 bit
        } else if (format >= MPP_FMT_RGB565 && format < MPP_FMT_BGR888) {
            align_size = hor_stride * MPP_ALIGN(ver_stride, 16) * 3; // (ver_stride, 16) * 3 bit
        } else if (format >= MPP_FMT_RGB101010 && format < MPP_FMT_RGB_BUTT) {
            align_size = hor_stride * MPP_ALIGN(ver_stride, 16) * 4; // (ver_stride, 16) * 4 bit
        } else {
            HDF_LOGE("unsupport input format:%d\n", format);
            ret = MPP_NOK;
            goto FUNC_RET;
        }
        if (align_size > 0) {
            MppBuffer buffer = nullptr;
            ret = (*(mRKMppApi.HdiMppBufferGetWithTag))(memGroup, &buffer, align_size, MODULE_TAG, __FUNCTION__);
            if (ret) {
                HDF_LOGE("allocate input picture buffer failed\n");
                goto FUNC_RET;
            }
            copy_align_raw_buffer_to_dest((RK_U8 *)(*(mRKMppApi.HdiMppBufferGetPtrWithCaller))(buffer, __FUNCTION__),
                aEncInStrm->buf, width, height, format);
            (*(mRKMppApi.HdiMppFrameSetBuffer))(frame, buffer);
            (*(mRKMppApi.HdiMppBufferPutWithCaller))(buffer, __FUNCTION__);
            buffer = nullptr;
        }
    }

PUT_FRAME:

    HDF_LOGE("w %d h %d input fd %d size %d pts %lld, flag %d \n", \
        width, height, fd, size, aEncInStrm->timeUs, aEncInStrm->nFlags);
    if (mlvec) {
        MppMeta meta = (*(mRKMppApi.HdiMppFrameGetMeta))(frame);

        vpu_api_mlvec_set_dy_cfg(mlvec, &mlvec_dy_cfg, meta);
    }

    ret = mpi->encode_put_frame(mpp_ctx, frame);
    if (ret)
        HDF_LOGE("encode_put_frame ret %d\n", ret);
    else
        aEncInStrm->size = 0;

FUNC_RET:
    if (frame)
        (*(mRKMppApi.HdiMppFrameDeinit))(&frame);

    HDF_LOGE("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy::encoder_getstream(VpuCodecContext *ctx, EncoderOut_t *aEncOut)
{
    RK_S32 ret = 0;
    MppPacket packet = nullptr;
    HDF_LOGE("enter\n");

    ret = mpi->encode_get_packet(mpp_ctx, &packet);
    if (ret) {
        HDF_LOGE("encode_get_packet failed ret %d\n", ret);
        goto FUNC_RET;
    }
    if (packet) {
        RK_U8 *src = (RK_U8 *)(*(mRKMppApi.Hdimpp_packet_get_data))(packet);
        RK_U32 eos = (*(mRKMppApi.HdiMppPacketGetEos))(packet);
        RK_S64 pts = (*(mRKMppApi.HdiMppPacketGetPts))(packet);
        size_t length = (*(mRKMppApi.HdiMppPacketGetLength))(packet);
        MppMeta meta = (*(mRKMppApi.Hdimpp_packet_get_meta))(packet);
        RK_S32 is_intra = 0;

        RK_U32 offset = 0;
        if (ctx->videoCoding == OMX_RK_VIDEO_CodingAVC) {
            offset = 4; //  offset = 4
            length = (length > offset) ? (length - offset) : 0;
        }
        aEncOut->data = nullptr;
        if (length > 0) {
            aEncOut->data = (RK_U8*)(*(mRKMppApi.Hdimpp_osal_calloc)) \
                (__FUNCTION__, sizeof(RK_U8)*MPP_ALIGN(length + 16, SZ_4K)); // length + 16
            if (aEncOut->data) {
                if (memcpy_s(aEncOut->data, length, src + offset, length) != EOK) {
                    HDF_LOGE("memcpy_s no");
                }
            }
        }

        (*(mRKMppApi.Hdimpp_meta_get_s32))(meta, KEY_OUTPUT_INTRA, &is_intra);

        aEncOut->size = (RK_S32)length;
        aEncOut->timeUs = pts;
        aEncOut->keyFrame = is_intra;

        HDF_LOGE("get packet %p size %d pts %lld keyframe %d eos %d\n", \
            packet, length, pts, aEncOut->keyFrame, eos);

        mEosSet = eos;
        (*(mRKMppApi.HdiMppPacketDeinit))(&packet);
    } else {
        aEncOut->size = 0;
        HDF_LOGE("get nullptr packet, eos %d\n", mEosSet);
        if (mEosSet)
            ret = -1;
    }

FUNC_RET:
    HDF_LOGE("leave ret %d\n", ret);
    return ret;
}

RK_S32 VpuApiLegacy::perform(PerformCmd cmd, RK_S32 *data)
{
    HDF_LOGE("enter\n");
    switch (cmd) {
        case INPUT_FORMAT_MAP : {
            EncInputPictureType vpu_frame_fmt = *(EncInputPictureType *)data;
            MppFrameFormat mpp_frame_fmt = vpu_pic_type_remap_to_mpp(vpu_frame_fmt);
            *(MppFrameFormat *)data = mpp_frame_fmt;
            } break;
        default:
            HDF_LOGE("cmd can not match with any option!");
            break;
    }
    HDF_LOGE("leave\n");
    return 0;
}

RK_S32 VpuApiLegacy::control(VpuCodecContext *ctx, VPU_API_CMD cmd, void *param)
{
    HDF_LOGE("enter cmd 0x%x param %p\n", cmd, param);

    if (mpi == nullptr && !init_ok) {
        return 0;
    }

    MpiCmd mpicmd = MPI_CMD_BUTT;
    switch (cmd) {
        case VPU_API_ENABLE_DEINTERLACE : {
            mpicmd = MPP_DEC_SET_ENABLE_DEINTERLACE;
            } break;
        case VPU_API_ENC_SETCFG : {
            MppCodingType coding = (MppCodingType)ctx->videoCoding;

            if (memcpy_s(&enc_param, sizeof(enc_param), param, sizeof(enc_param)) != EOK) {
                HDF_LOGE("memcpy_s no");
            }
            return vpu_api_set_enc_cfg(mpp_ctx, mpi, enc_cfg, coding, format, &enc_param);
            } break;
        case VPU_API_ENC_GETCFG : {
            if (memcpy_s(param, sizeof(enc_param), &enc_param, sizeof(enc_param)) != EOK) {
                HDF_LOGE("memcpy_s no");
            }
            return 0;
            } break;
        case VPU_API_ENC_SETFORMAT : {
            EncInputPictureType type = *((EncInputPictureType *)param);
            format = vpu_pic_type_remap_to_mpp(type);
            return 0;
            } break;
        case VPU_API_ENC_SETIDRFRAME : {
            mpicmd = MPP_ENC_SET_IDR_FRAME;
            } break;
        case VPU_API_SET_VPUMEM_CONTEXT: {
            mpicmd = MPP_DEC_SET_EXT_BUF_GROUP;
            } break;
        case VPU_API_USE_PRESENT_TIME_ORDER: {
            mpicmd = MPP_DEC_SET_PRESENT_TIME_ORDER;
            } break;
        case VPU_API_SET_INFO_CHANGE: {
            mpicmd = MPP_DEC_SET_INFO_CHANGE_READY;
            } break;
        case VPU_API_USE_FAST_MODE: {
            mpicmd = MPP_DEC_SET_PARSER_FAST_MODE;
            } break;
        case VPU_API_DEC_GET_STREAM_COUNT: {
            mpicmd = MPP_DEC_GET_STREAM_COUNT;
            } break;
        case VPU_API_GET_VPUMEM_USED_COUNT: {
            mpicmd = MPP_DEC_GET_VPUMEM_USED_COUNT;
            } break;
        case VPU_API_SET_OUTPUT_BLOCK: {
            mpicmd = MPP_SET_OUTPUT_TIMEOUT;
            if (param) {
                RK_S32 timeout = *((RK_S32*)param);

                if (timeout) {
                    if (timeout < 0)
                        HDF_LOGE("set output mode to block\n");
                    else
                        HDF_LOGE("set output timeout %d ms\n", timeout);
                } else {
                    HDF_LOGE("set output mode to non-block\n");
                }
            }
            } break;
        case VPU_API_GET_EOS_STATUS: {
            *((RK_S32 *)param) = mEosSet;
            mpicmd = MPI_CMD_BUTT;
            } break;
        case VPU_API_GET_FRAME_INFO: {
            *((VPU_GENERIC *)param) = vpug;
            mpicmd = MPI_CMD_BUTT;
            } break;
        case VPU_API_SET_OUTPUT_MODE: {
            mpicmd = MPP_DEC_SET_OUTPUT_FORMAT;
            } break;
        case VPU_API_SET_IMMEDIATE_OUT: {
            mpicmd = MPP_DEC_SET_IMMEDIATE_OUT;
            } break;
        case VPU_API_ENC_SET_VEPU22_CFG: {
            mpicmd = MPP_ENC_SET_CODEC_CFG;
            } break;
        case VPU_API_ENC_GET_VEPU22_CFG: {
            mpicmd = MPP_ENC_GET_CODEC_CFG;
            } break;
        case VPU_API_ENC_SET_VEPU22_CTU_QP: {
            mpicmd = MPP_ENC_SET_CTU_QP;
            } break;
        case VPU_API_ENC_SET_VEPU22_ROI: {
            mpicmd = MPP_ENC_SET_ROI_CFG;
            } break;
        case VPU_API_ENC_SET_MAX_TID: {
            RK_S32 max_tid = *(RK_S32 *)param;

            HDF_LOGE("VPU_API_ENC_SET_MAX_TID %d\n", max_tid);
            mlvec_dy_cfg.max_tid = max_tid;
            mlvec_dy_cfg.updated |= VPU_API_ENC_MAX_TID_UPDATED;

            if (mlvec)
                vpu_api_mlvec_set_dy_max_tid(mlvec, max_tid);

            return 0;
            } break;
        case VPU_API_ENC_SET_MARK_LTR: {
            RK_S32 mark_ltr = *(RK_S32 *)param;

            HDF_LOGE("VPU_API_ENC_SET_MARK_LTR %d\n", mark_ltr);

            mlvec_dy_cfg.mark_ltr = mark_ltr;
            if (mark_ltr >= 0)
                mlvec_dy_cfg.updated |= VPU_API_ENC_MARK_LTR_UPDATED;

            return 0;
            } break;
        case VPU_API_ENC_SET_USE_LTR: {
            RK_S32 use_ltr = *(RK_S32 *)param;

            HDF_LOGE("VPU_API_ENC_SET_USE_LTR %d\n", use_ltr);

            mlvec_dy_cfg.use_ltr = use_ltr;
            mlvec_dy_cfg.updated |= VPU_API_ENC_USE_LTR_UPDATED;

            return 0;
            } break;
        case VPU_API_ENC_SET_FRAME_QP: {
            RK_S32 frame_qp = *(RK_S32 *)param;

            HDF_LOGE("VPU_API_ENC_SET_FRAME_QP %d\n", frame_qp);

            mlvec_dy_cfg.frame_qp = frame_qp;
            mlvec_dy_cfg.updated |= VPU_API_ENC_FRAME_QP_UPDATED;

            return 0;
            } break;
        case VPU_API_ENC_SET_BASE_LAYER_PID: {
            RK_S32 base_layer_pid = *(RK_S32 *)param;

            HDF_LOGE("VPU_API_ENC_SET_BASE_LAYER_PID %d\n", base_layer_pid);

            mlvec_dy_cfg.base_layer_pid = base_layer_pid;
            mlvec_dy_cfg.updated |= VPU_API_ENC_BASE_PID_UPDATED;

            return 0;
            } break;
        case VPU_API_GET_EXTRA_INFO: {
            EncoderOut_t *out = (EncoderOut_t *)param;

            HDF_LOGE("VPU_API_GET_EXTRA_INFO\n");

            if (enc_hdr_pkt == nullptr) {
                if (enc_hdr_buf == nullptr) {
                    enc_hdr_buf_size = SZ_1K;
                    enc_hdr_buf = (*(mRKMppApi.Hdimpp_osal_calloc))(__FUNCTION__, enc_hdr_buf_size);
                }

                if (enc_hdr_buf)
                    (*(mRKMppApi.HdiMppPacketInit))(&enc_hdr_pkt, enc_hdr_buf, enc_hdr_buf_size);
            }

            if (enc_hdr_pkt) {
                mpi->control(mpp_ctx, MPP_ENC_GET_HDR_SYNC, enc_hdr_pkt);

                RK_S32 length = (*(mRKMppApi.HdiMppPacketGetLength))(enc_hdr_pkt);
                void *src = (*(mRKMppApi.Hdimpp_packet_get_data))(enc_hdr_pkt);

                if (memcpy_s(out->data, length, src, length) != EOK) {
                    HDF_LOGE("memcpy_s no");
                }
                out->size = length;
            }
            return 0;
            } break;
        case VPU_API_SET_PARSER_SPLIT_MODE: {
            mpicmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
            } break;
        default: {
            } break;
    }

    RK_S32 ret = -1;
    if (mpicmd < MPI_CMD_BUTT)
        ret = mpi->control(mpp_ctx, mpicmd, (MppParam)param);

    HDF_LOGE("leave\n");
    return ret;
}

