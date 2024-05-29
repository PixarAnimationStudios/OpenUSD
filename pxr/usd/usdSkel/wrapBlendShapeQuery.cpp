//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/blendShapeQuery.h"

#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/usd/usdSkel/bindingAPI.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE;

namespace {


boost::python::tuple
_ComputeSubShapeWeights(const UsdSkelBlendShapeQuery& self,
                        const VtFloatArray& weights)
{
    VtFloatArray subShapeWeights;
    VtUIntArray blendShapeIndices;
    VtUIntArray subShapeIndices;
    
    self.ComputeSubShapeWeights(weights, &subShapeWeights,
                                &blendShapeIndices, &subShapeIndices);

    return boost::python::make_tuple(
        subShapeWeights, blendShapeIndices, subShapeIndices);
}


template <typename T>
std::vector<T>
_PyListToVector(const list& l)
{
    std::vector<T> vec(len(l));
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = extract<T>(l[i]);
    }
    return vec;
}


bool
_ComputeDeformedPoints(const UsdSkelBlendShapeQuery& self,
                       const TfSpan<const float> subShapeWeights,
                       const TfSpan<const unsigned> blendShapeIndices,
                       const TfSpan<const unsigned> subShapeIndices,
                       const list& blendShapePointIndices,
                       const list& subShapePointOffsets,
                       TfSpan<GfVec3f> points)
{
    return self.ComputeDeformedPoints(
        subShapeWeights, blendShapeIndices, subShapeIndices,
        _PyListToVector<VtIntArray>(blendShapePointIndices),
        _PyListToVector<VtVec3fArray>(subShapePointOffsets), points);
}


} // namespace


void wrapUsdSkelBlendShapeQuery()
{
    using This = UsdSkelBlendShapeQuery;

    class_<This>("BlendShapeQuery", init<>())
        .def(init<UsdSkelBindingAPI>())

        .def("__str__", &This::GetDescription)

        .def("GetBlendShape", &This::GetBlendShape)
        .def("GetInbetween", &This::GetInbetween)
        .def("GetBlendShapeIndex", &This::GetBlendShapeIndex)

        .def("GetNumBlendShapes", &This::GetNumBlendShapes)
        .def("GetNumSubShapes", &This::GetNumSubShapes)

        .def("ComputeBlendShapePointIndices",
             &This::ComputeBlendShapePointIndices,
             return_value_policy<TfPySequenceToList>())

        .def("ComputeSubShapePointOffsets",
             &This::ComputeSubShapePointOffsets,
             return_value_policy<TfPySequenceToList>())

        .def("ComputeSubShapeWeights", &_ComputeSubShapeWeights)

        .def("ComputeDeformedPoints", &_ComputeDeformedPoints,
             (arg("subShapeWeights"),
              arg("blendShapeIndices"),
              arg("subShapeIndices"),
              arg("blendShapePointIndices"),
              arg("subShapePointOffset"),
              arg("points")))
        ;
}
