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
#include "pxr/base/work/utils.h"

#include <atomic>
#include <cstdio>
#include <thread>


PXR_NAMESPACE_USING_DIRECTIVE

struct _Tester {
    _Tester() = default;
    _Tester(_Tester const &) = delete;
    _Tester &operator=(_Tester const &) = delete;
    _Tester(_Tester &&other) : dtor(other.dtor) { other.dtor = nullptr; }
    _Tester &operator=(_Tester &&other) {
        dtor = other.dtor; other.dtor = nullptr;
        return *this;
    }
    ~_Tester() { if (dtor) { *dtor = true; } }
    std::atomic_bool *dtor = nullptr;
};

void swap(_Tester &l, _Tester &r) { std::swap(l.dtor, r.dtor); }

// This type provides a swap overload along with copy operations that cause
// test failure.  We want to ensure that we are able to perform swap-based
// async destruction without accidental copies.
struct _SwapOnlyTester {
    _SwapOnlyTester() = default;
    _SwapOnlyTester(_SwapOnlyTester const &) {
        TF_FATAL_ERROR("Unexpectedly invoked copy constructor");
    }
    _SwapOnlyTester &operator=(_SwapOnlyTester const &) {
        TF_FATAL_ERROR("Unexpectedly invoked copy assignment");
        return *this;
    }
    ~_SwapOnlyTester() { if (dtor) { *dtor = true; } }
    std::atomic_bool *dtor = nullptr;
};

void swap(_SwapOnlyTester &l, _SwapOnlyTester &r) { std::swap(l.dtor, r.dtor); }

int
main()
{
    constexpr size_t numIters = 10000;
    std::atomic_int counter;
    counter = 0;

    printf("Test WorkRunDetachedTask... ");
    for (size_t i = 0; i != numIters; ++i) {
        WorkRunDetachedTask([&counter]() { ++counter; });
    }
    while (counter != numIters) { /* spin */ std::this_thread::yield(); }
    printf("OK\n");

    _Tester t;
    std::atomic_bool ranDtor { false };

    printf("Test WorkSwapDestroyAsync... ");
    t.dtor = &ranDtor;
    WorkSwapDestroyAsync(t);
    TF_AXIOM(!t.dtor);
    while (!ranDtor) { /* spin */ std::this_thread::yield(); }
    printf("OK\n");

    printf("Test WorkMoveDestroyAsync... ");
    ranDtor = false;
    t.dtor = &ranDtor;
    WorkMoveDestroyAsync(t);
    while (!ranDtor) { /* spin */ std::this_thread::yield(); }
    printf("OK\n");

    _SwapOnlyTester s;

    printf("Test WorkSwapDestroyAsync (swap-only type)... ");
    ranDtor = false;
    s.dtor = &ranDtor;
    WorkSwapDestroyAsync(s);
    while (!ranDtor) { /* spin */ std::this_thread::yield(); }
    printf("OK\n");

    return 0;
}
