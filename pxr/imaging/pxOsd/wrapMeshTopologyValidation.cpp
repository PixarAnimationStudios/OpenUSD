//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/meshTopologyValidation.h"

#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"

#include <sstream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static std::string
_InvalidationRepr(
    PxOsdMeshTopologyValidation::Invalidation const& invalidation)
{
    return TfStringPrintf(
        "PxOsd.MeshTopologyValidation.Invalidation('%s', '%s')",
        TfPyRepr(invalidation.code).c_str(), invalidation.message.c_str());
}

static std::string
_ValidationRepr(
    PxOsdMeshTopologyValidation const& validation)
{
    std::ostringstream repr(std::ostringstream::ate);
    if (validation) {
        repr << "PxOsd.MeshTopologyValidation()";
    } else {
        repr << "PxOsd.MeshTopologyValidation<";
        for (auto const& element : validation) {
            repr << TfPyRepr(element.code) << ", " << element.message << "), ";
        }
        repr << ">";
    }
    return repr.str();
}

void
wrapMeshTopologyValidation(nb::module_& m)
{
    using This = PxOsdMeshTopologyValidation;

    nb::class_<This> cls(m, "MeshTopologyValidation");
    cls.def(nb::init<>());
    cls.def("__bool__", [](const This& self) {
        return !(!self);
    });
    TfNbWrapEnum<This::Code, true>(cls);
    nb::class_<This::Invalidation>(cls, "Invalidation")
        .def("__init__", [](This::Invalidation* self,
                            This::Code code,
                            std::string const& message) {
            new (self) This::Invalidation{code, message};
        })
        .def_rw("code", &This::Invalidation::code)
        .def_rw("message", &This::Invalidation::message)
        .def("__repr__", &::_InvalidationRepr);
    cls.def("__repr__", &::_ValidationRepr);
    cls.def("__iter__", [](const This& self) {
        return nb::make_iterator(
            nb::type<This>(), "Iterator", self.begin(), self.end());
    }, nb::keep_alive<0, 1>());
}
