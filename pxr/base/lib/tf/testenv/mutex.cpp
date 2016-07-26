#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/mutex.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/arch/threads.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using boost::bind;
using boost::function;

#define NUM_LOOPS       10000
#define WAIT_LOOPS      100
#define TEST_COUNT      3

TfMutex* mut;
volatile bool in = false;

/*
 * This test works by creating a mutex and having TEST_COUNT threads
 * attempt to enter it.  When a thread locks the mutex it checks to
 * see that it is the only thread in the mutex.
 * The test is run for recursive and non-recursive mutexes.
 */

static void
Lock(bool recursive)
{
    if(rand() % 2 == 0) mut->Lock();
    else while(!mut->TryLock());
    if(recursive) mut->Start();
}

static void
Unlock(bool recursive)
{
    if(recursive) mut->Stop();
    mut->Unlock();
}

static bool
Task(bool recursive)
{
    for(size_t i = 0; i < NUM_LOOPS; i++) {
        Lock(recursive);
        if(in) {
            Unlock(recursive);
            return false;
        }
        in = true;
        for(size_t j = 0; j < WAIT_LOOPS; j++);
        in = false;
        Unlock(recursive);
    }
    return true;
}

static bool
RunMutexTest(bool recursive)
{
    TfThreadDispatcher d;
    function<bool ()> fnTask = bind(&Task, recursive);
    TfThread<bool>::Ptr t[TEST_COUNT];
    size_t i = 0;
    bool ok = true;

    mut = new TfMutex(recursive?TfMutex::RECURSIVE:TfMutex::NON_RECURSIVE);

    if(!mut->TryLock()) {
        delete mut;
        return false;
    }
    mut->Unlock();

    ArchSetThreadConcurrency(TEST_COUNT + 2);

    for(i = 0; i < TEST_COUNT; i++) {
        t[i] = d.Start(fnTask);
    }

    for(i = 0; i < TEST_COUNT; i++) {
        ok &= t[i]->GetResult();
    }

    delete mut;

    return ok;
}

static bool
Test_TfMutex()
{
    bool retVal = true;

    srand(time(NULL));

    retVal &= RunMutexTest(true);
    retVal &= RunMutexTest(false);

    return retVal;
}

TF_ADD_REGTEST(TfMutex);
