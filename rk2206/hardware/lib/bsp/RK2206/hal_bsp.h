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

#ifndef _HAL_BSP_H_
#define _HAL_BSP_H_

#include "hal_base.h"

/***************************** MACRO Definition ******************************/

/***************************** Structure Definition **************************/
#ifdef HAL_ACDCDIG_MODULE_ENABLED
extern struct HAL_ACDCDIG_DEV g_acdcDigDev;
#endif

#ifdef HAL_AUDIOPWM_MODULE_ENABLED
extern struct HAL_AUDIOPWM_DEV g_audioPwmDev;
#endif

#ifdef HAL_DWDMA_MODULE_ENABLED
extern struct HAL_DWDMA_DEV g_dwDmaDev;
#endif

#ifdef HAL_HYPERPSRAM_MODULE_ENABLED
extern struct HAL_HYPERPSRAM_DEV g_hyperpsramDev;
#endif

#ifdef HAL_I2C_MODULE_ENABLED
extern const struct HAL_I2C_DEV g_i2c0Dev;
extern const struct HAL_I2C_DEV g_i2c1Dev;
extern const struct HAL_I2C_DEV g_i2c2Dev;
#endif

#ifdef HAL_PDM_MODULE_ENABLED
extern struct HAL_PDM_DEV g_pdm0Dev;
#endif

#if defined(HAL_PINCTRL_MODULE_ENABLED)
extern const struct HAL_PINCTRL_DEV g_pinDev;
#endif

#ifdef HAL_PWM_MODULE_ENABLED
extern const struct HAL_PWM_DEV g_pwm0Dev;
extern const struct HAL_PWM_DEV g_pwm1Dev;
extern const struct HAL_PWM_DEV g_pwm2Dev;
#endif

#ifdef HAL_UART_MODULE_ENABLED
extern const struct HAL_UART_DEV g_uart0Dev;
extern const struct HAL_UART_DEV g_uart1Dev;
extern const struct HAL_UART_DEV g_uart2Dev;
#endif

#ifdef HAL_I2STDM_MODULE_ENABLED
extern struct HAL_I2STDM_DEV g_i2sTdm0Dev;
extern struct HAL_I2STDM_DEV g_i2sTdm1Dev;
#endif

#ifdef HAL_TSADC_MODULE_ENABLED
extern const struct HAL_TSADC_DEV g_tsadcDev;
#endif

#ifdef HAL_FSPI_MODULE_ENABLED
extern struct HAL_FSPI_HOST g_fspi0Dev;
extern struct HAL_FSPI_HOST g_fspi1Dev;
#endif

#ifdef HAL_PVTM_MODULE_ENABLED
extern const struct HAL_PVTM_DEV g_pvtmDev;
#endif

#if defined(HAL_PCD_MODULE_ENABLED) || defined(HAL_HCD_MODULE_ENABLED)
extern const struct HAL_USB_DEV g_usbdDev;
#endif

#ifdef HAL_VAD_MODULE_ENABLED
extern struct HAL_VAD_DEV g_vadDev;
#endif

#ifdef HAL_SPI_MODULE_ENABLED
extern const struct HAL_SPI_DEV g_spiDev0;
extern const struct HAL_SPI_DEV g_spiDev1;
#endif

/***************************** Function Declare ******************************/

void BSP_Init(void);
void BSP_DeInit(void);

#endif /* _HAL_BSP_H_ */