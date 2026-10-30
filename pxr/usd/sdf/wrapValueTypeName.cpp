//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

void
wrapValueType(nanobind::module_& m)
{
    using This = SdfValueTypeName;

    nb::class_<This>(m, "ValueTypeName")
        .def(nb::init<>())
        .def("__bool__", &SdfValueTypeName::operator bool)
        .def(nb::self == std::string())
        .def(std::string() == nb::self)
        .def(nb::self != std::string())
        .def(std::string() != nb::self)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__hash__", &SdfValueTypeName::GetHash)
        .def("__str__", &SdfValueTypeName::GetAsToken)
        .def_prop_ro("type", &SdfValueTypeName::GetType)
        .def_prop_ro("cppTypeName", &SdfValueTypeName::GetCPPTypeName)
        .def_prop_ro("role", &SdfValueTypeName::GetRole)
        .def_prop_ro("defaultValue", &SdfValueTypeName::GetDefaultValue)
        .def_prop_ro("defaultUnit", &SdfValueTypeName::GetDefaultUnit)
        .def_prop_ro("scalarType", &SdfValueTypeName::GetScalarType)
        .def_prop_ro("arrayType", &SdfValueTypeName::GetArrayType)
        .def_prop_ro("isScalar", &SdfValueTypeName::IsScalar)
        .def_prop_ro("isArray", &SdfValueTypeName::IsArray)
        .def_prop_ro("aliasesAsStrings",
            [](const SdfValueTypeName& self) {
                return TfNbSequenceToTuple(self.GetAliasesAsTokens());
            })
        ;

    VtValueFromPython<SdfValueTypeName>();
}
