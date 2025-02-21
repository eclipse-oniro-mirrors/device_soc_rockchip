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
#ifndef __UART_DEBUG_H_
#define __UART_DEBUG_H_

#include <stdio.h>
#include <stdint.h>
#include "los_event.h"
#include "los_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

extern EVENT_CB_S g_shellInputEvent;

void uart_debug_init(void);
uint8_t uart_debug_getc(void);
void uart_debug_putc(char character);
void _putchar(char character);

#ifdef __cplusplus
}
#endif

#endif