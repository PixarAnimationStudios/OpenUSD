//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/meshTopologyValidation.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/iterator.hpp"
#include "pxr/external/boost/python/operators.hpp"

#include <sstream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

static PxOsdMeshTopologyValidation::Invalidation
_InvalidationInit(
    PxOsdMeshTopologyValidation::Code code, std::string const& message)
{
    return {code, message};
}

void
wrapMeshTopologyValidation()
{
    using This = PxOsdMeshTopologyValidation;

    class_<This> cls("MeshTopologyValidation", init<>());
    cls.def(!self);
    {
        scope obj = cls;
        TfPyWrapEnum<This::Code, true>();
        class_<This::Invalidation>("Invalidation", no_init)
            .def("__init__", &::_InvalidationInit)
            .def_readwrite("code", &This::Invalidation::code)
            .def_readwrite("message", &This::Invalidation::message)
            .def("__repr__", &::_InvalidationRepr);
    }
    cls.def("__repr__", &::_ValidationRepr);
    cls.def("__iter__", iterator<This>());
}
