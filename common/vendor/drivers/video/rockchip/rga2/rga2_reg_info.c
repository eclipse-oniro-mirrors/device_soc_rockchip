/* SPDX-License-Identifier: GPL-2.0 */

#include <linux/memory.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <asm/delay.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/syscalls.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <asm/cacheflush.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/wakelock.h>
#include <linux/version.h>

#include "rga2_reg_info.h"
#include "rga2_type.h"
#include "rga2_rop.h"
#include "rga2.h"

static void RGA2_reg_get_param(unsigned char *base, struct rga2_req *msg)
{
    unsigned int *bRGA_SRC_INFO;
    unsigned int *bRGA_SRC_X_FACTOR;
    unsigned int *bRGA_SRC_Y_FACTOR;
    unsigned int sw, sh;
    unsigned int dw, dh;
    unsigned int param_x, param_y;
    unsigned char x_flag, y_flag;

    unsigned int reg;

    bRGA_SRC_INFO = (unsigned int *)(base + RGA2_SRC_INFO_OFFSET);
    reg = *bRGA_SRC_INFO;

    bRGA_SRC_X_FACTOR = (unsigned int *)(base + RGA2_SRC_X_FACTOR_OFFSET);
    bRGA_SRC_Y_FACTOR = (unsigned int *)(base + RGA2_SRC_Y_FACTOR_OFFSET);

    x_flag = y_flag = 0;

    if (((msg->rotate_mode & 0x3) == 1) || ((msg->rotate_mode & 0x3) == RGA2_INDEX_TH)) {
        dw = msg->dst.act_h;
        dh = msg->dst.act_w;
    } else {
        dw = msg->dst.act_w;
        dh = msg->dst.act_h;
    }

    sw = msg->src.act_w;
    sh = msg->src.act_h;

    if (sw > dw) {
        x_flag = 1;
#if SCALE_DOWN_LARGE
        param_x = ((dw) << RGA2_INDEX_SIX) / (sw) + 1;
#else
        param_x = ((dw) << RGA2_INDEX_SIX) / (sw);
#endif
        *bRGA_SRC_X_FACTOR |= ((param_x & 0xffff) << 0);
    } else if (sw < dw) {
        x_flag = RGA2_INDEX_TW;
#if 1 // SCALE_MINUS1
        param_x = ((sw - 1) << RGA2_INDEX_SIX) / (dw - 1);
#else
        param_x = ((sw) << RGA2_INDEX_SIX) / (dw);
#endif
        *bRGA_SRC_X_FACTOR |= ((param_x & 0xffff) << RGA2_INDEX_SIX);
    } else {
        *bRGA_SRC_X_FACTOR = 0; // ((1 << 14) << RGA2_INDEX_SIX) | (1 << 14);
    }

    if (sh > dh) {
        y_flag = 1;
#if SCALE_DOWN_LARGE
        param_y = ((dh) << RGA2_INDEX_SIX) / (sh) + 1;
#else
        param_y = ((dh) << RGA2_INDEX_SIX) / (sh);
#endif
        *bRGA_SRC_Y_FACTOR |= ((param_y & 0xffff) << 0);
    } else if (sh < dh) {
        y_flag = RGA2_INDEX_TW;
#if 1 // SCALE_MINUS1
        param_y = ((sh - 1) << RGA2_INDEX_SIX) / (dh - 1);
#else
        param_y = ((sh) << RGA2_INDEX_SIX) / (dh);
#endif
        *bRGA_SRC_Y_FACTOR |= ((param_y & 0xffff) << RGA2_INDEX_SIX);
    } else {
        *bRGA_SRC_Y_FACTOR = 0; // ((1 << 14) << RGA2_INDEX_SIX) | (1 << 14);
    }

    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_HSCL_MODE)) | (s_RGA2_SRC_INFO_SW_SW_SRC_HSCL_MODE(x_flag)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_VSCL_MODE)) | (s_RGA2_SRC_INFO_SW_SW_SRC_VSCL_MODE(y_flag)));
}

static void RGA2_set_mode_ctrl(u8 *base, struct rga2_req *msg)
{
    unsigned int *bRGA_MODE_CTL;
    unsigned int reg = 0;
    unsigned int render_mode = msg->render_mode;

    bRGA_MODE_CTL = (u32 *)(base + RGA2_MODE_CTRL_OFFSET);

    if (msg->render_mode == RGA2_INDEX_FO) {
        render_mode = RGA2_INDEX_TH;
    }

    reg = ((reg & (~m_RGA2_MODE_CTRL_SW_RENDER_MODE)) | (s_RGA2_MODE_CTRL_SW_RENDER_MODE(render_mode)));
    reg = ((reg & (~m_RGA2_MODE_CTRL_SW_BITBLT_MODE)) | (s_RGA2_MODE_CTRL_SW_BITBLT_MODE(msg->bitblt_mode)));
    reg = ((reg & (~m_RGA2_MODE_CTRL_SW_CF_ROP4_PAT)) | (s_RGA2_MODE_CTRL_SW_CF_ROP4_PAT(msg->color_fill_mode)));
    reg = ((reg & (~m_RGA2_MODE_CTRL_SW_ALPHA_ZERO_KET)) | (s_RGA2_MODE_CTRL_SW_ALPHA_ZERO_KET(msg->alpha_zero_key)));
    reg = ((reg & (~m_RGA2_MODE_CTRL_SW_GRADIENT_SAT)) |
           (s_RGA2_MODE_CTRL_SW_GRADIENT_SAT(msg->alpha_rop_flag >> RGA2_INDEX_SE)));
    reg = ((reg & (~m_RGA2_MODE_CTRL_SW_INTR_CF_E)) | (s_RGA2_MODE_CTRL_SW_INTR_CF_E(msg->CMD_fin_int_enable)));

    *bRGA_MODE_CTL = reg;
}

static void RGA2_set_reg_src_info(unsigned char *base, struct rga2_req *msg)
{
    unsigned int *bRGA_SRC_INFO;
    unsigned int *bRGA_SRC_BASE0, *bRGA_SRC_BASE1, *bRGA_SRC_BASE2;
    unsigned int *bRGA_SRC_VIR_INFO;
    unsigned int *bRGA_SRC_ACT_INFO;
    unsigned int *bRGA_MASK_ADDR;
    unsigned int *bRGA_SRC_TR_COLOR0, *bRGA_SRC_TR_COLOR1;
    unsigned char src_fmt_yuv400_en = 0;

    unsigned int reg = 0;
    unsigned char src0_format = 0;

    unsigned char src0_rb_swp = 0;
    unsigned char src0_rgb_pack = 0;
    unsigned char src0_alpha_swp = 0;

    unsigned char src0_cbcr_swp = 0;
    unsigned char pixel_width = 1;
    unsigned int stride = 0;
    unsigned int uv_stride = 0;
    unsigned int mask_stride = 0;
    unsigned int ydiv = 1, xdiv = RGA2_INDEX_TW;
    unsigned char yuv10 = 0;

    unsigned int sw, sh;
    unsigned int dw, dh;
    unsigned char rotate_mode;
    unsigned char scale_w_flag, scale_h_flag;

    bRGA_SRC_INFO = (unsigned int *)(base + RGA2_SRC_INFO_OFFSET);

    bRGA_SRC_BASE0 = (unsigned int *)(base + RGA2_SRC_BASE0_OFFSET);
    bRGA_SRC_BASE1 = (unsigned int *)(base + RGA2_SRC_BASE1_OFFSET);
    bRGA_SRC_BASE2 = (unsigned int *)(base + RGA2_SRC_BASE2_OFFSET);

    bRGA_SRC_VIR_INFO = (unsigned int *)(base + RGA2_SRC_VIR_INFO_OFFSET);
    bRGA_SRC_ACT_INFO = (unsigned int *)(base + RGA2_SRC_ACT_INFO_OFFSET);

    bRGA_MASK_ADDR = (unsigned int *)(base + RGA2_MASK_BASE_OFFSET);

    bRGA_SRC_TR_COLOR0 = (unsigned int *)(base + RGA2_SRC_TR_COLOR0_OFFSET);
    bRGA_SRC_TR_COLOR1 = (unsigned int *)(base + RGA2_SRC_TR_COLOR1_OFFSET);

    if (msg->src.format == RGA2_FORMAT_YCbCr_420_SP_10B || msg->src.format == RGA2_FORMAT_YCrCb_420_SP_10B) {
        if ((msg->src.act_w == msg->dst.act_w) && (msg->src.act_h == msg->dst.act_h) && (msg->rotate_mode == 0)) {
            msg->rotate_mode = 1 << RGA2_INDEX_SI;
        }
    }

    {
        rotate_mode = msg->rotate_mode & 0x3;

        sw = msg->src.act_w;
        sh = msg->src.act_h;

        if ((rotate_mode == 1) | (rotate_mode == RGA2_INDEX_TH)) {
            dw = msg->dst.act_h;
            dh = msg->dst.act_w;
        } else {
            dw = msg->dst.act_w;
            dh = msg->dst.act_h;
        }

        if (sw > dw) {
            scale_w_flag = 1;
        } else if (sw < dw) {
            scale_w_flag = RGA2_INDEX_TW;
        } else {
            scale_w_flag = 0;
            if (msg->rotate_mode >> RGA2_INDEX_SI) {
                scale_w_flag = RGA2_INDEX_TH;
            }
        }

        if (sh > dh) {
            scale_h_flag = 1;
        } else if (sh < dh) {
            scale_h_flag = RGA2_INDEX_TW;
        } else {
            scale_h_flag = 0;
            if (msg->rotate_mode >> RGA2_INDEX_SI) {
                scale_h_flag = RGA2_INDEX_TH;
            }
        }
    }

    switch (msg->src.format) {
        case RGA2_FORMAT_RGBA_8888:
            src0_format = 0x0;
            pixel_width = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_BGRA_8888:
            src0_format = 0x0;
            src0_rb_swp = 0x1;
            pixel_width = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_RGBX_8888:
            src0_format = 0x1;
            pixel_width = RGA2_INDEX_FO;
            msg->src_trans_mode &= 0x07;
            break;
        case RGA2_FORMAT_BGRX_8888:
            src0_format = 0x1;
            src0_rb_swp = 0x1;
            pixel_width = RGA2_INDEX_FO;
            msg->src_trans_mode &= 0x07;
            break;
        case RGA2_FORMAT_RGB_888:
            src0_format = 0x2;
            src0_rgb_pack = 1;
            pixel_width = RGA2_INDEX_TH;
            msg->src_trans_mode &= 0x07;
            break;
        case RGA2_FORMAT_BGR_888:
            src0_format = 0x2;
            src0_rgb_pack = 1;
            src0_rb_swp = 1;
            pixel_width = RGA2_INDEX_TH;
            msg->src_trans_mode &= 0x07;
            break;
        case RGA2_FORMAT_RGB_565:
            src0_format = 0x4;
            pixel_width = RGA2_INDEX_TW;
            msg->src_trans_mode &= 0x07;
            break;
        case RGA2_FORMAT_RGBA_5551:
            src0_format = 0x5;
            pixel_width = RGA2_INDEX_TW;
            src0_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_RGBA_4444:
            src0_format = 0x6;
            pixel_width = RGA2_INDEX_TW;
            src0_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_BGR_565:
            src0_format = 0x4;
            pixel_width = RGA2_INDEX_TW;
            msg->src_trans_mode &= 0x07;
            src0_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_BGRA_5551:
            src0_format = 0x5;
            pixel_width = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_BGRA_4444:
            src0_format = 0x6;
            pixel_width = RGA2_INDEX_TW;
            break;

        /* ARGB */
        /* In colorkey mode, xrgb/xbgr does not need to enable the alpha channel */
        case RGA2_FORMAT_ARGB_8888:
            src0_format = 0x0;
            pixel_width = RGA2_INDEX_FO;
            src0_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_8888:
            src0_format = 0x0;
            pixel_width = RGA2_INDEX_FO;
            src0_alpha_swp = 1;
            src0_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_XRGB_8888:
            src0_format = 0x1;
            pixel_width = RGA2_INDEX_FO;
            src0_alpha_swp = 1;
            msg->src_trans_mode &= 0x07;
            break;
        case RGA2_FORMAT_XBGR_8888:
            src0_format = 0x1;
            pixel_width = RGA2_INDEX_FO;
            src0_alpha_swp = 1;
            src0_rb_swp = 0x1;
            msg->src_trans_mode &= 0x07;
            break;
        case RGA2_FORMAT_ARGB_5551:
            src0_format = 0x5;
            pixel_width = RGA2_INDEX_TW;
            src0_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_5551:
            src0_format = 0x5;
            pixel_width = RGA2_INDEX_TW;
            src0_alpha_swp = 1;
            src0_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_ARGB_4444:
            src0_format = 0x6;
            pixel_width = RGA2_INDEX_TW;
            src0_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_4444:
            src0_format = 0x6;
            pixel_width = RGA2_INDEX_TW;
            src0_alpha_swp = 1;
            src0_rb_swp = 0x1;
            break;

        case RGA2_FORMAT_YVYU_422:
            src0_format = 0x7;
            pixel_width = RGA2_INDEX_TW;
            src0_cbcr_swp = 1;
            src0_rb_swp = 0x1;
            break; // rbswap=ycswap
        case RGA2_FORMAT_VYUY_422:
            src0_format = 0x7;
            pixel_width = RGA2_INDEX_TW;
            src0_cbcr_swp = 1;
            src0_rb_swp = 0x0;
            break;
        case RGA2_FORMAT_YUYV_422:
            src0_format = 0x7;
            pixel_width = RGA2_INDEX_TW;
            src0_cbcr_swp = 0;
            src0_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_UYVY_422:
            src0_format = 0x7;
            pixel_width = RGA2_INDEX_TW;
            src0_cbcr_swp = 0;
            src0_rb_swp = 0x0;
            break;

        case RGA2_FORMAT_YCbCr_422_SP:
            src0_format = 0x8;
            xdiv = 1;
            ydiv = 1;
            break;
        case RGA2_FORMAT_YCbCr_422_P:
            src0_format = 0x9;
            xdiv = RGA2_INDEX_TW;
            ydiv = 1;
            break;
        case RGA2_FORMAT_YCbCr_420_SP:
            src0_format = 0xa;
            xdiv = 1;
            ydiv = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_YCbCr_420_P:
            src0_format = 0xb;
            xdiv = RGA2_INDEX_TW;
            ydiv = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_YCrCb_422_SP:
            src0_format = 0x8;
            xdiv = 1;
            ydiv = 1;
            src0_cbcr_swp = 1;
            break;
        case RGA2_FORMAT_YCrCb_422_P:
            src0_format = 0x9;
            xdiv = RGA2_INDEX_TW;
            ydiv = 1;
            src0_cbcr_swp = 1;
            break;
        case RGA2_FORMAT_YCrCb_420_SP:
            src0_format = 0xa;
            xdiv = 1;
            ydiv = RGA2_INDEX_TW;
            src0_cbcr_swp = 1;
            break;
        case RGA2_FORMAT_YCrCb_420_P:
            src0_format = 0xb;
            xdiv = RGA2_INDEX_TW;
            ydiv = RGA2_INDEX_TW;
            src0_cbcr_swp = 1;
            break;

        case RGA2_FORMAT_YCbCr_420_SP_10B:
            src0_format = 0xa;
            xdiv = 1;
            ydiv = RGA2_INDEX_TW;
            yuv10 = 1;
            break;
        case RGA2_FORMAT_YCrCb_420_SP_10B:
            src0_format = 0xa;
            xdiv = 1;
            ydiv = RGA2_INDEX_TW;
            src0_cbcr_swp = 1;
            yuv10 = 1;
            break;
        case RGA2_FORMAT_YCbCr_422_SP_10B:
            src0_format = 0x8;
            xdiv = 1;
            ydiv = 1;
            yuv10 = 1;
            break;
        case RGA2_FORMAT_YCrCb_422_SP_10B:
            src0_format = 0x8;
            xdiv = 1;
            ydiv = 1;
            src0_cbcr_swp = 1;
            yuv10 = 1;
            break;

        case RGA2_FORMAT_YCbCr_400:
            src0_format = 0x8;
            src_fmt_yuv400_en = 1;
            xdiv = 1;
            ydiv = 1;
            break;
        default:
            break;
    };

    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SRC_FMT)) | (s_RGA2_SRC_INFO_SW_SRC_FMT(src0_format)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_RB_SWAP)) | (s_RGA2_SRC_INFO_SW_SW_SRC_RB_SWAP(src0_rb_swp)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_ALPHA_SWAP)) | (s_RGA2_SRC_INFO_SW_SW_SRC_ALPHA_SWAP(src0_alpha_swp)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_UV_SWAP)) | (s_RGA2_SRC_INFO_SW_SW_SRC_UV_SWAP(src0_cbcr_swp)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_CSC_MODE)) | (s_RGA2_SRC_INFO_SW_SW_SRC_CSC_MODE(msg->yuv2rgb_mode)));

    reg =
        ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_ROT_MODE)) | (s_RGA2_SRC_INFO_SW_SW_SRC_ROT_MODE(msg->rotate_mode & 0x3)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_MIR_MODE)) |
           (s_RGA2_SRC_INFO_SW_SW_SRC_MIR_MODE((msg->rotate_mode >> RGA2_INDEX_FO) & 0x3)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_HSCL_MODE)) | (s_RGA2_SRC_INFO_SW_SW_SRC_HSCL_MODE((scale_w_flag))));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_VSCL_MODE)) | (s_RGA2_SRC_INFO_SW_SW_SRC_VSCL_MODE((scale_h_flag))));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_SCL_FILTER)) |
           (s_RGA2_SRC_INFO_SW_SW_SRC_SCL_FILTER((msg->scale_bicu_mode))));
    reg =
        ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_TRANS_MODE)) | (s_RGA2_SRC_INFO_SW_SW_SRC_TRANS_MODE(msg->src_trans_mode)));
    reg =
        ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_TRANS_E)) | (s_RGA2_SRC_INFO_SW_SW_SRC_TRANS_E(msg->src_trans_mode >> 1)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_SRC_DITHER_UP_E)) |
           (s_RGA2_SRC_INFO_SW_SW_SRC_DITHER_UP_E((msg->alpha_rop_flag >> RGA2_INDEX_FO) & 0x1)));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_VSP_MODE_SEL)) |
           (s_RGA2_SRC_INFO_SW_SW_VSP_MODE_SEL((msg->scale_bicu_mode >> RGA2_INDEX_FO))));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_YUV10_E)) | (s_RGA2_SRC_INFO_SW_SW_YUV10_E((yuv10))));
#if 1
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_YUV10_ROUND_E)) | (s_RGA2_SRC_INFO_SW_SW_YUV10_ROUND_E((yuv10))));
#else
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_YUV10_ROUND_E)) |
           (s_RGA2_SRC_INFO_SW_SW_YUV10_ROUND_E(((msg->yuv2rgb_mode >> RGA2_INDEX_SI) & 1))));
#endif
    RGA2_reg_get_param(base, msg);

    stride = (((msg->src.vir_w * pixel_width) + RGA2_INDEX_TH) & ~RGA2_INDEX_TH) >> RGA2_INDEX_TW;
    uv_stride = ((msg->src.vir_w / xdiv + RGA2_INDEX_TH) & ~RGA2_INDEX_TH);

    /* 10bit code */

    if (src_fmt_yuv400_en == 1) {
        /*
         * When Y400 as the input format, because the current RGA does not support closing
         * the access of the UV channel, the address of the UV channel access is equal to
         * the address of the Y channel access to ensure that the UV channel can access,
         * preventing the RGA hardware from reporting errors.
         */
        *bRGA_SRC_BASE0 = (unsigned int)(msg->src.yrgb_addr + msg->src.y_offset * (stride << RGA2_INDEX_TW) +
                                         msg->src.x_offset * pixel_width);
        *bRGA_SRC_BASE1 = *bRGA_SRC_BASE0;
        *bRGA_SRC_BASE2 = *bRGA_SRC_BASE0;
    } else {
        *bRGA_SRC_BASE0 = (unsigned int)(msg->src.yrgb_addr + msg->src.y_offset * (stride << RGA2_INDEX_TW) +
                                         msg->src.x_offset * pixel_width);
        *bRGA_SRC_BASE1 =
            (unsigned int)(msg->src.uv_addr + (msg->src.y_offset / ydiv) * uv_stride + (msg->src.x_offset / xdiv));
        *bRGA_SRC_BASE2 =
            (unsigned int)(msg->src.v_addr + (msg->src.y_offset / ydiv) * uv_stride + (msg->src.x_offset / xdiv));
    }

    mask_stride = msg->rop_mask_stride;

    *bRGA_SRC_VIR_INFO = stride | (mask_stride << RGA2_INDEX_SIX);

    *bRGA_SRC_ACT_INFO = (msg->src.act_w - 1) | ((msg->src.act_h - 1) << RGA2_INDEX_SIX);

    *bRGA_MASK_ADDR = (unsigned int)msg->rop_mask_addr;

    *bRGA_SRC_INFO = reg;

    *bRGA_SRC_TR_COLOR0 = msg->color_key_min;
    *bRGA_SRC_TR_COLOR1 = msg->color_key_max;
}

static void RGA2_set_reg_dst_info(u8 *base, struct rga2_req *msg)
{
    unsigned int *bRGA_DST_INFO;
    unsigned int *bRGA_DST_BASE0, *bRGA_DST_BASE1, *bRGA_DST_BASE2, *bRGA_SRC_BASE3;
    unsigned int *bRGA_DST_VIR_INFO;
    unsigned int *bRGA_DST_ACT_INFO;

    unsigned int *RGA_DST_Y4MAP_LUT0; // Y4 LUT0
    unsigned int *RGA_DST_Y4MAP_LUT1; // Y4 LUT1
    unsigned int *RGA_DST_NN_QUANTIZE_SCALE;
    unsigned int *RGA_DST_NN_QUANTIZE_OFFSET;

    unsigned int line_width_real;

    unsigned char ydither_en = 0;

    unsigned char src1_format = 0;
    unsigned char src1_rb_swp = 0;
    unsigned char src1_rgb_pack = 0;
    unsigned char src1_alpha_swp = 0;
    unsigned char dst_format = 0;
    unsigned char dst_rb_swp = 0;
    unsigned char dst_rgb_pack = 0;
    unsigned char dst_cbcr_swp = 0;
    unsigned char dst_alpha_swp = 0;

    unsigned char dst_fmt_yuv400_en = 0;
    unsigned char dst_fmt_y4_en = 0;
    unsigned char dst_nn_quantize_en = 0;

    unsigned int reg = 0;
    unsigned char spw, dpw;
    unsigned int s_stride, d_stride;
    unsigned int x_mirr, y_mirr, rot_90_flag;
    unsigned int yrgb_addr, u_addr, v_addr, s_yrgb_addr;
    unsigned int d_uv_stride, x_div, y_div;
    unsigned int y_lt_addr, y_ld_addr, y_rt_addr, y_rd_addr;
    unsigned int u_lt_addr, u_ld_addr, u_rt_addr, u_rd_addr;
    unsigned int v_lt_addr, v_ld_addr, v_rt_addr, v_rd_addr;

    dpw = 1;
    x_div = y_div = 1;

    dst_nn_quantize_en = (msg->alpha_rop_flag >> RGA2_INDEX_EI) & 0x1;

    bRGA_DST_INFO = (unsigned int *)(base + RGA2_DST_INFO_OFFSET);
    bRGA_DST_BASE0 = (unsigned int *)(base + RGA2_DST_BASE0_OFFSET);
    bRGA_DST_BASE1 = (unsigned int *)(base + RGA2_DST_BASE1_OFFSET);
    bRGA_DST_BASE2 = (unsigned int *)(base + RGA2_DST_BASE2_OFFSET);

    bRGA_SRC_BASE3 = (unsigned int *)(base + RGA2_SRC_BASE3_OFFSET);

    bRGA_DST_VIR_INFO = (unsigned int *)(base + RGA2_DST_VIR_INFO_OFFSET);
    bRGA_DST_ACT_INFO = (unsigned int *)(base + RGA2_DST_ACT_INFO_OFFSET);

    RGA_DST_Y4MAP_LUT0 = (unsigned int *)(base + RGA2_DST_Y4MAP_LUT0_OFFSET);
    RGA_DST_Y4MAP_LUT1 = (unsigned int *)(base + RGA2_DST_Y4MAP_LUT1_OFFSET);
    RGA_DST_NN_QUANTIZE_SCALE = (unsigned int *)(base + RGA2_DST_QUANTIZE_SCALE_OFFSET);
    RGA_DST_NN_QUANTIZE_OFFSET = (unsigned int *)(base + RGA2_DST_QUANTIZE_OFFSET_OFFSET);

    switch (msg->src1.format) {
        case RGA2_FORMAT_RGBA_8888:
            src1_format = 0x0;
            spw = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_BGRA_8888:
            src1_format = 0x0;
            src1_rb_swp = 0x1;
            spw = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_RGBX_8888:
            src1_format = 0x1;
            spw = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_BGRX_8888:
            src1_format = 0x1;
            src1_rb_swp = 0x1;
            spw = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_RGB_888:
            src1_format = 0x2;
            src1_rgb_pack = 1;
            spw = RGA2_INDEX_TH;
            break;
        case RGA2_FORMAT_BGR_888:
            src1_format = 0x2;
            src1_rgb_pack = 1;
            src1_rb_swp = 1;
            spw = RGA2_INDEX_TH;
            break;
        case RGA2_FORMAT_RGB_565:
            src1_format = 0x4;
            spw = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_RGBA_5551:
            src1_format = 0x5;
            spw = RGA2_INDEX_TW;
            src1_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_RGBA_4444:
            src1_format = 0x6;
            spw = RGA2_INDEX_TW;
            src1_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_BGR_565:
            src1_format = 0x4;
            spw = RGA2_INDEX_TW;
            src1_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_BGRA_5551:
            src1_format = 0x5;
            spw = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_BGRA_4444:
            src1_format = 0x6;
            spw = RGA2_INDEX_TW;
            break;

        /* ARGB */
        case RGA2_FORMAT_ARGB_8888:
            src1_format = 0x0;
            spw = RGA2_INDEX_FO;
            src1_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_8888:
            src1_format = 0x0;
            spw = RGA2_INDEX_FO;
            src1_alpha_swp = 1;
            src1_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_XRGB_8888:
            src1_format = 0x1;
            spw = RGA2_INDEX_FO;
            src1_alpha_swp = 1;
            break;
        case RGA2_FORMAT_XBGR_8888:
            src1_format = 0x1;
            spw = RGA2_INDEX_FO;
            src1_alpha_swp = 1;
            src1_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_ARGB_5551:
            src1_format = 0x5;
            spw = RGA2_INDEX_TW;
            src1_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_5551:
            src1_format = 0x5;
            spw = RGA2_INDEX_TW;
            src1_alpha_swp = 1;
            src1_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_ARGB_4444:
            src1_format = 0x6;
            spw = RGA2_INDEX_TW;
            src1_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_4444:
            src1_format = 0x6;
            spw = RGA2_INDEX_TW;
            src1_alpha_swp = 1;
            src1_rb_swp = 0x1;
            break;
        default:
            spw = RGA2_INDEX_FO;
            break;
    };

    reg = ((reg & (~m_RGA2_DST_INFO_SW_SRC1_FMT)) | (s_RGA2_DST_INFO_SW_SRC1_FMT(src1_format)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_SRC1_RB_SWP)) | (s_RGA2_DST_INFO_SW_SRC1_RB_SWP(src1_rb_swp)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_SRC1_ALPHA_SWP)) | (s_RGA2_DST_INFO_SW_SRC1_ALPHA_SWP(src1_alpha_swp)));

    switch (msg->dst.format) {
        case RGA2_FORMAT_RGBA_8888:
            dst_format = 0x0;
            dpw = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_BGRA_8888:
            dst_format = 0x0;
            dst_rb_swp = 0x1;
            dpw = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_RGBX_8888:
            dst_format = 0x1;
            dpw = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_BGRX_8888:
            dst_format = 0x1;
            dst_rb_swp = 0x1;
            dpw = RGA2_INDEX_FO;
            break;
        case RGA2_FORMAT_RGB_888:
            dst_format = 0x2;
            dst_rgb_pack = 1;
            dpw = RGA2_INDEX_TH;
            break;
        case RGA2_FORMAT_BGR_888:
            dst_format = 0x2;
            dst_rgb_pack = 1;
            dst_rb_swp = 1;
            dpw = RGA2_INDEX_TH;
            break;
        case RGA2_FORMAT_RGB_565:
            dst_format = 0x4;
            dpw = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_RGBA_5551:
            dst_format = 0x5;
            dpw = RGA2_INDEX_TW;
            dst_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_RGBA_4444:
            dst_format = 0x6;
            dpw = RGA2_INDEX_TW;
            dst_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_BGR_565:
            dst_format = 0x4;
            dpw = RGA2_INDEX_TW;
            dst_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_BGRA_5551:
            dst_format = 0x5;
            dpw = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_BGRA_4444:
            dst_format = 0x6;
            dpw = RGA2_INDEX_TW;
            break;

        /* ARGB */
        case RGA2_FORMAT_ARGB_8888:
            dst_format = 0x0;
            dpw = RGA2_INDEX_FO;
            dst_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_8888:
            dst_format = 0x0;
            dpw = RGA2_INDEX_FO;
            dst_alpha_swp = 1;
            dst_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_XRGB_8888:
            dst_format = 0x1;
            dpw = RGA2_INDEX_FO;
            dst_alpha_swp = 1;
            break;
        case RGA2_FORMAT_XBGR_8888:
            dst_format = 0x1;
            dpw = RGA2_INDEX_FO;
            dst_alpha_swp = 1;
            dst_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_ARGB_5551:
            dst_format = 0x5;
            dpw = RGA2_INDEX_TW;
            dst_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_5551:
            dst_format = 0x5;
            dpw = RGA2_INDEX_TW;
            dst_alpha_swp = 1;
            dst_rb_swp = 0x1;
            break;
        case RGA2_FORMAT_ARGB_4444:
            dst_format = 0x6;
            dpw = RGA2_INDEX_TW;
            dst_alpha_swp = 1;
            break;
        case RGA2_FORMAT_ABGR_4444:
            dst_format = 0x6;
            dpw = RGA2_INDEX_TW;
            dst_alpha_swp = 1;
            dst_rb_swp = 0x1;
            break;

        case RGA2_FORMAT_YCbCr_422_SP:
            dst_format = 0x8;
            x_div = 1;
            y_div = 1;
            break;
        case RGA2_FORMAT_YCbCr_422_P:
            dst_format = 0x9;
            x_div = RGA2_INDEX_TW;
            y_div = 1;
            break;
        case RGA2_FORMAT_YCbCr_420_SP:
            dst_format = 0xa;
            x_div = 1;
            y_div = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_YCbCr_420_P:
            dst_format = 0xb;
            dst_cbcr_swp = 1;
            x_div = RGA2_INDEX_TW;
            y_div = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_YCrCb_422_SP:
            dst_format = 0x8;
            dst_cbcr_swp = 1;
            x_div = 1;
            y_div = 1;
            break;
        case RGA2_FORMAT_YCrCb_422_P:
            dst_format = 0x9;
            dst_cbcr_swp = 1;
            x_div = RGA2_INDEX_TW;
            y_div = 1;
            break;
        case RGA2_FORMAT_YCrCb_420_SP:
            dst_format = 0xa;
            dst_cbcr_swp = 1;
            x_div = 1;
            y_div = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_YCrCb_420_P:
            dst_format = 0xb;
            x_div = RGA2_INDEX_TW;
            y_div = RGA2_INDEX_TW;
            break;

        case RGA2_FORMAT_YCbCr_400:
            dst_format = 0x8;
            dst_fmt_yuv400_en = 1;
            x_div = 1;
            y_div = 1;
            break;
        case RGA2_FORMAT_Y4:
            dst_format = 0x8;
            dst_fmt_y4_en = 1;
            dst_fmt_yuv400_en = 1;
            x_div = 1;
            y_div = 1;
            break;

        case RGA2_FORMAT_YUYV_422:
            dst_format = 0xe;
            dpw = RGA2_INDEX_TW;
            dst_cbcr_swp = 1;
            break;
        case RGA2_FORMAT_YVYU_422:
            dst_format = 0xe;
            dpw = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_YUYV_420:
            dst_format = 0xf;
            dpw = RGA2_INDEX_TW;
            dst_cbcr_swp = 1;
            break;
        case RGA2_FORMAT_YVYU_420:
            dst_format = 0xf;
            dpw = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_UYVY_422:
            dst_format = 0xc;
            dpw = RGA2_INDEX_TW;
            dst_cbcr_swp = 1;
            break;
        case RGA2_FORMAT_VYUY_422:
            dst_format = 0xc;
            dpw = RGA2_INDEX_TW;
            break;
        case RGA2_FORMAT_UYVY_420:
            dst_format = 0xd;
            dpw = RGA2_INDEX_TW;
            dst_cbcr_swp = 1;
            break;
        case RGA2_FORMAT_VYUY_420:
            dst_format = 0xd;
            dpw = RGA2_INDEX_TW;
            break;
        default:
            break;
    };

    reg = ((reg & (~m_RGA2_DST_INFO_SW_DST_FMT)) | (s_RGA2_DST_INFO_SW_DST_FMT(dst_format)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DST_RB_SWAP)) | (s_RGA2_DST_INFO_SW_DST_RB_SWAP(dst_rb_swp)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_ALPHA_SWAP)) | (s_RGA2_DST_INFO_SW_ALPHA_SWAP(dst_alpha_swp)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DST_UV_SWAP)) | (s_RGA2_DST_INFO_SW_DST_UV_SWAP(dst_cbcr_swp)));

    reg = ((reg & (~m_RGA2_DST_INFO_SW_DST_FMT_YUV400_EN)) | (s_RGA2_DST_INFO_SW_DST_FMT_YUV400_EN(dst_fmt_yuv400_en)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DST_FMT_Y4_EN)) | (s_RGA2_DST_INFO_SW_DST_FMT_Y4_EN(dst_fmt_y4_en)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DST_NN_QUANTIZE_EN)) |
           (s_RGA2_DST_INFO_SW_DST_NN_QUANTIZE_EN(dst_nn_quantize_en)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DITHER_UP_E)) |
           (s_RGA2_DST_INFO_SW_DITHER_UP_E(msg->alpha_rop_flag >> RGA2_INDEX_FI)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DITHER_DOWN_E)) |
           (s_RGA2_DST_INFO_SW_DITHER_DOWN_E(msg->alpha_rop_flag >> RGA2_INDEX_SI)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DITHER_MODE)) | (s_RGA2_DST_INFO_SW_DITHER_MODE(msg->dither_mode)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DST_CSC_MODE)) |
           (s_RGA2_DST_INFO_SW_DST_CSC_MODE(msg->yuv2rgb_mode >> RGA2_INDEX_TW)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_CSC_CLIP_MODE)) |
           (s_RGA2_DST_INFO_SW_CSC_CLIP_MODE(msg->yuv2rgb_mode >> RGA2_INDEX_FO)));
    /* full csc enable */
    reg = ((reg & (~m_RGA2_DST_INFO_SW_DST_CSC_MODE_2)) | (s_RGA2_DST_INFO_SW_DST_CSC_MODE_2(msg->full_csc.flag)));
    /* Some older chips do not support src1 csc mode, they do not have these two registers. */
    reg = ((reg & (~m_RGA2_DST_INFO_SW_SRC1_CSC_MODE)) |
           (s_RGA2_DST_INFO_SW_SRC1_CSC_MODE(msg->yuv2rgb_mode >> RGA2_INDEX_FI)));
    reg = ((reg & (~m_RGA2_DST_INFO_SW_SRC1_CSC_CLIP_MODE)) |
           (s_RGA2_DST_INFO_SW_SRC1_CSC_CLIP_MODE(msg->yuv2rgb_mode >> RGA2_INDEX_SE)));

    ydither_en = (msg->dst.format == RGA2_FORMAT_Y4) && ((msg->alpha_rop_flag >> RGA2_INDEX_SI) & 0x1);

    *bRGA_DST_INFO = reg;

    s_stride = ((msg->src1.vir_w * spw + RGA2_INDEX_TH) & ~RGA2_INDEX_TH) >> RGA2_INDEX_TW;
    d_stride = ((msg->dst.vir_w * dpw + RGA2_INDEX_TH) & ~RGA2_INDEX_TH) >> RGA2_INDEX_TW;

    if (dst_fmt_y4_en) {
        /* Y4 output will HALF */
        d_stride = ((d_stride + 1) & ~1) >> 1;
    }

    d_uv_stride = (d_stride << RGA2_INDEX_TW) / x_div;

    *bRGA_DST_VIR_INFO = d_stride | (s_stride << RGA2_INDEX_SIX);
    if ((msg->dst.vir_w % RGA2_INDEX_TW != 0) && (msg->dst.act_w == msg->src.act_w) &&
        (msg->dst.act_h == msg->src.act_h) &&
        (msg->dst.format == RGA2_FORMAT_BGR_888 || msg->dst.format == RGA2_FORMAT_RGB_888)) {
        *bRGA_DST_ACT_INFO = (msg->dst.act_w) | ((msg->dst.act_h - 1) << RGA2_INDEX_SIX);
    } else {
        *bRGA_DST_ACT_INFO = (msg->dst.act_w - 1) | ((msg->dst.act_h - 1) << RGA2_INDEX_SIX);
    }
    s_stride <<= RGA2_INDEX_TW;
    d_stride <<= RGA2_INDEX_TW;

    if (((msg->rotate_mode & 0xf) == 0) || ((msg->rotate_mode & 0xf) == 1)) {
        x_mirr = 0;
        y_mirr = 0;
    } else {
        x_mirr = 1;
        y_mirr = 1;
    }

    rot_90_flag = msg->rotate_mode & 1;
    x_mirr = (x_mirr + ((msg->rotate_mode >> RGA2_INDEX_FO) & 1)) & 1;
    y_mirr = (y_mirr + ((msg->rotate_mode >> RGA2_INDEX_FI) & 1)) & 1;

    if (ydither_en) {
        if (x_mirr && y_mirr) {
            printk(KERN_ERR "rga: [ERROR] YDITHER MODE DO NOT SUPPORT ROTATION !!x_mirr=%d,y_mirr=%d \n", x_mirr,
                   y_mirr);
        }
        if (msg->dst.act_w != msg->src.act_w) {
            printk(KERN_ERR "rga: [ERROR] YDITHER MODE DO NOT SUPPORT SCL !!src0.act_w=%d,dst.act_w=%d \n",
                   msg->src.act_w, msg->dst.act_w);
        }
        if (msg->dst.act_h != msg->src.act_h) {
            printk(KERN_ERR "rga: [ERROR] YDITHER MODE DO NOT SUPPORT SCL !!src0.act_h=%d,dst.act_h=%d \n",
                   msg->src.act_h, msg->dst.act_h);
        }
    }

    if (dst_fmt_y4_en) {
        *RGA_DST_Y4MAP_LUT0 = (msg->gr_color.gr_x_r & 0xffff) | (msg->gr_color.gr_x_g << RGA2_INDEX_SIX);
        *RGA_DST_Y4MAP_LUT1 = (msg->gr_color.gr_y_r & 0xffff) | (msg->gr_color.gr_y_g << RGA2_INDEX_SIX);
    }

    if (dst_nn_quantize_en) {
        *RGA_DST_NN_QUANTIZE_SCALE = (msg->gr_color.gr_x_r & 0xffff) | (msg->gr_color.gr_x_g << RGA2_INDEX_TE) |
                                     (msg->gr_color.gr_x_b << RGA2_INDEX_TWEN);
        *RGA_DST_NN_QUANTIZE_OFFSET = (msg->gr_color.gr_y_r & 0xffff) | (msg->gr_color.gr_y_g << RGA2_INDEX_TE) |
                                      (msg->gr_color.gr_y_b << RGA2_INDEX_TWEN);
    }

    s_yrgb_addr = (unsigned int)msg->src1.yrgb_addr + (msg->src1.y_offset * s_stride) + (msg->src1.x_offset * spw);

    *bRGA_SRC_BASE3 = s_yrgb_addr;

    if (dst_fmt_y4_en) {
        yrgb_addr =
            (unsigned int)msg->dst.yrgb_addr + (msg->dst.y_offset * d_stride) + ((msg->dst.x_offset * dpw) >> 1);
    } else {
        yrgb_addr = (unsigned int)msg->dst.yrgb_addr + (msg->dst.y_offset * d_stride) + (msg->dst.x_offset * dpw);
    }
    u_addr = (unsigned int)msg->dst.uv_addr + (msg->dst.y_offset / y_div) * d_uv_stride + msg->dst.x_offset / x_div;
    v_addr = (unsigned int)msg->dst.v_addr + (msg->dst.y_offset / y_div) * d_uv_stride + msg->dst.x_offset / x_div;

    y_lt_addr = yrgb_addr;
    u_lt_addr = u_addr;
    v_lt_addr = v_addr;

    /* Warning */
    line_width_real = dst_fmt_y4_en ? ((msg->dst.act_w) >> 1) : msg->dst.act_w;

    if (msg->dst.format < 0x18 ||
        (msg->dst.format >= RGA2_FORMAT_ARGB_8888 && msg->dst.format <= RGA2_FORMAT_ABGR_4444)) {
        /* 270 degree & Mirror V */
        y_ld_addr = yrgb_addr + (msg->dst.act_h - 1) * (d_stride);
        /* 90 degree & Mirror H  */
        y_rt_addr = yrgb_addr + (line_width_real - 1) * dpw;
        /* 180 degree */
        y_rd_addr = y_ld_addr + (line_width_real - 1) * dpw;
    } else {
        if (msg->dst.format == RGA2_FORMAT_YUYV_422 || msg->dst.format == RGA2_FORMAT_YVYU_422 ||
            msg->dst.format == RGA2_FORMAT_UYVY_422 || msg->dst.format == RGA2_FORMAT_VYUY_422) {
            y_ld_addr = yrgb_addr + (msg->dst.act_h - 1) * (d_stride);
            y_rt_addr = yrgb_addr + (msg->dst.act_w * RGA2_INDEX_TW - 1);
            y_rd_addr = y_ld_addr + (msg->dst.act_w * RGA2_INDEX_TW - 1);
        } else {
            y_ld_addr = (unsigned int)msg->dst.yrgb_addr + ((msg->dst.y_offset + (msg->dst.act_h - 1)) * d_stride) +
                        msg->dst.x_offset;
            y_rt_addr = yrgb_addr + (msg->dst.act_w * RGA2_INDEX_TW - 1);
            y_rd_addr = y_ld_addr + (msg->dst.act_w - 1);
        }
    }

    u_ld_addr = u_addr + ((msg->dst.act_h / y_div) - 1) * (d_uv_stride);
    v_ld_addr = v_addr + ((msg->dst.act_h / y_div) - 1) * (d_uv_stride);

    u_rt_addr = u_addr + (msg->dst.act_w / x_div) - 1;
    v_rt_addr = v_addr + (msg->dst.act_w / x_div) - 1;

    u_rd_addr = u_ld_addr + (msg->dst.act_w / x_div) - 1;
    v_rd_addr = v_ld_addr + (msg->dst.act_w / x_div) - 1;

    if (rot_90_flag == 0) {
        if (y_mirr == 1) {
            if (x_mirr == 1) {
                yrgb_addr = y_rd_addr;
                u_addr = u_rd_addr;
                v_addr = v_rd_addr;
            } else {
                yrgb_addr = y_ld_addr;
                u_addr = u_ld_addr;
                v_addr = v_ld_addr;
            }
        } else {
            if (x_mirr == 1) {
                yrgb_addr = y_rt_addr;
                u_addr = u_rt_addr;
                v_addr = v_rt_addr;
            } else {
                yrgb_addr = y_lt_addr;
                u_addr = u_lt_addr;
                v_addr = v_lt_addr;
            }
        }
    } else {
        if (y_mirr == 1) {
            if (x_mirr == 1) {
                yrgb_addr = y_ld_addr;
                u_addr = u_ld_addr;
                v_addr = v_ld_addr;
            } else {
                yrgb_addr = y_rd_addr;
                u_addr = u_rd_addr;
                v_addr = v_rd_addr;
            }
        } else {
            if (x_mirr == 1) {
                yrgb_addr = y_lt_addr;
                u_addr = u_lt_addr;
                v_addr = v_lt_addr;
            } else {
                yrgb_addr = y_rt_addr;
                u_addr = u_rt_addr;
                v_addr = v_rt_addr;
            }
        }
    }

    *bRGA_DST_BASE0 = (unsigned int)yrgb_addr;

    if ((msg->dst.format == RGA2_FORMAT_YCbCr_420_P) || (msg->dst.format == RGA2_FORMAT_YCrCb_420_P)) {
        if (dst_cbcr_swp == 0) {
            *bRGA_DST_BASE1 = (unsigned int)v_addr;
            *bRGA_DST_BASE2 = (unsigned int)u_addr;
        } else {
            *bRGA_DST_BASE1 = (unsigned int)u_addr;
            *bRGA_DST_BASE2 = (unsigned int)v_addr;
        }
    } else {
        *bRGA_DST_BASE1 = (unsigned int)u_addr;
        *bRGA_DST_BASE2 = (unsigned int)v_addr;
    }
}

static void RGA2_set_reg_alpha_info(u8 *base, struct rga2_req *msg)
{
    unsigned int *bRGA_ALPHA_CTRL0;
    unsigned int *bRGA_ALPHA_CTRL1;
    unsigned int *bRGA_FADING_CTRL;
    unsigned int reg0 = 0;
    unsigned int reg1 = 0;

    bRGA_ALPHA_CTRL0 = (unsigned int *)(base + RGA2_ALPHA_CTRL0_OFFSET);
    bRGA_ALPHA_CTRL1 = (unsigned int *)(base + RGA2_ALPHA_CTRL1_OFFSET);
    bRGA_FADING_CTRL = (unsigned int *)(base + RGA2_FADING_CTRL_OFFSET);

    reg0 = ((reg0 & (~m_RGA2_ALPHA_CTRL0_SW_ALPHA_ROP_0)) | (s_RGA2_ALPHA_CTRL0_SW_ALPHA_ROP_0(msg->alpha_rop_flag)));
    reg0 = ((reg0 & (~m_RGA2_ALPHA_CTRL0_SW_ALPHA_ROP_SEL)) |
            (s_RGA2_ALPHA_CTRL0_SW_ALPHA_ROP_SEL(msg->alpha_rop_flag >> 1)));
    reg0 = ((reg0 & (~m_RGA2_ALPHA_CTRL0_SW_ROP_MODE)) | (s_RGA2_ALPHA_CTRL0_SW_ROP_MODE(msg->rop_mode)));
    reg0 = ((reg0 & (~m_RGA2_ALPHA_CTRL0_SW_SRC_GLOBAL_ALPHA)) |
            (s_RGA2_ALPHA_CTRL0_SW_SRC_GLOBAL_ALPHA(msg->src_a_global_val)));
    reg0 = ((reg0 & (~m_RGA2_ALPHA_CTRL0_SW_DST_GLOBAL_ALPHA)) |
            (s_RGA2_ALPHA_CTRL0_SW_DST_GLOBAL_ALPHA(msg->dst_a_global_val)));

    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_COLOR_M0)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_COLOR_M0(msg->alpha_mode_0 >> RGA2_INDEX_FIF)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_COLOR_M0)) |
            (s_RGA2_ALPHA_CTRL1_SW_SRC_COLOR_M0(msg->alpha_mode_0 >> RGA2_INDEX_SE)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_FACTOR_M0)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_FACTOR_M0(msg->alpha_mode_0 >> RGA2_INDEX_TWE)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_FACTOR_M0)) |
            (s_RGA2_ALPHA_CTRL1_SW_SRC_FACTOR_M0(msg->alpha_mode_0 >> RGA2_INDEX_FO)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_ALPHA_CAL_M0)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_ALPHA_CAL_M0(msg->alpha_mode_0 >> RGA2_INDEX_EIE)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_ALPHA_CAL_M0)) |
            (s_RGA2_ALPHA_CTRL1_SW_SRC_ALPHA_CAL_M0(msg->alpha_mode_0 >> RGA2_INDEX_TH)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_BLEND_M0)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_BLEND_M0(msg->alpha_mode_0 >> RGA2_INDEX_NI)));
    reg1 =
        ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_BLEND_M0)) | (s_RGA2_ALPHA_CTRL1_SW_SRC_BLEND_M0(msg->alpha_mode_0 >> 1)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_ALPHA_M0)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_ALPHA_M0(msg->alpha_mode_0 >> RGA2_INDEX_EI)));
    reg1 =
        ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_ALPHA_M0)) | (s_RGA2_ALPHA_CTRL1_SW_SRC_ALPHA_M0(msg->alpha_mode_0 >> 0)));

    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_FACTOR_M1)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_FACTOR_M1(msg->alpha_mode_1 >> RGA2_INDEX_TWE)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_FACTOR_M1)) |
            (s_RGA2_ALPHA_CTRL1_SW_SRC_FACTOR_M1(msg->alpha_mode_1 >> RGA2_INDEX_FO)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_ALPHA_CAL_M1)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_ALPHA_CAL_M1(msg->alpha_mode_1 >> RGA2_INDEX_EIE)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_ALPHA_CAL_M1)) |
            (s_RGA2_ALPHA_CTRL1_SW_SRC_ALPHA_CAL_M1(msg->alpha_mode_1 >> RGA2_INDEX_TH)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_BLEND_M1)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_BLEND_M1(msg->alpha_mode_1 >> RGA2_INDEX_NI)));
    reg1 =
        ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_BLEND_M1)) | (s_RGA2_ALPHA_CTRL1_SW_SRC_BLEND_M1(msg->alpha_mode_1 >> 1)));
    reg1 = ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_DST_ALPHA_M1)) |
            (s_RGA2_ALPHA_CTRL1_SW_DST_ALPHA_M1(msg->alpha_mode_1 >> RGA2_INDEX_EI)));
    reg1 =
        ((reg1 & (~m_RGA2_ALPHA_CTRL1_SW_SRC_ALPHA_M1)) | (s_RGA2_ALPHA_CTRL1_SW_SRC_ALPHA_M1(msg->alpha_mode_1 >> 0)));

    *bRGA_ALPHA_CTRL0 = reg0;
    *bRGA_ALPHA_CTRL1 = reg1;

    if ((msg->alpha_rop_flag >> RGA2_INDEX_TW) & 1) {
        *bRGA_FADING_CTRL = (1 << RGA2_INDEX_TWEF) | (msg->fading_b_value << RGA2_INDEX_SIX) |
                            (msg->fading_g_value << RGA2_INDEX_EI) | (msg->fading_r_value);
    }
}

static void RGA2_set_reg_rop_info(u8 *base, struct rga2_req *msg)
{
    unsigned int *bRGA_ALPHA_CTRL0;
    unsigned int *bRGA_ROP_CTRL0;
    unsigned int *bRGA_ROP_CTRL1;
    unsigned int *bRGA_MASK_ADDR;
    unsigned int *bRGA_FG_COLOR;
    unsigned int *bRGA_PAT_CON;

    unsigned int rop_code0 = 0;
    unsigned int rop_code1 = 0;

    bRGA_ALPHA_CTRL0 = (unsigned int *)(base + RGA2_ALPHA_CTRL0_OFFSET);
    bRGA_ROP_CTRL0 = (unsigned int *)(base + RGA2_ROP_CTRL0_OFFSET);
    bRGA_ROP_CTRL1 = (unsigned int *)(base + RGA2_ROP_CTRL1_OFFSET);
    bRGA_MASK_ADDR = (unsigned int *)(base + RGA2_MASK_BASE_OFFSET);
    bRGA_FG_COLOR = (unsigned int *)(base + RGA2_SRC_FG_COLOR_OFFSET);
    bRGA_PAT_CON = (unsigned int *)(base + RGA2_PAT_CON_OFFSET);

    if (msg->rop_mode == 0) {
        rop_code0 = RGA2_ROP3_code[(msg->rop_code & 0xff)];
    } else if (msg->rop_mode == 1) {
        rop_code0 = RGA2_ROP3_code[(msg->rop_code & 0xff)];
    } else if (msg->rop_mode == RGA2_INDEX_TW) {
        rop_code0 = RGA2_ROP3_code[(msg->rop_code & 0xff)];
        rop_code1 = RGA2_ROP3_code[(msg->rop_code & 0xff00) >> RGA2_INDEX_EI];
    }

    *bRGA_ROP_CTRL0 = rop_code0;
    *bRGA_ROP_CTRL1 = rop_code1;
    *bRGA_FG_COLOR = msg->fg_color;
    *bRGA_MASK_ADDR = (unsigned int)msg->rop_mask_addr;
    *bRGA_PAT_CON = (msg->pat.act_w - 1) | ((msg->pat.act_h - 1) << RGA2_INDEX_EI) |
                    (msg->pat.x_offset << RGA2_INDEX_SIX) | (msg->pat.y_offset << RGA2_INDEX_TWEF);
    *bRGA_ALPHA_CTRL0 = *bRGA_ALPHA_CTRL0 | (((msg->endian_mode >> 1) & 1) << RGA2_INDEX_TWEN);
}

static void RGA2_set_reg_full_csc(u8 *base, struct rga2_req *msg)
{
    unsigned int *bRGA2_DST_CSC_00;
    unsigned int *bRGA2_DST_CSC_01;
    unsigned int *bRGA2_DST_CSC_02;
    unsigned int *bRGA2_DST_CSC_OFF0;

    unsigned int *bRGA2_DST_CSC_10;
    unsigned int *bRGA2_DST_CSC_11;
    unsigned int *bRGA2_DST_CSC_12;
    unsigned int *bRGA2_DST_CSC_OFF1;

    unsigned int *bRGA2_DST_CSC_20;
    unsigned int *bRGA2_DST_CSC_21;
    unsigned int *bRGA2_DST_CSC_22;
    unsigned int *bRGA2_DST_CSC_OFF2;

    bRGA2_DST_CSC_00 = (unsigned int *)(base + RGA2_DST_CSC_00_OFFSET);
    bRGA2_DST_CSC_01 = (unsigned int *)(base + RGA2_DST_CSC_01_OFFSET);
    bRGA2_DST_CSC_02 = (unsigned int *)(base + RGA2_DST_CSC_02_OFFSET);
    bRGA2_DST_CSC_OFF0 = (unsigned int *)(base + RGA2_DST_CSC_OFF0_OFFSET);

    bRGA2_DST_CSC_10 = (unsigned int *)(base + RGA2_DST_CSC_10_OFFSET);
    bRGA2_DST_CSC_11 = (unsigned int *)(base + RGA2_DST_CSC_11_OFFSET);
    bRGA2_DST_CSC_12 = (unsigned int *)(base + RGA2_DST_CSC_12_OFFSET);
    bRGA2_DST_CSC_OFF1 = (unsigned int *)(base + RGA2_DST_CSC_OFF1_OFFSET);

    bRGA2_DST_CSC_20 = (unsigned int *)(base + RGA2_DST_CSC_20_OFFSET);
    bRGA2_DST_CSC_21 = (unsigned int *)(base + RGA2_DST_CSC_21_OFFSET);
    bRGA2_DST_CSC_22 = (unsigned int *)(base + RGA2_DST_CSC_22_OFFSET);
    bRGA2_DST_CSC_OFF2 = (unsigned int *)(base + RGA2_DST_CSC_OFF2_OFFSET);

    /* full csc coefficient */
    /* Y coefficient */
    *bRGA2_DST_CSC_00 = msg->full_csc.coe_y.r_v;
    *bRGA2_DST_CSC_01 = msg->full_csc.coe_y.g_y;
    *bRGA2_DST_CSC_02 = msg->full_csc.coe_y.b_u;
    *bRGA2_DST_CSC_OFF0 = msg->full_csc.coe_y.off;
    /* U coefficient */
    *bRGA2_DST_CSC_10 = msg->full_csc.coe_u.r_v;
    *bRGA2_DST_CSC_11 = msg->full_csc.coe_u.g_y;
    *bRGA2_DST_CSC_12 = msg->full_csc.coe_u.b_u;
    *bRGA2_DST_CSC_OFF1 = msg->full_csc.coe_u.off;
    /* V coefficient */
    *bRGA2_DST_CSC_20 = msg->full_csc.coe_v.r_v;
    *bRGA2_DST_CSC_21 = msg->full_csc.coe_v.g_y;
    *bRGA2_DST_CSC_22 = msg->full_csc.coe_v.b_u;
    *bRGA2_DST_CSC_OFF2 = msg->full_csc.coe_v.off;
}

static void RGA2_set_reg_color_palette(unsigned char *base, struct rga2_req *msg)
{
    unsigned int *bRGA_SRC_BASE0, *bRGA_SRC_INFO, *bRGA_SRC_VIR_INFO, *bRGA_SRC_ACT_INFO, *bRGA_SRC_FG_COLOR,
        *bRGA_SRC_BG_COLOR;
    unsigned int *p;
    short x_off, y_off;
    unsigned short src_stride;
    unsigned char shift;
    unsigned int sw;
    unsigned int byte_num;
    unsigned int reg;

    bRGA_SRC_BASE0 = (unsigned int *)(base + RGA2_SRC_BASE0_OFFSET);
    bRGA_SRC_INFO = (unsigned int *)(base + RGA2_SRC_INFO_OFFSET);
    bRGA_SRC_VIR_INFO = (unsigned int *)(base + RGA2_SRC_VIR_INFO_OFFSET);
    bRGA_SRC_ACT_INFO = (unsigned int *)(base + RGA2_SRC_ACT_INFO_OFFSET);
    bRGA_SRC_FG_COLOR = (unsigned int *)(base + RGA2_SRC_FG_COLOR_OFFSET);
    bRGA_SRC_BG_COLOR = (unsigned int *)(base + RGA2_SRC_BG_COLOR_OFFSET);

    reg = 0;

    shift = RGA2_INDEX_TH - msg->palette_mode;

    x_off = msg->src.x_offset;
    y_off = msg->src.y_offset;

    sw = msg->src.vir_w;
    byte_num = sw >> shift;

    src_stride = (byte_num + RGA2_INDEX_TH) & (~RGA2_INDEX_TH);

    p = (unsigned int *)((unsigned long)msg->src.yrgb_addr);

    p = p + (x_off >> shift) + y_off * src_stride;

    *bRGA_SRC_BASE0 = (unsigned long)p;

    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SRC_FMT)) | (s_RGA2_SRC_INFO_SW_SRC_FMT((msg->palette_mode | 0xc))));
    reg = ((reg & (~m_RGA2_SRC_INFO_SW_SW_CP_ENDAIN)) | (s_RGA2_SRC_INFO_SW_SW_CP_ENDAIN(msg->endian_mode & 1)));
    *bRGA_SRC_VIR_INFO = src_stride >> RGA2_INDEX_TW;
    *bRGA_SRC_ACT_INFO = (msg->src.act_w - 1) | ((msg->src.act_h - 1) << RGA2_INDEX_SIX);
    *bRGA_SRC_INFO = reg;

    *bRGA_SRC_FG_COLOR = msg->fg_color;
    *bRGA_SRC_BG_COLOR = msg->bg_color;
}

static void RGA2_set_reg_color_fill(u8 *base, struct rga2_req *msg)
{
    unsigned int *bRGA_CF_GR_A;
    unsigned int *bRGA_CF_GR_B;
    unsigned int *bRGA_CF_GR_G;
    unsigned int *bRGA_CF_GR_R;
    unsigned int *bRGA_SRC_FG_COLOR;
    unsigned int *bRGA_MASK_ADDR;
    unsigned int *bRGA_PAT_CON;

    unsigned int mask_stride;
    unsigned int *bRGA_SRC_VIR_INFO;

    bRGA_SRC_FG_COLOR = (unsigned int *)(base + RGA2_SRC_FG_COLOR_OFFSET);

    bRGA_CF_GR_A = (unsigned int *)(base + RGA2_CF_GR_A_OFFSET);
    bRGA_CF_GR_B = (unsigned int *)(base + RGA2_CF_GR_B_OFFSET);
    bRGA_CF_GR_G = (unsigned int *)(base + RGA2_CF_GR_G_OFFSET);
    bRGA_CF_GR_R = (unsigned int *)(base + RGA2_CF_GR_R_OFFSET);

    bRGA_MASK_ADDR = (unsigned int *)(base + RGA2_MASK_BASE_OFFSET);
    bRGA_PAT_CON = (unsigned int *)(base + RGA2_PAT_CON_OFFSET);

    bRGA_SRC_VIR_INFO = (unsigned int *)(base + RGA2_SRC_VIR_INFO_OFFSET);

    mask_stride = msg->rop_mask_stride;

    if (msg->color_fill_mode == 0) {
        /* solid color */
        *bRGA_CF_GR_A = (msg->gr_color.gr_x_a & 0xffff) | (msg->gr_color.gr_y_a << RGA2_INDEX_SIX);
        *bRGA_CF_GR_B = (msg->gr_color.gr_x_b & 0xffff) | (msg->gr_color.gr_y_b << RGA2_INDEX_SIX);
        *bRGA_CF_GR_G = (msg->gr_color.gr_x_g & 0xffff) | (msg->gr_color.gr_y_g << RGA2_INDEX_SIX);
        *bRGA_CF_GR_R = (msg->gr_color.gr_x_r & 0xffff) | (msg->gr_color.gr_y_r << RGA2_INDEX_SIX);

        *bRGA_SRC_FG_COLOR = msg->fg_color;
    } else {
        /* patten color */
        *bRGA_MASK_ADDR = (unsigned int)msg->pat.yrgb_addr;
        *bRGA_PAT_CON = (msg->pat.act_w - 1) | ((msg->pat.act_h - 1) << RGA2_INDEX_EI) |
                        (msg->pat.x_offset << RGA2_INDEX_SIX) | (msg->pat.y_offset << RGA2_INDEX_TWEF);
    }
    *bRGA_SRC_VIR_INFO = mask_stride << RGA2_INDEX_SIX;
}

static void RGA2_set_reg_update_palette_table(unsigned char *base, struct rga2_req *msg)
{
    unsigned int *bRGA_MASK_BASE;
    unsigned int *bRGA_FADING_CTRL;

    bRGA_MASK_BASE = (unsigned int *)(base + RGA2_MASK_BASE_OFFSET);
    bRGA_FADING_CTRL = (unsigned int *)(base + RGA2_FADING_CTRL_OFFSET);

    *bRGA_FADING_CTRL = msg->fading_g_value << RGA2_INDEX_EI;
    *bRGA_MASK_BASE = (unsigned int)msg->pat.yrgb_addr;
}

static void RGA2_set_reg_update_patten_buff(unsigned char *base, struct rga2_req *msg)
{
    u32 *bRGA_PAT_MST;
    u32 *bRGA_PAT_CON;
    u32 *bRGA_PAT_START_POINT;
    unsigned int *bRGA_FADING_CTRL;
    u32 reg = 0;
    rga_img_info_t *pat;

    unsigned int num, offset;

    pat = &msg->pat;

    num = (pat->act_w * pat->act_h) - 1;

    offset = pat->act_w * pat->y_offset + pat->x_offset;

    bRGA_PAT_START_POINT = (unsigned int *)(base + RGA2_FADING_CTRL_OFFSET);
    bRGA_PAT_MST = (unsigned int *)(base + RGA2_MASK_BASE_OFFSET);
    bRGA_PAT_CON = (unsigned int *)(base + RGA2_PAT_CON_OFFSET);
    bRGA_FADING_CTRL = (unsigned int *)(base + RGA2_FADING_CTRL_OFFSET);

    *bRGA_PAT_MST = (unsigned int)msg->pat.yrgb_addr;
    *bRGA_PAT_START_POINT = (pat->act_w * pat->y_offset) + pat->x_offset;

    reg = (pat->act_w - 1) | ((pat->act_h - 1) << RGA2_INDEX_EI) | (pat->x_offset << RGA2_INDEX_SIX) |
          (pat->y_offset << RGA2_INDEX_TWEF);
    *bRGA_PAT_CON = reg;

    *bRGA_FADING_CTRL = (num << RGA2_INDEX_EI) | offset;
}

static void RGA2_set_pat_info(unsigned char *base, struct rga2_req *msg)
{
    u32 *bRGA_PAT_CON;
    u32 *bRGA_FADING_CTRL;
    u32 reg = 0;
    rga_img_info_t *pat;

    unsigned int num, offset;

    pat = &msg->pat;

    num = ((pat->act_w * pat->act_h) - 1) & 0xff;

    offset = (pat->act_w * pat->y_offset) + pat->x_offset;

    bRGA_PAT_CON = (unsigned int *)(base + RGA2_PAT_CON_OFFSET);
    bRGA_FADING_CTRL = (unsigned int *)(base + RGA2_FADING_CTRL_OFFSET);

    reg = (pat->act_w - 1) | ((pat->act_h - 1) << RGA2_INDEX_EI) | (pat->x_offset << RGA2_INDEX_SIX) |
          (pat->y_offset << RGA2_INDEX_TWEF);
    *bRGA_PAT_CON = reg;
    *bRGA_FADING_CTRL = (num << RGA2_INDEX_EI) | offset;
}

static void RGA2_set_mmu_info(unsigned char *base, struct rga2_req *msg)
{
    unsigned int *bRGA_MMU_CTRL1;
    unsigned int *bRGA_MMU_SRC_BASE;
    unsigned int *bRGA_MMU_SRC1_BASE;
    unsigned int *bRGA_MMU_DST_BASE;
    unsigned int *bRGA_MMU_ELS_BASE;

    unsigned int reg;

    bRGA_MMU_CTRL1 = (unsigned int *)(base + RGA2_MMU_CTRL1_OFFSET);
    bRGA_MMU_SRC_BASE = (unsigned int *)(base + RGA2_MMU_SRC_BASE_OFFSET);
    bRGA_MMU_SRC1_BASE = (unsigned int *)(base + RGA2_MMU_SRC1_BASE_OFFSET);
    bRGA_MMU_DST_BASE = (unsigned int *)(base + RGA2_MMU_DST_BASE_OFFSET);
    bRGA_MMU_ELS_BASE = (unsigned int *)(base + RGA2_MMU_ELS_BASE_OFFSET);

    reg = (msg->mmu_info.src0_mmu_flag & 0xf) | ((msg->mmu_info.src1_mmu_flag & 0xf) << RGA2_INDEX_FO) |
          ((msg->mmu_info.dst_mmu_flag & 0xf) << RGA2_INDEX_EI) |
          ((msg->mmu_info.els_mmu_flag & 0x3) << RGA2_INDEX_TWE);

    *bRGA_MMU_CTRL1 = reg;
    *bRGA_MMU_SRC_BASE = (unsigned int)(msg->mmu_info.src0_base_addr) >> RGA2_INDEX_FO;
    *bRGA_MMU_SRC1_BASE = (unsigned int)(msg->mmu_info.src1_base_addr) >> RGA2_INDEX_FO;
    *bRGA_MMU_DST_BASE = (unsigned int)(msg->mmu_info.dst_base_addr) >> RGA2_INDEX_FO;
    *bRGA_MMU_ELS_BASE = (unsigned int)(msg->mmu_info.els_base_addr) >> RGA2_INDEX_FO;
}

int RGA2_gen_reg_info(unsigned char *base, unsigned char *csc_base, struct rga2_req *msg)
{
    unsigned char dst_nn_quantize_en = 0;

    RGA2_set_mode_ctrl(base, msg);

    RGA2_set_pat_info(base, msg);

    switch (msg->render_mode) {
        case bitblt_mode:
            RGA2_set_reg_src_info(base, msg);
            RGA2_set_reg_dst_info(base, msg);
            dst_nn_quantize_en = (msg->alpha_rop_flag >> RGA2_INDEX_EI) & 0x1;
            if (dst_nn_quantize_en != 1) {
                if ((msg->dst.format != RGA2_FORMAT_Y4)) {
                    RGA2_set_reg_alpha_info(base, msg);
                    RGA2_set_reg_rop_info(base, msg);
                }
            }

            if (msg->full_csc.flag) {
                RGA2_set_reg_full_csc(csc_base, msg);
            }
            break;
        case color_fill_mode:
            RGA2_set_reg_color_fill(base, msg);
            RGA2_set_reg_dst_info(base, msg);
            RGA2_set_reg_alpha_info(base, msg);
            break;
        case color_palette_mode:
            RGA2_set_reg_color_palette(base, msg);
            RGA2_set_reg_dst_info(base, msg);
            break;
        case update_palette_table_mode:
            RGA2_set_reg_update_palette_table(base, msg);
            break;
        case update_patten_buff_mode:
            RGA2_set_reg_update_patten_buff(base, msg);
            break;
        default:
            printk("RGA2 ERROR msg render mode %d \n", msg->render_mode);
            break;
    }

    RGA2_set_mmu_info(base, msg);

    return 0;
}

static void format_name_convert(uint32_t *df, uint32_t sf)
{
    switch (sf) {
        case 0x0:
            *df = RGA2_FORMAT_RGBA_8888;
            break;
        case 0x1:
            *df = RGA2_FORMAT_RGBX_8888;
            break;
        case 0x2:
            *df = RGA2_FORMAT_RGB_888;
            break;
        case 0x3:
            *df = RGA2_FORMAT_BGRA_8888;
            break;
        case 0x4:
            *df = RGA2_FORMAT_RGB_565;
            break;
        case 0x5:
            *df = RGA2_FORMAT_RGBA_5551;
            break;
        case 0x6:
            *df = RGA2_FORMAT_RGBA_4444;
            break;
        case 0x7:
            *df = RGA2_FORMAT_BGR_888;
            break;
        case 0x16:
            *df = RGA2_FORMAT_BGRX_8888;
            break;
        case 0x8:
            *df = RGA2_FORMAT_YCbCr_422_SP;
            break;
        case 0x9:
            *df = RGA2_FORMAT_YCbCr_422_P;
            break;
        case 0xa:
            *df = RGA2_FORMAT_YCbCr_420_SP;
            break;
        case 0xb:
            *df = RGA2_FORMAT_YCbCr_420_P;
            break;
        case 0xc:
            *df = RGA2_FORMAT_YCrCb_422_SP;
            break;
        case 0xd:
            *df = RGA2_FORMAT_YCrCb_422_P;
            break;
        case 0xe:
            *df = RGA2_FORMAT_YCrCb_420_SP;
            break;
        case 0xf:
            *df = RGA2_FORMAT_YCrCb_420_P;
            break;

        case 0x10:
            *df = RGA2_FORMAT_BPP_1;
            break;
        case 0x11:
            *df = RGA2_FORMAT_BPP_2;
            break;
        case 0x12:
            *df = RGA2_FORMAT_BPP_4;
            break;
        case 0x13:
            *df = RGA2_FORMAT_BPP_8;
            break;

        case 0x14:
            *df = RGA2_FORMAT_Y4;
            break;
        case 0x15:
            *df = RGA2_FORMAT_YCbCr_400;
            break;

        case 0x18:
            *df = RGA2_FORMAT_YVYU_422;
            break;
        case 0x19:
            *df = RGA2_FORMAT_YVYU_420;
            break;
        case 0x1a:
            *df = RGA2_FORMAT_VYUY_422;
            break;
        case 0x1b:
            *df = RGA2_FORMAT_VYUY_420;
            break;
        case 0x1c:
            *df = RGA2_FORMAT_YUYV_422;
            break;
        case 0x1d:
            *df = RGA2_FORMAT_YUYV_420;
            break;
        case 0x1e:
            *df = RGA2_FORMAT_UYVY_422;
            break;
        case 0x1f:
            *df = RGA2_FORMAT_UYVY_420;
            break;

        case 0x20:
            *df = RGA2_FORMAT_YCbCr_420_SP_10B;
            break;
        case 0x21:
            *df = RGA2_FORMAT_YCrCb_420_SP_10B;
            break;
        case 0x22:
            *df = RGA2_FORMAT_YCbCr_422_SP_10B;
            break;
        case 0x23:
            *df = RGA2_FORMAT_YCrCb_422_SP_10B;
            break;

        case 0x24:
            *df = RGA2_FORMAT_BGR_565;
            break;
        case 0x25:
            *df = RGA2_FORMAT_BGRA_5551;
            break;
        case 0x26:
            *df = RGA2_FORMAT_BGRA_4444;
            break;

        case 0x28:
            *df = RGA2_FORMAT_ARGB_8888;
            break;
        case 0x29:
            *df = RGA2_FORMAT_XRGB_8888;
            break;
        case 0x2a:
            *df = RGA2_FORMAT_ARGB_5551;
            break;
        case 0x2b:
            *df = RGA2_FORMAT_ARGB_4444;
            break;
        case 0x2c:
            *df = RGA2_FORMAT_ABGR_8888;
            break;
        case 0x2d:
            *df = RGA2_FORMAT_XBGR_8888;
            break;
        case 0x2e:
            *df = RGA2_FORMAT_ABGR_5551;
            break;
        case 0x2f:
            *df = RGA2_FORMAT_ABGR_4444;
            break;
    }
}

void RGA_MSG_2_RGA2_MSG(struct rga_req *req_rga, struct rga2_req *req)
{
    u16 alpha_mode_0, alpha_mode_1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(RGA2_INDEX_FO, RGA2_INDEX_FO, 0))
    if (req_rga->render_mode & RGA_BUF_GEM_TYPE_MASK) {
        req->buf_type = RGA_BUF_GEM_TYPE_MASK & RGA_BUF_GEM_TYPE_DMA;
    }

    req_rga->render_mode &= (~RGA_BUF_GEM_TYPE_MASK);
#endif

    if (req_rga->render_mode == RGA2_INDEX_SI) {
        req->render_mode = update_palette_table_mode;
    } else if (req_rga->render_mode == RGA2_INDEX_SE) {
        req->render_mode = update_patten_buff_mode;
    } else if (req_rga->render_mode == RGA2_INDEX_FI) {
        req->render_mode = bitblt_mode;
    } else {
        req->render_mode = req_rga->render_mode;
    }

    memcpy(&req->src, &req_rga->src, sizeof(req_rga->src));
    memcpy(&req->dst, &req_rga->dst, sizeof(req_rga->dst));
    /* The application will only import pat or src1. */
    if (req->render_mode == update_palette_table_mode) {
        memcpy(&req->pat, &req_rga->pat, sizeof(req_rga->pat));
    } else {
        memcpy(&req->src1, &req_rga->pat, sizeof(req_rga->pat));
    }

    format_name_convert(&req->src.format, req_rga->src.format);
    format_name_convert(&req->dst.format, req_rga->dst.format);
    format_name_convert(&req->src1.format, req_rga->pat.format);

    switch (req_rga->rotate_mode & 0x0F) {
        case 1:
            if (req_rga->sina == 0 && req_rga->cosa == RGA2_INDEX_SIZE) {
                /* rotate 0 */
                req->rotate_mode = 0;
            } else if (req_rga->sina == RGA2_INDEX_SIZE && req_rga->cosa == 0) {
                /* rotate 90 */
                req->rotate_mode = 1;
                req->dst.x_offset = req_rga->dst.x_offset - req_rga->dst.act_h + 1;
                req->dst.act_w = req_rga->dst.act_h;
                req->dst.act_h = req_rga->dst.act_w;
            } else if (req_rga->sina == 0 && req_rga->cosa == -RGA2_INDEX_SIZE) {
                /* rotate 180 */
                req->rotate_mode = RGA2_INDEX_TW;
                req->dst.x_offset = req_rga->dst.x_offset - req_rga->dst.act_w + 1;
                req->dst.y_offset = req_rga->dst.y_offset - req_rga->dst.act_h + 1;
            } else if (req_rga->sina == -RGA2_INDEX_SIZE && req_rga->cosa == 0) {
                /* totate 270 */
                req->rotate_mode = RGA2_INDEX_TH;
                req->dst.y_offset = req_rga->dst.y_offset - req_rga->dst.act_w + 1;
                req->dst.act_w = req_rga->dst.act_h;
                req->dst.act_h = req_rga->dst.act_w;
            }
            break;
        case RGA2_INDEX_TW:
            // x_mirror
            req->rotate_mode |= (1 << RGA2_INDEX_FO);
            break;
        case RGA2_INDEX_TH:
            // y_mirror
            req->rotate_mode |= (RGA2_INDEX_TW << RGA2_INDEX_FO);
            break;
        case RGA2_INDEX_FO:
            // x_mirror+y_mirror
            req->rotate_mode |= (RGA2_INDEX_TH << RGA2_INDEX_FO);
            break;
        default:
            req->rotate_mode = 0;
            break;
    }

    switch ((req_rga->rotate_mode & 0xF0) >> RGA2_INDEX_FO) {
        case RGA2_INDEX_TW:
            // x_mirror
            req->rotate_mode |= (1 << RGA2_INDEX_FO);
            break;
        case RGA2_INDEX_TH:
            // y_mirror
            req->rotate_mode |= (RGA2_INDEX_TW << RGA2_INDEX_FO);
            break;
        case RGA2_INDEX_FO:
            // x_mirror+y_mirror
            req->rotate_mode |= (RGA2_INDEX_TH << RGA2_INDEX_FO);
            break;
    }

    if ((req->dst.act_w > RGA2_INDEX_BLOCK) && (req->src.act_h < req->dst.act_h)) {
        req->scale_bicu_mode |= (1 << RGA2_INDEX_FO);
    }

    req->LUT_addr = req_rga->LUT_addr;
    req->rop_mask_addr = req_rga->rop_mask_addr;

    req->bitblt_mode = req_rga->bsfilter_flag;

    req->src_a_global_val = req_rga->alpha_global_value;
    req->dst_a_global_val = req_rga->alpha_global_value;
    req->rop_code = req_rga->rop_code;
    req->rop_mode = req_rga->alpha_rop_mode;

    req->color_fill_mode = req_rga->color_fill_mode;
    req->alpha_zero_key = req_rga->alpha_rop_mode >> RGA2_INDEX_FO;
    req->src_trans_mode = req_rga->src_trans_mode;
    req->color_key_min = req_rga->color_key_min;
    req->color_key_max = req_rga->color_key_max;

    req->fg_color = req_rga->fg_color;
    req->bg_color = req_rga->bg_color;
    memcpy(&req->gr_color, &req_rga->gr_color, sizeof(req_rga->gr_color));
    memcpy(&req->full_csc, &req_rga->full_csc, sizeof(req_rga->full_csc));

    req->palette_mode = req_rga->palette_mode;
    req->yuv2rgb_mode = req_rga->yuv2rgb_mode;
    req->endian_mode = req_rga->endian_mode;
    req->rgb2yuv_mode = 0;

    req->fading_alpha_value = 0;
    req->fading_r_value = req_rga->fading.r;
    req->fading_g_value = req_rga->fading.g;
    req->fading_b_value = req_rga->fading.b;

    /* alpha mode set */
    req->alpha_rop_flag = 0;
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag & 1)));                                   // alpha_rop_enable
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> 1) & 1) << 1);                         // rop_enable
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> RGA2_INDEX_TW) & 1) << RGA2_INDEX_TW); // fading_enable
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> RGA2_INDEX_FO) & 1) << RGA2_INDEX_TH); // alpha_cal_mode_sel
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> RGA2_INDEX_FI) & 1) << RGA2_INDEX_SI); // dst_dither_down
    req->alpha_rop_flag |=
        (((req_rga->alpha_rop_flag >> RGA2_INDEX_SI) & 1) << RGA2_INDEX_SE); // gradient fill mode sel

    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> RGA2_INDEX_EI) & 1) << RGA2_INDEX_EI); // nn_quantize
    req->dither_mode = req_rga->dither_mode;

    if (((req_rga->alpha_rop_flag) & 1)) {
        if ((req_rga->alpha_rop_flag >> RGA2_INDEX_TH) & 1) {
            /* porter duff alpha enable */
            switch (req_rga->PD_mode) {
                case 0: // dst = 0
                    break;
                case 1: // dst = src
                    req->alpha_mode_0 = 0x0212;
                    req->alpha_mode_1 = 0x0212;
                    break;
                case RGA2_INDEX_TW: // dst = dst
                    req->alpha_mode_0 = 0x1202;
                    req->alpha_mode_1 = 0x1202;
                    break;
                case RGA2_INDEX_TH: // dst = (256*sc + (256 - sa)*dc) >> 8
                    if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == 0) {
                        /* both use globalAlpha. */
                        alpha_mode_0 = 0x3010;
                        alpha_mode_1 = 0x3010;
                    } else if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == 1) {
                        /* Do not use globalAlpha. */
                        alpha_mode_0 = 0x3212;
                        alpha_mode_1 = 0x3212;
                    } else if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == RGA2_INDEX_TW) {
                        /* dst use globalAlpha, and dst has pixelAlpha. */
                        alpha_mode_0 = 0x3014;
                        alpha_mode_1 = 0x3014;
                    } else {
                        /* dst use globalAlpha, and dst does not have pixelAlpha. */
                        alpha_mode_0 = 0x3012;
                        alpha_mode_1 = 0x3012;
                    }
                    req->alpha_mode_0 = alpha_mode_0;
                    req->alpha_mode_1 = alpha_mode_1;
                    break;
                case RGA2_INDEX_FO: // dst = (sc*(256-da) + 256*dc) >> 8
                    /* Do not use globalAlpha. */
                    req->alpha_mode_0 = 0x1232;
                    req->alpha_mode_1 = 0x1232;
                    break;
                case RGA2_INDEX_FI: // dst = (da*sc) >> 8
                    break;
                case RGA2_INDEX_SI: // dst = (sa*dc) >> 8
                    break;
                case RGA2_INDEX_SE: // dst = ((256-da)*sc) >> 8
                    break;
                case RGA2_INDEX_EI: // dst = ((256-sa)*dc) >> 8
                    break;
                case RGA2_INDEX_NI: // dst = (da*sc + (256-sa)*dc) >> 8
                    req->alpha_mode_0 = 0x3040;
                    req->alpha_mode_1 = 0x3040;
                    break;
                case RGA2_INDEX_TE: // dst = ((256-da)*sc + (sa*dc)) >> 8
                    break;
                case RGA2_INDEX_EIE: // dst = ((256-da)*sc + (256-sa)*dc) >> 8;
                    break;
                case RGA2_INDEX_TWE:
                    req->alpha_mode_0 = 0x0010;
                    req->alpha_mode_1 = 0x0820;
                    break;
                default:
                    break;
            }
            /* Real color mode */
            if ((req_rga->alpha_rop_flag >> RGA2_INDEX_NI) & 1) {
                if (req->alpha_mode_0 & (0x01 << 1)) {
                    req->alpha_mode_0 |= (1 << RGA2_INDEX_SE);
                }
                if (req->alpha_mode_0 & (0x01 << RGA2_INDEX_NI)) {
                    req->alpha_mode_0 |= (1 << RGA2_INDEX_FIF);
                }
            }
        } else {
            if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == 0) {
                req->alpha_mode_0 = 0x3040;
                req->alpha_mode_1 = 0x3040;
            } else if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == 1) {
                req->alpha_mode_0 = 0x3042;
                req->alpha_mode_1 = 0x3242;
            } else if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == RGA2_INDEX_TW) {
                req->alpha_mode_0 = 0x3044;
                req->alpha_mode_1 = 0x3044;
            }
        }
    }

    if (req_rga->mmu_info.mmu_en && (req_rga->mmu_info.mmu_flag & 1) == 1) {
        req->mmu_info.src0_mmu_flag = 1;
        req->mmu_info.dst_mmu_flag = 1;

        if (req_rga->mmu_info.mmu_flag >> RGA2_INDEX_THIO) {
            req->mmu_info.src0_mmu_flag = ((req_rga->mmu_info.mmu_flag >> RGA2_INDEX_EI) & 1);
            req->mmu_info.src1_mmu_flag = ((req_rga->mmu_info.mmu_flag >> RGA2_INDEX_NI) & 1);
            req->mmu_info.dst_mmu_flag = ((req_rga->mmu_info.mmu_flag >> RGA2_INDEX_TE) & 1);
            req->mmu_info.els_mmu_flag = ((req_rga->mmu_info.mmu_flag >> RGA2_INDEX_EIE) & 1);
        } else {
            if (req_rga->src.yrgb_addr >= 0xa0000000) {
                req->mmu_info.src0_mmu_flag = 0;
                req->src.yrgb_addr = req_rga->src.yrgb_addr - 0x60000000;
                req->src.uv_addr = req_rga->src.uv_addr - 0x60000000;
                req->src.v_addr = req_rga->src.v_addr - 0x60000000;
            }

            if (req_rga->dst.yrgb_addr >= 0xa0000000) {
                req->mmu_info.dst_mmu_flag = 0;
                req->dst.yrgb_addr = req_rga->dst.yrgb_addr - 0x60000000;
            }

            if (req_rga->pat.yrgb_addr >= 0xa0000000) {
                req->mmu_info.src1_mmu_flag = 0;
                req->src1.yrgb_addr = req_rga->pat.yrgb_addr - 0x60000000;
            }
        }
    }
}

static void memcpy_img_info(struct rga_img_info_t *dst, struct rga_img_info_32_t *src)
{
    dst->yrgb_addr = src->yrgb_addr; /* yrgb    mem addr         */
    dst->uv_addr = src->uv_addr;     /* cb/cr   mem addr         */
    dst->v_addr = src->v_addr;       /* cr      mem addr         */
    dst->format = src->format;       // definition by RK_FORMAT

    dst->act_w = src->act_w;
    dst->act_h = src->act_h;
    dst->x_offset = src->x_offset;
    dst->y_offset = src->y_offset;

    dst->vir_w = src->vir_w;
    dst->vir_h = src->vir_h;
    dst->endian_mode = src->endian_mode; // for BPP
    dst->alpha_swap = src->alpha_swap;
}

void RGA_MSG_2_RGA2_MSG_32(struct rga_req_32 *req_rga, struct rga2_req *req)
{
    u16 alpha_mode_0, alpha_mode_1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(RGA2_INDEX_FO, RGA2_INDEX_FO, 0))
    if (req_rga->render_mode & RGA_BUF_GEM_TYPE_MASK) {
        req->buf_type = RGA_BUF_GEM_TYPE_MASK & RGA_BUF_GEM_TYPE_DMA;
    }

    req_rga->render_mode &= (~RGA_BUF_GEM_TYPE_MASK);
#endif
    if (req_rga->render_mode == RGA2_INDEX_SI) {
        req->render_mode = update_palette_table_mode;
    } else if (req_rga->render_mode == RGA2_INDEX_SE) {
        req->render_mode = update_patten_buff_mode;
    } else if (req_rga->render_mode == RGA2_INDEX_FI) {
        req->render_mode = bitblt_mode;
    } else {
        req->render_mode = req_rga->render_mode;
    }
    memcpy_img_info(&req->src, &req_rga->src);
    memcpy_img_info(&req->dst, &req_rga->dst);
    /* The application will only import pat or src1. */
    if (req->render_mode == update_palette_table_mode) {
        memcpy_img_info(&req->pat, &req_rga->pat);
    } else {
        memcpy_img_info(&req->src1, &req_rga->pat);
    }
    format_name_convert(&req->src.format, req_rga->src.format);
    format_name_convert(&req->dst.format, req_rga->dst.format);
    format_name_convert(&req->src1.format, req_rga->pat.format);

    switch (req_rga->rotate_mode & 0x0F) {
        case 1:
            if (req_rga->sina == 0 && req_rga->cosa == RGA2_INDEX_SIZE) {
                /* rotate 0 */
                req->rotate_mode = 0;
            } else if (req_rga->sina == RGA2_INDEX_SIZE && req_rga->cosa == 0) {
                /* rotate 90 */
                req->rotate_mode = 1;
                req->dst.x_offset = req_rga->dst.x_offset - req_rga->dst.act_h + 1;
                req->dst.act_w = req_rga->dst.act_h;
                req->dst.act_h = req_rga->dst.act_w;
            } else if (req_rga->sina == 0 && req_rga->cosa == -RGA2_INDEX_SIZE) {
                /* rotate 180 */
                req->rotate_mode = RGA2_INDEX_TW;
                req->dst.x_offset = req_rga->dst.x_offset - req_rga->dst.act_w + 1;
                req->dst.y_offset = req_rga->dst.y_offset - req_rga->dst.act_h + 1;
            } else if (req_rga->sina == -RGA2_INDEX_SIZE && req_rga->cosa == 0) {
                /* totate 270 */
                req->rotate_mode = RGA2_INDEX_TH;
                req->dst.y_offset = req_rga->dst.y_offset - req_rga->dst.act_w + 1;
                req->dst.act_w = req_rga->dst.act_h;
                req->dst.act_h = req_rga->dst.act_w;
            }
            break;
        case RGA2_INDEX_TW:
            // x_mirror
            req->rotate_mode |= (1 << RGA2_INDEX_FO);
            break;
        case RGA2_INDEX_TH:
            // y_mirror
            req->rotate_mode |= (RGA2_INDEX_TW << RGA2_INDEX_FO);
            break;
        case RGA2_INDEX_FO:
            // x_mirror+y_mirror
            req->rotate_mode |= (RGA2_INDEX_TH << RGA2_INDEX_FO);
            break;
        default:
            req->rotate_mode = 0;
            break;
    }

    switch ((req_rga->rotate_mode & 0xF0) >> RGA2_INDEX_FO) {
        case RGA2_INDEX_TW:
            // x_mirror
            req->rotate_mode |= (1 << RGA2_INDEX_FO);
            break;
        case RGA2_INDEX_TH:
            // y_mirror
            req->rotate_mode |= (RGA2_INDEX_TW << RGA2_INDEX_FO);
            break;
        case RGA2_INDEX_FO:
            // x_mirror+y_mirror
            req->rotate_mode |= (RGA2_INDEX_TH << RGA2_INDEX_FO);
            break;
    }

    if ((req->dst.act_w > RGA2_INDEX_BLOCK) && (req->src.act_h < req->dst.act_h)) {
        req->scale_bicu_mode |= (1 << RGA2_INDEX_FO);
    }
    req->LUT_addr = req_rga->LUT_addr;
    req->rop_mask_addr = req_rga->rop_mask_addr;
    req->bitblt_mode = req_rga->bsfilter_flag;
    req->src_a_global_val = req_rga->alpha_global_value;
    req->dst_a_global_val = req_rga->alpha_global_value;
    req->rop_code = req_rga->rop_code;
    req->rop_mode = req_rga->alpha_rop_mode;
    req->color_fill_mode = req_rga->color_fill_mode;
    req->alpha_zero_key = req_rga->alpha_rop_mode >> RGA2_INDEX_FO;
    req->src_trans_mode = req_rga->src_trans_mode;
    req->color_key_min = req_rga->color_key_min;
    req->color_key_max = req_rga->color_key_max;
    req->fg_color = req_rga->fg_color;
    req->bg_color = req_rga->bg_color;
    memcpy(&req->gr_color, &req_rga->gr_color, sizeof(req_rga->gr_color));
    memcpy(&req->full_csc, &req_rga->full_csc, sizeof(req_rga->full_csc));

    req->palette_mode = req_rga->palette_mode;
    req->yuv2rgb_mode = req_rga->yuv2rgb_mode;
    req->endian_mode = req_rga->endian_mode;
    req->rgb2yuv_mode = 0;
    req->fading_alpha_value = 0;
    req->fading_r_value = req_rga->fading.r;
    req->fading_g_value = req_rga->fading.g;
    req->fading_b_value = req_rga->fading.b;

    /* alpha mode set */
    req->alpha_rop_flag = 0;
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag & 1)));                                   // alpha_rop_enable
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> 1) & 1) << 1);                         // rop_enable
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> RGA2_INDEX_TW) & 1) << RGA2_INDEX_TW); // fading_enable
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> RGA2_INDEX_FO) & 1) << RGA2_INDEX_TH); // alpha_cal_mode_sel
    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> RGA2_INDEX_FI) & 1) << RGA2_INDEX_SI); // dst_dither_down
    req->alpha_rop_flag |=
        (((req_rga->alpha_rop_flag >> RGA2_INDEX_SI) & 1) << RGA2_INDEX_SE); // gradient fill mode sel

    req->alpha_rop_flag |= (((req_rga->alpha_rop_flag >> RGA2_INDEX_EI) & 1) << RGA2_INDEX_EI); // nn_quantize
    req->dither_mode = req_rga->dither_mode;

    if (((req_rga->alpha_rop_flag) & 1)) {
        if ((req_rga->alpha_rop_flag >> RGA2_INDEX_TH) & 1) {
            /* porter duff alpha enable */
            switch (req_rga->PD_mode) {
                case 0: // dst = 0
                    break;
                case 1: // dst = src
                    req->alpha_mode_0 = 0x0212;
                    req->alpha_mode_1 = 0x0212;
                    break;
                case RGA2_INDEX_TW: // dst = dst
                    req->alpha_mode_0 = 0x1202;
                    req->alpha_mode_1 = 0x1202;
                    break;
                case RGA2_INDEX_TH: // dst = (256*sc + (256 - sa)*dc) >> 8
                    if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == 0) {
                        /* both use globalAlpha. */
                        alpha_mode_0 = 0x3010;
                        alpha_mode_1 = 0x3010;
                    } else if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == 1) {
                        /* dst use globalAlpha, and dst does not have pixelAlpha. */
                        alpha_mode_0 = 0x3012;
                        alpha_mode_1 = 0x3012;
                    } else if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == RGA2_INDEX_TW) {
                        /* dst use globalAlpha, and dst has pixelAlpha. */
                        alpha_mode_0 = 0x3014;
                        alpha_mode_1 = 0x3014;
                    } else {
                        /* Do not use globalAlpha. */
                        alpha_mode_0 = 0x3212;
                        alpha_mode_1 = 0x3212;
                    }
                    req->alpha_mode_0 = alpha_mode_0;
                    req->alpha_mode_1 = alpha_mode_1;
                    break;
                case RGA2_INDEX_FO: // dst = (sc*(256-da) + 256*dc) >> 8
                    /* Do not use globalAlpha. */
                    req->alpha_mode_0 = 0x1232;
                    req->alpha_mode_1 = 0x1232;
                    break;
                case RGA2_INDEX_FI: // dst = (da*sc) >> 8
                    break;
                case RGA2_INDEX_SI: // dst = (sa*dc) >> 8
                    break;
                case RGA2_INDEX_SE: // dst = ((256-da)*sc) >> 8
                    break;
                case RGA2_INDEX_EI: // dst = ((256-sa)*dc) >> 8
                    break;
                case RGA2_INDEX_NI: // dst = (da*sc + (256-sa)*dc) >> 8
                    req->alpha_mode_0 = 0x3040;
                    req->alpha_mode_1 = 0x3040;
                    break;
                case RGA2_INDEX_TE: // dst = ((256-da)*sc + (sa*dc)) >> 8
                    break;
                case RGA2_INDEX_EIE: // dst = ((256-da)*sc + (256-sa)*dc) >> 8;
                    break;
                case RGA2_INDEX_TWE:
                    req->alpha_mode_0 = 0x0010;
                    req->alpha_mode_1 = 0x0820;
                    break;
                default:
                    break;
            }
            /* Real color mode */
            if ((req_rga->alpha_rop_flag >> RGA2_INDEX_NI) & 1) {
                if (req->alpha_mode_0 & (0x01 << 1)) {
                    req->alpha_mode_0 |= (1 << RGA2_INDEX_SE);
                }
                if (req->alpha_mode_0 & (0x01 << RGA2_INDEX_NI)) {
                    req->alpha_mode_0 |= (1 << RGA2_INDEX_FIF);
                }
            }
        } else {
            if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == 0) {
                req->alpha_mode_0 = 0x3040;
                req->alpha_mode_1 = 0x3040;
            } else if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == 1) {
                req->alpha_mode_0 = 0x3042;
                req->alpha_mode_1 = 0x3242;
            } else if ((req_rga->alpha_rop_mode & RGA2_INDEX_TH) == RGA2_INDEX_TW) {
                req->alpha_mode_0 = 0x3044;
                req->alpha_mode_1 = 0x3044;
            }
        }
    }

    if (req_rga->mmu_info.mmu_en && (req_rga->mmu_info.mmu_flag & 1) == 1) {
        req->mmu_info.src0_mmu_flag = 1;
        req->mmu_info.dst_mmu_flag = 1;
        if (req_rga->mmu_info.mmu_flag >> RGA2_INDEX_THIO) {
            req->mmu_info.src0_mmu_flag = ((req_rga->mmu_info.mmu_flag >> RGA2_INDEX_EI) & 1);
            req->mmu_info.src1_mmu_flag = ((req_rga->mmu_info.mmu_flag >> RGA2_INDEX_NI) & 1);
            req->mmu_info.dst_mmu_flag = ((req_rga->mmu_info.mmu_flag >> RGA2_INDEX_TE) & 1);
            req->mmu_info.els_mmu_flag = ((req_rga->mmu_info.mmu_flag >> RGA2_INDEX_EIE) & 1);
        } else {
            if (req_rga->src.yrgb_addr >= 0xa0000000) {
                req->mmu_info.src0_mmu_flag = 0;
                req->src.yrgb_addr = req_rga->src.yrgb_addr - 0x60000000;
                req->src.uv_addr = req_rga->src.uv_addr - 0x60000000;
                req->src.v_addr = req_rga->src.v_addr - 0x60000000;
            }

            if (req_rga->dst.yrgb_addr >= 0xa0000000) {
                req->mmu_info.dst_mmu_flag = 0;
                req->dst.yrgb_addr = req_rga->dst.yrgb_addr - 0x60000000;
            }

            if (req_rga->pat.yrgb_addr >= 0xa0000000) {
                req->mmu_info.src1_mmu_flag = 0;
                req->src1.yrgb_addr = req_rga->pat.yrgb_addr - 0x60000000;
            }
        }
    }
}
