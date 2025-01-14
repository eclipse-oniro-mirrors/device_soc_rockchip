/*
 *
 * Copyright 2015 Rockchip Electronics Co., LTD.
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

#include "vpu_mem_legacy.h"
#include <string.h>
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "hdf_log.h"
#include "mpp_buffer.h"
#include "securec.h"
#include "vpu.h"

#define VPU_MEM_DBG_FUNCTION            (0x00000001)

static RK_U32 vpu_mem_debug = 0;

#define vpu_mem_dbg(flag, fmt, ...)     _mpp_dbg(vpu_mem_debug, flag, fmt, ## __VA_ARGS__)
#define vpu_mem_dbg_f(flag, fmt, ...)   _mpp_dbg_f(vpu_mem_debug, flag, fmt, ## __VA_ARGS__)

static RK_S32
commit_memory_handle(vpu_display_mem_pool *p, RK_S32 mem_hdl, RK_S32 size)
{
    MppBufferInfo info;
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;

    HDF_LOGD("in  pool %p hnl %p size %d", p, mem_hdl, size);
    memset_s(&info, sizeof(MppBufferInfo), 0, sizeof(MppBufferInfo));
    info.type = MPP_BUFFER_TYPE_ION;
    info.fd = mem_hdl;
    info.size = size & 0x07ffffff;
    info.index = (size & 0xf8000000) >> 27; // 27 bit

    p_mempool->size = size;
    p_mempool->buff_size = size;

    (*(mRKMppApi.Hdimpp_buffer_import_with_tag))(p_mempool->group, &info, NULL, MODULE_TAG, __FUNCTION__);
    HDF_LOGD("out pool %p fd %d", p, info.fd);
    return info.fd;
}

static void* get_free_memory_vpumem(vpu_display_mem_pool *p)
{
    MPP_RET ret = MPP_OK;
    MppBuffer buffer = NULL;
    VPUMemLinear_t *dmabuf = (*(mRKMppApi.Hdimpp_osal_calloc))(__FUNCTION__, sizeof(VPUMemLinear_t));
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    if (dmabuf == NULL) {
        return NULL;
    }
    HDF_LOGD("in pool %p", p);
    ret = (*(mRKMppApi.HdiMppBufferGetWithTag))(p_mempool->group, &buffer, p_mempool->size, MODULE_TAG, __FUNCTION__);
    if (MPP_OK != ret) {
        (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, dmabuf);
        return NULL;
    }
    dmabuf->phy_addr = (RK_U32)(*(mRKMppApi.HdiMppBufferGetFdWithCaller))(buffer, __FUNCTION__);
    dmabuf->vir_addr = (RK_U32*)(*(mRKMppApi.HdiMppBufferGetPtrWithCaller))(buffer, __FUNCTION__);
    dmabuf->size = p_mempool->size;
    dmabuf->offset = (RK_U32*)buffer;
    HDF_LOGD("out pool %p ret %p fd %d size %d buffer %p", p, dmabuf, \
        dmabuf->phy_addr, dmabuf->size, buffer);
    return dmabuf;
}

static RK_S32 inc_used_memory_handle_ref(vpu_display_mem_pool *p, void * hdl)
{
    VPUMemLinear_t *dmabuf = (VPUMemLinear_t *)hdl;
    MppBuffer buffer = (MppBuffer)dmabuf->offset;
    HDF_LOGD("pool %p hnd %p buffer %p", p, hdl, buffer);
    if (buffer != NULL) {
        (*(mRKMppApi.Hdimpp_buffer_inc_ref_with_caller))(buffer, __FUNCTION__);
    }

    (void)p;
    return MPP_OK;
}

static RK_S32 put_used_memory_handle(vpu_display_mem_pool *p, void *hdl)
{
    VPUMemLinear_t *dmabuf = (VPUMemLinear_t *)hdl;
    MppBuffer buf = (MppBuffer)dmabuf->offset;
    HDF_LOGD("pool %p hnd %p buffer %p", p, hdl, buf);
    if (buf != NULL) {
        (*(mRKMppApi.HdiMppBufferPutWithCaller))(buf, __FUNCTION__);
        memset_s(dmabuf, sizeof(VPUMemLinear_t), 0, sizeof(VPUMemLinear_t));
    }
    (void)p;
    return MPP_OK;
}

static RK_S32 get_free_memory_num(vpu_display_mem_pool *p)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    RK_S32 ret = (p_mempool->group) ?
                 ((*(mRKMppApi.Hdimpp_buffer_group_unused))(p_mempool->group)) : (0);

    HDF_LOGD("pool %p ret %d", p, ret);
    return ret;
}

static RK_S32 reset_vpu_mem_pool(vpu_display_mem_pool *p)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;
    (*(mRKMppApi.HdiMppBufferGroupClear))(p_mempool->group);
    return 0;
}


vpu_display_mem_pool* open_vpu_memory_pool()
{
    vpu_display_mem_pool_impl *p_mempool =
    (*(mRKMppApi.Hdimpp_osal_calloc))(__FUNCTION__, sizeof(vpu_display_mem_pool_impl));

    (*(mRKMppApi.HdiMppEnvGetU32))("vpu_mem_debug", &vpu_mem_debug, 0);
    HDF_LOGD("in pool %p", p_mempool);

    if (p_mempool == NULL) {
        return NULL;
    }
    (*(mRKMppApi.HdiMppBufferGroupGet))(&p_mempool->group,
        MPP_BUFFER_TYPE_ION, MPP_BUFFER_EXTERNAL, MODULE_TAG, __FUNCTION__);
    if (p_mempool->group == NULL) {
        return NULL;
    }
    p_mempool->commit_hdl     = commit_memory_handle;
    p_mempool->get_free       = get_free_memory_vpumem;
    p_mempool->put_used       = put_used_memory_handle;
    p_mempool->inc_used       = inc_used_memory_handle_ref;
    p_mempool->reset          = reset_vpu_mem_pool;
    p_mempool->get_unused_num = get_free_memory_num;
    p_mempool->version        = 1;
    p_mempool->buff_size      = -1;

    HDF_LOGD("out pool %p group %p", p_mempool, p_mempool->group);
    return (vpu_display_mem_pool*)p_mempool;
}

void close_vpu_memory_pool(vpu_display_mem_pool *p)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)p;

    HDF_LOGD("pool %p group %p", p_mempool, p_mempool->group);
    (*(mRKMppApi.HdiMppBufferGroupPut))(p_mempool->group);
    (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, p_mempool);
    return;
}

int create_vpu_memory_pool_allocator(vpu_display_mem_pool **ipool,
                                     int num, int size)
{
    vpu_display_mem_pool_impl *p_mempool =
    (*(mRKMppApi.Hdimpp_osal_calloc))(__FUNCTION__, sizeof(vpu_display_mem_pool_impl));

    (*(mRKMppApi.HdiMppEnvGetU32))("vpu_mem_debug", &vpu_mem_debug, 0);
    HDF_LOGD("in pool %p num %d size %d", p_mempool, num, size);

    if (p_mempool == NULL) {
        return -1;
    }

    (*(mRKMppApi.HdiMppBufferGroupGet))(&p_mempool->group,
        MPP_BUFFER_TYPE_ION, MPP_BUFFER_INTERNAL, MODULE_TAG, __FUNCTION__);
    (*(mRKMppApi.HdiMppBufferGroupLimitConfig))(p_mempool->group, 0, num + 4); // num + 4
    p_mempool->commit_hdl     = commit_memory_handle;
    p_mempool->get_free       = get_free_memory_vpumem;
    p_mempool->put_used       = put_used_memory_handle;
    p_mempool->inc_used       = inc_used_memory_handle_ref;
    p_mempool->reset          = reset_vpu_mem_pool;
    p_mempool->get_unused_num = get_free_memory_num;
    p_mempool->version        = 0;
    p_mempool->buff_size      = size;
    p_mempool->size           = size;
    *ipool = (vpu_display_mem_pool*)p_mempool;

    HDF_LOGD("out pool %p group %p", p_mempool, p_mempool->group);
    return 0;
}

void release_vpu_memory_pool_allocator(vpu_display_mem_pool *ipool)
{
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)ipool;
    if (p_mempool == NULL) {
        return;
    }

    HDF_LOGD("pool %p group %p", p_mempool, p_mempool->group);

    if (p_mempool->group) {
        (*(mRKMppApi.HdiMppBufferGroupPut))(p_mempool->group);
        p_mempool->group = NULL;
    }

    HDF_LOGD("free %p", p_mempool);
    (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, p_mempool);
    return;
}

RK_S32 VPUMemJudgeIommu()
{
    int ret = 0;

    if (VPUClientGetIOMMUStatus() > 0) {
        ret = 1;
    }

    return ret;
}


RK_S32 VPUMallocLinear(VPUMemLinear_t *p, RK_U32 size)
{
    int ret = 0;
    MppBuffer buffer = NULL;
    ret = (*(mRKMppApi.HdiMppBufferGetWithTag))(NULL, &buffer, size, MODULE_TAG, __FUNCTION__);
    if (ret != MPP_OK) {
        return -1;
    }
    p->phy_addr = (RK_U32)(*(mRKMppApi.HdiMppBufferGetFdWithCaller))(buffer, __FUNCTION__);
    p->vir_addr = (RK_U32*)(*(mRKMppApi.HdiMppBufferGetPtrWithCaller))(buffer, __FUNCTION__);
    p->size = size;
    p->offset = (RK_U32*)buffer;
    return 0;
}

RK_S32 VPUMallocLinearFromRender(VPUMemLinear_t *p, RK_U32 size, void *ctx)
{
    VPUMemLinear_t *dma_buf = NULL;
    vpu_display_mem_pool_impl *p_mempool = (vpu_display_mem_pool_impl *)ctx;
    if (ctx == NULL) {
        return VPUMallocLinear(p, size);
    }
    dma_buf = (VPUMemLinear_t *) \
                p_mempool->get_free((vpu_display_mem_pool *)ctx);
    memset_s(p, sizeof(VPUMemLinear_t), 0, sizeof(VPUMemLinear_t));
    if (dma_buf != NULL) {
        if (dma_buf->size < size) {
            (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, dma_buf);
            return -1;
        }
        if (memcpy_s(p, sizeof(VPUMemLinear_t), dma_buf, sizeof(VPUMemLinear_t)) != EOK) {
            HDF_LOGE("%s memcpy_s no", __func__);
        }
        (*(mRKMppApi.Hdimpp_osal_free))(__FUNCTION__, dma_buf);
        return 0;
    }
    return -1;
}

RK_S32 VPUFreeLinear(VPUMemLinear_t *p)
{
    if (p->offset != NULL) {
        put_used_memory_handle(NULL, p);
    }
    return 0;
}

RK_S32 VPUMemDuplicate(VPUMemLinear_t *dst, VPUMemLinear_t *src)
{
    MppBuffer buffer = (MppBuffer)src->offset;
    if (buffer != NULL) {
        (*(mRKMppApi.Hdimpp_buffer_inc_ref_with_caller))(buffer, __FUNCTION__);
    }
    if (memcpy_s(dst, sizeof(VPUMemLinear_t), src, sizeof(VPUMemLinear_t)) != EOK) {
        HDF_LOGE("%s memcpy_s no", __func__);
    }
    return 0;
}

RK_S32 VPUMemLink(VPUMemLinear_t *p)
{
    (void)p;
    return 0;
}

RK_S32 VPUMemFlush(VPUMemLinear_t *p)
{
    (void)p;
    return 0;
}

RK_S32 VPUMemClean(VPUMemLinear_t *p)
{
    (void)p;
    return 0;
}


RK_S32 VPUMemInvalidate(VPUMemLinear_t *p)
{
    (void)p;
    return 0;
}

RK_S32 VPUMemGetFD(VPUMemLinear_t *p)
{
    RK_S32 fd = 0;
    MppBuffer buffer = (MppBuffer)p->offset;
    fd = (*(mRKMppApi.HdiMppBufferGetFdWithCaller))(buffer, __FUNCTION__);
    return fd;
}

