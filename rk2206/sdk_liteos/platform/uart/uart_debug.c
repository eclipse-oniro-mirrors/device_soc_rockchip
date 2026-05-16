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
#include <stdarg.h>

#include "los_sem.h"

#include "lz_hardware.h"

/* uart debug */
#define UART_DEBUG_ID               1
/* uart baud rate */
#define UART_BAUD_RATE_DEFAULT      115200

static bool m_uart_debug_initialized = false;

void uart_debug_init(void)
{
    unsigned int ret;
    UartAttribute attr;
    
    LzUartDeinit(UART_DEBUG_ID);
    
    attr.baudRate = UART_BAUD_RATE_DEFAULT;
    attr.dataBits = UART_DATA_BIT_8;
    attr.pad = FLOW_CTRL_NONE;
    attr.parity = UART_PARITY_NONE;
    attr.rxBlock = UART_BLOCK_STATE_NONE_BLOCK;
    attr.stopBits = UART_STOP_BIT_1;
    attr.txBlock = UART_BLOCK_STATE_NONE_BLOCK;
    
    if (UART_DEBUG_ID == 0) {
        /* GPIO0_PB6 => UART1_RX */
        PinctrlSet(GPIO0_PB6, MUX_FUNC3, PULL_KEEP, DRIVE_LEVEL3);
        /* GPIO0_PB7 => UART1_TX */
        PinctrlSet(GPIO0_PB7, MUX_FUNC3, PULL_KEEP, DRIVE_LEVEL3);
    } else if (UART_DEBUG_ID == 1) {
        /* GPIO0_PA6 => UART1_RX */
        PinctrlSet(GPIO0_PA6, MUX_FUNC3, PULL_KEEP, DRIVE_LEVEL3);
        /* GPIO0_PA7 => UART1_TX */
        PinctrlSet(GPIO0_PA7, MUX_FUNC3, PULL_KEEP, DRIVE_LEVEL3);
    }
    
    ret = LzUartInit(UART_DEBUG_ID, &attr);
    if (ret != LZ_HARDWARE_SUCCESS) {
        printf("%s, %d: LzUartInit(%u) failed!\n", __FILE__, __LINE__, ret);
    }
    
#if (LOSCFG_USE_SHELL == 1)
    (void)LOS_EventWrite(&g_shellInputEvent, 0x1);
#endif
    
    m_uart_debug_initialized = true;
}

uint8_t uart_debug_getc(void)
{
    uint8_t ch = 0;
    LzUartRead(UART_DEBUG_ID, &ch, 1);
    return ch;
}

void uart_debug_putc(char character)
{
    DebugWrite(UART_DEBUG_ID, &character, 1);
}

void _putchar(char charactor)
{
    uart_debug_putc(charactor);
}

