#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/tf/diagnostic.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using boost::bind;
using boost::function;

static size_t
RecursiveRequestRelease(size_t n)
{
    if (n == 0)
        return 0;

    size_t nExtra = TfThreadDispatcher::RequestExtraPhysicalThreads(n);
    size_t total = nExtra + RecursiveRequestRelease(n-1);
    TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);
    return total;
}

int TDInt(int) {return 0;}

static bool
Test_TfThreadDispatcher()
{
    TF_AXIOM(TfThreadDispatcher::GetPhysicalThreadLimit() == 1);

    size_t nExtra = TfThreadDispatcher::RequestExtraPhysicalThreads(1);
    TF_AXIOM(nExtra == 0);
    TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);

    nExtra = TfThreadDispatcher::RequestExtraPhysicalThreads(5);
    TF_AXIOM(nExtra == 0);
    TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);

    TfThreadDispatcher::SetPhysicalThreadLimit(2);
    
    nExtra = TfThreadDispatcher::RequestExtraPhysicalThreads(1);
    TF_AXIOM(nExtra == 1);
    TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);

    nExtra = TfThreadDispatcher::RequestExtraPhysicalThreads(5);
    TF_AXIOM(nExtra == 1);

    size_t nExtra2 = TfThreadDispatcher::RequestExtraPhysicalThreads(5);
    TF_AXIOM(nExtra2 == 0);

    TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra2);
    TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);


    // Test case where RequestExtraPhysicalThreads gets called before
    // a call to SetPhysicalThreadLimit.
    TfThreadDispatcher::SetPhysicalThreadLimit(10);
    nExtra = TfThreadDispatcher::RequestExtraPhysicalThreads(5);
    TF_AXIOM(nExtra == 5);
    // Now set the physical thread limit to something smaller than the
    // original.
    TfThreadDispatcher::SetPhysicalThreadLimit(2);
    // In the bug, this produces a coding error.
    TfThreadDispatcher::ReleaseExtraPhysicalThreads(nExtra);



    TfThreadDispatcher::SetPhysicalThreadLimit(10);
    TF_AXIOM(TfThreadDispatcher::GetPhysicalThreadLimit() == 10);

    TF_AXIOM(RecursiveRequestRelease(5) == 9);
    TF_AXIOM(TfThreadDispatcher::RequestExtraPhysicalThreads(9) == 9);

    TF_AXIOM(TfThreadDispatcher::GetTotalPendingThreads() == 0);

    function<int ()> bound = bind(&TDInt, 0);
    TF_AXIOM(TfThreadDispatcher::ParallelRequestAndWait(bound) == 1);

    TfThreadDispatcher t;
    t.FlushPendingPoolThreads();
    TF_AXIOM(t.GetNumPendingThreads() == 0);
    return true;
}

TF_ADD_REGTEST(TfThreadDispatcher);
