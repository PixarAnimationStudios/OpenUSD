#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/threadDispatcher.h"
#include "pxr/base/tf/threadData.h"
#include "pxr/base/arch/threads.h"
#include "pxr/base/arch/atomicOperations.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

using boost::bind;
using boost::function;

struct Counted {
    Counted() {
        ArchAtomicIntegerIncrement(&_nCreated);
        ArchAtomicIntegerIncrement(&_n);
    }

    Counted(const Counted&) {
        ArchAtomicIntegerIncrement(&_nCreated);
        ArchAtomicIntegerIncrement(&_n);
    }

    ~Counted() {
        ArchAtomicIntegerDecrement(&_n);
    }

    void Noop() { }

    static int _n;
    static int _nCreated;

    static int GetCreatedCount() {
        return _nCreated;
    }

    static int GetTotalCount() {
        return _n;
    }
};

int Counted::_n = 0;
int Counted::_nCreated = 0;

TfThreadData<Counted> c(TfThreadInfo::LONG_TERM);
static void CountTask() {
    c->Noop();
}

TfThreadData<int> d(TfThreadInfo::LONG_TERM);
static int
Task3()
{
    return *d;
}

static int
Task2(int n)
{
    *d += n;
    return Task3();
}

static int
Task1(int n)
{
    *d = 0;
    return Task2(n);
}

static bool

Test_TfThreadData()
{
    TfThreadDispatcher d(1);

    ArchSetThreadConcurrency(7);

    TfThread<int>::Ptr ti1 = d.Start(bind(&Task1, 1));
    TfThread<int>::Ptr ti2 = d.Start(bind(&Task1, 2));
    TfThread<int>::Ptr ti3 = d.Start(bind(&Task1, 3));

    TF_AXIOM(ti1->GetResult() == 1);
    TF_AXIOM(ti2->GetResult() == 2);
    TF_AXIOM(ti3->GetResult() == 3);

    d.Start(bind(&CountTask));
    d.Start(bind(&CountTask));
    d.Start(bind(&CountTask));
    d.Wait();

    // global variable c itself has an instance of Counted
    TF_AXIOM(Counted::GetTotalCount() == 1);
    TF_AXIOM(Counted::GetCreatedCount() == 4);

    d.SetPoolMode(true);

    TfThread<int>::Ptr ti4 = d.Start(bind(&Task1, 4));
    TfThread<int>::Ptr ti5 = d.Start(bind(&Task2, 5));
    TfThread<int>::Ptr ti6 = d.Start(bind(&Task2, 6));

    d.Start(bind(&CountTask));
    d.Start(bind(&CountTask));
    d.Start(bind(&CountTask));
    d.Wait();

    TF_AXIOM(Counted::GetTotalCount() == 2);
    TF_AXIOM(Counted::GetCreatedCount() == 5);

    TF_AXIOM(ti4->GetResult() == 4);
    TF_AXIOM(ti5->GetResult() == 4+5);
    TF_AXIOM(ti6->GetResult() == 4+5+6);
    return true;
}

TF_ADD_REGTEST(TfThreadData);
