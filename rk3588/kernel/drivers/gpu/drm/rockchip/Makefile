# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI) in XFree86 4.1.0 and higher.

LINUXINCLUDE    += \
		-I$(srctree)/drivers/gpu/drm/rockchip

obj-$(CONFIG_ROCKCHIP_DW_MIPI_DSI) += dw-mipi-dsi2-rockchip.o
obj-$(CONFIG_ROCKCHIP_DW_DP) += dw-dp.o
obj-$(CONFIG_ROCKCHIP_DRM_DEBUG) += rockchip_drm_debugfs.o
obj--$(CONFIG_ROCKCHIP_VCONN) += rockchip_drm_vconn.o

obj-y += rockchip_drm_logo.o rockchip_drm_vop2.o rockchip_vop2_reg.o
