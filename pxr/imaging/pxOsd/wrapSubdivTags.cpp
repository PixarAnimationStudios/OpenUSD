//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/operators.hpp"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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
