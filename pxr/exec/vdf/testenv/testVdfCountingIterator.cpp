//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/countingIterator.h"

#include "pxr/base/tf/diagnostic.h"

#include <iterator>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char *argv[])
{
    // Create a counting iterator
    Vdf_CountingIterator<int> it;

    // We expect value initialization
    TF_AXIOM(*it == 0);

    // Test monotonically increasing values
    for (int i = 0; i < 10; ++i, ++it) {
        TF_AXIOM(i == *it);
    }

    // Post-increment
    TF_AXIOM(*it == 10);
    TF_AXIOM(*(it++) == 10);
    TF_AXIOM(*it == 11);

    // Post-decrement
    TF_AXIOM(*(it--) == 11);
    TF_AXIOM(*it == 10);

    // Test monotonically decreasing values
    for (int i = 10; i >= 0; --i, --it) {
        TF_AXIOM(i == *it);
    }

    // Rewind to 0
    TF_AXIOM(*it == -1);
    ++it;
    TF_AXIOM(*it == 0);

    // Test random access
    ++it;
    TF_AXIOM(it[5] == 6);

    it += 4;
    TF_AXIOM(*it == 5);

    it -= 4;
    TF_AXIOM(*it == 1);

    // Test distance
    Vdf_CountingIterator<int> it2;
    TF_AXIOM(std::distance(it2, it) == 1);
    it += 3;
    TF_AXIOM(std::distance(it2, it) == 4);

    ++it2;
    TF_AXIOM(std::distance(it2, it) == 3);
    TF_AXIOM(std::distance(it, it2) == -3);

    // Distance to default constructed iterator
    Vdf_CountingIterator<int> it3, it4;
    TF_AXIOM(std::distance(it3, it4) == 0);

    // Equality comparison
    ++it3;
    TF_AXIOM(it2 == it3);
    TF_AXIOM(it3 != it4);
    TF_AXIOM(it4 != it);

    // Ordering
    TF_AXIOM(it > it2);
    TF_AXIOM(it2 < it);
    TF_AXIOM(it2 >= it3);
    TF_AXIOM(it2 <= it3);

    return 0;
}
