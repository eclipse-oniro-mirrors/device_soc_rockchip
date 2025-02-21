/*
 * Copyright (c) 2022 HiHope Open Source Organization .
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

#ifndef HDI_MPP_CONFIG_H
#define HDI_MPP_CONFIG_H

#include "codec_type.h"
#include "rk_mpi.h"
#include "rk_vdec_cfg.h"
#include "rk_venc_cfg.h"
#include "hdi_mpp.h"

int32_t GetDefaultConfig(RKHdiBaseComponent *pBaseComponent);
int32_t GetDefaultHorStride(int32_t width, CodecPixelFormat fmtHDI);
int32_t InitConfig(RKHdiBaseComponent *pBaseComponent);
int32_t SetParamWidth(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamHeight(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamPixleFmt(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamHorStride(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamVerStride(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamFps(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamDrop(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamRateControl(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamGop(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamMimeCodecType(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamCodecType(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamSplitParse(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamCodecFrameNum(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t ValidateEncSetup(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t SetParamEncSetupAVC(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamBufferSize(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamWidth(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamHeight(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamPixleFmt(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamStride(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamFps(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamRateControl(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamPixleFmt(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamGop(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamMimeCodecType(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamCodecType(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamSplitParse(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamCodecFrameNum(RKHdiBaseComponent *pBaseComponent, Param *param);
int32_t GetParamDrop(RKHdiBaseComponent *pBaseComponent, Param *param);

#endif // HDI_MPP_CONFIG_H