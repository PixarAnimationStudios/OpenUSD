#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/condVar.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/tf/mutex.h"
#include "pxr/base/arch/threads.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using boost::bind;
using boost::function;

#define TEST_COUNT  1000
#define WAIT_COUNT  10000
#define WAIT        0.1

/* 
 * A var is created and its predicate is set to false.  Then a thread is
 * launched to wait on that var.  The main thread sets the predicate to
 * true and then broadcasts the var.  The launched thread checks that
 * the predicate is true once it passes the wait.
 * Both timed and untimed waits are tested.
 */

TfCondVar cv;
TfMutex m;
bool waitPred;

static bool
Wait(bool timed)
{
    if(timed) {
        cv.SetTimeLimit(600);
        return cv.TimedWait(m);
    } else {
        cv.Wait(m);
        return true;
    }
}

static bool
Task(bool timed)
{
    m.Lock();
    while(!waitPred) {
        if(!Wait(timed)) {
            m.Unlock();
            return false;
        }
        if(!waitPred) {
            m.Unlock();
            return false;
        }
    }
    m.Unlock();
    
    return true;
}

static bool
RunVarTest(bool timed)
{
    bool retVal = true;

    TfThreadDispatcher d;
    function<bool ()> fnTask = bind(&Task, timed);
    TfThread<bool>::Ptr t;

    waitPred = false;

    ArchSetThreadConcurrency(3);

    t = d.Start(fnTask);

    for(size_t j = 0; j < WAIT_COUNT; j++);    

    m.Lock();
    waitPred = true;
    cv.Broadcast();
    m.Unlock();

    retVal &= t->GetResult();

    return retVal;
}

static bool
Test_TfCondVar()
{
    bool retVal = true;
    int i;

    m.Lock();

    if(cv.TimedWait(m)) {
        printf("TimedWait is true, expected false\n");
        return false;
    }

    cv.SetTimeLimit(WAIT);
    if(cv.TimedWait(m)) {
        printf("TimedWait is true, expected false\n");
        return false;
    }
    m.Unlock();

    for(i = 0; i < TEST_COUNT; i++) {
        retVal &= RunVarTest(false);
    }
    if(!retVal) printf("Error during untimed wait\n");
    
    for(i = 0; i < TEST_COUNT; i++) {
        retVal &= RunVarTest(false);
    }
    if(!retVal) printf("Error during timed wait\n");

    return retVal;
}

TF_ADD_REGTEST(TfCondVar);

