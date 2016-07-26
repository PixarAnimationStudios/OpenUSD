#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/semaphore.h"
#include "pxr/base/tf/atomicInteger.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/arch/threads.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using boost::bind;
using boost::function;

#define NUM_LOOPS       10000
#define WAIT_LOOPS      100
#define MIN_TEST_COUNT  1
#define MAX_TEST_COUNT  3

TfSemaphore* sem;
TfAtomicInteger aiCount = 0;

/*
 * This test works by creating a semaphore and having twice the
 * number of threads as the semaphore's count attempt to enter it.
 * When a thread enters the semaphore it checks to see that no more
 * than the semaphore's count of threads are in it at once.
 * The test is repeated for semaphores of various counts.
 */

static bool
Task(int count)
{
    for(int i = 0; i < NUM_LOOPS; i++) {
        sem->Wait();
        aiCount.Increment();
        if(aiCount > count) {
            sem->Post();
            return false;
        }
        for(int j = 0; j < WAIT_LOOPS; j ++);
        aiCount.Decrement();
        sem->Post();
    }
    return true;
}

static bool
RunSemaphoreTest(int count)
{
    TfThreadDispatcher d;
    function<bool ()> fnTask = bind(&Task, count);
    TfThread<bool>::Ptr t[MAX_TEST_COUNT * 2];
    int i = 0,
        threadCount = count * 2;
    bool ok = true;

    sem = new TfSemaphore(count);
    aiCount.Set(0);

    ArchSetThreadConcurrency(threadCount + 2);

    for(i = 0; i < threadCount; i++) {
        t[i] = d.Start(fnTask);
    }

    for(i = 0; i < threadCount; i++) {
        ok &= t[i]->GetResult();
    }

    delete sem;

    if(!ok) printf("Semaphore with count %d and %d threads failed.\n",
                   count, threadCount);

    return ok;
}

static bool
Test_TfSemaphore()
{
    bool retVal = true;
    int i;

    for(i = MIN_TEST_COUNT; i <= MAX_TEST_COUNT; i ++) {
        retVal &= RunSemaphoreTest(i);
    }

    return retVal;
}

TF_ADD_REGTEST(TfSemaphore);
