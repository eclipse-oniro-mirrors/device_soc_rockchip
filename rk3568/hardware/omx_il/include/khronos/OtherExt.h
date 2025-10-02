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

#ifndef Other_Ext_h
#define Other_Ext_h

#include <OMX_Core.h>
/**
 * colorspace
 */
typedef enum OMX_ROCKCHIP_EXT_COLORSPACE {
    OMX_RK_EXT_ColorspaceBT709 = 1,
    OMX_RK_EXT_ColorspaceBT2020,
    OMX_RK_EXT_ColorspaceMax = 0x7FFFFFFF
} OMX_RK_EXT_COLORSPACE;

/**
 * dynamic range
 */
typedef enum OMX_ROCKCHIP_EXT_DYNCRANGE {
    OMX_RK_EXT_DyncrangeSDR = 0,
    OMX_RK_EXT_DyncrangeHDR10,
    OMX_RK_EXT_DyncrangeHDRHLG,
    OMX_RK_EXT_DyncrangeHDRDOLBY,
    OMX_RK_EXT_DyncrangeMax = 0x7FFFFFFF
} OMX_RK_EXT_DYNCRANGE;

/* Structure Rockchip extension HDR param of the component */
typedef struct OMX_EXTENSION_VIDEO_PARAM_HDR {
    OMX_U32 nSize;                  /**< size of the structure in bytes */
    OMX_VERSIONTYPE nVersion;       /**< OMX specification version information */
    OMX_RK_EXT_COLORSPACE eColorSpace;    /**< Color space */
    OMX_RK_EXT_DYNCRANGE eDyncRange;    /**< dynamic range */
} OMX_EXTENSION_VIDEO_PARAM_HDR;

#endif