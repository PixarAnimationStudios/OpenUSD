//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include "pxr/usd/usdProc/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, [](nanobind::handle) { return UsdProcTokens->name.GetString(); }, nanobind::rv_policy::copy);

void wrapUsdProcTokens(nanobind::module_ &m)
{
    nanobind::class_<UsdProcTokensType> cls(m, "Tokens");
    cls.def("__init__", [](nanobind::args, nanobind::kwargs) {
        TfPyThrowRuntimeError("This class cannot be instantiated from Python");
    });
    _ADD_TOKEN(cls, proceduralSystem);
    _ADD_TOKEN(cls, GenerativeProcedural);
}
