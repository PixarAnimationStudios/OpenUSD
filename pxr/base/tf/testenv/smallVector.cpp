//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/regTest.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <numeric>
#include <numeric>
#include <string.h>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static void
testConstructors()
{
    // Default constructor
    TfSmallVector<int, 1> v1;
    TF_AXIOM(v1.size() == 0);
    TF_AXIOM(v1.capacity() == 1);
    TF_AXIOM(v1.empty());

    // Resizing-constructor
    TfSmallVector<int, 1> v2(1, 13);
    TF_AXIOM(v2.size() == 1);
    TF_AXIOM(v2.capacity() == 1);
    TF_AXIOM(v2[0] == 13);
    TF_AXIOM(v2.front() == 13);
    TF_AXIOM(v2.back() == 13);
    TF_AXIOM(!v2.empty());

    // Resizing-constructor with more local storage
    TfSmallVector<int, 2> v3(2, 14);
    TF_AXIOM(v3.size() == 2);
    TF_AXIOM(v3.capacity() == 2);
    TF_AXIOM(v3[0] == 14);
    TF_AXIOM(v3[1] == 14);
    TF_AXIOM(v3.front() == 14);
    TF_AXIOM(v3.back() == 14);

    // Resizing-constructor with more local storage, but only resizing to 1
    TfSmallVector<int, 2> v4(1, 15);
    TF_AXIOM(v4.size() == 1);
    TF_AXIOM(v4.capacity() == 2);
    TF_AXIOM(v4[0] == 15);
    TF_AXIOM(v4.front() == 15);
    TF_AXIOM(v4.back() == 15);

    // Copy constructor
    TfSmallVector<int, 2> v5(v3);
    TF_AXIOM(v5.size() == 2);
    TF_AXIOM(v5.capacity() == 2);
    TF_AXIOM(v5[0] == 14);
    TF_AXIOM(v5[1] == 14);
    TF_AXIOM(v5.front() == 14);
    TF_AXIOM(v5.back() == 14);

    // Resizing-constructor, resizing to remote storage
    TfSmallVector<int, 1> v6(10, 15);
    TF_AXIOM(v6.size() == 10);
    TF_AXIOM(v6.capacity() == 10);
    for (int i : v6) {
        TF_AXIOM(i == 15);
    }
    TF_AXIOM(v6.front() == 15);
    TF_AXIOM(v6.back() == 15);

    // Move-constructor by moving local storage
    TfSmallVector<int, 2> v7(std::move(v5));
    TF_AXIOM(v7.size() == 2);
    TF_AXIOM(v7.capacity() == 2);
    TF_AXIOM(v7[0] == 14);
    TF_AXIOM(v7[1] == 14);
    TF_AXIOM(v7.front() == 14);
    TF_AXIOM(v7.back() == 14);
    TF_AXIOM(v5.size() == 0);
    TF_AXIOM(v5.capacity() == 2);

    // Move-constructor by moving remote storage
    TfSmallVector<int, 1> v8(std::move(v6));
    TF_AXIOM(v8.size() == 10);
    TF_AXIOM(v8.capacity() == 10);
    for (int i : v8) {
        TF_AXIOM(i == 15);
    }
    TF_AXIOM(v8.front() == 15);
    TF_AXIOM(v8.back() == 15);
    TF_AXIOM(v6.size() == 0);
    TF_AXIOM(v6.capacity() == 1);

    // Assignment operator with local storage.
    TfSmallVector<int, 2> v9;
    v9 = v7;
    TF_AXIOM(v9.size() == 2);
    TF_AXIOM(v9.capacity() == 2);
    TF_AXIOM(v9[0] == 14);
    TF_AXIOM(v9[1] == 14);
    TF_AXIOM(v9.front() == 14);
    TF_AXIOM(v9.back() == 14);

    // Assignment operator with remote storage.
    TfSmallVector<int, 1> v10;
    v10 = v8;
    TF_AXIOM(v10.size() == 10);
    TF_AXIOM(v10.capacity() == 10);
    for (int i : v10) {
        TF_AXIOM(i == 15);
    }
    TF_AXIOM(v10.front() == 15);
    TF_AXIOM(v10.back() == 15);

    // Move assignment with local storage.
    v9 = std::move(v7);
    TF_AXIOM(v9.size() == 2);
    TF_AXIOM(v9.capacity() == 2);
    TF_AXIOM(v9[0] == 14);
    TF_AXIOM(v9[1] == 14);
    TF_AXIOM(v9.front() == 14);
    TF_AXIOM(v9.back() == 14);
    TF_AXIOM(v7.size() == 2);
    TF_AXIOM(v7.capacity() == 2);

    // Move assignment with remote storage.
    v10 = std::move(v8);
    TF_AXIOM(v10.size() == 10);
    TF_AXIOM(v10.capacity() == 10);
    for (int i : v10) {
        TF_AXIOM(i == 15);
    }
    TF_AXIOM(v10.front() == 15);
    TF_AXIOM(v10.back() == 15);
    TF_AXIOM(v8.size() == 10);
    TF_AXIOM(v8.capacity() == 10);

    {
        constexpr size_t size = 100;
        std::vector<int> source(100);
        for (size_t i = 0; i < size; ++i) {
            source[i] = rand();
        }

        {
            TfSmallVector<int, 1> vv(source.begin(), source.end());
            TF_AXIOM(source.size() == vv.size());
            TF_AXIOM(vv.capacity() == size);
            for (size_t i = 0; i < vv.size(); ++i) {
                TF_AXIOM(source[i] == vv[i]);
            }
        }

        {
            TfSmallVector<int, 10> vv(source.begin(), source.begin()+10);
            TF_AXIOM(vv.size() == 10);
            TF_AXIOM(vv.capacity() == 10);
            for (size_t i = 0; i < vv.size(); ++i) {
                TF_AXIOM(source[i] == vv[i]);
            }
        }

        {
            TfSmallVector<int, 15> vv(source.begin(), source.begin()+10);
            TF_AXIOM(vv.size() == 10);
            TF_AXIOM(vv.capacity() == 15);
            for (size_t i = 0; i < vv.size(); ++i) {
                TF_AXIOM(source[i] == vv[i]);
            }
        }
    }

    // Initializer List Construction
    {
        TfSmallVector<int, 5> il0 = {};
        TF_AXIOM(il0.size() == 0);
        TF_AXIOM(il0.capacity() == 5);

        TfSmallVector<int, 5> il1 = {1, 2, 3};
        TF_AXIOM(il1.size() == 3);
        TF_AXIOM(il1.capacity() == 5);
        TF_AXIOM(il1[0] = 1);
        TF_AXIOM(il1[1] = 2);
        TF_AXIOM(il1[2] = 3);

        TfSmallVector<int, 5> il2 = {6, 5, 4, 3, 2, 1};
        TF_AXIOM(il2.size() == 6);
        TF_AXIOM(il2.capacity() == 6);
        TF_AXIOM(il2[0] = 6);
        TF_AXIOM(il2[1] = 5);
        TF_AXIOM(il2[2] = 4);
        TF_AXIOM(il2[3] = 3);
        TF_AXIOM(il2[4] = 2);
        TF_AXIOM(il2[5] = 1);
    }

    // Initializer List Assignment using operator=
    {
        TfSmallVector<int, 5> il0;
        il0 = {};
        TF_AXIOM(il0.size() == 0);
        TF_AXIOM(il0.capacity() == 5);

        TfSmallVector<int, 5> il1;
        il1 = {1, 2, 3};
        TF_AXIOM(il1.size() == 3);
        TF_AXIOM(il1.capacity() == 5);
        TF_AXIOM(il1[0] = 1);
        TF_AXIOM(il1[1] = 2);
        TF_AXIOM(il1[2] = 3);

        TfSmallVector<int, 5> il2;
        il2 = {6, 5, 4, 3, 2, 1};
        TF_AXIOM(il2.size() == 6);
        TF_AXIOM(il2.capacity() == 6);
        TF_AXIOM(il2[0] = 6);
        TF_AXIOM(il2[1] = 5);
        TF_AXIOM(il2[2] = 4);
        TF_AXIOM(il2[3] = 3);
        TF_AXIOM(il2[4] = 2);
        TF_AXIOM(il2[5] = 1);
    }

    // Initializer List Assignment using assign()
    {
        TfSmallVector<int, 5> il0;
        il0.assign({});
        TF_AXIOM(il0.size() == 0);
        TF_AXIOM(il0.capacity() == 5);

        TfSmallVector<int, 5> il1;
        il1.assign({1, 2, 3});
        TF_AXIOM(il1.size() == 3);
        TF_AXIOM(il1.capacity() == 5);
        TF_AXIOM(il1[0] = 1);
        TF_AXIOM(il1[1] = 2);
        TF_AXIOM(il1[2] = 3);

        TfSmallVector<int, 5> il2;
        il2.assign({6, 5, 4, 3, 2, 1});
        TF_AXIOM(il2.size() == 6);
        TF_AXIOM(il2.capacity() == 6);
        TF_AXIOM(il2[0] = 6);
        TF_AXIOM(il2[1] = 5);
        TF_AXIOM(il2[2] = 4);
        TF_AXIOM(il2[3] = 3);
        TF_AXIOM(il2[4] = 2);
        TF_AXIOM(il2[5] = 1);
    }
}

static void
testNoLocalStorage()
{
    // Vector with no local storage.
    TfSmallVector<int, 0> v;
    TF_AXIOM(v.size() == 0);
    TF_AXIOM(v.capacity() == 0);

    // Push back one entry
    v.push_back(1337);
    TF_AXIOM(v.size() == 1);
    TF_AXIOM(v.capacity() == 1);
    TF_AXIOM(v.front() == 1337);
    TF_AXIOM(v.back() == 1337);
    TF_AXIOM(*v.data() == 1337);

    // Push back one entry
    v.push_back(1338);
    TF_AXIOM(v.size() == 2);
    TF_AXIOM(v.capacity() == 2);
    TF_AXIOM(v.front() == 1337);
    TF_AXIOM(v.back() == 1338);
    TF_AXIOM(*v.data() == 1337);

    // Push back one entry
    v.push_back(1339);
    TF_AXIOM(v.size() == 3);
    TF_AXIOM(v.capacity() == 4);
    TF_AXIOM(v.front() == 1337);
    TF_AXIOM(v.back() == 1339);
    TF_AXIOM(*v.data() == 1337);

    // Insert in the front
    v.insert(v.begin(), 1313);
    TF_AXIOM(v.size() == 4);
    TF_AXIOM(v.capacity() == 4);
    TF_AXIOM(v.front() == 1313);
    TF_AXIOM(v.back() == 1339);
    TF_AXIOM(*v.data() == 1313);

    // Erase from the middle
    v.erase(v.begin() + 1, v.begin() + 3);
    TF_AXIOM(v.size() == 2);
    TF_AXIOM(v.capacity() == 4);
    TF_AXIOM(v.front() == 1313);
    TF_AXIOM(v.back() == 1339);
    TF_AXIOM(*v.data() == 1313);

    // Pop back
    v.pop_back();
    TF_AXIOM(v.size() == 1);
    TF_AXIOM(v.capacity() == 4);
    TF_AXIOM(v.front() == 1313);
    TF_AXIOM(v.back() == 1313);
    TF_AXIOM(*v.data() == 1313);

    // Clear
    v.clear();
    TF_AXIOM(v.size() == 0);
    TF_AXIOM(v.capacity() == 4);
}

static void
testGrowth()
{
    TfSmallVector<int, 2> v;

    // Push back (local storage)
    v.push_back(1);
    TF_AXIOM(v.size() == 1);
    TF_AXIOM(v.capacity() == 2);
    TF_AXIOM(v[0] == 1);

    // Emplace back (local storage)
    v.emplace_back(2);
    TF_AXIOM(v.size() == 2);
    TF_AXIOM(v.capacity() == 2);
    TF_AXIOM(v[0] == 1);
    TF_AXIOM(v[1] == 2);

    // Push back (remote storage)
    v.push_back(3);
    TF_AXIOM(v.size() == 3);
    TF_AXIOM(v.capacity() == 4);
    TF_AXIOM(v[0] == 1);
    TF_AXIOM(v[1] == 2);
    TF_AXIOM(v[2] == 3);

    // Emplace back (remote storage)
    v.emplace_back(4);
    TF_AXIOM(v.size() == 4);
    TF_AXIOM(v.capacity() == 4);
    TF_AXIOM(v[0] == 1);
    TF_AXIOM(v[1] == 2);
    TF_AXIOM(v[2] == 3);
    TF_AXIOM(v[3] == 4);

    // Clear
    v.clear();
    TF_AXIOM(v.size() == 0);
    TF_AXIOM(v.capacity() == 4);

    // Push back (still remote storage)
    v.push_back(5);
    TF_AXIOM(v.size() == 1);
    TF_AXIOM(v.capacity() == 4);
    TF_AXIOM(v[0] == 5);

    // Reserve some storage in an empty vector.
    TfSmallVector<int, 2> vr;
    TF_AXIOM(vr.size() == 0);
    TF_AXIOM(vr.capacity() == 2);

    vr.reserve(100);
    TF_AXIOM(vr.size() == 0);
    TF_AXIOM(vr.capacity() == 100);
}

static void
testIteration()
{
    const std::vector<int> cv { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };

    // Assignment
    TfSmallVector<int, 1> v1(3, 1313);
    v1.assign(cv.begin(), cv.end());
    TF_AXIOM(v1.size() == cv.size());

    // Indexing operator
    for (size_t i = 0; i < cv.size(); ++i) {
        TF_AXIOM(cv[i] == v1[i]);
    }

    // Range based for loop
    size_t j = 0;
    for (int i : v1) {
        TF_AXIOM(cv[j] == i);
        ++j;
    }

    // Forward iteration
    j = 0;
    TfSmallVector<int, 1>::iterator it = v1.begin();
    for (; it != v1.end(); ++it) {
        TF_AXIOM(cv[j] == *it);
        ++j;
    }

    // Forward iteration with const iterator
    j = 0;
    TfSmallVector<int, 1>::const_iterator cit = v1.cbegin();
    for (; cit != v1.cend(); ++cit) {
        TF_AXIOM(cv[j] == *cit);
        ++j;
    }

    // Reverse iteration 
    j = 9;
    TfSmallVector<int, 1>::reverse_iterator rit = v1.rbegin();
    for (; rit != v1.rend(); ++rit) {
        TF_AXIOM(cv[j] == *rit);
        --j;
    }

    // Reverse iteration with const iterator
    j = 9;
    TfSmallVector<int, 1>::const_reverse_iterator crit = v1.crbegin();
    for (; crit != v1.crend(); ++crit) {
        TF_AXIOM(cv[j] == *crit);
        --j;
    }

    // Equality comparison
    TfSmallVector<int, 1> v2(v1);
    TF_AXIOM(v1 == v2);

    TfSmallVector<int, 1> v3;
    TF_AXIOM(v2 != v3);
}

//
// This is testing copying stuff into a TfSmallVector.
// 
template<typename T, size_t _N>
bool
TestCopyIntoVector(const T (&data)[_N])
{
    const T *arrayData = reinterpret_cast<const T*>(data);

    const int numObjects = _N;

    // Test the inline storage case.
    {
        TfSmallVector<T, 10> v;

        v.resize(numObjects);
        memcpy(&v[0], arrayData, sizeof(T) * numObjects);

        for (int i = 0; i < numObjects; ++i) {
            TF_AXIOM(v[i] == data[i]);
        }
    }

    {
        TfSmallVector<T, 1> v;
        v.resize(numObjects);
        memcpy(&v[0], arrayData, sizeof(T) * numObjects);

        for (int i = 0; i < numObjects; ++i) {
            TF_AXIOM(v[i] == data[i]);
        }
    }

    return true;
}

// These correspond to the types in ExtUtil Numpysup.
static void
testCopyIntoVector()
{
    {
        // Testing vec2i
        std::array<int, 2> data[] = {
            {0, 0},
            {1, 0},
            {0, 1},
        };

        TestCopyIntoVector(data);
    }

    {
        // Testing vec3i
        std::array<int, 3> data[] = {
            {0, 0, 0},
            {1, 0, 0},
            {0, 1, 0},
            {0, 0, 1},
        };

        TestCopyIntoVector(data);
    }

    {
        // Testing vec4i
        std::array<int, 4> data[] = {
            {0, 0, 0, 0},
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
        };

        TestCopyIntoVector(data);
    }

    {
        // Testing vec2d
        std::array<double, 2> data[] = {
            {0.0, 0.0},
            {1.0, 0.0},
            {0.0, 1.0},
        };

        TestCopyIntoVector(data);
    }

    {
        // Testing vec3d
        std::array<double, 3> data[] = {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
        };

        TestCopyIntoVector(data);
    }

    {
        // Testing vec4d
        std::array<double, 4> data[] = {
            {0.0, 0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0, 0.0},
            {0.0, 1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0, 0.0},
        };

        TestCopyIntoVector(data);
    }

    {
        // Test mat9d
        std::array<double, 9> data[] = {
            {1.0,0.0,0.0,
             0.0,1.0,0.0,
             0.0,0.0,1.0},
            {1.0,0.0,0.0,
             0.0,0.0,0.0,
             0.0,0.0,0.0},
            {0.0,0.0,0.0,
             0.0,1.0,0.0,
             0.0,0.0,0.0},
            {0.0,0.0,0.0,
             0.0,0.0,0.0,
             0.0,0.0,1.0},
        };

        TestCopyIntoVector(data);
    }

    {
        // Test mat16d
        std::array<double, 16> data[] = {
            {1.0,0.0,0.0,0.0,
             0.0,1.0,0.0,0.0,
             0.0,0.0,1.0,0.0,
             0.0,0.0,0.0,1.0},
            {1.0,0.0,0.0,0.0,
             0.0,0.0,0.0,0.0,
             0.0,0.0,0.0,0.0,
             0.0,0.0,0.0,1.0},
            {0.0,0.0,0.0,0.0,
             0.0,1.0,0.0,0.0,
             0.0,0.0,0.0,0.0,
             0.0,0.0,0.0,1.0},
            {0.0,0.0,0.0,0.0,
             0.0,0.0,0.0,0.0,
             0.0,0.0,1.0,0.0,
             0.0,0.0,0.0,1.0}
        };

        TestCopyIntoVector(data);
    }

    {
        // Test double
        double data[] =
        {
            0.0,
            1.0,
            0.5,
            0.75,
        };

        TestCopyIntoVector(data);
    }

    {
        // Test float
        float data[] =
        {
            0.0f,
            1.0f,
            0.5f,
            0.75f,
        };

        TestCopyIntoVector(data);
    }

    {
        // Test int
        int data[] =
        {
            0,
            1,
            5,
            75,
        };

        TestCopyIntoVector(data);
    }

    {
        // Test size_t
        size_t data[] =
        {
            0,
            1,
            5,
            75,
        };

        TestCopyIntoVector(data);
    }
}

static void
testInsertNoMoveConstructor()
{
    struct _Foo {

        // No default constructor.
        _Foo(int i) : i(i) {}

        // Basic copy and assignment.
        // Note this causes the compiler to NOT generate a
        // move constructor and move assignment operator.
        _Foo(const _Foo &rhs) : i(rhs.i) {}
        _Foo &operator=(const _Foo &rhs) {
            i = rhs.i;
            return *this;
        }

        // 32-bit size.
        int i;

    };

    // Create an instance of _Foo.
    _Foo f(1);

    // Grow via push_back / emplace_back.
    TfSmallVector<_Foo, 1> u;
    u.push_back(f);
    u.push_back(f);

    std::vector<_Foo> su;
    su.push_back(f);
    su.push_back(f);

    // Grow via insertion.
    TfSmallVector<_Foo, 1> v;
    v.insert(v.begin(), f);
    v.insert(v.begin(), f);
    v.insert(v.begin(), f);

    std::vector<_Foo> sv;
    sv.insert(sv.begin(), f);
    sv.insert(sv.begin(), f);
    sv.insert(sv.begin(), f);

    // Attempt to move between local storage by swapping.
    TfSmallVector<_Foo, 1> x;
    x.push_back(f);
    TfSmallVector<_Foo, 1> y;
    y.swap(x);

    std::vector<_Foo> sx;
    sx.push_back(f);
    std::vector<_Foo> sy;
    sy.swap(sx);

    // Grow via reserve.
    TfSmallVector<_Foo, 1> z;
    z.push_back(f);
    z.reserve(100);

    std::vector<_Foo> sz;
    sz.push_back(f);
    sz.reserve(100);
}

/////////////////////////////////////////////////////////////////////////////// 

static void
testInsertionTrivial()
{
    // Bulk insertion tests.
    // 
    // Test inserting to the back of an empty vector.

    std::vector<int> sourceA(10);
    std::iota(sourceA.begin(), sourceA.end(), 0);

    std::vector<int> sourceB;
    sourceB.push_back(999);
    sourceB.push_back(998);
    sourceB.push_back(997);
    sourceB.push_back(996);

    // Local storage with enough space to absorb new entries
    {
        TfSmallVector<int, 15> a;
        a.insert(a.end(), sourceA.begin(), sourceA.end());

        for (size_t i = 0; i < a.size(); ++i) {
            TF_AXIOM(a[i] == sourceA[i]);
        }
    }

    // Remote storage with enough space to absorb new entries.
    {
        TfSmallVector<int, 1> a;
        a.reserve(15);
        a.insert(a.end(), sourceA.begin(), sourceA.end());

        for (size_t i = 0; i < a.size(); ++i) {
            TF_AXIOM(a[i] == sourceA[i]);
        }
    }

    // Local Growth case.
    {
        TfSmallVector<int, 1> a;
        a.insert(a.end(), sourceA.begin(), sourceA.end());

        for (size_t i = 0; i < a.size(); ++i) {
            TF_AXIOM(a[i] == sourceA[i]);
        }
    }

    // Remote Growth case.
    {
        TfSmallVector<int, 1> a;
        a.reserve(5);
        a.insert(a.end(), sourceA.begin(), sourceA.end());

        for (size_t i = 0; i < a.size(); ++i) {
            TF_AXIOM(a[i] == sourceA[i]);
        }
    }

    // Test inserting at the front.
    // 
    // Local storage with enough space to absorb new entries
    {
        TfSmallVector<int, 15> a(sourceA.begin(), sourceA.end());

        // Splice in B.
        a.insert(a.begin(), sourceB.begin(), sourceB.end());

        TF_AXIOM(a[0]  == 999);
        TF_AXIOM(a[1]  == 998);
        TF_AXIOM(a[2]  == 997);
        TF_AXIOM(a[3]  == 996);
        TF_AXIOM(a[4]  == 0);
        TF_AXIOM(a[5]  == 1);
        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);
    }

    // Remote storage with enough space to absorb new entries.
    {
        TfSmallVector<int, 1> a(sourceA.begin(), sourceA.end());

        // Splice in B.
        a.insert(a.begin(), sourceB.begin(), sourceB.end());

        TF_AXIOM(a[0]  == 999);
        TF_AXIOM(a[1]  == 998);
        TF_AXIOM(a[2]  == 997);
        TF_AXIOM(a[3]  == 996);
        TF_AXIOM(a[4]  == 0);
        TF_AXIOM(a[5]  == 1);
        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);
    }

    // Local Growth case.
    {
        TfSmallVector<int, 11> a(sourceA.begin(), sourceA.end());
        a.insert(a.begin(), sourceB.begin(), sourceB.end());

        TF_AXIOM(a[0]  == 999);
        TF_AXIOM(a[1]  == 998);
        TF_AXIOM(a[2]  == 997);
        TF_AXIOM(a[3]  == 996);
        TF_AXIOM(a[4]  == 0);
        TF_AXIOM(a[5]  == 1);
        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);
    }

    // Remote Growth case.
    {
        TfSmallVector<int, 1> a(sourceA.begin(), sourceA.end());
        a.insert(a.begin(), sourceB.begin(), sourceB.end());

        TF_AXIOM(a[0]  == 999);
        TF_AXIOM(a[1]  == 998);
        TF_AXIOM(a[2]  == 997);
        TF_AXIOM(a[3]  == 996);
        TF_AXIOM(a[4]  == 0);
        TF_AXIOM(a[5]  == 1);
        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);
    }

    // Middle insertion case.

    // Local storage with space to absorb new entries.
    {
        TfSmallVector<int, 15> a(sourceA.begin(), sourceA.end());
        a.insert(a.begin()+2, sourceB.begin(), sourceB.end());

        TF_AXIOM(a[0] == 0);
        TF_AXIOM(a[1] == 1);

        TF_AXIOM(a[2] == 999);
        TF_AXIOM(a[3] == 998);
        TF_AXIOM(a[4] == 997);
        TF_AXIOM(a[5] == 996);

        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);
    }

    // Remote storage with space to absorb new entries.
    {
        TfSmallVector<int, 1> a(sourceA.begin(), sourceA.end());
        a.reserve(15);
        a.insert(a.begin()+2, sourceB.begin(), sourceB.end());

        TF_AXIOM(a[0] == 0);
        TF_AXIOM(a[1] == 1);

        TF_AXIOM(a[2] == 999);
        TF_AXIOM(a[3] == 998);
        TF_AXIOM(a[4] == 997);
        TF_AXIOM(a[5] == 996);

        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);
    }

    // Local storage growth case.
    {
        TfSmallVector<int, 11> a(sourceA.begin(), sourceA.end());
        a.insert(a.begin()+2, sourceB.begin(), sourceB.end());

        TF_AXIOM(a[0] == 0);
        TF_AXIOM(a[1] == 1);

        TF_AXIOM(a[2] == 999);
        TF_AXIOM(a[3] == 998);
        TF_AXIOM(a[4] == 997);
        TF_AXIOM(a[5] == 996);

        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);
    }

    // Local storage growth case.
    {
        TfSmallVector<int, 1> a(sourceA.begin(), sourceA.end());
        a.insert(a.begin()+2, sourceB.begin(), sourceB.end());

        TF_AXIOM(a[0] == 0);
        TF_AXIOM(a[1] == 1);

        TF_AXIOM(a[2] == 999);
        TF_AXIOM(a[3] == 998);
        TF_AXIOM(a[4] == 997);
        TF_AXIOM(a[5] == 996);

        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);
    }

    // Test many repeated insertions.
    {
        TfSmallVector<int, 1> a;
        const int src[] = { 1, };
        const int NumInsertions = 2048;
        const size_t numInsertedElems = TfArraySize(src);
        for (int i=0; i<NumInsertions; ++i) {
            a.insert(a.end(), std::begin(src), std::end(src));
            // This is a loose bound on the growth during insertion just to
            // make sure that we don't have runaway allocation
            // (as in PRES-70771.)
            if (a.capacity() > 4*numInsertedElems*(i+1)) {
                TF_FATAL_ERROR(
                    "Capacity too large; after %d insertions of %zu elements, "
                    "vector has size %u and capacity %u",
                    i+1, numInsertedElems, a.size(), a.capacity());
            }
        }
        TF_AXIOM(a.size() == NumInsertions);
    }
}

/////////////////////////////////////////////////////////////////////////////// 
/// 
/// Small structs for testing insertion.
/// 
struct TestStruct
{
    TestStruct() : _value(0)
    {
        counter++;
    }

    TestStruct(int val) : _value(val) 
    {
        counter++;
    }

    TestStruct(const TestStruct &other) : _value(other._value) 
    {
        counter++;
    }

    ~TestStruct() {
        --counter;
    }

    bool operator==(const TestStruct &other)
    {
        return other._value == this->_value;
    }

    int _value;
    static int counter;
};

int TestStruct::counter = 0;

///////////////////////////////////////////////////////////////////////////////

static void
testInsertion()
{
    // Bulk insertion tests.
    // 
    // Test inserting to the back of an empty vector.

    const std::vector<TestStruct> sourceA = {
        TestStruct(0),
        TestStruct(1),
        TestStruct(2),
        TestStruct(3),
        TestStruct(4),
        TestStruct(5),
        TestStruct(6),
        TestStruct(7),
        TestStruct(8),
        TestStruct(9),
    };

    TF_AXIOM(TestStruct::counter == 10);

    const std::initializer_list<TestStruct> ilistB = {
        TestStruct(999),
        TestStruct(998),
        TestStruct(997),
        TestStruct(996),
    };

    TF_AXIOM(TestStruct::counter == 14);

    const std::vector<TestStruct> sourceB(ilistB);

    TF_AXIOM(TestStruct::counter == 18);

    // Local storage with enough space to absorb new entries
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 15> a(sourceA.begin(), sourceA.end());

        if (!useIlist) {
            a.insert(a.end(), sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.end(), ilistB);
        }

        for (size_t i = 0; i < sourceA.size(); ++i) {
            TF_AXIOM(a[i] == sourceA[i]);
        }

        TF_AXIOM(a[10] == sourceB[0]);
        TF_AXIOM(a[11] == sourceB[1]);
        TF_AXIOM(a[12] == sourceB[2]);
        TF_AXIOM(a[13] == sourceB[3]);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Remote storage with enough space to absorb new entries.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 1> a(sourceA.begin(), sourceA.end());
        a.reserve(15);

        if (!useIlist) {
            a.insert(a.end(), sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.end(), ilistB);
        }

        for (size_t i = 0; i < sourceA.size(); ++i) {
            TF_AXIOM(a[i] == sourceA[i]);
        }

        TF_AXIOM(a[10] == sourceB[0]);
        TF_AXIOM(a[11] == sourceB[1]);
        TF_AXIOM(a[12] == sourceB[2]);
        TF_AXIOM(a[13] == sourceB[3]);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Local Growth case.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 1> a(sourceA.begin(), sourceA.end());

        if (!useIlist) {
            a.insert(a.end(), sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.end(), ilistB);
        }

        TF_AXIOM(a.capacity() < a.size() + sourceB.size());

        for (size_t i = 0; i < sourceA.size(); ++i) {
            TF_AXIOM(a[i] == sourceA[i]);
        }

        TF_AXIOM(a[10] == sourceB[0]);
        TF_AXIOM(a[11] == sourceB[1]);
        TF_AXIOM(a[12] == sourceB[2]);
        TF_AXIOM(a[13] == sourceB[3]);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Remote Growth case.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 1> a(sourceA.begin(), sourceA.end());

        if (!useIlist) {
            a.insert(a.end(), sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.end(), ilistB);
        }

        TF_AXIOM(a.capacity() < a.size() + sourceB.size());

        for (size_t i = 0; i < sourceA.size(); ++i) {
            TF_AXIOM(a[i] == sourceA[i]);
        }

        TF_AXIOM(a[10] == sourceB[0]);
        TF_AXIOM(a[11] == sourceB[1]);
        TF_AXIOM(a[12] == sourceB[2]);
        TF_AXIOM(a[13] == sourceB[3]);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Test inserting at the front.
    //
    // Local storage with enough space to absorb new entries
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 15> a(sourceA.begin(), sourceA.end());

        // Splice in B.
        if (!useIlist) {
            a.insert(a.begin(), sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.begin(), ilistB);
        }

        TF_AXIOM(a[0]  == 999);
        TF_AXIOM(a[1]  == 998);
        TF_AXIOM(a[2]  == 997);
        TF_AXIOM(a[3]  == 996);
        TF_AXIOM(a[4]  == 0);
        TF_AXIOM(a[5]  == 1);
        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Remote storage with enough space to absorb new entries.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 1> a(sourceA.begin(), sourceA.end());

        // Splice in B.
        if (!useIlist) {
            a.insert(a.begin(), sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.begin(), ilistB);
        }

        TF_AXIOM(a[0]  == 999);
        TF_AXIOM(a[1]  == 998);
        TF_AXIOM(a[2]  == 997);
        TF_AXIOM(a[3]  == 996);
        TF_AXIOM(a[4]  == 0);
        TF_AXIOM(a[5]  == 1);
        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Local Growth case.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 11> a(sourceA.begin(), sourceA.end());

        TF_AXIOM(a.capacity() < a.size() + sourceB.size());

        if (!useIlist) {
            a.insert(a.begin(), sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.begin(), ilistB);
        }

        TF_AXIOM(a[0]  == 999);
        TF_AXIOM(a[1]  == 998);
        TF_AXIOM(a[2]  == 997);
        TF_AXIOM(a[3]  == 996);
        TF_AXIOM(a[4]  == 0);
        TF_AXIOM(a[5]  == 1);
        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Remote Growth case.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 1> a(sourceA.begin(), sourceA.end());

        TF_AXIOM(a.capacity() < a.size() + sourceB.size());

        if (!useIlist) {
            a.insert(a.begin(), sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.begin(), ilistB);
        }

        TF_AXIOM(a[0]  == 999);
        TF_AXIOM(a[1]  == 998);
        TF_AXIOM(a[2]  == 997);
        TF_AXIOM(a[3]  == 996);
        TF_AXIOM(a[4]  == 0);
        TF_AXIOM(a[5]  == 1);
        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Middle insertion case.

    // Local storage with space to absorb new entries.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 15> a(sourceA.begin(), sourceA.end());

        if (!useIlist) {
            a.insert(a.begin()+2, sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.begin()+2, ilistB);
        }

        TF_AXIOM(a[0] == 0);
        TF_AXIOM(a[1] == 1);

        TF_AXIOM(a[2] == 999);
        TF_AXIOM(a[3] == 998);
        TF_AXIOM(a[4] == 997);
        TF_AXIOM(a[5] == 996);

        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Remote storage with space to absorb new entries.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 1> a(sourceA.begin(), sourceA.end());
        a.reserve(15);

        if (!useIlist) {
            a.insert(a.begin()+2, sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.begin()+2, ilistB);
        }

        TF_AXIOM(a[0] == 0);
        TF_AXIOM(a[1] == 1);

        TF_AXIOM(a[2] == 999);
        TF_AXIOM(a[3] == 998);
        TF_AXIOM(a[4] == 997);
        TF_AXIOM(a[5] == 996);

        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Local storage growth case.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 11> a(sourceA.begin(), sourceA.end());

        TF_AXIOM(a.capacity() < a.size() + sourceB.size());

        if (!useIlist) {
            a.insert(a.begin()+2, sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.begin()+2, ilistB);
        }

        TF_AXIOM(a[0] == 0);
        TF_AXIOM(a[1] == 1);

        TF_AXIOM(a[2] == 999);
        TF_AXIOM(a[3] == 998);
        TF_AXIOM(a[4] == 997);
        TF_AXIOM(a[5] == 996);

        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);

    // Local storage growth case.
    for (bool useIlist : {false, true})
    {
        TfSmallVector<TestStruct, 1> a(sourceA.begin(), sourceA.end());

        TF_AXIOM(a.capacity() < a.size() + sourceB.size());

        if (!useIlist) {
            a.insert(a.begin()+2, sourceB.begin(), sourceB.end());
        }
        else {
            a.insert(a.begin()+2, ilistB);
        }

        TF_AXIOM(a[0] == 0);
        TF_AXIOM(a[1] == 1);

        TF_AXIOM(a[2] == 999);
        TF_AXIOM(a[3] == 998);
        TF_AXIOM(a[4] == 997);
        TF_AXIOM(a[5] == 996);

        TF_AXIOM(a[6]  == 2);
        TF_AXIOM(a[7]  == 3);
        TF_AXIOM(a[8]  == 4);
        TF_AXIOM(a[9]  == 5);
        TF_AXIOM(a[10] == 6);
        TF_AXIOM(a[11] == 7);
        TF_AXIOM(a[12] == 8);
        TF_AXIOM(a[13] == 9);

        TF_AXIOM(TestStruct::counter == 32);
    }

    TF_AXIOM(TestStruct::counter == 18);
}

static void
testResize()
{
    std::vector<int> sourceA(100);
    std::iota(sourceA.begin(), sourceA.end(), 0);

    // Shrink where T is trivial.
    {
        TfSmallVector<int, 10> v;
        v.insert(v.end(), sourceA.begin(), sourceA.end());

        TF_AXIOM(v.size() == 100);

        v.resize(73);

        TF_AXIOM(v.size() == 73);
        TF_AXIOM(v.capacity() == 100);
    }

    // grow where T is trivial
    {
        TfSmallVector<int, 10> v;
        v.insert(v.end(), sourceA.begin(), sourceA.end());

        TF_AXIOM(v.size() == 100);

        v.resize(150, 17);

        TF_AXIOM(v.size() == 150);
    }
}

static void
testErase()
{
    // Let's make sure we return the correct iterator after erase.
    {
        // Erase from the front...
        TfSmallVector<std::string, 1> vec;
        vec.push_back("0");
        vec.push_back("1");
        vec.push_back("2");
        vec.push_back("3");
        vec.push_back("4");
        vec.push_back("5");

        auto it = vec.begin();
        auto it2 = std::next(it, 2);

        auto retIt = vec.erase(it, it2);

        TF_AXIOM(*retIt == "2");
        TF_AXIOM(vec.size() == 4);
    }

    {
        // Erase from middle.
        TfSmallVector<std::string, 1> vec;
        vec.push_back("0");
        vec.push_back("1");
        vec.push_back("2");
        vec.push_back("3");
        vec.push_back("4");
        vec.push_back("5");

        auto it = std::next(vec.begin(), 2);
        auto it2 = std::next(it, 2);

        auto retIt = vec.erase(it, it2);

        TF_AXIOM(*retIt == "4");
        TF_AXIOM(vec.size() == 4);
    }

    {
        // Erase up to the end.
        TfSmallVector<std::string, 1> vec;
        vec.push_back("0");
        vec.push_back("1");
        vec.push_back("2");
        vec.push_back("3");
        vec.push_back("4");
        vec.push_back("5");

        auto it = std::next(vec.begin(), 3);
        auto it2 = vec.end();

        auto retIt = vec.erase(it, it2);

        TF_AXIOM(vec.end() == retIt);
        TF_AXIOM(vec.size() == 3);
    }

    {
        // Here's a case that covers moving stuff around, then deleting it.
        TfSmallVector<std::string, 1> vec;

        vec.push_back("asdf");
        vec.push_back("fdas");
        vec.push_back("qwer");
        vec.push_back("asdf");
        vec.push_back("zxcv");
        vec.push_back("fdas");
        vec.push_back("zxcv");
        vec.push_back("qwer");
        vec.push_back("zxcv");
        vec.push_back("123");
        vec.push_back("9087");
        vec.push_back("123");

        std::sort(vec.begin(), vec.end());
        auto it = std::unique(vec.begin(), vec.end());

        vec.erase(it, vec.end());

        TF_AXIOM(vec[0] == "123");
        TF_AXIOM(vec[1] == "9087");
        TF_AXIOM(vec[2] == "asdf");
        TF_AXIOM(vec[3] == "fdas");
        TF_AXIOM(vec[4] == "qwer");
        TF_AXIOM(vec[5] == "zxcv");
    }
}

////////////////////////////////////////////////////////////////////////////// 

static bool
Test_TfSmallVector()
{
    std::cout << "testConstructors" << std::endl;
    testConstructors();
    std::cout << "testNoLocalStorage" << std::endl;
    testNoLocalStorage();
    std::cout << "testGrowth" << std::endl;
    testGrowth();
    std::cout << "testIteration" << std::endl;
    testIteration();
    std::cout << "testInsertNoMoveConstructor" << std::endl;
    testInsertNoMoveConstructor();
    std::cout << "testCopyIntoVector" << std::endl;
    testCopyIntoVector();
    std::cout << "testInsertionTrivial of trivial types" << std::endl;
    testInsertionTrivial();
    std::cout << "testInsertion" << std::endl;
    testInsertion();
    std::cout << "testResize" << std::endl;
    testResize();
    std::cout << "testErase" << std::endl;
    testErase();
    std::cout << "... success" << std::endl;
    return true;
}

TF_ADD_REGTEST(TfSmallVector);
