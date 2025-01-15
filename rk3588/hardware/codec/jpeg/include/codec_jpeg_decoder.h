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
#include <atomic>
#include <cinttypes>
#include <codec_jpeg_vdi.h>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "hdi_mpp_mpi.h"
namespace OHOS {
namespace VDI {
namespace JPEG {
class CodecJpegDecoder {
public:
    explicit CodecJpegDecoder();

    ~CodecJpegDecoder();

    int32_t Init();

    void DeInit();

    int32_t DeCode(BufferHandle *buffer, BufferHandle *outBuffer, const struct CodecJpegDecInfo &decInfo,
        CodecJpegCallbackHwi *callback);

private:
    void ResetMppBuffer();

    bool PrePare(bool isDecoder = true);

    inline uint32_t AlignUp(uint32_t val, uint32_t align)
    {
        return (val + align - 1) & (~(align - 1));
    }

    inline void OnEvent(int32_t result)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (callback_ && callback_->OnEvent) {
            callback_->OnEvent(result);
        }
    }

    MPP_RET SendData(const struct CodecJpegDecInfo &decInfo, int32_t fd, BufferHandle *outHandle);

    MPP_RET InitPacketBuffer(const struct CodecJpegDecInfo &decInfo, int32_t fd, MppBuffer &pktBuf);

    MPP_RET MppTaskProcess();

    MPP_RET GetFrame();

    void DumpOutFile();

    void DumpInFile(char *data, int32_t size);

    bool SetFormat(int32_t format);

private:
    uint32_t width_;
    uint32_t height_;
    MppFrameFormat format_;
    MppCtx mppCtx_;
    MppApi *mpi_;
    static RKMppApi *mppApi_;
    MppBufferGroup memGroup_;
    MppPacket packet_;
    MppFrame frame_;
    CodecJpegCallbackHwi *callback_;
    std::atomic<bool> running_;
    std::mutex mutex_;
    std::shared_ptr<std::thread> threadTask_;
};
}  // namespace JPEG
}  // namespace VDI
}  // namespace OHOS
#endif  // HDI_JPEG_DECODER_H
