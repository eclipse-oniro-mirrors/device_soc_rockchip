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
#include <codec_jpeg_vdi.h>
#include <hdf_base.h>
#include <hdf_log.h>
#include <memory>
#include "codec_jpeg_impl.h"
#include "codec_log_wrapper.h"
using namespace OHOS::VDI::JPEG;
static std::shared_ptr<CodecJpegImpl> g_JpegImpl = nullptr;
static int32_t JpegInit()
{
    CODEC_LOGI("enter.");
    if (g_JpegImpl != nullptr) {
        CODEC_LOGE("jpeg impl is inited.");
        return HDF_ERR_DEVICE_BUSY;
    }
    g_JpegImpl = std::make_shared<CodecJpegImpl>();
    return g_JpegImpl->Init();
}

static int32_t JpegDeInit()
{
    CODEC_LOGI("enter.");
    if (g_JpegImpl) {
        g_JpegImpl->DeInit();
    }
    g_JpegImpl = nullptr;
    return HDF_SUCCESS;
}

static int32_t AllocateBuffer(BufferHandle **buffer, uint32_t size)
{
    CODEC_LOGI("enter.");
    if (g_JpegImpl == nullptr) {
        CODEC_LOGE("jpeg decoder is not init.");
        return HDF_ERR_NOPERM;
    }
    return g_JpegImpl->AllocateBuffer(buffer, size);
}

static int32_t FreeBuffer(BufferHandle *buffer)
{
    CODEC_LOGI("enter.");
    if (g_JpegImpl == nullptr) {
        CODEC_LOGE("jpeg decoder is not init.");
        return HDF_ERR_NOPERM;
    }
    return g_JpegImpl->FreeBuffer(buffer);
}

static int32_t DoJpegDecode(BufferHandle *buffer, BufferHandle *outBuffer, const struct CodecJpegDecInfo *decInfo)
{
    CODEC_LOGI("enter.");
    if (g_JpegImpl == nullptr) {
        CODEC_LOGE("jpeg decoder is not init.");
        return HDF_ERR_NOPERM;
    }

    return g_JpegImpl->DeCode(buffer, outBuffer, *decInfo);
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
