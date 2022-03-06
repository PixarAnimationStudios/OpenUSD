//
// Copyright 2016 Pixar
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
#include "pxr/usd/usdSkel/blendShapeQuery.h"

#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/usd/usdSkel/bindingAPI.h"

#include <boost/python.hpp>


PXR_NAMESPACE_USING_DIRECTIVE;

namespace pxrUsdUsdSkelWrapBlendShapeQuery {


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
_PyListToVector(const boost::python::list& l)
{
    std::vector<T> vec(boost::python::len(l));
    for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = boost::python::extract<T>(l[i]);
    }
    return vec;
}


bool
_ComputeDeformedPoints(const UsdSkelBlendShapeQuery& self,
                       const TfSpan<const float> subShapeWeights,
                       const TfSpan<const unsigned> blendShapeIndices,
                       const TfSpan<const unsigned> subShapeIndices,
                       const boost::python::list& blendShapePointIndices,
                       const boost::python::list& subShapePointOffsets,
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

    boost::python::class_<This>("BlendShapeQuery", boost::python::init<>())
        .def(boost::python::init<UsdSkelBindingAPI>())

        .def("__str__", &This::GetDescription)

        .def("GetBlendShape", &This::GetBlendShape)
        .def("GetInbetween", &This::GetInbetween)
        .def("GetBlendShapeIndex", &This::GetBlendShapeIndex)

        .def("GetNumBlendShapes", &This::GetNumBlendShapes)
        .def("GetNumSubShapes", &This::GetNumSubShapes)

        .def("ComputeBlendShapePointIndices",
             &This::ComputeBlendShapePointIndices,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("ComputeSubShapePointOffsets",
             &This::ComputeSubShapePointOffsets,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("ComputeSubShapeWeights", &pxrUsdUsdSkelWrapBlendShapeQuery::_ComputeSubShapeWeights)

        .def("ComputeDeformedPoints", &pxrUsdUsdSkelWrapBlendShapeQuery::_ComputeDeformedPoints,
             (boost::python::arg("subShapeWeights"),
              boost::python::arg("blendShapeIndices"),
              boost::python::arg("subShapeIndices"),
              boost::python::arg("blendShapePointIndices"),
              boost::python::arg("subShapePointOffset"),
              boost::python::arg("points")))
        ;
}
