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

#include "hdi_mpp_mpi.h"
#include <dlfcn.h>
#include "mpp_platform.h"
#include "hdf_log.h"


void *mLibHandle = nullptr;

void GetMppAPI(RKMppApi *pMppApi)
{
    mLibHandle = dlopen("librockchip_mpp.z.so", RTLD_NOW);
    if (mLibHandle == nullptr) {
        HDF_LOGE("jkf open no\n");
        return ;
    }
    HDF_LOGE("jkf open yes\n");
    pMppApi->HdiMppCreate = (hdiMppCreate)dlsym(mLibHandle, "mpp_create");
    pMppApi->HdiMppInit = (hdiMppInit)dlsym(mLibHandle, "mpp_init");
    pMppApi->HdiMppStart = (hdiMppStart)dlsym(mLibHandle, "mpp_start");
    pMppApi->HdiMppStop = (hdiMppStop)dlsym(mLibHandle, "mpp_stop");
    pMppApi->HdiMppDestroy = (hdiMppDestroy)dlsym(mLibHandle, "mpp_destroy");
    pMppApi->HdiMppCheckSupportFormat = \
        (hdiMppCheckSupportFormat)dlsym(mLibHandle, "mpp_check_support_format");
    pMppApi->HdiMppShowSupportFormat = \
        (hdiMppShowSupportFormat)dlsym(mLibHandle, "mpp_show_support_format");
    pMppApi->HdiMppShowColorFormat = \
        (hdiMppShowColorFormat)dlsym(mLibHandle, "mpp_show_color_format");

    pMppApi->HdiMppPacketNew = \
        (hdiMppPacketNew)dlsym(mLibHandle, "mpp_packet_new");
    pMppApi->HdiMppPacketInit = \
        (hdiMppPacketInit)dlsym(mLibHandle, "mpp_packet_init");
    pMppApi->HdiMppPacketInitWithBuffer = \
        (hdiMppPacketInitWithBuffer)dlsym(mLibHandle, "mpp_packet_init_with_buffer");
    pMppApi->HdiMppPacketCopyInit = \
        (hdiMppPacketCopyInit)dlsym(mLibHandle, "mpp_packet_copy_init");
    pMppApi->HdiMppPacketDeinit = \
        (hdiMppPacketDeinit)dlsym(mLibHandle, "mpp_packet_deinit");
    pMppApi->HdiMppPacketGetEos = \
        (hdiMppPacketGetEos)dlsym(mLibHandle, "mpp_packet_get_eos");
    pMppApi->HdiMppPacketSetEos = \
        (hdiMppPacketSetEos)dlsym(mLibHandle, "mpp_packet_set_eos");
    pMppApi->HdiMppPacketGetPts = \
        (hdiMppPacketGetPts)dlsym(mLibHandle, "mpp_packet_get_pts");
    pMppApi->HdiMppPacketSetPts = \
        (hdiMppPacketSetPts)dlsym(mLibHandle, "mpp_packet_set_pts");
    pMppApi->HdiMppPacketSetData = \
        (hdiMppPacketSetData)dlsym(mLibHandle, "mpp_packet_set_data");
    pMppApi->HdiMppPacketSetSize = \
        (hdiMppPacketSetSize)dlsym(mLibHandle, "mpp_packet_set_size");
    pMppApi->HdiMppPacketGetPos = \
        (hdiMppPacketGetPos)dlsym(mLibHandle, "mpp_packet_get_pos");
    pMppApi->HdiMppPacketSetPos = \
        (hdiMppPacketSetPos)dlsym(mLibHandle, "mpp_packet_set_pos");
    pMppApi->HdiMppPacketSetLength = \
        (hdiMppPacketSetLength)dlsym(mLibHandle, "mpp_packet_set_length");
    pMppApi->HdiMppPacketGetLength = \
        (hdiMppPacketGetLength)dlsym(mLibHandle, "mpp_packet_get_length");
    pMppApi->HdiMppPacketIsPartition = \
        (hdiMppPacketIsPartition)dlsym(mLibHandle, "mpp_packet_is_partition");
    pMppApi->HdiMppPacketIsEoi = \
        (hdiMppPacketIsEoi)dlsym(mLibHandle, "mpp_packet_is_eoi");
    pMppApi->HdiMppMetaSetPacket = \
        (hdiMppMetaSetPacket)dlsym(mLibHandle, "mpp_meta_set_packet");

    pMppApi->HdiMppFrameInit = \
        (hdiMppFrameInit)dlsym(mLibHandle, "mpp_frame_init");
    pMppApi->HdiMppFrameDeinit = \
        (hdiMppFrameDeinit)dlsym(mLibHandle, "mpp_frame_deinit");
    pMppApi->HdiMppFrameGetNext = \
        (hdiMppFrameGetNext)dlsym(mLibHandle, "mpp_frame_get_next");
    pMppApi->HdiMppFrameGetInfoChange = \
        (hdiMppFrameGetInfoChange)dlsym(mLibHandle, "mpp_frame_get_info_change");
    pMppApi->HdiMppFrameGetWidth = \
        (hdiMppFrameGetWidth)dlsym(mLibHandle, "mpp_frame_get_width");
    pMppApi->HdiMppFrameGetHeight = \
        (hdiMppFrameGetHeight)dlsym(mLibHandle, "mpp_frame_get_height");
    pMppApi->HdiMppFrameGetHorStride = \
        (hdiMppFrameGetHorStride)dlsym(mLibHandle, "mpp_frame_get_hor_stride");
    pMppApi->HdiMppFrameGetVerStride = \
        (hdiMppFrameGetVerStride)dlsym(mLibHandle, "mpp_frame_get_ver_stride");
    pMppApi->HdiMppFrameGetBufferSize = \
        (hdiMppFrameGetBufferSize)dlsym(mLibHandle, "mpp_frame_get_buf_size");
    pMppApi->HdiMppFrameGetFormat = \
        (hdiMppFrameGetFormat)dlsym(mLibHandle, "mpp_frame_get_fmt");
    pMppApi->HdiMppFrameGetErrinfo = \
        (hdiMppFrameGetErrinfo)dlsym(mLibHandle, "mpp_frame_get_errinfo");
    pMppApi->HdiMppFrameGetDiscard = \
        (hdiMppFrameGetDiscard)dlsym(mLibHandle, "mpp_frame_get_discard");
    pMppApi->HdiMppFrameGetBuffer = \
        (hdiMppFrameGetBuffer)dlsym(mLibHandle, "mpp_frame_get_buffer");
    pMppApi->HdiMppFrameSetBuffer = \
        (hdiMppFrameSetBuffer)dlsym(mLibHandle, "mpp_frame_set_buffer");
    pMppApi->HdiMppFrameGetEos = \
        (hdiMppFrameGetEos)dlsym(mLibHandle, "mpp_frame_get_eos");
    pMppApi->HdiMppFrameSetEos = \
        (hdiMppFrameSetEos)dlsym(mLibHandle, "mpp_frame_set_eos");
    pMppApi->HdiMppFrameSetFormat = \
        (hdiMppFrameSetFormat)dlsym(mLibHandle, "mpp_frame_set_fmt");
    pMppApi->HdiMppFrameSetWidth = \
        (hdiMppFrameSetWidth)dlsym(mLibHandle, "mpp_frame_set_width");
    pMppApi->HdiMppFrameSetHeight = \
        (hdiMppFrameSetHeight)dlsym(mLibHandle, "mpp_frame_set_height");
    pMppApi->HdiMppFrameSetHorStride = \
        (hdiMppFrameSetHorStride)dlsym(mLibHandle, "mpp_frame_set_hor_stride");
    pMppApi->HdiMppFrameSetVerStride = \
        (hdiMppFrameSetVerStride)dlsym(mLibHandle, "mpp_frame_set_ver_stride");
    pMppApi->HdiMppFrameGetMeta = \
        (hdiMppFrameGetMeta)dlsym(mLibHandle, "mpp_frame_get_meta");

    pMppApi->HdiMppDecCfgInit = \
        (hdiMppDecCfgInit)dlsym(mLibHandle, "mpp_dec_cfg_init");
    pMppApi->HdiMppDecCfgDeinit = \
        (hdiMppDecCfgDeinit)dlsym(mLibHandle, "mpp_dec_cfg_deinit");
    pMppApi->HdiMppDecCfgSetU32 = \
        (hdiMppDecCfgSetU32)dlsym(mLibHandle, "mpp_dec_cfg_set_u32");

    pMppApi->HdiMppEncCfgInit = \
        (hdiMppEncCfgInit)dlsym(mLibHandle, "mpp_enc_cfg_init");
    pMppApi->HdiMppEncCfgDeinit = \
        (hdiMppEncCfgDeinit)dlsym(mLibHandle, "mpp_enc_cfg_deinit");
    pMppApi->HdiMppEncCfgSetS32 = \
        (hdiMppEncCfgSetS32)dlsym(mLibHandle, "mpp_enc_cfg_set_s32");
    pMppApi->HdiMppEncCfgSetU32 = \
        (hdiMppEncCfgSetU32)dlsym(mLibHandle, "mpp_enc_cfg_set_u32");
    pMppApi->HdiMppEncRefCfgInit = \
        (hdiMppEncRefCfgInit)dlsym(mLibHandle, "mpp_enc_ref_cfg_init");
    pMppApi->HdiMppEncRefCfgDeinit = \
        (hdiMppEncRefCfgDeinit)dlsym(mLibHandle, "mpp_enc_ref_cfg_deinit");
    pMppApi->HdiMppEncGenRefCfg = \
        (hdiMppEncGenRefCfg)dlsym(mLibHandle, "mpi_enc_gen_ref_cfg");
    pMppApi->HdiMppEncGenSmartGopRefCfg = \
        (hdiMppEncGenSmartGopRefCfg)dlsym(mLibHandle, "mpi_enc_gen_smart_gop_ref_cfg");

    pMppApi->HdiMppBufferGroupGet = \
        (hdiMppBufferGroupGet)dlsym(mLibHandle, "mpp_buffer_group_get");
    pMppApi->HdiMppBufferGroupPut = \
        (hdiMppBufferGroupPut)dlsym(mLibHandle, "mpp_buffer_group_put");
    pMppApi->HdiMppBufferGroupClear = \
        (hdiMppBufferGroupClear)dlsym(mLibHandle, "mpp_buffer_group_clear");
    pMppApi->HdiMppBufferGroupLimitConfig = \
        (hdiMppBufferGroupLimitConfig)dlsym(mLibHandle, "mpp_buffer_group_limit_config");
    pMppApi->HdiMppBufferGetFdWithCaller = \
        (hdiMppBufferGetFdWithCaller)dlsym(mLibHandle, "mpp_buffer_get_fd_with_caller");
    pMppApi->HdiMppBufferGetWithTag = \
        (hdiMppBufferGetWithTag)dlsym(mLibHandle, "mpp_buffer_get_with_tag");
    pMppApi->HdiMppBufferGetPtrWithCaller = \
        (hdiMppBufferGetPtrWithCaller)dlsym(mLibHandle, "mpp_buffer_get_ptr_with_caller");
    pMppApi->HdiMppBufferGroupUsage = \
        (hdiMppBufferGroupUsage)dlsym(mLibHandle, "mpp_buffer_group_usage");
    pMppApi->HdiMppBufferPutWithCaller = \
        (hdiMppBufferPutWithCaller)dlsym(mLibHandle, "mpp_buffer_put_with_caller");

    pMppApi->HdiMppTaskMetaGetPacket = \
        (hdiMppTaskMetaGetPacket)dlsym(mLibHandle, "mpp_task_meta_get_packet");

    pMppApi->HdiMppEnvGetU32 = \
        (hdiMppEnvGetU32)dlsym(mLibHandle, "mpp_env_get_u32");

    pMppApi->Hdimpp_get_vcodec_dev_name = \
        (hdimpp_get_vcodec_dev_name)dlsym(mLibHandle, "mpp_get_vcodec_dev_name");
    pMppApi->Hdimpp_get_ioctl_version = \
        (hdimpp_get_ioctl_version)dlsym(mLibHandle, "mpp_get_ioctl_version");
    pMppApi->Hdimpp_get_vcodec_type = \
        (hdimpp_get_vcodec_type)dlsym(mLibHandle, "mpp_get_vcodec_type");
    pMppApi->Hdimpp_osal_calloc = \
        (hdimpp_osal_calloc)dlsym(mLibHandle, "mpp_osal_calloc");
    pMppApi->Hdimpp_get_soc_name = \
        (hdimpp_get_soc_name)dlsym(mLibHandle, "mpp_get_soc_name");
    pMppApi->Hdimpp_osal_free = \
        (hdimpp_osal_free)dlsym(mLibHandle, "mpp_osal_free");
    pMppApi->Hdimpp_packet_set_extra_data = \
        (hdimpp_packet_set_extra_data)dlsym(mLibHandle, "mpp_packet_set_extra_data");
    pMppApi->Hdimpp_frame_get_color_primaries = \
        (hdimpp_frame_get_color_primaries)dlsym(mLibHandle, "mpp_frame_get_color_primaries");
    pMppApi->Hdimpp_frame_get_color_trc = \
        (hdimpp_frame_get_color_trc)dlsym(mLibHandle, "mpp_frame_get_color_trc");
    pMppApi->Hdimpp_frame_get_color_range = \
        (hdimpp_frame_get_color_range)dlsym(mLibHandle, "mpp_frame_get_color_range");
    pMppApi->Hdimpp_frame_get_mode = \
        (hdimpp_frame_get_mode)dlsym(mLibHandle, "mpp_frame_get_mode");
    pMppApi->Hdimpp_osal_malloc = \
        (hdimpp_osal_malloc)dlsym(mLibHandle, "mpp_osal_malloc");
    pMppApi->Hdimpp_buffer_get_size_with_caller = \
        (hdimpp_buffer_get_size_with_caller)dlsym(mLibHandle, "mpp_buffer_get_size_with_caller");
    pMppApi->Hdimpp_frame_set_pts = \
        (hdimpp_frame_set_pts)dlsym(mLibHandle, "mpp_frame_set_pts");
    pMppApi->Hdimpp_meta_get_s32 = \
        (hdimpp_meta_get_s32)dlsym(mLibHandle, "mpp_meta_get_s32");
    pMppApi->Hdimpp_packet_get_meta = \
        (hdimpp_packet_get_meta)dlsym(mLibHandle, "mpp_packet_get_meta");
    pMppApi->Hdimpp_enc_ref_cfg_set_keep_cpb = \
        (hdimpp_enc_ref_cfg_set_keep_cpb)dlsym(mLibHandle, "mpp_enc_ref_cfg_set_keep_cpb");
    pMppApi->Hdimpp_enc_ref_cfg_add_st_cfg = \
        (hdimpp_enc_ref_cfg_add_st_cfg)dlsym(mLibHandle, "mpp_enc_ref_cfg_add_st_cfg");
    pMppApi->Hdimpp_enc_ref_cfg_add_lt_cfg = \
        (hdimpp_enc_ref_cfg_add_lt_cfg)dlsym(mLibHandle, "mpp_enc_ref_cfg_add_lt_cfg");
    pMppApi->Hdimpp_enc_ref_cfg_set_cfg_cnt = \
        (hdimpp_enc_ref_cfg_set_cfg_cnt)dlsym(mLibHandle, "mpp_enc_ref_cfg_set_cfg_cnt");
    pMppApi->Hdimpp_buffer_group_unused = \
        (hdimpp_buffer_group_unused)dlsym(mLibHandle, "mpp_buffer_group_unused");
    pMppApi->Hdimpp_enc_ref_cfg_check = \
        (hdimpp_enc_ref_cfg_check)dlsym(mLibHandle, "mpp_enc_ref_cfg_check");
    pMppApi->Hdimpp_frame_get_colorspace = \
        (hdimpp_frame_get_colorspace)dlsym(mLibHandle, "mpp_frame_get_colorspace");
    pMppApi->Hdimpp_meta_set_s32 = \
        (hdimpp_meta_set_s32)dlsym(mLibHandle, "mpp_meta_set_s32");
    pMppApi->Hdimpp_buffer_inc_ref_with_caller = \
        (hdimpp_buffer_inc_ref_with_caller)dlsym(mLibHandle, "mpp_buffer_inc_ref_with_caller");
    pMppApi->Hdimpp_task_meta_get_frame = \
        (hdimpp_task_meta_get_frame)dlsym(mLibHandle, "mpp_task_meta_get_frame");
    pMppApi->Hdimpp_task_meta_set_frame = \
        (hdimpp_task_meta_set_frame)dlsym(mLibHandle, "mpp_task_meta_set_frame");
    pMppApi->Hdimpp_task_meta_set_packet = \
        (hdimpp_task_meta_set_packet)dlsym(mLibHandle, "mpp_task_meta_set_packet");
    pMppApi->Hdimpp_frame_get_poc = \
        (hdimpp_frame_get_poc)dlsym(mLibHandle, "mpp_frame_get_poc");
    pMppApi->Hdimpp_packet_get_data = \
        (hdimpp_packet_get_data)dlsym(mLibHandle, "mpp_packet_get_data");
    pMppApi->Hdimpp_frame_get_pts = \
        (hdimpp_frame_get_pts)dlsym(mLibHandle, "mpp_frame_get_pts");
    pMppApi->Hdimpp_buffer_import_with_tag = \
        (hdimpp_buffer_import_with_tag)dlsym(mLibHandle, "mpp_buffer_import_with_tag");
}

void ReleaseMppAPI()
{
    if (mLibHandle != nullptr) {
        dlclose(mLibHandle);
        mLibHandle = nullptr;
    }
}
