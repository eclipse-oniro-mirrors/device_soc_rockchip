/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
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

#ifndef __MPP_RC_API_H__
#define __MPP_RC_API_H__

#include "mpp_err.h"
#include "rk_venc_rc.h"
#include "mpp_rc_defs.h"

/*
 * Mpp rate control has three parts:
 *
 * 1. MPI user config module
 *    MppEncRcCfg structure is provided to user for overall rate control config
 *    Mpp will receive MppEncRcCfg from user, check parameter and set it to
 *    encoder.
 *
 * 2. Encoder rate control module
 *    Encoder will implement the rate control strategy required by users
 *    including CBR, VBR, AVBR and so on.
 *    This module only implement the target bit calculation behavior and
 *    quality restriction. And the quality level will be controlled by hal.
 *
 * 3. Hal rate control module
 *    Hal will implement the rate control on hardware. Hal will calculate the
 *    QP parameter for hardware according to the frame level target bit
 *    specified by the encoder. And the report the real bitrate and quality to
 *    encoder.
 *
 * The header defines the communication interfaces and structures used between
 * MPI, encoder and hal.
 */

typedef enum RcMode_e {
    RC_VBR,
    RC_CBR,
    RC_FIXQP,
    RC_AVBR,
    RC_CVBR,
    RC_QVBR,
    RC_LEARNING,
    RC_MODE_BUTT,
} RcMode;

typedef enum GopMode_e {
    NORMAL_P,
    SMART_P,
} GopMode;

/*
 * frame rate parameters have great effect on rate control
 *
 * fps_in_flex
 * 0 - fix input frame rate
 * 1 - variable input frame rate
 *
 * fps_in_num
 * input frame rate numerator, if 0 then default 30
 *
 * fps_in_denorm
 * input frame rate denorminator, if 0 then default 1
 *
 * fps_out_flex
 * 0 - fix output frame rate
 * 1 - variable output frame rate
 *
 * fps_out_num
 * output frame rate numerator, if 0 then default 30
 *
 * fps_out_denorm
 * output frame rate denorminator, if 0 then default 1
 */
typedef struct RcFpsCfg_t {
    signed int fps_in_flex;
    signed int fps_in_num;
    signed int fps_in_denorm;
    signed int fps_out_flex;
    signed int fps_out_num;
    signed int fps_out_denorm;
} RcFpsCfg;

typedef struct RcSuperframeCfg_t {
    MppEncRcSuperFrameMode super_mode;
    unsigned int super_i_thd;
    unsigned int super_p_thd;
    MppEncRcPriority rc_priority;
} RcSuperframeCfg;

typedef struct RcDebreathCfg_t {
    unsigned int enable;
    unsigned int strength;
} RcDebreathCfg;

typedef struct RcHierQPCfg_t {
    signed int hier_qp_en;
    signed int hier_qp_delta[4];
    signed int hier_frame_num[4];
} RcHierQPCfg;

/*
 * Control parameter from external config
 *
 * It will be updated on rc/prep/gopref config changed.
 */
typedef struct RcCfg_s {
    /* encode image size */
    signed int width;
    signed int height;

    /* Use rc_mode to find different api */
    RcMode mode;

    RcFpsCfg fps;

    GopMode gop_mode;
    /* I frame gop len */
    signed int igop;
    /* visual gop len */
    signed int vgop;

    /* bitrate parameter */
    signed int bps_min;
    signed int bps_target;
    signed int bps_max;
    signed int stats_time;

    /* max I frame bit ratio to P frame bit */
    signed int max_i_bit_prop;
    signed int min_i_bit_prop;
    signed int init_ip_ratio;
    /* layer bitrate proportion */
    signed int layer_bit_prop[4];

    /* quality parameter */
    signed int init_quality;
    signed int max_quality;
    signed int min_quality;
    signed int max_i_quality;
    signed int min_i_quality;
    signed int i_quality_delta;
    signed int vi_quality_delta;
    /* layer quality proportion */
    signed int layer_quality_delta[4];

    /* reencode parameter */
    signed int max_reencode_times;

    /* still / motion desision parameter */
    signed int min_still_prop;
    signed int max_still_quality;

    /*
     * vbr parameter
     *
     * vbr_hi_prop  - high proportion bitrate for reduce quality
     * vbr_lo_prop  - low proportion bitrate for increase quality
     */
    signed int vbr_hi_prop;
    signed int vbr_lo_prop;

    MppEncRcDropFrmMode drop_mode;
    unsigned int drop_thd;
    unsigned int drop_gap;

    RcSuperframeCfg super_cfg;
    RcDebreathCfg debreath_cfg;
    RcHierQPCfg hier_qp_cfg;
} RcCfg;

/*
 * Different rate control strategy will be implemented by different API config
 */
typedef struct RcImplApi_t {
    char *name;
    MppCodingType type;
    unsigned int ctx_size;

    MPP_RET (*init)(void *ctx, RcCfg *cfg);
    MPP_RET (*deinit)(void *ctx);

    MPP_RET (*check_drop)(void *ctx, EncRcTask *task);
    MPP_RET (*check_reenc)(void *ctx, EncRcTask *task);

    /*
     * frm_start -  frame level rate control frm_start.
     *              The EncRcTaskInfo will be output to hal for hardware to implement.
     * frm_end   -  frame level rate control frm_end.
     *              The EncRcTaskInfo is returned for real quality and bitrate.
     */
    MPP_RET (*frm_start)(void *ctx, EncRcTask *task);
    MPP_RET (*frm_end)(void *ctx, EncRcTask *task);

    /*
     * hal_start -  hardware level rate control start.
     *              The EncRcTaskInfo will be output to hal for hardware to implement.
     * hal_end   -  hardware level rate control end.
     *              The EncRcTaskInfo is returned for real quality and bitrate.
     */
    MPP_RET (*hal_start)(void *ctx, EncRcTask *task);
    MPP_RET (*hal_end)(void *ctx, EncRcTask *task);
} RcImplApi;

/*
 * structures for RC API register and query
 */
typedef struct RcApiBrief_t {
    const char *name;
    MppCodingType type;
} RcApiBrief;

typedef struct RcApiQueryAll_t {
    /* input param for query */
    RcApiBrief *brief;
    signed int max_count;

    /* output query count */
    signed int count;
} RcApiQueryAll;

typedef struct RcApiQueryType_t {
    /* input param for query */
    RcApiBrief *brief;
    signed int max_count;
    MppCodingType type;

    /* output query count */
    signed int count;
} RcApiQueryType;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET rc_api_add(const RcImplApi *api);
MPP_RET rc_brief_get_all(RcApiQueryAll *query);
MPP_RET rc_brief_get_by_type(RcApiQueryType *query);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_RC_API_H__ */