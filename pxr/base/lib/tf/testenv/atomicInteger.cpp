#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/tf/atomicInteger.h"
#include "pxr/base/arch/threads.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using boost::function;
using boost::bind;

TfAtomicInteger ai;

static int
Task(int n, int amt) {
    int ctr = 0;
    for (int i = 0; i < n; i++) {
        if ((::ai.FetchAndAdd(amt) % 10) == 0)
            ctr++;
    }
    return ctr;
}

static bool
Test_TfAtomicInteger()
{
    int n = 1000000;
    TfThreadDispatcher d;
    
    function<int ()> fnUp = bind(&Task, n, 1);
    function<int ()> fnDown = bind(&Task, n, -1);
    TfThread<int>::Ptr t1, t2;
    
    ArchSetThreadConcurrency(2);
    
    t1 = d.Start(fnUp);
    t2 = d.Start(fnUp);

    /*
     * We're testing two things: that the total no. of times ai was
     * incremented was 2n, and that the combined number of times each
     * thread saw the counter divisable by 10 is 2n/10.  This tests the
     * atomicity of the counter AND the fact that each thread has an
     * atomicly consistent view of the counter prior to increment.
     * For example, the following implementation would yield an atomic
     * counter, but not satisfy the second property:
     *   int AtomicIncrement() {
     *      lock;
     *      modify counter;
     *      unlock
     *      return counter-1;
     *   }
     */

    int expected = (2 * n) / 10;
    int result = t1->GetResult() + t2->GetResult();
    bool ok = true;
    
    if (ai != 2 * n) {
        printf("expected %d, got %d\n", 2*n, ai.Get());
        ok = false;
    }
    
    if (result != expected) {
        printf("expected %d, got %d\n", expected, result);
        ok = false;
    }
    else {
        printf("expected result %d, got it [%d and %d]\n", expected,
               t1->GetResult(), t2->GetResult());
    }

    t1 = d.Start(fnDown);
    t2 = d.Start(fnDown);

    result = t1->GetResult() + t2->GetResult();
    
    if (ai != 0) {
        printf("expected %d, got %d\n", 0, ai.Get());
        ok = false;
    }

    if (result != expected) {
        printf("down: expected %d, got %d\n", expected, result);
        ok = false;
    }
    else {
        printf("down: expected result %d, got it [%d and %d]\n", expected,
               t1->GetResult(), t2->GetResult());
    }

    return ok;
}

TF_ADD_REGTEST(TfAtomicInteger);

