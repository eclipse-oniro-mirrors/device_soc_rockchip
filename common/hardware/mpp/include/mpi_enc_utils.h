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

#ifndef __MPI_ENC_UTILS_H__
#define __MPI_ENC_UTILS_H__

#include <stdio.h>

#include "rk_venc_cmd.h"

typedef struct MpiEncTestArgs_t {
    MppCodingType type;
    MppFrameFormat format;

    signed int width;
    signed int height;
} MpiEncTestArgs;

typedef struct {
    // global flow control flag
    unsigned int frm_eos;
    unsigned int pkt_eos;
    signed int frame_count;
    RK_U64 stream_size;

    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncCfg cfg;

    // input / output
    MppBufferGroup buf_grp;
    MppBuffer pkt_buf;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;

    // paramter for resource malloc
    unsigned int width;
    unsigned int height;
    unsigned int hor_stride;
    unsigned int ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    signed int num_frames;

    // resources
    size_t frame_size;

    unsigned int split_mode;
    unsigned int split_arg;

    // rate control runtime parameter

    signed int fps_in_flex;
    signed int fps_in_den;
    signed int fps_in_num;
    signed int fps_out_flex;
    signed int fps_out_den;
    signed int fps_out_num;
    signed int bps;
    signed int bps_max;
    signed int bps_min;
    signed int rc_mode;
    signed int gop_mode;
    signed int gop_len;
    signed int vi_len;
} MpiEncTestData;

int hal_mpp_get_sps(void *ctx, unsigned char *buf, size_t *buf_size);
int hal_mpp_encode(void *ctx, int dma_fd, unsigned char *buf, size_t *buf_size);

void *hal_mpp_ctx_create(MpiEncTestArgs *args);
void hal_mpp_ctx_delete(void *ctx);

#endif /* __MPI_ENC_UTILS_H__ */
