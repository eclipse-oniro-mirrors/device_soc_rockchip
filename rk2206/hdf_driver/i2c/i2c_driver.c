/*
 * Copyright (c) 2022 FuZhou Lockzhiner Electronic Co., Ltd. All rights reserved.
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "hdf_device_desc.h"
#include "device_resource_if.h"
#include "osal_mem.h"
#include "i2c_core.h"
#include "i2c_if.h"

#include "lz_hardware.h"

#define PRINT_ERR(fmt, args...)     do { \
    printf("%s, %d, error: "fmt, __func__, __LINE__, ##args); \
} while (0)

#define PRINT_WARR(fmt, args...)    do { \
    if (1) printf("%s, %d, warr: "fmt, __func__, __LINE__, ##args); \
} while (0)

#define PRINT_LOG(fmt, args...)    do { \
    if (1) printf("%s, %d, log: "fmt, __func__, __LINE__, ##args); \
} while (0)

struct i2c_bus {
    uint32_t bus;
    uint32_t id;
    uint32_t mode;
    uint32_t freq;
    uint32_t scl_gpio;
    uint32_t scl_func;
    uint32_t scl_type;
    uint32_t scl_drv;
    uint32_t scl_dir;
    uint32_t scl_val;
    uint32_t scl_mux;
    uint32_t sda_gpio;
    uint32_t sda_func;
    uint32_t sda_type;
    uint32_t sda_drv;
    uint32_t sda_dir;
    uint32_t sda_val;
    uint32_t sda_mux;
};

static int32_t i2cdrv_initdevice(const struct i2c_bus *bus)
{
    unsigned int ret;
    I2cBusIo i2cBus;
    unsigned int i2cBusId;
    unsigned int i2cFreq;
    GpioID sclGpio, sdaGpio;
    MuxFunc sclMux, sdaMux;

    if (bus == NULL) {
        PRINT_ERR("%s: bus is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    i2cBusId = (unsigned int)(bus->bus);
    i2cFreq = (unsigned int)(bus->freq);
    /* 初始化i2c配置结构体 */
    i2cBus.id = (FuncID)(bus->id);
    i2cBus.mode = (FuncMode)(bus->mode);
    i2cBus.scl.gpio = (GpioID)(bus->scl_gpio);
    i2cBus.scl.func = (MuxFunc)(bus->scl_func);
    i2cBus.scl.type = (PullType)(bus->scl_type);
    i2cBus.scl.drv = (DriveLevel)(bus->scl_drv);
    i2cBus.scl.dir = (LzGpioDir)(bus->scl_dir);
    i2cBus.scl.val = (LzGpioValue)(bus->scl_val);
    i2cBus.sda.gpio = (GpioID)(bus->sda_gpio);
    i2cBus.sda.func = (MuxFunc)(bus->sda_func);
    i2cBus.sda.type = (PullType)(bus->sda_type);
    i2cBus.sda.drv = (DriveLevel)(bus->sda_drv);
    i2cBus.sda.dir = (LzGpioDir)(bus->sda_dir);
    i2cBus.sda.val = (LzGpioValue)(bus->sda_val);
    /* 初始化引脚复用寄存器 */
    sclGpio = (GpioID)(bus->scl_gpio);
    sclMux = (MuxFunc)(bus->scl_mux);
    sdaGpio = (GpioID)(bus->sda_gpio);
    sdaMux = (MuxFunc)(bus->sda_mux);

    ret = I2cIoInit(i2cBus);
    if (ret != LZ_HARDWARE_SUCCESS) {
        PRINT_ERR("%s: I2cIoInit failed(%u)\n", __func__, ret);
        return HDF_FAILURE;
    }

    ret = LzI2cInit(i2cBusId, i2cFreq);
    if (ret != LZ_HARDWARE_SUCCESS) {
        PRINT_ERR("%s: LzI2cInit failed(%u)\n", __func__, ret);
        return HDF_FAILURE;
    }

    PinctrlSet(sclGpio, sclMux, PULL_KEEP, DRIVE_KEEP);
    PinctrlSet(sdaGpio, sdaMux, PULL_KEEP, DRIVE_KEEP);

    return HDF_SUCCESS;
}

static void i2cdrv_deinitdevice(const struct i2c_bus *bus)
{
    unsigned int i2cBusId;

    if (bus == NULL) {
        PRINT_ERR("%s: bus is null\n", __func__);
        return;
    }

    i2cBusId = (unsigned int)(bus->bus);
    LzI2cDeinit(i2cBusId);
}

static int32_t i2cdrv_readdrs(struct DeviceResourceNode *node, struct i2c_bus *bus)
{
    int32_t ret;
    struct DeviceResourceIface *iface = NULL;

    if (node == NULL) {
        PRINT_ERR("%s: node is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (bus == NULL) {
        PRINT_ERR("%s: bus is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (iface == NULL) {
        PRINT_ERR("%s: iface is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (iface->GetUint32 == NULL) {
        PRINT_ERR("%s: GetUint32 is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    memset_s(bus, sizeof(struct i2c_bus), 0, sizeof(struct i2c_bus));

    ret = iface->GetUint32(node, "bus", &bus->bus, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(bus) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "id", &bus->id, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(id) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "mode", &bus->mode, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(mode) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "freq", &bus->freq, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(freq) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "scl_gpio", &bus->scl_gpio, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(scl_gpio) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "scl_func", &bus->scl_func, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(scl_func) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "scl_type", &bus->scl_type, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(scl_type) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "scl_drv", &bus->scl_drv, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(scl_drv) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "scl_dir", &bus->scl_dir, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(scl_dir) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "scl_val", &bus->scl_val, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(scl_val) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "scl_mux", &bus->scl_mux, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(scl_mux) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "sda_gpio", &bus->sda_gpio, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(sda_gpio) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "sda_func", &bus->sda_func, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(sda_func) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "sda_type", &bus->sda_type, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(sda_type) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "sda_drv", &bus->sda_drv, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(sda_drv) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "sda_dir", &bus->sda_dir, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(sda_dir) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "sda_val", &bus->sda_val, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(sda_val) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "sda_mux", &bus->sda_mux, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(sda_mux) failed\n", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t i2cdrv_transfer(struct I2cCntlr *cntlr, struct I2cMsg *msgs, int16_t count)
{
#define I2C_MSGS_MAX        2 // LzI2cMsgs最大数组个数
    int16_t i;
    unsigned int ret;
    unsigned int i2cBusId;
    struct i2c_bus *bus;
    struct I2cMsg *msg, *msg2;
    unsigned short addr;
    unsigned char *buf;
    unsigned short buf_len;
    LzI2cMsg lzI2cMsgs[I2C_MSGS_MAX];

    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->priv == NULL) {
        PRINT_ERR("%s: cntlr->priv is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (msgs == NULL) {
        PRINT_ERR("%s: msgs is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    bus = (struct i2c_bus *)cntlr->priv;
    i2cBusId = (unsigned int)(bus->bus);

    for (i = 0; i < count; i++) {
        msg = &msgs[i];

        if (msg->flags == I2C_FLAG_READ) {
            addr = (unsigned short)(msg->addr);
            *buf = (unsigned char *)(msg->buf);
            buf_len = (unsigned short)(msg->len);

            ret = LzI2cRead(i2cBusId, addr, buf, buf_len);
            if (ret != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("%s: LzI2cRead failed(%u)\n", __func__, ret);
                return i;
            }
        } else if (msg->flags == I2C_FLAG_STOP) {
            /* 本字符串是写，下一个字符串是读，一起操作 */
            i++;
            msg2 = &msgs[i];

            lzI2cMsgs[0].addr = (unsigned short)(msg->addr);
            lzI2cMsgs[0].flags = (unsigned short)(msg->flags);
            lzI2cMsgs[0].len = (unsigned short)(msg->len);
            lzI2cMsgs[0].buf = (unsigned char *)(msg->buf);

            lzI2cMsgs[1].addr = (unsigned short)(msg2->addr);
            lzI2cMsgs[1].flags = (unsigned short)(msg2->flags);
            lzI2cMsgs[1].len = (unsigned short)(msg2->len);
            lzI2cMsgs[1].buf = (unsigned char *)(msg2->buf);

            ret = LzI2cTransfer(i2cBusId, &lzI2cMsgs[0], I2C_MSGS_MAX);
            if (ret != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("%s: LzI2cTransfer failed(%u)\n", __func__, ret);
                return i;
            }
        } else {
            addr = (unsigned short)(msg->addr);
            *buf = (unsigned char *)(msg->buf);
            buf_len = (unsigned short)(msg->len);

            ret = LzI2cWrite(i2cBusId, addr, buf, buf_len);
            if (ret != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("%s: LzI2cWrite failed(%u)\n", __func__, ret);
                return i;
            }
        }
    }

    return i;
}

static struct I2cMethod m_i2c_method = {
    .transfer = i2cdrv_transfer,
};

static int32_t i2cdrv_init(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct I2cCntlr *cntlr = NULL;
    struct i2c_bus *bus = NULL;

    PLAT_LOGI("%s: Enter", __func__);
    if ((device == NULL) || (device->property == NULL)) {
        PRINT_ERR("%s: device or property is null\n", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    cntlr = (struct I2cCntlr *)OsalMemAlloc(sizeof(struct I2cCntlr));
    bus = (struct i2c_bus *)OsalMemAlloc(sizeof(struct i2c_bus));

    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        if (bus != NULL) {
            OsalMemFree(bus);
            bus = NULL;
        }
        if (cntlr != NULL) {
            OsalMemFree(cntlr);
            cntlr = NULL;
        }
        return HDF_ERR_MALLOC_FAIL;
    }
    if (bus == NULL) {
        PRINT_ERR("%s: bus is null\n", __func__);
        if (bus != NULL) {
            OsalMemFree(bus);
            bus = NULL;
        }
        if (cntlr != NULL) {
            OsalMemFree(cntlr);
            cntlr = NULL;
        }
        return HDF_ERR_MALLOC_FAIL;
    }

    memset_s(cntlr, sizeof(struct I2cCntlr), 0, sizeof(struct I2cCntlr));
    cntlr->ops = &m_i2c_method;
    device->priv = (void *)cntlr;

    ret = i2cdrv_readdrs(device->property, bus);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: i2cdrv_readdrs error\n", __func__);
        cntlr->priv = NULL;
        cntlr->ops = NULL;
        device->priv = NULL;
        if (bus != NULL) {
            OsalMemFree(bus);
            bus = NULL;
        }
        if (cntlr != NULL) {
            OsalMemFree(cntlr);
            cntlr = NULL;
        }
        return ret;
    }

    ret = i2cdrv_initdevice(bus);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: i2cdrv_readdrs error\n", __func__);
        i2cdrv_deinitdevice(bus);
        cntlr->priv = NULL;
        cntlr->ops = NULL;
        device->priv = NULL;
        if (bus != NULL) {
            OsalMemFree(bus);
            bus = NULL;
        }
        if (cntlr != NULL) {
            OsalMemFree(cntlr);
            cntlr = NULL;
        }
        return ret;
    }

    cntlr->priv = (void *)bus;

    ret = I2cCntlrAdd(cntlr);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: i2c cntlr add failed\n", __func__);
        I2cCntlrRemove(cntlr);
        i2cdrv_deinitdevice(bus);
        cntlr->priv = NULL;
        cntlr->ops = NULL;
        device->priv = NULL;
        if (bus != NULL) {
            OsalMemFree(bus);
            bus = NULL;
        }
        if (cntlr != NULL) {
            OsalMemFree(cntlr);
            cntlr = NULL;
        }
        return ret;
    }

    PRINT_LOG("i2c service: %s init success!\n", HdfDeviceGetServiceName(device));
    return HDF_SUCCESS;
}

static void i2cdrv_release(struct HdfDeviceObject *device)
{
    struct I2cCntlr *cntlr = NULL;
    struct i2c_bus *bus = NULL;

    if (device == NULL) {
        PRINT_ERR("%s: device is null\n", __func__);
        return;
    }

    cntlr = (struct I2cCntlr *)device->priv;
    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        return;
    }

    bus = (struct i2c_bus *)cntlr->priv;
    if (bus == NULL) {
        PRINT_ERR("%s: bus is null\n", __func__);
        return;
    }

    I2cCntlrRemove(cntlr);
    i2cdrv_deinitdevice(bus);
    cntlr->priv = NULL;
    cntlr->ops = NULL;
    device->priv = NULL;
    if (bus != NULL) {
        OsalMemFree(bus);
        bus = NULL;
    }
    if (cntlr != NULL) {
        OsalMemFree(cntlr);
        cntlr = NULL;
    }

    PRINT_LOG("i2c service: %s release\n", HdfDeviceGetServiceName(device));
}

static int32_t i2cdrv_bind(struct HdfDeviceObject *device)
{
    return HDF_SUCCESS;
}

struct HdfDriverEntry g_i2cDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_PLATFORM_I2C",
    .Init = i2cdrv_init,
    .Release = i2cdrv_release,
    .Bind = i2cdrv_bind,
};

HDF_INIT(g_i2cDriverEntry);
