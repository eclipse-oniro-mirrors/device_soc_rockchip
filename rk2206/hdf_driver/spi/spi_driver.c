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
#include "hdf_log.h"
#include "osal_mem.h"
#include "spi_core.h"
#include "spi_if.h"

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

struct spi_params {
    uint32_t bus;
    uint32_t id;
    uint32_t func_mode;
    uint32_t cs_gpio;
    uint32_t cs_func;
    uint32_t cs_type;
    uint32_t cs_drv;
    uint32_t cs_dir;
    uint32_t cs_val;
    uint32_t clk_gpio;
    uint32_t clk_func;
    uint32_t clk_type;
    uint32_t clk_drv;
    uint32_t clk_dir;
    uint32_t clk_val;
    uint32_t mosi_gpio;
    uint32_t mosi_func;
    uint32_t mosi_type;
    uint32_t mosi_drv;
    uint32_t mosi_dir;
    uint32_t mosi_val;
    uint32_t miso_gpio;
    uint32_t miso_func;
    uint32_t miso_type;
    uint32_t miso_drv;
    uint32_t miso_dir;
    uint32_t miso_val;
    uint32_t bitsPerWord;
    uint32_t firstBit;
    uint32_t mode;
    uint32_t csm;
    uint32_t speed;
    uint32_t isSlave;
};

static int32_t spidrv_readdrs(struct DeviceResourceNode *node, struct spi_params *params)
{
    int32_t ret;
    struct DeviceResourceIface *iface = NULL;

    if (node == NULL) {
        PRINT_ERR("%s: node is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (params == NULL) {
        PRINT_ERR("%s: params is null\n", __func__);
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

    memset_s(params, sizeof(struct spi_params), 0, sizeof(struct spi_params));

    ret = iface->GetUint32(node, "bus", &params->bus, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(bus) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "id", &params->id, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(id) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "func_mode", &params->func_mode, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(func_mode) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "cs_gpio", &params->cs_gpio, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(cs_gpio) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "cs_func", &params->cs_func, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(cs_func) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "cs_type", &params->cs_type, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(cs_type) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "cs_drv", &params->cs_drv, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(cs_drv) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "cs_dir", &params->cs_dir, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(cs_dir) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "cs_val", &params->cs_val, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(cs_val) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "clk_gpio", &params->clk_gpio, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(clk_gpio) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "clk_func", &params->clk_func, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(clk_func) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "clk_type", &params->clk_type, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(clk_type) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "clk_drv", &params->clk_drv, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(clk_drv) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "clk_dir", &params->clk_dir, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(clk_dir) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "clk_val", &params->clk_val, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(clk_val) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "mosi_gpio", &params->mosi_gpio, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(mosi_gpio) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "mosi_func", &params->mosi_func, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(mosi_func) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "mosi_type", &params->mosi_type, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(mosi_type) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "mosi_drv", &params->mosi_drv, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(mosi_drv) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "mosi_dir", &params->mosi_dir, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(mosi_dir) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "mosi_val", &params->mosi_val, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(mosi_val) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "miso_gpio", &params->miso_gpio, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(miso_gpio) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "miso_func", &params->miso_func, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(miso_func) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "miso_type", &params->miso_type, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(miso_type) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "miso_drv", &params->miso_drv, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(miso_drv) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "miso_dir", &params->miso_dir, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(miso_dir) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "miso_val", &params->miso_val, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(miso_val) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "bitsPerWord", &params->bitsPerWord, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(bitsPerWord) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "firstBit", &params->firstBit, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(firstBit) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "mode", &params->mode, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(mode) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "csm", &params->csm, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(csm) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "speed", &params->speed, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(speed) failed\n", __func__);
        return HDF_FAILURE;
    }

    ret = iface->GetUint32(node, "isSlave", &params->isSlave, 0);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: GetUint32(isSlave) failed\n", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t spidrv_initdevice(const struct spi_params *params)
{
    unsigned int ret;
    SpiBusIo bus;
    LzSpiConfig config;
    unsigned int bus_id;

    if (params == NULL) {
        PRINT_ERR("%s: params is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    bus_id = (unsigned int)params->bus;

    memset_s(&bus, sizeof(bus), 0, sizeof(bus));
    bus.id = (FuncID)params->id;
    bus.mode = (FuncMode)params->func_mode;
    bus.cs.gpio = (GpioID)params->cs_gpio;
    bus.cs.func = (MuxFunc)params->cs_func;
    bus.cs.type = (PullType)params->cs_type;
    bus.cs.drv = (DriveLevel)params->cs_drv;
    bus.cs.dir = (LzGpioDir)params->cs_dir;
    bus.cs.val = (LzGpioValue)params->cs_val;
    bus.mosi.gpio = (GpioID)params->mosi_gpio;
    bus.mosi.func = (MuxFunc)params->mosi_func;
    bus.mosi.type = (PullType)params->mosi_type;
    bus.mosi.drv = (DriveLevel)params->mosi_drv;
    bus.mosi.dir = (LzGpioDir)params->mosi_dir;
    bus.mosi.val = (LzGpioValue)params->mosi_val;
    bus.miso.gpio = (GpioID)params->miso_gpio;
    bus.miso.func = (MuxFunc)params->miso_func;
    bus.miso.type = (PullType)params->miso_type;
    bus.miso.drv = (DriveLevel)params->miso_drv;
    bus.miso.dir = (LzGpioDir)params->miso_dir;
    bus.miso.val = (LzGpioValue)params->miso_val;

    memset_s(&config, sizeof(config), 0, sizeof(config));
    config.bitsPerWord = (unsigned int)params->bitsPerWord;
    config.firstBit = (unsigned int)params->firstBit;
    config.mode = (unsigned int)params->mode;
    config.csm = (unsigned int)params->csm;
    config.speed = (unsigned int)params->speed;
    config.isSlave = (unsigned int)params->isSlave;

    ret = SpiIoInit(bus);
    if (ret != LZ_HARDWARE_SUCCESS) {
        PRINT_ERR("%s: SpiIoInit failed(%u)\n", __func__, ret);
    }

    ret = LzSpiInit(bus_id, config);
    if (ret != LZ_HARDWARE_SUCCESS) {
        PRINT_ERR("%s: LzSpiInit failed(%u)\n", __func__, ret);
    }

    return HDF_SUCCESS;
}

static void spidrv_deinitdevice(const struct spi_params *params)
{
    unsigned int bus_id;

    if (params == NULL) {
        PRINT_ERR("%s: params is null\n", __func__);
        return;
    }

    bus_id = (unsigned int)params->bus;
    LzSpiDeinit(bus_id);
}

static int32_t spidrv_open(struct SpiCntlr *cntlr)
{
    int32_t ret;
    struct spi_params *params = NULL;

    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->priv == NULL) {
        PRINT_ERR("%s: cntlr->priv is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    params = (struct spi_params *)cntlr->priv;

    ret = spidrv_initdevice(params);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: spidrv_initdevice error\n", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t spidrv_close(struct SpiCntlr *cntlr)
{
    struct spi_params *params = NULL;

    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->priv == NULL) {
        PRINT_ERR("%s: cntlr->priv is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    params = (struct spi_params *)cntlr->priv;

    /* 销毁硬件设备 */
    spidrv_deinitdevice(params);

    return HDF_SUCCESS;
}

static int32_t spidrv_setcfg(struct SpiCntlr *cntlr, struct SpiCfg *cfg)
{
    struct spi_params *params = NULL;

    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->priv == NULL) {
        PRINT_ERR("%s: cntlr->priv is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cfg == NULL) {
        PRINT_ERR("%s: cfg is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    params = (struct spi_params *)cntlr->priv;

    params->speed = cfg->maxSpeedHz;
    params->bitsPerWord = cfg->bitsPerWord;

    /* LiteOS的标准定义转化为LzHardware的定义 */
    params->mode = 0;
    if (cfg->mode & SPI_CLK_PHASE) {
        params->mode |= SPI_CPHA;
    }
    if (cfg->mode & SPI_CLK_POLARITY) {
        params->mode |= SPI_CPOL;
    }
    if (cfg->mode & SPI_MODE_3WIRE) {
        PRINT_ERR("%s: SPI_MODE_3WIRE is not support!\n", __func__);
    }
    if (cfg->mode & SPI_MODE_LOOP) {
        PRINT_ERR("%s: SPI_MODE_3WIRE is not support!\n", __func__);
    }
    if ((cfg->mode & SPI_MODE_LSBFE) == 0) {
        params->mode |= SPI_MSB;
    }
    if (cfg->mode & SPI_MODE_NOCS) {
        PRINT_ERR("%s: SPI_MODE_NOCS is not support!\n", __func__);
    }
    if (cfg->mode & SPI_MODE_CS_HIGH) {
        PRINT_ERR("%s: SPI_MODE_CS_HIGH is not support!\n", __func__);
    }
    if (cfg->mode & SPI_MODE_READY) {
        PRINT_ERR("%s: SPI_MODE_READY is not support!\n", __func__);
    }
    if (params->isSlave == 0) {
        params->mode |= SPI_SLAVE;
    } else {
        params->mode &= ~(SPI_SLAVE);
    }

    switch (cfg->transferMode) {
        case SPI_INTERRUPT_TRANSFER:
            break;

        case SPI_POLLING_TRANSFER:
            PRINT_ERR("%s: SPI_POLLING_TRANSFER is not support!\n", __func__);
            break;

        case SPI_DMA_TRANSFER:
            PRINT_ERR("%s: SPI_DMA_TRANSFER is not support!\n", __func__);
            break;

        default:
            PRINT_ERR("%s: %d is not support!\n", __func__, cfg->transferMode);
            break;
    }

    spidrv_initdevice(params);

    return HDF_SUCCESS;
}

static int32_t spidrv_getcfg(struct SpiCntlr *cntlr, struct SpiCfg *cfg)
{
    struct spi_params *params = NULL;

    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cntlr->priv == NULL) {
        PRINT_ERR("%s: cntlr->priv is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (cfg == NULL) {
        PRINT_ERR("%s: cfg is null\n", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    params = (struct spi_params *)cntlr->priv;

    cfg->maxSpeedHz = params->speed;
    cfg->bitsPerWord = params->bitsPerWord;

    /* LzHardware的定义转化为LiteOS的标准定义 */
    cfg->mode = 0;
    if (params->mode & SPI_CPHA) {
        cfg->mode |= SPI_CLK_PHASE;
    }
    if (params->mode & SPI_CPOL) {
        cfg->mode |= SPI_CLK_POLARITY;
    }
    if ((params->mode & SPI_MSB) == 0) {
        cfg->mode |= SPI_MODE_LSBFE;
    }

    cfg->transferMode = SPI_INTERRUPT_TRANSFER;

    return HDF_SUCCESS;
}

static int32_t spidrv_transfer(struct SpiCntlr *cntlr, struct SpiMsg *msgs, uint32_t count)
{
    unsigned int ret;
    struct spi_params *params = NULL;
    unsigned int bus_id;
    uint32_t i;
    struct SpiMsg *msg;
    LzSpiMsg lz_spi_msg;

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

    params = (struct spi_params *)cntlr->priv;
    bus_id = (unsigned int)params->bus;

    for (i = 0; i < count; i++) {
        msg = &msgs[i];

        if ((msg->wbuf != NULL) && (msg->rbuf != NULL)) {
            ret = LzSpiWriteAndRead(bus_id, 0, msg->wbuf, msg->rbuf, msg->len);
            if (ret != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("%s: LzSpiWriteAndRead error\n", __func__);
                return HDF_ERR_IO;
            }
        } else if ((msg->wbuf != NULL) && (msg->rbuf == NULL)) {
            ret = LzSpiWrite(bus_id, 0, msg->wbuf, msg->len);
            if (ret != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("%s: LzSpiWrite error\n", __func__);
                return HDF_ERR_IO;
            }
        } else if ((msg->wbuf == NULL) && (msg->rbuf != NULL)) {
            ret = LzSpiRead(bus_id, 0, msg->rbuf, msg->len);
            if (ret != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("%s: LzSpiRead error\n", __func__);
                return HDF_ERR_IO;
            }
        } else {
            PRINT_ERR("%s: i = %d, msg->wbuf or msg->rbuf is null\n", __func__, i);
            return HDF_ERR_INVALID_PARAM;
        }
    }

    return HDF_SUCCESS;
}

static struct SpiCntlrMethod m_spi_method = {
    .Open = spidrv_open,
    .Close = spidrv_close,
    .SetCfg = spidrv_setcfg,
    .GetCfg = spidrv_getcfg,
    .Transfer = spidrv_transfer,
};

static int32_t spidrv_bind(struct HdfDeviceObject *device)
{
    return HDF_SUCCESS;
}

static int32_t spidrv_init(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct SpiCntlr *cntlr = NULL;
    struct spi_params *params = NULL;

    HDF_LOGI("%s: Enter", __func__);
    if ((device == NULL) || (device->property == NULL)) {
        PRINT_ERR("%s: device or property is null\n", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    cntlr = (struct SpiCntlr *)OsalMemAlloc(sizeof(struct SpiCntlr));
    params = (struct spi_params *)OsalMemAlloc(sizeof(struct spi_params));

    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        if (cntlr != NULL) {
            OsalMemFree(cntlr);
            cntlr = NULL;
        }
        if (params != NULL) {
            OsalMemFree(params);
            params = NULL;
        }
        return HDF_ERR_MALLOC_FAIL;
    }
    if (params == NULL) {
        PRINT_ERR("%s: params is null\n", __func__);
        if (cntlr != NULL) {
            OsalMemFree(cntlr);
            cntlr = NULL;
        }
        if (params != NULL) {
            OsalMemFree(params);
            params = NULL;
        }
        return HDF_ERR_MALLOC_FAIL;
    }

    ret = spidrv_readdrs(device->property, params);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        if (cntlr != NULL) {
            OsalMemFree(cntlr);
            cntlr = NULL;
        }
        if (params != NULL) {
            OsalMemFree(params);
            params = NULL;
        }
        return ret;
    }

    /* 数据对齐 */
    cntlr->busNum = params->bus;
    cntlr->numCs = 1;
    cntlr->curCs = 0;
    cntlr->priv = (void *)params;
    /* 注册回调函数，并初始化SpiCntlr */
    cntlr->method = &m_spi_method;
    device->service = &cntlr->service;
    cntlr->device = device;
    cntlr->priv = (void *)params;

    PRINT_LOG("spi service: %s init success!\n", HdfDeviceGetServiceName(device));
    return HDF_SUCCESS;
}

static void spidrv_release(struct HdfDeviceObject *device)
{
    struct SpiCntlr *cntlr = NULL;
    struct spi_params *params = NULL;

    HDF_LOGI("%s: Enter", __func__);
    if ((device == NULL) || (device->property == NULL)) {
        PRINT_ERR("%s: device or property is null\n", __func__);
        return;
    }

    cntlr = SpiCntlrFromDevice(device);
    if (cntlr == NULL) {
        PRINT_ERR("%s: cntlr is null\n", __func__);
        return;
    }

    params = (struct spi_params *)cntlr->priv;

    /* 销毁SPI参数 */
    if (params != NULL) {
        OsalMemFree(params);
        params = NULL;
    }

    /* 销毁SpiCntlr */
    cntlr->priv = NULL;
    if (cntlr != NULL) {
        OsalMemFree(cntlr);
        cntlr = NULL;
    }

    PRINT_LOG("spi service: %s release!\n", HdfDeviceGetServiceName(device));
}

struct HdfDriverEntry g_spiDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_PLATFORM_SPI",
    .Init = spidrv_init,
    .Release = spidrv_release,
    .Bind = spidrv_bind,
};

HDF_INIT(g_spiDriverEntry);
