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
#ifndef HDI_JPEG_IMPL_H
#define HDI_JPEG_IMPL_H
#include <cinttypes>
#include <codec_jpeg_vdi.h>
#include "hdi_mpp_mpi.h"
#include "idisplay_buffer_vdi.h"
#include "v1_0/display_buffer_type.h"
namespace OHOS {
namespace VDI {
namespace JPEG {
using namespace OHOS::HDI::Display::Buffer::V1_0;
class CodecJpegImpl {
public:
    explicit CodecJpegImpl();

    ~CodecJpegImpl();

    int32_t Init();

    void DeInit();

    int32_t AllocateBuffer(BufferHandle **buffer, uint32_t size);

    int32_t FreeBuffer(BufferHandle *buffer);

    int32_t DeCode(BufferHandle *buffer, BufferHandle *outBuffer, const struct CodecJpegDecInfo &decInfo);

private:
    inline uint32_t AlignUp(uint32_t val, uint32_t align)
    {
        return (val + align - 1) & (~(align - 1));
    }
private:
    static RKMppApi *mppApi_;
    static IDisplayBufferVdi* displayVdi_;
    static intptr_t libHandle_;
};
}  // namespace JPEG
}  // namespace VDI
}  // namespace OHOS
#endif  // HDI_JPEG_DECODER_H
