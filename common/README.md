# rockchip Common

#### 简介
合并rockchip soc公共部分
#### 约束
目前支持rk3566 rk3588 和 rk3568公共部分
#### 对应仓库

1. device_soc_rockchip/common/hardware/display

-display_device:显示设备支持
-display_gfx:提供硬件加速接口
-display_gralloc:负责显示模块内存的管理

2.  device_soc_rockchip/common/hardware/gpu

- rockchip libmali-bifrost-g52-g2p0-ohos.so

3.  device_soc_rockchip/common/hardware/isp

- rockchip isp 相关服务及效果文件

4.  device_soc_rockchip/common/hardware/mpp

- 实现多媒体接口

5.  device_soc_rockchip/common/hardware/rga

- 实现rga接口

6.  device_soc_rockchip/common/hardware/omx_il

- 实现omx_il接口

7.  device_soc_rockchip/common/hardware/codec

- 实现codec接口
8.  device_soc_rockchip/common/sdk_linux

- kernel部分修改文件

9.  device_soc_rockchip/common/vendor
- kernel部分新增文件

10.  device_soc_rockchip/common/kernel
- kernel部分新增文件
