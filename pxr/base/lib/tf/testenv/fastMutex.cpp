#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/fastMutex.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/arch/threads.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using boost::bind;
using boost::function;

#define NUM_LOOPS       10000
#define WAIT_LOOPS      100
#define TEST_COUNT      3

TfFastMutex* fmut;
volatile bool fin = false;

/*
 * This test works by creating a fast mutex and having TEST_COUNT threads
 * attempt to enter it.  When a thread locks the mutex it checks to
 * see that it is the only thread in the mutex.
 */

static bool
Task()
{
    for(size_t i = 0; i < NUM_LOOPS; i++) {
        fmut->Lock();
        if(fin) {
            fmut->Unlock();
            return false;
        }
        fin = true;
        for(size_t j = 0; j < WAIT_LOOPS; j++);
        fin = false;
        fmut->Unlock();
    }
    return true;
}

static bool
RunFastMutexTest()
{
    TfThreadDispatcher d;
    function<bool ()> fnTask(&Task);
    TfThread<bool>::Ptr t[TEST_COUNT];
    size_t i = 0;
    bool ok = true;

    fmut = new TfFastMutex();

    ArchSetThreadConcurrency(TEST_COUNT + 2);

    for(i = 0; i < TEST_COUNT; i++) {
        t[i] = d.Start(fnTask);
    }

    for(i = 0; i < TEST_COUNT; i++) {
        ok &= t[i]->GetResult();
    }

    delete fmut;

    return ok;
}

static bool
Test_TfFastMutex()
{
    bool retVal = true;

    retVal &= RunFastMutexTest();

    return retVal;
}

TF_ADD_REGTEST(TfFastMutex);
