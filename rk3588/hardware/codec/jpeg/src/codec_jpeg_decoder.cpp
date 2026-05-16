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
CodecJpegDecoder::CodecJpegDecoder(RKMppApi *mppApi)
    : width_(0), height_(0), format_(MPP_FMT_YUV420SP), mppCtx_(nullptr), mpi_(nullptr), mppApi_(mppApi),
      memGroup_(nullptr), packet_(nullptr), frame_(nullptr)
{}

CodecJpegDecoder::~CodecJpegDecoder()
{
    CODEC_LOGI("enter");
    width_ = 0;
    height_ = 0;
    Destory();
    format_ = MPP_FMT_YUV420SP;
}

void CodecJpegDecoder::Destory()
{
    ResetMppBuffer();
    if (memGroup_) {
        mppApi_->HdiMppBufferGroupPut(memGroup_);
        memGroup_ = nullptr;
    }
    if (mppCtx_) {
        mppApi_->HdiMppDestroy(mppCtx_);
        mppCtx_ = nullptr;
    }
    mpi_ = nullptr;
    mppApi_ = nullptr;
}

int32_t CodecJpegDecoder::DeCode(BufferHandle *buffer, BufferHandle *outBuffer, const struct CodecJpegDecInfo &decInfo)
{
    CODEC_LOGI("enter");

    if (!PrePare()) {
        CODEC_LOGE("PrePare failed");
        return HDF_FAILURE;
    }
    if (!SetFormat(outBuffer->format)) {
        CODEC_LOGE("format %{public}d set error", outBuffer->format);
        return HDF_ERR_INVALID_PARAM;
    }

    width_ = decInfo.imageWidth;
    height_ = decInfo.imageHeight;
    if (SendData(decInfo, buffer, outBuffer) != MPP_OK) {
        CODEC_LOGE("Send data error");
        return HDF_FAILURE;
    }

    if (GetFrame() != MPP_OK) {
        CODEC_LOGE("Recv frame error");
        return HDF_FAILURE;
    }
    DumpOutFile();
    CODEC_LOGI("jpeg decode end.");
    return HDF_SUCCESS;
}

MPP_RET CodecJpegDecoder::SendData(const struct CodecJpegDecInfo &decInfo, BufferHandle* buffer, BufferHandle *outHandle)
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
    MppBufferInfo info;
    memset(&info, 0, sizeof(MppBufferInfo));
    info.type = MPP_BUFFER_TYPE_DRM;
    info.fd =  buffer->fd;
    info.size = buffer->size;
    auto ret = mppApi_->HdiMppBufferImportWithTag(nullptr, &info, &pktBuf, MODULE_TAG, __func__);
    if (ret != MPP_OK) {
        CODEC_LOGE("import input packet error %{public}d", ret);
        return ret;
    }
    mppApi_->HdiMppPacketInitWithBuffer(&packet_, pktBuf); // input
   
    DumpInFile(pktBuf);
    // init frame_
    mppApi_->HdiMppFrameInit(&frame_);
#ifndef USE_RGA
    MppBufferInfo outputCommit;
    memset_s(&outputCommit, sizeof(outputCommit), 0, sizeof(outputCommit));
    outputCommit.type = MPP_BUFFER_TYPE_DRM;
    outputCommit.size = outHandle->size;
    outputCommit.fd = outHandle->fd;
    ret = mppApi_->HdiMppBufferImportWithTag(nullptr, &outputCommit, &frmBuf, MODULE_TAG, __func__);
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
    if (frameOut != frame_) {
        CODEC_LOGE("frameOut is not match with frame_ %{public}d", ret);
        mppApi_->HdiMppFrameDeinit(&frameOut);
        return MPP_NOK;
    }
    auto err = mppApi_->HdiMppFrameGetErrinfo(frameOut) | mppApi_->HdiMppFrameGetDiscard(frameOut);
    if (err) {
        CODEC_LOGE("err = %{public}d", err);
        return MPP_NOK;
    }

    /* output queue */
    ret = mpi_->enqueue(mppCtx_, MPP_PORT_OUTPUT, task);
    if (ret != MPP_OK) {
        CODEC_LOGE("enqueue output error %{public}d", ret);
        return ret;
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
    out.write(reinterpret_cast<char *>(ptr), size);
    out.flush();
#endif
}
void CodecJpegDecoder::DumpInFile(MppBuffer pktBuf)
{
#ifdef DUMP_FILE
    auto size = mppApi_->HdiMppBufferGetSizeWithCaller(pktBuf, __func__);
    auto data = mppApi_->HdiMppBufferGetPtrWithCaller(pktBuf, __func__);
    CODEC_LOGD("size %{public}d", size);
    if (data == nullptr || size == 0) {
        CODEC_LOGE("have no data in pktbuf");
        return;
    }
    
    std::ofstream out("/data/in.raw", std::ios::trunc | std::ios::binary);
    if (!out.is_open()) {
        CODEC_LOGE("file open error");
        return;
    }
    out.write(reinterpret_cast<char*>(data), size);
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