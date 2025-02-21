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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "los_tick.h"
#include "los_task.h"
#include "los_config.h"
#include "los_interrupt.h"
#include "los_debug.h"
#include "los_compiler.h"
#include "los_arch_interrupt.h"
#include "los_task.h"

#include "lz_hardware.h"
#include "uart_debug.h"
#include "config_network.h"

#define MAIN_TAG                    "MAIN"

#define IOTPROCESS_TASK_STACKSIZE   0x1000
#define IOTPROCESS_TASK_PRIO        6
#define IOTPROCESS_TASK_NAME        "IotProcess"

#define MASKLED_ON                  LZGPIO_LEVEL_HIGH
#define MASKLED_OFF                 LZGPIO_LEVEL_LOW

void OHOS_SystemInit(void);

static void IotProcess(void *arg)
{
    static const unsigned int SLEEP_MAX_SECOND = 5;
    static const unsigned int SECOND_TO_MSECOND = 1000;
    
    while (1) {
        printf("%s: sleep %d sec!\n", __func__, SLEEP_MAX_SECOND);
        
        LOS_Msleep(SLEEP_MAX_SECOND * SECOND_TO_MSECOND);
    }
}

static void IotInit(void)
{
    UINT32 uwRet;
    UINT32 taskID;
    TSK_INIT_PARAM_S stTask = {0};
    
    stTask.pfnTaskEntry = (TSK_ENTRY_FUNC)IotProcess;
    stTask.uwStackSize = IOTPROCESS_TASK_STACKSIZE;
    stTask.pcName = IOTPROCESS_TASK_NAME;
    stTask.uwArg = (UINT32)(0);
    stTask.usTaskPrio = IOTPROCESS_TASK_PRIO;
    uwRet = LOS_TaskCreate(&taskID, &stTask);
    if (uwRet != LOS_OK) {
        LZ_HARDWARE_LOGD(MAIN_TAG, "MainBoot task create failed!!!");
    }
}

int Main(void)
{
    UINT32 ret;
    
    HalInit();
    
    LZ_HARDWARE_LOGD(MAIN_TAG, "%s: OpenHarmony enter...", __func__);
    
    ret = LOS_KernelInit();
    if (ret == LOS_OK) {
        OHOS_SystemInit();
        DeviceManagerStart(); // HDF Drivers Init
        IotInit();
        LZ_HARDWARE_LOGD(MAIN_TAG, "%s: OpenHarmony start schedule...", __func__);
        LOS_Start();
    }
    
    while (1) {
        __asm volatile("wfi");
    }
    
    return 0;
}
