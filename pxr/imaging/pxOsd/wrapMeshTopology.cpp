//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/operators.hpp"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static std::string
_ReprMeshTopology(
    const PxOsdMeshTopology& topology)
{
    std::ostringstream repr(std::ostringstream::ate);
    repr << "PxOsd.MeshTopology("
         << TfPyRepr(topology.GetScheme()) << ", "
         << TfPyRepr(topology.GetOrientation()) << ", "
         << TfPyRepr(topology.GetFaceVertexCounts()) << ", "
         << TfPyRepr(topology.GetFaceVertexIndices()) << ", "
         << TfPyRepr(topology.GetHoleIndices()) << ")"
    ;
    return repr.str();
}

void wrapMeshTopology()
{
    using This = PxOsdMeshTopology;

    // Use the explicit function signature to select the overload to wrap.
    const PxOsdSubdivTags& (This::*getSubdivTags)() const =
        &This::GetSubdivTags;

    class_<This>("MeshTopology",
                 init<TfToken, TfToken, VtIntArray, VtIntArray>())
        .def(init<TfToken, TfToken, VtIntArray, VtIntArray, VtIntArray>())
        .def(init<TfToken, TfToken, VtIntArray, VtIntArray, VtIntArray, PxOsdSubdivTags>())
        .def(init<TfToken, TfToken, VtIntArray, VtIntArray, PxOsdSubdivTags>())
        .def(init<>())
        .def("__repr__", &::_ReprMeshTopology)
        .def(self == self)
        .def(self != self)
        .def(str(self))

        .def("GetScheme", &This::GetScheme)
        .def("WithScheme", &This::WithScheme)
        .def("GetFaceVertexCounts", &This::GetFaceVertexCounts,
             return_value_policy<copy_const_reference>())
        .def("GetFaceVertexIndices", &This::GetFaceVertexIndices,
             return_value_policy<copy_const_reference>())
        .def("GetOrientation", &This::GetOrientation,
             return_value_policy<copy_const_reference>())
        .def("WithOrientation", &This::WithOrientation)
        .def("GetHoleIndices", &This::GetHoleIndices,
             return_value_policy<copy_const_reference>())
        .def("WithHoleIndices", &This::WithHoleIndices)
        .def("GetSubdivTags", getSubdivTags,
             return_value_policy<copy_const_reference>())
        .def("WithSubdivTags", &This::WithSubdivTags)
        .def("ComputeHash", &This::ComputeHash)
        .def("Validate", &This::Validate)
    ;
}
