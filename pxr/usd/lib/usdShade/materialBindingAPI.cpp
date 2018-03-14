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
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeMaterialBindingAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (MaterialBindingAPI)
);

/* virtual */
UsdShadeMaterialBindingAPI::~UsdShadeMaterialBindingAPI()
{
}

/* static */
UsdShadeMaterialBindingAPI
UsdShadeMaterialBindingAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeMaterialBindingAPI();
    }
    return UsdShadeMaterialBindingAPI(stage->GetPrimAtPath(path));
}


/* static */
UsdShadeMaterialBindingAPI
UsdShadeMaterialBindingAPI::Apply(const UsdPrim &prim)
{
    return UsdAPISchemaBase::_ApplyAPISchema<UsdShadeMaterialBindingAPI>(
            prim, _schemaTokens->MaterialBindingAPI);
}

/* static */
const TfType &
UsdShadeMaterialBindingAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeMaterialBindingAPI>();
    return tfType;
}

/* static */
bool 
UsdShadeMaterialBindingAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeMaterialBindingAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdShadeMaterialBindingAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((materialBindingFull, "material:binding:full"))
    ((materialBindingPreview, "material:binding:preview"))

    ((materialBindingCollectionFull, "material:binding:collection:full"))
    ((materialBindingCollectionPreview, "material:binding:collection:preview"))
);

TF_DEFINE_ENV_SETTING(USD_SHADE_WARN_ON_LOOK_BINDING, true, 
    "When set to true, it causes a warning to be issued if we find an a prim "
    "with the deprecated\"look:binding\" relationship when computing resolved "
    "material bindings. Although a warning is issued, these relationships are "
    "no longer considered in the binding resolution. The warning exists solely "
    "for the purpose of assisting clients in identifying deprecated assets and "
    "debugging missing bindings.");

static 
TfToken
_GetDirectBindingRelName(const TfToken &materialPurpose) 
{
    //  Optimize for the three common values of materialPurpose.
    if (materialPurpose == UsdShadeTokens->allPurpose) {
        return UsdShadeTokens->materialBinding;
    } else if (materialPurpose == UsdShadeTokens->preview) {
        return _tokens->materialBindingPreview;
    } else if (materialPurpose == UsdShadeTokens->full) {
        return _tokens->materialBindingFull;
    }
    return TfToken(SdfPath::JoinIdentifier(UsdShadeTokens->materialBinding,     
                                           materialPurpose));
}

static 
TfToken
_GetCollectionBindingRelName(const TfToken &bindingName, 
                             const TfToken &materialPurpose)
{
    //  Optimize for the three common values of materialPurpose.
    if (materialPurpose == UsdShadeTokens->allPurpose) {
        return TfToken(SdfPath::JoinIdentifier(
            UsdShadeTokens->materialBindingCollection, bindingName));
    } else if (materialPurpose == UsdShadeTokens->preview) {
        return TfToken(SdfPath::JoinIdentifier(
            _tokens->materialBindingCollectionPreview, bindingName));
    } else if (materialPurpose == UsdShadeTokens->full) {
        return TfToken(SdfPath::JoinIdentifier(
            _tokens->materialBindingCollectionFull, bindingName));
    }
    return TfToken(SdfPath::JoinIdentifier(std::vector<TfToken>{
            UsdShadeTokens->materialBindingCollection,
            materialPurpose, bindingName}));
}

UsdRelationship
UsdShadeMaterialBindingAPI::GetDirectBindingRel(
    const TfToken &materialPurpose) const
{
    return GetPrim().GetRelationship(_GetDirectBindingRelName(materialPurpose));
}

UsdRelationship 
UsdShadeMaterialBindingAPI::GetCollectionBindingRel(
    const TfToken &bindingName,
    const TfToken &materialPurpose) const
{
    return GetPrim().GetRelationship(
        _GetCollectionBindingRelName(bindingName, materialPurpose));
}

// Returns the material purpose associated with the given binding relationship. 
// This returns UsdShadeTokens->allPurpose if the binding relationship does not 
// apply to a specific material purpose.
static
TfToken 
_GetMaterialPurpose(const UsdRelationship &bindingRel)
{
    std::vector<std::string> nameTokens = bindingRel.SplitName();
    if (nameTokens.size() == 5) {
        return TfToken(nameTokens[3]);
    } else if (nameTokens.size() == 3) {
        return TfToken(nameTokens[2]);
    }
    return UsdShadeTokens->allPurpose;
}


UsdShadeMaterialBindingAPI::DirectBinding::DirectBinding(
    const UsdRelationship &directBindingRel):
    _bindingRel(directBindingRel),
    _materialPurpose(_GetMaterialPurpose(directBindingRel))
{
    SdfPathVector targetPaths;
    _bindingRel.GetForwardedTargets(&targetPaths);
    if (targetPaths.size() == 1 && 
        targetPaths.front().IsPrimPath()) {
        _materialPath = targetPaths.front();
    }
}

UsdShadeMaterial
UsdShadeMaterialBindingAPI::DirectBinding::GetMaterial() const
{
    if (!_materialPath.IsEmpty()) {
        return UsdShadeMaterial(_bindingRel.GetStage()->GetPrimAtPath(
                _materialPath));
    }
    return UsdShadeMaterial();
}

UsdShadeMaterialBindingAPI::DirectBinding
UsdShadeMaterialBindingAPI::GetDirectBinding(
    const TfToken &materialPurpose) const
{
    UsdRelationship directBindingRel = GetDirectBindingRel(materialPurpose);
    return DirectBinding(directBindingRel);
}

std::vector<UsdRelationship> 
UsdShadeMaterialBindingAPI::GetCollectionBindingRels(
        const TfToken &materialPurpose) const
{
    std::vector<UsdProperty> collectionBindingProperties = 
        GetPrim().GetAuthoredPropertiesInNamespace(
            _GetCollectionBindingRelName(TfToken(), materialPurpose));

    std::vector<UsdRelationship> result;
    for (const UsdProperty &prop : collectionBindingProperties) {
        if (prop.Is<UsdRelationship>()) {
            UsdRelationship rel = prop.As<UsdRelationship>();            
            if (_GetMaterialPurpose(rel) == materialPurpose) {
                result.push_back(prop.As<UsdRelationship>());
            }
        }
    }

    return result;
}

UsdShadeMaterialBindingAPI::CollectionBinding::CollectionBinding(
    const UsdRelationship &collBindingRel):
    _bindingRel(collBindingRel)
{
    SdfPathVector targetPaths;
    collBindingRel.GetForwardedTargets(&targetPaths);
    // A collection binding relationship must have exactly two targets 
    // One of them should target a property path (i.e. the collection path)
    // and the other must target a prim (the bound material).
    if (targetPaths.size() == 2) {
        bool firstTargetPathIsPrimPath = targetPaths[0].IsPrimPath();
        bool secondTargetPathIsPrimPath = targetPaths[1].IsPrimPath(); 

        if (firstTargetPathIsPrimPath ^ secondTargetPathIsPrimPath) {
            _materialPath = targetPaths[firstTargetPathIsPrimPath ? 0 : 1];
            _collectionPath =  targetPaths[firstTargetPathIsPrimPath ? 1 : 0];
        }
    }
}

UsdShadeMaterial
UsdShadeMaterialBindingAPI::CollectionBinding::GetMaterial() const
{
    if (!_materialPath.IsEmpty()) {
        return UsdShadeMaterial(_bindingRel.GetStage()->GetPrimAtPath(
                _materialPath));
    }
    return UsdShadeMaterial();    
}

UsdCollectionAPI
UsdShadeMaterialBindingAPI::CollectionBinding::GetCollection() const
{
    if (!_collectionPath.IsEmpty()) {
        return UsdCollectionAPI::GetCollection(_bindingRel.GetStage(),
                                               _collectionPath);
    }
    return UsdCollectionAPI(); 
}


UsdShadeMaterialBindingAPI::CollectionBindingVector
UsdShadeMaterialBindingAPI::GetCollectionBindings(
    const TfToken &materialPurpose) const
{
    std::vector<UsdRelationship> collectionBindingRels = 
        GetCollectionBindingRels(materialPurpose);

    CollectionBindingVector result;
    result.reserve(collectionBindingRels.size());

    for (const auto &collBindingRel : collectionBindingRels) {
        result.emplace_back(collBindingRel);

        // If both the collection and the material are valid, keep them in
        // the result.
        if (!result.back().IsValid()) {
            result.pop_back();
        }
    }

    return result;
}

UsdShadeMaterialBindingAPI::CollectionBindingVector
UsdShadeMaterialBindingAPI::_GetCollectionBindings(
    const TfTokenVector &collBindingPropertyNames) const
{
    CollectionBindingVector result;
    // Assuming that all binding relationships have something bound by them.
    result.reserve(collBindingPropertyNames.size());

    for (const auto &collBindingPropName : collBindingPropertyNames) {
        if (UsdRelationship collBindingRel = 
                GetPrim().GetRelationship(collBindingPropName)) {
            result.emplace_back(collBindingRel);
            // If collection binding is valid, keep it in the result.
            if (!result.back().IsValid()) {
                result.pop_back();
            }
        }
    }

    return result;
}

/* static */
TfToken 
UsdShadeMaterialBindingAPI::GetMaterialBindingStrength(
        const UsdRelationship &bindingRel)
{
    TfToken bindingStrength;
    bindingRel.GetMetadata(UsdShadeTokens->bindMaterialAs, 
                           &bindingStrength);
    // Default binding strength is weakerThanDescendants, as bindings authored 
    // on a prim are considered to be stronger than those authored on an 
    // ancestor, unless the ancestor binding overrides the binding strength to 
    // strongerThanDescendants.
    return bindingStrength.IsEmpty() ? UsdShadeTokens->weakerThanDescendants 
                                     : bindingStrength;
}

/* static */
bool
UsdShadeMaterialBindingAPI::SetMaterialBindingStrength(
        const UsdRelationship &bindingRel, 
        const TfToken &bindingStrength)
{
    if (bindingStrength == UsdShadeTokens->fallbackStrength) {
        TfToken existingBindingStrength;
        bindingRel.GetMetadata(UsdShadeTokens->bindMaterialAs, 
                               &existingBindingStrength);
        if (!existingBindingStrength.IsEmpty() &&
            existingBindingStrength != UsdShadeTokens->weakerThanDescendants) {
            return bindingRel.SetMetadata(
                    UsdShadeTokens->bindMaterialAs, 
                    UsdShadeTokens->weakerThanDescendants);
        }
        return true;
    }
    return bindingRel.SetMetadata(UsdShadeTokens->bindMaterialAs, 
                                  bindingStrength);
}

UsdRelationship 
UsdShadeMaterialBindingAPI::_CreateDirectBindingRel(
    const TfToken &materialPurpose) const
{
    return GetPrim().CreateRelationship(
        _GetDirectBindingRelName(materialPurpose), /*custom*/ false);
}
    
UsdRelationship 
UsdShadeMaterialBindingAPI::_CreateCollectionBindingRel(
    const TfToken &bindingName,
    const TfToken &materialPurpose) const
{
    TfToken collBindingRelName = _GetCollectionBindingRelName(
            bindingName, materialPurpose);
    return GetPrim().CreateRelationship(collBindingRelName, /* custom */ false);
}

bool 
UsdShadeMaterialBindingAPI::Bind(
    const UsdShadeMaterial &material,
    const TfToken &bindingStrength,
    const TfToken &materialPurpose) const
{
    if (UsdRelationship bindingRel = _CreateDirectBindingRel(materialPurpose)) {
        SetMaterialBindingStrength(bindingRel, bindingStrength);
        return bindingRel.SetTargets({material.GetPath()});
    }

    return false;
}

bool 
UsdShadeMaterialBindingAPI::Bind(
    const UsdCollectionAPI &collection, 
    const UsdShadeMaterial& material,
    const TfToken &bindingName,
    const TfToken &bindingStrength,
    const TfToken &materialPurpose) const
{
    // BindingName should not contains any namespaces. 
    // Also, we use the collection-name when bindingName is empty.
    TfToken fixedBindingName = bindingName;

    if (bindingName.IsEmpty()) {
        fixedBindingName = SdfPath::StripNamespace(collection.GetName());
    } else if (bindingName.GetString().find(':') != std::string::npos) {
        TF_CODING_ERROR("Invalid bindingName '%s', as it contains namespaces. "
            "Not binding collection <%s> to material <%s>.", 
            bindingName.GetText(), collection.GetCollectionPath().GetText(),
            material.GetPath().GetText());
        return false;
    }

    UsdRelationship collBindingRel = _CreateCollectionBindingRel(
        fixedBindingName, materialPurpose);

    if (collBindingRel) {
        SetMaterialBindingStrength(collBindingRel, bindingStrength);
        return collBindingRel.SetTargets({collection.GetCollectionPath(), 
                                          material.GetPath()});
    }
    
    return false;
}

bool 
UsdShadeMaterialBindingAPI::UnbindDirectBinding(
    const TfToken &materialPurpose) const
{
    UsdRelationship bindingRel = GetPrim().CreateRelationship(
        _GetDirectBindingRelName(materialPurpose), /*custom*/ false);
    return bindingRel && bindingRel.BlockTargets();
}

bool 
UsdShadeMaterialBindingAPI::UnbindCollectionBinding(
    const TfToken &bindingName, 
    const TfToken &materialPurpose) const
{
    UsdRelationship collBindingRel = GetPrim().CreateRelationship(
        _GetCollectionBindingRelName(bindingName, materialPurpose), 
        /*custom*/ false);
    return collBindingRel && collBindingRel.BlockTargets();
}

bool
UsdShadeMaterialBindingAPI::UnbindAllBindings() const
{
    std::vector<UsdProperty> allBindingProperties = 
        GetPrim().GetPropertiesInNamespace(
            UsdShadeTokens->materialBinding);

    // The relationship named material:binding (Which is the default/all-purpose
    // direct binding relationship) isn't included in the result of 
    // GetPropertiesInNamespace. Add it here if it exists.
    if (UsdRelationship allPurposeDirectBindingRel = 
        GetPrim().GetRelationship(UsdShadeTokens->materialBinding)) {
        allBindingProperties.push_back(allPurposeDirectBindingRel);
    }

    bool success = true;
    std::vector<UsdRelationship> result;
    for (const UsdProperty &prop : allBindingProperties) {
        if (UsdRelationship bindingRel = prop.As<UsdRelationship>()) {
            success = bindingRel.BlockTargets() && success;
        }
    }

    return success;
}

bool 
UsdShadeMaterialBindingAPI::RemovePrimFromBindingCollection(
    const UsdPrim &prim, 
    const TfToken &bindingName,
    const TfToken &materialPurpose) const
{
    if (UsdRelationship collBindingRel = GetCollectionBindingRel(bindingName, 
            materialPurpose)) {
        auto collBinding = CollectionBinding(collBindingRel);
        if (UsdCollectionAPI collection = collBinding.GetCollection()) {
            return collection.ExcludePath(prim.GetPath());
        }
    }

    return true;
}

bool 
UsdShadeMaterialBindingAPI::AddPrimToBindingCollection(
    const UsdPrim &prim, 
    const TfToken &bindingName,
    const TfToken &materialPurpose) const
{
    if (UsdRelationship collBindingRel = GetCollectionBindingRel(bindingName, 
            materialPurpose)) {
        auto collBinding = CollectionBinding(collBindingRel);
        if (UsdCollectionAPI collection = collBinding.GetCollection()) {
            return collection.IncludePath(prim.GetPath());
        }
    }

    return true;
}

// Given all the property names that start with "material:binding", returns 
// the subset of properties that represents collection-based bindings for the
// given material purpose.
static 
TfTokenVector 
_GetCollectionBindingPropertyNames(
    const TfTokenVector &matBindingPropNames,
    const TfToken &purpose)
{
    TfToken collBindingPrefix = _GetCollectionBindingRelName(
        /* bindingName */ TfToken(), purpose);
    
    TfTokenVector collBindingRelNames;
    // Not reserving memory because we don't expect to find these on most 
    // prims.
    size_t indexOfNSDelim = collBindingPrefix.size();
    for (auto & matBindingPropName : matBindingPropNames) {
        if (matBindingPropName.size() > indexOfNSDelim &&
            matBindingPropName.GetString()[indexOfNSDelim] == ':' &&
            TfStringStartsWith(matBindingPropName.GetString(), 
                               collBindingPrefix.GetString()) &&
            // Ensure that the material purpose matches by making sure 
            // the second half does not contain a ":".
            (purpose != UsdShadeTokens->allPurpose || 
             matBindingPropName.GetString().find(':', indexOfNSDelim+1) == 
                std::string::npos)) {
            collBindingRelNames.push_back(matBindingPropName);
        }
    }

    return collBindingRelNames;
}

UsdShadeMaterialBindingAPI::BindingsAtPrim::BindingsAtPrim(
    const UsdPrim &prim, const TfToken &materialPurpose)
{
    // These are the properties we need to consider when looking for 
    // bindings (both direct and collection-based) at the prim itself 
    // and each ancestor prim.
    // Note: This vector is already ordered.
    const TfTokenVector matBindingPropNames = prim.GetAuthoredPropertyNames(
            /*predicate*/ [](const TfToken &name) {
                return TfStringStartsWith(name.GetString(), 
                    UsdShadeTokens->materialBinding);
            });

    static const bool warnOnLookBinding = TfGetEnvSetting(
            USD_SHADE_WARN_ON_LOOK_BINDING);
    if (warnOnLookBinding) {
        static const TfToken lookBinding("look:binding");
        if (UsdRelationship lookBindingRel = prim.GetRelationship(lookBinding)) 
        {
            TF_WARN("Found prim <%s> with deprecated 'look:binding' "
                "relationship targeting path <%s>.", prim.GetPath().GetText(),
                UsdShadeMaterialBindingAPI::DirectBinding(lookBindingRel)
                    .GetMaterialPath().GetText());
        }
    }
   
    if (matBindingPropNames.empty())
        return;

    auto foundMatBindingProp = [&matBindingPropNames](const TfToken &relName) {
        return std::find(matBindingPropNames.begin(), matBindingPropNames.end(),
                    relName) != matBindingPropNames.end();
    };

    TfToken directBindingRelName = _GetDirectBindingRelName(materialPurpose);
    if (foundMatBindingProp(directBindingRelName)) {
        UsdRelationship directBindingRel = 
                prim.GetRelationship(directBindingRelName);
        directBinding.reset(new DirectBinding(directBindingRel));
    }

    // If there is no restricted purpose direct binding, look for an
    // all-purpose direct-binding.
    if (materialPurpose != UsdShadeTokens->allPurpose && 
        (!directBinding || !directBinding->GetMaterial())) {

        // This may not be necessary if a specific purpose collection-binding 
        // already includes the prim for which the resolved binding is being 
        // computed.
         TfToken allPurposeDBRelName = _GetDirectBindingRelName(
            UsdShadeTokens->allPurpose);

        if (foundMatBindingProp(allPurposeDBRelName)) {
            UsdRelationship directBindingRel = prim.GetRelationship(
                        allPurposeDBRelName);
            directBinding.reset(new DirectBinding (directBindingRel));
        }
    }

    // If the direct-binding points to an invalid material then clear it.
    if (directBinding && !directBinding->GetMaterial()) {
        directBinding.release();
    }

    // Check if there are any collection-based binding relationships 
    // for the current "purpose" in matBindingPropNames.
    if (materialPurpose != UsdShadeTokens->allPurpose) {
        TfTokenVector collBindingPropertyNames =
            _GetCollectionBindingPropertyNames(matBindingPropNames, 
                                               materialPurpose);
        if (!collBindingPropertyNames.empty()) {
            UsdShadeMaterialBindingAPI bindingAPI(prim);
            restrictedPurposeCollBindings =
                bindingAPI._GetCollectionBindings(collBindingPropertyNames);
        }
    } 

    
    TfTokenVector collBindingPropertyNames =
        _GetCollectionBindingPropertyNames(matBindingPropNames, 
            UsdShadeTokens->allPurpose);
    if (!collBindingPropertyNames.empty()) {
        UsdShadeMaterialBindingAPI bindingAPI(prim);
        allPurposeCollBindings = 
            bindingAPI._GetCollectionBindings(collBindingPropertyNames);
    }
}

UsdShadeMaterial 
UsdShadeMaterialBindingAPI::ComputeBoundMaterial(
    BindingsCache *bindingsCache,
    CollectionQueryCache *collectionQueryCache,
    const TfToken &materialPurpose,
    UsdRelationship *bindingRel) const
{
    if (!GetPrim()) {
        TF_CODING_ERROR("Invalid prim (%s)", UsdDescribe(GetPrim()).c_str());
        return UsdShadeMaterial();
    }

    TRACE_FUNCTION();

    std::vector<TfToken> materialPurposes{materialPurpose};
    if (materialPurpose != UsdShadeTokens->allPurpose) {
        materialPurposes.push_back(UsdShadeTokens->allPurpose);
    }

    for (auto const & purpose : materialPurposes) {
        UsdShadeMaterial boundMaterial;
        UsdRelationship winningBindingRel;

        for (UsdPrim p = GetPrim(); !p.IsPseudoRoot(); p = p.GetParent()) {
            
            auto bindingsIt = bindingsCache->find(p.GetPath());
            if (bindingsIt == bindingsCache->end()) {
                TRACE_SCOPE("UsdShadeMaterialBindingAPI::ComputeBoundMaterial "
                        "(BindingsCache)");
                bindingsIt = bindingsCache->emplace(p.GetPath(), 
                    std::make_shared<BindingsAtPrim>(p, materialPurpose)).first;
            }

            const BindingsAtPrim &bindingsAtP = *bindingsIt->second;

            const DirectBindingPtr &directBindingPtr = 
                    bindingsAtP.directBinding;
            if (directBindingPtr && 
                directBindingPtr->GetMaterialPurpose() == purpose) 
            {
                const UsdRelationship &directBindingRel = 
                        directBindingPtr->GetBindingRel();
                if (!boundMaterial || 
                    (GetMaterialBindingStrength(directBindingRel) == 
                     UsdShadeTokens->strongerThanDescendants))
                {
                    boundMaterial = directBindingPtr->GetMaterial();
                    winningBindingRel = directBindingRel;
                }
            }

            const CollectionBindingVector &collBindings = 
                purpose == UsdShadeTokens->allPurpose ? 
                bindingsAtP.allPurposeCollBindings :
                bindingsAtP.restrictedPurposeCollBindings;

            for (auto &collBinding : collBindings) {
                TRACE_SCOPE("UsdShadeMaterialBindingAPI::ComputeBoundMaterial "
                            "(IsInBoundCollection)");

                const UsdCollectionAPI &collection = 
                    collBinding.GetCollection();
                const SdfPath &collectionPath = collBinding.GetCollectionPath();

                auto collIt = collectionQueryCache->find(collectionPath);
                if (collIt == collectionQueryCache->end()) {
                    TRACE_SCOPE("UsdShadeMaterialBindingAPI::"
                        "ComputeBoundMaterial (CollectionQuery)");

                    std::shared_ptr<UsdCollectionAPI::MembershipQuery> mQuery =
                        std::make_shared<UsdCollectionAPI::MembershipQuery>();
                    collection.ComputeMembershipQuery(mQuery.get());

                    collIt = collectionQueryCache->emplace(
                        collectionPath, mQuery).first; 
                }
                
                bool isPrimIncludedInCollection = 
                        collIt->second->IsPathIncluded(GetPath());
                if (isPrimIncludedInCollection) {
                    const UsdRelationship &collBindingRel = 
                            collBinding.GetBindingRel();
                    // If the collection binding is on the prim itself and if 
                    // the prim is included in the collection, the collection-based
                    // binding is considered to be stronger than the direct binding.
                    if (!boundMaterial || 
                        (boundMaterial && winningBindingRel.GetPrim() == p) ||
                        (GetMaterialBindingStrength(collBindingRel) == 
                            UsdShadeTokens->strongerThanDescendants)) {
                        boundMaterial = collBinding.GetMaterial();
                        winningBindingRel = collBindingRel;

                        // The first collection binding we match will be the 
                        // one we care about.
                        break;
                    }
                }
            }
        }

        // The first "purpose" with a valid binding wins.
        if (boundMaterial) {
            if (bindingRel) {
                *bindingRel = winningBindingRel;
            }

            return boundMaterial;
        }
    }
  
    return UsdShadeMaterial();
}

UsdShadeMaterial 
UsdShadeMaterialBindingAPI::ComputeBoundMaterial(
    const TfToken &materialPurpose,
    UsdRelationship *bindingRel) const
{
    BindingsCache bindingsCache;
    CollectionQueryCache collQueryCache;
    return ComputeBoundMaterial(&bindingsCache, 
                                &collQueryCache, 
                                materialPurpose, 
                                bindingRel);
}

/* static */
std::vector<UsdShadeMaterial> 
UsdShadeMaterialBindingAPI::ComputeBoundMaterials(
    const std::vector<UsdPrim> &prims, 
    const TfToken &materialPurpose,
    std::vector<UsdRelationship> *bindingRels)
{
    std::vector<UsdShadeMaterial> materials(prims.size());

    if (bindingRels) {
        bindingRels->clear();
        bindingRels->resize(prims.size());
    }

    // This ensures that bindings are only computed once per prim.
    BindingsCache bindingsCache;

    // The use of CollectionQueryCache ensures that every collection's 
    // MembershipQuery object is only constructed once.
    CollectionQueryCache collQueryCache;

    WorkParallelForN(prims.size(), 
        [&](size_t start, size_t end) {
            for (size_t i = start; i < end; ++i) {
                const UsdPrim &prim = prims[i];
                UsdRelationship *bindingRel = 
                        bindingRels ? &((*bindingRels)[i]) : nullptr;
                materials[i] = UsdShadeMaterialBindingAPI(prim).
                    ComputeBoundMaterial(&bindingsCache, &collQueryCache, 
                                         materialPurpose, bindingRel);
            }
        });

    return materials;
}

UsdGeomSubset 
UsdShadeMaterialBindingAPI::CreateMaterialBindSubset(
    const TfToken &subsetName,
    const VtIntArray &indices,
    const TfToken &elementType)
{
    UsdGeomImageable geom(GetPrim());

    UsdGeomSubset result = UsdGeomSubset::CreateGeomSubset(geom, subsetName, 
        elementType, indices, UsdShadeTokens->materialBind);

    TfToken familyType = UsdGeomSubset::GetFamilyType(geom, 
        UsdShadeTokens->materialBind);
    // Subsets that have materials bound to them should have 
    // mutually exclusive sets of indices. Hence, set the familyType 
    // to "nonOverlapping" if it's unset (or explicitly set to unrestricted).
    if (familyType.IsEmpty() || 
        familyType == UsdGeomTokens->unrestricted) {
        SetMaterialBindSubsetsFamilyType(UsdGeomTokens->nonOverlapping);
    }

    return result;
}


std::vector<UsdGeomSubset> 
UsdShadeMaterialBindingAPI::GetMaterialBindSubsets()
{
    UsdGeomImageable geom(GetPrim());
    return UsdGeomSubset::GetGeomSubsets(geom, /* elementType */ TfToken(), 
        UsdShadeTokens->materialBind);
}

bool 
UsdShadeMaterialBindingAPI::SetMaterialBindSubsetsFamilyType(
        const TfToken &familyType)
{
    if (familyType == UsdGeomTokens->unrestricted) {
        TF_CODING_ERROR("Attempted to set invalid familyType 'unrestricted' for"
            "the \"materialBind\" family of subsets on <%s>.", 
            GetPath().GetText());
        return false;
    }
    UsdGeomImageable geom(GetPrim());
    return UsdGeomSubset::SetFamilyType(geom, UsdShadeTokens->materialBind,
        familyType);
}

TfToken
UsdShadeMaterialBindingAPI::GetMaterialBindSubsetsFamilyType()
{
    UsdGeomImageable geom(GetPrim());
    return UsdGeomSubset::GetFamilyType(geom, UsdShadeTokens->materialBind);
}

PXR_NAMESPACE_CLOSE_SCOPE
