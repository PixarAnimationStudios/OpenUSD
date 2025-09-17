//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSemantics/labelsAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateLabelsAttr(UsdSemanticsLabelsAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLabelsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}

static bool _WrapIsSemanticsLabelsAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdSemanticsLabelsAPI::IsSemanticsLabelsAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdSemanticsLabelsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "UsdSemantics.LabelsAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdSemanticsLabelsAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdSemanticsLabelsAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdSemanticsLabelsAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdSemanticsLabelsAPI::CanApply(prim, name, &whyNot);
    return UsdSemanticsLabelsAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdSemanticsLabelsAPI()
{
    typedef UsdSemanticsLabelsAPI This;

    UsdSemanticsLabelsAPI_CanApplyResult::Wrap<UsdSemanticsLabelsAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("LabelsAPI");

    cls
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name"))))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name"))))
        .def(TfTypePythonClass())

        .def("Get",
            (UsdSemanticsLabelsAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            (UsdSemanticsLabelsAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
        .staticmethod("Get")

        .def("GetAll",
            (std::vector<UsdSemanticsLabelsAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            arg("prim"),
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAll")

        .def("CanApply", &_WrapCanApply, (arg("prim"), arg("name")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim"), arg("name")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .def("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             arg("includeInherited"),
             arg("instanceName"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetLabelsAttr",
             &This::GetLabelsAttr)
        .def("CreateLabelsAttr",
             &_CreateLabelsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("IsSemanticsLabelsAPIPath", _WrapIsSemanticsLabelsAPIPath)
            .staticmethod("IsSemanticsLabelsAPIPath")
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

namespace {

WRAP_CUSTOM {
    using This = UsdSemanticsLabelsAPI;
    _class
        .def("GetDirectTaxonomies", &This::GetDirectTaxonomies)
        .staticmethod("GetDirectTaxonomies")
        .def("ComputeInheritedTaxonomies", &This::ComputeInheritedTaxonomies)
        .staticmethod("ComputeInheritedTaxonomies")
        ;
}

}
