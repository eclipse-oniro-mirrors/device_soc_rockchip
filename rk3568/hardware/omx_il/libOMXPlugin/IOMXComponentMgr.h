/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
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
#ifndef I_OMX_COMPONENT_MANAGER_H
#define I_OMX_COMPONENT_MANAGER_H

#include <iostream>
#include <vector>
#include "OMX_Component.h"

namespace OHOS {
namespace Media {
    namespace OMX {
        class IOMXComponentMgr {
        public:
            IOMXComponentMgr() {}
            virtual ~IOMXComponentMgr() {}
            IOMXComponentMgr(const IOMXComponentMgr&) = delete;
            IOMXComponentMgr& operator=(const IOMXComponentMgr&) = delete;

            virtual OMX_ERRORTYPE
            CreateComponentInstance(const OMX_STRING        componentName,
                                    const OMX_CALLBACKTYPE* callbacks,
                                    OMX_PTR                 appData,
                                    OMX_COMPONENTTYPE**     component) = 0;

            virtual OMX_ERRORTYPE
            DeleteComponentInstance(OMX_COMPONENTTYPE* component) = 0;

            virtual OMX_ERRORTYPE
            EnumerateComponentsByIndex(OMX_U32    index,
                                       OMX_STRING componentName,
                                       size_t     componentNameSize) = 0;

            virtual OMX_ERRORTYPE
            GetRolesForComponent(const OMX_STRING          componentName,
                                 std::vector<std::string>* vRoles) = 0;
        };
    }  // namespace OMX
}  // namespace Media
}  // namespace OHOS

#endif