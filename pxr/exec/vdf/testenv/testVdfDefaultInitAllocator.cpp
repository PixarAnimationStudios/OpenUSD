//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/defaultInitAllocator.h"

#include "pxr/base/tf/diagnostic.h"

#include <numeric>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char *argv[])
{
    // Construct a vector, fill it with monotonically increasing integers.
    std::vector<int> valueInit;
    valueInit.resize(10);
    std::iota(valueInit.begin(), valueInit.end(), 1);

    // Resize the vector to chop off the tail, then resize it again to grow
    // back to full size. The second resize will cause value initialization.
    valueInit.resize(1);
    valueInit.resize(10);

    // Vector should look like this: [1, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    TF_AXIOM(valueInit[0] == 1);
    for (int i = 1; i < 10; ++i) {
        TF_AXIOM(valueInit[i] == 0);
    }

    // Construct a vector, fill it with monotonically increasing integers.
    std::vector<int, Vdf_DefaultInitAllocator<int>> defaultInit;
    defaultInit.resize(10);
    std::iota(defaultInit.begin(), defaultInit.end(), 1);

    // Resize the vector to chop off the tail, then resize it again to grow
    // back to full size. The second resize will cause default initialization,
    // i.e. the contents in memory should not change.
    defaultInit.resize(1);
    defaultInit.resize(10);

    // Vector should look like this: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    TF_AXIOM(defaultInit[0] == 1);
    for (int i = 1; i < 10; ++i) {
        TF_AXIOM(defaultInit[i] == (i + 1));
    }

    return 0;
}
