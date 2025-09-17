//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticLite.h"

#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"

#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
// Tracks the number of invocations to Base's special member functions.
struct Stats
{
    int numCtors = 0;
    int numCopys = 0;
    int numMoves = 0;
    int numDtors = 0;

    void Reset() { *this = Stats(); }
};

// Example class that derives from EsfFixedSizePolymorphicBase
class Base : public EsfFixedSizePolymorphicBase
{
public:
    Base() = default;

    Base(Stats &stats) : _stats(&stats)
    {
        _stats->numCtors++;
    }

    Base(const Base &other) : _stats(other._stats)
    {
        if (_stats) {
            _stats->numCopys++;
        }
    }

    Base(Base &&other) noexcept : _stats(other._stats)
    {
        if (_stats) {
            _stats->numMoves++;
        }
    }

    ~Base() override
    {
        if (_stats) {
            _stats->numDtors++;
        }
    }

    virtual std::string GetDescription() const = 0;

    // Base types are not required to implement the assignment operator.
    Base &operator=(const Base&) = delete;
    Base &operator=(Base&&) = delete;

private:
    Stats *_stats = nullptr;
};

class Derived1 : public Base
{
public:
    using Base::Base;
    std::string GetDescription() const override { return "Derived1"; }
};

class Derived2 : public Base
{
public:
    using Base::Base;
    std::string GetDescription() const override { return "Derived2"; }
};

// This derived class is larger than Base.
class BigDerived : public Base
{
public:
    using Base::Base;
    std::string GetDescription() const override { return "BigDerived"; }
    bool GetFlag() const { return _flag; }

private:
    bool _flag;
};

// This derived class requires additional alignment.
class alignas(sizeof(void *) * 2) OverAlignedDerived : public Base
{
public:
    using Base::Base;
    std::string GetDescription() const override
    {
        return "OverAlignedDerived";
    }
};

// Another base class compatible with EsfFixedSizePolymorphicHolder.
class OtherBase : public EsfFixedSizePolymorphicBase {};

// This derived class inherits from both base classes.
class MultiplyDerived : public Base, public OtherBase {
public:
    using Base::Base;
    std::string GetDescription() const override
    {
        return "MultiplyDerived";
    }
};

// Another base class with a greater alignment requirement.
class alignas(alignof(void *) * 2) OverAlignedBase
    : public EsfFixedSizePolymorphicBase
{};

// Classes derived from OverAlignedBase.
class OverAlignedBaseImpl1 : public OverAlignedBase {};
class OverAlignedBaseImpl2 : public OverAlignedBase {};

constexpr size_t BIG = 128;
using BaseHolder = EsfFixedSizePolymorphicHolder<Base>;
using BigBaseHolder = EsfFixedSizePolymorphicHolder<Base, BIG>;
using OtherBaseHolder = EsfFixedSizePolymorphicHolder<OtherBase, BIG>;
using OverAlignedBaseHolder = EsfFixedSizePolymorphicHolder<OverAlignedBase>;
}

// Tests that EsfFixedSizePolymorphicHolder can be constructed from different
// derived classes.
//
static void
TestConstructor()
{
    BaseHolder h1(std::in_place_type<Derived1>);
    BaseHolder h2(std::in_place_type<Derived2>);
    TF_AXIOM(h1->GetDescription() == "Derived1");
    TF_AXIOM(h2->GetDescription() == "Derived2");
}

// instance when it is destroyed. Tests that EsfFixedSizePolymorphicHolder
// properly destroys the held
//
static void
TestDestructor()
{
    Stats stats;
    {
        BaseHolder h(std::in_place_type<Derived1>, stats);
    }
    TF_AXIOM(stats.numCtors == 1);
    TF_AXIOM(stats.numDtors == 1);
}

// Tests that EsfFixedSizePolymorphicHolder copys the held instance in the copy
// constructor.
//
static void
TestCopyConstructor()
{
    Stats stats;
    BaseHolder h1(std::in_place_type<Derived2>, stats);
    BaseHolder h2(h1);
    TF_AXIOM(h2->GetDescription() == "Derived2");
    TF_AXIOM(stats.numCtors == 1);
    TF_AXIOM(stats.numCopys == 1);
    TF_AXIOM(stats.numMoves == 0);
}

// Tests that EsfFixedSizePolymorphicHolder moves the held instance in the move
// constructor.
//
static void
TestMoveConstructor()
{
    Stats stats;
    BaseHolder h1(std::in_place_type<Derived1>, stats);
    BaseHolder h2(std::move(h1));
    TF_AXIOM(h2->GetDescription() == "Derived1");
    TF_AXIOM(stats.numCtors == 1);
    TF_AXIOM(stats.numCopys == 0);
    TF_AXIOM(stats.numMoves == 1);
}

// Tests that EsfFixedSizePolymorphicHolder copies the held instance in the
// copy-assignment operator. The existing instance must also be destroyed.
//
static void
TestCopyAssignment()
{
    Stats stats;
    BaseHolder h1(std::in_place_type<Derived1>, stats);
    BaseHolder h2(std::in_place_type<Derived2>, stats);
    stats.Reset();
    h1 = h2;
    TF_AXIOM(stats.numDtors == 1);
    TF_AXIOM(stats.numCopys == 1);
    TF_AXIOM(h1->GetDescription() == "Derived2");
}

// Tests that EsfFixedSizePolymorphicHolder moves the held instance in the
// move-assignment operator. The existing instance must also be destroyed.
//
static void
TestMoveAssignment()
{
    Stats stats;
    BaseHolder h1(std::in_place_type<Derived1>, stats);
    BaseHolder h2(std::in_place_type<Derived2>, stats);
    stats.Reset();
    h2 = std::move(h1);
    TF_AXIOM(stats.numDtors == 1);
    TF_AXIOM(stats.numCopys == 0);
    TF_AXIOM(stats.numMoves == 1);
    TF_AXIOM(h2->GetDescription() == "Derived1");
}

// Tests that EsfFixedSizePolymorphicHolder can hold derived types of different
// sizes if the BufferSize parameter is large enough.
//
static void
TestBigBuffer()
{
    BigBaseHolder h1(std::in_place_type<Derived1>);
    BigBaseHolder h2(std::in_place_type<Derived2>);
    BigBaseHolder h3(std::in_place_type<BigDerived>);
    TF_AXIOM(h1->GetDescription() == "Derived1");
    TF_AXIOM(h2->GetDescription() == "Derived2");
    TF_AXIOM(h3->GetDescription() == "BigDerived");
}

// Tests that EsfFixedSizePolymorphicHolder works with base classes that are
// overaligned.
//
static void
TestOverAlignedBase()
{
    OverAlignedBaseHolder h1(std::in_place_type<OverAlignedBaseImpl1>);
    OverAlignedBaseHolder h2(std::in_place_type<OverAlignedBaseImpl2>);
    h1 = h2;
    h1 = std::move(h2);
}

// Tests that certain derived instances are detected as not compatible with a
// given EsfFixedSizePolymorphicHolder specialization.
//
static void
TestCompatibility()
{
    static_assert(!BaseHolder::Compatibility::FITS_IN_BUFFER<BigDerived>);
    static_assert(!BaseHolder::Compatibility::DERIVES_FROM_BASE<OtherBase>);
    static_assert(!OtherBaseHolder::Compatibility::DERIVES_FROM_BASE<Derived1>);
    static_assert(!BaseHolder::Compatibility::HAS_ALIGNMENT<OverAlignedDerived>);

    // MultiplyDerived inherits from OtherBase, but OtherBase is not it's first
    // base class.
    MultiplyDerived derived;
    TF_AXIOM(OtherBaseHolder::Compatibility::DERIVES_FROM_BASE<MultiplyDerived>);
    TF_AXIOM(!OtherBaseHolder::Compatibility::HasBaseAtSameAddress(derived));
}

// Tests that multiple-inherited derived classes work, so long as the derived
// instance shares its address with its base instance.
//
static void
TestMultipleInheritance()
{
    BigBaseHolder h1(std::in_place_type<MultiplyDerived>);
    TF_AXIOM(h1->GetDescription() == "MultiplyDerived");
}

// Tests that the move/copy constructors/assignment operators enable use of
// EsfFixedSizePolymorphicHolder with std::vector.
//
static void
TestVectorUsage()
{
    std::vector<BigBaseHolder> holders;
    holders.reserve(2);
    holders.emplace_back(std::in_place_type<Derived1>);
    holders.emplace_back(std::in_place_type<Derived2>);
    holders.emplace_back(std::in_place_type<BigDerived>);
    holders.emplace_back(std::in_place_type<MultiplyDerived>);

    // Vectors can be copied
    std::vector<BigBaseHolder> holders2 = holders;

    // Vector elements can be erased.
    holders2.erase(holders2.begin());

    // The vector can reserve more memory, and move the existing elements
    // to the new buffer.
    holders2.reserve(1024);

    // Vectors CANNOT be resized because this requires that
    // EsfFixedSizePolymorphicHolder provide a default-constructor.
    // 
    // holders.resize(20);

    // Can iterate over all objects of the vector.
    std::string actual;
    for (const BigBaseHolder &h : holders2) {
        actual += h->GetDescription();
        actual += ',';
    }
    const std::string expected = "Derived2,BigDerived,MultiplyDerived,";
    TF_AXIOM(actual == expected);
}

int main()
{
    TestConstructor();
    TestDestructor();
    TestCopyConstructor();
    TestMoveConstructor();
    TestCopyAssignment();
    TestMoveAssignment();
    TestBigBuffer();
    TestOverAlignedBase();
    TestCompatibility();
    TestMultipleInheritance();
    TestVectorUsage();
}
