//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyUtils.h"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

void wrapSubdivTags(nb::module_& m)
{
    using This = PxOsdSubdivTags;

    nb::class_<This>(m, "SubdivTags")
        .def(nb::init<>())
        .def(nb::init<TfToken, TfToken, TfToken, TfToken,
                      VtIntArray, VtIntArray, VtFloatArray,
                      VtIntArray, VtFloatArray>())
        .def("__repr__", &::_ReprSubdivTags)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__str__", [](const This& self) {
            std::ostringstream s;
            s << self;
            return s.str();
        })

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
        .def("GetCreaseIndices", &This::GetCreaseIndices)
        .def("SetCreaseIndices", &This::SetCreaseIndices)
        .def("GetCreaseLengths", &This::GetCreaseLengths)
        .def("SetCreaseLengths", &This::SetCreaseLengths)
        .def("GetCreaseWeights", &This::GetCreaseWeights)
        .def("SetCreaseWeights", &This::SetCreaseWeights)
        .def("GetCornerIndices", &This::GetCornerIndices)
        .def("SetCornerIndices", &This::SetCornerIndices)
        .def("GetCornerWeights", &This::GetCornerWeights)
        .def("SetCornerWeights", &This::SetCornerWeights)
        .def("ComputeHash", &This::ComputeHash)
    ;
}
