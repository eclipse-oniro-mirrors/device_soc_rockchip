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
#include "codec_jpeg_impl.h"
#include <algorithm>
#include <ashmem.h>
#include <display_type.h>
#include <dlfcn.h>
#include <hdf_base.h>
#include <mutex>
#include <securec.h>
#include <sys/mman.h>
#include <unistd.h>
#include "codec_jpeg_decoder.h"
#include "codec_log_wrapper.h"
namespace OHOS {
namespace VDI {
namespace JPEG {
RKMppApi *CodecJpegImpl::mppApi_ = nullptr;
IDisplayBufferVdi* CodecJpegImpl::displayVdi_ = nullptr;
intptr_t CodecJpegImpl::libHandle_ = 0;
static std::once_flag g_Initflag;
const static std::string g_libDispaly = "libdisplay_buffer_vdi_impl.z.so";

using CreateDisplayBufferVdi = IDisplayBufferVdi*(*)(void);

CodecJpegImpl::CodecJpegImpl()
{}

CodecJpegImpl::~CodecJpegImpl()
{}

int32_t CodecJpegImpl::Init()
{
    CODEC_LOGI("enter");
    std::call_once(g_Initflag, [&] {
        auto ret = GetMppApi(&mppApi_);
        if (ret != HDF_SUCCESS) {
            CODEC_LOGE("GetMppApi ret %{public}d.", ret);
            return;
        }
        libHandle_ = reinterpret_cast<intptr_t>(dlopen(g_libDispaly.c_str(), RTLD_LAZY));
        if (!libHandle_) {
            CODEC_LOGE("libHandle_ is null, errno:%{public}d", errno);
            return;
        }
        CreateDisplayBufferVdi displayVdiCreate = (CreateDisplayBufferVdi)dlsym(reinterpret_cast<void*>(libHandle_), "CreateDisplayBufferVdi");
        if (displayVdiCreate == nullptr) {
            CODEC_LOGE("displayVdiCreate is nullptr, errno:%{public}d", errno);
            return;
        }
        
        displayVdi_ = displayVdiCreate();
    });
    if (mppApi_ == nullptr) {
        CODEC_LOGE("mppApi_ is nullptr");
        return HDF_FAILURE;
    }

    if (displayVdi_ == nullptr) {
        CODEC_LOGE("displayVdi_ is nullptr");
        return HDF_FAILURE;
    }
    
    return HDF_SUCCESS;
}

void CodecJpegImpl::DeInit()
{
    CODEC_LOGI("enter");
}

int32_t CodecJpegImpl::AllocateBuffer(BufferHandle **buffer, uint32_t size)
{
    if (!displayVdi_) {
        CODEC_LOGE("displayVdi_ is nullptr");
        return HDF_FAILURE;
    }
    
    AllocInfo alloc = {.width = AlignUp(size/2/4, 16),    // 2: min size, 8:pixel size of RGBA8888, 16: stride
                       .height = 2,                     // 2: min size
                       .usage =  HBM_USE_CPU_READ | HBM_USE_CPU_WRITE | HBM_USE_MEM_DMA,
                       .format = PIXEL_FMT_RGBA_8888};
    return displayVdi_->AllocMem(alloc, *buffer);
}

int32_t CodecJpegImpl::FreeBuffer(BufferHandle *buffer)
{
    if (!displayVdi_) {
        CODEC_LOGE("displayVdi_ is nullptr");
        return HDF_FAILURE;
    }
    if (buffer == nullptr) {
        CODEC_LOGE("buffer is nullptr");
        return HDF_ERR_INVALID_PARAM;
    }
    
    displayVdi_->FreeMem(*buffer);
    return HDF_SUCCESS;
}

int32_t CodecJpegImpl::DeCode(BufferHandle *buffer, BufferHandle *outBuffer, const struct CodecJpegDecInfo &decInfo)
{
    if (buffer == nullptr || outBuffer == nullptr) {
        CODEC_LOGE("buffer is nullptr or outBuffer is nullptr.");
        return HDF_ERR_INVALID_PARAM;
    }
    if (mppApi_ == nullptr) {
        CODEC_LOGE("mppApi_ is nullptr, please Init first!");
        return HDF_ERR_INVALID_PARAM;
    }

    CodecJpegDecoder decoder(mppApi_);
    auto ret = decoder.DeCode(buffer, outBuffer, decInfo);
    if (ret != HDF_SUCCESS) {
        CODEC_LOGE("mppApi_ is nullptr, please Init first!");
    }

    return ret;
}
}  // namespace JPEG
}  // namespace VDI
}  // namespace OHOS

