//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#define ADDITION_OPERATOR
#define SUBTRACTION_OPERATOR
#define MULTIPLICATION_OPERATOR
#define DOUBLE_MULT_OPERATOR
#define DOUBLE_DIV_OPERATOR

#include "pxr/pxr.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayDualQuaternion() {
    TF_PP_SEQ_FOR_EACH(VT_WRAP_ARRAY, ~, VT_DUALQUATERNION_VALUE_TYPES);
}

#if defined(ARCH_COMPILER_CLANG) && defined(ARCH_OS_WINDOWS)
PXR_NAMESPACE_OPEN_SCOPE
// On Windows, the VtArray functions are not being defined in the translation
// unit and are left as undefined symbols during linking. Forcing the
// instantiation here to force the symbols to be created for linking.
template class VtArray<GfDualQuath>;
template class VtArray<GfDualQuatf>;
template class VtArray<GfDualQuatd>;
PXR_NAMESPACE_CLOSE_SCOPE
#endif // defined(ARCH_COMPILER_CLANG) && defined(ARCH_OS_WINDOWS)
