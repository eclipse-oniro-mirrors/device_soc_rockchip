/*
 * Copyright (C) 2021 HiHope Open Source Organization .
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

#include "OMXPlugin.h"
#include <dlfcn.h>
#include "media_log.h"

using namespace std;
using namespace OHOS;
using namespace Media;
using namespace OMX;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "OMXPluginhw"};
}

extern "C" IOMXComponentMgr *createOMXPlugin()
{
    MEDIA_LOGI("OMX IL createOMXPlugin");
    return new OMXPlugin;
}

extern "C" void destroyOMXPlugin(IOMXComponentMgr *OMXPlugin)
{
    MEDIA_LOGI("OMX IL destroyOMXPlugin");
    delete OMXPlugin;
}

OMXPlugin::OMXPlugin()
{
    AddCore("libOMX_Core.z.so");
}

OMX_ERRORTYPE OMXPlugin::AddCore(const char* coreName)
{
    void* libHandle = dlopen(coreName, RTLD_NOW);

    if (libHandle != nullptr) {
        OMXCore* core = (OMXCore*)calloc(1, sizeof(OMXCore));

    if (!core) {
        dlclose(libHandle);
        return OMX_ErrorUndefined;
    }

        core->mLibHandle = libHandle;
        core->mInit = (OMXCore::InitFunc)dlsym(libHandle, "OMX_Init");
        core->mDeinit = (OMXCore::DeinitFunc)dlsym(libHandle, "OMX_Deinit");
        core->mComponentNameEnum =
            (OMXCore::ComponentNameEnumFunc)dlsym(libHandle, "OMX_ComponentNameEnum");
        core->mGetHandle = (OMXCore::GetHandleFunc)dlsym(libHandle, "OMX_GetHandle");
        core->mFreeHandle = (OMXCore::FreeHandleFunc)dlsym(libHandle, "OMX_FreeHandle");
        core->mGetRolesOfComponentHandle =
            (OMXCore::GetRolesOfComponentFunc)dlsym(libHandle, "OMX_GetRolesOfComponent");

        if (core->mInit != nullptr) {
            (*(core->mInit))();
        }

        if (core->mComponentNameEnum != NULL) {
            // calculating number of components registered inside given OMX core
            char tmpComponentName[OMX_MAX_STRINGNAME_SIZE] = { 0 };
            OMX_U32 tmpIndex = 0;
            while (OMX_ErrorNone == ((*(core->mComponentNameEnum))(tmpComponentName,
                OMX_MAX_STRINGNAME_SIZE, tmpIndex))) {
                tmpIndex++;
            MEDIA_LOGI("OMX IL core %{public}s: declares component %{public}s", coreName, tmpComponentName);
            }
            core->mNumComponents = tmpIndex;
            MEDIA_LOGI("OMX IL core %{public}s: contains %{public}u components", coreName, core->mNumComponents);
        }
        // add plugin to the vector
        mCores.push_back(core);
    } else {
        MEDIA_LOGW("OMX IL core %{public}s not found", coreName);
        return OMX_ErrorUndefined; // Do we need to return error message
    }
    return OMX_ErrorNone;
}

OMXPlugin::~OMXPlugin()
{
    for (OMX_U32 i = 0; i < mCores.size(); i++) {
        if (mCores[i] != NULL && mCores[i]->mLibHandle != NULL) {
            (*(mCores[i]->mDeinit))();

            dlclose(mCores[i]->mLibHandle);
            free(mCores[i]);
        }
    }
}

OMX_ERRORTYPE OMXPlugin::CreateComponentInstance(
    const OMX_STRING name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
{
    for (OMX_U32 i = 0; i < mCores.size(); i++) {
        if (mCores[i] != NULL) {
            if (mCores[i]->mLibHandle == NULL) {
                continue;
            }

            OMX_ERRORTYPE omx_res = (*(mCores[i]->mGetHandle))(
                reinterpret_cast<OMX_HANDLETYPE *>(component),
                const_cast<char *>(name),
                appData, const_cast<OMX_CALLBACKTYPE *>(callbacks));
            if (omx_res == OMX_ErrorNone) {
                unique_lock<mutex> lock(mMutex);
                OMXComponent comp;

                comp.mComponent = *component;
                comp.mCore = mCores[i];

                mComponents.push_back(comp);
                return OMX_ErrorNone;
            } else if (omx_res == OMX_ErrorInsufficientResources) {
                return omx_res;
            }
        }
    }
    return OMX_ErrorInvalidComponentName;
}

OMX_ERRORTYPE OMXPlugin::DeleteComponentInstance(
    OMX_COMPONENTTYPE *component)
{
    unique_lock<mutex> lock(mMutex);
    for (OMX_U32 i = 0; i < mComponents.size(); i++) {
        if (mComponents[i].mComponent == component) {
            if (mComponents[i].mCore == NULL || mComponents[i].mCore->mLibHandle == NULL) {
                return OMX_ErrorUndefined;
            }
            OMX_ERRORTYPE omx_res =
                (*(mComponents[i].mCore->mFreeHandle))(reinterpret_cast<OMX_HANDLETYPE *>(component));
            mComponents.erase(mComponents.begin() + i);
            return omx_res;
        }
    }
    return OMX_ErrorInvalidComponent;
}

OMX_ERRORTYPE OMXPlugin::EnumerateComponentsByIndex(
    OMX_U32 index,
    OMX_STRING name,
    size_t size)
{
    // returning components
    OMX_U32 relativeIndex = index;
    for (OMX_U32 i = 0; i < mCores.size(); i++) {
        if (mCores[i]->mLibHandle == NULL) {
            continue;
        }
        if (relativeIndex < mCores[i]->mNumComponents) {
            return ((*(mCores[i]->mComponentNameEnum))(name, size, relativeIndex));
        } else {
            relativeIndex -= mCores[i]->mNumComponents;
        }
    }
    return OMX_ErrorNoMore;
}

OMX_ERRORTYPE OMXPlugin::GetRolesForComponent(
    const OMX_STRING name,
    vector<string> *roles)
{
    roles->clear();
    for (OMX_U32 j = 0; j < mCores.size(); j++) {
        if (mCores[j]->mLibHandle == NULL) {
            continue;
        }

        OMX_U32 numRoles;
        OMX_ERRORTYPE err = (*(mCores[j]->mGetRolesOfComponentHandle))(
                const_cast<OMX_STRING>(name), &numRoles, NULL);

        if (err != OMX_ErrorNone) {
            continue;
        }

        if (numRoles > 0) {
            OMX_U8 **array = new OMX_U8 *[numRoles];
            for (OMX_U32 i = 0; i < numRoles; ++i) {
                array[i] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
            }

            OMX_U32 numRoles2 = numRoles;
            err = (*(mCores[j]->mGetRolesOfComponentHandle))(
                    const_cast<OMX_STRING>(name), &numRoles2, array);
            if ((err != OMX_ErrorNone) || (numRoles != numRoles2)) {
                return OMX_ErrorInvalidComponent;
            }

            for (OMX_U32 i = 0; i < numRoles; ++i) {
                string s((const char *)array[i]);
                roles->push_back(s);

                delete[] array[i];
                array[i] = NULL;
            }

            delete[] array;
            array = NULL;
        }
        return OMX_ErrorNone;
    }
    return OMX_ErrorInvalidComponent;
}

