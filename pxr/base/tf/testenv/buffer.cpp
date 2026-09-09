//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/buffer.h"

#include <cstdio>
#include <numeric>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct Vec3 {
    float x, y, z;
};

static_assert(sizeof(Tf_Buffer<Vec3, 4>) == 2 * sizeof(void*),
              "Tf_Buffer should be two pointers wide");

}

static void
TestBasicPushAndAccess()
{
    Tf_Buffer<int, 8> buf;
    TF_AXIOM(buf.empty());
    TF_AXIOM(buf.size() == 0);
    TF_AXIOM(buf.capacity() == 8);

    buf.push_back(10);
    buf.push_back(20);
    buf.push_back(30);

    TF_AXIOM(buf.size() == 3);
    TF_AXIOM(!buf.empty());
    TF_AXIOM(buf[0] == 10);
    TF_AXIOM(buf[1] == 20);
    TF_AXIOM(buf[2] == 30);
    TF_AXIOM(buf.front() == 10);
    TF_AXIOM(buf.back() == 30);
}

static void
TestEmplaceBack()
{
    Tf_Buffer<Vec3, 4> buf;
    Vec3& v = buf.emplace_back(1.0f, 2.0f, 3.0f);

    TF_AXIOM(buf.size() == 1);
    TF_AXIOM(v.x == 1.0f && v.y == 2.0f && v.z == 3.0f);
    TF_AXIOM(&v == &buf[0]);
}

static void
TestPopBack()
{
    Tf_Buffer<int, 4> buf;
    buf.push_back(1);
    buf.push_back(2);
    buf.pop_back();

    TF_AXIOM(buf.size() == 1);
    TF_AXIOM(buf.back() == 1);
}

static void
TestIteration()
{
    Tf_Buffer<int, 8> buf;
    for (int i = 0; i < 5; ++i) {
        buf.push_back(i * 10);
    }

    int expected = 0;
    for (const int val : buf) {
        TF_AXIOM(val == expected);
        expected += 10;
    }
    TF_AXIOM(expected == 50);
}

static void
TestClearAndRefill()
{
    Tf_Buffer<int, 4> buf;
    buf.push_back(1);
    buf.push_back(2);

    const int* originalPtr = buf.data();
    buf.clear();
    TF_AXIOM(buf.empty());

    // clear() retains the allocation for reuse.
    TF_AXIOM(buf.data() == originalPtr);

    buf.push_back(99);
    TF_AXIOM(buf.size() == 1);
    TF_AXIOM(buf[0] == 99);
}

static void
TestMove()
{
    Tf_Buffer<int, 4> a;
    a.push_back(42);
    a.push_back(43);

    const int* originalPtr = &a[0];

    Tf_Buffer<int, 4> b = std::move(a);

    // Pointer swing - same heap block, no copy.
    TF_AXIOM(&b[0] == originalPtr);
    TF_AXIOM(b.size() == 2);
    TF_AXIOM(b[0] == 42 && b[1] == 43);

    // A moved-from buffer is empty and unallocated - not merely "unspecified".
    // size() must report 0 so that empty() and the iterator range stay honest.
    TF_AXIOM(a.size() == 0);
    TF_AXIOM(a.empty());
    TF_AXIOM(a.data() == nullptr);
    TF_AXIOM(a.begin() == a.end());

    // clear() does not resurrect a moved-from buffer - it stays unallocated.
    a.clear();
    TF_AXIOM(a.data() == nullptr);

    // allocate() is the explicit door back to a usable, empty state.
    a.allocate();
    TF_AXIOM(a.empty());
    TF_AXIOM(a.data() != nullptr);
    TF_AXIOM(a.data() != b.data());
}

static void
TestMoveAssign()
{
    Tf_Buffer<int, 4> a;
    a.push_back(1);
    a.push_back(2);
    a.push_back(3);

    Tf_Buffer<int, 4> b;
    b.push_back(99);

    b = std::move(a);
    TF_AXIOM(b.size() == 3);
    TF_AXIOM(b[0] == 1 && b[1] == 2 && b[2] == 3);
    TF_AXIOM(a.empty());
    TF_AXIOM(a.data() == nullptr);

    // Self move-assignment must not clobber size.
    Tf_Buffer<int, 4>& alias = b;
    b = std::move(alias);
    TF_AXIOM(b.size() == 3);
    TF_AXIOM(b[0] == 1 && b[1] == 2 && b[2] == 3);
}

static void
TestDeferAllocation()
{
    Tf_Buffer<int, 4> buf { tf_BufferDeferAllocation };

    // Identical to the moved-from state.
    TF_AXIOM(buf.empty());
    TF_AXIOM(buf.size() == 0);
    TF_AXIOM(buf.data() == nullptr);
    TF_AXIOM(buf.begin() == buf.end());

    // clear() is not the wake-up operation and must not allocate.
    buf.clear();
    TF_AXIOM(buf.data() == nullptr);
    TF_AXIOM(buf.empty());

    // allocate() brings the buffer into service.
    buf.allocate();
    TF_AXIOM(buf.data() != nullptr);
    TF_AXIOM(buf.empty());
    buf.push_back(7);
    TF_AXIOM(buf.size() == 1);
    TF_AXIOM(buf[0] == 7);

    // allocate() is idempotent - safe to call unconditionally, and it must not
    // discard existing contents.
    const int* allocated = buf.data();
    buf.allocate();
    TF_AXIOM(buf.data() == allocated);
    TF_AXIOM(buf.size() == 1);
    TF_AXIOM(buf[0] == 7);

    // Move-assignment is the other way to make a deferred buffer usable.
    Tf_Buffer<int, 4> other { tf_BufferDeferAllocation };
    other = std::move(buf);
    TF_AXIOM(other.size() == 1);
    TF_AXIOM(other[0] == 7);
}

static void
TestFull()
{
    Tf_Buffer<int, 4> buf;
    TF_AXIOM(!buf.full());

    for (int i = 0; i < 4; ++i) {
        buf.push_back(i);
    }
    TF_AXIOM(buf.full());
}

static bool
Test_Tf_Buffer(int argc, char *argv[])
{
    TestBasicPushAndAccess();
    TestEmplaceBack();
    TestPopBack();
    TestIteration();
    TestClearAndRefill();
    TestMove();
    TestMoveAssign();
    TestDeferAllocation();
    TestFull();
    
    return true;
}

TF_ADD_REGTEST(Tf_Buffer);
