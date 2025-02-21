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

#include "gpio/gpio_core.h"
#include "device_resource_if.h"
#include "hdf_device_desc.h"
#include "hdf_log.h"
#include "stdio.h"

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

static struct GpioCntlr m_gpioCntlr;
static int32_t m_groupNum = 0;
static int32_t m_bitNum = 0;
#define GPIO_MAXSIZE            256
static uint8_t m_gpio_init_flag[GPIO_MAXSIZE];

static int32_t iodrv_initgpio(uint16_t gpio)
{
    GpioID gpio_id = (GpioID)(gpio);

    if (gpio >= GPIO_MAXSIZE) {
        PRINT_ERR("%s: gpio(%d) >= GPIO_MAXSIZE(%d)", __func__, gpio, GPIO_MAXSIZE);
        return HDF_FAILURE;
    }

    if (m_gpio_init_flag[gpio] == 1) {
        return HDF_SUCCESS;
    }

    PinctrlSet(gpio_id, MUX_FUNC0, PULL_KEEP, DRIVE_LEVEL0);
    LzGpioInit(gpio_id);
    m_gpio_init_flag[gpio] = 1;

    return HDF_SUCCESS;
}

static int32_t iodrv_setdir(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t dir)
{
    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null", __func__);
        return HDF_FAILURE;
    }
    if (gpio >= cntlr->count) {
        PRINT_ERR("%s: gpio(%d) >= cntlr->count(%d)", __func__, gpio, cntlr->count);
        return HDF_FAILURE;
    }
    if ((dir != GPIO_DIR_IN) && (dir != GPIO_DIR_OUT)) {
        PRINT_ERR("%s: dir(%d) out of the range", __func__, dir);
        return HDF_FAILURE;
    }

    GpioID gpio_id = (GpioID)(gpio);
    LzGpioDir gpio_dir = (dir == GPIO_DIR_IN) ? (LZGPIO_DIR_IN) : (LZGPIO_DIR_OUT);

    iodrv_initgpio(gpio);
    LzGpioSetDir(gpio_id, gpio_dir);

    return HDF_SUCCESS;
}

static int32_t iodrv_getdir(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t *dir)
{
    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null", __func__);
        return HDF_FAILURE;
    }
    if (gpio >= cntlr->count) {
        PRINT_ERR("%s: gpio(%d) >= cntlr->count(%d)", __func__, gpio, cntlr->count);
        return HDF_FAILURE;
    }
    if (dir == NULL) {
        PRINT_ERR("%s: dir is null", __func__);
        return HDF_FAILURE;
    }

    GpioID gpio_id = (GpioID)(gpio);
    LzGpioDir gpio_dir;
    unsigned int ret;

    iodrv_initgpio(gpio);

    ret = LzGpioGetDir(gpio_id, &gpio_dir);
    if (ret != LZ_HARDWARE_SUCCESS) {
        PRINT_ERR("%s: LzGpioGetDir error", __func__);
        return HDF_FAILURE;
    }

    *dir = (gpio_dir == LZGPIO_DIR_IN) ? (GPIO_DIR_IN) : (GPIO_DIR_OUT);

    return HDF_SUCCESS;
}

static int32_t iodrv_write(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t value)
{
    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null", __func__);
        return HDF_FAILURE;
    }
    if (gpio >= cntlr->count) {
        PRINT_ERR("%s: gpio(%d) >= cntlr->count(%d)", __func__, gpio, cntlr->count);
        return HDF_FAILURE;
    }

    GpioID gpio_id = (GpioID)(gpio);
    LzGpioValue gpio_value = (value == 0) ? (LZGPIO_LEVEL_LOW) : (LZGPIO_LEVEL_HIGH);

    iodrv_initgpio(gpio);
    LzGpioSetDir(gpio_id, LZGPIO_DIR_OUT);
    LzGpioSetVal(gpio_id, gpio_value);

    return HDF_SUCCESS;
}

static int32_t iodrv_read(struct GpioCntlr *cntlr, uint16_t gpio, uint16_t *value)
{
    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null", __func__);
        return HDF_FAILURE;
    }
    if (gpio >= cntlr->count) {
        PRINT_ERR("%s: gpio(%d) >= cntlr->count(%d)", __func__, gpio, cntlr->count);
        return HDF_FAILURE;
    }
    if (value == NULL) {
        PRINT_ERR("%s: value is null", __func__);
        return HDF_FAILURE;
    }

    GpioID gpio_id = (GpioID)(gpio);
    LzGpioValue gpio_value;
    unsigned int ret;

    iodrv_initgpio(gpio);

    ret = LzGpioGetVal(gpio_id, &gpio_value);
    if (ret != LZ_HARDWARE_SUCCESS) {
        PRINT_ERR("%s: LzGpioGetVal error", __func__);
        return HDF_FAILURE;
    }

    *value = (gpio_value == LZGPIO_LEVEL_LOW) ? (0) : (1);

    return HDF_SUCCESS;
}

static int32_t iodrv_readdrs(struct DeviceResourceNode *node, uint32_t *groupNum, uint32_t *bitNum)
{
    int32_t ret;
    struct DeviceResourceIface *drsOps = NULL;

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL) {
        PRINT_ERR("%s: invalid drs ops!", __func__);
        return HDF_FAILURE;
    }
    if (drsOps->GetUint32 == NULL) {
        PRINT_ERR("%s: GetUint32 failed!", __func__);
        return HDF_FAILURE;
    }
    if (groupNum == NULL) {
        PRINT_ERR("%s: groupNum is null!", __func__);
        return HDF_FAILURE;
    }
    if (bitNum == NULL) {
        PRINT_ERR("%s: bitNum is null!", __func__);
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint32(node, "groupNum", groupNum, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: read groupNum failed!", __func__);
    }

    ret = drsOps->GetUint32(node, "bitNum", bitNum, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: read bitNum failed!", __func__);
    }

    return HDF_SUCCESS;
}

static struct GpioMethod m_gpio_method = {
    .request = NULL,
    .release = NULL,
    .write = iodrv_write,
    .read = iodrv_read,
    .setDir = iodrv_setdir,
    .getDir = iodrv_getdir,
    .toIrq = NULL,
    .setIrq = NULL,
    .unsetIrq = NULL,
    .enableIrq = NULL,
    .disableIrq = NULL,
};

static int32_t iodrv_init(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct GpioCntlr *cntlr = &m_gpioCntlr;
    uint32_t i;

    PRINT_LOG("%s: Enter\n", __func__);
    if ((device == NULL) || (device->property == NULL)) {
        PRINT_ERR("%s: device or property is null!", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = iodrv_readdrs(device->property, &m_groupNum, &m_bitNum);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: read drs failed(%d)", __func__, ret);
        return ret;
    }

    for (i = 0; i < GPIO_MAXSIZE; i++) {
        m_gpio_init_flag[i] = 0;
    }

    cntlr->start = 0;
    cntlr->count = m_groupNum * m_bitNum;
    cntlr->ops = &m_gpio_method;
    cntlr->priv = (void *)device->property;
    PlatformDeviceBind(&cntlr->device, device);
    ret = GpioCntlrAdd(cntlr);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: err add controller %d", __func__, ret);
        return ret;
    }

    PRINT_LOG("gpio service: %s init success!\n", HdfDeviceGetServiceName(device));
    return HDF_SUCCESS;
}

static void iodrv_release(struct HdfDeviceObject *device)
{
    struct Gpiocntlr *cntlr;

    if (device == NULL) {
        PRINT_ERR("%s: device is null", __func__);
        return;
    }

    cntlr = GpioCntlrFromHdfDev(device);
    if (cntlr == NULL) {
        PRINT_ERR("%s: no service binded!", __func__);
        return;
    }

    GpioCntlrRemove(cntlr);
    PRINT_LOG("gpio service: %s release!\n", HdfDeviceGetServiceName(device));
}

static int32_t iodrv_bind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

struct HdfDriverEntry g_gpioDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_PLATFORM_GPIO",
    .Bind = iodrv_bind,
    .Init = iodrv_init,
    .Release = iodrv_release,
};

HDF_INIT(g_gpioDriverEntry);
