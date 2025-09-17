//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/spinRWMutex.h"
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
TfSpinRWMutex::_WaitForWriter() const
{
    // Wait until we see a cleared WriterFlag.
    WaitWithBackoff([this]() {
        return !(_lockState.load() & WriterFlag);
    });
}

void
TfSpinRWMutex::_WaitForReaders() const
{
    // Wait until we see zero readers.
    WaitWithBackoff([this]() {
        return _lockState.load() == WriterFlag;
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
