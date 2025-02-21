/*
 * Copyright (c) 2023 Shenzhen Kaihong DID Co., Ltd.
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
#include <ashmem.h>
#include <codec_jpeg_vdi.h>
#include <display_type.h>
#include <hdf_base.h>
#include <hdf_log.h>
#include <memory>
#include <securec.h>
#include <sys/mman.h>
#include <unistd.h>
#include "codec_jpeg_decoder.h"
#include "codec_log_wrapper.h"
using namespace OHOS::VDI::JPEG;
static std::shared_ptr<CodecJpegDecoder> g_JpegDecoder = nullptr;
static int32_t JpegInit()
{
    CODEC_LOGI("enter.");
    if (g_JpegDecoder != nullptr) {
        CODEC_LOGE("jpeg decoder is inited.");
        return HDF_ERR_DEVICE_BUSY;
    }
    g_JpegDecoder = std::make_shared<CodecJpegDecoder>();
    return g_JpegDecoder->Init();
}

static int32_t JpegDeInit()
{
    CODEC_LOGI("enter.");
    if (g_JpegDecoder) {
        g_JpegDecoder->DeInit();
    }
    g_JpegDecoder = nullptr;
    return HDF_SUCCESS;
}

static int32_t AllocateBuffer(BufferHandle **buffer, uint32_t size)
{
    CODEC_LOGI("enter.");
    if (g_JpegDecoder == nullptr) {
        CODEC_LOGE("jpeg decoder is not init.");
        return HDF_ERR_NOPERM;
    }
    int fd = OHOS::AshmemCreate(0, size);
    OHOS::AshmemSetProt(fd, PROT_READ | PROT_WRITE);
    BufferHandle *bufferHandle = reinterpret_cast<BufferHandle *>(malloc(sizeof(BufferHandle)));
    if (bufferHandle == nullptr) {
        CODEC_LOGE("malloc ret nullptr");
        return HDF_ERR_MALLOC_FAIL;
    }
    auto ret = memset_s(bufferHandle, sizeof(BufferHandle), 0, sizeof(BufferHandle));
    if (ret != EOK) {
        CODEC_LOGE("memset_s ret err %{public}d", ret);
        return HDF_ERR_MALLOC_FAIL;
    }

    bufferHandle->width = size;
    bufferHandle->height = 1;
    bufferHandle->stride = size;
    bufferHandle->size = size;
    bufferHandle->format = 0;
    bufferHandle->usage = HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_SHARE;
    bufferHandle->fd = fd;
    *buffer = bufferHandle;
    return HDF_SUCCESS;
}

static int32_t FreeBuffer(BufferHandle *buffer)
{
    CODEC_LOGI("enter.");
    if (g_JpegDecoder == nullptr) {
        CODEC_LOGE("jpeg decoder is not init.");
        return HDF_ERR_NOPERM;
    }
    if (buffer == nullptr) {
        CODEC_LOGE("buffer is null.");
        return HDF_ERR_INVALID_PARAM;
    }
    if (buffer->fd > -1) {
        close(buffer->fd);
    }
    CODEC_LOGD("free bufferhandle %p width[%{public}d] height[%{public}d] ", buffer, buffer->width, buffer->height);
    free(buffer);
    return HDF_SUCCESS;
}

static int32_t DoJpegDecode(BufferHandle *buffer, BufferHandle *outBuffer, const struct CodecJpegDecInfo *decInfo,
    CodecJpegCallbackHwi *callback)
{
    CODEC_LOGI("enter.");
    if (g_JpegDecoder == nullptr) {
        CODEC_LOGE("jpeg decoder is not init.");
        return HDF_ERR_NOPERM;
    }
    if (buffer == nullptr || outBuffer == nullptr) {
        CODEC_LOGE("invalid param.");
        return HDF_ERR_INVALID_PARAM;
    }
    auto ret = g_JpegDecoder->DeCode(buffer, outBuffer, *decInfo, callback);
    if (ret != HDF_SUCCESS) {
        CODEC_LOGE("jpeg decode err %{public}d.", ret);
        return HDF_ERR_INVALID_PARAM;
    }
    return ret;
}

static ICodecJpegHwi g_jpegHwi = {.JpegInit = JpegInit,
                                  .JpegDeInit = JpegDeInit,
                                  .AllocateInBuffer = AllocateBuffer,
                                  .FreeInBuffer = FreeBuffer,
                                  .DoJpegDecode = DoJpegDecode};

extern "C" ICodecJpegHwi *GetCodecJpegHwi()
{
    return &g_jpegHwi;
}
