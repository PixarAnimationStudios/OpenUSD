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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/flyweight.h"
#include "pxr/base/tf/stopwatch.h"

#include <functional>
#include <iostream>
#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE

using _Bits = std::vector<bool>;
using FlyBits = TfFlyweight<_Bits, std::hash<_Bits>>;

static const size_t NumThreads = 50;
static const size_t NumIters = 100000;

static void 
Flyweight_ThreadTask21599(FlyBits *flybits, size_t taskIndex)
{
    for (size_t i = 0; i < NumIters; ++i) {
        _Bits bits(i%(NumThreads*100));
        flybits[taskIndex] = FlyBits(bits);
    }
}

static void 
Flyweight_MTStressTestBug21599()
{
    TfStopwatch sw;

    // This test creates one flyweight per thread that the threads then
    // write into.  This test helped tracked down the cause of bug 21599.
    //
    FlyBits flybits[NumThreads];

    std::vector<std::thread> threads;

    sw.Start();

    for (size_t i = 0; i < NumThreads; ++i) {
        threads.emplace_back(&Flyweight_ThreadTask21599, flybits, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    sw.Stop();

    FlyBits::DumpStats();

    std::cout << "Total time: " << sw.GetMilliseconds() << " ms " << std::endl;
}

static bool
Test_TfFlyweight()
{
    Flyweight_MTStressTestBug21599();
    return true;
}


TF_ADD_REGTEST(TfFlyweight);
