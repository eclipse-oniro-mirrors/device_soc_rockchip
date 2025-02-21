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
#include <sys/mount.h>
#include <string.h>
#include "los_config.h"
#include "hdf_log.h"
#include "hdf_device_desc.h"
#include "device_resource_if.h"
#include "osal_mem.h"
#include "lfs_api.h"

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

/* 分配给用户的Flash空间为4M ~ 8MB */
#define FLASH_ADDRESS_USER_START       (0x400000)
#define FLASH_ADDRESS_USER_END         (0x800000)

struct fs_cfg {
    char *mount_point;
    uint32_t block_start;
    struct lfs_config lfs_cfg;
};

static struct fs_cfg m_fs_cfg[LOSCFG_LFS_MAX_MOUNT_SIZE] = {0};

/* 定义lfs_cfg的相关参数 */
#define LFSCFG_READSIZE_MAX             4
#define LFSCFG_PROGSIZE_MAX             4
#define LFSCFG_CACHESIZE_MAX            256
#define LFSCFG_LOCKAHEADSIZE_MAX        64
#define LFSCFG_BLOCKCYCLES_MAX          1000

static int32_t fs_get_partition(void *str, uint32_t *partition)
{
    int i;
    char *src = (char *)str;

    for (i = 0; i < (sizeof(m_fs_cfg) / sizeof(struct fs_cfg)); i++) {
        if (strcmp(m_fs_cfg[i].mount_point, src) == 0) {
            *partition = i;
            return HDF_SUCCESS;
        }
    }

    return HDF_ERR_NOT_SUPPORT;
}

static int32_t fs_read(const struct lfs_config *c,
                       lfs_block_t block,
                       lfs_off_t off,
                       void *dst,
                       lfs_size_t size)
{
    int32_t ret;
    struct fs_cfg *fs;
    uint32_t addr;
    uint32_t partition_offset;
    unsigned char *str;

    if (c == NULL) {
        PRINT_ERR("c is null\n");
        return LFS_ERR_INVAL;
    }
    if ((block * c->block_size + off) >= (c->block_size * c->block_count)) {
        PRINT_ERR("read_start(%d) >= block_count(%d)\n", (block * c->block_size + off),
                (c->block_size * c->block_count));
        return LFS_ERR_INVAL;
    }
    if ((block * c->block_size + off + size) > (c->block_size * c->block_count)) {
        PRINT_ERR("read_end(%d) > maxsize(%d)\n", (block * c->block_size + off + size),
                (c->block_size * c->block_count));
        return LFS_ERR_INVAL;
    }
    if (dst == NULL) {
        PRINT_ERR("dst is null\n");
        return LFS_ERR_INVAL;
    }

    ret = fs_get_partition(c->context, &partition_offset);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("c->context(%s) is not support\n", c->context);
        return LFS_ERR_INVAL;
    }

    fs = &m_fs_cfg[partition_offset];
    addr = ((fs->block_start + block) * c->block_size) + off;
    str = (unsigned char *)dst;

    ret = (unsigned int)FlashRead(addr, size, str);
    if (ret != LZ_HARDWARE_SUCCESS) {
        PRINT_ERR("FlashRead failed(%d)\n", ret);
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}

static int32_t fs_write(const struct lfs_config *c,
                        lfs_block_t block,
                        lfs_off_t off,
                        const void *src,
                        lfs_size_t size)
{
    unsigned int res;
    int32_t ret;
    struct fs_cfg *fs;
    uint32_t addr;
    uint32_t partition_offset;
    unsigned char *buffer;
    uint32_t buffer_length;
    uint32_t block_size;
    uint32_t block_start, block_offset, block_end;
    uint32_t flash_block_address;
    unsigned char *str;
    uint32_t str_offset;
    uint32_t write_offset, write_length;

    if (c == NULL) {
        PRINT_ERR("c is null\n");
        return LFS_ERR_INVAL;
    }
    if ((c->block_size * block + off) >= (c->block_size * c->block_count)) {
        PRINT_ERR("write_start(%d) >= maxsize(%d)\n", (c->block_size * block + off),
                (c->block_size * c->block_count));
        return LFS_ERR_INVAL;
    }
    if ((c->block_size * block + off + size) > (c->block_size * c->block_count)) {
        PRINT_ERR("write_end(%d) > maxsize(%d)\n", (c->block_size * block + off + size),
                (c->block_size * c->block_count));
        return LFS_ERR_INVAL;
    }
    if (src == NULL) {
        PRINT_ERR("src is null\n");
        return LFS_ERR_INVAL;
    }

    ret = fs_get_partition(c->context, &partition_offset);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("c->context(%s) is not support\n", c->context);
        return LFS_ERR_INVAL;
    }

    block_size = c->block_size;
    addr = ((m_fs_cfg[partition_offset].block_start + block) * block_size) + off;
    block_start = addr / block_size;
    block_end = (addr + size) / block_size;
    if (((addr + size) % block_size) != 0) {
        block_end++;
    }

    buffer = (unsigned char *)malloc(sizeof(unsigned char) * block_size);
    if (buffer == NULL) {
        PRINT_ERR("malloc failed\n");
        return LFS_ERR_NOMEM;
    }

    str = (unsigned char *)src;
    str_offset = 0;

    for (block_offset = block_start; block_offset < block_end; block_offset++) {
        if (block_offset == block_start) {
            /* 开头特殊处理 */
            flash_block_address = block_offset * block_size;
            write_offset = (addr - flash_block_address);
            write_length = block_size - write_offset;
            /* 读取整块数据，擦除整块数据，修改部分数据，最后写入Flash */
            res = FlashRead(flash_block_address, block_size, buffer);
            if (res != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("FlashRead(%d, %d, buffer) failed(%u)\n", flash_block_address, block_size, res);
                return LFS_ERR_IO;
            }
            res = FlashErase(flash_block_address, block_size);
            if (res != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("FlashErase(%d, %d) failed(%u)\n", flash_block_address, block_size, res);
                return LFS_ERR_IO;
            }
            memcpy_s(&buffer[write_offset], block_size - write_offset,  &src[str_offset], write_length);
            res = FlashWrite(flash_block_address, block_size, buffer, 0);
            if (res != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("FlashWrite(%d, %d, buffer) failed(%u)\n", flash_block_address, block_size, res);
                return LFS_ERR_IO;
            }
            str_offset += write_length;
        } else if (block_offset == (block_end - 1)) {
            /* 结尾特殊处理 */
            flash_block_address = block_offset * block_size;
            write_offset = 0;
            write_length = (addr + size) - flash_block_address;
            /* 读取整块数据，擦除整块数据，修改部分数据，最后写入Flash */
            res = FlashRead(flash_block_address, block_size, buffer);
            if (res != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("FlashRead(%d, %d, buffer) failed(%u)\n", flash_block_address, block_size, res);
                return LFS_ERR_IO;
            }
            res = FlashErase(flash_block_address, block_size);
            if (res != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("FlashErase(%d, %d) failed(%u)\n", flash_block_address, block_size, res);
                return LFS_ERR_IO;
            }
            memcpy_s(&buffer[write_offset], block_size - write_offset, &src[str_offset], write_length);
            res = FlashWrite(flash_block_address, block_size, buffer, 0);
            if (res != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("FlashWrite(%d, %d, buffer) failed(%u)\n", flash_block_address, block_size, res);
                return LFS_ERR_IO;
            }
            str_offset += write_length;
        } else {
            /* 中间的部分数据，整块处理 */
            flash_block_address = block_offset * block_size;
            write_offset = 0;
            write_length = block_size;
            /* 读取整块数据，擦除整块数据，最后写入Flash */
            res = FlashErase(flash_block_address, block_size);
            if (res != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("FlashErase(%d, %d) failed(%u)\n", flash_block_address, block_size, res);
                return LFS_ERR_IO;
            }
            res = FlashWrite(flash_block_address, block_size, &src[str_offset], 0);
            if (res != LZ_HARDWARE_SUCCESS) {
                PRINT_ERR("FlashWrite(%d, %d, buffer) failed(%u)\n", flash_block_address, block_size, res);
                return LFS_ERR_IO;
            }
            str_offset += write_length;
        }
    }

    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    return LFS_ERR_OK;
}

static int32_t fs_erase(const struct lfs_config *c, lfs_block_t block)
{
    int32_t ret;
    struct fs_cfg *fs;
    uint32_t addr;
    uint32_t partition_offset;

    if (c == NULL) {
        PRINT_ERR("c is null\n");
        return LFS_ERR_INVAL;
    }
    if (block >= c->block_count) {
        PRINT_ERR("block(%d) >= block_count(%d)\n", block, c->block_count);
        return LFS_ERR_INVAL;
    }

    ret = fs_get_partition(c->context, &partition_offset);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("c->context(%s) is not support\n", c->context);
        return LFS_ERR_INVAL;
    }

    fs = &m_fs_cfg[partition_offset];
    addr = ((fs->block_start + block) * c->block_size);
    FlashErase(addr, c->block_size);

    return LFS_ERR_OK;
}

static int32_t fs_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

static uint32_t fsdrv_readdrs(const struct DeviceResourceNode *node, struct fs_cfg *fs)
{
    int32_t ret;
    struct DeviceResourceIface *iface = NULL;
    int32_t num = 0;
    uint32_t block_size = FlashGetBlockSize();
    uint32_t address_start, address_end;

    if (node == NULL) {
        PRINT_ERR("node is null\n");
        return HDF_ERR_INVALID_PARAM;
    }
    if (fs == NULL) {
        PRINT_ERR("fs  is null\n");
        return HDF_ERR_INVALID_PARAM;
    }

    iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (iface == NULL) {
        PRINT_ERR("iface is null\n");
        return HDF_ERR_INVALID_PARAM;
    }
    if (iface->GetElemNum == NULL) {
        PRINT_ERR("GetElemNum is null\n");
        return HDF_ERR_INVALID_PARAM;
    }
    if (iface->GetStringArrayElem == NULL) {
        PRINT_ERR("GetStringArrayElem is null\n");
        return HDF_ERR_INVALID_PARAM;
    }
    if (iface->GetUint32ArrayElem == NULL) {
        PRINT_ERR("GetStringArrayElem is null\n");
        return HDF_ERR_INVALID_PARAM;
    }

    num = iface->GetElemNum(node, "mount_points");
    if (num < 0 || num > LOSCFG_LFS_MAX_MOUNT_SIZE) {
        PRINT_ERR("invalid mount_points num %d", num);
        return HDF_FAILURE;
    }

    for (int32_t i = 0; i < num; i++) {
        ret = iface->GetStringArrayElem(node, "mount_points", i, &fs[i].mount_point, NULL);
        if (ret != HDF_SUCCESS) {
            PRINT_ERR("failed to get mount_points");
            return HDF_FAILURE;
        }

        fs[i].lfs_cfg.context = (void *)fs[i].mount_point;

        ret = iface->GetUint32ArrayElem(node, "block_size", i, &fs[i].lfs_cfg.block_size, NULL);
        if (ret != HDF_SUCCESS) {
            PRINT_ERR("failed to get block_size");
            return HDF_FAILURE;
        }

        ret = iface->GetUint32ArrayElem(node, "block_count", i, &fs[i].lfs_cfg.block_count, NULL);
        if (ret != HDF_SUCCESS) {
            PRINT_ERR("failed to get block_count");
            return HDF_FAILURE;
        }

        ret = iface->GetUint32ArrayElem(node, "block_start", i, &fs[i].block_start, NULL);
        if (ret != HDF_SUCCESS) {
            PRINT_ERR("failed to get block_start");
            return HDF_FAILURE;
        }
    }

    /* 检查相关参数是否正确 */
    for (int32_t i = 0; i < num; i++) {
        if (fs[i].mount_point == NULL) {
            break;
        }

        PRINT_LOG("fs[%d]:\n", i);
        PRINT_LOG("     mount_points = %s\n", fs[i].mount_point);
        PRINT_LOG("     context = %s\n", (char *)fs[i].lfs_cfg.context);
        PRINT_LOG("     block_size = %d\n", fs[i].lfs_cfg.block_size);
        PRINT_LOG("     block_count = %d\n", fs[i].lfs_cfg.block_count);
        PRINT_LOG("     block_start = %d\n", fs[i].block_start);

        /* 检查 */
        if (fs[i].lfs_cfg.block_size != block_size) {
            PRINT_ERR("littefs[%d].lfs_cfg.block_size(%d) != flash_block_size(%d)\n",
                    i, fs[i].lfs_cfg.block_size, block_size);
            return HDF_ERR_INVALID_PARAM;
        }

        address_start = (uint32_t)(fs[i].block_start * fs[i].lfs_cfg.block_size);
        if (address_start < FLASH_ADDRESS_USER_START) {
            PRINT_ERR("partition[%d].address_start(%d) < FLASH_ADDRESS_USER_START(%d)\n",
                i, address_start, FLASH_ADDRESS_USER_START);
            return HDF_FAILURE;
        }

        address_end = (uint32_t)((fs[i].block_start + fs[i].lfs_cfg.block_count) * fs[i].lfs_cfg.block_size);
        if (address_end > FLASH_ADDRESS_USER_END) {
            PRINT_ERR("partition[%d].address_end(%d) < FLASH_ADDRESS_USER_END(%d)\n",
                i, address_end, FLASH_ADDRESS_USER_END);
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

static int32_t fsdrv_init(struct HdfDeviceObject *device)
{
    int result;
    int32_t ret;
    struct FileOpInfo *file_op_info = NULL;

    if (device == NULL) {
        PRINT_ERR("device is null\n");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (device->property == NULL) {
        PRINT_ERR("device is null\n");
        return HDF_ERR_INVALID_OBJECT;
    }

    /* Flash设备初始化 */
    FlashInit();

    ret = fsdrv_readdrs(device->property, &m_fs_cfg[0]);
    if (ret != HDF_SUCCESS) {
        PRINT_ERR("%s: fsdrv_readdrs failed(%d)\n", __func__, ret);
        return ret;
    }

    for (int i = 0; i < sizeof(m_fs_cfg) / sizeof(m_fs_cfg[0]); i++) {
        if (m_fs_cfg[i].mount_point == NULL) {
            PRINT_LOG("m_fs_cfg[%d].mount_point is null\n", i);
            continue;
        }

        m_fs_cfg[i].lfs_cfg.read = fs_read;
        m_fs_cfg[i].lfs_cfg.prog = fs_write;
        m_fs_cfg[i].lfs_cfg.erase = fs_erase;
        m_fs_cfg[i].lfs_cfg.sync = fs_sync;

        m_fs_cfg[i].lfs_cfg.read_size = LFSCFG_READSIZE_MAX;
        m_fs_cfg[i].lfs_cfg.prog_size = LFSCFG_PROGSIZE_MAX;
        m_fs_cfg[i].lfs_cfg.cache_size = LFSCFG_CACHESIZE_MAX;
        m_fs_cfg[i].lfs_cfg.lookahead_size = LFSCFG_LOCKAHEADSIZE_MAX;
        m_fs_cfg[i].lfs_cfg.block_cycles = LFSCFG_BLOCKCYCLES_MAX;

        m_fs_cfg[i].lfs_cfg.file_max = LFS_FILE_MAX;
        m_fs_cfg[i].lfs_cfg.name_max = LFS_NAME_MAX;

        result = SetDefaultMountPath(i, m_fs_cfg[i].mount_point);
        if (result != VFS_OK) {
            PRINT_ERR("SetDefaultMountPath(%d, %d) failed(%d)\n", i, m_fs_cfg[i].mount_point, result);
            continue;
        }

        result = mount(NULL, m_fs_cfg[i].mount_point, "littlefs", 0, &m_fs_cfg[i].lfs_cfg);
        printf("%s: mount fs on '%s' %s\n", __func__, m_fs_cfg[i].mount_point, (result == 0) ? "succeed" : "failed");
        if (CheckPathIsMounted(m_fs_cfg[i].mount_point, &file_op_info) == TRUE) {
            int lfs_ret = lfs_mkdir(&file_op_info->lfsInfo, m_fs_cfg[i].mount_point);
            if (lfs_ret == LFS_ERR_OK) {
                PRINT_LOG("create root dir(%s) success.\n", m_fs_cfg[i].mount_point);
            } else if (lfs_ret == LFS_ERR_EXIST) {
                PRINT_LOG("root dir(%s) exist.\n", m_fs_cfg[i].mount_point);
            } else {
                PRINT_LOG("create root dir(%s) failed.", m_fs_cfg[i].mount_point);
            }
        }
    }

    PRINT_LOG("LittleFS service: %s init success!\n", HdfDeviceGetServiceName(device));
    return HDF_SUCCESS;
}

static int32_t fsdrv_bind(struct HdfDeviceObject *device)
{
    (void)device;
    return HDF_SUCCESS;
}

static void fsdrv_release(struct HdfDeviceObject *device)
{
    (void)device;
    PRINT_LOG("LittleFS service: %s release\n", HdfDeviceGetServiceName(device));
}

static struct HdfDriverEntry g_fsDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_PLATFORM_FS_LITTLEFS",
    .Bind = fsdrv_bind,
    .Init = fsdrv_init,
    .Release = fsdrv_release,
};

HDF_INIT(g_fsDriverEntry);
