//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/ar/resolvedPath.h"

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

static std::string
_Repr(const ArResolvedPath& p)
{
    return TfStringPrintf(
        "%sResolvedPath(%s)",
        TF_PY_REPR_PREFIX.c_str(), 
        !p ? "" : TfStringPrintf("'%s'", p.GetPathString().c_str()).c_str());
}

static bool
_IsValid(const ArResolvedPath& p)
{
    return static_cast<bool>(p);
}

#include "pxr/base/tf/pyNanobindBridge.h"

void
wrapResolvedPath(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = ArResolvedPath;

    nb::class_<This>(m, "ResolvedPath")
        .def(nb::init<>())
        .def(nb::init<const std::string&>())

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)
        .def(nb::self > nb::self)
        .def(nb::self <= nb::self)
        .def(nb::self >= nb::self)

        .def(nb::self == std::string())
        .def(nb::self != std::string())
        .def(nb::self < std::string())
        .def(nb::self > std::string())
        .def(nb::self <= std::string())
        .def(nb::self >= std::string())

        .def("__bool__", _IsValid)
        .def("__hash__", &This::GetHash)
        .def("__repr__", &_Repr)
        .def("__str__", &This::GetPathString)

        .def("GetPathString", &This::GetPathString)
        ;

}
