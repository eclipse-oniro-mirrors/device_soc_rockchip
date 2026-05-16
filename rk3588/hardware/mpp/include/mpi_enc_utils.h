/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#ifndef __MPI_ENC_UTILS_H__
#define __MPI_ENC_UTILS_H__

#include <stdio.h>

#include "rk_venc_cmd.h"
#include "iniparser.h"
#include "utils_mpp.h"
#include "mpp_enc_roi_utils.h"
#include "camera_source.h"

typedef struct MpiEncTestArgs_t {
    char                *file_input;
    char                *file_output;
    char                *file_cfg;
    dictionary          *cfg_ini;

    MppCodingType       type;
    MppCodingType       type_src;       /* for file source input */
    MppFrameFormat      format;
    RK_S32              frame_num;
    RK_S32              loop_cnt;
    RK_S32              nthreads;

    RK_S32              width;
    RK_S32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;

    /* -rc */
    RK_S32              rc_mode;

    /* -bps */
    RK_S32              bps_target;
    RK_S32              bps_max;
    RK_S32              bps_min;

    /* -fps */
    RK_S32              fps_in_flex;
    RK_S32              fps_in_num;
    RK_S32              fps_in_den;
    RK_S32              fps_out_flex;
    RK_S32              fps_out_num;
    RK_S32              fps_out_den;

    /* -qc */
    RK_S32              qp_init;
    RK_S32              qp_min;
    RK_S32              qp_max;
    RK_S32              qp_min_i;
    RK_S32              qp_max_i;

    /* -g gop mode */
    RK_S32              gop_mode;
    RK_S32              gop_len;
    RK_S32              vi_len;

    /* -v q runtime log disable flag */
    RK_U32              quiet;
    /* -v f runtime fps log flag */
    RK_U32              trace_fps;
    FpsCalc             fps;
    RK_U32              psnr_en;
    RK_U32              ssim_en;
    char                *file_slt;
    unsigned char       *buf;
    size_t              *buf_size;
} MpiEncTestArgs;

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 mpi_enc_width_default_stride(RK_S32 width, MppFrameFormat fmt);

/*
 * gop_mode
 * 0     - default IPPPP gop
 * 1 ~ 3 - tsvc2 ~ tsvc4
 * >=  4 - smart gop mode
 */
MPP_RET mpi_enc_gen_ref_cfg(MppEncRefCfg ref, RK_S32 gop_mode);
MPP_RET mpi_enc_gen_smart_gop_ref_cfg(MppEncRefCfg ref, RK_S32 gop_len, RK_S32 vi_len);
MPP_RET mpi_enc_gen_osd_data(MppEncOSDData *osd_data, MppBufferGroup group,
                             RK_U32 width, RK_U32 height, RK_U32 frame_cnt);
MPP_RET mpi_enc_gen_osd_plt(MppEncOSDPlt *osd_plt, RK_U32 frame_cnt);

MpiEncTestArgs *mpi_enc_test_cmd_get(void);
MPP_RET mpi_enc_test_cmd_update_by_args(MpiEncTestArgs* cmd, int argc, char **argv);
MPP_RET mpi_enc_test_cmd_put(MpiEncTestArgs* cmd);

MPP_RET mpi_enc_test_cmd_show_opt(MpiEncTestArgs* cmd);

//int RKYuvToH264(unsigned char *buf, size_t *buf_size, int width,
//    int height, MppFrameFormat format, MppCodingType type);
int hal_mpp_encode(void *ctxs, int dma_fd, unsigned char *buf, size_t *buf_size);
void *hal_mpp_ctx_create(MpiEncTestArgs *args);
void hal_mpp_ctx_delete(void *ctx);

typedef struct {
    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    RK_S32 chn;

    // global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frm_pkt_cnt;
    RK_S32 frame_num;
    RK_S32 frame_count;
    RK_U64 stream_size;
    /* end of encoding flag when set quit the loop */
    volatile RK_U32 loop_end;

    // src and dst
    FILE *fp_input;
    FILE *fp_output;
    FILE *fp_verify;

    /* encoder config set */
    MppEncCfg       cfg;
    MppEncPrepCfg   prep_cfg;
    MppEncRcCfg     rc_cfg;
    MppEncCodecCfg  codec_cfg;
    MppEncSliceSplit split_cfg;
    MppEncOSDPltCfg osd_plt_cfg;
    MppEncOSDPlt    osd_plt;
    MppEncOSDData   osd_data;
    RoiRegionCfg    roi_region;
    MppEncROICfg    roi_cfg;

    // input / output
    MppBufferGroup buf_grp;
    MppBuffer frm_buf;
    MppBuffer pkt_buf;
    MppBuffer md_info;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    RK_S32 loop_times;
    CamSource *cam_ctx;
    MppEncRoiCtx roi_ctx;

    // resources
    size_t header_size;
    size_t frame_size;
    size_t mdinfo_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;

    RK_U32 osd_enable;
    RK_U32 osd_mode;
    RK_U32 split_mode;
    RK_U32 split_arg;
    RK_U32 split_out;

    RK_U32 user_data_enable;
    RK_U32 roi_enable;

    // rate control runtime parameter
    RK_S32 fps_in_flex;
    RK_S32 fps_in_den;
    RK_S32 fps_in_num;
    RK_S32 fps_out_flex;
    RK_S32 fps_out_den;
    RK_S32 fps_out_num;
    RK_S32 bps;
    RK_S32 bps_max;
    RK_S32 bps_min;
    RK_S32 rc_mode;
    RK_S32 gop_mode;
    RK_S32 gop_len;
    RK_S32 vi_len;

    RK_S64 first_frm;
    RK_S64 first_pkt;
} MpiEncTestData;

/* For each instance thread return value */
typedef struct {
    float           frame_rate;
    RK_U64          bit_rate;
    RK_S64          elapsed_time;
    RK_S32          frame_count;
    RK_S64          stream_size;
    RK_S64          delay;
} MpiEncMultiCtxRet;

typedef struct {
    MpiEncTestArgs      *cmd;       // pointer to global command line info
    const char          *name;
    RK_S32              chn;

    pthread_t           thd;        // thread for for each instance
    MpiEncTestData      ctx;        // context of encoder
    MpiEncMultiCtxRet   ret;        // return of encoder
} MpiEncMultiCtxInfo;

#ifdef __cplusplus
}
#endif

#endif /*__MPI_ENC_UTILS_H__*/
