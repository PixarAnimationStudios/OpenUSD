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
#include "pxr/usd/usdSkel/binding.h"

 #include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/usd/usd/pyConversions.h"

#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"

#include <boost/python.hpp>


using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE


namespace {


object
_ComputeJointInfluences(const UsdSkelBindingTarget& self, UsdTimeCode time)
{
    VtIntArray indices;
    VtFloatArray weights;
    if(self.ComputeJointInfluences(&indices, &weights, time)) {
        return boost::python::make_tuple(indices, weights);
    }
    return object();
}


object
_ComputeVaryingJointInfluences(const UsdSkelBindingTarget& self,
                               size_t numPoints,
                               UsdTimeCode time)
{
    VtIntArray indices;
    VtFloatArray weights;
    if(self.ComputeVaryingJointInfluences(numPoints, &indices,
                                          &weights, time)) {
        return boost::python::make_tuple(indices, weights);
    }
    return object();
}


void _wrapUsdSkelBindingTarget()
{
    using This = UsdSkelBindingTarget;
    using ThisPtr = UsdSkelBindingTargetPtr;

    class_<This, ThisPtr, boost::noncopyable>("BindingTarget", no_init)

        .def(TfPyRefAndWeakPtr())
        
        .def(!self)

        .def("__str__", &This::GetDescription)

        .def("GetPrim", &This::GetPrim,
             return_value_policy<return_by_value>())
        
        .def("GetNumInfluencesPerComponent",
             &This::GetNumInfluencesPerComponent)

        .def("GetInterpolation", &This::GetInterpolation,
             return_value_policy<return_by_value>())

        .def("IsRigidlyDeformed", &This::IsRigidlyDeformed)

        .def("GetGeomBindTransformAttr", &This::GetGeomBindTransformAttr,
             return_value_policy<return_by_value>())

        .def("GetJointIndicesPrimvar", &This::GetJointIndicesPrimvar,
             return_value_policy<return_by_value>())

        .def("GetJointWeightsPrimvar", &This::GetJointWeightsPrimvar,
             return_value_policy<return_by_value>())

        .def("GetMapper", &This::GetMapper,
             return_value_policy<return_by_value>())
             
        .def("GetJointOrder", &This::GetJointOrder,
             return_value_policy<return_by_value>())

        .def("ComputeJointInfluences", &_ComputeJointInfluences,
             (arg("time")=UsdTimeCode::Default()))

        .def("ComputeVaryingJointInfluences", &_ComputeVaryingJointInfluences,
             (arg("numPoints"), arg("time")=UsdTimeCode::Default()))
        ;
}


const UsdSkelBindingTargetRefPtrVector&
_GetTargets(const UsdSkelBinding& self)
{   
    return self.GetTargets();
}


} // namespace


void wrapUsdSkelBinding()
{
    _wrapUsdSkelBindingTarget();

    using This = UsdSkelBinding;
    using ThisPtr = UsdSkelBindingPtr;

    class_<This, ThisPtr, boost::noncopyable>("Binding", no_init)

        .def(TfPyRefAndWeakPtr())

        .def(!self)
        
        .def("GetSkelQuery", &This::GetSkelQuery,
             return_value_policy<return_by_value>())

        .def("FindTarget", &This::FindTarget)

        .def("GetTargets", &_GetTargets,
             return_value_policy<TfPySequenceToList>())
        ;
}
