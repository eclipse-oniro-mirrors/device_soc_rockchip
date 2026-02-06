/*
 *
 * Copyright 2013 Rockchip Electronics Co., LTD.
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

#ifndef VPU_API_PRIVATE_CMD_H_
#define VPU_API_PRIVATE_CMD_H_

/* note: do not conlict with VPU_API_CMD */
typedef enum VPU_API_PRIVATE_CMD {
    VPU_API_PRIVATE_CMD_NONE        = 0x0,
    VPU_API_PRIVATE_HEVC_NEED_PARSE = 0x1000,

} VPU_API_PRIVATE_CMD;

#endif
