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
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateXformOpOrderAttr(UsdGeomXformable &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateXformOpOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}

void wrapUsdGeomXformable()
{
    typedef UsdGeomXformable This;

    class_<This, bases<UsdGeomImageable> >
        cls("Xformable");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")


        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetXformOpOrderAttr",
             &This::GetXformOpOrderAttr)
        .def("CreateXformOpOrderAttr",
             &_CreateXformOpOrderAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

    ;

    _CustomWrapCode(cls);
}

PXR_NAMESPACE_CLOSE_SCOPE

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
// Just remember to wrap code in the pxr namespace macros:
// PXR_NAMESPACE_OPEN_SCOPE, PXR_NAMESPACE_CLOSE_SCOPE.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/pyEnum.h"

PXR_NAMESPACE_OPEN_SCOPE

static GfMatrix4d
_GetLocalTransformation1(const UsdGeomXformable &self,
                        const UsdTimeCode time) 
{
    GfMatrix4d result(1);
    bool resetsXformStack;
    self.GetLocalTransformation(&result, &resetsXformStack, time);
    return result;
}

static GfMatrix4d
_GetLocalTransformation2(const UsdGeomXformable &self,
                         const std::vector<UsdGeomXformOp> &ops,
                         const UsdTimeCode time) 
{
    GfMatrix4d result(1);
    bool resetsXformStack;
    self.GetLocalTransformation(&result, &resetsXformStack, ops, time);
    return result;
}

static std::vector<double>
_GetTimeSamples(const UsdGeomXformable &self)
{
    std::vector<double> result;
    self.GetTimeSamples(&result);
    return result;
}

static std::vector<UsdGeomXformOp>
_GetOrderedXformOps(const UsdGeomXformable &self)
{
    bool resetsXformStack=false;
    return self.GetOrderedXformOps(&resetsXformStack);
}

WRAP_CUSTOM {
    typedef UsdGeomXformable This;

    bool (This::*TransformMightBeTimeVarying_1)() const
        = &This::TransformMightBeTimeVarying;
    bool (This::*TransformMightBeTimeVarying_2)(
        const std::vector<UsdGeomXformOp> &) const 
        = &This::TransformMightBeTimeVarying;

    scope s = _class
        .def("AddXformOp", &This::AddXformOp, 
            (arg("opType"), 
             arg("precision")=UsdGeomXformOp::PrecisionDouble,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddTranslateOp", &This::AddTranslateOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionDouble,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddScaleOp", &This::AddScaleOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateXOp", &This::AddRotateXOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateYOp", &This::AddRotateYOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateZOp", &This::AddRotateZOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateXYZOp", &This::AddRotateXYZOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateXZYOp", &This::AddRotateXZYOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateYXZOp", &This::AddRotateYXZOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateYZXOp", &This::AddRotateYZXOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateZXYOp", &This::AddRotateZXYOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddRotateZYXOp", &This::AddRotateZYXOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddOrientOp", &This::AddOrientOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionFloat,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("AddTransformOp", &This::AddTransformOp, 
            (arg("precision")=UsdGeomXformOp::PrecisionDouble,
             arg("opSuffix")=TfToken(),
             arg("isInverseOp")=false))

        .def("SetResetXformStack", &This::SetResetXformStack,
            (arg("resetXform")))

        .def("GetResetXformStack", &This::GetResetXformStack)

        .def("SetXformOpOrder", &This::SetXformOpOrder,
            (arg("orderedXformOps"),
             arg("resetXformStack")=false))

        .def("GetOrderedXformOps", &_GetOrderedXformOps, 
            return_value_policy<TfPySequenceToList>(),
            "Return the ordered list of transform operations to be applied to "
            "this prim, in least-to-most-local order.  This is determined by "
            "the intersection of authored op-attributes and the explicit "
            "ordering of those attributes encoded in the \"xformOpOrder\" "
            "attribute on this prim."
            "\n\nAny entries in \"xformOpOrder\" that do not correspond to "
            "valid attributes on the xformable prim are skipped and a warning "
            "is issued."
            "\n\nA UsdGeomTransformable that has not had any ops added via "
            "AddXformOp() will return an empty vector.\n\n"
            "\n\nThe python version of this function only returns the ordered "
            "list of xformOps. Clients must independently call "
            "GetResetXformStack() if they need the info.")

        .def("ClearXformOpOrder", &This::ClearXformOpOrder)

        .def("MakeMatrixXform", &This::MakeMatrixXform)

        .def("TransformMightBeTimeVarying", TransformMightBeTimeVarying_1)
        .def("TransformMightBeTimeVarying", TransformMightBeTimeVarying_2)

        .def("GetTimeSamples", _GetTimeSamples,
            return_value_policy<TfPySequenceToList>())

        .def("GetLocalTransformation", _GetLocalTransformation1,
            "Compute the fully-combined, local-to-parent transformation for "
            "this prim.\n"
            "If a client does not need to manipulate the individual ops "
            "themselves, and requires only the combined transform on this prim,"
            " this method will take care of all the data marshalling and linear"
            " algebra needed to combine the ops into a 4x4 affine "
            "transformation matrix, in double-precision, regardless of the "
            "precision of the op inputs.\n"
            "The python version of this function only returns the computed "
            " local-to-parent transformation. Clients must independently call "
            " GetResetXformStack() to be able to construct the local-to-world"
            " transformation.")
        .def("GetLocalTransformation", _GetLocalTransformation2,
            "Compute the fully-combined, local-to-parent transformation for "
            "this prim as efficiently as possible, using pre-fetched "
            "list of ordered xform ops supplied by the client.\n"
            "The python version of this function only returns the computed "
            " local-to-parent transformation. Clients must independently call "
            " GetResetXformStack() to be able to construct the local-to-world"
            " transformation.")

        .def("IsTransformationAffectedByAttrNamed", &This::IsTransformationAffectedByAttrNamed)
            .staticmethod("IsTransformationAffectedByAttrNamed")
    ;
}

PXR_NAMESPACE_CLOSE_SCOPE
