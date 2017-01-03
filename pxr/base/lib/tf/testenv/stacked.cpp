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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stacked.h"
#include "pxr/base/tf/instantiateStacked.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

#include "pxr/base/arch/demangle.h"

#include <iostream>
#include <thread>
#include <cstdio>



class Tf_SafeStacked : public TfStacked<Tf_SafeStacked> {
public:
    explicit Tf_SafeStacked(int v) : value(v) {}
    int value;
};
TF_INSTANTIATE_STACKED(Tf_SafeStacked);

class Tf_UnsafeStacked :
    public TfStacked<Tf_UnsafeStacked, /* PerThread = */ false> {
public:
    explicit Tf_UnsafeStacked(int v) : value(v) {}
    int value;
};
TF_INSTANTIATE_STACKED(Tf_UnsafeStacked);


class Tf_FallbackStacked : public TfStacked<Tf_FallbackStacked> {
public:
    explicit Tf_FallbackStacked(int v) : value(v) {}
    int value;

private:
    friend class TfStackedAccess;
    static void _InitializeStack() {
        new Tf_FallbackStacked(-2);
        new Tf_FallbackStacked(-1);
    }
};
TF_INSTANTIATE_STACKED(Tf_FallbackStacked);


template <class T>
static void PrintStack() {
    printf("%s : ", ArchGetDemangled<T>().c_str());
    TF_FOR_ALL(i, T::GetStack())
        printf("%d, ", (*i)->value);
    printf("\n");
}


template <class Stacked>
static void Test()
{
    typedef typename Stacked::Stack Stack;
    
    PrintStack<Stacked>();

    {
        TF_AXIOM(Stacked::GetStackTop() == 0);
        TF_AXIOM(Stacked::GetStackPrevious() == 0);
        Stack const &stack = Stacked::GetStack();
        TF_AXIOM(stack.size() == 0);
    }
        
    {
        Stacked a(1);
        Stacked b(2);

        {
            PrintStack<Stacked>();
            TF_AXIOM(Stacked::GetStackTop()->value == 2);
            TF_AXIOM(Stacked::GetStackPrevious()->value == 1);
            
            Stack const &stack = Stacked::GetStack();
            TF_AXIOM(stack.size() == 2);
            TF_AXIOM(stack[0]->value == 1 &&
                     stack[1]->value == 2);
        }

        {
            Stacked c(3);
            Stacked d(4);
            Stacked e(5);
            PrintStack<Stacked>();
            
            TF_AXIOM(Stacked::GetStackTop()->value == 5);
            TF_AXIOM(Stacked::GetStackPrevious()->value == 4);
            
            Stack const &stack = Stacked::GetStack();
            TF_AXIOM(stack.size() == 5);
            TF_AXIOM(stack[0]->value == 1 && 
                     stack[1]->value == 2 && 
                     stack[2]->value == 3 && 
                     stack[3]->value == 4 && 
                     stack[4]->value == 5);
        }

        {
            PrintStack<Stacked>();
            
            TF_AXIOM(Stacked::GetStackTop()->value == 2);
            TF_AXIOM(Stacked::GetStackPrevious()->value == 1);
            
            Stack const &stack = Stacked::GetStack();
            TF_AXIOM(stack.size() == 2);
            TF_AXIOM(stack[0]->value == 1 && 
                     stack[1]->value == 2);
        }

    }

    PrintStack<Stacked>();

    {
        TF_AXIOM(Stacked::GetStackTop() == 0);
        TF_AXIOM(Stacked::GetStackPrevious() == 0);
        Stack const &stack = Stacked::GetStack();
        TF_AXIOM(stack.size() == 0);
    }
}

struct TestFallbackStackedInAThread
{
    void operator()() {
        // This will run in multiple threads to make sure that the 
        // stack intializers have run once per-thread.
        TF_AXIOM(Tf_FallbackStacked::GetStackTop()->value == -1);
        TF_AXIOM(Tf_FallbackStacked::GetStackPrevious()->value == -2);
    }
};


static bool
Test_TfStacked() {

    Test<Tf_SafeStacked>();
    Test<Tf_UnsafeStacked>();

    // Test SafeStacked in multiple threads
    TF_AXIOM(std::thread::hardware_concurrency() > 1);
    std::vector<std::thread> threads;
    for (size_t i = 0, n = std::thread::hardware_concurrency(); i != n; ++i) {
        threads.emplace_back(TestFallbackStackedInAThread());
    }
    for (size_t i = 0, n = std::thread::hardware_concurrency(); i != n; ++i) {
        threads[i].join();
    }


    // Fallback values.
    PrintStack<Tf_FallbackStacked>();
    TF_AXIOM(Tf_FallbackStacked::GetStackTop()->value == -1);
    TF_AXIOM(Tf_FallbackStacked::GetStackPrevious()->value == -2);
    {
        Tf_FallbackStacked s(1234);
        PrintStack<Tf_FallbackStacked>();
        TF_AXIOM(Tf_FallbackStacked::GetStackTop()->value == 1234);
        TF_AXIOM(Tf_FallbackStacked::GetStackPrevious()->value == -1);

        {
            Tf_FallbackStacked s(2345);
            PrintStack<Tf_FallbackStacked>();
            TF_AXIOM(Tf_FallbackStacked::GetStackTop()->value == 2345);
            TF_AXIOM(Tf_FallbackStacked::GetStackPrevious()->value == 1234);
        }
            
        PrintStack<Tf_FallbackStacked>();
        TF_AXIOM(Tf_FallbackStacked::GetStackTop()->value == 1234);
        TF_AXIOM(Tf_FallbackStacked::GetStackPrevious()->value == -1);
    }
    PrintStack<Tf_FallbackStacked>();
    TF_AXIOM(Tf_FallbackStacked::GetStackTop()->value == -1);
    TF_AXIOM(Tf_FallbackStacked::GetStackPrevious()->value == -2);
    
    
    return true;
}


TF_ADD_REGTEST(TfStacked);
