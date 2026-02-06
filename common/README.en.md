# rockchip hardware

## Introduction
Merge the same parts of rockchip soc 
## Constraints

Currently,the same parts rk3566 rk3588 and rk3568 are supported.
## Repositories Involved
1.  device_soc_rockchip/common/hardware/display


-display_device:display device support
-display_gfx:manages memory of the display module
-display_gralloc:implements hardware acceleration

2.  device_soc_rockchip/common/hardware/gpu

- add libmali lib "libmali-bifrost-g52-g2p0-ohos.so lib"

3.  device_soc_rockchip/common/hardware/isp

- implement isp interfaces

4.  device_soc_rockchip/common/hardware/mpp

- implement mpp interfaces

5.  device_soc_rockchip/common/hardware/rga

- implement rga interfaces

6.  device_soc_rockchip/common/hardware/omx_il

- implement omx_il interfaces

7.  device_soc_rockchip/common/hardware/codec

- implement codec interfaces

8.  device_soc_rockchip/common/sdk_linux

- Kernel part of the modified files

9.  device_soc_rockchip/common/vendor

- Kernel part of the new files

10.  device_soc_rockchip/common/kernel

- Kernel part of the new files
