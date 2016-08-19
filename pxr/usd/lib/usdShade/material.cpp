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

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeMaterial,
        TfType::Bases< UsdShadeSubgraph > >();
    
    // Register the usd prim typename to associate it with the TfType, under
    // UsdSchemaBase. This enables one to call TfType::FindByName("Material") to find
    // TfType<UsdShadeMaterial>, which is how IsA queries are answered.
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
    if (not stage) {
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
    if (not stage) {
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
        UsdShadeSubgraph::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/base/tf/envSetting.h"

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (material)
    ((bindingRelationshipName, "material:binding"))
    ((legacyBindingRelationshipName, "look:binding"))
    ((materialVariantName, "materialVariant"))
    ((derivesFromName, "derivesFrom"))
    ((surfaceTerminal, "surface"))
    ((displacementTerminal, "displacement"))
);

TF_DEFINE_ENV_SETTING(
    USD_HONOR_LEGACY_USD_LOOK, true,
    "If on, keep reading look bindings when material bindings are missing.");


static 
UsdRelationship
_CreateBindingRel(UsdPrim& prim)
{
    return prim.CreateRelationship(_tokens->bindingRelationshipName,
                                   /* custom = */ false);
}

bool 
UsdShadeMaterial::Bind(UsdPrim& prim) const
{
    // We cannot enforce this test because we do not always know at authoring
    // time what we are binding to.
    
    // if (!prim.IsA<UsdGeomImageable>()) {
    //     TF_CODING_ERROR("Trying to bind a prim that is not Imageable: %s",
    //         prim.GetPath().GetString().c_str());
    //     return;
    // }
    ;

    // delete old relationship, if any
    UsdRelationship oldRel = 
        prim.GetRelationship(_tokens->legacyBindingRelationshipName);
    if (oldRel) {
        oldRel.BlockTargets();
    }

    if (UsdRelationship rel = _CreateBindingRel(prim)){
        SdfPathVector  targets(1, GetPath());
        return rel.SetTargets(targets);
    }

    return false;
}

bool 
UsdShadeMaterial::Unbind(UsdPrim& prim)
{
    // delete old relationship too, if any
    UsdRelationship oldRel = 
        prim.GetRelationship(_tokens->legacyBindingRelationshipName);
    if (oldRel) {
        oldRel.BlockTargets();
    }

    return _CreateBindingRel(prim).BlockTargets();
}

UsdRelationship
UsdShadeMaterial::GetBindingRel(const UsdPrim& prim)
{
    UsdRelationship rel = prim.GetRelationship(_tokens->bindingRelationshipName);
    if (TfGetEnvSetting(USD_HONOR_LEGACY_USD_LOOK)) {
        if (not rel) {
            // honor legacy assets using UsdShadeLook
            return prim.GetRelationship(_tokens->legacyBindingRelationshipName);
        }
    }
    return rel;
}

UsdShadeMaterial
UsdShadeMaterial::GetBoundMaterial(const UsdPrim &prim)
{
    if (UsdRelationship rel = UsdShadeMaterial::GetBindingRel(prim)) {
        SdfPathVector targetPaths;
        rel.GetForwardedTargets(&targetPaths);
        if ((targetPaths.size() == 1) and targetPaths.front().IsPrimPath()) {
            return UsdShadeMaterial(
                prim.GetStage()->GetPrimAtPath(targetPaths.front()));
        }
    }
    return UsdShadeMaterial();
}

std::pair<UsdStagePtr, UsdEditTarget >
UsdShadeMaterial::GetEditContextForVariant(const TfToken &materialVariation,
                                       const SdfLayerHandle &layer) const
{
    // First make sure localLayer belongs to the prim's stage
    UsdPrim         prim = GetPrim();
    UsdStageWeakPtr stage = prim.GetStage();
    
    UsdVariantSet materialVariant = prim.GetVariantSet(_tokens->materialVariantName);
    UsdEditTarget target = stage->GetEditTarget();
    if (materialVariant.FindOrCreateVariant(materialVariation) and
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

    while (not path.IsRootPrimPath())
        path = path.GetParentPath();

    return path;
}

UsdVariantSet
UsdShadeMaterial::GetMaterialVariant() const
{
    return GetPrim().GetVariantSet(_tokens->materialVariantName);
}

/* static */
bool
UsdShadeMaterial::CreateMasterMaterialVariant(const UsdPrim &masterPrim,
                                      const std::vector<UsdPrim> &materials,
                                      const TfToken &masterVariantSetName)
{
    if (not masterPrim){
        TF_CODING_ERROR("MasterPrim is not a valid UsdPrim.");
        return false;
    }
    TfToken  masterSetName = masterVariantSetName.IsEmpty() ? 
        _tokens->materialVariantName : masterVariantSetName;
    UsdStagePtr  stage = masterPrim.GetStage();
    std::vector<std::string>  allMaterialVariants;
    
    // Error Checking!
    if (materials.size() == 0){
        TF_CODING_ERROR("No material prims specified on which to operate.");
        return false;
    }
    TF_FOR_ALL(material, materials){
        if (not *material){
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
            material->GetVariantSet(_tokens->materialVariantName).GetVariantNames();
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
        if (not masterSet.FindOrCreateVariant(*varName)){
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
                if (not *material){
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
                    material->GetVariantSet(_tokens->materialVariantName).
                        SetVariantSelection(*varName);
                }
                else {
                    SdfPath derivedPath = material->GetPrimPath().
                        ReplacePrefix(_GetRootPath(*material), masterPrim.GetPath());
                    if (UsdPrim over = stage->OverridePrim(derivedPath)) {
                        over.GetVariantSet(_tokens->materialVariantName).
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

UsdShadeMaterial
UsdShadeMaterial::GetBaseMaterial() const 
{
    SdfPath basePath = GetBaseMaterialPath();
    if (!basePath.IsEmpty()) {
        return UsdShadeMaterial(GetPrim().GetStage()->GetPrimAtPath(basePath));
    }
    return UsdShadeMaterial();
}

SdfPath
UsdShadeMaterial::GetBaseMaterialPath() const 
{
    UsdRelationship baseRel = GetPrim().GetRelationship(
            _tokens->derivesFromName);
    if (baseRel.IsValid()) {
        SdfPathVector targets;
        baseRel.GetTargets(&targets);
        if (targets.size() == 1) {
            return targets[0];
        }
    }
    return SdfPath();
}

void
UsdShadeMaterial::SetBaseMaterialPath(const SdfPath& baseMaterialPath) const 
{
    UsdRelationship baseRel = GetPrim().CreateRelationship(
        _tokens->derivesFromName, /* custom = */ false);

    if (!baseMaterialPath.IsEmpty()) {
        SdfPathVector targets(1, baseMaterialPath);
        baseRel.SetTargets(targets);
    } else {
        baseRel.ClearTargets(false);
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
    return faceSet.GetIsPartitionAttr().Get(&isPartition) and isPartition;
}

UsdRelationship
UsdShadeMaterial::GetSurfaceTerminal() const
{
    return GetTerminal(_tokens->surfaceTerminal);
}

UsdRelationship
UsdShadeMaterial::CreateSurfaceTerminal(const SdfPath& targetPath) const
{
    return CreateTerminal(_tokens->surfaceTerminal, targetPath);
}

UsdRelationship
UsdShadeMaterial::GetDisplacementTerminal() const
{
    return GetTerminal(_tokens->displacementTerminal);
}

UsdRelationship
UsdShadeMaterial::CreateDisplacementTerminal(const SdfPath& targetPath) const
{
    return CreateTerminal(_tokens->displacementTerminal, targetPath);
}
