//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyUtils.h"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

void wrapMeshTopology(nb::module_& m)
{
    using This = PxOsdMeshTopology;

    // Use the explicit function signature to select the overload to wrap.
    const PxOsdSubdivTags& (This::*getSubdivTags)() const =
        &This::GetSubdivTags;

    nb::class_<This>(m, "MeshTopology")
        .def(nb::init<TfToken, TfToken, VtIntArray, VtIntArray>())
        .def(nb::init<TfToken, TfToken, VtIntArray, VtIntArray, VtIntArray>())
        .def(nb::init<TfToken, TfToken, VtIntArray, VtIntArray, VtIntArray, PxOsdSubdivTags>())
        .def(nb::init<TfToken, TfToken, VtIntArray, VtIntArray, PxOsdSubdivTags>())
        .def(nb::init<>())
        .def("__repr__", &::_ReprMeshTopology)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__str__", [](const This& self) {
            std::ostringstream s;
            s << self;
            return s.str();
        })

        .def("GetScheme", &This::GetScheme)
        .def("WithScheme", &This::WithScheme)
        .def("GetFaceVertexCounts", &This::GetFaceVertexCounts)
        .def("GetFaceVertexIndices", &This::GetFaceVertexIndices)
        .def("GetOrientation", &This::GetOrientation)
        .def("WithOrientation", &This::WithOrientation)
        .def("GetHoleIndices", &This::GetHoleIndices)
        .def("WithHoleIndices", &This::WithHoleIndices)
        .def("GetSubdivTags", getSubdivTags)
        .def("WithSubdivTags", &This::WithSubdivTags)
        .def("ComputeHash", &This::ComputeHash)
        .def("Validate", &This::Validate)
    ;
}
