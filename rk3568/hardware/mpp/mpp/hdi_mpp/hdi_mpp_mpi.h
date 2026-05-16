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

#ifndef CODEC_MPP_HDI_MPI_H
#define CODEC_MPP_HDI_MPI_H

#include "rk_mpi.h"
#include "mpp_platform.h"

// mpp base api
typedef MPP_RET (*hdiMppCreate)(MppCtx *, MppApi **);
typedef MPP_RET (*hdiMppInit)(MppCtx, MppCtxType, MppCodingType);
typedef MPP_RET (*hdiMppStart)(MppCtx);
typedef MPP_RET (*hdiMppStop)(MppCtx);
typedef MPP_RET (*hdiMppDestroy)(MppCtx);
typedef MPP_RET (*hdiMppCheckSupportFormat)(MppCtxType, MppCodingType);
typedef void (*hdiMppShowSupportFormat)(void);
typedef void (*hdiMppShowColorFormat)(void);
// mpp packet api
typedef MPP_RET (*hdiMppPacketNew)(MppPacket *);
typedef MPP_RET (*hdiMppPacketInit)(MppPacket *, void *, size_t);
typedef MPP_RET (*hdiMppPacketInitWithBuffer)(MppPacket *, MppBuffer);
typedef MPP_RET (*hdiMppPacketCopyInit)(MppPacket *, const MppPacket);
typedef MPP_RET (*hdiMppPacketDeinit)(MppPacket*);
typedef RK_U32  (*hdiMppPacketGetEos)(MppPacket);
typedef MPP_RET (*hdiMppPacketSetEos)(MppPacket);
typedef RK_S64  (*hdiMppPacketGetPts)(const MppPacket);
typedef void    (*hdiMppPacketSetPts)(MppPacket, RK_S64);
typedef void    (*hdiMppPacketSetData)(MppPacket, void *);
typedef void    (*hdiMppPacketSetSize)(MppPacket, size_t);
typedef void*   (*hdiMppPacketGetPos)(const MppPacket);
typedef void    (*hdiMppPacketSetPos)(MppPacket, void *);
typedef void    (*hdiMppPacketSetLength)(MppPacket, size_t);
typedef size_t  (*hdiMppPacketGetLength)(const MppPacket);
typedef RK_U32  (*hdiMppPacketIsPartition)(const MppPacket);
typedef RK_U32  (*hdiMppPacketIsEoi)(const MppPacket);
typedef MPP_RET (*hdiMppMetaSetPacket)(MppMeta, MppMetaKey, MppPacket);
// mpp frame api
typedef MPP_RET (*hdiMppFrameInit)(MppFrame*);
typedef MPP_RET (*hdiMppFrameDeinit)(MppFrame*);
typedef MppFrame (*hdiMppFrameGetNext)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetInfoChange)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetWidth)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetHeight)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetHorStride)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetVerStride)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetBufferSize)(MppFrame);
typedef MppFrameFormat (*hdiMppFrameGetFormat)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetErrinfo)(const MppFrame);
typedef RK_U32 (*hdiMppFrameGetDiscard)(const MppFrame);
typedef MppBuffer (*hdiMppFrameGetBuffer)(const MppFrame);
typedef void (*hdiMppFrameSetBuffer)(MppFrame, MppBuffer);
typedef RK_U32 (*hdiMppFrameGetEos)(const MppFrame);
typedef void (*hdiMppFrameSetEos)(const MppFrame, RK_U32);
typedef void (*hdiMppFrameSetFormat)(MppFrame, MppFrameFormat);
typedef void (*hdiMppFrameSetWidth)(MppFrame, RK_U32);
typedef void (*hdiMppFrameSetHeight)(MppFrame, RK_U32);
typedef void (*hdiMppFrameSetHorStride)(MppFrame, RK_U32);
typedef void (*hdiMppFrameSetVerStride)(MppFrame, RK_U32);
typedef MppMeta (*hdiMppFrameGetMeta)(const MppFrame);
// mpp dec config api
typedef MPP_RET (*hdiMppDecCfgDeinit)(MppDecCfg *);
typedef MPP_RET (*hdiMppDecCfgInit)(MppDecCfg *);
typedef MPP_RET (*hdiMppDecCfgSetU32)(MppDecCfg, const char *, RK_U32);
// mpp enc config api
typedef MPP_RET (*hdiMppEncCfgInit)(MppEncCfg *);
typedef MPP_RET (*hdiMppEncCfgDeinit)(MppEncCfg);
typedef MPP_RET (*hdiMppEncCfgSetS32)(MppEncCfg, const char *, RK_S32);
typedef MPP_RET (*hdiMppEncCfgSetU32)(MppEncCfg, const char *, RK_U32);
typedef MPP_RET (*hdiMppEncRefCfgInit)(MppEncRefCfg *);
typedef MPP_RET (*hdiMppEncRefCfgDeinit)(MppEncRefCfg *);
typedef MPP_RET (*hdiMppEncGenRefCfg)(MppEncRefCfg, RK_U32);
typedef MPP_RET (*hdiMppEncGenSmartGopRefCfg)(MppEncRefCfg, RK_U32, RK_S32);
// mpp buffer group api
typedef MPP_RET (*hdiMppBufferGroupGet)(MppBufferGroup *, MppBufferType, MppBufferMode, const char *, const char *);
typedef MPP_RET (*hdiMppBufferGroupPut)(MppBufferGroup);
typedef MPP_RET (*hdiMppBufferGroupClear)(MppBufferGroup);
typedef MPP_RET (*hdiMppBufferGroupLimitConfig)(MppBufferGroup, size_t, RK_S32);
typedef int (*hdiMppBufferGetFdWithCaller)(MppBuffer buffer, const char *caller);
typedef MPP_RET (*hdiMppBufferGetWithTag)(MppBufferGroup, MppBuffer *, size_t, const char *, const char *);
typedef void* (*hdiMppBufferGetPtrWithCaller)(MppBuffer, const char *);
typedef size_t (*hdiMppBufferGroupUsage)(MppBufferGroup);
typedef MPP_RET (*hdiMppBufferPutWithCaller)(MppBuffer, const char *);
// mpp task api
typedef MPP_RET (*hdiMppTaskMetaGetPacket)(MppTask task, MppMetaKey key, MppPacket *packet);
// mpp env api
typedef RK_S32 (*hdiMppEnvGetU32)(const char *, RK_U32 *, RK_U32);
typedef char* (*hdimpp_get_vcodec_dev_name)(MppCtxType type, MppCodingType coding);
typedef MppIoctlVersion (*hdimpp_get_ioctl_version)(void);
typedef RK_U32 (*hdimpp_get_vcodec_type)(void);
typedef void* (*hdimpp_osal_calloc)(const char *caller, size_t size);
typedef char* (*hdimpp_get_soc_name)(void);
typedef void (*hdimpp_osal_free)(const char *caller, void *ptr);
typedef MPP_RET (*hdimpp_packet_set_extra_data)(MppPacket packet);
typedef MppFrameColorPrimaries (*hdimpp_frame_get_color_primaries)(const MppFrame frame);
typedef MppFrameColorTransferCharacteristic (*hdimpp_frame_get_color_trc)(const MppFrame frame);
typedef MppFrameColorRange (*hdimpp_frame_get_color_range)(const MppFrame frame);
typedef RK_U32  (*hdimpp_frame_get_mode)(const MppFrame frame);
typedef void* (*hdimpp_osal_malloc)(const char *caller, size_t size);
typedef size_t (*hdimpp_buffer_get_size_with_caller)(MppBuffer buffer, const char *caller);
typedef void (*hdimpp_frame_set_pts)(MppFrame frame, RK_S64 pts);
typedef MPP_RET (*hdimpp_meta_get_s32)(MppMeta meta, MppMetaKey key, RK_S32 *val);
typedef MppMeta (*hdimpp_packet_get_meta)(const MppPacket packet);
typedef MPP_RET (*hdimpp_enc_ref_cfg_set_keep_cpb)(MppEncRefCfg ref, RK_S32 keep);
typedef MPP_RET (*hdimpp_enc_ref_cfg_add_st_cfg)(MppEncRefCfg ref, RK_S32 cnt, MppEncRefStFrmCfg *frm);
typedef MPP_RET (*hdimpp_enc_ref_cfg_add_lt_cfg)(MppEncRefCfg ref, RK_S32 cnt, MppEncRefLtFrmCfg *frm);
typedef MPP_RET (*hdimpp_enc_ref_cfg_set_cfg_cnt)(MppEncRefCfg ref, RK_S32 lt_cnt, RK_S32 st_cnt);
typedef RK_S32  (*hdimpp_buffer_group_unused)(MppBufferGroup group);
typedef MPP_RET (*hdimpp_enc_ref_cfg_check)(MppEncRefCfg ref);
typedef MppFrameColorSpace (*hdimpp_frame_get_colorspace)(const MppFrame frame);
typedef MPP_RET (*hdimpp_meta_set_s32)(MppMeta meta, MppMetaKey key, RK_S32 val);
typedef MPP_RET (*hdimpp_buffer_inc_ref_with_caller)(MppBuffer buffer, const char *caller);
typedef MPP_RET (*hdimpp_task_meta_get_frame)(MppTask task, MppMetaKey key, MppFrame *frame);
typedef MPP_RET (*hdimpp_task_meta_set_frame)(MppTask task, MppMetaKey key, MppFrame frame);
typedef MPP_RET (*hdimpp_task_meta_set_packet)(MppTask task, MppMetaKey key, MppPacket packet);
typedef RK_U32  (*hdimpp_frame_get_poc)(const MppFrame frame);
typedef void*   (*hdimpp_packet_get_data)(const MppPacket packet);
typedef RK_S64  (*hdimpp_frame_get_pts)(const MppFrame frame);
typedef MPP_RET (*hdimpp_buffer_import_with_tag)(MppBufferGroup group, MppBufferInfo *info, MppBuffer *buffer,
                                   const char *tag, const char *caller);

typedef struct {
    // mpp base api
    hdiMppCreate HdiMppCreate;
    hdiMppInit HdiMppInit;
    hdiMppStart HdiMppStart;
    hdiMppStop HdiMppStop;
    hdiMppDestroy HdiMppDestroy;
    hdiMppCheckSupportFormat HdiMppCheckSupportFormat;
    hdiMppShowSupportFormat HdiMppShowSupportFormat;
    hdiMppShowColorFormat HdiMppShowColorFormat;
    // mpp packet api
    hdiMppPacketNew HdiMppPacketNew;
    hdiMppPacketInit HdiMppPacketInit;
    hdiMppPacketInitWithBuffer HdiMppPacketInitWithBuffer;
    hdiMppPacketCopyInit HdiMppPacketCopyInit;
    hdiMppPacketDeinit HdiMppPacketDeinit;
    hdiMppPacketGetEos HdiMppPacketGetEos;
    hdiMppPacketSetEos HdiMppPacketSetEos;
    hdiMppPacketGetPts HdiMppPacketGetPts;
    hdiMppPacketSetPts HdiMppPacketSetPts;
    hdiMppPacketSetData HdiMppPacketSetData;
    hdiMppPacketSetSize HdiMppPacketSetSize;
    hdiMppPacketGetPos HdiMppPacketGetPos;
    hdiMppPacketSetPos HdiMppPacketSetPos;
    hdiMppPacketSetLength HdiMppPacketSetLength;
    hdiMppPacketGetLength HdiMppPacketGetLength;
    hdiMppPacketIsPartition HdiMppPacketIsPartition;
    hdiMppPacketIsEoi HdiMppPacketIsEoi;
    hdiMppMetaSetPacket HdiMppMetaSetPacket;
    // mpp frame api
    hdiMppFrameInit HdiMppFrameInit;
    hdiMppFrameDeinit HdiMppFrameDeinit;
    hdiMppFrameGetNext HdiMppFrameGetNext;
    hdiMppFrameGetInfoChange HdiMppFrameGetInfoChange;
    hdiMppFrameGetWidth HdiMppFrameGetWidth;
    hdiMppFrameGetHeight HdiMppFrameGetHeight;
    hdiMppFrameGetHorStride HdiMppFrameGetHorStride;
    hdiMppFrameGetVerStride HdiMppFrameGetVerStride;
    hdiMppFrameGetBufferSize HdiMppFrameGetBufferSize;
    hdiMppFrameGetFormat HdiMppFrameGetFormat;
    hdiMppFrameGetErrinfo HdiMppFrameGetErrinfo;
    hdiMppFrameGetDiscard HdiMppFrameGetDiscard;
    hdiMppFrameGetBuffer HdiMppFrameGetBuffer;
    hdiMppFrameSetBuffer HdiMppFrameSetBuffer;
    hdiMppFrameGetEos HdiMppFrameGetEos;
    hdiMppFrameSetEos HdiMppFrameSetEos;
    hdiMppFrameSetFormat HdiMppFrameSetFormat;
    hdiMppFrameSetWidth HdiMppFrameSetWidth;
    hdiMppFrameSetHeight HdiMppFrameSetHeight;
    hdiMppFrameSetHorStride HdiMppFrameSetHorStride;
    hdiMppFrameSetVerStride HdiMppFrameSetVerStride;
    hdiMppFrameGetMeta HdiMppFrameGetMeta;
    // mpp dec config api
    hdiMppDecCfgDeinit HdiMppDecCfgDeinit;
    hdiMppDecCfgInit HdiMppDecCfgInit;
    hdiMppDecCfgSetU32 HdiMppDecCfgSetU32;
    // mpp enc config api
    hdiMppEncCfgInit HdiMppEncCfgInit;
    hdiMppEncCfgDeinit HdiMppEncCfgDeinit;
    hdiMppEncCfgSetS32 HdiMppEncCfgSetS32;
    hdiMppEncCfgSetU32 HdiMppEncCfgSetU32;
    hdiMppEncRefCfgInit HdiMppEncRefCfgInit;
    hdiMppEncRefCfgDeinit HdiMppEncRefCfgDeinit;
    hdiMppEncGenRefCfg HdiMppEncGenRefCfg;
    hdiMppEncGenSmartGopRefCfg HdiMppEncGenSmartGopRefCfg;
    // mpp buffer group api
    hdiMppBufferGroupGet HdiMppBufferGroupGet;
    hdiMppBufferGroupPut HdiMppBufferGroupPut;
    hdiMppBufferGroupClear HdiMppBufferGroupClear;
    hdiMppBufferGroupLimitConfig HdiMppBufferGroupLimitConfig;
    hdiMppBufferGetFdWithCaller HdiMppBufferGetFdWithCaller;
    hdiMppBufferGetWithTag HdiMppBufferGetWithTag;
    hdiMppBufferGetPtrWithCaller HdiMppBufferGetPtrWithCaller;
    hdiMppBufferGroupUsage HdiMppBufferGroupUsage;
    hdiMppBufferPutWithCaller HdiMppBufferPutWithCaller;
    // mpp task api
    hdiMppTaskMetaGetPacket HdiMppTaskMetaGetPacket;
    // mpp env api
    hdiMppEnvGetU32 HdiMppEnvGetU32;
    hdimpp_get_vcodec_dev_name Hdimpp_get_vcodec_dev_name;
    hdimpp_get_ioctl_version  Hdimpp_get_ioctl_version;
    hdimpp_get_vcodec_type Hdimpp_get_vcodec_type;
    hdimpp_osal_calloc Hdimpp_osal_calloc;
    hdimpp_get_soc_name Hdimpp_get_soc_name;
    hdimpp_osal_free Hdimpp_osal_free;
    hdimpp_packet_set_extra_data Hdimpp_packet_set_extra_data;
    hdimpp_frame_get_color_primaries Hdimpp_frame_get_color_primaries;
    hdimpp_frame_get_color_trc Hdimpp_frame_get_color_trc;
    hdimpp_frame_get_color_range Hdimpp_frame_get_color_range;
    hdimpp_frame_get_mode Hdimpp_frame_get_mode;
    hdimpp_osal_malloc Hdimpp_osal_malloc;
    hdimpp_buffer_get_size_with_caller Hdimpp_buffer_get_size_with_caller;
    hdimpp_frame_set_pts Hdimpp_frame_set_pts;
    hdimpp_meta_get_s32 Hdimpp_meta_get_s32;
    hdimpp_packet_get_meta Hdimpp_packet_get_meta;
    hdimpp_enc_ref_cfg_set_keep_cpb Hdimpp_enc_ref_cfg_set_keep_cpb;
    hdimpp_enc_ref_cfg_add_st_cfg Hdimpp_enc_ref_cfg_add_st_cfg;
    hdimpp_enc_ref_cfg_add_lt_cfg Hdimpp_enc_ref_cfg_add_lt_cfg;
    hdimpp_enc_ref_cfg_set_cfg_cnt Hdimpp_enc_ref_cfg_set_cfg_cnt;
    hdimpp_buffer_group_unused Hdimpp_buffer_group_unused;
    hdimpp_enc_ref_cfg_check Hdimpp_enc_ref_cfg_check;
    hdimpp_frame_get_colorspace Hdimpp_frame_get_colorspace;
    hdimpp_meta_set_s32 Hdimpp_meta_set_s32;
    hdimpp_buffer_inc_ref_with_caller Hdimpp_buffer_inc_ref_with_caller;
    hdimpp_task_meta_get_frame Hdimpp_task_meta_get_frame;
    hdimpp_task_meta_set_frame Hdimpp_task_meta_set_frame;
    hdimpp_task_meta_set_packet Hdimpp_task_meta_set_packet;
    hdimpp_frame_get_poc Hdimpp_frame_get_poc;
    hdimpp_packet_get_data Hdimpp_packet_get_data;
    hdimpp_frame_get_pts Hdimpp_frame_get_pts;
    hdimpp_buffer_import_with_tag Hdimpp_buffer_import_with_tag;
}RKMppApi;

void GetMppAPI(RKMppApi *pMppApi);
void ReleaseMppAPI();

#endif // CODEC_MPP_HDI_MPI_H