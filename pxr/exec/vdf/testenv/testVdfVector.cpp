//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/indexedData.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/typedVector.h"
#include "pxr/exec/vdf/vectorData.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/vt/array.h"

#include <iostream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using std::string;
using std::vector;

// Helper for reporting errors in With(No)ExpectedErrors().
static void
_ReportErrors(std::string const &message, TfErrorMark const &mark)
{
    if (!mark.IsClean()) {
        std::cout << message << '\n';
        for (TfError const &err: mark) {
            std::cout 
                << "  "
                << err.GetContext().GetPrettyFunction() << " ("
                << err.GetContext().GetFile() << ":"
                << err.GetContext().GetLine() << ") - "
                << err.GetCommentary() << "\n";
        }
    }
}

// Invoke \p fn and report TfErrors issued during its execution.  If no errors
// were issued, issue a fatal error.
template <class Fn>
static void
WithExpectedErrors(Fn &&fn)
{
    TfErrorMark mark;
    std::forward<Fn>(fn)();
    _ReportErrors("Expected errors", mark);
    if (mark.IsClean()) {
        TF_FATAL_ERROR("Expected TfErrors but none issued");
    }
}

// Invoke \p fn and report TfErrors issued during its execution.  If errors were
// issued, issue a fatal error.
template <class Fn>
static void
WithNoExpectedErrors(Fn &&fn)
{
    TfErrorMark mark;
    std::forward<Fn>(fn)();
    _ReportErrors("Unexpected errors", mark);
    if (!mark.IsClean()) {
        TF_FATAL_ERROR("Unexpected TfErrors issued");
    }
}

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
     }()


template < typename T >
void SetFromVector(VdfVector *v, const std::vector<T> &s)
{
    v->Resize<T>(s.size());
    VdfVector::ReadWriteAccessor<T> a = v->GetReadWriteAccessor<T>();
    for (size_t i = 0; i < s.size(); ++i) {
        a[i] = s[i];
    }
}

template < typename T >
void SetFromVector(VdfVector *v, const std::vector<T> &s, const VdfMask &m)
{
    v->Resize<T>(m.GetBits());
    VdfVector::ReadWriteAccessor<T> a = v->GetReadWriteAccessor<T>();
    for (VdfMask::iterator it = m.begin(); !it.IsAtEnd(); ++it) {
        const int i = *it;
        a[i] = s[i];
    }
}

// --- Tests -------------------------------------------------------------------
// --- Tests -------------------------------------------------------------------
// --- Tests -------------------------------------------------------------------


static bool TestSize()
{
    TRACE_FUNCTION();

    size_t size = sizeof(VdfVector);

    printf("> sizeof = %zu\n", size);

    return size == 40;
}

namespace {
struct _SizeEstimatedStruct {
    static constexpr int FakeSize = 100;

    friend size_t VdfEstimateSize(const _SizeEstimatedStruct &v)
    {
        return _SizeEstimatedStruct::FakeSize;
    }
};
}

static bool TestEstimateSize()
{
    TRACE_FUNCTION();

    const double d = 13.0;
    VdfTypedVector<double> vd(d);
    ASSERT_EQ(vd.GetNumStoredElements(), 1);
    ASSERT_EQ(vd.EstimateElementMemory(), sizeof(double));

    struct _CustomStruct { double a = 1, b = 2; } const cs;
    VdfTypedVector<_CustomStruct> vcs(cs);
    ASSERT_EQ(vcs.GetNumStoredElements(), 1);
    ASSERT_EQ(vcs.EstimateElementMemory(), sizeof(_CustomStruct));

    const _SizeEstimatedStruct ses;
    VdfTypedVector<_SizeEstimatedStruct> vses(ses);
    ASSERT_EQ(vses.GetNumStoredElements(), 1);
    ASSERT_EQ(vses.EstimateElementMemory(), VdfEstimateSize(ses));
    ASSERT_EQ(vses.EstimateElementMemory(), _SizeEstimatedStruct::FakeSize);

    const std::vector<int> vec1 = {0, 1, 2, 3};
    VdfTypedVector<std::vector<int>> vv1(vec1);
    ASSERT_EQ(vv1.GetNumStoredElements(), 1);
    TF_AXIOM(vv1.EstimateElementMemory() == 
        sizeof(std::vector<int>) + vec1.capacity() * sizeof(int));

    const TfSmallVector<int, 4> vec2 = {0, 1, 2, 3};
    VdfTypedVector<TfSmallVector<int, 4>> vv2(vec2);
    ASSERT_EQ(vv2.GetNumStoredElements(), 1);
    ASSERT_EQ(vv2.EstimateElementMemory(), sizeof(TfSmallVector<int, 4>));

    const std::shared_ptr<float> sp1 = std::make_shared<float>(13.0f);
    VdfTypedVector<std::shared_ptr<float>> vsp1(sp1);
    ASSERT_EQ(vsp1.GetNumStoredElements(), 1);
    TF_AXIOM(vsp1.EstimateElementMemory() ==
        sizeof(std::shared_ptr<float>) + sizeof(float));

    const std::shared_ptr<float> sp2 = nullptr;
    VdfTypedVector<std::shared_ptr<float>> vsp2(sp2);
    ASSERT_EQ(vsp2.GetNumStoredElements(), 1);
    ASSERT_EQ(vsp2.EstimateElementMemory(), sizeof(std::shared_ptr<float>));

    return true;
}

static bool TestSingleElement()
{
    TRACE_FUNCTION();

    VdfTypedVector<string> v;

    TF_AXIOM(v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 0);
    ASSERT_EQ(v.GetNumStoredElements(), 0);

    string a = "a";
    string b = "b";

    v.Set(a);
    TF_AXIOM(!v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 1);
    ASSERT_EQ(v.GetNumStoredElements(), 1);
    ASSERT_EQ(v.GetReadAccessor<string>()[0], a);

    v.Set(b);
    TF_AXIOM(!v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 1);
    ASSERT_EQ(v.GetNumStoredElements(), 1);
    TF_AXIOM(v.GetReadAccessor<string>()[0] != a);
    ASSERT_EQ(v.GetReadAccessor<string>()[0], b);

    return true;
}

static bool TestDenseVector()
{
    TRACE_FUNCTION();

    VdfTypedVector<string> v;

    TF_AXIOM(v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 0);

    string a = "a";
    string b = "b";
    string c = "c";

    vector<string> vec;
    vec.push_back(a);
    vec.push_back(b);
    vec.push_back(c);

    SetFromVector(&v, vec);
    TF_AXIOM(!v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 3);
    ASSERT_EQ(v.GetSize(), vec.size());
    ASSERT_EQ(v.GetNumStoredElements(), 3);

    // Also test low level access via GetReadAccessor().
    ASSERT_EQ(v.GetSize(), v.GetReadAccessor<string>().GetNumValues());
    ASSERT_EQ(v.IsEmpty(), v.GetReadAccessor<string>().IsEmpty());

    for(size_t i=0; i<v.GetSize(); i++) {
        ASSERT_EQ(v.GetReadAccessor<string>()[i], vec[i]);
    }

    VdfVector::ReadAccessor<string> access = v.GetReadAccessor<string>();

    for(size_t i=0; i<access.GetNumValues(); i++) {
        ASSERT_EQ(access[i], vec[i]);
    }

    return true;
}

static bool TestSparseVector()
{
    TRACE_FUNCTION();

    VdfTypedVector<string> v;

    TF_AXIOM(v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 0);

    string a = "a";
    string b = "b";
    string c = "c";

    vector<string> vec;
    vec.push_back(a);
    vec.push_back(b);
    vec.push_back(c);

    VdfMask mask(vec.size());
    mask.SetIndex(1);

    SetFromVector(&v, vec, mask);
    TF_AXIOM(!v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 3);
    ASSERT_EQ(v.GetSize(), vec.size());
    ASSERT_EQ(v.GetNumStoredElements(), 1);

    // Also test low level access via GetReadAccessor().
    ASSERT_EQ(v.GetSize(), v.GetReadAccessor<string>().GetNumValues());
    ASSERT_EQ(v.IsEmpty(), v.GetReadAccessor<string>().IsEmpty());

    VdfVector::ReadAccessor<string> access = v.GetReadAccessor<string>();

    for(size_t i=0; i<access.GetNumValues(); i++) {
        if (i == 1) {
            ASSERT_EQ(access[i], vec[i]);
        }
    }

    return true;
}

static bool TestCompressedVector() 
{
    TRACE_FUNCTION();

    VdfTypedVector<string> v;

    TF_AXIOM(v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 0);
    ASSERT_EQ(v.GetNumStoredElements(), 0);

    // Create a large, very sparse vector and mask to be sure to trip the
    // heuristic that decides when to use compressed vectors.
    size_t numElements = 1500;
    vector<string> vec(numElements);
    for (size_t i=0; i<numElements; i++) {
        vec[i] = TfStringify((double)i);
    }

    VdfMask mask(vec.size());
    mask.SetIndex(1);
    mask.SetIndex(3);
    mask.SetIndex(4);
    mask.SetIndex(1499);

    VdfTypedVector<string> w;
    SetFromVector(&w, vec, mask);
    v.Copy(w, mask);

    // Also test low level access via GetReadAccessor().
    ASSERT_EQ(v.GetSize(), numElements);
    ASSERT_EQ(v.GetNumStoredElements(), 4);
    TF_AXIOM(!v.IsEmpty());
    ASSERT_EQ(v.GetSize(), v.GetReadAccessor<string>().GetNumValues());
    ASSERT_EQ(v.IsEmpty(), v.GetReadAccessor<string>().IsEmpty());

    VdfVector::ReadAccessor<string> access = v.GetReadAccessor<string>();

    ASSERT_EQ(access[1], vec[1]);
    ASSERT_EQ(access[3], vec[3]);
    ASSERT_EQ(access[4], vec[4]);
    ASSERT_EQ(access[1499], vec[1499]);

    // Test extracting an array from a compressed impl
    const VtArray<std::string> array = v.ExtractAsVtArray<std::string>(2, 3);
    ASSERT_EQ(array.size(), 2);
    ASSERT_EQ(array[0], "3");
    ASSERT_EQ(array[1], "4");

    return true;
}

struct TestStruct
{
    TestStruct() = default;
    explicit TestStruct(int a, int b) : _m(a*b) { ctorCalled++; }

    TestStruct(const TestStruct& rhs) : _m(rhs._m) { copyCalled++; }
    TestStruct(TestStruct&& rhs) : _m(std::move(rhs._m)) { moveCalled++; }

    TestStruct& operator=(const TestStruct& rhs) { 
        _m = rhs._m; 
        copyCalled++; 
        return *this; 
    }

    TestStruct& operator=(TestStruct&& rhs) {
        _m = std::move(rhs._m);
        moveCalled++;
        return *this;
    }

    ~TestStruct() { dtorCalled++; }

    int _m;

    static int ctorCalled;
    static int copyCalled;
    static int moveCalled;
    static int dtorCalled;
};

int TestStruct::ctorCalled = 0;
int TestStruct::copyCalled = 0;
int TestStruct::moveCalled = 0;
int TestStruct::dtorCalled = 0;

static bool TestSharedVector()
{
    TRACE_FUNCTION();

    // Tests for basic shared vector usage
    {

        // Non-sharable vector.
        {
            VdfTypedVector<int> vec;

            TF_AXIOM(vec.IsEmpty());
            ASSERT_EQ(vec.GetSize(), 0);

            vector<int> v;
            v.push_back(1);
            v.push_back(2);
            v.push_back(3);

            SetFromVector(&vec, v);
            TF_AXIOM(!vec.IsEmpty());
            ASSERT_EQ(vec.GetSize(), 3);
            ASSERT_EQ(vec.GetSize(), v.size());
            ASSERT_EQ(vec.GetNumStoredElements(), 3);

            bool success = vec.Share();

            TF_AXIOM(!success);

            // Nothing should change
            TF_AXIOM(!vec.IsEmpty());
            ASSERT_EQ(vec.GetSize(), 3);
            ASSERT_EQ(vec.GetSize(), v.size());
            ASSERT_EQ(vec.GetNumStoredElements(), 3);

            VdfVector::ReadAccessor<int> access0 = vec.GetReadAccessor<int>();
            for (size_t i = 0; i < access0.GetNumValues(); i++) {
                ASSERT_EQ(access0[i], v[i]);
            }
        }

        // Sharable vector.
        {
            // Big enough to trip metric.
            constexpr size_t size = 50001;

            VdfTypedVector<int> vec;

            vector<int> v(size);
            for (size_t i = 0; i < size; ++i) {
                v[i] = i;
            }

            SetFromVector(&vec, v);
            bool success = vec.Share();

            TF_AXIOM(success);
            ASSERT_EQ(vec.GetSize(), v.size());

            VdfVector::ReadAccessor<int> access0 = vec.GetReadAccessor<int>();
            for (size_t i = 0; i < access0.GetNumValues(); i++) {
                ASSERT_EQ(access0[i], v[i]);
            }

            // Test Extraction
            VtArray<int> array = vec.ExtractAsVtArray<int>(size, 0);

            ASSERT_EQ(array.size(), vec.GetSize());
            for (size_t i = 0; i < access0.GetNumValues(); i++) {
                ASSERT_EQ(access0[i], array[i]);
            }
        
            // Make a copy
            VdfTypedVector<int> copyVec = vec;

            // Test read-only access of copy
            VdfVector::ReadAccessor<int> access1 = 
                copyVec.GetReadAccessor<int>();

            for (size_t i = 0; i < access1.GetNumValues(); i++) {
                ASSERT_EQ(access1[i], v[i]);
            }

            // We expect the actual objects for shared vectors to be the same, 
            // so make sure they are stored at the same locations. 
            for (size_t i = 0; i < access0.GetNumValues(); i++) {
                ASSERT_EQ(&access0[i], &access1[i]);
            }

            // Detach a copy
            VdfVector::ReadWriteAccessor<int> access2 = 
                vec.GetReadWriteAccessor<int>();

            // Write to the detached copy and make sure shared values 
            // don't change.
            access2[0] = -1;
            ASSERT_EQ(access0[0], v[0]);
            ASSERT_EQ(access1[0], v[0]);
            TF_AXIOM(access2[0] != v[0]); 

            // A detached vector object should not be at the same location as 
            // a shared one.
            for (size_t i = 0; i < access1.GetNumValues(); i++) {
                TF_AXIOM(&access1[i] != &access2[i]);
            }
        }
    }

    /// Test that no-copying occurs when copying and extracting.
    {
        // The TF_AXIOM for dtorCalled == 3 fails without doing this even
        // though the value is 3?
        TestStruct::ctorCalled = 0;
        TestStruct::copyCalled = 0;
        TestStruct::moveCalled = 0;
        TestStruct::dtorCalled = 0;

        VdfTypedVector<TestStruct> v;
        TF_AXIOM(v.IsEmpty());
        ASSERT_EQ(v.GetSize(), 0);

        SetFromVector(&v, std::vector<TestStruct>(50000, TestStruct(2, 7)));
        ASSERT_EQ(v.GetSize(), 50000);
        TF_AXIOM(!v.IsEmpty());

        // Create initial v.
        // One call to ctor for creating a temp TestStruct.
        // 100k calls to copy, 50k for copying temp into std::vector, and 50k 
        // for copying std::vector into v.
        // 50k + 1 calls to dtor, one for the temp TestStruct and 50k for the 
        // std::vector values.
        ASSERT_EQ(TestStruct::ctorCalled, 1);
        ASSERT_EQ(TestStruct::copyCalled, 100000);
        ASSERT_EQ(TestStruct::moveCalled, 0);
        ASSERT_EQ(TestStruct::dtorCalled, 50001);
        TestStruct::ctorCalled = 0;
        TestStruct::copyCalled = 0;
        TestStruct::moveCalled = 0;
        TestStruct::dtorCalled = 0;

        // Do the share.
        TF_AXIOM(v.Share());

        // No calls should happen. Moving happens to the DataHolder, not 
        // individual elements. Internal reference count is set to one.
        ASSERT_EQ(TestStruct::ctorCalled, 0);
        ASSERT_EQ(TestStruct::copyCalled, 0);
        ASSERT_EQ(TestStruct::moveCalled, 0);
        ASSERT_EQ(TestStruct::dtorCalled, 0);
        TestStruct::ctorCalled = 0;
        TestStruct::copyCalled = 0;
        TestStruct::moveCalled = 0;
        TestStruct::dtorCalled = 0;

        {
            // Make a copy
            VdfTypedVector<TestStruct> vCopy = v;

            // No calls should happen.
            // Internal reference count is set to two.
            ASSERT_EQ(TestStruct::ctorCalled, 0);
            ASSERT_EQ(TestStruct::copyCalled, 0);
            ASSERT_EQ(TestStruct::moveCalled, 0);
            ASSERT_EQ(TestStruct::dtorCalled, 0);
            TestStruct::ctorCalled = 0;
            TestStruct::copyCalled = 0;
            TestStruct::moveCalled = 0;
            TestStruct::dtorCalled = 0;

            // Detach vCopy.
            // A copy should happen for each value in vCopy, decrease the
            // reference count to one.
            vCopy.GetReadWriteAccessor<TestStruct>();
            ASSERT_EQ(TestStruct::ctorCalled, 0);
            ASSERT_EQ(TestStruct::copyCalled, 50000);
            ASSERT_EQ(TestStruct::moveCalled, 0);
            ASSERT_EQ(TestStruct::dtorCalled, 0);
            TestStruct::ctorCalled = 0;
            TestStruct::copyCalled = 0;
            TestStruct::moveCalled = 0;
            TestStruct::dtorCalled = 0;
        }

        // Exit scope and destroy vCopy
        // A dtor should be called for each value in vCopy because vCopy is not
        // shared.
        ASSERT_EQ(TestStruct::ctorCalled, 0);
        ASSERT_EQ(TestStruct::copyCalled, 0);
        ASSERT_EQ(TestStruct::moveCalled, 0);
        ASSERT_EQ(TestStruct::dtorCalled, 50000);
        TestStruct::ctorCalled = 0;
        TestStruct::copyCalled = 0;
        TestStruct::moveCalled = 0;
        TestStruct::dtorCalled = 0;

        {
            // Make a copy
            VdfTypedVector<TestStruct> vCopy = v;

            // No calls should happen.
            // Internal reference count is set to two.
            ASSERT_EQ(TestStruct::ctorCalled, 0);
            ASSERT_EQ(TestStruct::copyCalled, 0);
            ASSERT_EQ(TestStruct::moveCalled, 0);
            ASSERT_EQ(TestStruct::dtorCalled, 0);
            TestStruct::ctorCalled = 0;
            TestStruct::copyCalled = 0;
            TestStruct::moveCalled = 0;
            TestStruct::dtorCalled = 0;
        }

        // Exit scope and destroy vCopy
        // No dtors should be called cause vCopy was shared.
        ASSERT_EQ(TestStruct::ctorCalled, 0);
        ASSERT_EQ(TestStruct::copyCalled, 0);
        ASSERT_EQ(TestStruct::moveCalled, 0);
        ASSERT_EQ(TestStruct::dtorCalled, 0);
        TestStruct::ctorCalled = 0;
        TestStruct::copyCalled = 0;
        TestStruct::moveCalled = 0;
        TestStruct::dtorCalled = 0;

        // Extract v.
        VtArray<TestStruct> array = v.ExtractAsVtArray<TestStruct>(50000, 0);

        // No calls should happen.
        // Internal reference count is set to two.
        ASSERT_EQ(TestStruct::ctorCalled, 0);
        ASSERT_EQ(TestStruct::copyCalled, 0);
        ASSERT_EQ(TestStruct::moveCalled, 0);
        ASSERT_EQ(TestStruct::dtorCalled, 0);
        TestStruct::ctorCalled = 0;
        TestStruct::copyCalled = 0;
        TestStruct::moveCalled = 0;
        TestStruct::dtorCalled = 0;
    }

    // Exit scope and destroy v.
    // Reference count will reach zero and dtors should be called for each
    // value in v.
    ASSERT_EQ(TestStruct::ctorCalled, 0);
    ASSERT_EQ(TestStruct::copyCalled, 0);
    ASSERT_EQ(TestStruct::moveCalled, 0);
    ASSERT_EQ(TestStruct::dtorCalled, 50000);

    // Test that dropping the last reference via VtArray destroys the shared
    // data.
    {
        VtArray<TestStruct> array;
        {
            VdfTypedVector<TestStruct> v;
            TF_AXIOM(v.IsEmpty());
            ASSERT_EQ(v.GetSize(), 0);

            SetFromVector(&v, std::vector<TestStruct>(50000, TestStruct(2, 7)));
            ASSERT_EQ(v.GetSize(), 50000);
            TF_AXIOM(!v.IsEmpty());

            TF_AXIOM(v.Share());

            TestStruct::ctorCalled = 0;
            TestStruct::copyCalled = 0;
            TestStruct::moveCalled = 0;
            TestStruct::dtorCalled = 0;

            array = v.ExtractAsVtArray<TestStruct>(50000, 0);

            ASSERT_EQ(TestStruct::ctorCalled, 0);
            ASSERT_EQ(TestStruct::copyCalled, 0);
            ASSERT_EQ(TestStruct::moveCalled, 0);
            ASSERT_EQ(TestStruct::dtorCalled, 0);
        }
    }
    ASSERT_EQ(TestStruct::ctorCalled, 0);
    ASSERT_EQ(TestStruct::copyCalled, 0);
    ASSERT_EQ(TestStruct::moveCalled, 0);
    ASSERT_EQ(TestStruct::dtorCalled, 50000);

    // Tests that taking ownership over the last instance of a shared vector,
    // and then dropping that last instance, destroys the shared data.
    {
        VdfTypedVector<TestStruct> v;
        TF_AXIOM(v.IsEmpty());
        ASSERT_EQ(v.GetSize(), 0);

        SetFromVector(&v, std::vector<TestStruct>(50000, TestStruct(2, 7)));
        ASSERT_EQ(v.GetSize(), 50000);
        TF_AXIOM(!v.IsEmpty());

        TF_AXIOM(v.Share());
        TF_AXIOM(v.IsShared());

        TestStruct::ctorCalled = 0;
        TestStruct::copyCalled = 0;
        TestStruct::moveCalled = 0;
        TestStruct::dtorCalled = 0;

        // Make a copy of the shared vector. This should not make a copy of
        // the shared data.
        {
            VdfVector vCopy(v);
            TF_AXIOM(vCopy.IsShared());
            ASSERT_EQ(TestStruct::ctorCalled, 0);
            ASSERT_EQ(TestStruct::copyCalled, 0);
            ASSERT_EQ(TestStruct::moveCalled, 0);
            ASSERT_EQ(TestStruct::dtorCalled, 0);
        }

        // When the copy of the shared vector goes out of scope, the shared
        // data should not be destructed.
        TF_AXIOM(v.IsShared());
        ASSERT_EQ(TestStruct::ctorCalled, 0);
        ASSERT_EQ(TestStruct::copyCalled, 0);
        ASSERT_EQ(TestStruct::moveCalled, 0);
        ASSERT_EQ(TestStruct::dtorCalled, 0);

        // Detach the last remaining instance of the shared vector by
        // constructing a read/write accessor to the data.
        v.GetReadWriteAccessor<TestStruct>();

        // Detaching the last remaining instance above should not have made
        // a copy of the shared data. Instead, it we should have just taken
        // ownership of the shared data, meaning it is now no longer shared.
        TF_AXIOM(!v.IsShared());
        ASSERT_EQ(TestStruct::ctorCalled, 0);
        ASSERT_EQ(TestStruct::copyCalled, 0);
        ASSERT_EQ(TestStruct::moveCalled, 0);
        ASSERT_EQ(TestStruct::dtorCalled, 0);

        // When the last remaining instance goes out of scope, we expect the
        // destructor to be called on the data. Note that the data is now no
        // longer shared, since we have taken ownership of the shared data
        // above.
    }
    ASSERT_EQ(TestStruct::ctorCalled, 0);
    ASSERT_EQ(TestStruct::copyCalled, 0);
    ASSERT_EQ(TestStruct::moveCalled, 0);
    ASSERT_EQ(TestStruct::dtorCalled, 50000);
    
    // Tests for shared vector boxed iteration
    {
        constexpr size_t size = 50000;

        VdfTypedVector<int> vec;
        
        Vdf_BoxedContainer<int> source(size);
        for (size_t i = 0; i < size; ++i) {
            source[i] = i;
        }

        vec.Set(source);
        TF_AXIOM(vec.Share());

        // Direct iteration.
        {
            VdfVector::ReadAccessor<int> accessor = vec.GetReadAccessor<int>();
            ASSERT_EQ(accessor.GetNumValues(), size);
            TF_AXIOM(accessor.IsBoxed());

            for (size_t i = 0; i < accessor.GetNumValues(); ++i)
            {
                ASSERT_EQ(accessor[i], source[i]);
            }
        }

        // Pull out TYPE* and use it as a basic array.
        {
            VdfVector::ReadAccessor<int> accessor = vec.GetReadAccessor<int>();
            ASSERT_EQ(accessor.GetNumValues(), size);
            TF_AXIOM(accessor.IsBoxed());

            const int* data = &accessor[0];

            for (size_t i = 0; i < accessor.GetNumValues(); ++i)
            {
                ASSERT_EQ(data[i], source[i]);
            }
        }
    }

    // Tests for shared to non-shared vector copying
    {
        // Here we test copying from various kinds of shared vectors in light of 
        // subset copying.
        VdfTypedVector<double> denseVector;
        std::vector<double> rawVector;
        const size_t numElements = 50002;
        
        for (size_t i = 0; i < numElements; ++i) {
            rawVector.push_back((double)i);
        }

        SetFromVector(&denseVector, rawVector);
        denseVector.Share();

        VdfTypedVector<double> sparseVector;
        VdfMask sparseMask(numElements);

        // Copy a subset of the elements from a shared denseVector
        for (size_t i=1; i<numElements-1; i++) {
            sparseMask.SetIndex(i);
        }
        sparseVector.Copy(denseVector, sparseMask);

        ASSERT_EQ(sparseVector.GetNumStoredElements(), numElements-2);
        ASSERT_EQ(sparseVector.GetSize(), denseVector.GetSize());

        for (VdfMask::iterator it = sparseMask.begin(); !it.IsAtEnd(); ++it) {
            VdfVector::ReadAccessor<double> accessor = 
                sparseVector.GetReadAccessor<double>();
            if (accessor[*it] != (double)(*it)) {
                std::cout << "ERROR: Got "  
                        << accessor[*it] << " expected: " 
                        << (double)(*it) << std::endl;
                return false;
            }
        }

        TF_AXIOM(denseVector.IsShared());
        TF_AXIOM(!sparseVector.IsShared());

        // Copy a single element from a shared sparse vector.
        sparseVector.Share();

        VdfTypedVector<double> sparserVector;
        VdfMask singleEntryMask(numElements);
        singleEntryMask.SetIndex(33);

        sparserVector.Copy(sparseVector, singleEntryMask);
        ASSERT_EQ(sparserVector.GetNumStoredElements(), 1);
        ASSERT_EQ(sparserVector.GetSize(), sparseVector.GetSize());

        // Test expected value
        {
            VdfVector::ReadAccessor<double> accessor = 
                sparserVector.GetReadAccessor<double>();
            if (accessor[33] != 33) {
                std::cout << "ERROR: Got "  
                        << accessor[33] << " expected: " 
                        << 33 << std::endl;
                return false;
            }
        }

        TF_AXIOM(sparseVector.IsShared());
        TF_AXIOM(!sparserVector.IsShared());

        // Copy nothing from the shared denseVector
        VdfTypedVector<double> emptySparseVector;
        VdfMask emptyMask(numElements);
        emptySparseVector.Copy(denseVector, emptyMask);
        ASSERT_EQ(emptySparseVector.GetNumStoredElements(), 0);
        ASSERT_EQ(emptySparseVector.GetSize(), denseVector.GetSize());
        if (emptySparseVector.GetSize() != denseVector.GetSize()) {
            std::cout << "ERROR: empty sparse vector size is " 
                    << emptySparseVector.GetSize() 
                    << " expected " << denseVector.GetSize()
                    << std::endl;
            return false;
        }

        TF_AXIOM(denseVector.IsShared());
        TF_AXIOM(!emptySparseVector.IsShared());

        // Copy nothing from a shared sparse Vector
        emptySparseVector.Copy(sparseVector, emptyMask);
        ASSERT_EQ(emptySparseVector.GetNumStoredElements(), 0);
        ASSERT_EQ(emptySparseVector.GetSize(), sparseVector.GetSize());
        if (emptySparseVector.GetSize() != denseVector.GetSize()) {
            std::cout << "ERROR: empty sparse vector size is " 
                    << emptySparseVector.GetSize() 
                    << " expected " << denseVector.GetSize()
                    << std::endl;
            return false;
        }

        TF_AXIOM(sparseVector.IsShared());
        TF_AXIOM(!emptySparseVector.IsShared());

        // Copy discontiguous chunks from a share dense vector to a
        // compressed sparse vector.
        VdfTypedVector<double> compressedVector;
        VdfMask discontiguousMask(numElements);
        // Set three groups of ten.
        for (size_t i=0; i<10; ++i)
            discontiguousMask.SetIndex(i);
        for (size_t i=750; i<760; ++i)
            discontiguousMask.SetIndex(i);
        for (size_t i=1400; i<1500; ++i)
            discontiguousMask.SetIndex(i);

        compressedVector.Copy(denseVector, discontiguousMask);
        ASSERT_EQ(compressedVector.GetSize(), discontiguousMask.GetSize());
        TF_AXIOM(compressedVector.GetNumStoredElements()
            == discontiguousMask.GetNumSet());

        // Test expected values
        for (VdfMask::iterator it = discontiguousMask.begin();
            !it.IsAtEnd(); ++it)
        {
            VdfVector::ReadAccessor<double> accessor = 
                compressedVector.GetReadAccessor<double>();
            if (accessor[*it] != (double)(*it)) {
                std::cout << "ERROR: Got "  
                        << accessor[*it] << " expected: " 
                        << (double)(*it) << std::endl;
                return false;
            }
        }

        TF_AXIOM(denseVector.IsShared());
        TF_AXIOM(!compressedVector.IsShared());
    }

    // Tests for non-shared to shared vector copying
    {
        // Here we test copying to various kinds of shared vectors in light of 
        // subset copying.
        VdfTypedVector<double> denseVector;
        std::vector<double> rawVector;
        const size_t numElements = 50000;
        
        for (size_t i = 0; i < numElements; ++i) {
            rawVector.push_back((double)i);
        }

        SetFromVector(&denseVector, rawVector);

        VdfTypedVector<double> sparseVector;
        sparseVector.Share();
        VdfMask sparseMask(numElements);

        // Copy a subset of the elements from a denseVector
        for (size_t i=1; i<numElements-1; i++) {
            sparseMask.SetIndex(i);
        }
        sparseVector.Copy(denseVector, sparseMask);

        ASSERT_EQ(sparseVector.GetNumStoredElements(), numElements-2);
        ASSERT_EQ(sparseVector.GetSize(), denseVector.GetSize());

        for (VdfMask::iterator it = sparseMask.begin(); !it.IsAtEnd(); ++it) {
            VdfVector::ReadAccessor<double> accessor = 
                sparseVector.GetReadAccessor<double>();
            if (accessor[*it] != (double)(*it)) {
                std::cout << "ERROR: Got "  
                        << accessor[*it] << " expected: " 
                        << (double)(*it) << std::endl;
                return false;
            }
        }

        TF_AXIOM(!denseVector.IsShared());
        TF_AXIOM(!sparseVector.IsShared());
    }

    // Tests for shared to shared vector copying
    {
        // Here we test copying to and from various kinds of shared vectors in 
        // light of subset copying.
        VdfTypedVector<double> denseVector;
        std::vector<double> rawVector;
        const size_t numElements = 50000;
        
        for (size_t i = 0; i < numElements; ++i) {
            rawVector.push_back((double)i);
        }

        SetFromVector(&denseVector, rawVector);
        denseVector.Share();

        VdfTypedVector<double> sparseVector;
        sparseVector.Share();
        VdfMask sparseMask(numElements);

        // Copy a subset of the elements from a denseVector
        for (size_t i=1; i<numElements-1; i++) {
            sparseMask.SetIndex(i);
        }
        sparseVector.Copy(denseVector, sparseMask);

        ASSERT_EQ(sparseVector.GetNumStoredElements(), numElements-2);
        ASSERT_EQ(sparseVector.GetSize(), denseVector.GetSize());

        for (VdfMask::iterator it = sparseMask.begin(); !it.IsAtEnd(); ++it) {
            VdfVector::ReadAccessor<double> accessor = 
                sparseVector.GetReadAccessor<double>();
            if (accessor[*it] != (double)(*it)) {
                std::cout << "ERROR: Got "  
                        << accessor[*it] << " expected: " 
                        << (double)(*it) << std::endl;
                return false;
            }
        }

        TF_AXIOM(denseVector.IsShared());
        TF_AXIOM(!sparseVector.IsShared());
    }

    // Tests for shared vector merge
    {
        // Create the soure vector and fill it with ints 0 - 49999
        VdfTypedVector<int> source;
        std::vector<int> sourceData;
        for (int i = 0; i <= 49999; ++i) {
            sourceData.push_back(i);
        }

        // Create the destination vector and fill it with ints 50000-99999
        VdfTypedVector<int> dest;
        std::vector<int> destData;
        for (int i = 50000; i <= 99999; ++i) {
            destData.push_back(i);
        }

        VdfMask mask(50000);
        for (size_t i = 0; i < 50000; ++i) {
            mask.SetIndex(i);
        }

        // source vector shared, dest vector is not
        {
            SetFromVector(&source, sourceData);
            SetFromVector(&dest, destData);
            source.Share();

            dest.Merge(source, mask);

            TF_AXIOM(source.IsShared());
            TF_AXIOM(!dest.IsShared());
        }

        // dest vector shared, source vector is not
        {
            SetFromVector(&source, sourceData);
            SetFromVector(&dest, destData);
            dest.Share();

            dest.Merge(source, mask);

            TF_AXIOM(!source.IsShared());
            TF_AXIOM(!dest.IsShared());
        }

        // source and dest vectors are shared
        {
            SetFromVector(&source, sourceData);
            SetFromVector(&dest, destData);
            source.Share();
            dest.Share();

            dest.Merge(source, mask);

            TF_AXIOM(source.IsShared());
            TF_AXIOM(!dest.IsShared());
        }
    }

    return true;
}

static bool TestAssignmentOperator()
{
    TRACE_FUNCTION();

    VdfTypedVector<string> v;

    TF_AXIOM(v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 0);

    string a = "a";
    string b = "b";
    string c = "c";

    vector<string> vec;
    vec.push_back(a);
    vec.push_back(b);
    vec.push_back(c);

    VdfMask mask(vec.size());
    mask.SetAll();

    SetFromVector(&v, vec, mask);
    TF_AXIOM(!v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 3);
    ASSERT_EQ(v.GetSize(), vec.size());
    ASSERT_EQ(v.GetNumStoredElements(), 3);

    VdfTypedVector<string> copyVec;

    copyVec = v;

    VdfVector::ReadAccessor<string> access0 =       v.GetReadAccessor<string>();
    VdfVector::ReadAccessor<string> access1 = copyVec.GetReadAccessor<string>();

    ASSERT_EQ(access0.GetNumValues(), access1.GetNumValues());

    // Make sure elements are the same, but copied.
    for(size_t i=0; i<access0.GetNumValues(); i++)
    {
        printf("%s %s %s\n", vec[i].c_str(),
            access0[i].c_str(), access1[i].c_str());

        ASSERT_EQ(access0[i], access1[i]);
        ASSERT_EQ(access0[i], vec[i]);

        // We expect the actual objects to be different instances, so make sure
        // they are stored at different locations. 
        TF_AXIOM(&access0[i] != &access1[i]);
    }

    return true;
}

static bool TestTyping()
{
    TRACE_FUNCTION();

    // Test Holds() API
    VdfTypedVector<string> stringVec;
    TF_AXIOM(stringVec.Holds<string>());
    TF_AXIOM(!stringVec.Holds<double>());

    // Test boxed container cross-typing: Setting values.
    VdfTypedVector<double> doubleVec;
    doubleVec.Set(1.0);
    TF_AXIOM(doubleVec.Holds<double>());

    Vdf_BoxedContainer<double> boxed(2);
    doubleVec.Set(boxed);
    TF_AXIOM(doubleVec.Holds<double>());

    doubleVec.Set(1.0);
    TF_AXIOM(doubleVec.Holds<double>());

    // Test boxed container cross-typing: Moving values.
    doubleVec.Set(std::move(boxed));
    TF_AXIOM(doubleVec.Holds<double>());

    // Test boxed container cross-typing: Resizing.
    doubleVec.Resize<double>(0);
    TF_AXIOM(doubleVec.Holds<double>());

    // Test boxed container cross-typing: Copying.
    VdfTypedVector<double> otherDoubleVec1;
    otherDoubleVec1.Set(1.0);
    doubleVec.Copy(otherDoubleVec1, VdfMask::AllOnes(1));
    TF_AXIOM(doubleVec.Holds<double>());

    // Test boxed container cross-typing: Merging.
    VdfTypedVector<double> otherDoubleVec2;
    otherDoubleVec2.Set(Vdf_BoxedContainer<double>(2));
    doubleVec.Merge(otherDoubleVec2, VdfMask::AllOnes(1));
    TF_AXIOM(doubleVec.Holds<double>());

    // Test boxed container cross-typing: Assignment.
    doubleVec = otherDoubleVec1;
    TF_AXIOM(doubleVec.Holds<double>());

    doubleVec = otherDoubleVec2;
    TF_AXIOM(doubleVec.Holds<double>());

    return true;
}

/// Suite of tests for boxed vector types.
static bool TestBoxedIteration()
{
    TRACE_FUNCTION();

    // Iterate over a Vdf_BoxedContainer containing trivially copyable data.
    {
        constexpr size_t size = 1000;

        VdfTypedVector<int> vec;
        
        Vdf_BoxedContainer<int> source(size);
        for (size_t i = 0; i < size; ++i) {
            source[i] = i;
        }

        vec.Set(source);

        // Direct iteration.
        {
            VdfVector::ReadAccessor<int> accessor = vec.GetReadAccessor<int>();
            ASSERT_EQ(accessor.GetNumValues(), size);
            TF_AXIOM(accessor.IsBoxed());

            for (size_t i = 0; i < accessor.GetNumValues(); ++i)
            {
                ASSERT_EQ(source[i], accessor[i]);
            }
        }

        // Pull out TYPE* and use it as a basic array.
        {
            VdfVector::ReadAccessor<int> accessor = vec.GetReadAccessor<int>();
            ASSERT_EQ(accessor.GetNumValues(), size);
            TF_AXIOM(accessor.IsBoxed());

            const int *data = &accessor[0];

            for (size_t i = 0; i < accessor.GetNumValues(); ++i)
            {
                ASSERT_EQ(data[i], source[i]);
            }
        }

        // When backed by TfSmallVector<TYPE, 1>, make sure the size == 1
        // case works.
        {
            Vdf_BoxedContainer<int> source(1);
            source[0] = 9999;

            vec.Set(source);

            // Basic iteration
            {
                VdfVector::ReadAccessor<int> accessor = vec.GetReadAccessor<int>();
                ASSERT_EQ(accessor.GetNumValues(), source.size());
                TF_AXIOM(accessor.IsBoxed());

                for (size_t i = 0; i < accessor.GetNumValues(); ++i)
                {
                    ASSERT_EQ(source[i], accessor[i]);
                }
            }

            // Pull out TYPE* and use it as a basic array.
            {
                VdfVector::ReadAccessor<int> accessor = vec.GetReadAccessor<int>();
                ASSERT_EQ(accessor.GetNumValues(), source.size());
                TF_AXIOM(accessor.IsBoxed());

                const int *data = &accessor[0];

                for (size_t i = 0; i < accessor.GetNumValues(); ++i)
                {
                    ASSERT_EQ(data[i], source[i]);
                }
            }
        }
    }

    // Iterating over Vdf_BoxedContainer<bool>
    {
        using BoolVector = Vdf_BoxedContainer<bool>;

        VdfTypedVector<bool> vec;

        BoolVector source(10);
        for (size_t i = 0; i < 10; ++i)
        {
            source[i] = bool(i);
        }

        vec.Set(source);

        // Iterate directly.
        {
            VdfVector::ReadAccessor<bool> accessor = vec.GetReadAccessor<bool>();

            ASSERT_EQ(accessor.GetNumValues(), 10);
            TF_AXIOM(accessor.IsBoxed());

            for (size_t i = 0; i < source.size(); ++i)
            {
                ASSERT_EQ(accessor[i], source[i]);
            }
        }
    }

    // Vectors holding vectors
    {
        const size_t size = 1000;
        std::vector<int> a0(1000);
        std::vector<int> a1(1000);
        std::vector<int> a2(1000);
        std::vector<int> a3(1000);
        std::vector<int> a4(1000);

        Vdf_BoxedContainer<int> b0(1000);
        Vdf_BoxedContainer<int> b1(1000);
        Vdf_BoxedContainer<int> b2(1000);
        Vdf_BoxedContainer<int> b3(1000);
        Vdf_BoxedContainer<int> b4(1000);

        srand(0);

        for (size_t i = 0; i < size; ++i)
        {
            int val = rand();
            a0[i] = val;
            a1[i] = val;
            a2[i] = val;
            a3[i] = val;
            a4[i] = val;

            int val2 = rand();
            b0[i] = val2;
            b1[i] = val2;
            b2[i] = val2;
            b3[i] = val2;
            b4[i] = val2;
        }

        // Vdf_BoxedContainer of std::vector
        {
            using VectorType = std::vector<int>;
            using VecOfVec = Vdf_BoxedContainer< VectorType >;

            VecOfVec source(5);
            source[0] = a0;
            source[1] = a1;
            source[2] = a2;
            source[3] = a3;
            source[4] = a4;

            VdfTypedVector<VectorType> vec;
            vec.Set(source);

            // 7a: Iterate directly
            {
                VdfVector::ReadAccessor<VectorType> accessor =
                    vec.GetReadAccessor<VectorType>();

                ASSERT_EQ(accessor.GetNumValues(), source.size());
                TF_AXIOM(accessor.IsBoxed());

                for (size_t i = 0; i < source.size(); ++i)
                {
                    ASSERT_EQ(accessor[i].size(), size);

                    for (size_t j = 0; j < accessor[i].size(); ++j)
                    {
                        ASSERT_EQ(accessor[i][j], a0[j]);
                    }
                }
            }

            {
                VdfVector::ReadAccessor<VectorType> accessor =
                    vec.GetReadAccessor<VectorType>();

                ASSERT_EQ(accessor.GetNumValues(), source.size());
                TF_AXIOM(accessor.IsBoxed());

                const VectorType *data = &accessor[0];

                for (size_t i = 0; i < source.size(); ++i)
                {
                    ASSERT_EQ(data[i].size(), size);

                    for (size_t j = 0; j < data[i].size(); ++j)
                    {
                        ASSERT_EQ(data[i][j], a0[j]);
                    }
                }
            }
        }
    }

    return true;
}

static bool TestCopying()
{
    TRACE_FUNCTION();

    // Here we test copying from various kinds of vectors in light of 
    // subset copying.
    VdfTypedVector<double> denseVector;
    std::vector<double> rawVector;
    const size_t numElements = 1500;
    
    for (size_t i = 0; i < numElements; ++i) {
        rawVector.push_back((double)i);
    }

    SetFromVector(&denseVector, rawVector);

    VdfTypedVector<double> sparseVector;
    VdfMask sparseMask(numElements);

    // Now let's try to copy a subset of the elements from denseVector.  In
    // this case every element except the first and last.
    for (size_t i=1; i<numElements-1; i++) {
        sparseMask.SetIndex(i);
    }
    sparseVector.Copy(denseVector, sparseMask);

    ASSERT_EQ(sparseVector.GetNumStoredElements(), numElements-2);
    ASSERT_EQ(sparseVector.GetSize(), denseVector.GetSize());

    for (VdfMask::iterator it = sparseMask.begin();
         !it.IsAtEnd(); ++it)
    {
        VdfVector::ReadAccessor<double> accessor = 
            sparseVector.GetReadAccessor<double>();
        if (accessor[*it] != (double)(*it)) {
            std::cout << "ERROR: Got "  
                      << accessor[*it] << " expected: " 
                      << (double)(*it) << std::endl;
            return false;
        }
    }

    // Copy a single element from a sparse vector.
    VdfTypedVector<double> sparserVector;
    VdfMask singleEntryMask(numElements);
    singleEntryMask.SetIndex(33);

    sparserVector.Copy(sparseVector, singleEntryMask);
    ASSERT_EQ(sparserVector.GetNumStoredElements(), 1);
    ASSERT_EQ(sparserVector.GetSize(), sparseVector.GetSize());

    // Test expected value
    {
        VdfVector::ReadAccessor<double> accessor = 
            sparserVector.GetReadAccessor<double>();
        if (accessor[33] != 33) {
            std::cout << "ERROR: Got "  
                      << accessor[33] << " expected: " 
                      << 33 << std::endl;
            return false;
        }
    }

    // Copy an empty vector
    VdfTypedVector<double> emptyVector;
    VdfTypedVector<double> anotherEmptyVector;
    anotherEmptyVector.Copy(emptyVector, VdfMask(0));
    ASSERT_EQ(anotherEmptyVector.GetNumStoredElements(), 0);
    ASSERT_EQ(anotherEmptyVector.GetSize(), emptyVector.GetSize());

    // Copy nothing from the denseVector
    VdfTypedVector<double> emptySparseVector;
    VdfMask emptyMask(numElements);
    emptySparseVector.Copy(denseVector, emptyMask);
    ASSERT_EQ(emptySparseVector.GetNumStoredElements(), 0);
    ASSERT_EQ(emptySparseVector.GetSize(), denseVector.GetSize());
    if (emptySparseVector.GetSize() != denseVector.GetSize()) {
        std::cout << "ERROR: empty sparse vector size is " 
                  << emptySparseVector.GetSize() 
                  << " expected " << denseVector.GetSize()
                  << std::endl;
        return false;
    }

    // Copy nothing from a sparse Vector
    emptySparseVector.Copy(sparseVector, emptyMask);
    ASSERT_EQ(emptySparseVector.GetNumStoredElements(), 0);
    ASSERT_EQ(emptySparseVector.GetSize(), sparseVector.GetSize());
    if (emptySparseVector.GetSize() != denseVector.GetSize()) {
        std::cout << "ERROR: empty sparse vector size is " 
                  << emptySparseVector.GetSize() 
                  << " expected " << denseVector.GetSize()
                  << std::endl;
        return false;
    }

    // Copy discontiguous chunks from a dense vector to a
    // compressed sparse vector.
    VdfTypedVector<double> compressedVector;
    VdfMask discontiguousMask(numElements);
    // Set three groups of ten.
    for (size_t i=0; i<10; ++i)
        discontiguousMask.SetIndex(i);
    for (size_t i=750; i<760; ++i)
        discontiguousMask.SetIndex(i);
    for (size_t i=1400; i<1500; ++i)
        discontiguousMask.SetIndex(i);

    compressedVector.Copy(denseVector, discontiguousMask);
    ASSERT_EQ(compressedVector.GetSize(), discontiguousMask.GetSize());
    TF_AXIOM(compressedVector.GetNumStoredElements()
        == discontiguousMask.GetNumSet());

    // Test expected values
    for (VdfMask::iterator it = discontiguousMask.begin();
         !it.IsAtEnd(); ++it)
    {
        VdfVector::ReadAccessor<double> accessor = 
            compressedVector.GetReadAccessor<double>();
        if (accessor[*it] != (double)(*it)) {
            std::cout << "ERROR: Got "  
                      << accessor[*it] << " expected: " 
                      << (double)(*it) << std::endl;
            return false;
        }
    }

    // Copy discontiguous chunks from a sparse vector to a compressed
    // sparse vector.
    // "sparseVector" already contains all elements except the first, last
    // and middle.  We copy a few scattered elements to make a very sparse
    // compressed vector.
    discontiguousMask = VdfMask(numElements);
    discontiguousMask.SetIndex(1);
    discontiguousMask.SetIndex(20);
    discontiguousMask.SetIndex(40);
    discontiguousMask.SetIndex(60);
    discontiguousMask.SetIndex(70);
    discontiguousMask.SetIndex(80);
    discontiguousMask.SetIndex(1498);
    compressedVector.Copy(sparseVector, discontiguousMask);
    ASSERT_EQ(compressedVector.GetSize(), discontiguousMask.GetSize());
    TF_AXIOM(compressedVector.GetNumStoredElements()
        == discontiguousMask.GetNumSet());
    // Test expected values
    for (VdfMask::iterator it = discontiguousMask.begin();
         !it.IsAtEnd(); ++it)
    {
        VdfVector::ReadAccessor<double> accessor = 
            compressedVector.GetReadAccessor<double>();
        if (accessor[*it] != (double)(*it)) {
            std::cout << "ERROR: Got "  
                      << accessor[*it] << " expected: " 
                      << (double)(*it) << std::endl;
            return false;
        }
    }

    // Now copy a subset of the discontiguous chunks from a compressed sparse
    // vector to another sparse vector, making sure the result it compact 
    // as expected.
    VdfTypedVector<double> anotherCompressedVector;
    discontiguousMask = VdfMask(numElements);
    discontiguousMask.SetIndex(20);
    discontiguousMask.SetIndex(40);
    discontiguousMask.SetIndex(60);
    discontiguousMask.SetIndex(1498);
    anotherCompressedVector.Copy(compressedVector, discontiguousMask);
    ASSERT_EQ(anotherCompressedVector.GetSize(), discontiguousMask.GetSize());
    TF_AXIOM(anotherCompressedVector.GetNumStoredElements() == 
        discontiguousMask.GetNumSet());

    // Test expected values
    for (VdfMask::iterator it = discontiguousMask.begin();
         !it.IsAtEnd(); ++it)
    {
        VdfVector::ReadAccessor<double> accessor = 
            anotherCompressedVector.GetReadAccessor<double>();
        if (accessor[*it] != (double)(*it)) {
            std::cout << "ERROR: Got "  
                      << accessor[*it] << " expected: " 
                      << (double)(*it) << std::endl;
            return false;
        }
    }

    return true;
}

struct _TestMoveStruct {
    _TestMoveStruct() : str(NULL) {}

    explicit _TestMoveStruct(const char* cstr) {
        str = new char[strlen(cstr) + 1];
        strcpy(str, cstr);
    }

    _TestMoveStruct(const _TestMoveStruct &) {
        TF_FATAL_ERROR("Attempted to copy move-only type");
    }

    _TestMoveStruct& operator=(const _TestMoveStruct &) {
        TF_FATAL_ERROR("Attempted to copy move-only type");
        return *this;
    }

    _TestMoveStruct(_TestMoveStruct &&rhs) {
        TF_AXIOM(this != &rhs);
        str = rhs.str;
        rhs.str = nullptr;
        ++_TestMoveStruct::numMoved;
    }

    _TestMoveStruct& operator=(_TestMoveStruct &&rhs) {
        TF_AXIOM(this != &rhs);
        str = rhs.str;
        rhs.str = nullptr;
        ++_TestMoveStruct::numMoved;
        return *this;
    }

    ~_TestMoveStruct() {
        delete [] str;
    }

    char *str;
    static size_t numMoved;
};

size_t _TestMoveStruct::numMoved = 0;

static bool TestMoving()
{
    TRACE_FUNCTION();

    VdfTypedVector<_TestMoveStruct> v1;

    TF_AXIOM(v1.IsEmpty());
    ASSERT_EQ(v1.GetSize(), 0);
    ASSERT_EQ(_TestMoveStruct::numMoved, 0);

    _TestMoveStruct test1("TestMoving 1");
    v1.Set(std::move(test1));

    TF_AXIOM(!v1.IsEmpty());
    ASSERT_EQ(v1.GetSize(), 1);
    ASSERT_EQ(_TestMoveStruct::numMoved, 1);

    VdfVector::ReadAccessor<_TestMoveStruct> a1 =
        v1.GetReadAccessor<_TestMoveStruct>();
    TF_AXIOM(a1[0].str);
    ASSERT_EQ(strcmp(a1[0].str, "TestMoving 1"), 0);
    ASSERT_EQ(test1.str, NULL);


    _TestMoveStruct test2("TestMoving 2");

    v1.Set(std::move(test2));

    TF_AXIOM(!v1.IsEmpty());
    ASSERT_EQ(v1.GetSize(), 1);
    ASSERT_EQ(_TestMoveStruct::numMoved, 2);

    VdfVector::ReadAccessor<_TestMoveStruct> a2 =
        v1.GetReadAccessor<_TestMoveStruct>();
    TF_AXIOM(a2[0].str);
    ASSERT_EQ(strcmp(a2[0].str, "TestMoving 2"), 0);
    ASSERT_EQ(test2.str, NULL);


    Vdf_BoxedContainer<int> test3(100);
    for (size_t i = 0; i < test3.size(); ++i) {
        test3[i] = 1;
    }
    ASSERT_EQ(test3.size(), 100);

    VdfTypedVector<int> v2;
    v2.Set(std::move(test3));

    TF_AXIOM(!v2.IsEmpty());
    ASSERT_EQ(v2.GetSize(), 1);

    VdfVector::ReadAccessor<int> a3 = v2.GetReadAccessor<int>();
    ASSERT_EQ(a3.GetNumValues(), 100);
    for (size_t i = 0; i < a3.GetNumValues(); ++i) {
        ASSERT_EQ(a3[i], 1);
    }

    ASSERT_EQ(test3.size(), 0);

    return true;
}

static bool TestSet()
{
    TRACE_FUNCTION();

    std::vector<double> empty;
    VdfMask zeroMask(0);
    VdfTypedVector<double> emptyVector;
    SetFromVector(&emptyVector, empty, zeroMask);

    if (emptyVector.GetSize() != 0) {
        std::cout << "ERROR: expected emptyVector to have a size of 0 "
                  << "instead of " << emptyVector.GetSize() << std::endl;
        return false;
    }

    const int numElements = 10;
    VdfMask allOnes = VdfMask::AllOnes(numElements);
    VdfMask allZeros(numElements);

    std::vector<double> full(numElements, 0.1);
    VdfTypedVector<double> fullVector;
    SetFromVector(&fullVector, full, allOnes);

    if (fullVector.GetSize() != numElements) {
        std::cout << "ERROR: expected fullVector to have a size of "
                  << numElements 
                  << " instead of " << fullVector.GetSize() << std::endl;
        return false;
    }

    SetFromVector(&fullVector, full, allZeros);
    if (fullVector.GetSize() != numElements) {
        std::cout << "ERROR: expected fullVector to have a size of "
                  << numElements 
                  << " instead of " << fullVector.GetSize() << std::endl;
        return false;
    }

    return true;
}

// Note: default and copy ctor required.
struct A
{
    A() = default;
    A(int a, int b) : _m(a*b)    {}
    A(const A &rhs) : _m(rhs._m) {}
    ~A()                         { dtorCalled++; }

    int _m;

    static int dtorCalled;
};

std::ostream& operator<<(std::ostream& out, const A &a) {
    return out << a._m;
}

int A::dtorCalled = 0;

static bool TestVectorCtorDtorCalling()
{
    TRACE_FUNCTION();

    // Dense vector    
    if (true)
    {
        VdfTypedVector<A> v;
    
        TF_AXIOM(v.IsEmpty());
        ASSERT_EQ(v.GetSize(), 0);
    
        // Note: This call ~A() four times (one temp., three vector elements).
        SetFromVector(&v, std::vector<A>(3, A(2, 7))); 
        ASSERT_EQ(v.GetSize(), 3);
        TF_AXIOM(!v.IsEmpty());
    
        ASSERT_EQ(A::dtorCalled, 4);
        A::dtorCalled = 0;

        for(size_t i=0; i<v.GetSize(); i++) {
            printf("%zu: %d\n", i, v.GetReadAccessor<A>()[i]._m);
            ASSERT_EQ(v.GetReadAccessor<A>()[i]._m, 14);
        }
    }

    // Make sure the dtor has been called during dtor of v.
    ASSERT_EQ(A::dtorCalled, 3);
    A::dtorCalled = 0;

    // Sparse vector    
    if (true)
    {
        VdfTypedVector<A> v;
    
        TF_AXIOM(v.IsEmpty());
        ASSERT_EQ(v.GetSize(), 0);
    
        VdfMask mask(3);
        mask.SetIndex(1);

        // Note: This call ~A() four times (one temp., three vector elements).
        SetFromVector(&v, std::vector<A>(3, A(2, 7)), mask);
        ASSERT_EQ(v.GetSize(), 3);
        TF_AXIOM(!v.IsEmpty());

        ASSERT_EQ(A::dtorCalled, 4);
        A::dtorCalled = 0;

        for(size_t i=0; i<v.GetSize(); i++) {
            if (mask.GetBits().IsSet(i)) {
                printf("%zu: %d\n", i, v.GetReadAccessor<A>()[i]._m);
                ASSERT_EQ(v.GetReadAccessor<A>()[i]._m, 14);
            }
        }
    }

    // Make sure the dtor has been called during dtor of v.
    ASSERT_EQ(A::dtorCalled, 1);

    return true;
}

static bool TestVectorResize()
{
    TRACE_FUNCTION();

    VdfTypedVector<string> v;

    TF_AXIOM(v.IsEmpty());
    ASSERT_EQ(v.GetSize(), 0);

    v.Resize<string>(VdfMask::AllOnes(10).GetBits());
    ASSERT_EQ(v.GetSize(), 10);
    TF_AXIOM(!v.IsEmpty());

    for(size_t i=0; i<v.GetSize(); i++) {
        printf("%zu: %s\n", i, v.GetReadAccessor<string>()[i].c_str());
        ASSERT_EQ(v.GetReadAccessor<string>()[i], "");
    }

    VdfMask mask(10);
    mask.SetIndex(2);
    mask.SetIndex(7);

    v.Resize<string>(mask.GetBits());
    ASSERT_EQ(v.GetSize(), 10);
    // The resulting vector is sparse, rather than compressed!
    ASSERT_EQ(v.GetNumStoredElements(), 6);
    TF_AXIOM(!v.IsEmpty());

    for(size_t i=0; i<v.GetSize(); i++) {
        if (mask.GetBits().IsSet(i)) {
            printf("%zu: %s\n", i, v.GetReadAccessor<string>()[i].c_str());
            ASSERT_EQ(v.GetReadAccessor<string>()[i], "");
        }
    }

    // Test resize works when using boxed containers.

    VdfTypedVector<int> vec;
    TF_AXIOM(vec.IsEmpty());
    ASSERT_EQ(vec.GetSize(), 0);
    ASSERT_EQ(vec.GetReadAccessor<int>().GetNumValues(), 0);
    vec.Set(Vdf_BoxedContainer<int>());
    TF_AXIOM(!vec.IsEmpty());
    ASSERT_EQ(vec.GetSize(), 1);
    ASSERT_EQ(vec.GetReadAccessor<int>().GetNumValues(), 0);
    WithNoExpectedErrors([&]() { vec.Resize<int>(10); });
    TF_AXIOM(!vec.IsEmpty());
    ASSERT_EQ(vec.GetSize(), 10);
    ASSERT_EQ(vec.GetReadAccessor<int>().GetNumValues(), 10);

    VdfTypedVector<int> vec2;
    vec2.Set(Vdf_BoxedContainer<int>());
    WithNoExpectedErrors([&]() {
        vec.Resize<int>(VdfMask::AllOnes(10).GetBits());
    });
    TF_AXIOM(!vec.IsEmpty());
    ASSERT_EQ(vec.GetSize(), 10);
    ASSERT_EQ(vec.GetReadAccessor<int>().GetNumValues(), 10);

    return true;
}

static bool TestVectorMerge()
{
    TRACE_FUNCTION();

    // Seed the random number generator
    srand(time(NULL));

    // Create the soure vector and fill it with integer data 0 - 9
    VdfTypedVector<int> source;
    std::vector<int> sourceData;
    for (int i = 0; i <= 9; ++i) {
        sourceData.push_back(i);
    }
    SetFromVector(&source, sourceData);

    // Create the destination vector and fill it with integer data 10 - 19
    VdfTypedVector<int> dest;
    std::vector<int> destData;
    for (int i = 10; i <= 19; ++i) {
        destData.push_back(i);
    }
    SetFromVector(&dest, destData);

    // Now, merge some source data into the destination data
    // Mask contains: 0-2-4-6-8
    VdfMask mask(10);
    for (size_t i = 0; i < 10; i += 2) {
        mask.SetIndex(i);
    }
    dest.Merge(source, mask);

    // Verify the new contents of dest
    for (int i = 0; i < 10; ++i) {
        if (mask.IsSet(i)) {
            ASSERT_EQ(dest.GetReadAccessor<int>()[i], i);
        } else {
            ASSERT_EQ(dest.GetReadAccessor<int>()[i], i + 10);
        }
    }

    // Invert the mask
    VdfMask invMask(mask);
    invMask ^= VdfMask::AllOnes(10);

    // Merge the rest of the data
    dest.Merge(source, invMask);

    // Verify
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(dest.GetReadAccessor<int>()[i], i);
    }


    // Create a source and destination vector containing a single element and
    // attempt to merge them.
    VdfTypedVector<int> singleSource;
    singleSource.Set(1);

    VdfTypedVector<int> singleDest;
    singleDest.Set(2);

    // Merge nothing
    singleDest.Merge(singleSource, VdfMask(1));

    // Verify
    ASSERT_EQ(singleDest.GetReadAccessor<int>()[0], 2);

    // Merge
    singleDest.Merge(singleSource, VdfMask::AllOnes(1));
    ASSERT_EQ(singleDest.GetReadAccessor<int>()[0], 1);


    // Self-merging should be handled gracefully (nothing to do)
    WithExpectedErrors([&]() {
        singleDest.Merge(singleDest, VdfMask::AllOnes(1));
    });
    ASSERT_EQ(singleDest.GetReadAccessor<int>()[0], 1);


    // Create an empty source and destination vector and attempt to merge them.
    // This should be handled gracefully.
    VdfTypedVector<int> emptySource;
    VdfTypedVector<int> emptyDest;
    emptyDest.Merge(emptySource, VdfMask(0));


    // Try merging into a sparse vector
    VdfTypedVector<int> sparseSource;
    VdfMask sparseMask(10);
    sparseMask.SetIndex(2);
    sparseMask.SetIndex(3);
    sparseMask.SetIndex(4);

    int sourceRand = rand();
    std::vector<int> sparseSourceData;
    sparseSourceData.push_back(rand());
    sparseSourceData.push_back(rand());
    sparseSourceData.push_back(sourceRand + 0);
    sparseSourceData.push_back(sourceRand + 1);
    sparseSourceData.push_back(sourceRand + 2);
    sparseSourceData.push_back(rand());
    sparseSourceData.push_back(rand());
    sparseSourceData.push_back(rand());
    sparseSourceData.push_back(rand());
    sparseSourceData.push_back(rand());
    SetFromVector(&sparseSource, sparseSourceData, sparseMask);
    TF_AXIOM(sparseSource.GetSize() > sparseSource.GetNumStoredElements());

    VdfTypedVector<int> sparseDest;
    VdfMask invSparseMask(10);
    invSparseMask.SetIndex(6);
    invSparseMask.SetIndex(7);
    invSparseMask.SetIndex(8);

    int destRand = rand();
    std::vector<int> sparseDestData;
    sparseDestData.push_back(rand());
    sparseDestData.push_back(rand());
    sparseDestData.push_back(rand());
    sparseDestData.push_back(rand());
    sparseDestData.push_back(rand());
    sparseDestData.push_back(rand());
    sparseDestData.push_back(destRand + 0);
    sparseDestData.push_back(destRand + 1);
    sparseDestData.push_back(destRand + 2);
    sparseDestData.push_back(rand());
    SetFromVector(&sparseDest, sparseDestData, invSparseMask);
    TF_AXIOM(sparseDest.GetSize() > sparseDest.GetNumStoredElements());

    // This operation will force an expansion of the data storage provided
    // by the sparse destination vector.
    sparseDest.Merge(sparseSource, sparseMask);

    // Verify
    int sourceI = 0;
    int destI = 0;
    ASSERT_EQ(sparseDest.GetNumStoredElements(), 7);
    ASSERT_EQ(sparseDest.GetSize(), 10);
    for (size_t i = 0; i < sparseMask.GetSize(); ++i) {
        if (sparseMask.IsSet(i)) {
            TF_AXIOM(sparseDest.GetReadAccessor<int>()[i] ==
                (sourceRand + sourceI));
            ++sourceI;
        } else if(invSparseMask.IsSet(i)) {
            TF_AXIOM(sparseDest.GetReadAccessor<int>()[i] == 
                (destRand + destI));
            ++destI;
        }
    }

    // Try merging a subset into a bigger sparse vector. No re-allocation will
    // be required.
    sparseMask = VdfMask(6);
    sparseMask.SetIndex(0);
    sparseMask.SetIndex(1);
    sparseMask.SetIndex(2);
    SetFromVector(&sparseSource, std::vector<int>(6, 1), sparseMask);
    TF_AXIOM(sparseSource.GetSize() > sparseSource.GetNumStoredElements());

    VdfTypedVector<int> sparseDest2;
    VdfMask sparseMask2(6);
    sparseMask2.SetIndex(0);
    sparseMask2.SetIndex(1);
    sparseMask2.SetIndex(2);
    sparseMask2.SetIndex(3);
    SetFromVector(&sparseDest2, std::vector<int>(6, 3), sparseMask2);
    TF_AXIOM(sparseDest2.GetSize() > sparseDest2.GetNumStoredElements());

    // Merge
    sparseDest2.Merge(sparseSource, sparseMask);

    // Verify
    ASSERT_EQ(sparseDest2.GetNumStoredElements(), 4);
    ASSERT_EQ(sparseDest2.GetNumStoredElements(), sparseMask2.GetNumSet());
    ASSERT_EQ(sparseDest2.GetSize(), 6);
    for (size_t i = 0; i < sparseMask2.GetNumSet(); ++i) {
        if (sparseMask.IsSet(i)) {
            ASSERT_EQ(sparseDest2.GetReadAccessor<int>()[i], 1);
        } else {
            ASSERT_EQ(sparseDest2.GetReadAccessor<int>()[i], 3);
        }
    }

    // Make the destination vector dense
    SetFromVector(&sparseDest, std::vector<int>(6, 2));

    // Test merging from a sparse source
    sparseDest.Merge(sparseSource, sparseMask);

    // Verify
    for (size_t i = 0; i < 6; ++i) {
        if (sparseMask.IsSet(i)) {
            ASSERT_EQ(sparseDest.GetReadAccessor<int>()[i], 1);
        } else {
            ASSERT_EQ(sparseDest.GetReadAccessor<int>()[i], 2);
        }
    }


    // Try merging into a compressed vector. This will promote the compressed
    // vector to a sparse vector.
    const size_t numElements = 1500;
    VdfTypedVector<int> compressedSource;
    VdfMask compressedSourceMask(numElements);
    compressedSourceMask.SetIndex(1);
    compressedSourceMask.SetIndex(1499);
    SetFromVector(&compressedSource, 
        std::vector<int>(numElements, 1), compressedSourceMask);
    TF_AXIOM(
        compressedSource.GetSize() > compressedSource.GetNumStoredElements());
    TF_AXIOM(Vdf_VectorData::ShouldStoreCompressed(
        compressedSourceMask.GetBits(), sizeof(int)));

    VdfTypedVector<int> compressedDest;
    VdfMask compressedDestMask(numElements);
    compressedDestMask.SetIndex(2);
    compressedDestMask.SetIndex(1498);
    SetFromVector(&compressedDest, 
        std::vector<int>(numElements, 2), compressedDestMask);
    TF_AXIOM(
        compressedDest.GetSize() > compressedDest.GetNumStoredElements());
    TF_AXIOM(Vdf_VectorData::ShouldStoreCompressed(
        compressedDestMask.GetBits(), sizeof(int)));

    // Merge
    compressedDest.Merge(compressedSource, compressedSourceMask);

    // Verify
    ASSERT_EQ(compressedDest.GetNumStoredElements(), 1499);
    ASSERT_EQ(compressedDest.GetSize(), 1500);
    VdfMask::iterator it;
    for (it = compressedSourceMask.begin(); !it.IsAtEnd(); ++it) {
        ASSERT_EQ(compressedDest.GetReadAccessor<int>()[*it], 1);
    }
    for (it = compressedDestMask.begin(); !it.IsAtEnd(); ++it) {
        ASSERT_EQ(compressedDest.GetReadAccessor<int>()[*it], 2);
    }

    // Make the destination vector dense
    SetFromVector(&compressedDest, std::vector<int>(numElements, 2));

    // Test merging from a compressed source
    compressedDest.Merge(compressedSource, compressedSourceMask);

    // Verify
    for (size_t i = 0; i < 6; ++i) {
        if (compressedSourceMask.IsSet(i)) {
            ASSERT_EQ(compressedDest.GetReadAccessor<int>()[i], 1);
        } else {
            ASSERT_EQ(compressedDest.GetReadAccessor<int>()[i], 2);
        }
    }

    return true;
}

// Used below to test printing a type for which there's no registered debug
// print function.
//
// The type is defined outside the test function to avoid name mangling
// differences on different platforms.
struct _UserDefinedType {};

static bool TestDebugPrint()
{
    TRACE_FUNCTION();

    VdfTypedVector<int> empty;

    {
        std::ostringstream os;
        os << empty.GetDebugPrintable(VdfMask());
        ASSERT_EQ(os.str(), "");
    }


    VdfTypedVector<int> single(13);

    {
        std::ostringstream os;
        os << single.GetDebugPrintable(VdfMask::AllOnes(1));
        ASSERT_EQ(os.str(), "0: 13\n");
    }

    {
        std::ostringstream os;
        os << single.GetDebugPrintable(VdfMask::AllZeros(1));
        // ASSERT_EQ(os.str(), "");
    }


    std::vector<int> data = { 0, 1, 2, 3, 4, 5 };
    VdfTypedVector<int> dense;
    SetFromVector(&dense, data, VdfMask::AllOnes(data.size()));

    {
        std::ostringstream os;
        os << dense.GetDebugPrintable(VdfMask::AllOnes(data.size()));
        ASSERT_EQ(os.str(), "0: 0\n1: 1\n2: 2\n3: 3\n4: 4\n5: 5\n");
    }

    {
        std::ostringstream os;
        os << dense.GetDebugPrintable(VdfMask::AllZeros(data.size()));
        ASSERT_EQ(os.str(), "");
    }

    {
        VdfMask mask(data.size());
        mask.SetIndex(1);
        mask.SetIndex(4);

        std::ostringstream os;
        os << dense.GetDebugPrintable(mask);
        ASSERT_EQ(os.str(), "1: 1\n4: 4\n");
    }


    VdfMask sparseMask(data.size());
    sparseMask.SetIndex(1);
    sparseMask.SetIndex(2);
    sparseMask.SetIndex(3);
    sparseMask.SetIndex(4);

    VdfTypedVector<int> sparse;
    SetFromVector(&sparse, data, sparseMask);

    {
        std::ostringstream os;
        os << sparse.GetDebugPrintable(sparseMask);
        ASSERT_EQ(os.str(), "1: 1\n2: 2\n3: 3\n4: 4\n");
    }

    {
        std::ostringstream os;
        os << sparse.GetDebugPrintable(VdfMask::AllZeros(data.size()));
        ASSERT_EQ(os.str(), "");
    }

    {
        sparseMask.ClearIndex(1);
        sparseMask.ClearIndex(4);

        std::ostringstream os;
        os << sparse.GetDebugPrintable(sparseMask);
        ASSERT_EQ(os.str(), "2: 2\n3: 3\n");
    }

    {
        _UserDefinedType instance;
        VdfTypedVector<_UserDefinedType> vec(instance);

        std::ostringstream os;
        os << vec.GetDebugPrintable(VdfMask::AllOnes(1));
        ASSERT_EQ(os.str(), "(_UserDefinedType)\n");
    }

    return true;
}

static bool
TestConstructWithSize()
{
    {
        const auto empty = VdfTypedVector<int>::CreateWithSize(0);
        TF_AXIOM(empty.Holds<int>());
        TF_AXIOM(empty.IsEmpty());
    }

    {
        const auto single = VdfTypedVector<int>::CreateWithSize(1);
        TF_AXIOM(single.Holds<int>());
        ASSERT_EQ(single.GetSize(), 1);
    }

    {
        const auto many = VdfTypedVector<int>::CreateWithSize(3);
        TF_AXIOM(many.Holds<int>());
        ASSERT_EQ(many.GetSize(), 3);
    }

    return true;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// The list of tests to run.
typedef bool(*TestFunction)(void);

struct Tests
{
    TestFunction func;
    const char  *name;
};

static Tests tests[] =
{
    { TestSize,                  "TestSize"                  },
    { TestEstimateSize,          "TestEstimateSize"          },          
    { TestSingleElement,         "TestSingleElement"         },
    { TestDenseVector,           "TestDenseVector"           },
    { TestSparseVector,          "TestSparseVector"          },
    { TestCompressedVector,      "TestCompressedVector"      },
    { TestSharedVector,          "TestSharedVector"          },
    { TestAssignmentOperator,    "TestAssignmentOperator"    },
    { TestTyping,                "TestTyping"                },
    { TestBoxedIteration,        "TestBoxedIteration"        },
    { TestCopying,               "TestCopying"               },
    { TestMoving,                "TestMoving"                },
    { TestSet,                   "TestSet"                   },
    { TestVectorCtorDtorCalling, "TestVectorCtorDtorCalling" },
    { TestVectorResize,          "TestVectorResize"          },
    { TestVectorMerge,           "TestVectorMerge"           },
    { TestDebugPrint,            "TestDebugPrint"            },
    { TestConstructWithSize,     "TestConstructWithSize"     },

    NULL
};


int 
main(int argc, char **argv) 
{
    int res = 0;

    TraceCollector::GetInstance().SetEnabled(true);

    {
        TRACE_SCOPE("main");

        // This test tests very basic functionality of VdfVector.

        // Run through all the registered tests, and if any of them fail
        // fail the whole test.

        for(int i = 0; tests[i].func; ++i)
        {
            printf("*** %s\n", tests[i].name);

            if (!tests[i].func())
                printf("> failed...\n"),
                res = -1;
            else
                printf("> ok...\n");
        }
    }

    TraceReporter::GetGlobalReporter()->Report(std::cout);

    return res;
}
