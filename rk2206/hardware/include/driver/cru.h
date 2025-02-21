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

/**
 * @addtogroup Driver
 *
 * @file cru.h
 */
#ifndef DRIVER_CRU_H
#define DRIVER_CRU_H

#include "sys/queue.h"

typedef struct _CLK_GATE {
    SLIST_ENTRY(_CLK_GATE) entries;
    unsigned int gateID;
    int enableCount;
    int refCount;
} CLK_GATE;

typedef struct _CLK_INIT {
    const char *name;
    unsigned int clkID;
    unsigned int initRate;
} CLK_INIT;

typedef struct _CLK_UNUSED {
    unsigned int isPmucru : 1;
    unsigned int gateCon : 31;
    unsigned int gateVal;
} CLK_UNUSED;

/**
 * @brief Clock enable by gate ID.
 *
 * @param gateID Indicates the clock gate ID.
 * @return Returns {@link HAL_SUCCESS} if the clock is enabled;
 * returns {@link HAL_FAILURE} otherwise. For details about other return values, see the chip description.
 */
unsigned int ClkEnableByID(int gateID);

/**
 * @brief Clock disable by gate ID.
 *
 * @param gateID Indicates the clock gate ID.
 * @return Returns {@link HAL_SUCCESS} if the clock is disabled;
 * returns {@link HAL_FAILURE} otherwise. For details about other return values, see the chip description.
 */
unsigned int ClkDisableByID(int gateID);

/**
 * @brief Clock enable.
 *
 * @param gate Indicates the clock gate.
 * @return Returns {@link HAL_SUCCESS} if the clock is enabled;
 * returns {@link HAL_FAILURE} otherwise. For details about other return values, see the chip description.
 */
unsigned int ClkEnable(CLK_GATE *gate);

/**
 * @brief Clock disable.
 *
 * @param gate Indicates the clock gate.
 * @return Returns {@link HAL_SUCCESS} if the clock is enabled;
 * returns {@link HAL_FAILURE} otherwise. For details about other return values, see the chip description.
 */
unsigned int ClkDisable(CLK_GATE *gate);

/**
 * @brief Whether the clock is enabled.
 *
 * @param gate Indicates the clock gate.
 * @return Returns 1 if the clock is enabled; otherwise return 0.
 */
unsigned int ClkIsEnabled(CLK_GATE *gate);

/**
 * @brief Get clock gate struct.
 *
 * @param gateID Indicates the clock gate ID.
 * @return Returns the pointer of clock gate; otherwise return NULL.
 */
CLK_GATE *GetClkGate(int gateID);

/**
 * @brief Put clock gate.
 *
 * @param gate Indicates the clock gate.
 */
void PutClkGate(CLK_GATE *gate);

/**
 * @brief Get clock rate.
 *
 * @param clkID Indicates the clock ID.
 * @return Returns the clock rate.
 */
unsigned int ClkGetRate(eCLOCK_Name clkID);

/**
 * @brief Set clock rate.
 *
 * @param clkID Indicates the clock ID.
 * @param rate Indicates the clock rate.
 * @return Returns {@link HAL_SUCCESS} if the clock rate is set;
 * returns {@link HAL_FAILURE} otherwise. For details about other return values, see the chip description.
 */
unsigned int ClkSetRate(eCLOCK_Name clkID, unsigned int rate);

/**
 * @brief Get HCLK system core frequency.
 *
 * @return Returns the frequency.
 */
unsigned int ClkHclkSysCoreFreq(void);

/**
 * @brief Initializes clock device.
 *
 * @return Returns {@link HAL_SUCCESS} if the clock rate is initialized;
 * returns {@link HAL_FAILURE} otherwise. For details about other return values, see the chip description.
 */
unsigned int ClkDevInit(void);

/**
 * @brief Deinitializes clock device.
 *
 * @return Returns {@link HAL_SUCCESS} if the clock rate is initialized;
 * returns {@link HAL_FAILURE} otherwise. For details about other return values, see the chip description.
 */
unsigned int ClkDeDeinit(void);

/**
 * @brief Set rate and enable clock list.
 *
 * @param clkInits Indicates the clock list to be set and enable.
 * @param clkCount Indicates the count of clock list.
 * @param clkDump Indicates whether dump clock init results.
 */
void ClkInit(const CLK_INIT *clkInits, unsigned int clkCount, bool clkDump);

/**
 * @brief Disable unused clock list.
 *
 * @param clkUnuseds Indicates the clock list to be disabled.
 * @param clkCount Indicates the count of clock list.
 */
void clkDisableUnused(const CLK_UNUSED *clkUnuseds, unsigned int clkCount);

#endif
