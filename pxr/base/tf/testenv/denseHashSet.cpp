//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/denseHashSet.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/tf/stringUtils.h"

#include <cstdio>

PXR_NAMESPACE_USING_DIRECTIVE

// Test equality operator.
struct TestTf_DenseHashSetModuloEqual
{
    TestTf_DenseHashSetModuloEqual(size_t mod = 1)
    :   _mod(mod) {}

    bool operator()(const size_t &x, const size_t &y) const {
        return x%_mod == y%_mod;
    }

    size_t _mod;
};

static void Run()
{
    typedef TfDenseHashSet<size_t, TfHash> _Set;
    _Set _set;

    // Make sure size expectations are ok.
    // Due to empty base optimization, because both HashFn and EqualKey are
    // 0-size, should only hold a vector + pointer
    // (Note that on windows, debug mode will change sizeof vector)
    TF_AXIOM(sizeof(_Set) == sizeof(std::vector<_Set::value_type>)
                             + sizeof(void *));

    // Insert a bunch of numbers in order.
    printf("inserting numbers to 10000\n");
    for(size_t i = 1; i <= 10000; ++i) {
        _set.insert(i);
        TF_AXIOM(_set.size() == i);
        TF_AXIOM(_set.find(i) != _set.end());
        TF_AXIOM(_set.count(i) == 1);
    }

    TF_AXIOM(!_set.empty());
    TF_AXIOM(_set.size() == 10000);

    printf("Exercise assignment operator and swap.\n");
    _Set newSet;

    newSet = _set;
    TF_AXIOM(newSet.size() == _set.size());
    newSet.insert(9999999);
    TF_AXIOM(newSet.size() == _set.size()+1);

    newSet.swap(_set);
    TF_AXIOM(newSet.size()+1 == _set.size());
    newSet.swap(_set);
    TF_AXIOM(newSet.size() == _set.size()+1);


    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i)
        TF_AXIOM(_set.count(i) == 1);

    // Remove some stuff.
    printf("erasing 8000 elements\n");
    for(size_t i = 1000; i < 9000; ++i)
        TF_AXIOM(_set.erase(i) == 1);

    // Attempt to remove some stuff again.
    printf("erasing 8000 elements\n");
    for(size_t i = 1000; i < 9000; ++i)
        TF_AXIOM(_set.erase(i) == 0);

    TF_AXIOM(!_set.empty());
    TF_AXIOM(_set.size() == 2000);

    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i) {
        if (i < 1000 || i >= 9000)
            TF_AXIOM(_set.count(i) == 1);
        else
            TF_AXIOM(_set.count(i) == 0);
    }

    printf("testing shrink to fit\n");
    _set.shrink_to_fit();

    TF_AXIOM(!_set.empty());
    TF_AXIOM(_set.size() == 2000);

    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i) {
        if (i < 1000 || i >= 9000)
            TF_AXIOM(_set.count(i) == 1);
        else
            TF_AXIOM(_set.count(i) == 0);
    }

    // Put it back.
    printf("reinserting 8000 elements\n");
    for(size_t i = 1000; i < 9000; ++i)
        _set.insert(i);

    TF_AXIOM(!_set.empty());
    TF_AXIOM(_set.size() == 10000);

    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i)
        TF_AXIOM(_set.count(i) == 1);

    // Remove some stuff.
    printf("erasing 8000 elements\n");
    for(size_t i = 1000; i < 9000; ++i)
        _set.erase(i);

    TF_AXIOM(!_set.empty());
    TF_AXIOM(_set.size() == 2000);

    printf("checking containment\n");
    for(size_t i = 1; i <= 10000; ++i) {
        TF_AXIOM(_set.count(i) == (i < 1000 || i >= 9000));
    }

    // iterate
    printf("iterating\n");
    size_t count = 0;
    for(_Set::iterator i = _set.begin(); i != _set.end(); ++i, ++count) {
        TF_AXIOM(*i < 1000 || *i >= 9000);
    }
    TF_AXIOM(count == 2000);

    // iterate
    printf("const iterating\n");
    count = 0;
    for(_Set::const_iterator i = _set.begin(); i != _set.end(); ++i, ++count) {
        TF_AXIOM(*i < 1000 || *i >= 9000);
    }
    TF_AXIOM(count == 2000);
            
    printf("remove all but the first two elements using erase(range)...\n");
    _Set::iterator i0 = _set.begin();
    size_t keys[2];
    keys[0] = *i0++;
    keys[1] = *i0++;
    _set.erase(i0, _set.end());
    TF_AXIOM(_set.size() == 2);
    i0 = _set.begin();
    TF_AXIOM(*i0++ == keys[0]);
    TF_AXIOM(*i0++ == keys[1]);

    printf("inserting using insert(range)\n");
    std::vector<size_t> morekeys;
    for(size_t i=100; i<200; i++)
        morekeys.push_back(i);
    _set.insert(morekeys.begin(), morekeys.end());
    TF_AXIOM(_set.size() == 102);
    for(size_t i=100; i<200; i++)
        TF_AXIOM(_set.count(i));


    // copying and comparing sets.
    printf("copying and comparing...\n");
    _Set other(_set);
    TF_AXIOM(other.size() == _set.size());
    TF_AXIOM(other == _set);
    other.insert(4711);
    TF_AXIOM(other.size() != _set.size());
    TF_AXIOM(other != _set);


    // clear it.
    printf("clearing\n");
    _set.clear();
    TF_AXIOM(_set.empty());
    TF_AXIOM(_set.size() == 0);

    printf("shrinking\n");
    _set.shrink_to_fit();
    TF_AXIOM(_set.empty());
    TF_AXIOM(_set.size() == 0);

    printf("exercise initialize_list ctor / assignment.\n");
    _Set init {
        { 100 },
        { 110 },
        { 120 },
        { 130 }
    };
    TF_AXIOM(init.size() == 4);
    TF_AXIOM(init.count(100));
    TF_AXIOM(init.count(110));
    TF_AXIOM(init.count(120));
    TF_AXIOM(init.count(130));
    TF_AXIOM(!init.count(140));

    init = {
        { 2717 },
        { 2129 },
    };

    TF_AXIOM(init.size() == 2);
    TF_AXIOM(init.count(2717));
    TF_AXIOM(init.count(2129));

    printf("\nTesting TfDenseHashSet using an EqualKey.\n");

    typedef
        TfDenseHashSet<
            size_t, TfHash, TestTf_DenseHashSetModuloEqual, 128>
        _Set2;

    _Set2 _set2(TfHash(), TestTf_DenseHashSetModuloEqual(2));

    // Make sure size expectations are ok.
    TF_AXIOM(sizeof(TestTf_DenseHashSetModuloEqual) > 0);
    TF_AXIOM(sizeof(_Set2) == sizeof(std::vector<_Set2::value_type>)
                              + sizeof(void *)
                              + sizeof(TestTf_DenseHashSetModuloEqual));

    // Insert a bunch of numbers in order.
    printf("inserting numbers to 10000\n");
    for(size_t i = 1; i <= 10000; ++i) {
        _set2.insert(i);
    }

    printf("expecting only two elements\n");
    TF_AXIOM(!_set2.empty());
    TF_AXIOM(_set2.size() == 2);
}

static void
TestMoveOperations()
{
    using Set = TfDenseHashSet<int, TfHash>;

    printf("\nTesting TfDenseHashSet move constructor & assignment...\n");

    // Move some small (without hash index) sets

    // Move construction of empty set from empty set
    Set emptySet1(Set{});
    TF_AXIOM(emptySet1.empty());

    // Move assignment of empty set into empty set
    Set emptySet2;
    emptySet2 = Set();
    TF_AXIOM(emptySet2.empty());

    Set smallSet1;
    smallSet1.insert(1);
    smallSet1.insert(2);

    // Move construction of small set
    Set smallSet2(std::move(smallSet1));
    TF_AXIOM(smallSet2.size() == 2);
    TF_AXIOM(smallSet2.find(1) != smallSet2.end());
    TF_AXIOM(smallSet2.find(2) != smallSet2.end());

    // Move assignment of small set into small set
    Set smallSet3;
    smallSet3.insert(0);
    TF_AXIOM(smallSet3.size() == 1);
    smallSet3 = std::move(smallSet2);
    TF_AXIOM(smallSet3.size() == 2);
    TF_AXIOM(smallSet3.find(1) != smallSet3.end());
    TF_AXIOM(smallSet3.find(2) != smallSet3.end());

    // Move assignment of small set into empty set
    Set smallSet4;
    smallSet4 = std::move(smallSet3);
    TF_AXIOM(smallSet4.size() == 2);
    TF_AXIOM(smallSet4.find(1) != smallSet4.end());
    TF_AXIOM(smallSet4.find(2) != smallSet4.end());

    // Move assignment of an empty set into a small set
    Set emptySet3;
    smallSet4 = std::move(emptySet3);
    TF_AXIOM(smallSet4.empty());

    // Move some large (with hash index) sets

    Set largeSet1;
    for (int i=0; i<10000; ++i) {
        largeSet1.insert(i);
    }
    TF_AXIOM(largeSet1.size() == 10000);

    // Move construction of large set
    Set largeSet2(std::move(largeSet1));
    TF_AXIOM(largeSet2.size() == 10000);
    TF_AXIOM(largeSet2.find(2319) != largeSet2.end());

    // Move assignment of large set into large set
    Set largeSet3;
    for (int i=10000; i<20000; ++i) {
        largeSet3.insert(i);
    }
    TF_AXIOM(largeSet3.size() == 10000);
    largeSet3 = std::move(largeSet2);
    TF_AXIOM(largeSet3.size() == 10000);
    TF_AXIOM(largeSet3.find(2319) != largeSet3.end());

    // Move assignment of large set into empty set
    Set largeSet4;
    largeSet4 = std::move(largeSet3);
    TF_AXIOM(largeSet4.size() == 10000);
    TF_AXIOM(largeSet4.find(2319) != largeSet4.end());

    // Move assignment of an empty set into a large set
    Set emptySet4;
    largeSet4 = std::move(emptySet4);
    TF_AXIOM(largeSet4.empty());

    // Move assignment of a small set into a large set
    Set smallSet5;
    smallSet5.insert(3);
    smallSet5.insert(4);
    Set largeSet5;
    for (int i=20000; i<30000; ++i) {
        largeSet5.insert(i);
    }
    largeSet5 = std::move(smallSet5);
    TF_AXIOM(largeSet5.size() == 2);
    TF_AXIOM(largeSet5.find(3) != largeSet5.end());
    TF_AXIOM(largeSet5.find(4) != largeSet5.end());

    // Move assignment of a large set into a small set
    Set smallSet6;
    smallSet6.insert(5);
    smallSet6.insert(6);
    Set largeSet6;
    for (int i=30000; i<40000; ++i) {
        largeSet6.insert(i);
    }
    smallSet6 = std::move(largeSet6);
    TF_AXIOM(smallSet6.size() == 10000);
    TF_AXIOM(smallSet6.find(35000) != smallSet6.end());
}

/// Test contiguous storage by using TfSpan to access TfDenseHashMap data.
static void
TestContiguousStorage()
{
    // Build a TfDenseHashMap and then delete elements from the middle.
    using Set = TfDenseHashSet<size_t, TfHash>;
    Set set;

    printf("Inserting numbers to 10000\n");
    for(size_t i = 0; i <= 10000; ++i) {
        set.insert(i);
    }

    printf("Removing numbers 2500 to 5000\n");
    for(size_t i = 2500; i <= 5000; ++i) {
        set.erase(i);
    }

    printf("Verify by iterating over contents.\n");
    for (auto it = set.begin(); it != set.end(); ++it) {
        TF_AXIOM(*it < 2500 || *it > 5000);
    }

    // Iterate using cbegin/cend over a const copy of the set.
    const Set constSet(set);
    for (auto it = constSet.cbegin(); it != constSet.cend(); ++it) {
        TF_AXIOM(*it < 2500 || *it > 5000);
    }

    // Iterate using data/size.
    {
        size_t *data = set.data();
        for (size_t i=0; i<set.size(); ++i, ++data) {
            TF_AXIOM(*data < 2500 || *data > 5000);
        }
    }

    // Iterate using cdata/size.
    {
        const size_t *data = constSet.cdata();
        for (size_t i=0; i<constSet.size(); ++i, ++data) {
            TF_AXIOM(*data < 2500 || *data > 5000);
        }
    }

    printf("Verify contents using TfSpan\n");

    auto verifySpan = [](TfSpan<const size_t> span) {
        TF_AXIOM(span.size() == 7500);
        for (const auto &value : span) {
            TF_AXIOM(value < 2500 || value > 5000);
        }
    };

    // Implicitly convert a set to a span.
    verifySpan(set);

    // Explicitly construct a const span.
    verifySpan(TfMakeConstSpan(set));

    printf("Test TfSpan with an empty set.\n");
    Set emptySet;
    auto emptySpan = TfMakeSpan(emptySet);
    TF_AXIOM(emptySpan.empty());
    for (const auto &value : emptySpan) {
        TF_UNUSED(value);
        TF_AXIOM(false);
    }
    for (const auto &value : TfMakeConstSpan(emptySet)) {
        TF_UNUSED(value);
        TF_AXIOM(false);
    }
}

static bool
Test_TfDenseHashSet()
{
    Run();
    TestMoveOperations();
    TestContiguousStorage();
    return true;
}

TF_ADD_REGTEST(TfDenseHashSet);

