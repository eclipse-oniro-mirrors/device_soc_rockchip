/*
 * Copyright (c) 2022 Shenzhen Kaihong DID Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hdi_mpp_component_manager.h"
#include <map>
#include <mutex>

#ifdef __cplusplus
extern "C"
{
#endif

std::map<uintptr_t, uintptr_t> g_mppComponents;
std::mutex g_mppComponentLock;

bool AddToMppComponentManager(CODEC_HANDLETYPE codecHandle, RKHdiBaseComponent *mppComponent)
{
    uintptr_t handle = reinterpret_cast<uintptr_t>(codecHandle);
    uintptr_t component = reinterpret_cast<uintptr_t>(mppComponent);
    std::unique_lock<std::mutex> autoLock(g_mppComponentLock);
    std::pair<std::map<uintptr_t, uintptr_t>::iterator, bool> ret =
        g_mppComponents.insert(std::pair<uintptr_t, uintptr_t>(handle, component));
    bool result = ret.second;
    return result;
}

RKHdiBaseComponent* FindInMppComponentManager(CODEC_HANDLETYPE codecHandle)
{
    uintptr_t handle = reinterpret_cast<uintptr_t>(codecHandle);
    std::unique_lock<std::mutex> autoLock(g_mppComponentLock);
    std::map<uintptr_t, uintptr_t>::iterator it = g_mppComponents.find(handle);
    if (it == g_mppComponents.end()) {
        return nullptr;
    }
    uintptr_t ret = it->second;
    return reinterpret_cast<RKHdiBaseComponent *>(ret);
}

void RemoveFromMppComponentManager(CODEC_HANDLETYPE codecHandle)
{
    uintptr_t handle = reinterpret_cast<uintptr_t>(codecHandle);
    std::unique_lock<std::mutex> autoLock(g_mppComponentLock);
    g_mppComponents.erase(handle);
}

#ifdef __cplusplus
}
#endif
