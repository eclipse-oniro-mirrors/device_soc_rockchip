/*
 * Copyright (c) 2022 Shenzhen Kaihong DID Co., Ltd.
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

#ifndef HDI_MPP_EXT_PARAMS_H
#define HDI_MPP_EXT_PARAMS_H

#include "codec_type.h"

typedef enum {
    KEY_EXT_START = KEY_VENDOR_START_NONE,
    KEY_EXT_DEFAULT_CFG_RK,             /**< Default config. Used for RK codec. */
    KEY_EXT_SPLIT_PARSE_RK,             /**< Split parse. Used for RK codec. */
    KEY_EXT_DEC_FRAME_NUM_RK,           /**< Decode frame number. Used for RK codec. */
    KEY_EXT_SETUP_DROP_MODE_RK,         /**< Drop mode setup. Used for RK codec. */
    KEY_EXT_ENC_VALIDATE_SETUP_RK,      /**< Validate all config setup. Used for RK codec. */
    KEY_EXT_ENC_SETUP_AVC_RK,           /**< AVC config setup. Used for RK codec. */
    KEY_EXT_ENC_FRAME_NUM_RK,           /**< Frame num setup. Used for RK codec. */
} ParamExtKey;

#endif // _EXT_PARAMS