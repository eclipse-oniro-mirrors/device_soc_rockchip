/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
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

#ifndef __MPP_ENV_H__
#define __MPP_ENV_H__

#ifdef __cplusplus
extern "C" {
#endif

signed int mpp_env_get_u32(const char *name, unsigned int *value, unsigned int default_value);
signed int mpp_env_get_str(const char *name, const char **value, const char *default_value);

signed int mpp_env_set_u32(const char *name, unsigned int value);
signed int mpp_env_set_str(const char *name, char *value);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_ENV_H__ */
