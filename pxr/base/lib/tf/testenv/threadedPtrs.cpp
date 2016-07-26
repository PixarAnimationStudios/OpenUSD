/*
 * This test is obsolete.
 * TfRefPtr/TfWeakPtr isn't atomic, so the concepts tested below
 * will sometimes fail.
 */

#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/barrier.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/arch/threads.h"

#include <boost/bind.hpp>

using boost::bind;

struct Simple : public TfRefBase, public TfWeakBase {
public:
    static TfRefPtr<Simple> New() {
        return TfCreateRefPtr(new Simple);
    }

    static bool IsAlive() {
        return _alive;
    }
    
    ~Simple() {
        _alive = false;
    }

    Simple() {
        _alive = true;
    }

    static bool _alive;
};

bool Simple::_alive = false;

static TfRefPtr<Simple> simplePtr;
static TfWeakPtr<Simple> simpleBackPtr;

static TfBarrier barrier;
static int nScrewups = 0;

static void
RefTask(int n) {
    for (int i = 0; i < n; i++) {
        ::simplePtr = Simple::New();
        ::simpleBackPtr = simplePtr;
        ::barrier.Wait();

        ::simplePtr = NULL;
        ::barrier.Wait();
    }
}

void WeakTask(int n) {
    for (int i = 0; i < n; i++) {
        ::barrier.Wait();

        TfRefPtr<Simple> recover = ::simpleBackPtr;

        if (recover && !Simple::IsAlive())
            ::nScrewups++;

        recover = NULL;
        ::barrier.Wait();
    }
}

static bool
Test_TfThreadedPtrs()
{
    int n = 25000;
    TfThreadDispatcher d;
    ::barrier.SetSize(2);

    ArchSetThreadConcurrency(3);
    
    d.Start(bind(&RefTask, n));
    d.Start(bind(&WeakTask, n));

    if (!d.IsDone(10.0)) {
        fprintf(stderr, "test failed to complete within 10 seconds!\n");
        return false;
    }
    else if (::nScrewups > 0) {
        fprintf(stderr, "alive/dead mismatch %d out of %d\n", ::nScrewups, n);
        return false;
    }
    else
        return true;
}

TF_ADD_REGTEST(TfThreadedPtrs);
