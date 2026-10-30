//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdf/opaqueValue.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/vt/valueFromPython.h"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

static std::string
_SdfOpaqueValueRepr(const SdfOpaqueValue &self)
{
    return TF_PY_REPR_PREFIX + "OpaqueValue()";
}

static size_t
_SdfOpaqueValueHash(const SdfOpaqueValue &self)
{
    return TfHash()(self);
}

void wrapOpaqueValue(nanobind::module_ &m)
{
    namespace nb = nanobind;
    nb::class_<SdfOpaqueValue>(m, "OpaqueValue")
        .def(nb::init<>())
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__repr__", _SdfOpaqueValueRepr)
        .def("__hash__", _SdfOpaqueValueHash);
    VtValueFromPython<SdfOpaqueValue>();
}
