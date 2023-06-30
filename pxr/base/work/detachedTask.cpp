//
// Copyright 2016 Pixar
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
#include "pxr/base/work/detachedTask.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/threadLimits.h"

#include <atomic>
#include <chrono>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

WorkDispatcher &
Work_GetDetachedDispatcher()
{
    // Deliberately leak this in case there are tasks still using it after we
    // exit from main().
    static WorkDispatcher *theDispatcher = new WorkDispatcher;
    return *theDispatcher;
}

static std::atomic<std::thread *> detachedWaiter { nullptr };

void
Work_EnsureDetachedTaskProgress()
{
    // Check to see if there's a waiter thread already.  If not, try to create
    // one.
    std::thread *c = detachedWaiter.load();
    if (ARCH_UNLIKELY(!c)) {
        std::thread *newThread = new std::thread;
        if (detachedWaiter.compare_exchange_strong(c, newThread)) {
            // We won the race, so start the waiter thread.
            WorkDispatcher &dispatcher = Work_GetDetachedDispatcher();
            *newThread =
                std::thread([&dispatcher]() {
                        while (true) {
                            // Process detached tasks.
                            dispatcher.Wait();
                            // Now sleep for a bit, and try again.
                            using namespace std::chrono_literals;
                            std::this_thread::sleep_for(50ms);
                        }
                    });
            newThread->detach();
        }
        else {
            // We lost the race, so delete our temporary thread.
            delete newThread;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
