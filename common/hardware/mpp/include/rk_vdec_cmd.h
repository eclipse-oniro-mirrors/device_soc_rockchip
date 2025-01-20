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

#ifndef __RK_VDEC_CMD_H__
#define __RK_VDEC_CMD_H__

/*
 * decoder query interface is only for debug usage
 */
#define MPP_DEC_QUERY_STATUS (0x00000001)
#define MPP_DEC_QUERY_WAIT (0x00000002)
#define MPP_DEC_QUERY_FPS (0x00000004)
#define MPP_DEC_QUERY_BPS (0x00000008)
#define MPP_DEC_QUERY_DEC_IN_PKT (0x00000010)
#define MPP_DEC_QUERY_DEC_WORK (0x00000020)
#define MPP_DEC_QUERY_DEC_OUT_FRM (0x00000040)

#define MPP_DEC_QUERY_ALL                                                                                              \
    (MPP_DEC_QUERY_STATUS | MPP_DEC_QUERY_WAIT | MPP_DEC_QUERY_FPS | MPP_DEC_QUERY_BPS | MPP_DEC_QUERY_DEC_IN_PKT |    \
     MPP_DEC_QUERY_DEC_WORK | MPP_DEC_QUERY_DEC_OUT_FRM)

typedef struct MppDecQueryCfg_t {
    /*
     * 32 bit query flag for query data check
     * Each bit represent a query data switch.
     * bit 0 - for querying decoder runtime status
     * bit 1 - for querying decoder runtime waiting status
     * bit 2 - for querying decoder realtime decode fps
     * bit 3 - for querying decoder realtime input bps
     * bit 4 - for querying decoder input packet count
     * bit 5 - for querying decoder start hardware times
     * bit 6 - for querying decoder output frame count
     */
    unsigned int query_flag;

    /* 64 bit query data output */
    unsigned int rt_status;
    unsigned int rt_wait;
    unsigned int rt_fps;
    unsigned int rt_bps;
    unsigned int dec_in_pkt_cnt;
    unsigned int dec_hw_run_cnt;
    unsigned int dec_out_frm_cnt;
} MppDecQueryCfg;

#endif /* __RK_VDEC_CMD_H__ */