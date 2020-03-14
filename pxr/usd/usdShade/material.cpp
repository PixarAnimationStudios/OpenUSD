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
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeMaterial,
        TfType::Bases< UsdShadeNodeGraph > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Material")
    // to find TfType<UsdShadeMaterial>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdShadeMaterial>("Material");
}

/* virtual */
UsdShadeMaterial::~UsdShadeMaterial()
{
}

/* static */
UsdShadeMaterial
UsdShadeMaterial::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeMaterial();
    }
    return UsdShadeMaterial(stage->GetPrimAtPath(path));
}

/* static */
UsdShadeMaterial
UsdShadeMaterial::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Material");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeMaterial();
    }
    return UsdShadeMaterial(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdShadeMaterial::_GetSchemaType() const {
    return UsdShadeMaterial::schemaType;
}

/* static */
const TfType &
UsdShadeMaterial::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeMaterial>();
    return tfType;
}

/* static */
bool 
UsdShadeMaterial::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeMaterial::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdShadeMaterial::GetSurfaceAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->outputsSurface);
}

UsdAttribute
UsdShadeMaterial::CreateSurfaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->outputsSurface,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdShadeMaterial::GetDisplacementAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->outputsDisplacement);
}

UsdAttribute
UsdShadeMaterial::CreateDisplacementAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->outputsDisplacement,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdShadeMaterial::GetVolumeAttr() const
{
    return GetPrim().GetAttribute(UsdShadeTokens->outputsVolume);
}

UsdAttribute
UsdShadeMaterial::CreateVolumeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdShadeTokens->outputsVolume,
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
UsdShadeMaterial::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdShadeTokens->outputsSurface,
        UsdShadeTokens->outputsDisplacement,
        UsdShadeTokens->outputsVolume,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdShadeNodeGraph::GetSchemaAttributeNames(true),
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

#include "pxr/usd/pcp/mapExpression.h"

#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/variantSets.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (material)
);


std::pair<UsdStagePtr, UsdEditTarget >
UsdShadeMaterial::GetEditContextForVariant(const TfToken &materialVariation,
                                       const SdfLayerHandle &layer) const
{
    // First make sure localLayer belongs to the prim's stage
    UsdPrim         prim = GetPrim();
    UsdStageWeakPtr stage = prim.GetStage();
    
    UsdVariantSet materialVariant = prim.GetVariantSet(
            UsdShadeTokens->materialVariant);
    UsdEditTarget target = stage->GetEditTarget();
    if (materialVariant.AddVariant(materialVariation) && 
        materialVariant.SetVariantSelection(materialVariation)) {
        target = materialVariant.GetVariantEditTarget(layer);
    }

    return std::make_pair(GetPrim().GetStage(), target);
}

// Somewhat surprised this isn't a method of SdfPath...
static
SdfPath
_GetRootPath(const UsdPrim & prim)
{
    SdfPath path = prim.GetPrimPath();

    // special-case pseudo-root
    if (path == SdfPath::AbsoluteRootPath())
        return path;

    while (!path.IsRootPrimPath())
        path = path.GetParentPath();

    return path;
}

UsdVariantSet
UsdShadeMaterial::GetMaterialVariant() const
{
    return GetPrim().GetVariantSet(UsdShadeTokens->materialVariant);
}

/* static */
bool
UsdShadeMaterial::CreateMasterMaterialVariant(const UsdPrim &masterPrim,
                                      const std::vector<UsdPrim> &materials,
                                      const TfToken &masterVariantSetName)
{
    if (!masterPrim){
        TF_CODING_ERROR("MasterPrim is not a valid UsdPrim.");
        return false;
    }
    TfToken  masterSetName = masterVariantSetName.IsEmpty() ? 
        UsdShadeTokens->materialVariant : masterVariantSetName;
    UsdStagePtr  stage = masterPrim.GetStage();
    std::vector<std::string>  allMaterialVariants;
    
    // Error Checking!
    if (materials.size() == 0){
        TF_CODING_ERROR("No material prims specified on which to operate.");
        return false;
    }
    TF_FOR_ALL(material, materials){
        if (!*material){
            TF_CODING_ERROR("Unable to process invalid material: %s",
                            material->GetDescription().c_str());
            return false;
        }
        if (stage != material->GetStage()){
            TF_CODING_ERROR("All material prims to be controlled by masterPrim "
                            "%s must originate on the same UsdStage as "
                            "masterPrim.  Prim %s does not.",
                            masterPrim.GetPath().GetText(),
                            material->GetPrimPath().GetText());
            return false;
        }

        std::vector<std::string>   materialVariants = 
            material->GetVariantSet(
                    UsdShadeTokens->materialVariant).GetVariantNames();
        if (materialVariants.size() == 0){
            TF_CODING_ERROR("All Material prims to be switched by master "
                            "materialVariant must actually possess a "
                            "non-empty materialVariant themselves.  %s does not.",
                            material->GetPrimPath().GetText());
            return false;
        }
        
        if (allMaterialVariants.size() == 0){
            allMaterialVariants.swap(materialVariants);
        }
        else if (allMaterialVariants != materialVariants){
            TF_CODING_ERROR("All Material prims to be switched by master "
                            "materialVariant must possess the SAME material variants. "
                            "%s has a different set of variants.",
                            material->GetPrimPath().GetText());
            return false;
        }
    }

    UsdVariantSet masterSet = masterPrim.GetVariantSet(masterSetName);
    TF_FOR_ALL(varName, allMaterialVariants){
        if (!masterSet.AddVariant(*varName)){
            TF_RUNTIME_ERROR("Unable to create Material variant %s on prim %s. "
                             "Aborting master materialVariant creation.",
                             varName->c_str(),
                             masterPrim.GetPath().GetText());
            return false;
        }
        masterSet.SetVariantSelection(*varName);

        {
            UsdEditContext  ctxt(masterSet.GetVariantEditContext());
            
            TF_FOR_ALL(material, materials){
                if (!*material){
                    // Somehow, switching the variant caused this prim
                    // to expire.
                    TF_RUNTIME_ERROR("Switching master variant %s to %s "
                                     "caused one or more material prims to "
                                     "expire.  First such: %s.",
                                     masterSetName.GetText(),
                                     varName->c_str(),
                                     material->GetDescription().c_str());
                    return false;
                }

                // Here's the heart of the whole thing
                if (material->GetPath().HasPrefix(masterPrim.GetPath())){
                    material->GetVariantSet(
                            UsdShadeTokens->materialVariant).
                        SetVariantSelection(*varName);
                }
                else {
                    SdfPath derivedPath = material->GetPrimPath().
                        ReplacePrefix(_GetRootPath(*material), masterPrim.GetPath());
                    if (UsdPrim over = stage->OverridePrim(derivedPath)) {
                        over.GetVariantSet(
                                UsdShadeTokens->materialVariant).
                            SetVariantSelection(*varName);
                    }
                    else {
                        TF_RUNTIME_ERROR("Unable to create over for Material prim "
                                         "%s, so cannot set its materialVariant",
                                         derivedPath.GetText());
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

// --------------------------------------------------------------------- //

static
UsdShadeMaterial
_GetMaterialAtPath(
        const UsdPrim & prim,
        const SdfPath & path)
{
    if (prim && !path.IsEmpty()) {
        auto material =
            UsdShadeMaterial(prim.GetStage()->GetPrimAtPath(path));
        if (material) {
            return material;
        }
    }
    return UsdShadeMaterial();

}

UsdShadeMaterial
UsdShadeMaterial::GetBaseMaterial() const 
{
    return _GetMaterialAtPath(GetPrim(), GetBaseMaterialPath());
}

SdfPath
UsdShadeMaterial::GetBaseMaterialPath() const 
{
    SdfPath parentMaterialPath = FindBaseMaterialPathInPrimIndex(
        GetPrim().GetPrimIndex(), [=](const SdfPath &p) {
            return bool(_GetMaterialAtPath(GetPrim(), p));
        });

    if (parentMaterialPath != SdfPath::EmptyPath()) {
        UsdPrim p = GetPrim().GetStage()->GetPrimAtPath(parentMaterialPath);
        if (p.IsInstanceProxy()) {
            // this looks like an instance but it's acting as the master path.
            // Return the master path
            parentMaterialPath = p.GetPrimInMaster().GetPath();
        }
    }
    return parentMaterialPath;
}

/* static */
SdfPath
UsdShadeMaterial::FindBaseMaterialPathInPrimIndex(
        const PcpPrimIndex & primIndex,
        const PathPredicate & pathIsMaterialPredicate)
{
    for(const PcpNodeRef &node: primIndex.GetNodeRange()) {
        if (PcpIsSpecializeArc(node.GetArcType())) {
            // We only consider children of the prim's root node because any
            // specializes arc we care about that is authored inside referenced
            // scene description will "imply" up into the root layer stack.
            // This enables us to trim our search space, potentially
            // significantly.
            if (node.GetParentNode() != node.GetRootNode()) {
                continue;
            }

            if (node.GetMapToParent().MapSourceToTarget(
                SdfPath::AbsoluteRootPath()).IsEmpty()) {
                // Skip this child node because it crosses a reference arc.
                // (Reference mappings never map the absolute root path </>.)
                continue;
            }

            // stop at the first one that's a material
            const SdfPath & path = node.GetPath();
            if (pathIsMaterialPredicate(path)) {
                return path;
            }
        }
    }
    return SdfPath();
}

void
UsdShadeMaterial::SetBaseMaterialPath(const SdfPath& baseMaterialPath) const 
{
    UsdSpecializes specializes = GetPrim().GetSpecializes();
    if (baseMaterialPath.IsEmpty()) {
        specializes.ClearSpecializes();
        return;
    }
    // Only one specialize is allowed
    SdfPathVector v = { baseMaterialPath };
    specializes.SetSpecializes(v);
}

void
UsdShadeMaterial::SetBaseMaterial(const UsdShadeMaterial& baseMaterial) const 
{
    UsdPrim basePrim = baseMaterial.GetPrim();
    if (basePrim.IsValid()) {
        SdfPath basePath = basePrim.GetPath();
        SetBaseMaterialPath(basePath);
    } else {
        SetBaseMaterialPath(SdfPath());
    }
}

void
UsdShadeMaterial::ClearBaseMaterial() const 
{
    SetBaseMaterialPath(SdfPath());
}

bool
UsdShadeMaterial::HasBaseMaterial() const 
{
    return !GetBaseMaterialPath().IsEmpty();
}

// --------------------------------------------------------------------- //
static 
TfToken 
_GetOutputName(const TfToken &baseName, const TfToken &renderContext)
{
    return TfToken(SdfPath::JoinIdentifier(renderContext, baseName));
}

bool
UsdShadeMaterial::_ComputeNamedOutputSource(
    const TfToken &baseName, 
    const TfToken &renderContext,
    UsdShadeConnectableAPI *source,
    TfToken *sourceName,
    UsdShadeAttributeType *sourceType) const
{
    const TfToken outputName = _GetOutputName(baseName, renderContext);
    UsdShadeOutput output = GetOutput(outputName);
    if (output) {
        if (renderContext == UsdShadeTokens->universalRenderContext && 
            !output.GetAttr().IsAuthored()) {
            return false;
        } else if (output.GetConnectedSource(source, sourceName, sourceType)) {
            return true;
        }
    }

    if (renderContext != UsdShadeTokens->universalRenderContext) {
        const TfToken universalOutputName = _GetOutputName(
                baseName, UsdShadeTokens->universalRenderContext);
        UsdShadeOutput universalOutput = GetOutput(universalOutputName);
        if (TF_VERIFY(universalOutput)) {
            if (renderContext == UsdShadeTokens->universalRenderContext && 
                !universalOutput.GetAttr().IsAuthored()) {
                return false;
            } else if (universalOutput.GetConnectedSource(source, sourceName, 
                    sourceType)) {
                return true;
            }
        }
    }

    return false;
}

UsdShadeShader 
UsdShadeMaterial::_ComputeNamedOutputShader(
    const TfToken &baseName,
    const TfToken &renderContext,
    TfToken *sourceName, 
    UsdShadeAttributeType *sourceType) const
{
    UsdShadeConnectableAPI source;
    TfToken srcName; 
    UsdShadeAttributeType srcType;
    if (_ComputeNamedOutputSource(baseName, renderContext, 
                                  &source, &srcName, &srcType)) {
        if (source.IsNodeGraph()) {
            source = UsdShadeNodeGraph(source.GetPrim()).ComputeOutputSource(
                srcName, &srcName, &srcType);
        }
        if (sourceName)
            *sourceName = srcName;
        if (sourceType)
            *sourceType = srcType;
    }
    return source;
}

UsdShadeOutput 
UsdShadeMaterial::CreateSurfaceOutput(const TfToken &renderContext) const
{
    return CreateOutput(_GetOutputName(UsdShadeTokens->surface, renderContext),
                        SdfValueTypeNames->Token);
}

UsdShadeOutput 
UsdShadeMaterial::GetSurfaceOutput(const TfToken &renderContext) const
{
    return GetOutput(_GetOutputName(UsdShadeTokens->surface, renderContext));
}

UsdShadeShader 
UsdShadeMaterial::ComputeSurfaceSource(
    const TfToken &renderContext,
    TfToken *sourceName, 
    UsdShadeAttributeType *sourceType) const
{
    return _ComputeNamedOutputShader(UsdShadeTokens->surface, 
            renderContext, sourceName, sourceType);
}

UsdShadeOutput 
UsdShadeMaterial::CreateDisplacementOutput(const TfToken &renderContext) const
{
    return CreateOutput(_GetOutputName(UsdShadeTokens->displacement, renderContext),
                        SdfValueTypeNames->Token);
}

UsdShadeOutput 
UsdShadeMaterial::GetDisplacementOutput(const TfToken &renderContext) const
{
    return GetOutput(_GetOutputName(UsdShadeTokens->displacement, renderContext));
}

UsdShadeShader 
UsdShadeMaterial::ComputeDisplacementSource(
    const TfToken &renderContext,
    TfToken *sourceName, 
    UsdShadeAttributeType *sourceType) const
{
    return _ComputeNamedOutputShader(UsdShadeTokens->displacement, 
            renderContext, sourceName, sourceType);
}

UsdShadeOutput 
UsdShadeMaterial::CreateVolumeOutput(const TfToken &renderContext) const
{
    return CreateOutput(_GetOutputName(UsdShadeTokens->volume, renderContext),
                        SdfValueTypeNames->Token);
}

UsdShadeOutput 
UsdShadeMaterial::GetVolumeOutput(const TfToken &renderContext) const
{
    return GetOutput(_GetOutputName(UsdShadeTokens->volume, renderContext));
}

UsdShadeShader 
UsdShadeMaterial::ComputeVolumeSource(
    const TfToken &renderContext,
    TfToken *sourceName, 
    UsdShadeAttributeType *sourceType) const
{
    return _ComputeNamedOutputShader(UsdShadeTokens->volume, renderContext, 
            sourceName, sourceType);
}


PXR_NAMESPACE_CLOSE_SCOPE
