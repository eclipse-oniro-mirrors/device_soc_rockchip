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
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "los_tick.h"
#include "los_task.h"
#include "los_config.h"
#include "los_interrupt.h"
#include "los_debug.h"
#include "los_compiler.h"

#include "lz_hardware.h"
#include "los_mux.h"
#include "config_network.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

/* 定义无线模块功能 */
enum enum_network_mode {
    ENETWORK_MODE_AP = 0,
    ENETWORK_MODE_WIFI,
    ENETWORK_MODE_MAX
} network_mode_e;

/* 定义bssid数组位移 */
enum enum_b_ssid {
    EBSSID_0 = 0,
    EBSSID_1,
    EBSSID_2,
    EBSSID_3,
    EBSSID_4,
    EBSSID_5,
    EBSSID_MAX
} b_ssid_e;

/* 定义mac地址数组位移 */
enum enum_mac_address {
    EMAC_ADDRESS_0 = 0,
    EMAC_ADDRESS_1,
    EMAC_ADDRESS_2,
    EMAC_ADDRESS_3,
    EMAC_ADDRESS_4,
    EMAC_ADDRESS_5,
    EMAC_ADDRESS_MAX
}mac_address_e;

/* 定义网络AP模式的任务优先级 */
#define TASK_PRIO_AP_MODE       5
/* 定义网络WiFi模式的任务优先级 */
#define TASK_PRIO_WIFI_MODE     15

#define OS_SLEEP_S(val)         ((val) * 1000)
#define OS_TASK_STACK_SIZE      0x2000
#define OS_PORT                 9997
#define RKNC_HEAD               "rknc"
#define RKBC_HEAD               "rkbc"

#define LOG_TAG                 "config_network"

#define SSID                    "凌智电子"
#define PASSWORD                "88888888"

STATIC RKWifiConfig g_wificonfig = {0};

STATIC UINT32 g_wifiTask;
STATIC UINT32 g_softbusliteTask;
STATIC UINT32 g_apTask;

typedef struct {
    bool   init;
    bool   ap_on;
    bool   sta_on;
    UINT32 muxlock;
} wifi_mode_lock_t;

wifi_mode_lock_t m_wml = {
    .init   = false,
    .ap_on  = false,
    .sta_on = false,
};

#define MUX_LOCK_TO_TIME        30000

/* 定义SN字符串长度 */
#define SN_STRING_MAXSIZE       100
/* 定义MAC地址长度 */
#define MAC_ADDRESS_MAXSIZE     (EMAC_ADDRESS_MAX)
/* 定义IP地址长度 */
#define IP_ADDRESS_MAXSIZE      4
/* 定义bssid字符串长度 */
#define BSSID_MAXSIZE           18
/* 定义SSID字符串长度 */
#define SSID_MAXSIZE            33

WifiErrorCode SetApModeOn(void)
{
    if (!m_wml.init) {
        if (LOS_MuxCreate(&m_wml.muxlock) == LOS_OK) {
            m_wml.init = true;
        } else {
            printf("%s muxcreate err\n", __FUNCTION__);
        }
    }
    LOS_MuxPend(m_wml.muxlock, MUX_LOCK_TO_TIME);
    
    WifiErrorCode error;
    HotspotConfig config = {0};
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork SetApModeOn start ...\n");
    
    memcpy_s(config.ssid, WIFI_MAX_SSID_LEN, SSID, sizeof(SSID));
    memcpy_s(config.preSharedKey, WIFI_MAX_KEY_LEN, PASSWORD, sizeof(PASSWORD));
    config.channelNum = 1;
    error = SetHotspotConfig(&config);
    if (error != WIFI_SUCCESS) {
        LZ_HARDWARE_LOGE(LOG_TAG, "rknetwork SetHotspotConfig ...error: %d\n", error);
        LOS_MuxPost(m_wml.muxlock);
        return error;
    }
    
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork EnableHotspot ...\n");
    
    error = EnableHotspot();
    if (error != WIFI_SUCCESS) {
        LZ_HARDWARE_LOGE(LOG_TAG, "rknetwork EnableHotspot ...error: %d\n", error);
        LOS_MuxPost(m_wml.muxlock);
        return error;
    }
    
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork EnableHotspot done");
    FlashSetResidentFlag(1);
    m_wml.ap_on = true;

    LOS_MuxPost(m_wml.muxlock);
    return error;
}

WifiErrorCode SetApModeOff(void)
{
    if (!m_wml.init) {
        if (LOS_MuxCreate(&m_wml.muxlock) == LOS_OK) {
            m_wml.init = true;
        } else {
            printf("%s muxcreate err\n", __FUNCTION__);
        }
    }
    LOS_MuxPend(m_wml.muxlock, MUX_LOCK_TO_TIME);
    
    WifiErrorCode error;
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork SetApModeOff start ...\n");
    // check AP stat
    error = IsHotspotActive();
    if (error == WIFI_HOTSPOT_ACTIVE) {
        LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork AP is active, disable now...\n");
        error = DisableHotspot();
        if (error == WIFI_SUCCESS) {
            LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork disable AP success\n");
        } else {
            LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork disable AP fail, please disable ap, error: %d\n", error);
            LOS_MuxPost(m_wml.muxlock);
            return error;
        }
    } else {
        LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork AP is inactive\n");
    }
    
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork SetApModeOff end ...\n");
    m_wml.ap_on = false;
    if (!m_wml.sta_on) {
        FlashSetResidentFlag(0);
    }
    
    LOS_MuxPost(m_wml.muxlock);
    return error;
}

WifiErrorCode SetWifiModeOff(void)
{
    if (!m_wml.init) {
        if (LOS_MuxCreate(&m_wml.muxlock) == LOS_OK) {
            m_wml.init = true;
        } else {
            printf("%s muxcreate err\n", __FUNCTION__);
        }
    }
    LOS_MuxPend(m_wml.muxlock, MUX_LOCK_TO_TIME);
    
    WifiErrorCode error;
    // check wifi stat
    int ret = IsWifiActive();
    if (ret == WIFI_STATE_AVALIABLE) {
        LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork wifi is active, disable now...\n");
        error = DisableWifi();
        if (error == WIFI_SUCCESS) {
            LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork disable wifi success\n");
        } else {
            LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork disable wifi fail, please disable wifi, error: %d\n", error);
            LOS_MuxPost(m_wml.muxlock);
            return error;
        }
    } else {
        LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork wifi is inactive\n");
    }
    
    m_wml.sta_on = false;
    if (!m_wml.ap_on) {
        FlashSetResidentFlag(0);
    }
    
    LOS_MuxPost(m_wml.muxlock);
    return error;
}

static WifiErrorCode SetWifiScan(void)
{
#if WIFI_SCAN_ON
#define RETRY_SLEEP_MSEC         1000
    WifiErrorCode error;
    WifiScanInfo *infoList;
    unsigned int retry = 15;
    unsigned int size = WIFI_SCAN_HOTSPOT_LIMIT;
    
    error = Scan();
    if (error != WIFI_SUCCESS) {
        LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork Scan error: %d\n", error);
        return error;
    }
    
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork Scan done\n");
    
    infoList = malloc(sizeof(WifiScanInfo) * WIFI_SCAN_HOTSPOT_LIMIT);
    while (retry) {
        error = GetScanInfoList(infoList, &size);
        if (error != WIFI_SUCCESS || size == 0) {
            LOS_TaskDelay(RETRY_SLEEP_MSEC);
            retry--;
            continue;
        }
        
        LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork GetScanResult done\n");
        break;
    }

    if (retry == 0) {
        free(infoList);
        return;
    }
    
    if (size > 0) {
        LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork BSSID    SSID    RSSI    Channel\n");
        LZ_HARDWARE_LOGD(LOG_TAG, "========================================\n");
        for (int i = 0; i < size; i++) {
            char bssid[BSSID_MAXSIZE];
            char ssid[SSID_MAXSIZE];

            snprintf_s(bssid,
                BSSID_MAXSIZE,
                SSID_MAXSIZE - 1,
                "%02x:%02x:%02x:%02x:%02x:%02x",
                infoList[i].bssid[EBSSID_0],
                infoList[i].bssid[EBSSID_1],
                infoList[i].bssid[EBSSID_2],
                infoList[i].bssid[EBSSID_3],
                infoList[i].bssid[EBSSID_4],
                infoList[i].bssid[EBSSID_5]);
            snprintf_s(ssid, SSID_MAXSIZE, SSID_MAXSIZE - 1, "%-32s", infoList[i].ssid);
            LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork %s    %s    %d    %u\n",
                             bssid,
                             ssid,
                             infoList[i].rssi,
                             infoList[i].frequency);
            if (strncmp(ssid, g_wificonfig.ssid) == 0) {
                snprintf_s(g_wificonfig.bssid,
                    sizeof(g_wificonfig.bssid),
                    sizeof(g_wificonfig.bssid) - 1,
                    "%s",
                    bssid);
                LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork g_wificonfig.bssid: %s\n", g_wificonfig.bssid);
            }
        }
        LZ_HARDWARE_LOGD(LOG_TAG, "========================================\n");
    }
    free(infoList);
    return error;
#endif  /* WIFI_SCAN_ON */
    return WIFI_SUCCESS;
}

WifiErrorCode SetWifiModeOn(void)
{
    uint8_t hwaddr[MAC_ADDRESS_MAXSIZE] = {0x10, 0xdc, 0xb6, 0x90, 0x00, 0x00};
    uint8_t ip[IP_ADDRESS_MAXSIZE] = {192, 168, 2, 10};
    uint8_t gateway[IP_ADDRESS_MAXSIZE] = {192, 168, 2, 1};
    uint8_t mask[IP_ADDRESS_MAXSIZE] = {255, 255, 255, 0};
    char temp[SN_STRING_MAXSIZE];
    UINT32 poll_msec = 1000;
    
    if (!m_wml.init) {
        if (LOS_MuxCreate(&m_wml.muxlock) == LOS_OK) {
            m_wml.init = true;
        } else {
            printf("%s muxcreate err\n", __FUNCTION__);
        }
    }
    LOS_MuxPend(m_wml.muxlock, MUX_LOCK_TO_TIME);
    
    WifiErrorCode error = WIFI_SUCCESS;
    int netId = 0;
    WifiDeviceConfig config = {0};
    
    memcpy_s(g_wificonfig.ssid, sizeof(g_wificonfig.ssid), SSID, sizeof(SSID));
    memcpy_s(g_wificonfig.psk, sizeof(g_wificonfig.psk), PASSWORD, sizeof(PASSWORD));
    
    FlashInit();
    memset_s(temp, sizeof(temp), 0, sizeof(temp));
    
    if (LZ_HARDWARE_SUCCESS == VendorGet(VENDOR_ID_SN, temp, SN_STRING_MAXSIZE)) {
        if (strcmp(temp, "LZ01") != 0) {
            VendorSet(VENDOR_ID_SN,          "LZ01", sizeof("LZ01"));
            VendorSet(VENDOR_ID_PRODUCT,     "小凌派", sizeof("小凌派"));
            VendorSet(VENDOR_ID_FACTORY,     "凌睿智捷", sizeof("凌睿智捷"));
            VendorSet(VENDOR_ID_MAC,         hwaddr, MAC_ADDRESS_MAXSIZE);
            VendorSet(VENDOR_ID_NET_IP,      ip, IP_ADDRESS_MAXSIZE);
            VendorSet(VENDOR_ID_NET_GW,      gateway, IP_ADDRESS_MAXSIZE);
            VendorSet(VENDOR_ID_NET_MASK,    mask, IP_ADDRESS_MAXSIZE);
            VendorSet(VENDOR_ID_WIFI_SSID,   SSID, sizeof(SSID));
            VendorSet(VENDOR_ID_WIFI_PASSWD, PASSWORD, sizeof(PASSWORD));
        }
    }
    
    VendorGet(VENDOR_ID_MAC, hwaddr, MAC_ADDRESS_MAXSIZE);
    LZ_HARDWARE_LOGD(LOG_TAG, "%02x:%02x:%02x:%02x:%02x:%02x\n",
                     hwaddr[EMAC_ADDRESS_0], hwaddr[EMAC_ADDRESS_1], hwaddr[EMAC_ADDRESS_2],
                     hwaddr[EMAC_ADDRESS_3], hwaddr[EMAC_ADDRESS_4], hwaddr[EMAC_ADDRESS_5]);
    
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork SetWifiModeOn\n");
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork g_wificonfig.ssid %s\n", g_wificonfig.ssid);
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork g_wificonfig.psk %s\n", g_wificonfig.psk);
    error = EnableWifi();
    if (error != WIFI_SUCCESS) {
        LZ_HARDWARE_LOGE(LOG_TAG, "rknetwork EnableWifi error: %d\n", error);
        LOS_MuxPost(m_wml.muxlock);
        return error;
    }
    
    FlashSetResidentFlag(1);
    m_wml.sta_on = true;
    
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork EnableWifi done\n");
    
    SetWifiScan();
    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork SetWifiScan after g_wificonfig.bssid: %s\n", g_wificonfig.bssid);
    
    config.freq = 1;
    config.securityType = WIFI_SEC_TYPE_PSK;
    config.wapiPskType = WIFI_PSK_TYPE_ASCII;
    
    memcpy_s(config.ssid, WIFI_MAX_SSID_LEN, g_wificonfig.ssid, sizeof(g_wificonfig.ssid));
    memcpy_s(config.bssid, WIFI_MAC_LEN, g_wificonfig.bssid, sizeof(g_wificonfig.bssid));
    memcpy_s(config.preSharedKey, WIFI_MAX_KEY_LEN, g_wificonfig.psk, sizeof(g_wificonfig.psk));
    
    if (WifiConnect(config.ssid, config.preSharedKey) != LZ_HARDWARE_SUCCESS) {
        LZ_HARDWARE_LOGE(LOG_TAG, "WifiConnect  error");
        error = LZ_HARDWARE_FAILURE;
        LOS_MuxPost(m_wml.muxlock);
        return error;
    }
    
    LZ_HARDWARE_LOGI(LOG_TAG, "ConnectTo (%s) done", config.ssid);
    
    WifiLinkedInfo info;
    struct in_addr ipaddr, gw, netmask;
    unsigned int retry = 15;

    memset_s(&info, sizeof(WifiLinkedInfo), 0, sizeof(WifiLinkedInfo));

    while (retry) {
        if (GetLinkedInfo(&info) == WIFI_SUCCESS) {
            if (info.connState == WIFI_CONNECTED) {
                if (info.ipAddress != 0) {
                    ipaddr.s_addr = info.ipAddress;
                    LZ_HARDWARE_LOGD(LOG_TAG, "rknetwork IP (%s)", inet_ntoa(ipaddr));
                    if (WIFI_SUCCESS == GetLocalWifiGw(&gw.s_addr)) {
                        LZ_HARDWARE_LOGD(LOG_TAG, "network GW (%s)", inet_ntoa(gw));
                    }
                    if (WIFI_SUCCESS == GetLocalWifiNetmask(&netmask.s_addr)) {
                        LZ_HARDWARE_LOGD(LOG_TAG, "network NETMASK (%s)", inet_ntoa(netmask));
                    }
                    if (WIFI_SUCCESS == SetLocalWifiGw()) {
                        LZ_HARDWARE_LOGD(LOG_TAG, "set network GW");
                    }
                    if (WIFI_SUCCESS == GetLocalWifiGw(&gw.s_addr)) {
                        LZ_HARDWARE_LOGD(LOG_TAG, "network GW (%s)", inet_ntoa(gw));
                    }
                    if (WIFI_SUCCESS == GetLocalWifiNetmask(&netmask.s_addr)) {
                        LZ_HARDWARE_LOGD(LOG_TAG, "network NETMASK (%s)", inet_ntoa(netmask));
                    }
                    LOS_MuxPost(m_wml.muxlock);
                    return error;
                }
            }
        }
        
        LOS_Msleep(poll_msec);
        retry--;
    }
    
    LOS_MuxPost(m_wml.muxlock);
    return error;
}

static void TaskConfigApModeEntry(void)
{
    SetWifiModeOff();
    SetApModeOn();
}

static void TaskConfigWifiModeEntry(void)
{
    SetApModeOff();
    SetWifiModeOn();
}

UINT32 TaskConfigApMode(VOID)
{
    UINT32  ret;
    TSK_INIT_PARAM_S task = { 0 };
    
    task.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskConfigApModeEntry;
    task.uwStackSize  = OS_TASK_STACK_SIZE;
    task.pcName       = "taskConfigApModeEntry";
    task.usTaskPrio   = TASK_PRIO_AP_MODE;
    ret = LOS_TaskCreate(&g_apTask, &task);
    if (ret != LOS_OK) {
        LZ_HARDWARE_LOGE(LOG_TAG, "rknetwork LOS_TaskCreate taskConfigApModeEntry error: %d\n", ret);
        return ret;
    }
    
    return LOS_OK;
}

UINT32 TaskConfigWifiMode(VOID)
{
    UINT32  ret;
    TSK_INIT_PARAM_S task = { 0 };
    
    task.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskConfigWifiModeEntry;
    task.uwStackSize  = OS_TASK_STACK_SIZE;
    task.pcName       = "taskConfigWifiModeEntry";
    task.usTaskPrio   = TASK_PRIO_WIFI_MODE;
    ret = LOS_TaskCreate(&g_wifiTask, &task);
    if (ret != LOS_OK) {
        LZ_HARDWARE_LOGE(LOG_TAG, "rknetwork LOS_TaskCreate taskConfigWifiModeEntry error: %d\n", ret);
        return ret;
    }
    
    return LOS_OK;
}

UINT32 ExternalTaskConfigNetwork(VOID)
{
    int enable_mode = ENETWORK_MODE_WIFI;
    UINT32 ret = LOS_OK;
    
    if (enable_mode == ENETWORK_MODE_AP) {
        ret = TaskConfigApMode();
    } else if (enable_mode == ENETWORK_MODE_WIFI) {
        ret = TaskConfigWifiMode();
    } else {
        LZ_HARDWARE_LOGE(LOG_TAG, "rknetwork LOS_TaskCreate taskConfigWifiModeEntry error: %d\n", ret);
    }
    
    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */