//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRi/materialAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiMaterialAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdRiMaterialAPI::~UsdRiMaterialAPI()
{
}

/* static */
UsdRiMaterialAPI
UsdRiMaterialAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiMaterialAPI();
    }
    return UsdRiMaterialAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdRiMaterialAPI::_GetSchemaKind() const
{
    return UsdRiMaterialAPI::schemaKind;
}

/* static */
bool
UsdRiMaterialAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdRiMaterialAPI>(whyNot);
}

/* static */
UsdRiMaterialAPI
UsdRiMaterialAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdRiMaterialAPI>()) {
        return UsdRiMaterialAPI(prim);
    }
    return UsdRiMaterialAPI();
}

/* static */
const TfType &
UsdRiMaterialAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiMaterialAPI>();
    return tfType;
}

/* static */
bool 
UsdRiMaterialAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiMaterialAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiMaterialAPI::GetSurfaceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->outputsRiSurface);
}

UsdAttribute
UsdRiMaterialAPI::CreateSurfaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->outputsRiSurface,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiMaterialAPI::GetDisplacementAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->outputsRiDisplacement);
}

UsdAttribute
UsdRiMaterialAPI::CreateDisplacementAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->outputsRiDisplacement,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiMaterialAPI::GetVolumeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->outputsRiVolume);
}

UsdAttribute
UsdRiMaterialAPI::CreateVolumeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->outputsRiVolume,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdRiMaterialAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->outputsRiSurface,
        UsdRiTokens->outputsRiDisplacement,
        UsdRiTokens->outputsRiVolume,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/envSetting.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((defaultOutputName, "outputs:out"))
    (ri)

    // deprecated tokens for handling backwards compatibility.
    ((bxdfOutputName, "ri:bxdf"))
    ((bxdfOutputAttrName, "outputs:ri:bxdf"))
    ((riLookBxdf, "riLook:bxdf"))
);

UsdShadeShader
UsdRiMaterialAPI::_GetSourceShaderObject(const UsdShadeOutput &output,
                                         bool ignoreBaseMaterial) const
{
    // If output doesn't have a valid attribute, return an invalid shader.
    if (!output.GetAttr()) {
        return UsdShadeShader();
    }

    if (ignoreBaseMaterial && 
        UsdShadeConnectableAPI::IsSourceConnectionFromBaseMaterial(output)) {
        return UsdShadeShader();
    }

    UsdShadeConnectableAPI source;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;

    if (UsdShadeConnectableAPI::GetConnectedSource(output, 
            &source, &sourceName, &sourceType)) {
        return source;
    }

    return UsdShadeShader();
}

UsdShadeOutput
UsdRiMaterialAPI::_GetBxdfOutput(const UsdPrim &materialPrim) const
{
    // Check if the deprecated bxdf output is present.
    if (const UsdAttribute bxdfAttr = materialPrim.GetAttribute(
            _tokens->bxdfOutputAttrName)) {
        return UsdShadeOutput(bxdfAttr);
    }
    return UsdShadeOutput();    
}

UsdShadeShader
UsdRiMaterialAPI::GetSurface(bool ignoreBaseMaterial) const
{
    if (UsdShadeShader surface = 
            _GetSourceShaderObject(GetSurfaceOutput(), ignoreBaseMaterial)) {
        return surface;
    }

    if (UsdShadeOutput bxdfOutput = _GetBxdfOutput(GetPrim())) {
        return _GetSourceShaderObject(bxdfOutput, ignoreBaseMaterial);
    }

    return UsdShadeShader();
}

UsdShadeShader
UsdRiMaterialAPI::GetDisplacement(bool ignoreBaseMaterial) const
{
    return _GetSourceShaderObject(GetDisplacementOutput(), ignoreBaseMaterial);
}

UsdShadeShader 
UsdRiMaterialAPI::GetVolume(bool ignoreBaseMaterial) const
{
    return _GetSourceShaderObject(GetVolumeOutput(), ignoreBaseMaterial);    
}

UsdShadeOutput 
UsdRiMaterialAPI::GetSurfaceOutput() const
{
    return  UsdShadeMaterial(GetPrim()).GetSurfaceOutput(_tokens->ri);
}

UsdShadeOutput 
UsdRiMaterialAPI::GetDisplacementOutput() const
{
    return UsdShadeMaterial(GetPrim()).GetDisplacementOutput(_tokens->ri);
}

UsdShadeOutput 
UsdRiMaterialAPI::GetVolumeOutput() const
{
    return UsdShadeMaterial(GetPrim()).GetVolumeOutput(_tokens->ri);
}

bool
UsdRiMaterialAPI::SetSurfaceSource(const SdfPath &surfacePath) const
{
    UsdShadeOutput surfaceOutput = UsdShadeMaterial(GetPrim())
            .CreateSurfaceOutput(/*purpose*/ _tokens->ri);
    return UsdShadeConnectableAPI::ConnectToSource(
        surfaceOutput, surfacePath.IsPropertyPath() ? surfacePath :
            surfacePath.AppendProperty(_tokens->defaultOutputName));
}

bool
UsdRiMaterialAPI::SetDisplacementSource(const SdfPath &displacementPath) const
{
    UsdShadeOutput displacementOutput = UsdShadeMaterial(GetPrim())
            .CreateDisplacementOutput(/*purpose*/ _tokens->ri);
    return UsdShadeConnectableAPI::ConnectToSource(
        displacementOutput, displacementPath.IsPropertyPath() ? 
            displacementPath :
            displacementPath.AppendProperty(_tokens->defaultOutputName));
}

bool
UsdRiMaterialAPI::SetVolumeSource(const SdfPath &volumePath) const
{
    UsdShadeOutput volumeOutput = UsdShadeMaterial(GetPrim())
            .CreateVolumeOutput(/*purpose*/ _tokens->ri);
    return UsdShadeConnectableAPI::ConnectToSource(
        volumeOutput, volumePath.IsPropertyPath() ? 
            volumePath :
            volumePath.AppendProperty(_tokens->defaultOutputName));
}

UsdShadeNodeGraph::InterfaceInputConsumersMap
UsdRiMaterialAPI::ComputeInterfaceInputConsumersMap(
        bool computeTransitiveConsumers) const
{
    return UsdShadeNodeGraph(GetPrim()).ComputeInterfaceInputConsumersMap(
        computeTransitiveConsumers);
}

PXR_NAMESPACE_CLOSE_SCOPE
