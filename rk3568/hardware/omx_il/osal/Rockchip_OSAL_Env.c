/*
 *
 * Copyright 2018 Rockchip Electronics Co., LTD.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <securec.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "Rockchip_OSAL_Log.h"
#include "Rockchip_OSAL_Env.h"

#ifndef OHOS
#include <sys/system_properties.h>

OMX_ERRORTYPE Rockchip_OSAL_GetEnvU32(const char *name, OMX_U32 *value, OMX_U32 default_value)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    char prop[PROP_VALUE_MAX + 1];
    int len = __system_property_get(name, prop);
    if (len > 0) {
        char *endptr;
        int base = (prop[0] == '0' && prop[1] == 'x') ? (16) : (10);
        errno = 0;
        *value = strtoul(prop, &endptr, base);
        if (errno || (prop == endptr)) {
            errno = 0;
            *value = default_value;
        }
    } else {
        *value = default_value;
    }

    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_GetEnvStr(const char *name, char *value, OMX_U32 valueSize, char *default_value)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    if (value != NULL) {
        int len = __system_property_get(name, value);
        if (len <= 0 && default_value != NULL) {
            if (strcpy_s(value, strlen(default_value)+1, default_value) != EOK) {
                omx_err("strcpy_s error.\n");
                ret = OMX_ErrorBadParameter;
                goto EXIT;
            }
        }
    } else {
        omx_err("get env string failed, value is null");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_SetEnvU32(const char *name, OMX_U32 value)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    char buf[PROP_VALUE_MAX + 1];
    if (snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "%lu", value) < 0) {
        omx_err("snprintf_s error!");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    int len = __system_property_set(name, buf);
    if (len <= 0) {
        omx_err("property set failed!");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
EXIT:
    return ret;
}

OMX_ERRORTYPE Rockchip_OSAL_SetEnvStr(const char *name, char *value)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    int len = __system_property_set(name, value);
    if (len <= 0) {
        omx_err("property set failed!");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    return ret;
}

#else

#include "parameter.h"
#define VDEC_DBG_RECORD_MASK 0xff000000
#define VDEC_DBG_RECORD_IN   0x01000000
#define VDEC_DBG_RECORD_OUT  0x02000000

#define VIDEO_DBG_LOG_MASK                  0x0000ffff
#define VIDEO_DBG_LOG_PTS                   0x00000001
#define VIDEO_DBG_LOG_FPS                   0x00000002
#define VIDEO_DBG_LOG_BUFFER                0x00000004
#define VIDEO_DBG_LOG_DBG                   0x00000008
#define VIDEO_DBG_LOG_PORT                  0x00000010
#define VIDEO_DBG_LOG_BUFFER_POSITION       0x00000020

#define VDEC_DBG_VPU_MPP_FIRST              0x00000001
#define VDEC_DBG_VPU_VPUAPI_FIRST           0x00000002
OMX_ERRORTYPE Rockchip_OSAL_GetEnvU32(const char *name, OMX_U32 *value, OMX_U32 default_value)
{
    if ((value == NULL) || (name == NULL)) {
        return OMX_ErrorUndefined;
    }
    omx_info("%s defval: 0x%lx", name, default_value);
    *value = default_value;
    char bufDef[10] = {0};
    if (snprintf(bufDef, sizeof(bufDef), "%d", default_value) <= 0) {
        omx_err("snprintf error");
        return OMX_ErrorUndefined;
    }
    char bufValue[10] = {0};
    OMX_ERRORTYPE ret = Rockchip_OSAL_GetEnvStr(name, bufValue, sizeof(bufValue), bufDef);
    if (ret != OMX_ErrorNone) {
        return ret;
    }
    *value = atoi(bufValue);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Rockchip_OSAL_GetEnvStr(const char *name, char *value, OMX_U32 valueSize, char *default_value)
{
    if ((value == NULL) || (default_value == NULL) || (name == NULL)) {
        return OMX_ErrorUndefined;
    }
    omx_info("%s def: %s", name, default_value);
    if (GetParameter(name, default_value, value, valueSize) <= 0) {
        omx_err("GetParameter error");
        strcpy_s(value, valueSize > strlen(default_value) ? strlen(default_value) : valueSize, default_value);
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE Rockchip_OSAL_SetEnvU32(const char *name, OMX_U32 value)
{
    if (name == NULL) {
        return OMX_ErrorUndefined;
    }
    char buf[10] = {0};
    if (snprintf(buf, sizeof(buf), "%d", value) <= 0) {
        omx_err("snprintf error");
        return OMX_ErrorUndefined;
    }
    return Rockchip_OSAL_SetEnvStr(name, buf);
}

OMX_ERRORTYPE Rockchip_OSAL_SetEnvStr(const char *name, char *value)
{
    if ((name == NULL) || (value == NULL)) {
        return OMX_ErrorUndefined;
    }
    if (SetParameter(name, value) <= 0) {
        omx_err("SetParameter error");
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
}
#endif

