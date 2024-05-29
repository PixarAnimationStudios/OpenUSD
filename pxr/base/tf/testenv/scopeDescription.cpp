//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticLite.h"
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
