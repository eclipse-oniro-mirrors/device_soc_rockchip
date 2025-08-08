# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux kernel device drivers.
#
# 15 Sep 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#

obj-$(CONFIG_GENERIC_PHY)	+= phy/

# GPIO must come after pinctrl as gpios may need to mux pins etc
obj-$(CONFIG_PINCTRL)		+= pinctrl/
obj-$(CONFIG_GPIOLIB)		+= gpio/
obj-y				+= pci/
obj-y				+= video/

obj-y				+= clk/

# SOC specific infrastructure drivers.
obj-y				+= soc/

# regulators early, since some subsystems rely on them to initialize
obj-$(CONFIG_REGULATOR)		+= regulator/

# tty/ comes before char/ so that the VT console is the boot-time
# default.
#obj-y				+= tty/

# gpu/ comes after char for AGP vs DRM startup and after iommu
obj-y				+= gpu/

obj-y				+= mfd/
obj-$(CONFIG_DMA_SHARED_BUFFER) += dma-buf/

obj-y				+= net/

obj-$(CONFIG_INPUT)		+= input/
obj-y				+= media/

obj-y				+= power/
obj-$(CONFIG_CPU_FREQ)		+= cpufreq/

obj-y				+= mmc/

obj-$(CONFIG_PM_DEVFREQ)	+= devfreq/
obj-y				+= firmware/
obj-$(CONFIG_STAGING)		+= staging/
