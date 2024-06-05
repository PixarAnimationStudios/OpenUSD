//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/error.h"

#include <cstdio>
#include <cstdlib>

PXR_NAMESPACE_USING_DIRECTIVE

enum Operation {
    Ctor20Op,
    Ctor30Op,
    CtorTestOp,
    CtorTest2Op,
    MainOp,
    MainAtExitOp,
    DtorTest2Op,
    DtorTestOp,
    Ctor30AtExitOp,
    Ctor20AtExitOp,
    Dtor30Op,
    Dtor20Op,
    NumOperations
};

#define BIT(x) (1 << (x))
typedef unsigned int Bits;
static Bits done = 0;

// Required order of operations.  Some things must happen before others
// and this defines that order.  We take advantage of implied dependencies
// so if A precedes B and B precedes A we don't necessarily say that A
// precedes C.  Note that platforms have some flexibility in the order.
static const Bits dependencies[NumOperations] = {
    /* Ctor20Op       */   0,
    /* Ctor30Op       */   BIT(Ctor20Op),
    /* CtorTestOp      */   0,
    /* CtorTest2Op     */   BIT(CtorTestOp),
    /* MainOp          */   0,
    /* MainAtExitOp    */   BIT(MainOp) | BIT(Ctor20Op) | BIT(CtorTest2Op),
    /* DtorTest2Op     */   BIT(MainAtExitOp),
    /* DtorTestOp      */   BIT(DtorTest2Op),
    /* Ctor30AtExitOp */   BIT(MainAtExitOp),
    /* Ctor20AtExitOp */   BIT(Ctor30AtExitOp),
    /* Dtor30Op       */   BIT(MainAtExitOp),
    /* Dtor20Op       */   BIT(Dtor30Op)
};

static void TestAndSet(Operation operation)
{
    static const char binary[][5] = {
        "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111",
        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"
    };

    // Check dependencies.
    const Bits deps = dependencies[operation];
    if ((done & deps) != deps) {
        fprintf(stderr, "Failed on operation %d: %s%s%s%s expected %s%s%s%s\n",
                operation,
                binary[(done >> 12) & 15],
                binary[(done >>  8) & 15],
                binary[(done >>  4) & 15],
                binary[(done      ) & 15],
                binary[(deps >> 12) & 15],
                binary[(deps >>  8) & 15],
                binary[(deps >>  4) & 15],
                binary[(deps      ) & 15]);
    }
    ARCH_AXIOM((done & deps) == deps);
    done |= BIT(operation);
}

static void Ctor20AtExit()
{
    TestAndSet(Ctor20AtExitOp);
}

static void Ctor30AtExit()
{
    TestAndSet(Ctor30AtExitOp);
}

static void MainAtExit()
{
    TestAndSet(MainAtExitOp);
}

ARCH_CONSTRUCTOR(Ctor20, 20, void)
{
    TestAndSet(Ctor20Op);
    atexit(Ctor20AtExit);
}

ARCH_CONSTRUCTOR(Ctor30, 30, void)
{
    TestAndSet(Ctor30Op);
    atexit(Ctor30AtExit);
}

ARCH_DESTRUCTOR(Dtor20, 20, void)
{
    TestAndSet(Dtor20Op);
}

ARCH_DESTRUCTOR(Dtor30, 30, void)
{
    TestAndSet(Dtor30Op);
}

struct Test {
    Test() : _dtor(DtorTestOp)
    {
        TestAndSet(CtorTestOp);
    }

    Test(Operation ctor, Operation dtor) : _dtor(dtor)
    {
        TestAndSet(ctor);
    }

    ~Test()
    {
        TestAndSet(_dtor);
    }

    void Foo() { }

    Operation _dtor;
};

Test test;
Test test2(CtorTest2Op, DtorTest2Op);

int main()
{
    // Make sure the global objects are created.
    test.Foo();
    test2.Foo();

    atexit(MainAtExit);

    TestAndSet(MainOp);

    return 0;
}
