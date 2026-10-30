//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateXformOpOrderAttr(UsdGeomXformable &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateXformOpOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}

static std::string
_Repr(const UsdGeomXformable &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Xformable(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomXformable(nb::module_ &m)
{
    typedef UsdGeomXformable This;

    nb::class_<This, UsdGeomImageable> cls(m, "Xformable");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetXformOpOrderAttr",
             &This::GetXformOpOrderAttr)
        .def("CreateXformOpOrderAttr",
             &_CreateXformOpOrderAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def("__repr__", ::_Repr)
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

#include "pxr/base/tf/pyEnum.h"
#include "pxr/usd/usd/timeCode.h"

namespace {

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

static std::vector<double>
_GetTimeSamplesInInterval(const UsdGeomXformable &self, const GfInterval &interval)
{
    std::vector<double> result;
    self.GetTimeSamplesInInterval(interval, &result);
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

    _class
        .def("AddXformOp", &This::AddXformOp, 
            nb::arg("opType"), 
             nb::arg("precision")=UsdGeomXformOp::PrecisionDouble,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddTranslateXOp", &This::AddTranslateXOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionDouble,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddTranslateYOp", &This::AddTranslateYOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionDouble,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddTranslateZOp", &This::AddTranslateZOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionDouble,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddTranslateOp", &This::AddTranslateOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionDouble,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddScaleXOp", &This::AddScaleXOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddScaleYOp", &This::AddScaleYOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddScaleZOp", &This::AddScaleZOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddScaleOp", &This::AddScaleOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateXOp", &This::AddRotateXOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateYOp", &This::AddRotateYOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateZOp", &This::AddRotateZOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateXYZOp", &This::AddRotateXYZOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateXZYOp", &This::AddRotateXZYOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateYXZOp", &This::AddRotateYXZOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateYZXOp", &This::AddRotateYZXOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateZXYOp", &This::AddRotateZXYOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddRotateZYXOp", &This::AddRotateZYXOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddOrientOp", &This::AddOrientOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionFloat,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("AddTransformOp", &This::AddTransformOp, 
            nb::arg("precision")=UsdGeomXformOp::PrecisionDouble,
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetXformOp", &This::GetXformOp, 
            nb::arg("opType"), 
             nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetTranslateXOp", &This::GetTranslateXOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetTranslateYOp", &This::GetTranslateYOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetTranslateZOp", &This::GetTranslateZOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetTranslateOp", &This::GetTranslateOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetScaleXOp", &This::GetScaleXOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetScaleYOp", &This::GetScaleYOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetScaleZOp", &This::GetScaleZOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetScaleOp", &This::GetScaleOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateXOp", &This::GetRotateXOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateYOp", &This::GetRotateYOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateZOp", &This::GetRotateZOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateXYZOp", &This::GetRotateXYZOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateXZYOp", &This::GetRotateXZYOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateYXZOp", &This::GetRotateYXZOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateYZXOp", &This::GetRotateYZXOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateZXYOp", &This::GetRotateZXYOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetRotateZYXOp", &This::GetRotateZYXOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetOrientOp", &This::GetOrientOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("GetTransformOp", &This::GetTransformOp, 
            nb::arg("opSuffix")=TfToken(),
             nb::arg("isInverseOp")=false)

        .def("SetResetXformStack", &This::SetResetXformStack,
            nb::arg("resetXform"))

        .def("GetResetXformStack", &This::GetResetXformStack)

        .def("SetXformOpOrder", &This::SetXformOpOrder,
            nb::arg("orderedXformOps"),
             nb::arg("resetXformStack")=false)

        .def("GetOrderedXformOps", &_GetOrderedXformOps,
             nb::rv_policy::copy,
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
             nb::rv_policy::copy)

        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval,
             nb::rv_policy::copy)

        .def("GetLocalTransformation", _GetLocalTransformation1,
            (nb::arg("time")=UsdTimeCode::Default()),
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
            nb::arg("ops"),nb::arg("time")=UsdTimeCode::Default(),
            "Compute the fully-combined, local-to-parent transformation for "
            "this prim as efficiently as possible, using pre-fetched "
            "list of ordered xform ops supplied by the client.\n"
            "The python version of this function only returns the computed "
            " local-to-parent transformation. Clients must independently call "
            " GetResetXformStack() to be able to construct the local-to-world"
            " transformation.")

        .def_static("IsTransformationAffectedByAttrNamed", &This::IsTransformationAffectedByAttrNamed)
    ;
}

} // anonymous namespace
