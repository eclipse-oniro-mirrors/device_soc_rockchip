/*
 *
 * Copyright 2013 Rockchip Electronics Co., LTD.
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

/*
 * @file        Rkon2_OMX_Vdec.c
 * @brief
 * @author      csy (csy@rock-chips.com)
 * @version     2.0.0
 * @history
 *   2013.11.28 : Create
 */
#undef  ROCKCHIP_LOG_TAG
#define ROCKCHIP_LOG_TAG    "omx_vdec"
#include <securec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "Rockchip_OMX_Macros.h"
#include "Rockchip_OSAL_Event.h"
#include "Rkvpu_OMX_VdecControl.h"
#include "Rockchip_OMX_Basecomponent.h"
#include "Rockchip_OSAL_Thread.h"
#include "Rockchip_OSAL_Semaphore.h"
#include "Rockchip_OSAL_Mutex.h"
#include "Rockchip_OSAL_ETC.h"
#include "Rockchip_OSAL_OHOS.h"
#include "Rockchip_OSAL_RGA_Process.h"
#include "Rockchip_OSAL_SharedMemory.h"
#include "Rockchip_OSAL_Log.h"
#include "Rockchip_OSAL_Env.h"
#include "Rockchip_OSAL_ColorUtils.h"
#include "Rkvpu_OMX_Vdec.h"

#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "vpu_mem_pool.h"
#include "omx_video_global.h"
#include "vpu_api_private_cmd.h"
#include "vpu_api.h"
#include "drm_mode.h"
#include "VideoExt.h"

#define int64_t long long int

static int64_t lastPts = 0;

typedef struct {
    OMX_RK_VIDEO_CODINGTYPE codec_id;
    OMX_VIDEO_CODINGTYPE    omx_id;
} CodeMap;

static const CodeMap kCodeMap[] = {
    {OMX_RK_VIDEO_CodingMPEG2, OMX_VIDEO_CodingMPEG2},
    {OMX_RK_VIDEO_CodingH263, OMX_VIDEO_CodingH263},
    {OMX_RK_VIDEO_CodingMPEG4, OMX_VIDEO_CodingMPEG4},
    {OMX_RK_VIDEO_CodingVC1, (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVC1},
    {OMX_RK_VIDEO_CodingAVC, OMX_VIDEO_CodingAVC},
    {OMX_RK_VIDEO_CodingMJPEG, OMX_VIDEO_CodingMJPEG},
    {OMX_RK_VIDEO_CodingFLV1, (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingFLV1},
    {OMX_RK_VIDEO_CodingVP8, (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP8EXT},
    {OMX_RK_VIDEO_CodingWMV, OMX_VIDEO_CodingWMV},
    {OMX_RK_VIDEO_CodingHEVC, (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC},
    {OMX_RK_VIDEO_CodingVP9, (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingVP9},
    /*
     * remove copyright coding type
     *
     * { OMX_RK_VIDEO_CodingRV,    OMX_VIDEO_CodingRV},
     * { OMX_RK_VIDEO_CodingVP6,   OMX_VIDEO_CodingVP6},
     * { OMX_RK_VIDEO_CodingDIVX3, OMX_VIDEO_CodingDIVX3 },
     */
};

int calc_plane(int width, int height)
{
    int mbX, mbY;

    mbX = (width + 15) / 16; // 15:byte alignment, 16:byte alignment
    mbY = (height + 15) / 16; // 15:byte alignment, 16:byte alignment

    /* Alignment for interlaced processing */
    mbY = (mbY + 1) / 2 * 2; // 2:byte alignment

    return (mbX * 16) * (mbY * 16); // 16:byte alignment
}

static void controlFPS(OMX_BOOL isInput)
{
    static int inFrameCount = 0;
    static int inLastFrameCount = 0;
    static long inLastFpsTimeUs = 0;
    static float inFps = 0;
    static long inDiff = 0;
    static long inNowUs = 0;
    static int outFrameCount = 0;
    static int outLastFrameCount = 0;
    static long outLastFpsTimeUs = 0;
    static float outFps = 0;
    static long outDiff = 0;
    static long outNowUs = 0;

    if (isInput == OMX_TRUE) {
        inFrameCount++;
        if (!(inFrameCount & 0x1F)) {
            struct timeval now;
            gettimeofday(&now, NULL);
            inNowUs = (long)now.tv_sec * 1000000 + (long)now.tv_usec; // 1000000:time unit conversion
            inDiff = inNowUs - inLastFpsTimeUs;
            inFps = ((float)(inFrameCount - inLastFrameCount) * 1.0f) * 1000.0f * 1000.0f / (float)inDiff; /* 1000:
                                                                                            time unit conversion */
            inLastFpsTimeUs = inNowUs;
            inLastFrameCount = inFrameCount;
            omx_info("decode input frameCount = %d frameRate = %f HZ", inFrameCount, inFps);
        }
    } else {
        outFrameCount++;
        if (!(outFrameCount & 0x1F)) {
            struct timeval now;
            gettimeofday(&now, NULL);
            outNowUs = (long)now.tv_sec * 1000000 + (long)now.tv_usec; // 1000000:time unit conversion
            outDiff = outNowUs - outLastFpsTimeUs;
            outFps = ((float)(outFrameCount - outLastFrameCount) * 1.0f) * 1000.0f * 1000.0f / (float)outDiff; /* 1000:
                                                                                            time unit conversion */
            outLastFpsTimeUs = outNowUs;
            outLastFrameCount = outFrameCount;
            omx_info("decode output frameCount = %d frameRate = %f HZ", outFrameCount, outFps);
        }
    }
    return;
}


void UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent)
{
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    ROCKCHIP_OMX_BASEPORT      *rockchipInputPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    ROCKCHIP_OMX_BASEPORT      *rockchipOutputPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];

    if ((rockchipOutputPort->portDefinition.format.video.nFrameWidth !=
         rockchipInputPort->portDefinition.format.video.nFrameWidth) ||
        (rockchipOutputPort->portDefinition.format.video.nFrameHeight !=
         rockchipInputPort->portDefinition.format.video.nFrameHeight) ||
        (rockchipOutputPort->portDefinition.format.video.nStride !=
         rockchipInputPort->portDefinition.format.video.nStride) ||
        (rockchipOutputPort->portDefinition.format.video.nSliceHeight !=
         rockchipInputPort->portDefinition.format.video.nSliceHeight)) {
        OMX_U32 width = 0, height = 0;

        rockchipOutputPort->portDefinition.format.video.nFrameWidth =
            rockchipInputPort->portDefinition.format.video.nFrameWidth;
        rockchipOutputPort->portDefinition.format.video.nFrameHeight =
            rockchipInputPort->portDefinition.format.video.nFrameHeight;
        width = rockchipOutputPort->portDefinition.format.video.nStride =
                    rockchipInputPort->portDefinition.format.video.nStride;
        height = rockchipOutputPort->portDefinition.format.video.nSliceHeight =
                     rockchipInputPort->portDefinition.format.video.nSliceHeight;

        switch (rockchipOutputPort->portDefinition.format.video.eColorFormat) {
            case OMX_COLOR_FormatYUV420Planar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
                if (width && height)
                    rockchipOutputPort->portDefinition.nBufferSize =
                        (width * height * 3) / 2; // 3byte alignment, 2:byte alignment
                break;
            default:
                if (width && height)
                    rockchipOutputPort->portDefinition.nBufferSize = width * height * 2; // 2:byte alignment
                break;
        }
    }

    return;
}

OMX_BOOL Rkvpu_Check_BufferProcess_State(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent, OMX_U32 nPortIndex)
{
    OMX_BOOL ret = OMX_FALSE;

    if ((pRockchipComponent->currentState == OMX_StateExecuting) &&
        (pRockchipComponent->pRockchipPort[nPortIndex].portState == OMX_StateIdle) &&
        (pRockchipComponent->transientState != ROCKCHIP_OMX_TransStateExecutingToIdle) &&
        (pRockchipComponent->transientState != ROCKCHIP_OMX_TransStateIdleToExecuting)) {
        ret = OMX_TRUE;
    } else {
        ret = OMX_FALSE;
    }

    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_CheckIsNeedFastmode(
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    RKVPU_OMX_VIDEODEC_COMPONENT  *pVideoDec    = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT         *pInputPort       = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    VpuCodecContext_t             *p_vpu_ctx         = pVideoDec->vpu_ctx;
    if (pVideoDec->bFastMode == OMX_FALSE
        && pVideoDec->codecId == CODEC_OMX_VIDEO_CodingHEVC
        && pInputPort->portDefinition.format.video.nFrameWidth > 1920 // 1920:Resolution size
        && pInputPort->portDefinition.format.video.nFrameHeight > 1080) { // 1080:Resolution size
        pVideoDec->bFastMode = OMX_TRUE;
        int fast_mode = 1;
        p_vpu_ctx->control(p_vpu_ctx, VPU_API_USE_FAST_MODE, &fast_mode);
        omx_info("used fast mode, h265decoder, width = %ld, height = %ld",
                 pInputPort->portDefinition.format.video.nFrameWidth,
                 pInputPort->portDefinition.format.video.nFrameHeight);
    }
    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_DebugSwitchfromPropget(
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    RKVPU_OMX_VIDEODEC_COMPONENT  *pVideoDec    = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    char pValue[128 + 1]; // 128:array length

    if (!Rockchip_OSAL_GetEnvU32("vendor.omx.vdec.debug", &omx_vdec_debug, 0)
        && omx_vdec_debug > 0) {
        omx_info("open video decoder debug, value: 0x%lx", omx_vdec_debug);
    }

    if (omx_vdec_debug & VDEC_DBG_RECORD_IN) {
        char file_name_in[128]; // 128:array length
        int pid_in = getpid();
        int tid_in = syscall(224); // 224:syscall incoming parameters
        Rockchip_OSAL_Memset(file_name_in, 0, 128); // 128:array length
        sprintf_s(file_name_in, sizeof(file_name_in), "/data/video/dec_in_%d_%d.bin", pid_in, tid_in);
        omx_info("Start recording stream to %s", file_name_in);
        if (pVideoDec->fp_in != NULL) {
            fclose(pVideoDec->fp_in);
        }
        pVideoDec->fp_in = fopen(file_name_in, "wb");
        if (pVideoDec->fp_in == NULL) {
            omx_err("record in file fopen failed, err: %s", strerror(errno));
        }
    }

    if (omx_vdec_debug & VDEC_DBG_RECORD_OUT) {
        char file_name_out[128]; // 128:array length
        int pid_out = getpid();
        int tid_out = syscall(224); // 128:syscall incoming parameters
        Rockchip_OSAL_Memset(file_name_out, 0, 128);
        sprintf_s(file_name_out, sizeof(file_name_out), "/data/video/dec_out_%d_%d.bin", pid_out, tid_out);
        omx_info("Start recording stream to %s", file_name_out);
        if (pVideoDec->fp_out != NULL) {
            fclose(pVideoDec->fp_out);
        }
        pVideoDec->fp_out = fopen(file_name_out, "wb");
        if (pVideoDec->fp_out == NULL) {
            omx_err("record in file fopen failed, err: %s", strerror(errno));
        }
    }
    if (omx_vdec_debug & VIDEO_DBG_LOG_FPS) {
        omx_info("Start print framerate when frameCount = 32");
        pVideoDec->bPrintFps = OMX_TRUE;
    }

    if (omx_vdec_debug & VIDEO_DBG_LOG_BUFFER_POSITION) {
        omx_info("print all buffer status");
        pVideoDec->bPrintBufferPosition = OMX_TRUE;
    }
    Rockchip_OSAL_Memset(pValue,  0, 128 + 1); // 128:array length
    if (!Rockchip_OSAL_GetEnvStr("cts_gts.media.gts", pValue, sizeof(pValue), NULL) && !strcasecmp(pValue, "true")) {
        omx_info("This is gts media test. pValue: %s", pValue);
        pVideoDec->bGtsMediaTest = OMX_TRUE;
    }
    return ret;
}

OMX_ERRORTYPE Rkvpu_ResetAllPortConfig(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT    *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    ROCKCHIP_OMX_BASEPORT           *pRockchipInputPort  = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    ROCKCHIP_OMX_BASEPORT           *pRockchipOutputPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];

    /* Input port */
    pRockchipInputPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pRockchipInputPort->portDefinition.format.video.nFrameHeight = DEFAULT_FRAME_HEIGHT;
    pRockchipInputPort->portDefinition.format.video.nStride = 0; /* DEFAULT_FRAME_WIDTH; */
    pRockchipInputPort->portDefinition.format.video.nSliceHeight = 0;
    pRockchipInputPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pRockchipInputPort->portDefinition.format.video.pNativeRender = 0;
    pRockchipInputPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pRockchipInputPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pRockchipInputPort->portDefinition.bEnabled = OMX_TRUE;
    pRockchipInputPort->bufferProcessType = BUFFER_COPY;
    pRockchipInputPort->portWayType = WAY2_PORT;

    /* Output port */
    pRockchipOutputPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pRockchipOutputPort->portDefinition.format.video.nFrameHeight = DEFAULT_FRAME_HEIGHT;
    pRockchipOutputPort->portDefinition.format.video.nStride = 0; /* DEFAULT_FRAME_WIDTH; */
    pRockchipOutputPort->portDefinition.format.video.nSliceHeight = 0;
    pRockchipOutputPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pRockchipOutputPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    if (pRockchipOutputPort->portDefinition.format.video.cMIMEType != NULL) {
        Rockchip_OSAL_Memset(pRockchipOutputPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipOutputPort->portDefinition.format.video.cMIMEType, "raw/video");
    }
    pRockchipOutputPort->portDefinition.format.video.pNativeRender = 0;
    pRockchipOutputPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pRockchipOutputPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pRockchipOutputPort->portDefinition.nBufferCountActual = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pRockchipOutputPort->portDefinition.nBufferCountMin = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pRockchipOutputPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pRockchipOutputPort->portDefinition.bEnabled = OMX_TRUE;
    pRockchipOutputPort->bufferProcessType = BUFFER_COPY | BUFFER_ANBSHARE;
    pRockchipOutputPort->portWayType = WAY2_PORT;

    return ret;
}

void Rkvpu_Wait_ProcessPause(ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent, OMX_U32 nPortIndex)
{
    ROCKCHIP_OMX_BASEPORT *rockchipOMXPort = NULL;

    FunctionIn();
    if (!pRockchipComponent || !pRockchipComponent->pRockchipPort) {
        omx_err_f("component or port is null.");
        FunctionOut();
        return;
    }
    rockchipOMXPort = &pRockchipComponent->pRockchipPort[nPortIndex];

    if (((pRockchipComponent->currentState == OMX_StatePause) ||
         (pRockchipComponent->currentState == OMX_StateIdle) ||
         (pRockchipComponent->transientState == ROCKCHIP_OMX_TransStateLoadedToIdle) ||
         (pRockchipComponent->transientState == ROCKCHIP_OMX_TransStateExecutingToIdle)) &&
        (pRockchipComponent->transientState != ROCKCHIP_OMX_TransStateIdleToLoaded) &&
        (!CHECK_PORT_BEING_FLUSHED(rockchipOMXPort))) {
        Rockchip_OSAL_SignalWait(pRockchipComponent->pRockchipPort[nPortIndex].pauseEvent, DEF_MAX_WAIT_TIME);
        Rockchip_OSAL_SignalReset(pRockchipComponent->pRockchipPort[nPortIndex].pauseEvent);
    }

    FunctionOut();

    return;
}

OMX_BOOL Rkvpu_SendInputData(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL               ret = OMX_FALSE;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT      *rockchipInputPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    ROCKCHIP_OMX_BASEPORT      *rockchipOutputPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    ROCKCHIP_OMX_DATABUFFER    *inputUseBuffer = &rockchipInputPort->way.port2WayDataBuffer.inputDataBuffer;
    VpuCodecContext_t *p_vpu_ctx = pVideoDec->vpu_ctx;
    OMX_S32 i = 0;
    OMX_S32 numInOmxAl = 0;
    OMX_S32 maxBufferNum = rockchipInputPort->portDefinition.nBufferCountActual;
    OMX_S32 dec_ret = 0;
    FunctionIn();

    for (i = 0; i < maxBufferNum; i++) {
        if (rockchipInputPort->extendBufferHeader[i].bBufferInOMX == OMX_FALSE) {
            numInOmxAl++;
        }
    }

    if (pVideoDec->bPrintBufferPosition) {
        omx_err("in buffer position: in app and display num = %ld", numInOmxAl);
        omx_err("in buffer position: in omx and vpu num = %ld", maxBufferNum - numInOmxAl);
    }

    if (inputUseBuffer->dataValid == OMX_TRUE) {
        VideoPacket_t pkt;
        if (pVideoDec->bFirstFrame == OMX_TRUE) {
            OMX_U8 *extraData = NULL;
            OMX_U32 extraSize = 0;
            OMX_U32 extraFlag = 0;
            OMX_U32 enableDinterlace = 1;
            OMX_BOOL fbcMode = OMX_FALSE;
            OMX_U32 fbcOutFmt = 0;
            if (((inputUseBuffer->nFlags & OMX_BUFFERFLAG_EXTRADATA) == OMX_BUFFERFLAG_EXTRADATA)
                || ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG) == OMX_BUFFERFLAG_CODECCONFIG)) {
                if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
                    omx_trace("inputUseBuffer->bufferHeader->pBuffer = %p", inputUseBuffer->bufferHeader->pBuffer);
                    extraData = inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen;
                    if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
#ifdef AVS80
                        OMX_PTR trueAddress = Rockchip_OSAL_SharedMemory_HandleToSecureAddress(
                            pVideoDec->hSharedMemory, (OMX_HANDLETYPE)extraData, DEFAULT_VIDEO_INPUT_BUFFER_SIZE);
                        extraData = trueAddress;
#endif
                    }
                } else {
                    omx_trace("Rkvpu_SendInputData malloc");
                    extraData = (OMX_U8 *)Rockchip_OSAL_Malloc(inputUseBuffer->dataLen);
                    if (extraData == NULL) {
                        omx_err("malloc Extra Data fail");
                        ret = OMX_FALSE;
                        goto EXIT;
                    }

                    Rockchip_OSAL_Memcpy(extraData, inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen,
                                         inputUseBuffer->dataLen);
                }

                extraSize = inputUseBuffer->dataLen;
                extraFlag = 1;
            }

            omx_trace("decode init");
            // add by xhr
            int32_t depth = (pVideoDec->bIs10bit) ? OMX_DEPTH_BIT_10 : OMX_DEPTH_BIT_8;
            fbcMode = Rockchip_OSAL_Check_Use_FBCMode(pVideoDec->codecId, depth, rockchipOutputPort);
            if (fbcMode) {
                /* fbc_Output_format: FBC_AFBC_V2 */
                fbcOutFmt = 0x00200000;
            }

            p_vpu_ctx->private_data = &fbcOutFmt;
            p_vpu_ctx->init(p_vpu_ctx, extraData, extraSize);
            p_vpu_ctx->private_data = NULL;
            if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
                Rockchip_OSAL_SharedMemory_SecureUnmap(pVideoDec->hSharedMemory,
                    extraData, DEFAULT_VIDEO_INPUT_BUFFER_SIZE);
            }

            if (fbcMode) {
                omx_info("use vpu fbc output mode");

                p_vpu_ctx->control(p_vpu_ctx, VPU_API_SET_OUTPUT_MODE, &fbcOutFmt);
            }

            // not use iep when thumbNail decode
            if (!(pVideoDec->flags & RKVPU_OMX_VDEC_THUMBNAIL)) {
                p_vpu_ctx->control(p_vpu_ctx, VPU_API_ENABLE_DEINTERLACE, &enableDinterlace);
            }
            if (pVideoDec->vpumem_handle != NULL) {
                p_vpu_ctx->control(p_vpu_ctx, VPU_API_SET_VPUMEM_CONTEXT, pVideoDec->vpumem_handle);
            }

            if (rockchipInputPort->portDefinition.format.video.bFlagErrorConcealment) {
                omx_trace("use directly output mode for media");
                RK_U32 flag = 1;
                p_vpu_ctx->control(p_vpu_ctx, VPU_API_SET_IMMEDIATE_OUT, (void*)&flag);
            }

            if (p_vpu_ctx->videoCoding == OMX_RK_VIDEO_CodingHEVC && pVideoDec->bOld_api == OMX_TRUE) {
                p_vpu_ctx->control(p_vpu_ctx, (VPU_API_CMD)VPU_API_PRIVATE_HEVC_NEED_PARSE, NULL);
            }

            pVideoDec->bFirstFrame = OMX_FALSE;
            if (extraFlag) {
                ret = OMX_TRUE;
                if (extraData  && !pVideoDec->bDRMPlayerMode) {
                    Rockchip_OSAL_Free(extraData);
                    extraData = NULL;
                    Rkvpu_InputBufferReturn(pOMXComponent, inputUseBuffer);
                } else if (extraData && pVideoDec->bDRMPlayerMode) {
                    Rkvpu_InputBufferReturn(pOMXComponent, inputUseBuffer);
                } else {
                    ;
                }

                goto EXIT;
            }
        }

        if ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            omx_trace("bSaveFlagEOS : OMX_TRUE");
            pRockchipComponent->bSaveFlagEOS = OMX_TRUE;
        }
        Rockchip_OSAL_Memset(&pkt, 0, sizeof(VideoPacket_t));
        pkt.data =  inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen;
        omx_trace("in sendInputData data = %p", pkt.data);
        if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
#ifdef AVS80
            OMX_PTR trueAddress = Rockchip_OSAL_SharedMemory_HandleToSecureAddress(pVideoDec->hSharedMemory,
                (OMX_HANDLETYPE)pkt.data, DEFAULT_VIDEO_INPUT_BUFFER_SIZE);
            pkt.data = trueAddress;
#endif
            omx_trace("out sendInputData data = %p", pkt.data);
        }
        pkt.size = inputUseBuffer->dataLen;

        if (pVideoDec->flags & RKVPU_OMX_VDEC_USE_DTS) {
            pkt.pts = VPU_API_NOPTS_VALUE;
            pkt.dts = inputUseBuffer->timeStamp;
        } else {
            pkt.pts = pkt.dts = inputUseBuffer->timeStamp;
        }
        if ((inputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            omx_info("send eos");
            pkt.nFlags |= OMX_BUFFERFLAG_EOS;
        }
        omx_trace("pkt.size:%d, pkt.dts:%lld,pkt.pts:%lld,pkt.nFlags:%d",
                  pkt.size, pkt.dts, pkt.pts, pkt.nFlags);
        omx_trace("decode_sendstream pkt.data = %p", pkt.data);
        dec_ret = p_vpu_ctx->decode_sendstream(p_vpu_ctx, &pkt);
        if (dec_ret < 0) {
            omx_err("decode_sendstream failed , ret = %lx", dec_ret);
        }
        if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
            Rockchip_OSAL_SharedMemory_SecureUnmap(pVideoDec->hSharedMemory,
                pkt.data, DEFAULT_VIDEO_INPUT_BUFFER_SIZE);
        }
        if (pkt.size != 0) {
            goto EXIT;
        }

        if (pVideoDec->bPrintFps == OMX_TRUE) {
            OMX_BOOL isInput = OMX_TRUE;
            controlFPS(isInput);
        }

        if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
            Rkvpu_InputBufferReturn(pOMXComponent, inputUseBuffer);
        } else {
            Rkvpu_InputBufferReturn(pOMXComponent, inputUseBuffer);
        }
        if (pRockchipComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) {
            pRockchipComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_TRUE;
            pRockchipComponent->checkTimeStamp.startTimeStamp = inputUseBuffer->timeStamp;
            pRockchipComponent->checkTimeStamp.nStartFlags = inputUseBuffer->nFlags;
            pRockchipComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
            omx_trace("first frame timestamp after seeking %lld us (%.2f secs)",
                      inputUseBuffer->timeStamp, inputUseBuffer->timeStamp / 1E6);
        }
        ret = OMX_TRUE;
    }

EXIT:
    FunctionOut();
    return ret;
}

OMX_BOOL Rkvpu_Post_OutputFrame(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_BOOL                   ret = OMX_FALSE;
    ROCKCHIP_OMX_BASECOMPONENT  *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT           *pInputPort         = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    ROCKCHIP_OMX_BASEPORT           *pOutputPort        = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    ROCKCHIP_OMX_DATABUFFER     *outputUseBuffer = &pOutputPort->way.port2WayDataBuffer.outputDataBuffer;
    VpuCodecContext_t           *p_vpu_ctx = pVideoDec->vpu_ctx;
    OMX_U32         pOWnBycomponetNum = Rockchip_OSAL_GetElemNum(&pOutputPort->bufferQ);
    OMX_S32 maxBufferNum = 0;
    OMX_S32 i = 0, numInOmxAl = 0, limitNum = 8;
    OMX_S32 bufferUnusedInVpu = 0;
    OMX_U32 fullRange = 0;
    OMX_U32 primaries = 0;
    OMX_U32 transfer  = 0;
    OMX_U32 coeffs    = 0;
    OMX_COLORASPECTS Aspects = { 0, 0, 0, 0 };
    OMX_COLORASPECTS *colorAspects = NULL;

    FunctionIn();
    if (p_vpu_ctx == NULL ||
        (pVideoDec->bFirstFrame == OMX_TRUE) ||
        (pVideoDec->bDecSendEOS == OMX_TRUE)) {
        goto EXIT;
    }

    maxBufferNum = pOutputPort->portDefinition.nBufferCountActual;
    for (i = 0; i < maxBufferNum; i++) {
        if (pOutputPort->extendBufferHeader[i].bBufferInOMX == OMX_FALSE) {
            numInOmxAl++;
        }
    }
    if (pVideoDec->bPrintBufferPosition) {
        struct vpu_display_mem_pool *pMem_pool = (struct vpu_display_mem_pool*)pVideoDec->vpumem_handle;
        omx_info("out buffer position: in app and display num = %ld", numInOmxAl);
        omx_info("out buffer position: in omx and vpu num = %ld", maxBufferNum - numInOmxAl);
        omx_info("out buffer position: in component num = %ld", pOWnBycomponetNum);
        if (pMem_pool) {
            bufferUnusedInVpu = pMem_pool->get_unused_num(pMem_pool);
            omx_info("out buffer position: in vpu unused buffer = %ld", bufferUnusedInVpu);
        }
    }
    if (pOutputPort->bufferProcessType == BUFFER_SHARE) {
        OMX_S32 dec_ret = 0;
        DecoderOut_t pOutput;
        VPU_FRAME *pframe = (VPU_FRAME *)Rockchip_OSAL_Malloc(sizeof(VPU_FRAME));
        OMX_BUFFERHEADERTYPE     *bufferHeader = NULL;
        Rockchip_OSAL_Memset(&pOutput, 0, sizeof(DecoderOut_t));
        Rockchip_OSAL_Memset(pframe, 0, sizeof(VPU_FRAME));
        pOutput.data = (unsigned char *)pframe;
        if ((numInOmxAl < limitNum) ||
            (pVideoDec->maxCount > 20)) { // 20:framecount
            dec_ret =  p_vpu_ctx->decode_getframe(p_vpu_ctx, &pOutput);
            omx_trace("pOutput.size %d", pOutput.size);
            pVideoDec->maxCount = 0;
        } else {
            pVideoDec->maxCount++;
            omx_trace("pVideoDec %p numInOmxAl %ld", pVideoDec, numInOmxAl);
        }
        if (dec_ret < 0) {
            if (dec_ret == VPU_API_EOS_STREAM_REACHED && !pframe->ErrorInfo) {
                outputUseBuffer->dataLen = 0;
                outputUseBuffer->remainDataLen = 0;
                outputUseBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                outputUseBuffer->timeStamp = 0;
                outputUseBuffer->dataValid = OMX_FALSE;
                ret = OMX_TRUE;
                pVideoDec->bDecSendEOS = OMX_TRUE;
                omx_info("OMX_BUFFERFLAG_EOS");
            } else {
                omx_err("OMX_DECODER ERROR");
                pRockchipComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                             pRockchipComponent->callbackData, OMX_EventError,
                                                             OUTPUT_PORT_INDEX,
                                                             OMX_IndexParamPortDefinition, NULL);
            }
            Rkvpu_OutputBufferReturn(pOMXComponent, outputUseBuffer);
        }
        /*
         * when decode frame (width > 8192 || Height > 4096), mpp not to check it
         * do not check here, ACodec will alloc large 4K size memory
         * cause lower memory fault
        */
        if (pframe->DisplayWidth > 8192 ||  pframe->DisplayHeight > 4096) { // 8192:width, 4096:height
            pRockchipComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                         pRockchipComponent->callbackData,
                                                         OMX_EventError, OMX_ErrorUndefined, 0, NULL);
            if (pframe->vpumem.phy_addr > 0) {
                VPUMemLink(&pframe->vpumem);
                VPUFreeLinear(&pframe->vpumem);
            }
            ret = OMX_FALSE;
            goto EXIT;
        }
        if ((pOutput.size > 0) && (!CHECK_PORT_BEING_FLUSHED(pOutputPort))) {
            {
                if (pVideoDec->codecId != OMX_VIDEO_CodingVP8EXT) {
                    fullRange  = pframe->ColorRange;
                    primaries  = pframe->ColorPrimaries;
                    transfer   = pframe->ColorTransfer;
                    coeffs     = pframe->ColorCoeffs;

                    colorAspects = &Aspects;
                    convertIsoColorAspectsToCodecAspects(primaries, transfer, coeffs, fullRange, colorAspects);

                    if (colorAspectsDiffer(colorAspects, &pVideoDec->mBitstreamColorAspects)) {
                        pVideoDec->mBitstreamColorAspects.mRange = Aspects.mRange;
                        pVideoDec->mBitstreamColorAspects.mPrimaries = Aspects.mPrimaries;
                        pVideoDec->mBitstreamColorAspects.mTransfer = Aspects.mTransfer;
                        pVideoDec->mBitstreamColorAspects.mMatrixCoeffs = Aspects.mMatrixCoeffs;
                        handleColorAspectsChange(&pVideoDec->mDefaultColorAspects /* mDefaultColorAspects */,
                            &pVideoDec->mBitstreamColorAspects /* mBitstreamColorAspects */,
                            &pVideoDec->mFinalColorAspects /* mFinalColorAspects */,
                            kPreferBitstream);
                    }
                }
            }
            OMX_COLOR_FORMATTYPE eColorFormat = Rockchip_OSAL_CheckFormat(pRockchipComponent, pframe);
            if ((pInputPort->portDefinition.format.video.nFrameWidth != pframe->DisplayWidth)
                || (pInputPort->portDefinition.format.video.nFrameHeight != pframe->DisplayHeight)
                || (pInputPort->portDefinition.format.video.nSliceHeight != pframe->FrameHeight)
                || (pInputPort->portDefinition.format.video.nStride != (OMX_S32)pframe->FrameWidth)
                || (pOutputPort->portDefinition.format.video.eColorFormat != eColorFormat)) {
                omx_info("info-change with frame[%d,%d] -> [%d,%d]  eColorFormat: 0x%x",
                    (int)pInputPort->portDefinition.format.video.nStride,
                    (int)pInputPort->portDefinition.format.video.nSliceHeight,
                    pframe->FrameWidth, pframe->FrameHeight, eColorFormat);

                pOutputPort->newCropRectangle.nWidth = pframe->DisplayWidth;
                pOutputPort->newCropRectangle.nHeight = pframe->DisplayHeight;
                pOutputPort->newPortDefinition.format.video.eColorFormat = eColorFormat;
                pOutputPort->newPortDefinition.nBufferCountActual = pOutputPort->portDefinition.nBufferCountActual;
                pOutputPort->newPortDefinition.nBufferCountMin = pOutputPort->portDefinition.nBufferCountMin;
                pInputPort->newPortDefinition.format.video.nFrameWidth = pframe->DisplayWidth;
                pInputPort->newPortDefinition.format.video.nFrameHeight = pframe->DisplayHeight;

                pInputPort->newPortDefinition.format.video.nStride         = pframe->FrameWidth;
                pInputPort->newPortDefinition.format.video.nSliceHeight    = pframe->FrameHeight;

                Rkvpu_ResolutionUpdate(pOMXComponent);
                pRockchipComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                    pRockchipComponent->callbackData, OMX_EventPortSettingsChanged,
                    OUTPUT_PORT_INDEX,
                    OMX_IndexParamPortDefinition, NULL);
                /*
                 * get dpb size from vpuapi
                 */
                OMX_U32 nDpbSize = 0;
                if (p_vpu_ctx) {
                    p_vpu_ctx->control(p_vpu_ctx, VPU_API_DEC_GET_DPB_SIZE, &nDpbSize);
                    if (nDpbSize > 0) {
                        omx_info("info change, nDpbSize: %d", (int)nDpbSize);
                        pVideoDec->nDpbSize = nDpbSize;
                    }
                }

                if (pframe->vpumem.phy_addr > 0) {
                    VPUMemLink(&pframe->vpumem);
                    VPUFreeLinear(&pframe->vpumem);
                }
                Rockchip_OSAL_Free(pframe);
                Rockchip_OSAL_resetVpumemPool(pRockchipComponent);
                p_vpu_ctx->control(p_vpu_ctx, VPU_API_SET_INFO_CHANGE, NULL);
                pVideoDec->bInfoChange = OMX_TRUE;
                goto EXIT;
            }

            if (pVideoDec->bPrintFps == OMX_TRUE) {
                OMX_BOOL isInput = OMX_FALSE;
                controlFPS(isInput);
            }
            if (pframe->ErrorInfo && (pVideoDec->bGtsMediaTest == OMX_FALSE) &&
                (pVideoDec->bDRMPlayerMode == OMX_FALSE)) {   // drop frame when this frame mark error from dec
                omx_err("this frame is Error frame!,pOutput.timeUs = %lld", pOutput.timeUs);
                if (pframe->vpumem.phy_addr > 0) {
                    VPUMemLink(&pframe->vpumem);
                    VPUFreeLinear(&pframe->vpumem);
                }
                goto EXIT;
            }

            bufferHeader = Rockchip_OSAL_Fd2OmxBufferHeader(pOutputPort, VPUMemGetFD(&pframe->vpumem), pframe);
            if (pVideoDec->fp_out != NULL) {
                fwrite(pframe->vpumem.vir_addr, 1,
                    pframe->FrameHeight * pframe->FrameWidth * 3 / 2, // 3:byte alignment, 2:byte alignment
                    pVideoDec->fp_out);
                fflush(pVideoDec->fp_out);
            }
            if (bufferHeader != NULL) {
                if (pVideoDec->bStoreMetaData == OMX_TRUE) {
                    bufferHeader->nFilledLen = bufferHeader->nAllocLen;
                    omx_trace("nfill len %d", (int)bufferHeader->nFilledLen);
                } else {
                    bufferHeader->nFilledLen =
                        pframe->DisplayHeight * pframe->DisplayWidth * 3 / 2; // 3:byte alignment, 2:byte alignment
                }
                bufferHeader->nOffset    = 0;
                if ((VPU_API_ERR)pOutput.nFlags == VPU_API_EOS_STREAM_REACHED) {
                    bufferHeader->nFlags |= OMX_BUFFERFLAG_EOS;
                    pVideoDec->bDecSendEOS = OMX_TRUE;
                    omx_info("reach eos");
                } else {
                    bufferHeader->nFlags     = 0;
                }
                bufferHeader->nTimeStamp = pOutput.timeUs;
                if (pframe->FrameType > 0) {
                    bufferHeader->nFlags = OMX_BUFFERFLAG_SYNCFRAME;
                }
            } else {
                if (pframe->vpumem.phy_addr > 0) {
                    VPUMemLink(&pframe->vpumem);
                    VPUFreeLinear(&pframe->vpumem);
                }
                Rockchip_OSAL_Free(pframe);
                goto EXIT;
            }

            if ((bufferHeader->nFilledLen > 0) ||
                ((bufferHeader->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
                (CHECK_PORT_BEING_FLUSHED(pOutputPort))) {
                Rockchip_OMX_OutputBufferReturn(pOMXComponent, bufferHeader);
            }

            ret = OMX_TRUE;
        } else if (CHECK_PORT_BEING_FLUSHED(pOutputPort)) {
            if (pOutput.size && (pframe->vpumem.phy_addr > 0)) {
                VPUMemLink(&pframe->vpumem);
                VPUFreeLinear(&pframe->vpumem);
                Rockchip_OSAL_Free(pframe);
            }
            outputUseBuffer->dataLen = 0;
            outputUseBuffer->remainDataLen = 0;
            outputUseBuffer->nFlags = 0;
            outputUseBuffer->timeStamp = 0;
            ret = OMX_TRUE;
            Rkvpu_OutputBufferReturn(pOMXComponent, outputUseBuffer);
        } else {
            if (pframe != NULL) {
                Rockchip_OSAL_Free(pframe);
                pframe = NULL;
            }
            ret = OMX_FALSE;
        }
    } else {
        if (outputUseBuffer->dataValid == OMX_TRUE) {
            int ret = 0;
            DecoderOut_t pOutput;
            VPU_FRAME pframe;
            Rockchip_OSAL_Memset(&pOutput, 0, sizeof(DecoderOut_t));
            Rockchip_OSAL_Memset(&pframe, 0, sizeof(VPU_FRAME));
            pOutput.data = (unsigned char *)&pframe;
            ret =  p_vpu_ctx->decode_getframe(p_vpu_ctx, &pOutput);
            if (ret < 0) {
                if (ret == VPU_API_EOS_STREAM_REACHED && !pframe.ErrorInfo) {
                    outputUseBuffer->dataLen = 0;
                    outputUseBuffer->remainDataLen = 0;
                    outputUseBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                    outputUseBuffer->timeStamp = 0;
                    outputUseBuffer->dataValid = OMX_FALSE;
                    ret = OMX_TRUE;
                    pVideoDec->bDecSendEOS = OMX_TRUE;
                    omx_info("OMX_BUFFERFLAG_EOS");
                } else {
                    omx_err("OMX_DECODER ERROR");
                    pRockchipComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                                 pRockchipComponent->callbackData, OMX_EventError,
                                                                 OUTPUT_PORT_INDEX,
                                                                 OMX_IndexParamPortDefinition, NULL);
                }
                Rkvpu_OutputBufferReturn(pOMXComponent, outputUseBuffer);
            }
            // error in outframe
            if (pframe.ErrorInfo) {
                if (pframe.vpumem.phy_addr > 0) {
                    VPUMemLink(&pframe.vpumem);
                    VPUFreeLinear(&pframe.vpumem);
                }
                goto EXIT;
            }
            /*
             * when decode frame (width > 8192 || Height > 4096), mpp not to check it.
             * do not check here, ACodec will alloc large 4K size memory.
             * cause lower memory fault
            */
            if (pframe.DisplayWidth > 8192 ||  pframe.DisplayHeight > 4096) { // 8192:width, 4096:height
                pRockchipComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                             pRockchipComponent->callbackData,
                                                             OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                if (pframe.vpumem.phy_addr > 0) {
                    VPUMemLink(&pframe.vpumem);
                    VPUFreeLinear(&pframe.vpumem);
                }
                ret = OMX_FALSE;
                goto EXIT;
            }

            if ((pOutput.size > 0) && (!CHECK_PORT_BEING_FLUSHED(pOutputPort))) {
                {
                    if (pVideoDec->codecId != OMX_VIDEO_CodingVP8EXT) {
                        fullRange  = pframe.ColorRange;
                        primaries  = pframe.ColorPrimaries;
                        transfer   = pframe.ColorTransfer;
                        coeffs     = pframe.ColorCoeffs;

                        colorAspects = &Aspects;
                        convertIsoColorAspectsToCodecAspects(primaries, transfer, coeffs, fullRange, colorAspects);

                        if (colorAspectsDiffer(colorAspects, &pVideoDec->mBitstreamColorAspects)) {
                            pVideoDec->mBitstreamColorAspects.mRange = Aspects.mRange;
                            pVideoDec->mBitstreamColorAspects.mPrimaries = Aspects.mPrimaries;
                            pVideoDec->mBitstreamColorAspects.mTransfer = Aspects.mTransfer;
                            pVideoDec->mBitstreamColorAspects.mMatrixCoeffs = Aspects.mMatrixCoeffs;
                            handleColorAspectsChange(&pVideoDec->mDefaultColorAspects /* mDefaultColorAspects */,
                                                     &pVideoDec->mBitstreamColorAspects /* mBitstreamColorAspects */,
                                                     &pVideoDec->mFinalColorAspects /* mFinalColorAspects */,
                                                     kPreferBitstream);
                        }
                    }
                }
                OMX_COLOR_FORMATTYPE eColorFormat = Rockchip_OSAL_CheckFormat(pRockchipComponent, &pframe);
                if (pInputPort->portDefinition.format.video.nFrameWidth != pframe.DisplayWidth ||
                    pInputPort->portDefinition.format.video.nFrameHeight != pframe.DisplayHeight ||
                    pOutputPort->portDefinition.format.video.eColorFormat != eColorFormat) {
                    omx_info("info-change with frame[%d,%d] -> [%d,%d]  eColorFormat: 0x%x",
                             (int)pInputPort->portDefinition.format.video.nStride,
                             (int)pInputPort->portDefinition.format.video.nSliceHeight,
                             pframe.FrameWidth, pframe.FrameHeight, eColorFormat);

                    pOutputPort->newCropRectangle.nWidth = pframe.DisplayWidth;
                    pOutputPort->newCropRectangle.nHeight = pframe.DisplayHeight;
                    pOutputPort->newPortDefinition.format.video.eColorFormat = eColorFormat;
                    pOutputPort->newPortDefinition.nBufferCountActual = pOutputPort->portDefinition.nBufferCountActual;
                    pOutputPort->newPortDefinition.nBufferCountMin = pOutputPort->portDefinition.nBufferCountMin;
                    pInputPort->newPortDefinition.format.video.nFrameWidth = pframe.DisplayWidth;
                    pInputPort->newPortDefinition.format.video.nFrameHeight = pframe.DisplayHeight;
                    pInputPort->newPortDefinition.format.video.nStride         = pframe.DisplayWidth;
                    pInputPort->newPortDefinition.format.video.nSliceHeight    = pframe.DisplayHeight;

                    Rkvpu_ResolutionUpdate(pOMXComponent);
                    pRockchipComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                                 pRockchipComponent->callbackData,
                                                                 OMX_EventPortSettingsChanged,
                                                                 OUTPUT_PORT_INDEX,
                                                                 OMX_IndexParamPortDefinition, NULL);
                    if (pframe.vpumem.phy_addr > 0) {
                        VPUMemLink(&pframe.vpumem);
                        VPUFreeLinear(&pframe.vpumem);
                    }
                    p_vpu_ctx->control(p_vpu_ctx, VPU_API_SET_INFO_CHANGE, NULL);
                    goto EXIT;
                }

                if (!pframe.vpumem.phy_addr) { /* in mpp process may be notify a null frame for info change */
                    p_vpu_ctx->control(p_vpu_ctx, VPU_API_SET_INFO_CHANGE, NULL);
                    goto EXIT;
                }
                /* repeat frame should not return for VtsHalMediaOmxV1_0TargetVideoDecTest */
                if (pVideoDec->codecId == OMX_VIDEO_CodingMPEG2) {
                    if (lastPts != 0 && lastPts == pOutput.timeUs &&
                        VPU_API_EOS_STREAM_REACHED != (VPU_API_ERR)pOutput.nFlags) {
                        if (pframe.vpumem.phy_addr > 0) {
                            VPUMemLink(&pframe.vpumem);
                            VPUFreeLinear(&pframe.vpumem);
                        }
                        omx_err("repeat frame should not return, delete!");
                        goto EXIT;
                    }
                    if (lastPts != pOutput.timeUs)
                        lastPts = pOutput.timeUs;
                }
                Rkvpu_Frame2Outbuf(pOMXComponent, outputUseBuffer->bufferHeader, &pframe);
                if ((pVideoDec->codecProfile == OMX_VIDEO_AVCProfileHigh10 &&
                    pVideoDec->codecId == OMX_VIDEO_CodingAVC)
                    || ((pVideoDec->codecProfile == CODEC_HEVC_PROFILE_MAIN10 ||
                        pVideoDec->codecProfile == CODEC_HEVC_PROFILE_MAIN10_HDR10)
                        && pVideoDec->codecId == CODEC_OMX_VIDEO_CodingHEVC)) {
                    OMX_U32 horStride = Get_Video_HorAlign(pVideoDec->codecId, pframe.DisplayWidth,
                        pframe.DisplayHeight, pVideoDec->codecProfile);
                    OMX_U32 verStride = Get_Video_VerAlign(pVideoDec->codecId, pframe.DisplayHeight,
                        pVideoDec->codecProfile);
                    outputUseBuffer->remainDataLen = GetDataSize(horStride, verStride, eColorFormat);
                } else
                    outputUseBuffer->remainDataLen =
                        GetDataSize(pframe.DisplayWidth, pframe.DisplayHeight, eColorFormat);
                outputUseBuffer->timeStamp = pOutput.timeUs;
                omx_trace("outputUseBuffer->remainDataLen = %d", (int)outputUseBuffer->remainDataLen);
                if (pVideoDec->fp_out != NULL && (pVideoDec->bOhosBufferHandle == OMX_FALSE)) {
                    fwrite(outputUseBuffer->bufferHeader->pBuffer, 1,
                        outputUseBuffer->remainDataLen, pVideoDec->fp_out);
                    fflush(pVideoDec->fp_out);
                }
                if (VPU_API_EOS_STREAM_REACHED == (VPU_API_ERR)pOutput.nFlags) {
                    outputUseBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
                    pVideoDec->bDecSendEOS = OMX_TRUE;
                    omx_info("OMX_BUFFERFLAG_EOS");
                }
                if ((outputUseBuffer->remainDataLen > 0) ||
                    ((outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
                    (CHECK_PORT_BEING_FLUSHED(pOutputPort))) {
                    Rkvpu_OutputBufferReturn(pOMXComponent, outputUseBuffer);
                }
                ret = OMX_TRUE;
            } else if (CHECK_PORT_BEING_FLUSHED(pOutputPort)) {
                if (pOutput.size) {
                    VPUMemLink(&pframe.vpumem);
                    VPUFreeLinear(&pframe.vpumem);
                }
                outputUseBuffer->dataLen = 0;
                outputUseBuffer->remainDataLen = 0;
                outputUseBuffer->nFlags = 0;
                outputUseBuffer->timeStamp = 0;
                ret = OMX_TRUE;
                Rkvpu_OutputBufferReturn(pOMXComponent, outputUseBuffer);
            } else {
                ret = OMX_FALSE;
            }
        } else {
            ret = OMX_FALSE;
        }
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_InputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT      *rockchipInputPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    ROCKCHIP_OMX_BASEPORT      *rockchipOutputPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    ROCKCHIP_OMX_DATABUFFER    *srcInputUseBuffer = &rockchipInputPort->way.port2WayDataBuffer.inputDataBuffer;

    FunctionIn();

    while (!pVideoDec->bExitBufferProcessThread) {
        Rockchip_OSAL_SleepMillisec(0);
        Rkvpu_Wait_ProcessPause(pRockchipComponent, INPUT_PORT_INDEX);
        omx_trace("Rkvpu_Check_BufferProcess_State in");
        while ((Rkvpu_Check_BufferProcess_State(pRockchipComponent, INPUT_PORT_INDEX)) &&
               (!pVideoDec->bExitBufferProcessThread)) {
            omx_trace("Rkvpu_OMX_InputBufferProcess in");
            if ((CHECK_PORT_BEING_FLUSHED(rockchipInputPort)) ||
                (((ROCKCHIP_OMX_EXCEPTION_STATE)rockchipOutputPort->exceptionFlag != GENERAL_STATE) &&
                    ((ROCKCHIP_OMX_ERRORTYPE)ret == OMX_ErrorInputDataDecodeYet)))
                break;

            if (rockchipInputPort->portState != OMX_StateIdle)
                break;

            Rockchip_OSAL_MutexLock(srcInputUseBuffer->bufferMutex);
            if ((ROCKCHIP_OMX_ERRORTYPE)ret != OMX_ErrorInputDataDecodeYet) {
                if ((srcInputUseBuffer->dataValid != OMX_TRUE) &&
                    (!CHECK_PORT_BEING_FLUSHED(rockchipInputPort))) {
                    ret = Rkvpu_InputBufferGetQueue(pRockchipComponent);
                    if (ret != OMX_ErrorNone) {
                        Rockchip_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                        break;
                    }

                    if (pVideoDec->fp_in != NULL) {
                        fwrite(srcInputUseBuffer->bufferHeader->pBuffer + srcInputUseBuffer->usedDataLen, 1,
                            srcInputUseBuffer->dataLen, pVideoDec->fp_in);
                        fflush(pVideoDec->fp_in);
                    }
                }

                if (srcInputUseBuffer->dataValid == OMX_TRUE) {
                    if (Rkvpu_SendInputData(hComponent) != OMX_TRUE) {
                        omx_trace("stream list is full");
                        Rockchip_OSAL_SleepMillisec(5); // 5:sleep
                    }
                }
                if (CHECK_PORT_BEING_FLUSHED(rockchipInputPort)) {
                    Rockchip_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                    break;
                }
            }
            Rockchip_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
            if ((ROCKCHIP_OMX_ERRORTYPE)ret == OMX_ErrorCodecInit)
                pVideoDec->bExitBufferProcessThread = OMX_TRUE;
        }
    }

    goto EXIT;
EXIT:

    FunctionOut();

    return ret;
}


OMX_ERRORTYPE Rkvpu_OMX_OutputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    ROCKCHIP_OMX_BASEPORT      *rockchipOutputPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    ROCKCHIP_OMX_DATABUFFER    *dstOutputUseBuffer = &rockchipOutputPort->way.port2WayDataBuffer.outputDataBuffer;

    FunctionIn();

    while (!pVideoDec->bExitBufferProcessThread) {
        Rockchip_OSAL_SleepMillisec(0);
        Rkvpu_Wait_ProcessPause(pRockchipComponent, OUTPUT_PORT_INDEX);

        while ((Rkvpu_Check_BufferProcess_State(pRockchipComponent, OUTPUT_PORT_INDEX)) &&
               (!pVideoDec->bExitBufferProcessThread)) {
            if (CHECK_PORT_BEING_FLUSHED(rockchipOutputPort))
                break;

            Rockchip_OSAL_MutexLock(dstOutputUseBuffer->bufferMutex);
            if (rockchipOutputPort->bufferProcessType == BUFFER_SHARE) {
                if (Rkvpu_Post_OutputFrame(pOMXComponent) != OMX_TRUE) {
                    Rockchip_OSAL_SleepMillisec(3); // 3:sleep
                }
            } else {
                if ((dstOutputUseBuffer->dataValid != OMX_TRUE) &&
                    (!CHECK_PORT_BEING_FLUSHED(rockchipOutputPort))) {
                    ret = Rkvpu_OutputBufferGetQueue(pRockchipComponent);
                    if (ret != OMX_ErrorNone) {
                        Rockchip_OSAL_MutexUnlock(dstOutputUseBuffer->bufferMutex);
                        break;
                    }
                }

                if (dstOutputUseBuffer->dataValid == OMX_TRUE) {
                    if (Rkvpu_Post_OutputFrame(pOMXComponent) != OMX_TRUE) {
                        Rockchip_OSAL_SleepMillisec(3); // 3:sleep
                    }
                }
            }
            /* reset outputData */
            Rockchip_OSAL_MutexUnlock(dstOutputUseBuffer->bufferMutex);
        }
    }

    goto EXIT;
EXIT:

    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Rkvpu_OMX_InputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Rkvpu_OMX_InputBufferProcess(pOMXComponent);

    Rockchip_OSAL_ThreadExit(NULL);
EXIT:
    FunctionOut();
    return ret;
}

static OMX_ERRORTYPE Rkvpu_OMX_OutputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Rkvpu_OMX_OutputBufferProcess(pOMXComponent);

    Rockchip_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_BufferProcess_Create(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;

    FunctionIn();

    pVideoDec->bExitBufferProcessThread = OMX_FALSE;

    ret = Rockchip_OSAL_ThreadCreate(&pVideoDec->hOutputThread,
        Rkvpu_OMX_OutputProcessThread,
        pOMXComponent,
        "omx_dec_output");
    if (ret == OMX_ErrorNone)
        ret = Rockchip_OSAL_ThreadCreate(&pVideoDec->hInputThread,
            Rkvpu_OMX_InputProcessThread,
            pOMXComponent,
            "omx_dec_input");

    goto EXIT;
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rkvpu_OMX_BufferProcess_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    OMX_S32                countValue = 0;

    FunctionIn();

    pVideoDec->bExitBufferProcessThread = OMX_TRUE;

    Rockchip_OSAL_Get_SemaphoreCount(pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX].bufferSemID, &countValue);
    if (countValue == 0)
        Rockchip_OSAL_SemaphorePost(pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX].bufferSemID);

    Rockchip_OSAL_SignalSet(pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX].pauseEvent);
    Rockchip_OSAL_ThreadTerminate(pVideoDec->hInputThread);
    pVideoDec->hInputThread = NULL;

    Rockchip_OSAL_Get_SemaphoreCount(pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX].bufferSemID, &countValue);
    if (countValue == 0)
        Rockchip_OSAL_SemaphorePost(pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX].bufferSemID);

    Rockchip_OSAL_SignalSet(pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX].pauseEvent);

    Rockchip_OSAL_SignalSet(pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX].pauseEvent);
    Rockchip_OSAL_ThreadTerminate(pVideoDec->hOutputThread);
    pVideoDec->hOutputThread = NULL;

    pRockchipComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
    pRockchipComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;

    goto EXIT;
EXIT:
    FunctionOut();

    return ret;
}
static const char* g_libVpu[] = {
    "librockchip_vpu.z.so",
    "librk_vpuapi.so"
};
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
OMX_ERRORTYPE omx_open_vpudec_context(RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec)
{
    static void* handle = NULL;
    static OMX_BOOL oldApi = OMX_FALSE;
    static void* openCtx = NULL;
    Rockchip_OSAL_MutexLock(&g_mutex);
    if (handle == NULL) {
        for (int i = 0; i < (int)ARRAY_SIZE(g_libVpu); i++) {
            if ((handle = dlopen(g_libVpu[i], RTLD_LAZY))== NULL) {
                continue;
            }
            openCtx = dlsym(handle, "vpu_open_context");
            if (openCtx == NULL) {
                dlclose(handle);
                handle = NULL;
                continue;
            }
            if (i == 0) {
                oldApi = OMX_FALSE;
            } else {
                oldApi = OMX_TRUE;
            }
            break;
        }
    }
    Rockchip_OSAL_MutexUnlock(&g_mutex);
    if (handle == NULL) {
        return OMX_ErrorHardware;
    }
    pVideoDec->bOld_api = oldApi;
    pVideoDec->rkapi_hdl = handle;
    pVideoDec->rkvpu_open_cxt = (OMX_S32 (*)(VpuCodecContext_t **ctx))openCtx;
    pVideoDec->rkvpu_close_cxt = (OMX_S32 (*)(VpuCodecContext_t **ctx))dlsym(pVideoDec->rkapi_hdl,
        "vpu_close_context");
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Rkvpu_Dec_ComponentInit(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pRockchipComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec    =  (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    OMX_RK_VIDEO_CODINGTYPE codecId = OMX_RK_VIDEO_CodingUnused;
    ROCKCHIP_OMX_BASEPORT           *pRockchipInputPort  = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    VpuCodecContext_t *p_vpu_ctx = (VpuCodecContext_t *)Rockchip_OSAL_Malloc(sizeof(VpuCodecContext_t));
    if (p_vpu_ctx == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    OMX_U32          version_sdk = 0;

    if (pRockchipComponent->rkversion != NULL) {
        omx_err("omx decoder info : %s", pRockchipComponent->rkversion);
    }
    if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
        omx_info("drm player mode is true, force to mpp");
        Rockchip_OSAL_GetEnvU32("ro.build.version.sdk", &version_sdk, 0);
    }
    Rockchip_OSAL_Memset((void*)p_vpu_ctx, 0, sizeof(VpuCodecContext_t));
    if (omx_open_vpudec_context(pVideoDec)) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    {
        OMX_U32 i = 0;
        for (i = 0; i < ARRAY_SIZE(kCodeMap); i++) {
            if (kCodeMap[i].omx_id == pVideoDec->codecId) {
                codecId = kCodeMap[i].codec_id;
                break;
            }
        }
    }

    if (pVideoDec->bOld_api == OMX_FALSE) {
        p_vpu_ctx->width = pRockchipInputPort->portDefinition.format.video.nFrameWidth;
        p_vpu_ctx->height = pRockchipInputPort->portDefinition.format.video.nFrameHeight;
        p_vpu_ctx->codecType = CODEC_DECODER;

        p_vpu_ctx->videoCoding = codecId;
    } else {
        Rockchip_OSAL_Free(p_vpu_ctx);
        p_vpu_ctx = NULL;
    }

    /*
     * switch to vpu
     * 0: original mode
     * 1: use mpp first
     * 2: use vpuapi first
     */
    OMX_U32 use_mpp = 0;
    Rockchip_OSAL_GetEnvU32("vendor.omx.vpu.switch", &use_mpp, 0);
    if (use_mpp == VDEC_DBG_VPU_MPP_FIRST) {
        omx_info("switch vpu driver to mpp");
        /* reserved[1] = mpp_mode */
        p_vpu_ctx->extra_cfg.reserved[1] = 1;
    } else if (use_mpp == VDEC_DBG_VPU_VPUAPI_FIRST) {
        /* when vpu ctx is NULL, force to vpuapi */
        omx_info("switch vpu driver to vpuapi");
        Rockchip_OSAL_Free(p_vpu_ctx);
        p_vpu_ctx = NULL;
    }

    if (pVideoDec->rkvpu_open_cxt != NULL) {
        pVideoDec->rkvpu_open_cxt(&p_vpu_ctx);
    }

    if (p_vpu_ctx == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    p_vpu_ctx->enableparsing = 1;
    p_vpu_ctx->extradata_size = 0;
    p_vpu_ctx->extradata = NULL;
    p_vpu_ctx->width = pRockchipInputPort->portDefinition.format.video.nFrameWidth;
    p_vpu_ctx->height = pRockchipInputPort->portDefinition.format.video.nFrameHeight;
    p_vpu_ctx->codecType = CODEC_DECODER;

    p_vpu_ctx->videoCoding = codecId;
    pVideoDec->vpu_ctx = p_vpu_ctx;

    pVideoDec->bFirstFrame = OMX_TRUE;
    pVideoDec->maxCount = 0;
    pVideoDec->bInfoChange = OMX_FALSE;

    if (pVideoDec->bDRMPlayerMode == OMX_FALSE) {
        if (Rkvpu_OMX_CheckIsNeedFastmode(pRockchipComponent) != OMX_ErrorNone) {
            omx_err("check fast mode failed!");
        }
    }

    if (pVideoDec->codecId == CODEC_OMX_VIDEO_CodingHEVC) {
        pVideoDec->bIsHevc = 1;
    }
    if (p_vpu_ctx->width > 1920 && p_vpu_ctx->height > 1088) { // 1920:width, 1088:height
        Rockchip_OSAL_PowerControl(pRockchipComponent, 3840, 2160, pVideoDec->bIsHevc, // 3840:width, 2160:height
            pRockchipInputPort->portDefinition.format.video.xFramerate,
            OMX_TRUE,
            8); // 8:byte alignment
        pVideoDec->bIsPowerControl = OMX_TRUE;
    }

    if (p_vpu_ctx->width > 1920 && p_vpu_ctx->height > 1080) { // 1920:width, 1080:height
        pVideoDec->b4K_flags = OMX_TRUE;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rkvpu_Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    ROCKCHIP_OMX_BASECOMPONENT   *pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    RKVPU_OMX_VIDEODEC_COMPONENT  *pVideoDec   = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;

    FunctionIn();
    if (pVideoDec && pVideoDec->vpu_ctx) {
        if (pVideoDec->rkvpu_close_cxt) {
            pVideoDec->rkvpu_close_cxt(&pVideoDec->vpu_ctx);
        }
        pVideoDec->vpu_ctx = NULL;
        if (pVideoDec->rkapi_hdl) {
            pVideoDec->rkapi_hdl = NULL;
        }
    }
    Rkvpu_ResetAllPortConfig(pOMXComponent);

    goto EXIT;
EXIT:
    FunctionOut();

    return ret;
}


OMX_ERRORTYPE Rockchip_OMX_ComponentConstructor(OMX_HANDLETYPE hComponent, OMX_STRING componentName)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    ROCKCHIP_OMX_BASEPORT      *pRockchipPort = NULL;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        omx_err("OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        omx_err("OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Rockchip_OMX_BaseComponent_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        omx_err("OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Rockchip_OMX_Port_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Rockchip_OMX_BaseComponent_Destructor(pOMXComponent);
        omx_err("OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pVideoDec = Rockchip_OSAL_Malloc(sizeof(RKVPU_OMX_VIDEODEC_COMPONENT));
    if (pVideoDec == NULL) {
        Rockchip_OMX_BaseComponent_Destructor(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        omx_err("OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    Rockchip_OSAL_Memset(pVideoDec, 0, sizeof(RKVPU_OMX_VIDEODEC_COMPONENT));
    pVideoDec->hSharedMemory = Rockchip_OSAL_SharedMemory_Open();
    if (pVideoDec->hSharedMemory == NULL) {
        omx_err("Rockchip_OSAL_SharedMemory_Open open fail");
    }

    pRockchipComponent->componentName = (OMX_STRING)Rockchip_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pRockchipComponent->componentName == NULL) {
        Rockchip_OMX_ComponentDeInit(hComponent);
        ret = OMX_ErrorInsufficientResources;
        omx_err("OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Rockchip_OSAL_Memset(pRockchipComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);
    pVideoDec->nDpbSize = 0;
    pRockchipComponent->hComponentHandle = (OMX_HANDLETYPE)pVideoDec;

    pRockchipComponent->bSaveFlagEOS = OMX_FALSE;
    pRockchipComponent->nRkFlags = 0;
    pRockchipComponent->bBehaviorEOS = OMX_FALSE;
    pVideoDec->bDecSendEOS = OMX_FALSE;
    pVideoDec->bPvr_Flag = OMX_FALSE;
    pVideoDec->bFastMode = OMX_FALSE;
    pVideoDec->bOhosBufferHandle = OMX_FALSE;
    pVideoDec->fp_in = NULL;
    pVideoDec->fp_out = NULL;
    pVideoDec->b4K_flags = OMX_FALSE;
    pVideoDec->codecProfile = 0;
    pVideoDec->power_fd = -1;
    pVideoDec->bIsPowerControl = OMX_FALSE;
    pVideoDec->bIsHevc = 0;
    pVideoDec->bIs10bit = OMX_FALSE;
    pRockchipComponent->bMultiThreadProcess = OMX_TRUE;
    pRockchipComponent->codecType = HW_VIDEO_DEC_CODEC;

    /* for debug */
    pVideoDec->bPrintFps = OMX_FALSE;
    pVideoDec->bPrintBufferPosition = OMX_FALSE;
    pVideoDec->bGtsMediaTest = OMX_FALSE;
    pVideoDec->nVdecDebug = 0;

    pVideoDec->bFirstFrame = OMX_TRUE;

    pVideoDec->vpumem_handle = NULL;

    /* Set componentVersion */
    pRockchipComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pRockchipComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pRockchipComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pRockchipComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pRockchipComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pRockchipComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pRockchipComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pRockchipComponent->specVersion.s.nStep         = STEP_NUMBER;
    /* Input port */

    pRockchipPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    pRockchipPort->portDefinition.nBufferCountActual = MAX_VIDEO_INPUTBUFFER_NUM;
    pRockchipPort->portDefinition.nBufferCountMin = MAX_VIDEO_INPUTBUFFER_NUM;
    pRockchipPort->portDefinition.nBufferSize = 0;
    pRockchipPort->portDefinition.eDomain = OMX_PortDomainVideo;
    pRockchipPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pRockchipPort->portDefinition.format.video.nFrameHeight = DEFAULT_FRAME_HEIGHT;
    pRockchipPort->portDefinition.format.video.nStride = 0; /* DEFAULT_FRAME_WIDTH; */
    pRockchipPort->portDefinition.format.video.nSliceHeight = 0;
    pRockchipPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pRockchipPort->portDefinition.format.video.cMIMEType =  Rockchip_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    pRockchipPort->portDefinition.format.video.pNativeRender = 0;
    pRockchipPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pRockchipPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pRockchipPort->portDefinition.bEnabled = OMX_TRUE;
    pRockchipPort->portWayType = WAY2_PORT;

    /* Output port */
    pRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    pRockchipPort->portDefinition.nBufferCountActual = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pRockchipPort->portDefinition.nBufferCountMin = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pRockchipPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pRockchipPort->portDefinition.eDomain = OMX_PortDomainVideo;
    pRockchipPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pRockchipPort->portDefinition.format.video.nFrameHeight = DEFAULT_FRAME_HEIGHT;
    pRockchipPort->portDefinition.format.video.nStride = 0; /* DEFAULT_FRAME_WIDTH; */
    pRockchipPort->portDefinition.format.video.nSliceHeight = 0;
    pRockchipPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pRockchipPort->portDefinition.format.video.cMIMEType = Rockchip_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "raw/video");
    pRockchipPort->portDefinition.format.video.pNativeRender = 0;
    pRockchipPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pRockchipPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pRockchipPort->portDefinition.bEnabled = OMX_TRUE;
    pRockchipPort->portWayType = WAY2_PORT;
    pRockchipPort->portDefinition.eDomain = OMX_PortDomainVideo;
    pRockchipPort->bufferProcessType = BUFFER_COPY | BUFFER_ANBSHARE;

    pRockchipPort->processData.extInfo = (OMX_PTR)Rockchip_OSAL_Malloc(sizeof(DECODE_CODEC_EXTRA_BUFFERINFO));
    Rockchip_OSAL_Memset(((char *)pRockchipPort->processData.extInfo), 0, sizeof(DECODE_CODEC_EXTRA_BUFFERINFO));
    {
        DECODE_CODEC_EXTRA_BUFFERINFO *pBufferInfo = NULL;
        pBufferInfo = (DECODE_CODEC_EXTRA_BUFFERINFO *)(pRockchipPort->processData.extInfo);
    }
    pOMXComponent->UseBuffer              = &Rkvpu_OMX_UseBuffer;
    pOMXComponent->AllocateBuffer         = &Rkvpu_OMX_AllocateBuffer;
    pOMXComponent->FreeBuffer             = &Rkvpu_OMX_FreeBuffer;
    pOMXComponent->ComponentTunnelRequest = &Rkvpu_OMX_ComponentTunnelRequest;
    pOMXComponent->GetParameter           = &Rkvpu_OMX_GetParameter;
    pOMXComponent->SetParameter           = &Rkvpu_OMX_SetParameter;
    pOMXComponent->GetConfig              = &Rkvpu_OMX_GetConfig;
    pOMXComponent->SetConfig              = &Rkvpu_OMX_SetConfig;
    pOMXComponent->GetExtensionIndex      = &Rkvpu_OMX_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum      = &Rkvpu_OMX_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit        = &Rockchip_OMX_ComponentDeInit;

    pRockchipComponent->rockchip_codec_componentInit      = &Rkvpu_Dec_ComponentInit;
    pRockchipComponent->rockchip_codec_componentTerminate = &Rkvpu_Dec_Terminate;

    pRockchipComponent->rockchip_AllocateTunnelBuffer = &Rkvpu_OMX_AllocateTunnelBuffer;
    pRockchipComponent->rockchip_FreeTunnelBuffer     = &Rkvpu_OMX_FreeTunnelBuffer;
    pRockchipComponent->rockchip_BufferProcessCreate    = &Rkvpu_OMX_BufferProcess_Create;
    pRockchipComponent->rockchip_BufferProcessTerminate = &Rkvpu_OMX_BufferProcess_Terminate;
    pRockchipComponent->rockchip_BufferFlush          = &Rkvpu_OMX_BufferFlush;

    pRockchipPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    if (!strcmp(componentName, RK_OMX_COMPONENT_H264_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/avc");
        pVideoDec->codecId = OMX_VIDEO_CodingAVC;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_H264_DRM_DEC)) {
        omx_err("Rockchip_OMX_ComponentConstructor h264 secure");
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/avc");
        pVideoDec->codecId = OMX_VIDEO_CodingAVC;
#ifdef HAVE_L1_SVP_MODE
        pVideoDec->bDRMPlayerMode = OMX_TRUE;
#endif
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_MPEG4_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/mp4v-es");
        pVideoDec->codecId = OMX_VIDEO_CodingMPEG4;
        pRockchipPort->portDefinition.format.video.eCompressionFormat =  OMX_VIDEO_CodingMPEG4;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_MPEG4_DRM_DEC)) {
        omx_err("Rockchip_OMX_ComponentConstructor mpeg4 secure");
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/mp4v-es");
        pVideoDec->codecId = OMX_VIDEO_CodingMPEG4;
#ifdef HAVE_L1_SVP_MODE
        pVideoDec->bDRMPlayerMode = OMX_TRUE;
#endif
        pRockchipPort->portDefinition.format.video.eCompressionFormat =  OMX_VIDEO_CodingMPEG4;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_H263_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/3gpp");
        pVideoDec->codecId = OMX_VIDEO_CodingH263;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_FLV_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/flv");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingFLV1;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingFLV1;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_MPEG2_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/mpeg2");
        pVideoDec->codecId = OMX_VIDEO_CodingMPEG2;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_MPEG2_DRM_DEC)) {
        omx_err("Rockchip_OMX_ComponentConstructor mpeg2 secure");
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/mpeg2");
        pVideoDec->codecId = OMX_VIDEO_CodingMPEG2;
#ifdef HAVE_L1_SVP_MODE
        pVideoDec->bDRMPlayerMode = OMX_TRUE;
#endif
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_RMVB_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/vnd.rn-realvideo");
        pVideoDec->codecId = OMX_VIDEO_CodingRV;
        pRockchipPort->portDefinition.format.video.eCompressionFormat =  OMX_VIDEO_CodingRV;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_VP8_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/x-vnd.on2.vp8");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP8EXT;
        pRockchipPort->portDefinition.format.video.eCompressionFormat =
            pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP8EXT;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_VC1_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/vc1");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVC1;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVC1;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_WMV3_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/x-ms-wmv");
        pVideoDec->codecId = OMX_VIDEO_CodingWMV;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_VP6_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/vp6");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP6;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP6;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_HEVC_DRM_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/hevc");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC;
#ifdef HAVE_L1_SVP_MODE
        pVideoDec->bDRMPlayerMode = OMX_TRUE;
#endif
#ifndef LOW_VRESION
        pRockchipPort->portDefinition.format.video.eCompressionFormat =
            (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC;
#else
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_OLD_CodingHEVC;
#endif
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_HEVC_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/hevc");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC;
#ifndef LOW_VRESION
        pRockchipPort->portDefinition.format.video.eCompressionFormat =
            (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingHEVC;
#else
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_OLD_CodingHEVC;
#endif
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_MJPEG_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/mjpeg");
        pVideoDec->codecId = OMX_VIDEO_CodingMJPEG;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMJPEG;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_VP9_DEC)) {
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/x-vnd.on2.vp9");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingVP9;
        pRockchipPort->portDefinition.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingVP9;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_VP9_DRM_DEC)) {
        omx_err("Rockchip_OMX_ComponentConstructor VP9 secure");
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/x-vnd.on2.vp9");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingVP9;
#ifdef HAVE_L1_SVP_MODE
        pVideoDec->bDRMPlayerMode = OMX_TRUE;
#endif
        pRockchipPort->portDefinition.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)CODEC_OMX_VIDEO_CodingVP9;
    } else if (!strcmp(componentName, RK_OMX_COMPONENT_VP8_DRM_DEC)) {
        omx_err("Rockchip_OMX_ComponentConstructor VP8 secure");
        Rockchip_OSAL_Memset(pRockchipPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Rockchip_OSAL_Strcpy(pRockchipPort->portDefinition.format.video.cMIMEType, "video/x-vnd.on2.vp8");
        pVideoDec->codecId = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP8EXT;
#ifdef HAVE_L1_SVP_MODE
        pVideoDec->bDRMPlayerMode = OMX_TRUE;
#endif
        pRockchipPort->portDefinition.format.video.eCompressionFormat = (OMX_VIDEO_CODINGTYPE)OMX_VIDEO_CodingVP8EXT;
    } else {
        // IL client specified an invalid component name
        omx_err("VPU Component Invalid Component Name\n");
        ret =  OMX_ErrorInvalidComponentName;
        goto EXIT;
    }
    {
        int gpu_fd = -1;
        gpu_fd = open("/dev/pvrsrvkm", O_RDWR, 0);
        if (gpu_fd > 0) {
            pVideoDec->bPvr_Flag = OMX_TRUE;
            close(gpu_fd);
        }
    }

    Rockchip_OSAL_Strcpy(pRockchipComponent->componentName, componentName);
    Rkvpu_OMX_DebugSwitchfromPropget(pRockchipComponent);

    pRockchipComponent->currentState = OMX_StateLoaded;
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Rockchip_OMX_ComponentDeInit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    ROCKCHIP_OMX_BASECOMPONENT *pRockchipComponent = NULL;
    ROCKCHIP_OMX_BASEPORT      *pRockchipPort = NULL;
    ROCKCHIP_OMX_BASEPORT      *pInputPort = NULL;
    RKVPU_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    OMX_U32                    version_sdk = 0;
    int                        i = 0;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Rockchip_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pRockchipComponent = (ROCKCHIP_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pVideoDec = (RKVPU_OMX_VIDEODEC_COMPONENT *)pRockchipComponent->hComponentHandle;
    if (pVideoDec == NULL) {
        goto EXIT;
    }
    if (pVideoDec->hSharedMemory != NULL) {
        Rockchip_OSAL_SharedMemory_Close(pVideoDec->hSharedMemory, pVideoDec->bDRMPlayerMode);
        pVideoDec->hSharedMemory = NULL;
    }

    Rockchip_OSAL_Closevpumempool(pRockchipComponent);

    if (pVideoDec->fp_in != NULL) {
        fclose(pVideoDec->fp_in);
    }
    if (pVideoDec->fp_out != NULL) {
        fclose(pVideoDec->fp_out);
    }
    if (pVideoDec->b4K_flags == OMX_TRUE) {
        // add for kodi
#ifdef AVS80
        Rockchip_OSAL_SetEnvU32("vendor.gpu.frames_num_of_sectionKD", 0);
        Rockchip_OSAL_SetEnvU32("vendor.gpu.frames_num_to_skip_KD", 0);
#else
        Rockchip_OSAL_SetEnvU32("sys.gpu.frames_num_of_sectionKD", 0);
        Rockchip_OSAL_SetEnvU32("sys.gpu.frames_num_to_skip_KD", 0);
#endif
        pVideoDec->b4K_flags = OMX_FALSE;
    }
    if (pRockchipComponent->pRockchipPort == NULL) {
        return ret;
    }
    pInputPort = &pRockchipComponent->pRockchipPort[INPUT_PORT_INDEX];
    if (pVideoDec->bIsPowerControl == OMX_TRUE) {
        if (pVideoDec->bIs10bit) {
            Rockchip_OSAL_PowerControl(pRockchipComponent, 3840, 2160, pVideoDec->bIsHevc, // 3840:width, 2160:height
                pInputPort->portDefinition.format.video.xFramerate,
                OMX_FALSE,
                10); // 10:byte alignment
        } else {
            Rockchip_OSAL_PowerControl(pRockchipComponent, 3840, 2160, pVideoDec->bIsHevc, // 3840:width, 2160:height
                pInputPort->portDefinition.format.video.xFramerate,
                OMX_FALSE,
                8); // 8:byte alignment
        }
        pVideoDec->bIsPowerControl = OMX_FALSE;
    }

    if (pVideoDec->bDRMPlayerMode == OMX_TRUE) {
        omx_info("drm player mode is true, force to mpp");
        Rockchip_OSAL_GetEnvU32("ro.build.version.sdk", &version_sdk, 0);
    }

    Rockchip_OSAL_Free(pVideoDec);
    pRockchipComponent->hComponentHandle = pVideoDec = NULL;

    if (pRockchipComponent->componentName != NULL) {
        Rockchip_OSAL_Free(pRockchipComponent->componentName);
        pRockchipComponent->componentName = NULL;
    }

    pRockchipPort = &pRockchipComponent->pRockchipPort[OUTPUT_PORT_INDEX];
    if (pRockchipPort->processData.extInfo != NULL) {
        Rockchip_OSAL_Free(pRockchipPort->processData.extInfo);
        pRockchipPort->processData.extInfo = NULL;
    }

    for (i = 0; i < ALL_PORT_NUM; i++) {
        pRockchipPort = &pRockchipComponent->pRockchipPort[i];
        Rockchip_OSAL_Free(pRockchipPort->portDefinition.format.video.cMIMEType);
        if (pRockchipPort->portDefinition.format.video.cMIMEType != NULL)
            pRockchipPort->portDefinition.format.video.cMIMEType = NULL;
    }

    ret = Rockchip_OMX_Port_Destructor(pOMXComponent);

    ret = Rockchip_OMX_BaseComponent_Destructor(hComponent);

EXIT:
    FunctionOut();

    return ret;
}