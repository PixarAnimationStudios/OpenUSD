//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/arch/export.h"
#include "pxr/pxr.h"
#include "pxr/base/arch/testArchAbi.h"


PXR_NAMESPACE_USING_DIRECTIVE

extern "C" {

ARCH_EXPORT ArchAbiBase2* newDerived() { return new ArchAbiDerived<int>; }

}
