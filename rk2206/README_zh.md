# device_rockchip_rk2206

## 介绍

### 小凌派-RK2206开发板

小凌派-RK2206开发板主控器为瑞芯微高性能、高性价比的RK2206芯片，搭载OpenHarmony操作系统，内置WiFi/AP功能、NFC功能、液晶显示接口以及E53接口，E53接口兼容各类传感器模块，便于多样化的IoT物联网应用；目前小凌派-RK2006开发板已经拥有20+个成熟的应用案例，以及完善的教学课程，可广泛的应用于智慧城市、智能家居、智慧教学、智慧车载以及智慧医疗等多种场景。

参考[小凌派-RK2206开发板简介](https://gitee.com/openharmony-sig/vendor-lockzhiner/tree/master/lingpi)

## rockchip目录

```
device/soc/rockchip/rk2206
├── adapter                      # 
|   └── hals                     # hals适配目录
├── hardware                     # rk2206底层静态库和头文件
|   └── docs                     # rk2206底层的说明文档
|   └── include                  # rk2206底层变量和函数头文件
|   └── libhardware.a            # rk2206底层静态库
├── hdf_config                   # hdf驱动配置
├── hdf_driver                   # hdf驱动程序
|── sdk_liteos                   # RK2206芯片liteos
|   └── image                    # 打包shell脚本
|   └── liteos_m                 # 内核配置和link文件
|   └── loader                   # 第一阶镜像文件
|   └── platform                 # 程序，包括主函数，打印函数
|── tools                        # 打包工具、烧写工具等
```

仓库包含编译构建脚本和打包镜像工具。

系统要求： Ubuntu 20.04.3 LTS 64位系统版本。

编译环境搭建包含如下几步：

1. 准备工作
2. 安装VBox虚拟机
3. 安装Ubuntu操作系统
4. 安装的库和工具
5. 安装python3
6. 安装hb
7. 安装arm-none-eabi-gcc
8. 编译流程
9. 烧录打印

## 准备工作

准备一台电脑，安装Windows系统

## 安装VBox虚拟机

下载网址（百度云）：https://pan.baidu.com/s/1EYgUAO1_2N0GluF7h8HvBQ
提取码：eekp

## 下载Ubuntu操作系统

下载网址（百度云）：https://pan.baidu.com/s/1IfT0onLb1kcoByhOUU-kyAeik
提取码：eikl

## 安装的库和工具

> - 通常系统默认安装samba、vim等常用软件。

> - 使用如下apt-get命令安装下面的库和工具：

```
sudo apt-get install build-essential gcc g++ make zlib* libffi-dev e2fsprogs pkg-config flex bison perl bc openssl libssl-dev libelf-dev libc6-dev-amd64 binutils binutils-dev libdwarf-dev u-boot-tools mtd-utils gcc-arm-linux-gnueabi
```

## 安装Python3

1. 打开Linux编译服务器终端。
2. 输入如下命令，查看python版本号：
   
   ```
   python3 --version
   ```
   
   （1）运行如下命令，查看Ubuntu版本：
   
   ```
   cat /etc/issue
   ```
   
   （2）ubuntu 20安装python。
   
   ```
   sudo apt-get install python3
   ```
3. 设置python和python3软链接为python3。
   
   ```
   sudo update-alternatives --install /usr/bin/python python /usr/bin/python3.8 1
   sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.8 1
   ```
4. 安装并升级Python包管理工具（pip3），任选如下一种方式。
   
   ***（1)命令行方式***
   
   ```
   sudo apt-get install python3-setuptools python3-pip -y
   sudo pip3 install --upgrade pip
   ```
   
   ***（2）安装包方式***
   
   ```
   curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
   python get-pip.py
   ```

## 安装hb

### 安装方法

1. 运行如下命令安装hb
   
   ```
   pip3 uninstall ohos-build # 如果安装了hb,先卸载
   pip3 install ohos-build # 安装hb
   pip3 install build/lite
   ```
2. 设置环境变量
   
   ```
   vim ~/.bashrc
   ```
   
   将以下命令拷贝到.bashrc文件的最后一行，保存并退出。
   
   ```
   export PATH=~/.local/bin:$PATH
   ```
   
   执行如下命令更新环境变量。
   
   ```
   source ~/.bashrc
   ```
3. 执行"hb -h"，有打印以下信息即表示安装成功：
   
   ```
   usage: hb
   
   OHOS build system
   
   positional arguments:
     {build,set,env,clean}
       build               Build source code
       set                 OHOS build settings
       env                 Show OHOS build env
       clean               Clean output
   
   optional arguments:
     -h, --help            show this help message and exit
   ```

## 安装arm-none-eabi-gcc

1. Ubuntu自动安装arm-none-eabi-gcc
   
   ```shell
   sudo apt-get install gcc-arm-none-eabi
   ```
2. 手动安装arm-none-eabi-gcc
   
   下载[arm-none-eabi-gcc 编译工具下载](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2)
   
   解压 [gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2](https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2) 安装包至\~/toolchain/路径下。
   
   ```shell
   mkdir -p ~/toolchain/
   tar -jxvf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C ~/toolchain/
   ```
   
   设置环境变量。
   
   ```
   vim ~/.bashrc
   ```
   
   将以下命令拷贝到.bashrc文件的最后一行，保存并退出。
   
   ```
   export PATH=~/toolchain/gcc-arm-none-eabi-10.3-2021.10/bin:$PATH
   ```
   
   生效环境变量。
   
   ```
   source ~/.bashrc
   ```

## 编译流程

编译步骤如下所示：

```shell
hb set -root .
hb set
lockzhiner
   lingpi

选择lingpi

hb build -f
```

## 烧录打印

### windows烧录打印

1. 进入DriverAssitant目录，点击 DriverInstall.exe，安装驱动文件。
2. 进入RKDevTool目录，点击 RKDevTool.exe。
3. 进入烧写工具主界面，选择“下载镜像”界面
4. 使用USB线，连接小凌派-RK2206开发板的USB烧写口
5. 在小凌派-RK2206开发板上，长按MaskRom按键（详见板子的按钮），点击ReSet按键（详见板子的按钮），烧写工具出现：“发现一个MASKROM设备”
6. 点击“执行”按钮，下载烧写
7. 使用USB串口线，连接USB_UART口，打开串口工具(波特率:115200)，reset（详见板子的按钮） 启动板子，查看log。

# 相关仓

## 小凌派-RK2206开发板

* [device/board/lockzhiner](https://gitee.com/openharmony-sig/device_board_lockzhiner)
* [vendor/lockzhiner](https://gitee.com/openharmony-sig/vendor-lockzhiner)

