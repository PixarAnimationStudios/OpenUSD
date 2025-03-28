//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/delegatedCountPtr.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/regTest.h"


PXR_NAMESPACE_USING_DIRECTIVE

// Test class which stores its own reference count.
struct RefCountedValue {
    int value{5};
    mutable int count{0};
    RefCountedValue() = default;
    explicit RefCountedValue(int v) : value(v) {}
    RefCountedValue(int v, int c) : value(v), count(c) {}
};

struct DerivedRefCountedValue : public RefCountedValue {};

void TfDelegatedCountIncrement(const RefCountedValue* ptr) {
    ++(ptr->count);
}

void TfDelegatedCountDecrement(const RefCountedValue* ptr) noexcept {
    if (--(ptr->count); ptr->count == 0) {
        delete ptr;
    }
}

using TfTestDelegatedCountPtr = TfDelegatedCountPtr<RefCountedValue>;
using TfTestDelegatedCountConstPtr =
    TfDelegatedCountPtr<const RefCountedValue>;

static bool
TestDefault()
{
    TF_AXIOM(TfTestDelegatedCountPtr{}.get() == nullptr);
    TF_AXIOM(!TfTestDelegatedCountPtr{});
    return true;
}

static bool
TestIncrementTag()
{
    RefCountedValue stackOwnedValue{10, 1};
    TF_AXIOM(stackOwnedValue.count == 1);
    TfTestDelegatedCountPtr adopted{
        TfDelegatedCountIncrementTag, &stackOwnedValue};
    TF_AXIOM(stackOwnedValue.count == 2);
    return true;
}

static bool
TestDoNotIncrementTag()
{
    // Set the reference count to 2 to account for `adopted`'s
    // usage of the DoNotIncrementTag.
    RefCountedValue stackOwnedValue{10, 2};
    TF_AXIOM(stackOwnedValue.count == 2);
    TfTestDelegatedCountPtr adopted{
        TfDelegatedCountDoNotIncrementTag, &stackOwnedValue};
    TF_AXIOM(stackOwnedValue.count == 2);
    return true;
}

static bool
TestScopedDecrement()
{
    RefCountedValue stackOwnedValue{7, 2};
    TF_AXIOM(stackOwnedValue.count == 2);
    {
        TfTestDelegatedCountPtr adopted{
            TfDelegatedCountDoNotIncrementTag, &stackOwnedValue};
        TF_AXIOM(stackOwnedValue.count == 2);
        TF_AXIOM(adopted.get() == &stackOwnedValue);
    }
    TF_AXIOM(stackOwnedValue.count == 1);
    return true;
}

static bool
TestMake()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(12);
    TF_AXIOM(made);
    TF_AXIOM(made->count == 1);
    TF_AXIOM(made->value == 12);
    return true;
}

static bool
TestEquality()
{
    RefCountedValue stackOwnedValue{10, 1};
    TfTestDelegatedCountPtr adopted{
        TfDelegatedCountIncrementTag, &stackOwnedValue};
    TfTestDelegatedCountPtr another{
        TfDelegatedCountIncrementTag, &stackOwnedValue};
    TF_AXIOM(adopted == another);
    TF_AXIOM(adopted != TfTestDelegatedCountPtr{});
    TF_AXIOM(TfTestDelegatedCountPtr{} == TfTestDelegatedCountPtr{});

    // Value equivalance doesn't imply address equivalence
    TF_AXIOM(TfMakeDelegatedCountPtr<RefCountedValue>(12) !=
             TfMakeDelegatedCountPtr<RefCountedValue>(12));
    return true;
}

static bool
TestPointerOperators()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(15);
    TF_AXIOM(made.get()->value == 15);
    TF_AXIOM(made->value == made.get()->value);
    TF_AXIOM((*made).value == made.get()->value);
    return true;
}

static bool
TestNullAssignment()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(12);
    TF_AXIOM(made->count == 1);
    auto copy = made;
    TF_AXIOM(made->count == 2);
    copy = nullptr;
    TF_AXIOM(!copy);
    TF_AXIOM(made);
    TF_AXIOM(made->count == 1);
    return true;
}

static bool
TestMoving()
{
    // Exercise moving back and forth using both constructor
    // and assignment operator
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(12);
    TfTestDelegatedCountPtr moved{std::move(made)};
    TF_AXIOM(!made);
    TF_AXIOM(made.get() == nullptr);
    TF_AXIOM(moved);
    TF_AXIOM(moved->value == 12);
    TF_AXIOM(moved->count == 1);

    made = std::move(moved);
    TF_AXIOM(!moved);
    TF_AXIOM(moved.get() == nullptr);
    TF_AXIOM(made);
    TF_AXIOM(made->value == 12);
    TF_AXIOM(made->count == 1);
    return true;
}

static bool
TestMovingSelf()
{
    // Validate that self move-assignment leaves `adopted` in a valid
    // state.
    RefCountedValue stackOwnedValue{7, 1};
    TfTestDelegatedCountPtr adopted{
        TfDelegatedCountIncrementTag, &stackOwnedValue};
    TF_AXIOM(adopted);
    TF_AXIOM(stackOwnedValue.count == 2);
    adopted = std::move(adopted);
    TF_AXIOM(adopted.get() == nullptr);
    TF_AXIOM(stackOwnedValue.count == 1);
    return true;
}

static bool
TestMovingSameHeldPointer()
{
    // Validate that move assignment from two distinct TfDelegatedCountPtr
    // values that hold the same pointer is successful.
    RefCountedValue stackOwnedValue{7, 1};
    TfTestDelegatedCountPtr adopted{
        TfDelegatedCountIncrementTag, &stackOwnedValue};
    TfTestDelegatedCountPtr another{
        TfDelegatedCountIncrementTag, &stackOwnedValue};
    TF_AXIOM(stackOwnedValue.count == 3);
    TF_AXIOM(another == adopted);
    adopted = std::move(another);
    TF_AXIOM(!another);
    TF_AXIOM(adopted);
    TF_AXIOM(stackOwnedValue.count == 2);
    return true;
}

static bool
TestCopyAssignment()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(85);
    TF_AXIOM(made->count == 1);

    TfTestDelegatedCountPtr copied;
    copied = made;
    TF_AXIOM(made->count == 2);
    TF_AXIOM(copied == made);
    return true;
}

static bool
TestCopyConstructor()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(87);
    TF_AXIOM(made->count == 1);

    TfTestDelegatedCountPtr copied{made};
    TF_AXIOM(made->count == 2);
    TF_AXIOM(copied == made);
    return true;
}

static bool
TestCopySelfAssignment()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(87);
    TF_AXIOM(made);
    TF_AXIOM(made->count == 1);
    made = made;
    TF_AXIOM(made->count == 1);
    TF_AXIOM(made);
    return true;
}

static bool
TestCopySameHeldPointer()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(86);
    TfTestDelegatedCountPtr copied{made};
    TF_AXIOM(copied == made);
    TF_AXIOM(copied->count == 2);
    copied = made;
    TF_AXIOM(copied == made);
    TF_AXIOM(copied->count == 2);
    return true;
}

static bool
TestSwap()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(16);
    auto copy = made;
    auto another = TfMakeDelegatedCountPtr<RefCountedValue>(12);
    TF_AXIOM(made->count == 2);
    TF_AXIOM(made->value == 16);
    TF_AXIOM(another->count == 1);
    TF_AXIOM(another->value == 12);
    made.swap(another);
    TF_AXIOM(copy == another);
    TF_AXIOM(copy != made);
    TF_AXIOM(another->count == 2);
    TF_AXIOM(another->value == 16);
    TF_AXIOM(made->count == 1);
    TF_AXIOM(made->value == 12);
    return true;
}

static bool
TestConstConversion()
{
    auto made = TfMakeDelegatedCountPtr<RefCountedValue>(20);
    TfTestDelegatedCountConstPtr constCopy{made};
    TfTestDelegatedCountConstPtr anotherConstCopy;
    anotherConstCopy = made;
    TF_AXIOM(made == constCopy);
    TF_AXIOM(made == anotherConstCopy);
    TF_AXIOM(made->count == 3);
    return true;
}

static bool
TestAssignDerived()
{
    const auto derived = TfMakeDelegatedCountPtr<DerivedRefCountedValue>();
    TfDelegatedCountPtr<RefCountedValue> base;
    base = derived;
    TF_AXIOM(derived->count == 2);
    TF_AXIOM(base->count == 2);
    TF_AXIOM(derived == base);
    return true;
}

static bool
TestInitializeDerived()
{
    const auto derived = TfMakeDelegatedCountPtr<DerivedRefCountedValue>();
    TfDelegatedCountPtr<RefCountedValue> base{derived};
    TF_AXIOM(derived->count == 2);
    TF_AXIOM(base->count == 2);
    TF_AXIOM(derived == base);
    return true;
}

static bool
Test_TfDelegatedCountPtr() {
    return TestDefault() &&
           TestIncrementTag() &&
           TestDoNotIncrementTag() &&
           TestScopedDecrement() &&
           TestMake() &&
           TestEquality() &&
           TestPointerOperators() &&
           TestNullAssignment() &&
           TestMoving() &&
           TestMovingSelf() &&
           TestMovingSameHeldPointer() &&
           TestCopyAssignment() &&
           TestCopyConstructor() &&
           TestCopySelfAssignment() &&
           TestCopySameHeldPointer() &&
           TestSwap() &&
           TestConstConversion() &&
           TestInitializeDerived() &&
           TestAssignDerived();
}

TF_ADD_REGTEST(TfDelegatedCountPtr);