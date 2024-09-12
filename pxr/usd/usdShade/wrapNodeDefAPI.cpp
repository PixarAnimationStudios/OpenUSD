//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdShade/nodeDefAPI.h"
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
_CreateImplementationSourceAttr(UsdShadeNodeDefAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateImplementationSourceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIdAttr(UsdShadeNodeDefAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateIdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdShadeNodeDefAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdShade.NodeDefAPI(%s)",
        primRepr.c_str());
}

struct UsdShadeNodeDefAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdShadeNodeDefAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdShadeNodeDefAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdShadeNodeDefAPI::CanApply(prim, &whyNot);
    return UsdShadeNodeDefAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdShadeNodeDefAPI()
{
    typedef UsdShadeNodeDefAPI This;

    UsdShadeNodeDefAPI_CanApplyResult::Wrap<UsdShadeNodeDefAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("NodeDefAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetImplementationSourceAttr",
             &This::GetImplementationSourceAttr)
        .def("CreateImplementationSourceAttr",
             &_CreateImplementationSourceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetIdAttr",
             &This::GetIdAttr)
        .def("CreateIdAttr",
             &_CreateIdAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

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

static object 
_WrapGetShaderId(const UsdShadeNodeDefAPI &shader)
{
    TfToken id;
    if (shader.GetShaderId(&id)) {
        return object(id);
    }
    return object();
}

static object 
_WrapGetSourceAsset(const UsdShadeNodeDefAPI &shader,
                    const TfToken &sourceType)
{
    SdfAssetPath asset;
    if (shader.GetSourceAsset(&asset, sourceType)) {
        return object(asset);
    }
    return object();
}

static object
_WrapGetSourceAssetSubIdentifier(const UsdShadeNodeDefAPI &shader,
                    const TfToken &sourceType)
{
    TfToken subIdentifier;
    if (shader.GetSourceAssetSubIdentifier(&subIdentifier, sourceType)) {
        return object(subIdentifier);
    }
    return object();
}

static object 
_WrapGetSourceCode(const UsdShadeNodeDefAPI &shader,
                   const TfToken &sourceType)
{
    std::string code;
    if (shader.GetSourceCode(&code, sourceType)) {
        return object(code);
    }
    return object();
}

WRAP_CUSTOM {
    _class
        .def("GetImplementationSource", &UsdShadeNodeDefAPI::GetImplementationSource)
        .def("SetShaderId", &UsdShadeNodeDefAPI::SetShaderId)
        .def("SetSourceAsset", &UsdShadeNodeDefAPI::SetSourceAsset,
            (arg("sourceAsset"), 
             arg("sourceType")=UsdShadeTokens->universalSourceType))
        .def("SetSourceAssetSubIdentifier",
            &UsdShadeNodeDefAPI::SetSourceAssetSubIdentifier,
            (arg("subIdentifier"),
             arg("sourceType")=UsdShadeTokens->universalSourceType))
        .def("SetSourceCode", &UsdShadeNodeDefAPI::SetSourceCode,
            (arg("sourceCode"), 
             arg("sourceType")=UsdShadeTokens->universalSourceType))
        .def("GetShaderId", _WrapGetShaderId)
        .def("GetSourceAsset", _WrapGetSourceAsset, 
             arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceAssetSubIdentifier", _WrapGetSourceAssetSubIdentifier,
             arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceCode", _WrapGetSourceCode, 
             arg("sourceType")=UsdShadeTokens->universalSourceType)
        .def("GetSourceTypes", 
             &UsdShadeNodeDefAPI::GetSourceTypes)
        .def("GetShaderNodeForSourceType", 
             &UsdShadeNodeDefAPI::GetShaderNodeForSourceType,
             (arg("sourceType")),
             return_internal_reference<>())
        ;
}

}
