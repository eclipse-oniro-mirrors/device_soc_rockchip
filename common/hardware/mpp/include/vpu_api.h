/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
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

#ifndef __VPU_API_H__
#define __VPU_API_H__

#include "rk_type.h"

/**
 * @brief rockchip media process interface
 */

#define VPU_API_NOPTS_VALUE (0x8000000000000000LL)

/*
 * bit definition of ColorType in structure VPU_FRAME
 */
#define VPU_OUTPUT_FORMAT_TYPE_MASK (0x0000ffff)
#define VPU_OUTPUT_FORMAT_ARGB8888 (0x00000000)
#define VPU_OUTPUT_FORMAT_ABGR8888 (0x00000001)
#define VPU_OUTPUT_FORMAT_RGB888 (0x00000002)
#define VPU_OUTPUT_FORMAT_RGB565 (0x00000003)
#define VPU_OUTPUT_FORMAT_RGB555 (0x00000004)
#define VPU_OUTPUT_FORMAT_YUV420_SEMIPLANAR (0x00000005)
#define VPU_OUTPUT_FORMAT_YUV420_PLANAR (0x00000006)
#define VPU_OUTPUT_FORMAT_YUV422 (0x00000007)
#define VPU_OUTPUT_FORMAT_YUV444 (0x00000008)
#define VPU_OUTPUT_FORMAT_YCH420 (0x00000009)
#define VPU_OUTPUT_FORMAT_BIT_MASK (0x000f0000)
#define VPU_OUTPUT_FORMAT_BIT_8 (0x00000000)
#define VPU_OUTPUT_FORMAT_BIT_10 (0x00010000)
#define VPU_OUTPUT_FORMAT_BIT_12 (0x00020000)
#define VPU_OUTPUT_FORMAT_BIT_14 (0x00030000)
#define VPU_OUTPUT_FORMAT_BIT_16 (0x00040000)
#define VPU_OUTPUT_FORMAT_COLORSPACE_MASK (0x00f00000)
#define VPU_OUTPUT_FORMAT_COLORSPACE_BT709 (0x00100000)
#define VPU_OUTPUT_FORMAT_COLORSPACE_BT2020 (0x00200000)
#define VPU_OUTPUT_FORMAT_DYNCRANGE_MASK (0x0f000000)
#define VPU_OUTPUT_FORMAT_DYNCRANGE_SDR (0x00000000)
#define VPU_OUTPUT_FORMAT_DYNCRANGE_HDR10 (0x01000000)
#define VPU_OUTPUT_FORMAT_DYNCRANGE_HDR_HLG (0x02000000)
#define VPU_OUTPUT_FORMAT_DYNCRANGE_HDR_DOLBY (0x03000000)

/**
 * @brief input picture type
 */
typedef enum {
    ENC_INPUT_YUV420_PLANAR = 0,           /**< YYYY... UUUU... VVVV */
    ENC_INPUT_YUV420_SEMIPLANAR = 1,       /**< YYYY... UVUVUV...    */
    ENC_INPUT_YUV422_INTERLEAVED_YUYV = 2, /**< YUYVYUYV...          */
    ENC_INPUT_YUV422_INTERLEAVED_UYVY = 3, /**< UYVYUYVY...          */
    ENC_INPUT_RGB565 = 4,                  /**< 16-bit RGB           */
    ENC_INPUT_BGR565 = 5,                  /**< 16-bit RGB           */
    ENC_INPUT_RGB555 = 6,                  /**< 15-bit RGB           */
    ENC_INPUT_BGR555 = 7,                  /**< 15-bit RGB           */
    ENC_INPUT_RGB444 = 8,                  /**< 12-bit RGB           */
    ENC_INPUT_BGR444 = 9,                  /**< 12-bit RGB           */
    ENC_INPUT_RGB888 = 10,                 /**< 24-bit RGB           */
    ENC_INPUT_BGR888 = 11,                 /**< 24-bit RGB           */
    ENC_INPUT_RGB101010 = 12,              /**< 30-bit RGB           */
    ENC_INPUT_BGR101010 = 13               /**< 30-bit RGB           */
} EncInputPictureType;

typedef enum VPU_API_CMD {
    VPU_API_ENC_SETCFG,
    VPU_API_ENC_GETCFG,
    VPU_API_ENC_SETFORMAT,
    VPU_API_ENC_SETIDRFRAME,

    VPU_API_ENABLE_DEINTERLACE,
    VPU_API_SET_VPUMEM_CONTEXT,
    VPU_API_USE_PRESENT_TIME_ORDER,
    VPU_API_SET_DEFAULT_WIDTH_HEIGH,
    VPU_API_SET_INFO_CHANGE,
    VPU_API_USE_FAST_MODE,
    VPU_API_DEC_GET_STREAM_COUNT,
    VPU_API_GET_VPUMEM_USED_COUNT,
    VPU_API_GET_FRAME_INFO,
    VPU_API_SET_OUTPUT_BLOCK,
    VPU_API_GET_EOS_STATUS,
    VPU_API_SET_OUTPUT_MODE,

    VPU_API_DEC_GET_DPB_SIZE = 0X100, // add by zhanghanmeng

    /* get sps/pps header */
    VPU_API_GET_EXTRA_INFO = 0x200,

    VPU_API_SET_IMMEDIATE_OUT = 0x1000,
    VPU_API_SET_PARSER_SPLIT_MODE, /* NOTE: should control before init */

    VPU_API_ENC_VEPU22_START = 0x2000,
    VPU_API_ENC_SET_VEPU22_CFG,
    VPU_API_ENC_GET_VEPU22_CFG,
    VPU_API_ENC_SET_VEPU22_CTU_QP,
    VPU_API_ENC_SET_VEPU22_ROI,

    /* mlvec dynamic configure */
    VPU_API_ENC_MLVEC_CFG = 0x4000,
    VPU_API_ENC_SET_MAX_TID,
    VPU_API_ENC_SET_MARK_LTR,
    VPU_API_ENC_SET_USE_LTR,
    VPU_API_ENC_SET_FRAME_QP,
    VPU_API_ENC_SET_BASE_LAYER_PID,
} VPU_API_CMD;

typedef struct {
    unsigned int TimeLow;
    unsigned int TimeHigh;
} TIME_STAMP;

typedef struct {
    unsigned int CodecType;
    unsigned int ImgWidth;
    unsigned int ImgHeight;
    unsigned int ImgHorStride;
    unsigned int ImgVerStride;
    unsigned int BufSize;
} VPU_GENERIC;

typedef struct VPUMem {
    unsigned int phy_addr;
    unsigned int *vir_addr;
    unsigned int size;
    unsigned int *offset;
} VPUMemLinear_t;

typedef struct tVPU_FRAME {
    unsigned int FrameBusAddr[2]; // 0: Y address; 1: UV address;
    unsigned int FrameWidth;      // buffer horizontal stride
    unsigned int FrameHeight;     // buffer vertical   stride
    unsigned int OutputWidth;     // deprecated
    unsigned int OutputHeight;    // deprecated
    unsigned int DisplayWidth;    // valid width  for display
    unsigned int DisplayHeight;   // valid height for display
    unsigned int CodingType;
    unsigned int FrameType; // frame; top_field_first; bot_field_first
    unsigned int ColorType;
    unsigned int DecodeFrmNum;
    TIME_STAMP ShowTime;
    unsigned int ErrorInfo; // error information
    unsigned int employ_cnt;
    VPUMemLinear_t vpumem;
    struct tVPU_FRAME *next_frame;
    union {
        struct {
            unsigned int Res0[2];
            struct {
                unsigned int ColorPrimaries : 8;
                unsigned int ColorTransfer : 8;
                unsigned int ColorCoeffs : 8;
                unsigned int ColorRange : 1;
                unsigned int Res1 : 7;
            };

            unsigned int Res2;
        };

        unsigned int Res[4];
    };
} VPU_FRAME;

typedef struct VideoPacket {
    RK_S64 pts; /* with unit of us */
    RK_S64 dts; /* with unit of us */
    unsigned char *data;
    signed int size;
    unsigned int capability;
    unsigned int nFlags;
} VideoPacket_t;

typedef struct DecoderOut {
    unsigned char *data;
    unsigned int size;
    RK_S64 timeUs;
    signed int nFlags;
} DecoderOut_t;

typedef struct ParserOut {
    unsigned char *data;
    unsigned int size;
    RK_S64 timeUs;
    unsigned int nFlags;
    unsigned int width;
    unsigned int height;
} ParserOut_t;

typedef struct EncInputStream {
    unsigned char *buf;
    signed int size;
    unsigned int bufPhyAddr;
    RK_S64 timeUs;
    unsigned int nFlags;
} EncInputStream_t;

typedef struct EncoderOut {
    unsigned char *data;
    signed int size;
    RK_S64 timeUs;
    signed int keyFrame;
} EncoderOut_t;

/*
 * @brief Enumeration used to define the possible video compression codings.
 * @note  This essentially refers to file extensions. If the coding is
 *        being used to specify the ENCODE type, then additional work
 *        must be done to configure the exact flavor of the compression
 *        to be used.  For decode cases where the user application can
 *        not differentiate between MPEG-4 and H.264 bit streams, it is
 *        up to the codec to handle this.
 *
 *        sync with the omx_video.h
 */
typedef enum OMX_RK_VIDEO_CODINGTYPE {
    OMX_RK_VIDEO_CodingUnused,           /**< Value when coding is N/A */
    OMX_RK_VIDEO_CodingAutoDetect,       /**< Autodetection of coding type */
    OMX_RK_VIDEO_CodingMPEG2,            /**< AKA: H.262 */
    OMX_RK_VIDEO_CodingH263,             /**< H.263 */
    OMX_RK_VIDEO_CodingMPEG4,            /**< MPEG-4 */
    OMX_RK_VIDEO_CodingWMV,              /**< Windows Media Video (WMV1,WMV2,WMV3)*/
    OMX_RK_VIDEO_CodingRV,               /**< all versions of Real Video */
    OMX_RK_VIDEO_CodingAVC,              /**< H.264/AVC */
    OMX_RK_VIDEO_CodingMJPEG,            /**< Motion JPEG */
    OMX_RK_VIDEO_CodingVP8,              /**< VP8 */
    OMX_RK_VIDEO_CodingVP9,              /**< VP9 */
    OMX_RK_VIDEO_CodingVC1 = 0x01000000, /**< Windows Media Video (WMV1,WMV2,WMV3)*/
    OMX_RK_VIDEO_CodingFLV1,             /**< Sorenson H.263 */
    OMX_RK_VIDEO_CodingDIVX3,            /**< DIVX3 */
    OMX_RK_VIDEO_CodingVP6,
    OMX_RK_VIDEO_CodingHEVC, /**< H.265/HEVC */
    OMX_RK_VIDEO_CodingAVS,  /**< AVS+ */
    OMX_RK_VIDEO_CodingKhronosExtensions = 0x6F000000,
    OMX_RK_VIDEO_CodingVendorStartUnused = 0x7F000000,
    OMX_RK_VIDEO_CodingMax = 0x7FFFFFFF
} OMX_RK_VIDEO_CODINGTYPE;

typedef enum CODEC_TYPE {
    CODEC_NONE,
    CODEC_DECODER,
    CODEC_ENCODER,
    CODEC_BUTT,
} CODEC_TYPE;

typedef enum VPU_API_ERR {
    VPU_API_OK = 0,
    VPU_API_ERR_UNKNOW = -1,
    VPU_API_ERR_BASE = -1000,
    VPU_API_ERR_LIST_STREAM = VPU_API_ERR_BASE - 1,
    VPU_API_ERR_INIT = VPU_API_ERR_BASE - 2,
    VPU_API_ERR_VPU_CODEC_INIT = VPU_API_ERR_BASE - 3,
    VPU_API_ERR_STREAM = VPU_API_ERR_BASE - 4,
    VPU_API_ERR_FATAL_THREAD = VPU_API_ERR_BASE - 5,
    VPU_API_EOS_STREAM_REACHED = VPU_API_ERR_BASE - 11,

    VPU_API_ERR_BUTT,
} VPU_API_ERR;

typedef enum VPU_FRAME_ERR {
    VPU_FRAME_ERR_UNKNOW = 0x0001,
    VPU_FRAME_ERR_UNSUPPORT = 0x0002,
} VPU_FRAME_ERR;

typedef struct EncParameter {
    signed int width;
    signed int height;
    signed int rc_mode; /* 0 - CQP mode; 1 - CBR mode; 2 - FIXQP mode */
    signed int bitRate; /* target bitrate */
    signed int framerate;
    signed int qp;
    signed int enableCabac;
    signed int cabacInitIdc;
    signed int format;
    signed int intraPicRate;
    signed int framerateout;
    signed int profileIdc;
    signed int levelIdc;
    signed int reserved[3];
} EncParameter_t;

typedef struct EXtraCfg {
    signed int vc1extra_size;
    signed int vp6codeid;
    signed int tsformat;
    unsigned int ori_vpu; /* use origin vpu framework */
    /* below used in decode */
    unsigned int mpp_mode;   /* use mpp framework */
    unsigned int bit_depth;  /* 8 or 10 bit */
    unsigned int yuv_format; /* 0:420 1:422 2:444 */
    unsigned int reserved[16];
} EXtraCfg_t;

/**
 * @brief vpu function interface
 */
typedef struct VpuCodecContext {
    void *vpuApiObj;

    CODEC_TYPE codecType;
    OMX_RK_VIDEO_CODINGTYPE videoCoding;

    unsigned int width;
    unsigned int height;
    void *extradata;
    signed int extradata_size;

    unsigned char enableparsing;

    signed int no_thread;
    EXtraCfg_t extra_cfg;

    void *private_data;

    /*
     ** 1: error state(not working)  0: working
     */
    signed int decoder_err;

    /**
     * Allocate and initialize an VpuCodecContext.
     *
     * @param ctx The context of vpu api, allocated in this function.
     * @param extraData The extra data of codec, some codecs need / can
     *        use extradata like Huffman tables, also live VC1 codec can
     *        use extradata to initialize itself.
     * @param extra_size The size of extra data.
     *
     * @return 0 for init success, others for failure.
     * @note check whether ctx has been allocated success after you do init.
     */
    signed int (*init)(struct VpuCodecContext *ctx, unsigned char *extraData, unsigned int extra_size);
    /**
     * @brief both send video stream packet to decoder and get video frame from
     *        decoder at the same time
     * @param ctx The context of vpu codec
     * @param pkt[in] Stream to be decoded
     * @param aDecOut[out] Decoding frame
     * @return 0 for decode success, others for failure.
     */
    signed int (*decode)(struct VpuCodecContext *ctx, VideoPacket_t *pkt, DecoderOut_t *aDecOut);
    /**
     * @brief both send video frame to encoder and get encoded video stream from
     *        encoder at the same time.
     * @param ctx The context of vpu codec
     * @param aEncInStrm[in] Frame to be encoded
     * @param aEncOut[out] Encoding stream
     * @return 0 for encode success, others for failure.
     */
    signed int (*encode)(struct VpuCodecContext *ctx, EncInputStream_t *aEncInStrm, EncoderOut_t *aEncOut);
    /**
     * @brief flush codec while do fast forward playing.
     * @param ctx The context of vpu codec
     * @return 0 for flush success, others for failure.
     */
    signed int (*flush)(struct VpuCodecContext *ctx);
    signed int (*control)(struct VpuCodecContext *ctx, VPU_API_CMD cmdType, void *param);
    /**
     * @brief send video stream packet to decoder only, async interface
     * @param ctx The context of vpu codec
     * @param pkt Stream to be decoded
     * @return 0 for success, others for failure.
     */
    signed int (*decode_sendstream)(struct VpuCodecContext *ctx, VideoPacket_t *pkt);
    /**
     * @brief get video frame from decoder only, async interface
     * @param ctx The context of vpu codec
     * @param aDecOut Decoding frame
     * @return 0 for success, others for failure.
     */
    signed int (*decode_getframe)(struct VpuCodecContext *ctx, DecoderOut_t *aDecOut);
    /**
     * @brief send video frame to encoder only, async interface
     * @param ctx The context of vpu codec
     * @param aEncInStrm Frame to be encoded
     * @return 0 for success, others for failure.
     */
    signed int (*encoder_sendframe)(struct VpuCodecContext *ctx, EncInputStream_t *aEncInStrm);
    /**
     * @brief get encoded video packet from encoder only, async interface
     * @param ctx The context of vpu codec
     * @param aEncOut Encoding stream
     * @return 0 for success, others for failure.
     */
    signed int (*encoder_getstream)(struct VpuCodecContext *ctx, EncoderOut_t *aEncOut);
} VpuCodecContext_t;

/* allocated vpu codec context */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief open context of vpu
 * @param ctx pointer of vpu codec context
 */
signed int vpu_open_context(struct VpuCodecContext **ctx);
/**
 * @brief close context of vpu
 * @param ctx pointer of vpu codec context
 */
signed int vpu_close_context(struct VpuCodecContext **ctx);

#ifdef __cplusplus
}
#endif

/*
 * vpu_mem api
 */
#define vpu_display_mem_pool_FIELDS                                                                                    \
    signed int (*commit_hdl)(vpu_display_mem_pool * p, signed int hdl, signed int size);                               \
    void *(*get_free)(vpu_display_mem_pool * p);                                                                       \
    signed int (*inc_used)(vpu_display_mem_pool * p, void *hdl);                                                       \
    signed int (*put_used)(vpu_display_mem_pool * p, void *hdl);                                                       \
    signed int (*reset)(vpu_display_mem_pool * p);                                                                     \
    signed int (*get_unused_num)(vpu_display_mem_pool * p);                                                            \
    signed int buff_size;                                                                                              \
    float version;                                                                                                     \
    signed int res[18]

typedef struct vpu_display_mem_pool vpu_display_mem_pool;

struct vpu_display_mem_pool {
    vpu_display_mem_pool_FIELDS;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * vpu memory handle interface
 */
signed int VPUMemJudgeIommu(void);
signed int VPUMallocLinear(VPUMemLinear_t *p, unsigned int size);
signed int VPUFreeLinear(VPUMemLinear_t *p);
signed int VPUMemDuplicate(VPUMemLinear_t *dst, VPUMemLinear_t *src);
signed int VPUMemLink(VPUMemLinear_t *p);
signed int VPUMemFlush(VPUMemLinear_t *p);
signed int VPUMemClean(VPUMemLinear_t *p);
signed int VPUMemInvalidate(VPUMemLinear_t *p);
signed int VPUMemGetFD(VPUMemLinear_t *p);
signed int VPUMallocLinearFromRender(VPUMemLinear_t *p, unsigned int size, void *ctx);

/*
 * vpu memory allocator and manager interface
 */
vpu_display_mem_pool *open_vpu_memory_pool(void);
void close_vpu_memory_pool(vpu_display_mem_pool *p);
int create_vpu_memory_pool_allocator(vpu_display_mem_pool **ipool, int num, int size);
void release_vpu_memory_pool_allocator(vpu_display_mem_pool *ipool);

#ifdef __cplusplus
}
#endif

#endif
