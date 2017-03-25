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
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdGeom/constraintTarget.h"

#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

} // anonymous namespace 

void wrapUsdGeomModelAPI()
{
    typedef UsdGeomModelAPI This;

    class_<
        This, bases<UsdModelAPI>
        > cls("ModelAPI", init<UsdPrim>(arg("prim")));

    cls
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())
        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def(!self)

    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

static object
_GetExtentsHint(
        const UsdGeomModelAPI& self,
        const UsdTimeCode &time)
{
    VtVec3fArray extents;
    if (!self.GetExtentsHint(&extents, time)) {
        return object();
    }

    return object(extents);
}

static bool
_SetExtentsHint(
        UsdGeomModelAPI& self,
        const TfPyObjWrapper pyVal,
        const UsdTimeCode &timeVal)
{
    VtValue value = UsdPythonToSdfType(pyVal, SdfValueTypeNames->Float3Array);
    if (!value.IsHolding<VtVec3fArray>()) {
        TF_CODING_ERROR("Improper value for 'extentsHint' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return false;
    }

    return self.SetExtentsHint(value.UncheckedGet<VtVec3fArray>(), timeVal);
}

WRAP_CUSTOM {
    _class
        .def("GetExtentsHint", &_GetExtentsHint,
                (arg("time")=UsdTimeCode::Default()))
        .def("SetExtentsHint", &_SetExtentsHint,
                (arg("extents"),
                 arg("time")=UsdTimeCode::Default()))
        .def("ComputeExtentsHint", &UsdGeomModelAPI::ComputeExtentsHint,
                (arg("bboxCache")))

        .def("GetExtentsHintAttr", &UsdGeomModelAPI::GetExtentsHintAttr)

        .def("GetConstraintTarget", &UsdGeomModelAPI::GetConstraintTarget)
        .def("CreateConstraintTarget", &UsdGeomModelAPI::CreateConstraintTarget)
        .def("GetConstraintTargets", &UsdGeomModelAPI::GetConstraintTargets,
            return_value_policy<TfPySequenceToList>())
    ;
}

} // anonymous namespace 
