#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/barrier.h"
#include "pxr/base/tf/atomicInteger.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/arch/inttypes.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using boost::bind;
using boost::function;

#define NUM_LOOPS       500
#define WAIT_LOOPS      100
#define MIN_TEST_COUNT  2
#define MAX_TEST_COUNT  4

TfBarrier* bar;
TfAtomicInteger aiWaitCount;

/*
 * In this test a barrier is created and a number of threads equal to its count
 * are executed.  Threads are launched at the barrier with tasks of random
 * length.  The number of threads waiting at the barrier is counted, and
 * when a thread passes the barrier it checks that this is the same as the
 * barrier's count and that no threads are still waiting on the barrier.
 * This test is repeated for barriers of various counts.
 * Barriers with low counts are used to test spin lock.
 */

static bool
Task(size_t count)
{
    int loops = rand() % WAIT_LOOPS;
    for (int i = 0; i < loops; i++)
        ;
    aiWaitCount.Increment();
    bar->Wait();
    return !bar->IsWaitActive() && aiWaitCount == int(count);
}

static bool
RunBarrierTest(size_t count, bool spin)
{
    TfThreadDispatcher d;
    function<bool ()> fnTask = bind(&Task, count);
    TfThread<bool>::Ptr t[MAX_TEST_COUNT];
    size_t i, j;
    bool ok = true;

    bar = new TfBarrier(count);

    if(bar->GetSpinMode()) {
        printf("GetSpinMode is true, expected false\n");
        return false;
    }
    if(bar->IsWaitActive()) {
        printf("IsWaitActive is true, expected false\n");
        return false;
    }
    if(bar->GetSize() != count) {
        printf("GetSize is %zu, expected %zu\n", bar->GetSize(), count);
        return false;
    }
    bar->SetSize(count);
    if(bar->GetSize() != count) {
        printf("GetSize is %zu, expected %zu\n",
               bar->GetSize(), count);
        return false;
    }
    bar->SetSpinMode(spin);
    if(bar->GetSpinMode() != spin) {
        printf("GetSpinMode is %s, expected %s\n",
               bar->GetSpinMode()?"true":"false", spin?"true":"false");
        return false;
    }

    ArchSetThreadConcurrency(count + 2);

    for(j = 0; j < NUM_LOOPS; j++) {
        aiWaitCount.Set(0);

        for(i = 0; i < count; i++) {
            t[i] = d.Start(fnTask);
        }

        for(i = 0; i < count; i++) {
            ok &= t[i]->GetResult();
        }
    }

    delete bar;

    if(!ok) printf("Barrier with count %zu and spin mode %s failed\n",
                   count, spin?"true":"false");

    return ok;
}

static bool
Test_TfBarrier()
{
    bool retVal = true;
    int i;

    srand(time(NULL));

    for(i = MIN_TEST_COUNT; i <= MAX_TEST_COUNT; i ++) {
        retVal &= RunBarrierTest(i, i <= 2);
    }

    return retVal;
}

TF_ADD_REGTEST(TfBarrier);
