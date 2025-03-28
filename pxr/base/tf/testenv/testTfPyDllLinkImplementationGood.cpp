//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include <cstdio>

extern "C"
__declspec(dllexport)
int testTfPyDllLinkImplementation() {
    printf("Good implementation - returning zero...\n");
    return 0;
}