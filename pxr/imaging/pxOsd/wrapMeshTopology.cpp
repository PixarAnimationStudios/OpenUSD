//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/pxOsd/meshTopology.h"

#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/operators.hpp>

#include <sstream>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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
