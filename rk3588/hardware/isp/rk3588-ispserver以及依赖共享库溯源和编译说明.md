OpenHarmony 中device_soc_rockchip仓rk3588/hardware/isp/中 ispserver 和 librkaiq.z.so 溯源和编译说明
====================================================

1. 获取mpp源码，请参考以下网址：
https://gitee.com/hihope-rk3588/camera_engine_rkaiq_linux

2. 构建指导
------------------

```
# 获取代码
git clone https://gitee.com/hihope-rk3588/camera_engine_rkaiq_linux.git

将下载的源码目录移动至device/soc/rockchip/rk3588/hardware/isp/下

# 编译isp
修改device/soc/rockchip/rk3588/hardware/BUILD.gn 增加编译项，如下
group("hardware_group") {
  deps = [
    "//device/soc/rockchip/rk3588/hardware/gpu:mali-g610-ohos",
    "//device/soc/rockchip/rk3588/hardware/gpu:firmware",
    "//device/soc/rockchip/rk3588/hardware/isp:isp",
    /*下面两条是新增加的编译项*/
    "//device/soc/rockchip/rk3588/hardware/isp/camera_engine_rkaiq_linux/rkaiq_3A_server:rkaiq_3A_server",
    "//device/soc/rockchip/rk3588/hardware/isp/camera_engine_rkaiq_linux:rkaiq",
    "//device/soc/rockchip/rk3588/hardware/mpp:mpp",
    "//device/soc/rockchip/rk3588/hardware/rga:rga",
    "//device/soc/rockchip/rk3588/hardware/wifi:ap6xxx"
  ]
}


编译整个工程或单独编译该模块：
./build.sh --product-name dayu210

out目录下获取新编译的文件：
out/rk3588/common/common/librkaiq.z.so
out/rk3588/common/common/rkaiq_3A_server

注：rkaiq_3A_server文件需要更名为ispserver后替换原有文件使用
```



----------------


