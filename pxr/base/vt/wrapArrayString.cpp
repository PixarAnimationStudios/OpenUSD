//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#define ADDITION_OPERATOR

#include "pxr/pxr.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/wrapArray.h"

#include <string>
using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayString() {
    VtWrapArray<VtArray<string> >();
}

#if defined(ARCH_COMPILER_CLANG) && defined(ARCH_OS_WINDOWS)
PXR_NAMESPACE_OPEN_SCOPE
// On Windows, the VtArray functions are not being defined in the translation
// unit and are left as undefined symbols during linking. Forcing the
// instantiation here to force the symbols to be created for linking.
template class VtArray<string>;
PXR_NAMESPACE_CLOSE_SCOPE
#endif // defined(ARCH_COMPILER_CLANG) && defined(ARCH_OS_WINDOWS)
