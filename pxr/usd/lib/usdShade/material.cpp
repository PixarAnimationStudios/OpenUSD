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

/*static*/
const TfTokenVector&
UsdShadeMaterial::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdShadeNodeGraph::GetSchemaAttributeNames(true);

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

#include "pxr/base/tf/envSetting.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (material)
);

TF_DEFINE_ENV_SETTING(
    USD_USE_LEGACY_BASE_MATERIAL, false,
    "If on, store base material as derivesFrom relationship.");

TF_DEFINE_ENV_SETTING(
    USD_HONOR_LEGACY_BASE_MATERIAL, true,
    "If on, read base material as derivesFrom relationship when available.");

bool 
UsdShadeMaterial::Bind(const UsdPrim& prim) const
{
    return UsdShadeMaterialBindingAPI(prim).Bind(*this);
}

bool 
UsdShadeMaterial::Unbind(const UsdPrim& prim)
{
    return UsdShadeMaterialBindingAPI(prim).UnbindDirectBinding();
}

UsdRelationship
UsdShadeMaterial::GetBindingRel(const UsdPrim& prim)
{
    return UsdShadeMaterialBindingAPI(prim).GetDirectBindingRel();
}

UsdShadeMaterial
UsdShadeMaterial::GetBoundMaterial(const UsdPrim &prim)
{
    return UsdShadeMaterialBindingAPI(prim).ComputeBoundMaterial();
}

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
// old vs new style controlled by env var:
// USD_USE_LEGACY_BASE_MATERIAL

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
    // first look for deriveFrom relationship
    if (TfGetEnvSetting(USD_HONOR_LEGACY_BASE_MATERIAL)) {
        UsdRelationship baseRel = GetPrim().GetRelationship(
                UsdShadeTokens->derivesFrom);
        if (baseRel.IsValid()) {
            SdfPathVector targets;
            baseRel.GetTargets(&targets);
            if (targets.size() == 1) {
                return targets[0];
            }
        }
    }

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
        if (PcpIsSpecializesArc(node.GetArcType())) {
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
    if (TfGetEnvSetting(USD_USE_LEGACY_BASE_MATERIAL)) {
        UsdRelationship baseRel = GetPrim().CreateRelationship(
            UsdShadeTokens->derivesFrom, /* custom = */ false);

        if (!baseMaterialPath.IsEmpty()) {
            SdfPathVector targets(1, baseMaterialPath);
            baseRel.SetTargets(targets);
        } else {
            baseRel.ClearTargets(false);
        }
    }
    else {
        // Only one specialize is allowed
        UsdSpecializes specializes = GetPrim().GetSpecializes();
        if (!baseMaterialPath.IsEmpty()) {
            SdfPathVector v;
            v.push_back(baseMaterialPath);
            specializes.SetSpecializes(v);
        }
        else {
            specializes.ClearSpecializes();
        }
    }
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

/* static */
UsdGeomSubset 
UsdShadeMaterial::CreateMaterialBindSubset(
    const UsdGeomImageable &geom,
    const TfToken &subsetName,
    const VtIntArray &indices,
    const TfToken &elementType)
{
    UsdGeomSubset result = UsdGeomSubset::CreateGeomSubset(geom, subsetName, 
        elementType, indices, UsdShadeTokens->materialBind);

    TfToken familyType = UsdGeomSubset::GetFamilyType(geom, 
        UsdShadeTokens->materialBind);
    // Subsets that have materials bound to them should have 
    // mutually exclusive sets of indices. Hence, set the familyType 
    // to "nonOverlapping" if it's unset (or explicitly set to unrestricted).
    if (familyType == UsdGeomTokens->unrestricted) {
        SetMaterialBindSubsetsFamilyType(geom, UsdGeomTokens->nonOverlapping);
    }

    return result;
}


/* static */
std::vector<UsdGeomSubset> 
UsdShadeMaterial::GetMaterialBindSubsets(
    const UsdGeomImageable &geom)
{
    return UsdGeomSubset::GetGeomSubsets(geom, /* elementType */ TfToken(), 
        UsdShadeTokens->materialBind);
}

/* static */
bool UsdShadeMaterial::SetMaterialBindSubsetsFamilyType(
        const UsdGeomImageable &geom,
        const TfToken &familyType)
{
    return UsdGeomSubset::SetFamilyType(geom, UsdShadeTokens->materialBind,
        familyType);
}

/* static */
TfToken
UsdShadeMaterial::GetMaterialBindSubsetsFamilyType(
        const UsdGeomImageable &geom)
{
    return UsdGeomSubset::GetFamilyType(geom, UsdShadeTokens->materialBind);
}
   
// --------------------------------------------------------------------- //

/* static */
UsdGeomFaceSetAPI 
UsdShadeMaterial::CreateMaterialFaceSet(const UsdPrim &prim)
{
    if (HasMaterialFaceSet(prim))
        return UsdGeomFaceSetAPI(prim, _tokens->material);

    // No face can be bound to more than one Material, hence set isPartition to 
    // true.
    UsdGeomFaceSetAPI faceSet(prim, _tokens->material);
    faceSet.SetIsPartition(true);

    return faceSet;
}

/* static */
UsdGeomFaceSetAPI 
UsdShadeMaterial::GetMaterialFaceSet(const UsdPrim &prim) 
{
    if (HasMaterialFaceSet(prim))
        return UsdGeomFaceSetAPI(prim, _tokens->material);

    return UsdGeomFaceSetAPI();
}

/* static */
bool 
UsdShadeMaterial::HasMaterialFaceSet(const UsdPrim &prim)
{
    UsdGeomFaceSetAPI faceSet(prim, _tokens->material);
    bool isPartition=false;
    return faceSet.GetIsPartitionAttr().Get(&isPartition) && isPartition;
}

PXR_NAMESPACE_CLOSE_SCOPE
