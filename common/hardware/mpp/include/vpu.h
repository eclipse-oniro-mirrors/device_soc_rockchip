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

#ifndef __VPU_H__
#define __VPU_H__

#ifdef __cplusplus
extern "C" {
#endif

#define VPU_SUCCESS (0)
#define VPU_FAILURE (-1)

#define VPU_HW_WAIT_OK VPU_SUCCESS
#define VPU_HW_WAIT_ERROR VPU_FAILURE
#define VPU_HW_WAIT_TIMEOUT 1

// vpu decoder 60 registers, size 240B
#define VPU_REG_NUM_DEC (60)
// vpu post processor 41 registers, size 164B
#define VPU_REG_NUM_PP (41)
// vpu decoder + post processor 101 registers, size 404B
#define VPU_REG_NUM_DEC_PP (VPU_REG_NUM_DEC + VPU_REG_NUM_PP)
// vpu encoder 96 registers, size 384B
#define VPU_REG_NUM_ENC (96)

typedef enum {
    VPU_ENC = 0x0,
    VPU_DEC = 0x1,
    VPU_PP = 0x2,
    VPU_DEC_PP = 0x3,
    VPU_DEC_HEVC = 0x4,
    VPU_DEC_RKV = 0x5,
    VPU_ENC_RKV = 0x6,
    VPU_DEC_AVS = 0x7,
    VPU_ENC_VEPU22 = 0x8,
    VPU_TYPE_BUTT,
} VPU_CLIENT_TYPE;

/* Hardware decoder configuration description */

typedef struct VPUHwDecConfig {
    unsigned int maxDecPicWidth;       /* Maximum video decoding width supported  */
    unsigned int maxPpOutPicWidth;     /* Maximum output width of Post-Processor */
    unsigned int h264Support;          /* HW supports h.264 */
    unsigned int jpegSupport;          /* HW supports JPEG */
    unsigned int mpeg4Support;         /* HW supports MPEG-4 */
    unsigned int customMpeg4Support;   /* HW supports custom MPEG-4 features */
    unsigned int vc1Support;           /* HW supports VC-1 Simple */
    unsigned int mpeg2Support;         /* HW supports MPEG-2 */
    unsigned int ppSupport;            /* HW supports post-processor */
    unsigned int ppConfig;             /* HW post-processor functions bitmask */
    unsigned int sorensonSparkSupport; /* HW supports Sorenson Spark */
    unsigned int refBufSupport;        /* HW supports reference picture buffering */
    unsigned int vp6Support;           /* HW supports VP6 */
    unsigned int vp7Support;           /* HW supports VP7 */
    unsigned int vp8Support;           /* HW supports VP8 */
    unsigned int avsSupport;           /* HW supports AVS */
    unsigned int jpegESupport;         /* HW supports JPEG extensions */
    unsigned int rvSupport;            /* HW supports REAL */
    unsigned int mvcSupport;           /* HW supports H264 MVC extension */
} VPUHwDecConfig_t;

/* Hardware encoder configuration description */

typedef struct VPUHwEndConfig {
    unsigned int maxEncodedWidth; /* Maximum supported width for video encoding (not JPEG) */
    unsigned int h264Enabled;     /* HW supports H.264 */
    unsigned int jpegEnabled;     /* HW supports JPEG */
    unsigned int mpeg4Enabled;    /* HW supports MPEG-4 */
    unsigned int vsEnabled;       /* HW supports video stabilization */
    unsigned int rgbEnabled;      /* HW supports RGB input */
    unsigned int reg_size;        /* HW bus type in use */
    unsigned int reserv[2];
} VPUHwEncConfig_t;

typedef enum {
    // common command
    VPU_CMD_REGISTER,
    VPU_CMD_REGISTER_ACK_OK,
    VPU_CMD_REGISTER_ACK_FAIL,
    VPU_CMD_UNREGISTER,

    VPU_SEND_CONFIG,
    VPU_SEND_CONFIG_ACK_OK,
    VPU_SEND_CONFIG_ACK_FAIL,

    VPU_GET_HW_INFO,
    VPU_GET_HW_INFO_ACK_OK,
    VPU_GET_HW_INFO_ACK_FAIL,

    VPU_CMD_BUTT,
} VPU_CMD_TYPE;

int VPUClientInit(VPU_CLIENT_TYPE type);
signed int VPUClientRelease(int socket);
signed int VPUClientSendReg(int socket, unsigned int *regs, unsigned int nregs);
signed int VPUClientSendReg2(signed int socket, signed int offset, signed int size, void *param);
signed int VPUClientWaitResult(int socket, unsigned int *regs, unsigned int nregs, VPU_CMD_TYPE *cmd, signed int *len);
signed int VPUClientGetHwCfg(int socket, unsigned int *cfg, unsigned int cfg_size);
signed int VPUClientGetIOMMUStatus(void);
unsigned int VPUCheckSupportWidth(void);

#ifdef __cplusplus
}
#endif

#endif /* __VPU_H__ */
