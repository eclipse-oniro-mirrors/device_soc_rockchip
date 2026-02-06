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

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "link.h"
#include "lz_hardware.h"
#include "lfs.h"
#include "los_config.h"
#include "lfs_api.h"

/* 作为主目录文件夹 */
#define MOUNT_ROOT          "/data"
/* 文件名字字符串长度 */
#define FILE_NAME_MAXSIZE   32

static int HalCheckPath(const char *src, char *dst, int max_len)
{
    if (src == NULL) {
        return -1;
    }
    if (dst == NULL) {
        return -1;
    }
    if (strlen(src) > max_len) {
        return -1;
    }
    if (strlen(src) > max_len) {
        return -1;
    }

    memset_s(dst, sizeof(char) * max_len, 0, sizeof(char) * max_len);
    if (src[0] == '/') {
        if (memcpy_s(dst, sizeof(char) * max_len, src, strlen(src)) == NULL) {
            return -1;
        }
    } else {
        if (strlen(src) > max_len) {
            return -1;
        }

        if (snprintf_s(dst, max_len, max_len -1, "%s/%s", MOUNT_ROOT, src) < 0) {
            return -1;
        }
    }

    return 0;
}

int HalFileOpen(const char* path, int oflag, int mode)
{
    int ret = 0;
    char full_path[2 * LFS_NAME_MAX];

    ret = HalCheckPath(path, full_path, FILE_NAME_MAXSIZE);
    if (ret != 0) {
        return -1;
    }

    ret = LfsOpen(full_path, oflag, mode);
    if (ret < 0) {
        printf("%s, %d: LfsOpen(%s) failed(%d)\n", __FILE__, __LINE__, full_path, ret);
    }

    return ret;
}

int HalFileClose(int fd)
{
    return LfsClose(fd);
}

int HalFileRead(int fd, char *buf, unsigned int len)
{
    return LfsRead(fd, buf, len);
}

int HalFileWrite(int fd, const char *buf, unsigned int len)
{
    return LfsWrite(fd, buf, len);
}

int HalFileSeek(int fd, int offset, unsigned int whence)
{
    return LfsSeek(fd, offset, whence);
}

int HalFileStat(const char* path, unsigned int* fileSize)
{
    struct stat st_buf = { 0 };
    int ret = 0;
    char full_path[2 * LFS_NAME_MAX];

    if (path == NULL) {
        return -1;
    }
    if (fileSize == NULL) {
        return -1;
    }

    ret = HalCheckPath(path, full_path, FILE_NAME_MAXSIZE);
    if (ret != 0) {
        return -1;
    }

    if (LfsStat(full_path, &st_buf) != 0) {
        return -1;
    }

    *fileSize = st_buf.st_size;

    return 0;
}

int HalFileDelete(const char* path)
{
    int ret = 0;
    char full_path[LFS_NAME_MAX];

    if (path == NULL) {
        return -1;
    }

    ret = HalCheckPath(path, full_path, FILE_NAME_MAXSIZE);
    if (ret != 0) {
        return -1;
    }

    return LfsUnlink(full_path);
}

