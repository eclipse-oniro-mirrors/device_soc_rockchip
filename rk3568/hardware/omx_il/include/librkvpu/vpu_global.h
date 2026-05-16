/*
 *
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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
/*
   * File:
   * vpu_global.h
   * Description:
   * Global struct definition in VPU module
   * Author:
   *     Jian Huan
   * Date:
   *    2010-11-23 21:48:40
 */
#ifndef __VPU_GLOBAL_H_
#define __VPU_GLOBAL_H_

#include "vpu_macro.h"
#include "vpu_mem.h"
#include "vpu_api.h"

typedef struct {
    RK_U32      StartCode;
    RK_U32      SliceLength;
    TIME_STAMP  SliceTime;
    RK_U32      SliceType;
    RK_U32      SliceNum;
    RK_U32      Res[2];
}VPU_BITSTREAM;  /* completely same as RK28 */

typedef struct {
    RK_U32      InputAddr[2];
    RK_U32      OutputAddr[2];
    RK_U32      InputWidth;
    RK_U32      InputHeight;
    RK_U32      OutputWidth;
    RK_U32      OutputHeight;
    RK_U32      ColorType;
    RK_U32      ScaleEn;
    RK_U32      RotateEn;
    RK_U32      DitherEn;
    RK_U32      DeblkEn;
    RK_U32      DeinterlaceEn;
    RK_U32      Res[5];
}VPU_POSTPROCESSING;

typedef enum {
    VPU_H264ENC_YUV420_PLANAR = 0,  /* YYYY... UUUU... VVVV */
    VPU_H264ENC_YUV420_SEMIPLANAR = 1,  /* YYYY... UVUVUV...    */
    VPU_H264ENC_YUV422_INTERLEAVED_YUYV = 2,    /* YUYVYUYV...          */
    VPU_H264ENC_YUV422_INTERLEAVED_UYVY = 3,    /* UYVYUYVY...          */
    VPU_H264ENC_RGB565 = 4, /* 16-bit RGB           */
    VPU_H264ENC_BGR565 = 5, /* 16-bit RGB           */
    VPU_H264ENC_RGB555 = 6, /* 15-bit RGB           */
    VPU_H264ENC_BGR555 = 7, /* 15-bit RGB           */
    VPU_H264ENC_RGB444 = 8, /* 12-bit RGB           */
    VPU_H264ENC_BGR444 = 9, /* 12-bit RGB           */
    VPU_H264ENC_RGB888 = 10,    /* 24-bit RGB           */
    VPU_H264ENC_BGR888 = 11,    /* 24-bit RGB           */
    VPU_H264ENC_RGB101010 = 12, /* 30-bit RGB           */
    VPU_H264ENC_BGR101010 = 13  /* 30-bit RGB           */
} H264EncPictureType;

#endif /* _VPU_GLOBAL_ */
