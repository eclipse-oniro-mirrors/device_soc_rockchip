# SPDX-License-Identifier: GPL-2.0
#
# Rockchip Clock specific Makefile
#

LINUXINCLUDE    += \
		-I$(srctree)/drivers/clk/rockchip

obj-$(CONFIG_COMMON_CLK_ROCKCHIP) += clk-dclk-divider.o clk-pvtm.o

obj-$(CONFIG_ROCKCHIP_CLK_LINK)	+= clk-link.o

obj-$(CONFIG_CLK_RV1126)		+= clk-rv1126.o
obj-$(CONFIG_CLK_RK1808)        += clk-rk1808.o
obj-$(CONFIG_CLK_RK3568)        += clk-rk3568.o
obj-$(CONFIG_CLK_RK3588)		+= clk-rk3588.o
