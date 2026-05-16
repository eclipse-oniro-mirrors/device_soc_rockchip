// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) Rockchip Electronics Co., Ltd. */
#include <linux/clk.h>
#include <linux/proc_fs.h>
#include <linux/sem.h>
#include <linux/seq_file.h>

#include "dev.h"
#include "procfs.h"
#include "version.h"
#include "regs.h"
#include "regs_v2x.h"

#ifdef CONFIG_PROC_FS

static void isp20_show(struct rkisp_device *dev, struct seq_file *p)
{
    u32 full_range_flg = CIF_ISP_CTRL_ISP_CSM_Y_FULL_ENA | CIF_ISP_CTRL_ISP_CSM_C_FULL_ENA;
    static const char *const effect[] = {"BLACKWHITE", "NEGATIVE", "SEPIA",   "COLOR_SEL",
                                         "EMBOSS",     "SKETCH",   "SHARPEN", "RKSHARPEN"};
    u32 val;

    val = rkisp_read(dev, ISP_DPCC0_MODE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DPCC0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_DPCC1_MODE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DPCC1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_DPCC2_MODE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DPCC2", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_BLS_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "BLS", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CIF_ISP_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SDG", (val & CIF_ISP_CTRL_ISP_GAMMA_IN_ENA) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_LSC_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "LSC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CIF_ISP_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x) (gain: 0x%08x, 0x%08x)\n", "AWBGAIN", (val & CIF_ISP_CTRL_ISP_AWB_ENA) ? "ON" : "OFF",
               val, rkisp_read(dev, CIF_ISP_AWB_GAIN_G_V12, false), rkisp_read(dev, CIF_ISP_AWB_GAIN_RB_V12, false));
    val = rkisp_read(dev, ISP_DEBAYER_CONTROL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DEBAYER", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_CCM_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CCM", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_GAMMA_OUT_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "GAMMA_OUT", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CPROC_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CPROC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CIF_IMG_EFF_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x) (effect: %s)\n", "IE", (val & 1) ? "ON" : "OFF", val,
               effect[(val & CIF_IMG_EFF_CTRL_MODE_MASK) >> 1]);
    val = rkisp_read(dev, ISP_WDR_CTRL0, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "WDR", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_HDRTMO_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "HDRTMO", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_HDRMGE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "HDRMGE", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWNR_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWNR", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_GIC_CONTROL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "GIC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_DHAZ_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DHAZ", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_3DLUT_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "3DLUT", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_GAIN_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "GAIN", val ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_LDCH_STS, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "LDCH", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CSM", (val & full_range_flg) ? "FULL" : "LIMITED", val);

    val = rkisp_read(dev, ISP_AFM_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SIAF", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CIF_ISP_AWB_PROP_V10, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SIAWB", (val & CIF_ISP_AWB_ENABLE) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_YUVAE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "YUVAE", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_HIST_HIST_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SIHST", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWAF_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAF", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWAWB_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAWB", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWAE_LITE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, RAWAE_BIG2_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, RAWAE_BIG3_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE2", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, RAWAE_BIG1_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE3", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWHIST_LITE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWHIST_BIG2_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWHIST_BIG3_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST2", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWHIST_BIG1_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST3", (val & 1) ? "ON" : "OFF", val);
}

static void isp21_show(struct rkisp_device *dev, struct seq_file *p)
{
    u32 full_range_flg = CIF_ISP_CTRL_ISP_CSM_Y_FULL_ENA | CIF_ISP_CTRL_ISP_CSM_C_FULL_ENA;
    static const char *const effect[] = {"BLACKWHITE", "NEGATIVE", "SEPIA",   "COLOR_SEL",
                                         "EMBOSS",     "SKETCH",   "SHARPEN", "RKSHARPEN"};
    u32 val, tmp;

    val = rkisp_read(dev, ISP_DPCC0_MODE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DPCC0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_DPCC1_MODE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DPCC1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_BLS_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "BLS", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CIF_ISP_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SDG", (val & CIF_ISP_CTRL_ISP_GAMMA_IN_ENA) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_LSC_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "LSC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CIF_ISP_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x) (gain: 0x%08x, 0x%08x)\n", "AWBGAIN", (val & CIF_ISP_CTRL_ISP_AWB_ENA) ? "ON" : "OFF",
               val, rkisp_read(dev, CIF_ISP_AWB_GAIN_G_V12, false), rkisp_read(dev, CIF_ISP_AWB_GAIN_RB_V12, false));
    val = rkisp_read(dev, ISP_DEBAYER_CONTROL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DEBAYER", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_CCM_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CCM", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_GAMMA_OUT_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "GAMMA_OUT", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CPROC_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CPROC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CIF_IMG_EFF_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x) (effect: %s)\n", "IE", (val & 1) ? "ON" : "OFF", val,
               effect[(val & CIF_IMG_EFF_CTRL_MODE_MASK) >> 1]);
    val = rkisp_read(dev, ISP21_DRC_CTRL0, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "HDRDRC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_HDRMGE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "HDRMGE", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP21_BAYNR_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "BAYNR", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP21_BAY3D_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "BAY3D", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP21_YNR_GLOBAL_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "YNR", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP21_CNR_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CNR", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP21_SHARP_SHARP_EN, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SHARP", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_GIC_CONTROL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "GIC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_DHAZ_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DHAZ", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_3DLUT_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "3DLUT", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_LDCH_STS, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "LDCH", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_CTRL, false);
    tmp = rkisp_read(dev, ISP_CC_COEFF_0, false);
    seq_printf(p,
               "%-10s %s(0x%x), y_offs:0x%x c_offs:0x%x\n"
               "\t   coeff Y:0x%x 0x%x 0x%x CB:0x%x 0x%x 0x%x CR:0x%x 0x%x 0x%x\n",
               "CSM", (val & full_range_flg) ? "FULL" : "LIMIT", val, (tmp >> 0x18) & 0x3f,
               (tmp >> 0x10) & 0xff ? (tmp >> 0x10) & 0xff : 0x80, tmp & 0x1ff, rkisp_read(dev, ISP_CC_COEFF_1, false),
               rkisp_read(dev, ISP_CC_COEFF_2, false), rkisp_read(dev, ISP_CC_COEFF_3, false),
               rkisp_read(dev, ISP_CC_COEFF_4, false), rkisp_read(dev, ISP_CC_COEFF_5, false),
               rkisp_read(dev, ISP_CC_COEFF_6, false), rkisp_read(dev, ISP_CC_COEFF_7, false),
               rkisp_read(dev, ISP_CC_COEFF_8, false));
    val = rkisp_read(dev, ISP_AFM_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SIAF", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, CIF_ISP_AWB_PROP_V10, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SIAWB", (val & CIF_ISP_AWB_ENABLE) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_YUVAE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "YUVAE", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_HIST_HIST_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SIHST", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWAF_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAF", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP21_RAWAWB_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAWB", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWAE_LITE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, RAWAE_BIG2_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, RAWAE_BIG3_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE2", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, RAWAE_BIG1_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE3", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWHIST_LITE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWHIST_BIG2_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWHIST_BIG3_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST2", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP_RAWHIST_BIG1_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST3", (val & 1) ? "ON" : "OFF", val);
}

static void isp30_show(struct rkisp_device *dev, struct seq_file *p)
{
    u32 full_range_flg = CIF_ISP_CTRL_ISP_CSM_Y_FULL_ENA | CIF_ISP_CTRL_ISP_CSM_C_FULL_ENA;
    static const char *const effect[] = {"BLACKWHITE", "NEGATIVE", "SEPIA",   "COLOR_SEL",
                                         "EMBOSS",     "SKETCH",   "SHARPEN", "RKSHARPEN"};
    u32 val, tmp;

    val = rkisp_read(dev, ISP3X_CMSK_CTRL0, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CMSK", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_DPCC0_MODE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DPCC0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_DPCC1_MODE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DPCC1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_DPCC2_MODE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DPCC2", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_BLS_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "BLS", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_ISP_CTRL0, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SDG", (val & BIT(0x06)) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_LSC_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "LSC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_ISP_CTRL0, false);
    seq_printf(p, "%-10s %s(0x%x) (gain: 0x%08x, 0x%08x)\n", "AWBGAIN", (val & BIT(0x07)) ? "ON" : "OFF", val,
               rkisp_read(dev, ISP3X_ISP_AWB_GAIN0_G, false), rkisp_read(dev, ISP3X_ISP_AWB_GAIN0_RB, false));
    val = rkisp_read(dev, ISP3X_DEBAYER_CONTROL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DEBAYER", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_CCM_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CCM", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_GAMMA_OUT_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "GAMMA_OUT", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_CPROC_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CPROC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_IMG_EFF_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x) (effect: %s)\n", "IE", (val & 1) ? "ON" : "OFF", val,
               effect[(val & CIF_IMG_EFF_CTRL_MODE_MASK) >> 1]);
    val = rkisp_read(dev, ISP3X_DRC_CTRL0, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "HDRDRC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_HDRMGE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "HDRMGE", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_BAYNR_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "BAYNR", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_BAY3D_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "BAY3D", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_YNR_GLOBAL_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "YNR", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_CNR_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CNR", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_SHARP_EN, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "SHARP", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_GIC_CONTROL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "GIC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_DHAZ_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "DHAZ", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_3DLUT_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "3DLUT", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_LDCH_STS, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "LDCH", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_ISP_CTRL0, false);
    tmp = rkisp_read(dev, ISP3X_ISP_CC_COEFF_0, false);
    seq_printf(p,
               "%-10s %s(0x%x), y_offs:0x%x c_offs:0x%x\n"
               "\t   coeff Y:0x%x 0x%x 0x%x CB:0x%x 0x%x 0x%x CR:0x%x 0x%x 0x%x\n",
               "CSM", (val & full_range_flg) ? "FULL" : "LIMIT", val, (tmp >> 0x18) & 0x3f,
               (tmp >> 0x10) & 0xff ? (tmp >> 0x10) & 0xff : 0x80, tmp & 0x1ff, \
               rkisp_read(dev, ISP3X_ISP_CC_COEFF_1, false), rkisp_read(dev, ISP3X_ISP_CC_COEFF_2, false),
               rkisp_read(dev, ISP3X_ISP_CC_COEFF_3, false), rkisp_read(dev, ISP3X_ISP_CC_COEFF_4, false),
               rkisp_read(dev, ISP3X_ISP_CC_COEFF_5, false), rkisp_read(dev, ISP3X_ISP_CC_COEFF_6, false),
               rkisp_read(dev, ISP3X_ISP_CC_COEFF_7, false), rkisp_read(dev, ISP3X_ISP_CC_COEFF_8, false));
    val = rkisp_read(dev, ISP3X_CAC_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "CAC", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_GAIN_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "GAIN", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWAF_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAF", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWAWB_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAWB", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWAE_LITE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWAE_BIG2_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWAE_BIG3_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE2", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWAE_BIG1_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWAE3", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWHIST_LITE_CTRL, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST0", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWHIST_BIG2_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST1", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWHIST_BIG3_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST2", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_RAWHIST_BIG1_BASE, false);
    seq_printf(p, "%-10s %s(0x%x)\n", "RAWHIST3", (val & 1) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_ISP_CTRL1, true);
    seq_printf(p, "%-10s %s(0x%x)\n", "BigMode", val & BIT(0x1C) ? "ON" : "OFF", val);
    val = rkisp_read(dev, ISP3X_ISP_DEBUG1, true);
    seq_printf(p,
               "%-10s space full status group (0x%x)\n"
               "\t   ibuf2:0x%x ibuf1:0x%x ibuf0:0x%x mpfbc_infifo:0x%x\n"
               "\t   r1fifo:0x%x r0fifo:0x%x outfifo:0x%x lafifo:0x%x\n",
               "DEBUG1", val, val >> 0x1C, (val >> 0x18) & 0xf, (val >> 0x14) & 0xf, (val >> 0x10) & 0xf,
               (val >> 0x0C) & 0xf, (val >> 0x08) & 0xf, (val >> 0x04) & 0xf, val & 0xf);
    val = rkisp_read(dev, ISP3X_ISP_DEBUG2, true);
    seq_printf(p,
               "%-10s 0x%x\n"
               "\t   bay3d_fifo_full iir:%d cur:%d\n"
               "\t   module outform vertical counter:%d, out frame counter:%d\n"
               "\t   isp output line counter:%d\n",
               "DEBUG2", val, !!(val & BIT(0x1F)), !!(val & BIT(0x1E)), (val >> 0x10) & 0x3fff, (val >> 0x0E) & 0x3,
               val & 0x3fff);
    val = rkisp_read(dev, ISP3X_ISP_DEBUG3, true);
    seq_printf(p,
               "%-10s isp pipeline group (0x%x)\n"
               "\t   mge(%d %d) rawnr(%d %d) bay3d(%d %d) tmo(%d %d)\n"
               "\t   gic(%d %d) dbr(%d %d) debayer(%d %d) dhaz(%d %d)\n"
               "\t   lut3d(%d %d) ldch(%d %d) ynr(%d %d) shp(%d %d)\n"
               "\t   cgc(%d %d) cac(%d %d) isp_out(%d %d) isp_in(%d %d)\n",
               "DEBUG3", val, !!(val & BIT(0x1F)), !!(val & BIT(0x1E)), !!(val & BIT(0x1D)), !!(val & BIT(0x1C)),
               !!(val & BIT(0x1B)), !!(val & BIT(0x1A)), !!(val & BIT(0x19)), !!(val & BIT(0x18)), !!(val & BIT(0x17)),
               !!(val & BIT(0x16)), !!(val & BIT(0x15)), !!(val & BIT(0x4)), !!(val & BIT(0x13)), !!(val & BIT(0x12)),
               !!(val & BIT(0x11)), !!(val & BIT(0x10)), !!(val & BIT(0x0F)), !!(val & BIT(0x0E)), !!(val & BIT(0x0D)),
               !!(val & BIT(0x0C)), !!(val & BIT(0x0B)), !!(val & BIT(0x0A)), !!(val & BIT(0x09)), !!(val & BIT(0x08)),
               !!(val & BIT(0x07)), !!(val & BIT(0x06)), !!(val & BIT(0x05)), !!(val & BIT(0x04)), !!(val & BIT(0x03)),
               !!(val & BIT(0x02)), !!(val & BIT(1)), !!(val & BIT(0)));
}

static void isp30_unite_show(struct rkisp_device *dev, struct seq_file *p)
{
    u32 full_range_flg = CIF_ISP_CTRL_ISP_CSM_Y_FULL_ENA | CIF_ISP_CTRL_ISP_CSM_C_FULL_ENA;
    static const char *const effect[] = {"BLACKWHITE", "NEGATIVE", "SEPIA",   "COLOR_SEL",
                                         "EMBOSS",     "SKETCH",   "SHARPEN", "RKSHARPEN"};
    u32 v0, v1;

    v0 = rkisp_read(dev, ISP3X_CMSK_CTRL0, false);
    v1 = rkisp_next_read(dev, ISP3X_CMSK_CTRL0, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "CMSK", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_DPCC0_MODE, false);
    v1 = rkisp_next_read(dev, ISP3X_DPCC0_MODE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "DPCC0", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_DPCC1_MODE, false);
    v1 = rkisp_next_read(dev, ISP3X_DPCC1_MODE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "DPCC1", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_DPCC2_MODE, false);
    v1 = rkisp_next_read(dev, ISP3X_DPCC2_MODE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "DPCC2", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_BLS_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_BLS_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "BLS", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_ISP_CTRL0, false);
    v1 = rkisp_next_read(dev, ISP3X_ISP_CTRL0, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "SDG", (v0 & BIT(0x06)) ? "ON" : "OFF", v0,
               (v1 & BIT(0x06)) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_LSC_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_LSC_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "LSC", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_ISP_CTRL0, false);
    v1 = rkisp_next_read(dev, ISP3X_ISP_CTRL0, false);
    seq_printf(p, "%-10s Left %s(0x%x) gain:0x%08x 0x%08x, Right %s(0x%x) gain:0x%08x 0x%08x\n", "AWBGAIN",
               (v0 & BIT(0x07)) ? "ON" : "OFF", v0, rkisp_read(dev, ISP3X_ISP_AWB_GAIN0_G, false),
               rkisp_read(dev, ISP3X_ISP_AWB_GAIN0_RB, false), (v1 & BIT(0x07)) ? "ON" : "OFF", v1,
               rkisp_next_read(dev, ISP3X_ISP_AWB_GAIN0_G, false), rkisp_next_read(dev, ISP3X_ISP_AWB_GAIN0_RB, false));
    v0 = rkisp_read(dev, ISP3X_DEBAYER_CONTROL, false);
    v1 = rkisp_next_read(dev, ISP3X_DEBAYER_CONTROL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "DEBAYER", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_CCM_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_CCM_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "CCM", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_GAMMA_OUT_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_GAMMA_OUT_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "GAMMA_OUT", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_CPROC_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_CPROC_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "CPROC", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_IMG_EFF_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_IMG_EFF_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x) effect:%s, Right %s(0x%x) effect:%s\n", "IE", (v0 & 1) ? "ON" : "OFF", v0,
               effect[(v0 & CIF_IMG_EFF_CTRL_MODE_MASK) >> 1], (v1 & 1) ? "ON" : "OFF", v1,
               effect[(v1 & CIF_IMG_EFF_CTRL_MODE_MASK) >> 1]);
    v0 = rkisp_read(dev, ISP3X_DRC_CTRL0, false);
    v1 = rkisp_next_read(dev, ISP3X_DRC_CTRL0, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "HDRDRC", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_HDRMGE_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_HDRMGE_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "HDRMGE", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_BAYNR_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_BAYNR_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "BAYNR", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_BAY3D_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_BAY3D_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "BAY3D", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_YNR_GLOBAL_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_YNR_GLOBAL_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "YNR", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_CNR_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_CNR_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "CNR", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_SHARP_EN, false);
    v1 = rkisp_next_read(dev, ISP3X_SHARP_EN, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "SHARP", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_GIC_CONTROL, false);
    v1 = rkisp_next_read(dev, ISP3X_GIC_CONTROL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "GIC", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_DHAZ_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_DHAZ_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "DHAZ", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_3DLUT_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_3DLUT_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "3DLUT", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_LDCH_STS, false);
    v1 = rkisp_next_read(dev, ISP3X_LDCH_STS, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "LDCH", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_ISP_CTRL0, false);
    v1 = rkisp_next_read(dev, ISP3X_ISP_CTRL0, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "CSM", (v0 & full_range_flg) ? "FULL" : "LIMIT", v0,
               (v1 & full_range_flg) ? "FULL" : "LIMIT", v1);
    v0 = rkisp_read(dev, ISP3X_CAC_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_CAC_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "CAC", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_GAIN_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_GAIN_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "GAIN", (v0 & 1) ? "ON" : "OFF", v0, (v1 & 1) ? "ON" : "OFF",
               v1);
    v0 = rkisp_read(dev, ISP3X_RAWAF_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWAF_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWAF", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWAWB_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWAWB_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWAWB", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWAE_LITE_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWAE_LITE_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWAE0", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWAE_BIG2_BASE, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWAE_BIG2_BASE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWAE1", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWAE_BIG3_BASE, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWAE_BIG3_BASE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWAE2", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWAE_BIG1_BASE, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWAE_BIG1_BASE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWAE3", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWHIST_LITE_CTRL, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWHIST_LITE_CTRL, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWHIST0", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWHIST_BIG2_BASE, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWHIST_BIG2_BASE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWHIST1", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWHIST_BIG3_BASE, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWHIST_BIG3_BASE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWHIST2", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_RAWHIST_BIG1_BASE, false);
    v1 = rkisp_next_read(dev, ISP3X_RAWHIST_BIG1_BASE, false);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "RAWHIST3", (v0 & 1) ? "ON" : "OFF", v0,
               (v1 & 1) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_ISP_CTRL1, true);
    v1 = rkisp_next_read(dev, ISP3X_ISP_CTRL1, true);
    seq_printf(p, "%-10s Left %s(0x%x), Right %s(0x%x)\n", "BigMode", v0 & BIT(0x1C) ? "ON" : "OFF", v0,
               v1 & BIT(0x1C) ? "ON" : "OFF", v1);
    v0 = rkisp_read(dev, ISP3X_ISP_DEBUG1, true);
    v1 = rkisp_next_read(dev, ISP3X_ISP_DEBUG1, true);
    seq_printf(p,
               "%-10s space full status group. Left:0x%x Right:0x%x\n"
               "\t   ibuf2(L:0x%x R:0x%x) ibuf1(L:0x%x R:0x%x)\n"
               "\t   ibuf0(L:0x%x R:0x%x) mpfbc_infifo(L:0x%x R:0x%x)\n"
               "\t   r1fifo(L:0x%x R:0x%x) r0fifo(L:0x%x R:0x%x)\n"
               "\t   outfifo(L:0x%x R:0x%x) lafifo(L:0x%x R:0x%x)\n",
               "DEBUG1", v0, v1, v0 >> 0x1C, v1 >> 0x1C, (v0 >> 0x18) & 0xf, (v1 >> 0x18) & 0xf, (v0 >> 0x14) & 0xf,
               (v1 >> 0x14) & 0xf, (v0 >> 0x10) & 0xf, (v1 >> 0x10) & 0xf, (v0 >> 0x0C) & 0xf, (v1 >> 0x0C) & 0xf,
               (v0 >> 0x08) & 0xf, (v1 >> 0x08) & 0xf, (v0 >> 0x04) & 0xf, (v1 >> 0x04) & 0xf, v0 & 0xf, v1 & 0xf);
    v0 = rkisp_read(dev, ISP3X_ISP_DEBUG2, true);
    v1 = rkisp_next_read(dev, ISP3X_ISP_DEBUG2, true);
    seq_printf(p,
               "%-10s Left:0x%x Right:0x%x\n"
               "\t   bay3d_fifo_full iir(L:%d R:%d) cur(L:%d R:%d)\n"
               "\t   module outform vertical counter(L:%d R:%d), out frame counter:(L:%d R:%d)\n"
               "\t   isp output line counter(L:%d R:%d)\n",
               "DEBUG2", v0, v1, !!(v0 & BIT(0x1F)), !!(v1 & BIT(0x1F)), !!(v0 & BIT(0x1E)), !!(v1 & BIT(0x1E)),
               (v0 >> 0x10) & 0x3fff, (v1 >> 0x10) & 0x3fff, (v0 >> 0x0E) & 0x3, (v1 >> 0x0E) & 0x3, \
                v0 & 0x3fff, v1 & 0x3fff);
    v0 = rkisp_read(dev, ISP3X_ISP_DEBUG3, true);
    v1 = rkisp_next_read(dev, ISP3X_ISP_DEBUG3, true);
    seq_printf(
        p,
        "%-10s isp pipeline group Left:0x%x Right:0x%x\n"
        "\t   mge(L:%d %d R:%d %d) rawnr(L:%d %d R:%d %d)\n"
        "\t   bay3d(L:%d %d R:%d %d) tmo(L:%d %d R:%d %d)\n"
        "\t   gic(L:%d %d R:%d %d) dbr(L:%d %d R:%d %d)\n"
        "\t   debayer(L:%d %d R:%d %d) dhaz(L:%d %d R:%d %d)\n"
        "\t   lut3d(L:%d %d R:%d %d) ldch(L:%d %d R:%d %d)\n"
        "\t   ynr(L:%d %d R:%d %d) shp(L:%d %d R:%d %d)\n"
        "\t   cgc(L:%d %d R:%d %d) cac(L:%d %d R:%d %d)\n"
        "\t   isp_out(L:%d %d R:%d %d) isp_in(L:%d %d R:%d %d)\n",
        "DEBUG3", v0, v1, !!(v0 & BIT(0x1F)), !!(v0 & BIT(0x1E)), !!(v1 & BIT(0x1F)), !!(v1 & BIT(0x1E)),
        !!(v0 & BIT(0x1D)), !!(v0 & BIT(0x1C)), !!(v1 & BIT(0x1D)), !!(v1 & BIT(0x1C)), !!(v0 & BIT(0x1B)),
        !!(v0 & BIT(0x1A)), !!(v1 & BIT(0x1B)), !!(v1 & BIT(0x1A)), !!(v0 & BIT(0x19)), !!(v0 & BIT(0x18)),
        !!(v1 & BIT(0x19)), !!(v1 & BIT(0x18)), !!(v0 & BIT(0x17)), !!(v0 & BIT(0x16)), !!(v1 & BIT(0x17)),
        !!(v1 & BIT(0x16)), !!(v0 & BIT(0x15)), !!(v0 & BIT(0x14)), !!(v1 & BIT(0x15)), !!(v1 & BIT(0x14)),
        !!(v0 & BIT(0x13)), !!(v0 & BIT(0x12)), !!(v1 & BIT(0x13)), !!(v1 & BIT(0x12)), !!(v0 & BIT(0x11)),
        !!(v0 & BIT(0x10)), !!(v1 & BIT(0x11)), !!(v1 & BIT(0x10)), !!(v0 & BIT(0x0F)), !!(v0 & BIT(0x0E)),
        !!(v1 & BIT(0x0F)), !!(v1 & BIT(0x0E)), !!(v0 & BIT(0x0D)), !!(v0 & BIT(0x0C)), !!(v1 & BIT(0x0D)),
        !!(v1 & BIT(0x0C)), !!(v0 & BIT(0x0B)), !!(v0 & BIT(0x0A)), !!(v1 & BIT(0x0B)), !!(v1 & BIT(0x0A)),
        !!(v0 & BIT(0x09)), !!(v0 & BIT(0x08)), !!(v1 & BIT(0x09)), !!(v1 & BIT(0x08)), !!(v0 & BIT(0x07)),
        !!(v0 & BIT(0x06)), !!(v1 & BIT(0x07)), !!(v1 & BIT(0x06)), !!(v0 & BIT(0x05)), !!(v0 & BIT(0x04)),
        !!(v1 & BIT(0x05)), !!(v1 & BIT(0x04)), !!(v0 & BIT(0x03)), !!(v0 & BIT(0x02)), !!(v1 & BIT(0x03)),
        !!(v1 & BIT(0x02)), !!(v0 & BIT(1)), !!(v0 & BIT(0)), !!(v1 & BIT(1)), !!(v1 & BIT(0)));
}

static int isp_show(struct seq_file *p, void *v)
{
    struct rkisp_device *dev = p->private;
    struct rkisp_isp_subdev *sdev = &dev->isp_sdev;
    struct rkisp_sensor_info *sensor = dev->active_sensor;
    u32 val = 0;

    seq_printf(p, "%-10s Version:v%02x.%02x.%02x\n", dev->name, RKISP_DRIVER_VERSION >> 0x10,
               (RKISP_DRIVER_VERSION & 0xff00) >> 0x08, RKISP_DRIVER_VERSION & 0x00ff);
    for (val = 0; val < dev->hw_dev->num_clks; val++) {
        seq_printf(p, "%-10s %ld\n", dev->hw_dev->match_data->clks[val], clk_get_rate(dev->hw_dev->clks[val]));
    }
    if (!(dev->isp_state & ISP_START)) {
        return 0;
    }

    seq_printf(p, "%-10s Cnt:%d ErrCnt:%d\n", "Interrupt", dev->isp_isr_cnt, dev->isp_err_cnt);

    if (sensor && sensor->fi.interval.numerator) {
        val = sensor->fi.interval.denominator / sensor->fi.interval.numerator;
    }
    seq_printf(p, "%-10s %s Format:%s Size:%dx%d@%dfps Offset(%d,%d)\n", "Input", sensor ? sensor->sd->name : NULL,
               sdev->in_fmt.name, sdev->in_crop.width, sdev->in_crop.height, val, sdev->in_crop.left,
               sdev->in_crop.top);
    if (IS_HDR_RDBK(dev->hdr.op_mode)) {
        seq_printf(
            p, "%-10s mode:frame%d (frame:%d rate:%dms %s time:%dms frameloss:%d) cnt(total:%d X1:%d X2:%d X3:%d)\n",
            "Isp Read", dev->rd_mode - 0x03, dev->dmarx_dev.cur_frame.id,
            (u32)(dev->dmarx_dev.cur_frame.timestamp - dev->dmarx_dev.pre_frame.timestamp) / 0x3E8 / 0x3E8,
            (dev->isp_state & ISP_FRAME_END) ? "idle" : "working", sdev->dbg.interval / 0x3E8 / 0x3E8,
            sdev->dbg.frameloss, dev->rdbk_cnt, dev->rdbk_cnt_x1, dev->rdbk_cnt_x2, dev->rdbk_cnt_x3);
    } else {
        seq_printf(p, "%-10s frame:%d %s time:%dms v-blank:%dus\n", "Isp online", sdev->dbg.id,
                   (dev->isp_state & ISP_FRAME_END) ? "idle" : "working", sdev->dbg.interval / 0x3E8 / 0x3E8,
                   sdev->dbg.delay / 0x3E8);
    }

    if (dev->br_dev.en) {
        seq_printf(p, "%-10s rkispp%d Format:%s%s Size:%dx%d (frame:%d rate:%dms frameloss:%d)\n", "Output",
                   dev->dev_id, (dev->br_dev.work_mode & ISP_ISPP_FBC) ? "FBC" : "YUV",
                   (dev->br_dev.work_mode & ISP_ISPP_422) ? "422" : "420", dev->br_dev.crop.width,
                   dev->br_dev.crop.height, dev->br_dev.dbg.id, dev->br_dev.dbg.interval / 0x3E8 / 0x3E8,
                   dev->br_dev.dbg.frameloss);
    }
    for (val = 0; val < RKISP_MAX_STREAM; val++) {
        struct rkisp_stream *stream = &dev->cap_dev.stream[val];

        if (!stream->streaming) {
            continue;
        }
        seq_printf(p, "%-10s %s Format:%c%c%c%c Size:%dx%d (frame:%d rate:%dms delay:%dms frameloss:%d)\n", "Output",
                   stream->vnode.vdev.name, stream->out_fmt.pixelformat, stream->out_fmt.pixelformat >> 0x08,
                   stream->out_fmt.pixelformat >> 0x10, stream->out_fmt.pixelformat >> 0x18, stream->out_fmt.width,
                   stream->out_fmt.height, stream->dbg.id, stream->dbg.interval / 0x3E8 / 0x3E8,
                   stream->dbg.delay / 0x3E8 / 0x3E8, stream->dbg.frameloss);
    }

    switch (dev->isp_ver) {
        case ISP_V20:
            isp20_show(dev, p);
            break;
        case ISP_V21:
            isp21_show(dev, p);
            break;
        case ISP_V30:
            if (dev->hw_dev->is_unite) {
                isp30_unite_show(dev, p);
            } else {
                isp30_show(dev, p);
            }
            break;
        default:
            break;
    }

    seq_printf(p, "%-10s %s Cnt:%d\n", "Monitor", dev->hw_dev->monitor.is_en ? "ON" : "OFF",
               dev->hw_dev->monitor.retry);
    return 0;
}

static int isp_open(struct inode *inode, struct file *file)
{
    struct rkisp_device *data = PDE_DATA(inode);

    return single_open(file, isp_show, data);
}

static const struct proc_ops ops = {
    .proc_open = isp_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

int rkisp_proc_init(struct rkisp_device *dev)
{
    dev->procfs = proc_create_data(dev->name, 0, NULL, &ops, dev);
    if (!dev->procfs) {
        return -EINVAL;
    }
    return 0;
}

void rkisp_proc_cleanup(struct rkisp_device *dev)
{
    if (dev->procfs) {
        remove_proc_entry(dev->name, NULL);
    }
    dev->procfs = NULL;
}

#endif /* CONFIG_PROC_FS */
