/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef HDI_SESSION_H
#define HDI_SESSION_H
#include <unordered_map>
#include <vector>
#include <memory>
#include "common/include/display_common.h"
#include "display_log.h"
#include "hdi_device_interface.h"
#include "hdi_display.h"
#include "hdi_netlink_monitor.h"
#include "v1_0/display_composer_type.h"
#include <mutex>

namespace OHOS {
namespace HDI {
namespace DISPLAY {
class HdiSession {
public:
    void Init();
    static HdiSession &GetInstance();

    template<typename... Args>
    int32_t CallDisplayFunction(uint32_t devId, int32_t (HdiDisplay::*member)(Args...), Args... args)
    {
        DISPLAY_LOGD("device Id : %{public}d", devId);
        DISPLAY_CHK_RETURN((devId == INVALIDE_DISPLAY_ID), DISPLAY_FAILURE, DISPLAY_LOGE("invalide device id"));
        std::lock_guard<std::mutex> lock(mMutex);
        auto iter = mHdiDisplays.find(devId);
        DISPLAY_CHK_RETURN((iter == mHdiDisplays.end()), DISPLAY_FAILURE,
            DISPLAY_LOGE("can not find display %{public}d", devId));
        auto display = iter->second.get();
		if (display == nullptr) {
            DISPLAY_LOGD("CallDisplayFunction display is null");
			return DISPLAY_FAILURE;
		}
        return (display->*member)(std::forward<Args>(args)...);
    }

    template<typename... Args>
    int32_t CallLayerFunction(uint32_t devId, uint32_t layerId, int32_t (HdiLayer::*member)(Args...), Args... args)
    {
        DISPLAY_LOGD("device Id : %{public}d", devId);
        DISPLAY_CHK_RETURN((devId == INVALIDE_DISPLAY_ID), DISPLAY_FAILURE, DISPLAY_LOGE("invalide device id"));
        std::lock_guard<std::mutex> lock(mMutex);
        auto iter = mHdiDisplays.find(devId);
        DISPLAY_CHK_RETURN((iter == mHdiDisplays.end()), DISPLAY_FAILURE,
            DISPLAY_LOGE("can not find display %{public}d", devId));
        auto display = iter->second.get();
		if (display == nullptr) {
            DISPLAY_LOGD("CallLayerFunction display is null");
			return DISPLAY_FAILURE;
		}		
        auto layer = display->GetHdiLayer(layerId);
        DISPLAY_CHK_RETURN((layer == nullptr), DISPLAY_FAILURE,
            DISPLAY_LOGE("can not find the layer %{public}d", layerId));
        return (layer->*member)(std::forward<Args>(args)...);
    }

    int32_t RegHotPlugCallback(HotPlugCallback callback, void *data);
    void DoHotPlugCallback(uint32_t devId, bool connect);
    void HandleHotplug(bool plugIn);

private:
    std::shared_ptr<HdiNetLinkMonitor> mNetLinkMonitor;
    std::unordered_map<uint32_t, std::shared_ptr<HdiDisplay>> mHdiDisplays;
    std::vector<std::shared_ptr<HdiDeviceInterface>> mHdiDevices;
    std::unordered_map<HotPlugCallback, void *> mHotPlugCallBacks;
    std::mutex mMutex;
};
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY

#endif // HDI_SESSION_H
