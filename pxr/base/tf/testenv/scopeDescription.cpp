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
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/stopwatch.h"

#include <string>
#include <thread>
#include <vector>

using std::string;
using std::vector;
PXR_NAMESPACE_USING_DIRECTIVE

static void
_PushPopStackDescriptions(int i) {
    TF_DESCRIBE_SCOPE("Description %d 1", i); {
        TF_DESCRIBE_SCOPE("Description %d 3", i); {
            TF_DESCRIBE_SCOPE("=== Intermission ==="); {
                TF_DESCRIBE_SCOPE("Description %d 5", i); {
                    TF_DESCRIBE_SCOPE("Description %d 6", i); {
                        TF_DESCRIBE_SCOPE("Description %d 7", i); {
                            TF_DESCRIBE_SCOPE("Description %d 8", i); {
                                TF_DESCRIBE_SCOPE("!!! Finale !!!"); {
                                    TF_DESCRIBE_SCOPE("Description %d 10", i); {
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

static void
TestThreads()
{
    TF_DESCRIBE_SCOPE("Test TfScopeDescription: TestThreads");
    
    constexpr int nthreads = 64;

    vector<std::thread> threads;
    for (int i = 0; i != nthreads; ++i) {
        threads.emplace_back([i]() {
                TfStopwatch sw;
                while (sw.GetSeconds() < 1.0) {
                    sw.Start();
                    _PushPopStackDescriptions(i);
                    sw.Stop();
                }
            });
    }

#if 0 // Force a segfault, for debugging & testing
    int *x = nullptr;
    int y = *x;
    printf("y is %d\n", y);
#endif
    
    for (auto &t: threads) {
        t.join();
    }
}

static void
TestOverhead()
{
    size_t count = 0;
    unsigned int val = 0;
    TfStopwatch sw;
    do {
        sw.Start();
        val += rand();
        ++count;
        sw.Stop();
    } while (sw.GetSeconds() < 0.5);
    // printf("%zd rand calls in %f seconds: %u\n",
    //        count, sw.GetSeconds(), val);
    double baseSecsPerCall = sw.GetSeconds() / double(count);

    count = 0;
    val = 0;
    sw.Reset();
    do {
        sw.Start();
        TF_DESCRIBE_SCOPE("calling rand");
        val += rand();
        ++count;
        sw.Stop();
    } while (sw.GetSeconds() < 0.5);
    // printf("%zd described rand calls in %f seconds: %u\n",
    //        count, sw.GetSeconds(), val);
    double describedSecsPerCall = sw.GetSeconds() / double(count);

    printf("TF_DESCRIBE_SCOPE overhead approx %f ns\n",
           (describedSecsPerCall - baseSecsPerCall) / 1e-9);
}

static void
TestBasics()
{
    vector<string> stack;
    
    TF_AXIOM(TfGetCurrentScopeDescriptionStack().empty());
    
    {
        TF_DESCRIBE_SCOPE("one");
        
        stack = TfGetCurrentScopeDescriptionStack();
        TF_AXIOM(stack.size() == 1 && stack.back() == "one");
        
        {
            TF_DESCRIBE_SCOPE("two");
            
            stack = TfGetCurrentScopeDescriptionStack();
            TF_AXIOM(stack.size() == 2 && stack.back() == "two");
            
        }
        
        stack = TfGetCurrentScopeDescriptionStack();
        TF_AXIOM(stack.size() == 1 && stack.back() == "one");
        {
            TF_DESCRIBE_SCOPE("%s", "three");
            
            stack = TfGetCurrentScopeDescriptionStack();
            TF_AXIOM(stack.size() == 2 && stack.back() == "three");
            
        }
        
        stack = TfGetCurrentScopeDescriptionStack();
        TF_AXIOM(stack.size() == 1 && stack.back() == "one");
        
    }
    
    TF_AXIOM(TfGetCurrentScopeDescriptionStack().empty());
}

static bool
Test_TfScopeDescription()
{
    TestBasics();
    TestThreads();
    TestOverhead();
    return true;
};


TF_ADD_REGTEST(TfScopeDescription);
