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
#include "codec_jpeg_decoder.h"
#include <ashmem.h>
#include <display_type.h>
#include <hdf_base.h>
#include <securec.h>
#include <thread>
#include <unistd.h>
#include "codec_jpeg_helper.h"
#include "codec_log_wrapper.h"
#include "codec_scope_guard.h"
#ifdef DUMP_FILE
#include <fstream>
#endif
namespace OHOS {
namespace VDI {
namespace JPEG {
RKMppApi *CodecJpegDecoder::mppApi_ = nullptr;
CodecJpegDecoder::CodecJpegDecoder()
    : width_(0), height_(0), format_(MPP_FMT_YUV420SP), mppCtx_(nullptr), mpi_(nullptr), memGroup_(nullptr),
      packet_(nullptr), frame_(nullptr), callback_(nullptr), running_(false), threadTask_(nullptr)
{}

CodecJpegDecoder::~CodecJpegDecoder()
{
    CODEC_LOGI("enter");
    width_ = 0;
    height_ = 0;
    running_.store(false);
    // DeInit();// mpp_destroy in descontruct may crash
    format_ = MPP_FMT_YUV420SP;
}

int32_t CodecJpegDecoder::Init()
{
    CODEC_LOGI("enter");

    if (mppApi_ == nullptr && GetMppApi(&mppApi_) != HDF_SUCCESS) {
        CODEC_LOGE("GetMppApi error");
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}
void CodecJpegDecoder::DeInit()
{
    CODEC_LOGI("enter");
    if (threadTask_) {
        threadTask_->join();
        threadTask_ = nullptr;
    }

    if (mppApi_ != NULL) {
        ResetMppBuffer();
        if (memGroup_) {
            mppApi_->HdiMppBufferGroupPut(memGroup_);
            memGroup_ = nullptr;
        }
        if (mppCtx_) {
            mppApi_->HdiMppDestroy(mppCtx_);
            mppCtx_ = nullptr;
            mpi_ = nullptr;
        }
    }
    std::lock_guard<std::mutex> lk(mutex_);
    callback_ = nullptr;
}
int32_t CodecJpegDecoder::DeCode(BufferHandle *buffer, BufferHandle *outBuffer, const struct CodecJpegDecInfo &decInfo,
    CodecJpegCallbackHwi *callback)
{
    CODEC_LOGI("enter");
    if (running_.load()) {
        CODEC_LOGE("task is running");
        return HDF_ERR_DEVICE_BUSY;
    }
    if (threadTask_ != nullptr) {
        threadTask_->join();
    }

    if (!PrePare()) {
        CODEC_LOGE("PrePare failed");
        return HDF_FAILURE;
    }
    if (!SetFormat(outBuffer->format)) {
        CODEC_LOGE("format %{public}d set error", outBuffer->format);
        return HDF_ERR_INVALID_PARAM;
    }
    callback_ = callback;
    // thread start
    threadTask_ = std::make_shared<std::thread>([&, buffer, outBuffer, decInfo] {
        CODEC_LOGI("jpeg task decode run.");
        running_.store(true);
        width_ = decInfo.imageWidth;
        height_ = decInfo.imageHeight;
        if (SendData(decInfo, buffer->fd, outBuffer) != MPP_OK) {
            CODEC_LOGE("Send data error");
            running_.store(false);
            OnEvent(HDF_FAILURE);
            return;
        }
        if (GetFrame() != MPP_OK) {
            CODEC_LOGE("Recv frame error");
            running_.store(false);
            OnEvent(HDF_FAILURE);
            return;
        }
        DumpOutFile();
        CODEC_LOGI("jpeg decode end.");
        OnEvent(HDF_SUCCESS);
        running_.store(false);
    });
    return HDF_SUCCESS;
}

MPP_RET CodecJpegDecoder::SendData(const struct CodecJpegDecInfo &decInfo, int32_t fd, BufferHandle *outHandle)
{
    CODEC_LOGI("enter");
    MppBuffer pktBuf = nullptr;
    MppBuffer frmBuf = nullptr;
    OHOS::CodecScopeGuard scope([&] {
        if (pktBuf) {
            mppApi_->HdiMppBufferPutWithCaller(pktBuf, __func__);
            pktBuf = nullptr;
        }
        if (frmBuf) {
            mppApi_->HdiMppBufferPutWithCaller(frmBuf, __func__);
            frmBuf = nullptr;
        }
    });
    ResetMppBuffer();
    auto ret = InitPacketBuffer(decInfo, fd, pktBuf);
    if (ret != MPP_OK) {
        CODEC_LOGE("InitPacketBuffer error %{public}d", ret);
        return ret;
    }
    // init frame_
    mppApi_->HdiMppFrameInit(&frame_);
#ifndef USE_RGA
    MppBufferInfo inputCommit;
    memset_s(&inputCommit, sizeof(inputCommit), 0, sizeof(inputCommit));
    inputCommit.type = MPP_BUFFER_TYPE_DRM;
    inputCommit.size = outHandle->size;
    inputCommit.fd = outHandle->fd;
    ret = mppApi_->HdiMppBufferImportWithTag(nullptr, &inputCommit, &frmBuf, MODULE_TAG, __func__);
#else
    ret = mppApi_->HdiMppBufferGetWithTag(memGroup_, &frmBuf, horStride * verStride * 2,  // 2: max len = 2*width*height
        MODULE_TAG, __func__);
#endif
    if (ret != MPP_OK) {
        CODEC_LOGE(" mpp buffer import/get  error %{public}d", ret);
        return ret;
    }
    mppApi_->HdiMppFrameSetBuffer(frame_, frmBuf);
    ret = MppTaskProcess();
    return ret;
}
MPP_RET CodecJpegDecoder::InitPacketBuffer(const struct CodecJpegDecInfo &decInfo, int32_t fd, MppBuffer &pktBuf)
{
    uint32_t horStride = AlignUp(width_, 16);   // 16: width alignment
    uint32_t verStride = AlignUp(height_, 16);  // 16: height alignment
    // get pkt
    MPP_RET ret = mppApi_->HdiMppBufferGetWithTag(memGroup_, &pktBuf, horStride * verStride * 2, MODULE_TAG, __func__);
    if (ret != MPP_OK) {
        CODEC_LOGE(" mpp_buffer_get packet buffer error %{public}d", ret);
        return ret;
    }
    mppApi_->HdiMppPacketInitWithBuffer(&packet_, pktBuf);
    int8_t *bufAddr = reinterpret_cast<int8_t *>(mppApi_->HdiMppBufferGetPtrWithCaller(pktBuf, __func__));
    if (bufAddr == nullptr) {
        CODEC_LOGE(" mpp_buffer_get_ptr  error");
        return MPP_NOK;
    }
    CodecJpegHelper jpegHelper;
    auto len = jpegHelper.JpegAssemble(decInfo, bufAddr, fd);
    if (len < 0) {
        CODEC_LOGE(" JpegAssemble error %{public}d", len);
        return MPP_NOK;
    }
    DumpInFile(reinterpret_cast<char *>(bufAddr), len);
    mppApi_->HdiMppPacketSetLength(packet_, len);
    mppApi_->HdiMppPacketSetEos(packet_);
    return MPP_OK;
}

MPP_RET CodecJpegDecoder::MppTaskProcess()
{
    MppTask task = nullptr;
    /* start queue input task */
    auto ret = mpi_->poll(mppCtx_, MPP_PORT_INPUT, MPP_POLL_BLOCK);
    if (MPP_OK != ret) {
        CODEC_LOGE("poll input error %{public}d", ret);
        return ret;
    }
    /* input queue */
    ret = mpi_->dequeue(mppCtx_, MPP_PORT_INPUT, &task);
    if (MPP_OK != ret) {
        CODEC_LOGE("dequeue input error %{public}d", ret);
        return ret;
    }
    mppApi_->HdiMppTaskMetaSetPacket(task, KEY_INPUT_PACKET, packet_);
    mppApi_->HdiMppTaskMetaSetFrame(task, KEY_OUTPUT_FRAME, frame_);
    /* input queue */
    ret = mpi_->enqueue(mppCtx_, MPP_PORT_INPUT, task);
    if (ret != MPP_OK) {
        CODEC_LOGE("enqueue input error %{public}d", ret);
    }
    return ret;
}
MPP_RET CodecJpegDecoder::GetFrame()
{
    CODEC_LOGI("enter.");
    MppTask task = nullptr;
    /* poll and wait here */
    MPP_RET ret = mpi_->poll(mppCtx_, MPP_PORT_OUTPUT, MPP_POLL_BLOCK);
    if (ret != MPP_OK) {
        CODEC_LOGE("poll output error %{public}d", ret);
        return ret;
    }

    /* output queue */
    ret = mpi_->dequeue(mppCtx_, MPP_PORT_OUTPUT, &task);
    if (ret != MPP_OK) {
        CODEC_LOGE("dequeue output error %{public}d", ret);
        return ret;
    }

    MppFrame frameOut = NULL;
    mppApi_->HdiMppTaskMetaGetFrame(task, KEY_OUTPUT_FRAME, &frameOut);
    auto err = mppApi_->HdiMppFrameGetErrinfo(frameOut) | mppApi_->HdiMppFrameGetDiscard(frameOut);
    (void)err;
    /* output queue */
    ret = mpi_->enqueue(mppCtx_, MPP_PORT_OUTPUT, task);
    if (ret != MPP_OK) {
        CODEC_LOGE("enqueue output error %{public}d", ret);
        return ret;
    }
    if (frameOut != frame_) {
        CODEC_LOGE("frameOut is not match with frame_ %{public}d", ret);
        mppApi_->HdiMppFrameDeinit(&frameOut);
        return MPP_NOK;
    }
    return MPP_OK;
}

void CodecJpegDecoder::DumpOutFile()
{
#ifdef DUMP_FILE
    MppBuffer buf = mppApi_->HdiMppFrameGetBuffer(frame_);
    auto size = mppApi_->HdiMppBufferGetSizeWithCaller(buf, __func__);
    auto ptr = mppApi_->HdiMppBufferGetPtrWithCaller(buf, __func__);
    std::ofstream out("/data/out.raw", std::ios::trunc | std::ios::binary);
    if (!out.is_open()) {
        CODEC_LOGE("file open error");
        return;
    }
    out.write(reinterpret_cast<char *>(ptr), size);  // 3: byte alignment, 2:byte alignment
    out.flush();
#endif
}
void CodecJpegDecoder::DumpInFile(char *data, int32_t size)
{
#ifdef DUMP_FILE
    std::ofstream out("/data/in.raw", std::ios::trunc | std::ios::binary);
    if (!out.is_open()) {
        CODEC_LOGE("file open error");
        return;
    }
    out.write(data, size);  // 3: byte alignment, 2:byte alignment
    out.flush();
#endif
}
bool CodecJpegDecoder::SetFormat(int32_t format)
{
    bool ret = true;
    switch (format) {
        case PIXEL_FMT_YCBCR_420_SP:
            format_ = MPP_FMT_YUV420SP;
            break;
        case PIXEL_FMT_YCRCB_420_SP:
            format_ = MPP_FMT_YUV420SP_VU;
            break;
        case PIXEL_FMT_BGR_565:
            format_ = MPP_FMT_BGR565;
            break;
        case PIXEL_FMT_RGB_888:
            format_ = MPP_FMT_RGB888;
            break;
        default:
            CODEC_LOGE("unsupport pixformat %{public}d", format);
            ret = false;
            break;
    }
    if (ret) {
        auto err = mpi_->control(mppCtx_, MPP_DEC_SET_OUTPUT_FORMAT, &format_);
        if (err != MPP_OK) {
            CODEC_LOGE("set output fromat error %{public}d", err);
            return false;
        }
    }
    return ret;
}
void CodecJpegDecoder::ResetMppBuffer()
{
    if (memGroup_) {
        mppApi_->HdiMppBufferGroupClear(memGroup_);
    }
    if (packet_ != nullptr) {
        mppApi_->HdiMppPacketDeinit(&packet_);
        packet_ = nullptr;
    }
    if (frame_ != nullptr) {
        mppApi_->HdiMppFrameDeinit(&frame_);
        frame_ = nullptr;
    }
}

bool CodecJpegDecoder::PrePare(bool isDecoder)
{
    MPP_RET ret = mppApi_->HdiMppCreate(&mppCtx_, &mpi_);
    if (ret != MPP_OK) {
        CODEC_LOGE("HdiMppCreate error %{public}d", ret);
        return false;
    }
    ret = mppApi_->HdiMppInit(mppCtx_, isDecoder ? MPP_CTX_DEC : MPP_CTX_ENC, MPP_VIDEO_CodingMJPEG);
    if (ret != MPP_OK) {
        CODEC_LOGE("HdiMppInit error %{public}d", ret);
        return false;
    }
    MppPollType timeout = MPP_POLL_BLOCK;
    ret = mpi_->control(mppCtx_, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (ret != MPP_OK) {
        CODEC_LOGE("set output timeout error %{public}d", ret);
        return false;
    }
    ret = mpi_->control(mppCtx_, MPP_SET_INPUT_TIMEOUT, &timeout);
    if (ret != MPP_OK) {
        CODEC_LOGE("set input timeout error %{public}d", ret);
        return false;
    }
    mppApi_->HdiMppBufferGroupGet(&memGroup_, MPP_BUFFER_TYPE_DRM, MPP_BUFFER_INTERNAL, nullptr, __func__);
    mppApi_->HdiMppBufferGroupLimitConfig(memGroup_, 0, 24);  // 24:buffer group limit
    return true;
}
}  // namespace JPEG
}  // namespace VDI
}  // namespace OHOS