//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/stringHash.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Hash should work at compile time
static_assert(0x7c885313 == TraceStringHash::Hash("Test"), "Hash Error");
static_assert(5381 == TraceStringHash::Hash(""), "Hash Error");

int
main(int argc, char *argv[]) {
    printf ("Testing runtime hash\n");
    TF_AXIOM(0x7c885313 == TraceStringHash::Hash("Test"));
    TF_AXIOM(5381 == TraceStringHash::Hash(""));
    printf ("  passed\n");
}