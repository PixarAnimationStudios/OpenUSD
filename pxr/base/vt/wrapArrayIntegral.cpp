//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#define NUMERIC_OPERATORS
#define MOD_OPERATOR

#include "pxr/pxr.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayIntegral() {
    TF_PP_SEQ_FOR_EACH(VT_WRAP_ARRAY, ~,
                       VT_INTEGRAL_BUILTIN_VALUE_TYPES);
}

#if defined(ARCH_COMPILER_CLANG) && defined(ARCH_OS_WINDOWS)
PXR_NAMESPACE_OPEN_SCOPE
// On Windows, the VtArray functions are not being defined in the translation
// unit and are left as undefined symbols during linking. Forcing the
// instantiation here to force the symbols to be created for linking.
template class VtArray<bool>;
template class VtArray<char>;
template class VtArray<unsigned char>;
template class VtArray<short>;
template class VtArray<unsigned short>;
template class VtArray<int>;
template class VtArray<unsigned int>;
template class VtArray<__int64>;
template class VtArray<unsigned __int64>;
PXR_NAMESPACE_CLOSE_SCOPE
#endif // defined(ARCH_COMPILER_CLANG) && defined(ARCH_OS_WINDOWS)
