/*
 * Copyright (c) 2023 Shenzhen Kaihong DID Co., Ltd.
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
#ifndef SCOPE_GUARD_H
#define SCOPE_GUARD_H
#include <functional>
#include <nocopyable.h>
namespace OHOS {
class CodecScopeGuard : public NoCopyable {
public:
    explicit CodecScopeGuard(std::function<void()> onExitFunc) : exitFunc_(std::move(onExitFunc)), sacked_(false)
    {}
    ~CodecScopeGuard()
    {
        if (!sacked_ && exitFunc_ != nullptr) {
            exitFunc_();
        }
    }
    void Sack()
    {
        sacked_ = true;
    }

private:
    std::function<void()> exitFunc_;
    bool sacked_;
};
}  // namespace OHOS
#endif
