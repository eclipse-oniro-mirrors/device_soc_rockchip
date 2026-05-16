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
 
#ifndef Index_Ext_h
#define Index_Ext_h

#include <OMX_Index.h>

#define ROCKCHIP_INDEX_PARAM_ENABLE_THUMBNAIL \
    "OMX.SEC.index.enableThumbnailMode"
#define ROCKCHIP_INDEX_CONFIG_VIDEO_INTRAPERIOD \
    "OMX.SEC.index.VideoIntraPeriod"
#define ROCKCHIP_INDEX_PARAM_ENABLE_ANB    \
    "OMX.rockchip.index.enableAndroidNativeBuffers"
#define ROCKCHIP_INDEX_PARAM_GET_ANB_Usage  \
    "OMX.rockchip.index.getAndroidNativeBufferUsage"
#define ROCKCHIP_INDEX_PARAM_USE_ANB      \
    "OMX.rockchip.index.useAndroidNativeBuffer2"
#define ROCKCHIP_INDEX_PARAM_STORE_METADATA_BUFFER  \
    "OMX.rockchip.index.storeMetaDataInBuffers"
#define ROCKCHIP_INDEX_PARAM_STORE_ANW_BUFFER   \
    "OMX.rockchip.index.storeANWBufferInMetadata"
#define ROCKCHIP_INDEX_PARAM_PREPEND_SPSPPS_TO_IDR  \
    "OMX.rockchip.index.prependSPSPPSToIDRFrames"
#define ROCKCHIP_INDEX_PARAM_RKWFD  \
    "OMX.rk.index.encoder.wifidisplay"
#define ROCKCHIP_INDEX_PARAM_THUMBNAIL_CODECPROFILE \
    "OMX.rk.index.decoder.thumbnail.codecprofile"
#define ROCKCHIP_INDEX_PREPARE_ADAPTIVE_PLAYBACK \
    "OMX.rockchip.index.prepareForAdaptivePlayback"
#define ROCKCHIP_INDEX_DESCRIBE_COLORFORMAT \
    "OMX.rockchip.index.describeColorFormat"
#define ROCKCHIP_INDEX_PARAM_ROCKCHIP_DEC_EXTENSION_DIV3  \
    "OMX.rk.index.decoder.extension.div3"
#define ROCKCHIP_INDEX_PARAM_ROCKCHIP_DEC_EXTENSION_USE_DTS \
    "OMX.rk.index.decoder.extension.useDts"
#define ROCKCHIP_INDEX_PARAM_ROCKCHIP_DEC_EXTENSION_THUMBNAILCODECPROFILE  \
    "OMX.rk.index.decoder.extension.thumbNailcodecProfile"
#define ROCKCHIP_INDEX_PARAM_EXTENDED_VIDEO \
    "OMX.Topaz.index.param.extended_video"
#define ROCKCHIP_INDEX_PARAM_DSECRIBECOLORASPECTS \
    "OMX.rockchip.index.describeColorAspects"
#define ROCKCHIP_INDEX_PARAM_ALLOCATENATIVEHANDLE \
    "OMX.rockchip.index.allocateNativeHandle"

typedef enum OMX_INDEXEXEXTTYPE {
    OMX_IndexRockchipExtensions = 0x70000000,
    OMX_IndexParamVideoHDRRockchipExtensions,
    OMX_IndexParamEnableThumbnailMode = 0x7F000001,
    OMX_IndexConfigVideoIntraPeriod = 0x7F000002,
    OMX_IndexParamEnableAndroidBuffers = 0x7F000011,
    OMX_IndexParamGetAndroidNativeBufferUsage = 0x7F000012,
    OMX_IndexParamUseAndroidNativeBuffer = 0x7F000013,
    OMX_IndexParamStoreMetaDataBuffer = 0x7F000014,
    OMX_IndexParamPrependSPSPPSToIDR = 0x7F000015,
    OMX_IndexRkEncExtendedWfdState = 0x7F000018,
    OMX_IndexParamprepareForAdaptivePlayback = 0x7F000016,
    OMX_IndexParamdescribeColorFormat = 0x7F000017,
    OMX_IndexParamRkDecoderExtensionDiv3 = 0x7F050000,
    OMX_IndexParamRkDecoderExtensionUseDts = 0x7F050001,
    OMX_IndexParamRkDecoderExtensionThumbNailCodecProfile  = 0x7F050002,
    OMX_IndexParamRkEncExtendedVideo = 0x7F050003,
    OMX_IndexParamRkDescribeColorAspects = 0x7F000062,
    OMX_IndexParamAllocateNativeHandle = 0x7F00005D,
    OMX_IndexParamStoreANWBuffer = 0x7F00006D,

    OMX_IndexParamAudioAndroidAc3 = OMX_IndexKhronosExtensions + 0x00400001,
    OMX_IndexParamAudioAndroidOpus,
    OMX_IndexParamAudioAndroidAacPresentation,
    OMX_IndexParamAudioAndroidEac3,
    OMX_IndexParamAudioProfileQuerySupported,
    OMX_IndexParamVideoAndroidVp8Encoder = OMX_IndexKhronosExtensions + 0x00600007,
    OMX_IndexParamSliceSegments,
    OMX_IndexConfigAndroidIntraRefresh,
    OMX_IndexParamAndroidVideoTemporalLayering,
    OMX_IndexConfigAndroidVideoTemporalLayering,
    OMX_IndexConfigAutoFramerateConversion = OMX_IndexKhronosExtensions + 0x00800000,
    OMX_IndexConfigPriority,
    OMX_IndexConfigOperatingRate,
    OMX_IndexParamConsumerUsageBits,
} OMX_INDEXEXEXTTYPE;


#endif