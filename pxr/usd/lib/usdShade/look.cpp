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
#include "pxr/usd/usdShade/look.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeLook,
        TfType::Bases< UsdShadeMaterial > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Look")
    // to find TfType<UsdShadeLook>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdShadeLook>("Look");
}

/* virtual */
UsdShadeLook::~UsdShadeLook()
{
}

/* static */
UsdShadeLook
UsdShadeLook::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeLook();
    }
    return UsdShadeLook(stage->GetPrimAtPath(path));
}

/* static */
UsdShadeLook
UsdShadeLook::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Look");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeLook();
    }
    return UsdShadeLook(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdShadeLook::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeLook>();
    return tfType;
}

/* static */
bool 
UsdShadeLook::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeLook::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdShadeLook::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdShadeMaterial::GetSchemaAttributeNames(true);

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

#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usdShade/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (look)
    ((lookVariantName, "lookVariant"))
);

static 
UsdRelationship
_CreateBindingRel(UsdPrim& prim)
{
    return prim.CreateRelationship(UsdShadeTokens->lookBinding,
                                   /* custom = */ false);
}

bool 
UsdShadeLook::Bind(UsdPrim& prim) const
{
    // We cannot enforce this test because we do not always know at authoring
    // time what we are binding to.
    
    // if (!prim.IsA<UsdGeomImageable>()) {
    //     TF_CODING_ERROR("Trying to bind a prim that is not Imageable: %s",
    //         prim.GetPath().GetString().c_str());
    //     return;
    // }
    ;
    if (UsdRelationship rel = _CreateBindingRel(prim)){
        SdfPathVector  targets(1, GetPath());
        return rel.SetTargets(targets);
    }

    return false;
}

bool 
UsdShadeLook::Unbind(UsdPrim& prim)
{
    return _CreateBindingRel(prim).BlockTargets();
}

UsdRelationship
UsdShadeLook::GetBindingRel(const UsdPrim& prim)
{
    return prim.GetRelationship(UsdShadeTokens->lookBinding);
}

UsdShadeLook
UsdShadeLook::GetBoundLook(const UsdPrim &prim)
{
    if (UsdRelationship rel = UsdShadeLook::GetBindingRel(prim)) {
        SdfPathVector targetPaths;
        rel.GetForwardedTargets(&targetPaths);
        if ((targetPaths.size() == 1) && targetPaths.front().IsPrimPath()) {
            return UsdShadeLook(
                prim.GetStage()->GetPrimAtPath(targetPaths.front()));
        }
    }
    return UsdShadeLook();
}

std::pair<UsdStagePtr, UsdEditTarget >
UsdShadeLook::GetEditContextForVariant(const TfToken &lookVariation,
                                       const SdfLayerHandle &layer) const
{
    // First make sure localLayer belongs to the prim's stage
    UsdPrim         prim = GetPrim();
    UsdStageWeakPtr stage = prim.GetStage();
    
    UsdVariantSet lookVariant = prim.GetVariantSet(_tokens->lookVariantName);
    UsdEditTarget target = stage->GetEditTarget();
    if (lookVariant.AddVariant(lookVariation) && 
        lookVariant.SetVariantSelection(lookVariation)) {
        target = lookVariant.GetVariantEditTarget(layer);
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
UsdShadeLook::GetLookVariant() const
{
    return GetPrim().GetVariantSet(_tokens->lookVariantName);
}

/* static */
bool
UsdShadeLook::CreateMasterLookVariant(const UsdPrim &masterPrim,
                                      const std::vector<UsdPrim> &looks,
                                      const TfToken &masterVariantSetName)
{
    if (!masterPrim){
        TF_CODING_ERROR("MasterPrim is not a valid UsdPrim.");
        return false;
    }
    TfToken  masterSetName = masterVariantSetName.IsEmpty() ? 
        _tokens->lookVariantName : masterVariantSetName;
    UsdStagePtr  stage = masterPrim.GetStage();
    std::vector<std::string>  allLookVariants;
    
    // Error Checking!
    if (looks.size() == 0){
        TF_CODING_ERROR("No look prims specified on which to operate.");
        return false;
    }
    TF_FOR_ALL(look, looks){
        if (!*look){
            TF_CODING_ERROR("Unable to process invalid look: %s",
                            look->GetDescription().c_str());
            return false;
        }
        if (stage != look->GetStage()){
            TF_CODING_ERROR("All look prims to be controlled by masterPrim "
                            "%s must originate on the same UsdStage as "
                            "masterPrim.  Prim %s does not.",
                            masterPrim.GetPath().GetText(),
                            look->GetPrimPath().GetText());
            return false;
        }

        std::vector<std::string>   lookVariants = 
            look->GetVariantSet(_tokens->lookVariantName).GetVariantNames();
        if (lookVariants.size() == 0){
            TF_CODING_ERROR("All Look prims to be switched by master "
                            "lookVariant must actually possess a "
                            "non-empty lookVariant themselves.  %s does not.",
                            look->GetPrimPath().GetText());
            return false;
        }
        
        if (allLookVariants.size() == 0){
            allLookVariants.swap(lookVariants);
        }
        else if (allLookVariants != lookVariants){
            TF_CODING_ERROR("All Look prims to be switched by master "
                            "lookVariant must possess the SAME look variants. "
                            "%s has a different set of variants.",
                            look->GetPrimPath().GetText());
            return false;
        }
    }

    UsdVariantSet masterSet = masterPrim.GetVariantSet(masterSetName);
    TF_FOR_ALL(varName, allLookVariants){
        if (!masterSet.AddVariant(*varName)){
            TF_RUNTIME_ERROR("Unable to create Look variant %s on prim %s. "
                             "Aborting master lookVariant creation.",
                             varName->c_str(),
                             masterPrim.GetPath().GetText());
            return false;
        }
        masterSet.SetVariantSelection(*varName);

        {
            UsdEditContext  ctxt(masterSet.GetVariantEditContext());
            
            TF_FOR_ALL(look, looks){
                if (!*look){
                    // Somehow, switching the variant caused this prim
                    // to expire.
                    TF_RUNTIME_ERROR("Switching master variant %s to %s "
                                     "caused one or more look prims to "
                                     "expire.  First such: %s.",
                                     masterSetName.GetText(),
                                     varName->c_str(),
                                     look->GetDescription().c_str());
                    return false;
                }

                // Here's the heart of the whole thing
                if (look->GetPath().HasPrefix(masterPrim.GetPath())){
                    look->GetVariantSet(_tokens->lookVariantName).
                        SetVariantSelection(*varName);
                }
                else {
                    SdfPath derivedPath = look->GetPrimPath().
                        ReplacePrefix(_GetRootPath(*look), masterPrim.GetPath());
                    if (UsdPrim over = stage->OverridePrim(derivedPath)) {
                        over.GetVariantSet(_tokens->lookVariantName).
                            SetVariantSelection(*varName);
                    }
                    else {
                        TF_RUNTIME_ERROR("Unable to create over for Look prim "
                                         "%s, so cannot set its lookVariant",
                                         derivedPath.GetText());
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

UsdShadeLook
UsdShadeLook::GetBaseLook() const 
{
    SdfPath basePath = GetBaseLookPath();
    if (!basePath.IsEmpty()) {
        return UsdShadeLook(GetPrim().GetStage()->GetPrimAtPath(basePath));
    }
    return UsdShadeLook();
}

SdfPath
UsdShadeLook::GetBaseLookPath() const 
{
    UsdRelationship baseRel = GetPrim().GetRelationship(
            UsdShadeTokens->derivesFrom);
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
UsdShadeLook::SetBaseLookPath(const SdfPath& baseLookPath) const 
{
    UsdRelationship baseRel = GetPrim().CreateRelationship(
        UsdShadeTokens->derivesFrom, /* custom = */ false);

    if (!baseLookPath.IsEmpty()) {
        SdfPathVector targets(1, baseLookPath);
        baseRel.SetTargets(targets);
    } else {
        baseRel.ClearTargets(false);
    }
}

void
UsdShadeLook::SetBaseLook(const UsdShadeLook& baseLook) const 
{
    UsdPrim basePrim = baseLook.GetPrim();
    if (basePrim.IsValid()) {
        SdfPath basePath = basePrim.GetPath();
        SetBaseLookPath(basePath);
    } else {
        SetBaseLookPath(SdfPath());
    }
}

void
UsdShadeLook::ClearBaseLook() const 
{
    SetBaseLookPath(SdfPath());
}

bool
UsdShadeLook::HasBaseLook() const 
{
    return !GetBaseLookPath().IsEmpty();
}

/* static */
UsdGeomFaceSetAPI 
UsdShadeLook::CreateLookFaceSet(const UsdPrim &prim)
{
    if (HasLookFaceSet(prim))
        return UsdGeomFaceSetAPI(prim, _tokens->look);

    // No face can be bound to more than one Look, hence set isPartition to 
    // true.
    UsdGeomFaceSetAPI faceSet(prim, _tokens->look);
    faceSet.SetIsPartition(true);

    return faceSet;
}

/* static */
UsdGeomFaceSetAPI 
UsdShadeLook::GetLookFaceSet(const UsdPrim &prim) 
{
    if (HasLookFaceSet(prim))
        return UsdGeomFaceSetAPI(prim, _tokens->look);

    return UsdGeomFaceSetAPI();
}

/* static */
bool 
UsdShadeLook::HasLookFaceSet(const UsdPrim &prim)
{
    UsdGeomFaceSetAPI faceSet(prim, _tokens->look);
    bool isPartition=false;
    return faceSet.GetIsPartitionAttr().Get(&isPartition) && isPartition;
}

PXR_NAMESPACE_CLOSE_SCOPE
