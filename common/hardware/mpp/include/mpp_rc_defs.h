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

#ifndef __MPP_RC_DEFS_H__
#define __MPP_RC_DEFS_H__

#include "rk_venc_ref.h"

#define MAX_CPB_REFS (8)

/*
 * EncFrmStatus controls record the encoding frame status and also control
 * work flow of encoder. It is the communicat channel between encoder implement
 * module, rate control module and hardware module.
 *
 * bit  0 ~ 31 frame status
 *      0 ~ 15 current frame status
 *     16 ~ 31 reference frame status
 * bit 32 ~ 63 encoding flow control
 */
typedef union EncFrmStatus_u {
    struct {
        /*
         * bit  0 ~ 31  frame status
         */
        /* status flag */
        unsigned int valid : 1;
        /*
         * 0 - write the reconstructed frame pixel to memory
         * 1 - do not write the reconstructed frame pixel to memory
         */
        unsigned int non_recn : 1;
        unsigned int reserved0 : 2;

        /* reference status flag */
        /*
         * 0 - inter frame
         * 1 - intra frame
         */
        unsigned int is_intra : 1;

        /*
         * Valid when is_intra is true
         * 0 - normal intra frame
         * 1 - IDR frame
         */
        unsigned int is_idr : 1;

        /*
         * 0 - mark as reference frame
         * 1 - mark as non-refernce frame
         */
        unsigned int is_non_ref : 1;

        /*
         * Valid when is_non_ref is false
         * 0 - mark as short-term reference frame
         * 1 - mark as long-term refernce frame
         */
        unsigned int is_lt_ref : 1;

        /* bit 8 - 15 */
        unsigned int lt_idx : 4;
        unsigned int temporal_id : 4;

        /* distance between current frame and reference frame */
        MppEncRefMode ref_mode : 6;
        signed int ref_arg : 8;
        signed int ref_dist : 2;

        /*
         * bit 32 ~ 63  encoder flow control flags
         */
        /*
         * 0 - normal frame encoding
         * 1 - current frame will be dropped
         */
        unsigned int drop : 1;

        /*
         * 0 - rate control module does not change frame type parameter
         * 1 - rate control module changes frame type parameter reencode is needed
         *     to reprocess the dpb process. Also this means dpb module will follow
         *     the frame status parameter provided by rate control module.
         */
        unsigned int re_dpb_proc : 1;

        /*
         * 0 - current frame encoding is in normal flow
         * 1 - current frame encoding is in reencode flow
         */
        unsigned int reencode : 1;

        /*
         * When true current frame size is super large then the frame should be reencoded.
         */
        unsigned int super_frame : 1;

        /*
         * When true currnet frame is force to encoded as software skip frame
         */
        unsigned int force_pskip : 1;
        unsigned int reserved1 : 3;

        /* reencode times */
        unsigned int reencode_times : 8;

        /* sequential index for each frame */
        unsigned int seq_idx : 16;
    };
    RK_U64 val;
} EncFrmStatus;

typedef struct EncCpbStatus_t {
    signed int seq_idx;

    EncFrmStatus curr;
    EncFrmStatus refr;

    /* initial cpb status for current frame encoding */
    EncFrmStatus init[MAX_CPB_REFS];
    /* final cpb status after current frame encoding */
    EncFrmStatus final[MAX_CPB_REFS];
} EncCpbStatus;

#define ENC_RC_FORCE_QP (0x00000001)

typedef struct EncRcForceCfg_t {
    unsigned int force_flag;
    signed int force_qp;
    unsigned int reserve[6];
} EncRcForceCfg;

/*
 * communication channel between rc / hal / hardware
 *
 * rc   -> hal      bit_target / bit_max / bit_min
 * hal  -> hw       quality_target / quality_max / quality_min
 * hw   -> rc / hal bit_real / quality_real / madi / madp
 */
typedef struct EncRcCommonInfo_t {
    /* rc to hal */
    signed int bit_target;
    signed int bit_max;
    signed int bit_min;

    signed int quality_target;
    signed int quality_max;
    signed int quality_min;

    /* rc from hardware */
    signed int bit_real;
    signed int quality_real;
    signed int madi;
    signed int madp;

    unsigned int iblk4_prop; // scale 256
    signed int reserve[15];
} EncRcTaskInfo;

typedef struct EncRcTask_s {
    EncCpbStatus cpb;
    EncFrmStatus frm;
    EncRcTaskInfo info;
    EncRcForceCfg force;
    MppFrame frame;
} EncRcTask;

#endif /* __MPP_RC_DEFS_H__ */