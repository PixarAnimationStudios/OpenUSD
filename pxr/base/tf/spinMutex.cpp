//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/spinMutex.h"
#include "pxr/base/arch/threads.h"

#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

static constexpr int SpinsBeforeBackoff = 32;

template <class Fn>
static void WaitWithBackoff(Fn &&fn) {
    // Hope for the best...
    if (ARCH_LIKELY(fn())) {
        return;
    }
    // Otherwise spin for a bit...
    for (int i = 0; i != SpinsBeforeBackoff; ++i) {
        ARCH_SPIN_PAUSE();
        if (fn()) {
            return;
        }
    }
    // Keep checking but yield our thread...
    do {
        std::this_thread::yield();
    } while (!fn());
}

void
TfSpinMutex::_AcquireContended()
{
    WaitWithBackoff([this]() {
        return _lockState.exchange(true, std::memory_order_acquire) == false;
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
