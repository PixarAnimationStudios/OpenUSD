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
#include "pxr/usd/usdSkel/animQuery.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/prim.h"

#include "pxr/base/gf/interval.h"

#include <boost/python.hpp>

#include <vector>


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


VtMatrix4dArray
_ComputeJointLocalTransforms(const UsdSkelAnimQuery& self, UsdTimeCode time)
{
    VtMatrix4dArray xforms;
    self.ComputeJointLocalTransforms(&xforms, time);
    return xforms;
}


boost::python::tuple
_ComputeJointLocalTransformComponents(const UsdSkelAnimQuery& self, UsdTimeCode time)
{
    VtVec3fArray translations;
    VtQuatfArray rotations;
    VtVec3hArray scales;
    self.ComputeJointLocalTransformComponents(&translations, &rotations,
                                              &scales, time);
    return boost::python::make_tuple(translations, rotations, scales);
}


VtFloatArray
_ComputeBlendShapeWeights(const UsdSkelAnimQuery& self, UsdTimeCode time)
{
    VtFloatArray weights;
    self.ComputeBlendShapeWeights(&weights, time);
    return weights;
}


std::vector<double>
_GetJointTransformTimeSamples(const UsdSkelAnimQuery& self)
{   
    std::vector<double> times;
    self.GetJointTransformTimeSamples(&times);
    return times;
}


std::vector<double>
_GetJointTransformTimeSamplesInInterval(const UsdSkelAnimQuery& self,
                                        const GfInterval& interval)
{   
    std::vector<double> times;
    self.GetJointTransformTimeSamplesInInterval(interval, &times);
    return times;
}


} // namespace


void wrapUsdSkelAnimQuery()
{
    using This = UsdSkelAnimQuery;

    class_<This>("AnimQuery", no_init)

        .def(!self)
        .def(self == self)
        .def(self != self)

        .def("__str__", &This::GetDescription)

        .def("GetPrim", &This::GetPrim)

        .def("ComputeJointLocalTransforms", &_ComputeJointLocalTransforms,
             (arg("time")=UsdTimeCode::Default()))

        .def("ComputeJointLocalTransformComponents",
             &_ComputeJointLocalTransformComponents,
             (arg("time")=UsdTimeCode::Default()))

        .def("ComputeBlendShapeWeights", &_ComputeBlendShapeWeights,
             (arg("time")=UsdTimeCode::Default()))

        .def("GetJointTransformTimeSamples", &_GetJointTransformTimeSamples)

        .def("GetJointTransformTimeSamplesInInterval",
             &_GetJointTransformTimeSamplesInInterval,
             (arg("interval")))
        
        .def("JointTransformsMightBeTimeVarying",
             &This::JointTransformsMightBeTimeVarying)

        .def("GetJointOrder", &This::GetJointOrder)

        .def("GetBlendShapeOrder", &This::GetBlendShapeOrder)
        ;
}            
