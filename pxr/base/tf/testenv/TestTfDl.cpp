//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//
// This is a dummy shared library whose only purpose in life is to
// be used by the TfDl unit test (dl.cpp). It tries to dlopen this
// library and fails if it can't. It doesn't really matter what's
// actually in this file.
//

#include "pxr/pxr.h"
#include <stdio.h>

PXR_NAMESPACE_USING_DIRECTIVE

int
TestTfDl_SaySomething()
{
    printf("We've got a piper down, I repeat, the piper is DOWN!\n");
    return 0;
}
