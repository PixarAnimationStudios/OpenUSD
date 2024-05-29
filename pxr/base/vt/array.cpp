//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    VT_LOG_STACK_ON_ARRAY_DETACH_COPY, false,
    "Log a stack trace when a VtArray is copied to detach it from shared "
    "storage, to help track down unintended copies.");

void
Vt_ArrayBase::_DetachCopyHook(char const *funcName) const
{
    static bool log = TfGetEnvSetting(VT_LOG_STACK_ON_ARRAY_DETACH_COPY);
    if (ARCH_UNLIKELY(log)) {
        TfLogStackTrace(TfStringPrintf("Detach/copy VtArray (%s)", funcName));
    }
}

// Instantiate basic array templates.
#define VT_ARRAY_EXPLICIT_INST(unused, elem) \
    template class VT_API VtArray< VT_TYPE(elem) >;
TF_PP_SEQ_FOR_EACH(VT_ARRAY_EXPLICIT_INST, ~, VT_SCALAR_VALUE_TYPES)


PXR_NAMESPACE_CLOSE_SCOPE
