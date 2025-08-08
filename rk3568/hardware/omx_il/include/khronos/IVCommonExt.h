/*
 * Copyright (C) 2021 HiHope Open Source Organization .
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

#ifndef IVCommon_Ext_h
#define IVCommon_Ext_h

typedef enum OMX_COLOR_FORMATEXTTYPE {
    OMX_COLOR_FormatAndroidOpaque = 0x7F000789,
}OMX_COLOR_FORMATEXTTYPE;

typedef enum {
    HAL_PIXEL_FORMAT_sRGB_A_8888 = 0xC,
    HAL_PIXEL_FORMAT_sRGB_X_8888 = 0xD,

    HAL_PIXEL_FORMAT_YCbCr_422_I = 0x14,
    HAL_PIXEL_FORMAT_YCrCb_NV12 = 0x15, // YUY2
    HAL_PIXEL_FORMAT_YCrCb_NV12_VIDEO = 0x16,
    HAL_PIXEL_FORMAT_YCrCb_NV12_10 = 0x17, // YUY2_1obit
    HAL_PIXEL_FORMAT_YCbCr_422_SP_10 = 0x18, //
    HAL_PIXEL_FORMAT_YCrCb_420_SP_10 = 0x19, //

    HAL_PIXEL_FORMAT_YUV420_8BIT_I = 0x1A, // 420I 8bit
    HAL_PIXEL_FORMAT_YUV420_10BIT_I = 0x1B, // 420I 10bit
    HAL_PIXEL_FORMAT_Y210 = 0x1C, // 422I 10bit

    HAL_PIXEL_FORMAT_BPP_1 = 0x30,
    HAL_PIXEL_FORMAT_BPP_2 = 0x31,
    HAL_PIXEL_FORMAT_BPP_4 = 0x32,
    HAL_PIXEL_FORMAT_BPP_8 = 0x33,
    HAL_PIXEL_FORMAT_YCbCr_420_888,
    HAL_PIXEL_FORMAT_YV12,
    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
}rk_pixel_format_t;

enum {
    HAL_PIXEL_FORMAT_RGBA_8888 = 1,
    HAL_PIXEL_FORMAT_RGBX_8888 = 2,
    HAL_PIXEL_FORMAT_RGB_888 = 3,
    HAL_PIXEL_FORMAT_RGB_565 = 4,
    HAL_PIXEL_FORMAT_BGRA_8888 = 5
};

#endif