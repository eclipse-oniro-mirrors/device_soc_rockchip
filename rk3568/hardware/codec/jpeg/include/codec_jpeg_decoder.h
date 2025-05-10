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
#ifndef HDI_JPEG_DECODER_H
#define HDI_JPEG_DECODER_H
#include <cinttypes>
#include <codec_jpeg_vdi.h>
#include "hdi_mpp_mpi.h"
namespace OHOS {
namespace VDI {
namespace JPEG {
class CodecJpegDecoder {
public:
    explicit CodecJpegDecoder(RKMppApi *mppApi);

    ~CodecJpegDecoder();

    int32_t DeCode(BufferHandle *buffer, BufferHandle *outBuffer, const struct CodecJpegDecInfo &decInfo);

private:
    void Destory();

    void ResetMppBuffer();

    bool PrePare(bool isDecoder = true);

    inline uint32_t AlignUp(uint32_t val, uint32_t align)
    {
        return (val + align - 1) & (~(align - 1));
    }

    MPP_RET SendData(const struct CodecJpegDecInfo &decInfo, BufferHandle* buffer, BufferHandle *outHandle);

    MPP_RET MppTaskProcess();

    MPP_RET GetFrame();

    void DumpOutFile();

    void DumpInFile(MppBuffer pktBuf);

    bool SetFormat(int32_t format);

private:
    uint32_t width_;
    uint32_t height_;
    MppFrameFormat format_;
    MppCtx mppCtx_;
    MppApi *mpi_;
    RKMppApi *mppApi_;
    MppBufferGroup memGroup_;
    MppPacket packet_;
    MppFrame frame_;
};
}  // namespace JPEG
}  // namespace VDI
}  // namespace OHOS
#endif  // HDI_JPEG_DECODER_H
