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
#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/operators.hpp>

#include <sstream>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static std::string
_ReprSubdivTags(
    const PxOsdSubdivTags& tags)
{
    std::ostringstream repr(std::ostringstream::ate);
    repr << "PxOsd.SubdivTags("
         << TfPyRepr(tags.GetVertexInterpolationRule()) << ", "
         << TfPyRepr(tags.GetFaceVaryingInterpolationRule()) << ", "
         << TfPyRepr(tags.GetCreaseMethod()) << ", "
         << TfPyRepr(tags.GetTriangleSubdivision()) << ", "
         << TfPyRepr(tags.GetCreaseIndices()) << ", "
         << TfPyRepr(tags.GetCreaseLengths()) << ", "
         << TfPyRepr(tags.GetCreaseWeights()) << ", "
         << TfPyRepr(tags.GetCornerIndices()) << ", "
         << TfPyRepr(tags.GetCornerWeights()) << ")"
    ;
    return repr.str();
}

void wrapSubdivTags()
{
    using This = PxOsdSubdivTags;

    class_<This>("SubdivTags", init<>())
        .def(init<TfToken, TfToken, TfToken, TfToken,
                  VtIntArray, VtIntArray, VtFloatArray,
                  VtIntArray, VtFloatArray>())
        .def("__repr__", &::_ReprSubdivTags)
        .def(self == self)
        .def(self != self)
        .def(str(self))

        .def("GetVertexInterpolationRule", &This::GetVertexInterpolationRule)
        .def("SetVertexInterpolationRule", &This::SetVertexInterpolationRule)
        .def("GetFaceVaryingInterpolationRule",
             &This::GetFaceVaryingInterpolationRule)
        .def("SetFaceVaryingInterpolationRule",
             &This::SetFaceVaryingInterpolationRule)
        .def("GetCreaseMethod", &This::GetCreaseMethod)
        .def("SetCreaseMethod", &This::SetCreaseMethod)
        .def("GetTriangleSubdivision", &This::GetTriangleSubdivision)
        .def("SetTriangleSubdivision", &This::SetTriangleSubdivision)
        .def("GetCreaseIndices", &This::GetCreaseIndices,
             return_value_policy<copy_const_reference>())
        .def("SetCreaseIndices", &This::SetCreaseIndices)
        .def("GetCreaseLengths", &This::GetCreaseLengths,
             return_value_policy<copy_const_reference>())
        .def("SetCreaseLengths", &This::SetCreaseLengths)
        .def("GetCreaseWeights", &This::GetCreaseWeights,
             return_value_policy<copy_const_reference>())
        .def("SetCreaseWeights", &This::SetCreaseWeights)
        .def("GetCornerIndices", &This::GetCornerIndices,
             return_value_policy<copy_const_reference>())
        .def("SetCornerIndices", &This::SetCornerIndices)
        .def("GetCornerWeights", &This::GetCornerWeights,
             return_value_policy<copy_const_reference>())
        .def("SetCornerWeights", &This::SetCornerWeights)
        .def("ComputeHash", &This::ComputeHash)
    ;
}
