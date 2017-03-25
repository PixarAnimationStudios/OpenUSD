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
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/constraintTarget.h"

#include "pxr/usd/usd/conversions.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/implicit.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// The python wrapping of ComputeInWorldSpace does not take an xform cache.
static GfMatrix4d
_ComputeInWorldSpace(
    const UsdGeomConstraintTarget &self,
    const UsdTimeCode &time = UsdTimeCode::Default())
{
    return self.ComputeInWorldSpace(time);
}

} // anonymous namespace 

void wrapUsdGeomConstraintTarget()
{
    typedef UsdGeomConstraintTarget ConstraintTarget;

    class_<ConstraintTarget>("ConstraintTarget")
        .def(init<UsdAttribute>(arg("attr")))
        .def(!self)

        .def("GetAttr", &ConstraintTarget::GetAttr,
             return_value_policy<return_by_value>())
        .def("IsDefined", &ConstraintTarget::IsDefined)

        .def("IsValid", &ConstraintTarget::IsValid)

        .def("SetIdentifier", &ConstraintTarget::SetIdentifier, arg("identifier"))
        .def("GetIdentifier", &ConstraintTarget::GetIdentifier,
             return_value_policy<return_by_value>())

        .def("Get", &ConstraintTarget::Get, (arg("time")=UsdTimeCode::Default()))
        .def("Set", &ConstraintTarget::Set, (arg("value"), 
            arg("time")=UsdTimeCode::Default()))

        .def("GetConstraintAttrName", &ConstraintTarget::GetConstraintAttrName)
            .staticmethod("GetConstraintAttrName")

        .def("ComputeInWorldSpace", _ComputeInWorldSpace,
             (arg("time")=UsdTimeCode::Default()))
        ;

    implicitly_convertible<ConstraintTarget, UsdAttribute>();
}
