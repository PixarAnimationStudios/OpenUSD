//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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


std::vector<double>
_GetBlendShapeWeightTimeSamples(const UsdSkelAnimQuery& self)
{   
    std::vector<double> times;
    self.GetBlendShapeWeightTimeSamples(&times);
    return times;
}


std::vector<double>
_GetBlendShapeWeightTimeSamplesInInterval(const UsdSkelAnimQuery& self,
                                        const GfInterval& interval)
{   
    std::vector<double> times;
    self.GetBlendShapeWeightTimeSamplesInInterval(interval, &times);
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

        .def("GetBlendShapeWeightTimeSamples", &_GetBlendShapeWeightTimeSamples)

        .def("GetBlendShapeWeightTimeSamplesInInterval",
             &_GetBlendShapeWeightTimeSamplesInInterval,
             (arg("interval")))

        .def("BlendShapeWeightsMightBeTimeVarying",
             &This::BlendShapeWeightsMightBeTimeVarying)

        .def("GetJointOrder", &This::GetJointOrder)

        .def("GetBlendShapeOrder", &This::GetBlendShapeOrder)
        ;
}            
