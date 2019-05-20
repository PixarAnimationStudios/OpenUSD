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
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdCollectionAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (CollectionAPI)
    (collection)
);

/* virtual */
UsdCollectionAPI::~UsdCollectionAPI()
{
}

/* static */
UsdCollectionAPI
UsdCollectionAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdCollectionAPI();
    }
    TfToken name;
    if (!IsCollectionAPIPath(path, &name)) {
        TF_CODING_ERROR("Invalid collection path <%s>.", path.GetText());
        return UsdCollectionAPI();
    }
    return UsdCollectionAPI(stage->GetPrimAtPath(path.GetPrimPath()), name);
}

UsdCollectionAPI
UsdCollectionAPI::Get(const UsdPrim &prim, const TfToken &name)
{
    return UsdCollectionAPI(prim, name);
}


/* static */
bool 
UsdCollectionAPI::IsSchemaPropertyBaseName(const TfToken &baseName)
{
    static TfTokenVector attrsAndRels = {
        UsdTokens->expansionRule,
        UsdTokens->includeRoot,
        UsdTokens->includes,
        UsdTokens->excludes,
    };

    return find(attrsAndRels.begin(), attrsAndRels.end(), baseName)
            != attrsAndRels.end();
}

/* static */
bool
UsdCollectionAPI::IsCollectionAPIPath(
    const SdfPath &path, TfToken *name)
{
    if (!path.IsPropertyPath()) {
        return false;
    }

    std::string propertyName = path.GetName();
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(propertyName);

    // The baseName of the  path can't be one of the 
    // schema properties. We should validate this in the creation (or apply)
    // API.
    TfToken baseName = *tokens.rbegin();
    if (IsSchemaPropertyBaseName(baseName)) {
        return false;
    }

    if (tokens.size() >= 2
        && tokens[0] == _schemaTokens->collection) {
        *name = TfToken(propertyName.substr(
            _schemaTokens->collection.GetString().size() + 1));
        return true;
    }

    return false;
}

/* virtual */
UsdSchemaType UsdCollectionAPI::_GetSchemaType() const {
    return UsdCollectionAPI::schemaType;
}

/* static */
UsdCollectionAPI
UsdCollectionAPI::_Apply(const UsdPrim &prim, const TfToken &name)
{
    return UsdAPISchemaBase::_MultipleApplyAPISchema<UsdCollectionAPI>(
            prim, _schemaTokens->CollectionAPI, name);
}

/* static */
const TfType &
UsdCollectionAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdCollectionAPI>();
    return tfType;
}

/* static */
bool 
UsdCollectionAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdCollectionAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/// Returns the property name prefixed with the correct namespace prefix, which
/// is composed of the the API's propertyNamespacePrefix metadata and the
/// instance name of the API.
inline
TfToken
_GetNamespacedPropertyName(const TfToken instanceName, const TfToken propName)
{
    TfTokenVector identifiers =
        {_schemaTokens->collection, instanceName, propName};
    return TfToken(SdfPath::JoinIdentifier(identifiers));
}

UsdAttribute
UsdCollectionAPI::GetExpansionRuleAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->expansionRule));
}

UsdAttribute
UsdCollectionAPI::CreateExpansionRuleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->expansionRule),
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdCollectionAPI::GetIncludeRootAttr() const
{
    return GetPrim().GetAttribute(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->includeRoot));
}

UsdAttribute
UsdCollectionAPI::CreateIncludeRootAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(
                       _GetNamespacedPropertyName(
                            GetName(),
                           UsdTokens->includeRoot),
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdCollectionAPI::GetIncludesRel() const
{
    return GetPrim().GetRelationship(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->includes));
}

UsdRelationship
UsdCollectionAPI::CreateIncludesRel() const
{
    return GetPrim().CreateRelationship(
                       _GetNamespacedPropertyName(
                           GetName(),
                           UsdTokens->includes),
                       /* custom = */ false);
}

UsdRelationship
UsdCollectionAPI::GetExcludesRel() const
{
    return GetPrim().GetRelationship(
        _GetNamespacedPropertyName(
            GetName(),
            UsdTokens->excludes));
}

UsdRelationship
UsdCollectionAPI::CreateExcludesRel() const
{
    return GetPrim().CreateRelationship(
                       _GetNamespacedPropertyName(
                           GetName(),
                           UsdTokens->excludes),
                       /* custom = */ false);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(
    const TfToken instanceName,
    const TfTokenVector& left,
    const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());

    for (const TfToken attrName : right) {
        result.push_back(
            _GetNamespacedPropertyName(instanceName, attrName));
    }
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdCollectionAPI::GetSchemaAttributeNames(
    bool includeInherited, const TfToken instanceName)
{
    static TfTokenVector localNames = {
        UsdTokens->expansionRule,
        UsdTokens->includeRoot,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            instanceName,
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

#include "pxr/usd/usd/primRange.h"

#include <boost/functional/hash.hpp>

#include <set>

PXR_NAMESPACE_OPEN_SCOPE

/* static */
UsdCollectionAPI 
UsdCollectionAPI::ApplyCollection(
    const UsdPrim& prim, 
    const TfToken &name, 
    const TfToken &expansionRule /*=UsdTokens->expandPrims*/) 
{
    // Ensure that the collection name is valid.
    TfTokenVector tokens = SdfPath::TokenizeIdentifierAsTokens(name);

    if (tokens.empty()) {
        TF_CODING_ERROR("Invalid collection name '%s'.", name.GetText());
        return UsdCollectionAPI();
    }

    TfToken baseName = *tokens.rbegin();
    if (IsSchemaPropertyBaseName(baseName)) {
        TF_CODING_ERROR("Invalid collection name '%s'. The base-name '%s' is a "
            "schema property name.", name.GetText(), baseName.GetText());
        return UsdCollectionAPI();
    }

    UsdCollectionAPI collection = UsdCollectionAPI::_Apply(prim, name);
    collection.CreateExpansionRuleAttr(VtValue(expansionRule));
    return collection;
}

/* static */
UsdCollectionAPI 
UsdCollectionAPI::GetCollection(const UsdStagePtr &stage, 
                                const SdfPath &collectionPath)
{
    TfToken collectionName;
    if (!IsCollectionAPIPath(collectionPath, &collectionName)) {
        TF_CODING_ERROR("Invalid collection path <%s>.", 
                        collectionPath.GetText());
        return UsdCollectionAPI();
    }

    return UsdCollectionAPI(stage->GetPrimAtPath(collectionPath.GetPrimPath()), 
                            collectionName);
}

/* static */
UsdCollectionAPI 
UsdCollectionAPI::GetCollection(const UsdPrim &prim, 
                                const TfToken &name)
{
   return UsdCollectionAPI(prim, name);
}

SdfPath 
UsdCollectionAPI::GetCollectionPath() const
{
    return GetPath().AppendProperty(_GetCollectionPropertyName());
}

/* static */
SdfPath 
UsdCollectionAPI::GetNamedCollectionPath(
    const UsdPrim &prim, 
    const TfToken &collectionName)
{
    return prim.GetPath().AppendProperty(TfToken(SdfPath::JoinIdentifier(
            UsdTokens->collection, collectionName)));
}

// XXX: This functionality should probably be exposed in the base-class for use 
// in other API schemas.UsdPrim::HasAPI has similar code as well.
static std::vector<std::string> 
_GetCollectionAPIAliases(const TfType &collSchemaType) 
{
    // The alias for UsdCollectionAPI is already available as a static token 
    // in _schemaTokes.
    std::vector<std::string> collectionAPIAliases{_schemaTokens->CollectionAPI};

    // If there are derived types of the CollectionAPI, include their aliases
    // too.
    std::set<TfType> derivedTypes;
    collSchemaType.GetAllDerivedTypes(&derivedTypes);
    if (!derivedTypes.empty()) {
        collectionAPIAliases.reserve(collectionAPIAliases.size() + 
                                     derivedTypes.size());
        const auto schemaBaseType = TfType::Find<UsdSchemaBase>();
        for (const auto& derived : derivedTypes) {
            for (const auto &derivedAlias : schemaBaseType.GetAliases(derived)){
                collectionAPIAliases.push_back(derivedAlias);
            }
        }
    }

    return collectionAPIAliases;
}

/* static */
std::vector<UsdCollectionAPI>
UsdCollectionAPI::GetAllCollections(const UsdPrim &prim)
{
    std::vector<UsdCollectionAPI> collections;

    auto appliedSchemas = prim.GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return collections;
    }

    static const std::vector<std::string> collectionAPIAliases = 
        _GetCollectionAPIAliases(_GetStaticTfType());

    for (const auto &appliedSchema : appliedSchemas) {
        for (const std::string &alias : collectionAPIAliases) {
            const std::string collAPIPrefix = alias + 
                    UsdObject::GetNamespaceDelimiter();
            if (TfStringStartsWith(appliedSchema, collAPIPrefix)) {
                const std::string collectionName = 
                        appliedSchema.GetString().substr(collAPIPrefix.size());
                collections.emplace_back(prim, TfToken(collectionName));
            }
        }
    }

    return collections;
}

TfToken 
UsdCollectionAPI::_GetCollectionPropertyName(
    const TfToken &baseName /* =TfToken() */) const
{
    return TfToken(UsdTokens->collection.GetString() + ":" + 
                   GetName().GetString() + 
                   (baseName.IsEmpty() ? "" : (":" + baseName.GetString())));
}

bool 
UsdCollectionAPI::IncludePath(const SdfPath &pathToInclude) const
{
    // If the prim is already included in the collection, do nothing.
    MembershipQuery query  = ComputeMembershipQuery();
    if (query.IsPathIncluded(pathToInclude)) {
        return true;
    }

    if (pathToInclude == SdfPath::AbsoluteRootPath()) {
        CreateIncludeRootAttr(VtValue(true));
        return true;
    }

    // Check if the prim is directly excluded from the collection.
    SdfPathVector excludes;
    if (UsdRelationship excludesRel = GetExcludesRel()) {
        excludesRel.GetTargets(&excludes);

        if (std::find(excludes.begin(), excludes.end(), pathToInclude) != 
                excludes.end()) {
            excludesRel.RemoveTarget(pathToInclude);
            // Update the query object we have, instead of having to 
            // recompute it.
            auto it = query._pathExpansionRuleMap.find(pathToInclude);
            if (TF_VERIFY(it != query._pathExpansionRuleMap.end())) {
                query._pathExpansionRuleMap.erase(it);
            }
        }
    }

    // Now that we've removed the explicit excludes if there was one, 
    // we can add the prim if it's not already included in the collection. 
    if (!query.IsPathIncluded(pathToInclude)) {
        return CreateIncludesRel().AddTarget(pathToInclude);
    }

    return true;
}

bool 
UsdCollectionAPI::ExcludePath(const SdfPath &pathToExclude) const
{
    // If the path is already excluded from a non-empty collection
    // (or simply not included at all), do nothing.
    MembershipQuery query = ComputeMembershipQuery();
    if (!query._pathExpansionRuleMap.empty() &&
        !query.IsPathIncluded(pathToExclude)) {
        return true;
    }

    if (pathToExclude == SdfPath::AbsoluteRootPath()) {
        CreateIncludeRootAttr(VtValue(false));
        return true;
    }

    // Check if the path is directly included in the collection.
    SdfPathVector includes;
    if (UsdRelationship includesRel = GetIncludesRel()) {
        includesRel.GetTargets(&includes);

        if (std::find(includes.begin(), includes.end(), pathToExclude) != 
                includes.end()) {
            includesRel.RemoveTarget(pathToExclude);
            // Update the query object we have, instead of having to 
            // recompute it.
            auto it = query._pathExpansionRuleMap.find(pathToExclude);
            if (TF_VERIFY(it != query._pathExpansionRuleMap.end())) {
                query._pathExpansionRuleMap.erase(it);
            }
        }
    }

    // Now that we've removed the explicit include if there was one, 
    // we can add an explicit exclude, if required.
    if (query._pathExpansionRuleMap.empty() ||
        query.IsPathIncluded(pathToExclude)) {
        return CreateExcludesRel().AddTarget(pathToExclude);
    }

    return true;
}

bool
UsdCollectionAPI::HasNoIncludedPaths() const
{
    SdfPathVector includes;
    GetIncludesRel().GetTargets(&includes);
    bool includeRoot = false;
    GetIncludeRootAttr().Get(&includeRoot);
    return includes.empty() && !includeRoot;
}

UsdCollectionAPI::MembershipQuery 
UsdCollectionAPI::ComputeMembershipQuery() const
{
    UsdCollectionAPI::MembershipQuery query;
    ComputeMembershipQuery(&query);
    return query;
}

void 
UsdCollectionAPI::ComputeMembershipQuery(MembershipQuery *query) const
{
    if (!query) {
        TF_CODING_ERROR("Invalid query pointer.");
        return;
    }

    SdfPathSet chainedCollectionPaths { GetCollectionPath() };
    _ComputeMembershipQueryImpl(query, chainedCollectionPaths);
}

void
UsdCollectionAPI::_ComputeMembershipQueryImpl(
    MembershipQuery *query,
    const SdfPathSet &chainedCollectionPaths,
    bool *foundCircularDependency) const
{
    // Get this collection's expansionRule.
    TfToken expRule;
    GetExpansionRuleAttr().Get(&expRule);

    if (expRule.IsEmpty()) {
        expRule = UsdTokens->expandPrims;
    }

    SdfPathVector includes, excludes;
    GetIncludesRel().GetTargets(&includes);
    GetExcludesRel().GetTargets(&excludes);

    // Consult includeRoot and include </> if requested.
    // (The separate attribute is necessary since </> cannot be a
    // target path in a relationship.)
    bool includeRoot = false;
    GetIncludeRootAttr().Get(&includeRoot);
    if (includeRoot) {
        includes.push_back(SdfPath::AbsoluteRootPath());
    }

    UsdStageWeakPtr stage = GetPrim().GetStage();
    
    for (const SdfPath &includedPath : includes) {
        TfToken collectionName;
        // Check if the included path is a collection. If it is, then 
        // handle it specially.
        if (IsCollectionAPIPath(includedPath, &collectionName)) {
            if (chainedCollectionPaths.count(includedPath) > 0) {
                if (foundCircularDependency) {
                    *foundCircularDependency = true;
                } else {
                    // Issue a warning message if the clients of this method
                    // don't care about knowing if there's a circular 
                    // dependency.
                    std::string includedCollectionsStr; 
                    for (const SdfPath &collPath : chainedCollectionPaths) {
                        includedCollectionsStr.append(collPath.GetString());
                        includedCollectionsStr.append(", ");
                    }
                    TF_WARN("Found circular dependency involving the following "
                        "collections: [%s]", includedCollectionsStr.c_str());
                }
                // Continuing here avoids infinite recursion.
                continue;
            }

            SdfPath includedPrimPath = includedPath.GetPrimPath();
            UsdPrim includedPrim = stage->GetPrimAtPath(includedPrimPath);

            // The included collection must belong to a valid prim.
            // XXX: Should we check validity? We should skip circular 
            // dependency check if we do validate.
            if (!includedPrim) {
                TF_WARN("Could not get prim at path <%s>, therefore cannot "
                    "include its collection '%s' in collection '%s'.",
                    includedPrimPath.GetText(), collectionName.GetText(),
                    GetName().GetText());
                continue;
            }

            UsdCollectionAPI includedCollection(includedPrim, collectionName);

            // Recursively compute the included collection's membership map with
            // an updated set of seen/included collection paths.
            // 
            // Create a copy so we can add this collection to the list 
            // before calling ComputeMembershipQuery.
            SdfPathSet seenCollectionPaths = chainedCollectionPaths;
            seenCollectionPaths.insert(includedPath);
            UsdCollectionAPI::MembershipQuery includedQuery;
            includedCollection._ComputeMembershipQueryImpl(&includedQuery,
                    seenCollectionPaths, foundCircularDependency);

            query->_MergeMembershipQuery(includedQuery);
        } else {
            query->_AppendIncludedPath(includedPath, expRule);
        }
    }

    // Process the excludes after the includes.
    for (const auto &p: excludes) {
        query->_AppendExcludedPath(p);
    }
}

/* static */
std::set<UsdObject> 
UsdCollectionAPI::ComputeIncludedObjects(
    const MembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred)
{
    std::set<UsdObject> result;
    _ComputeIncludedImpl(query, stage, pred, &result, nullptr);
    return result;
}

/* static */
SdfPathSet
UsdCollectionAPI::ComputeIncludedPaths(
    const MembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred)
{
    SdfPathSet result;
    _ComputeIncludedImpl(query, stage, pred, nullptr, &result);
    return result;
}

/* static */
void
UsdCollectionAPI::_ComputeIncludedImpl(
    const MembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred,
    std::set<UsdObject> *includedObjects,
    SdfPathSet *includedPaths)
{
    if (!((bool)includedObjects ^ (bool)includedPaths)) {
        TF_CODING_ERROR("Either includedObjects or includedPaths must be"
                        " valid, but not both");
    }

    std::set<UsdObject> result;

    const auto &pathExpRuleMap = query._pathExpansionRuleMap;
    const bool hasExcludes = query.HasExcludes();

    // A path is excluded if the path itself or any of its ancestors are 
    // excluded.
    auto IsExcluded = [hasExcludes,pathExpRuleMap](const SdfPath &path) {
        // Return early if we know that there are no excludes.
        if (!hasExcludes) {
            return false;
        }
        for (SdfPath p = path; p != SdfPath::EmptyPath();  
             p = p.GetParentPath()) {
            // Include if the nearest ancestor path with an opinion in 
            // path->expansionRuleMap isn't excluded.
            auto it = pathExpRuleMap.find(p);
            if (it != pathExpRuleMap.end()) {
                return it->second == UsdTokens->exclude;
            }
        }
        return false;
    };

    // Helper function to get the UsdProperty object associated with a given 
    // property path.
    auto GetPropertyAtPath = [stage](const SdfPath &path) {
        if (const UsdPrim p = stage->GetPrimAtPath(path.GetPrimPath())) {
            return p.GetProperty(path.GetNameToken());
        }
        return UsdProperty();
    };

    // Returns true if a property is excluded in the PathExpansionRuleMap.
    auto IsPropertyExplicitlyExcluded = [hasExcludes,pathExpRuleMap](
            const SdfPath &propPath) {
        if (!hasExcludes) {       
            return false;
        }
        auto it = pathExpRuleMap.find(propPath);
        if (it != pathExpRuleMap.end()) {
            return it->second == UsdTokens->exclude;
        }
        return false;
    };
    
    auto AppendIncludedObject = [includedObjects, includedPaths](
            const UsdObject &obj) {
        if (includedObjects) {
            includedObjects->insert(obj);
        } else if (includedPaths) {
            includedPaths->insert(obj.GetPath());
        }
    };

    // Iterate through all the entries in the PathExpansionRuleMap.
    for (const auto &pathAndExpansionRule : pathExpRuleMap) {
        const TfToken &expansionRule = pathAndExpansionRule.second;

        // Skip excluded paths.
        if (expansionRule == UsdTokens->exclude) {
            continue;
        }

        const SdfPath &path = pathAndExpansionRule.first;

        if (expansionRule == UsdTokens->explicitOnly) {
            if (path.IsPrimPath()) {
                UsdPrim p = stage->GetPrimAtPath(path);
                if (p && pred(p)) {
                    AppendIncludedObject(p);
                }
            } else if (path.IsPropertyPath()) {
                if (UsdProperty property = GetPropertyAtPath(path)) {
                    AppendIncludedObject(property.As<UsdObject>());
                }
            } else {
                TF_CODING_ERROR("Unknown path type in membership-map.");
            }
        }
        
        else if (expansionRule == UsdTokens->expandPrims ||
                 expansionRule == UsdTokens->expandPrimsAndProperties) 
        {
            if (path.IsPropertyPath()) {
                if (UsdProperty property = GetPropertyAtPath(path)) {
                    AppendIncludedObject(property.As<UsdObject>());
                }
            } else if (UsdPrim prim = stage->GetPrimAtPath(path)) {

                UsdPrimRange range(prim, pred);
                auto iter = range.begin();
                for (; iter != range.end() ; ++iter) {
                    const UsdPrim &descendantPrim = *iter;

                    // Skip the descendant prim and its subtree  
                    // if it's excluded. 
                    // If an object below the excluded object is included, 
                    // it will have a separate entry in the 
                    // path<->expansionRule map.                         
                    if (IsExcluded(descendantPrim.GetPath())) {
                        iter.PruneChildren();
                        continue;
                    }

                    AppendIncludedObject(descendantPrim.As<UsdObject>());

                    if (expansionRule != UsdTokens->expandPrimsAndProperties) {
                        continue;
                    }

                    // Call GetProperties() on the prim (which is known to be 
                    // slow), only when the client is interested in property 
                    // objects.
                    // 
                    // Call GetPropertyNames() otherwise.
                    if (includedObjects) {
                        std::vector<UsdProperty> properties = 
                            descendantPrim.GetProperties();
                        for (const auto &property : properties) {
                            // Add the property to the result only if it's 
                            // not explicitly excluded.
                            if (!IsPropertyExplicitlyExcluded(
                                    property.GetPath())) {
                                AppendIncludedObject(property.As<UsdObject>());
                            }
                        }
                    } else {
                        for (const auto &propertyName : 
                                descendantPrim.GetPropertyNames()) {
                            SdfPath propertyPath = 
                                descendantPrim.GetPath().AppendProperty(
                                    propertyName);
                            if (!IsPropertyExplicitlyExcluded(propertyPath)) {
                                // Can't call IncludeObject here since we're 
                                // avoiding creation of the object.
                                includedPaths->insert(propertyPath);
                            }
                        }
                    }
                }
            }
        }
    }
}

static bool
_AllRootmostRulesPassFilter(
    UsdCollectionAPI::MembershipQuery::PathExpansionRuleMap const& ruleMap,
    std::function<bool(std::pair<SdfPath, TfToken> const&)> const& filter)
{
    if (ruleMap.empty()) {
        return false;
    }
    for (const auto &rule: ruleMap) {
        // Check if this is a rootmost path (in other words, not
        // contained under another rule).
        bool isRootmost = true;
        for (SdfPath p = rule.first.GetParentPath();
             p != SdfPath::EmptyPath(); p = p.GetParentPath()) {
            if (ruleMap.find(p) != ruleMap.end()) {
                isRootmost = false;
                break;
            }
        }
        if (isRootmost && !filter(rule)) {
            return false;
        }
    }
    return true;
}

bool 
UsdCollectionAPI::Validate(std::string *reason) const
{
    TfToken expansionRule; 
    if (UsdAttribute expRuleAttr = GetExpansionRuleAttr()) {
        expRuleAttr.Get(&expansionRule);
    }

    // Validate value of expansionRule.
    if (!expansionRule.IsEmpty() && 
        expansionRule != UsdTokens->explicitOnly && 
        expansionRule != UsdTokens->expandPrims &&
        expansionRule != UsdTokens->expandPrimsAndProperties) {

        if (reason) {
            *reason += TfStringPrintf("Invalid expansionRule value '%s'\n", 
                                      expansionRule.GetText());
        }
        return false;
    }

    // Check for circular dependencies.
    bool foundCircularDependency = false; 
    SdfPathSet chainedCollectionPaths{GetCollectionPath()};
    // We're not interested in the computed query object here.
    MembershipQuery query;
    _ComputeMembershipQueryImpl(&query, chainedCollectionPaths,
                                &foundCircularDependency);
    if (foundCircularDependency) {
        if (reason) {
            *reason += "Found one or more circular dependencies amongst "
                "the set of included (directly and transitively) collections.";
        }
        return false;
    }

    // Prohibit using both includes and excludes in top-level rules,
    // since the intent is ambiguous.
    if (query.HasExcludes()) {
        bool allExcludes = _AllRootmostRulesPassFilter(
            query._pathExpansionRuleMap,
            [](std::pair<SdfPath, TfToken> const& rule) -> bool {
                return rule.second == UsdTokens->exclude;
            });
        bool allIncludes = _AllRootmostRulesPassFilter(
            query._pathExpansionRuleMap,
            [](std::pair<SdfPath, TfToken> const& rule) -> bool {
                return rule.second != UsdTokens->exclude;
            });
        if (!allExcludes && !allIncludes) {
            if (reason) {
                *reason += "Found both includes and excludes among the "
                    "root-most rules -- interpretation is ambiguous";
            }
            return false;
        }
    }

    return true;
}

bool 
UsdCollectionAPI::ResetCollection() const
{
    bool success = true;
    if (UsdRelationship includesRel = GetIncludesRel()) {
        success = includesRel.ClearTargets(/* removeSpec */ true) && success;  
    }
    if (UsdRelationship excludesRel = GetExcludesRel()) {
        success = excludesRel.ClearTargets(/* removeSpec */ true) && success;
    }
    return success;
}

bool 
UsdCollectionAPI::BlockCollection() const
{
    bool success = true;
    if (UsdRelationship includesRel = GetIncludesRel()) {
        success = includesRel.BlockTargets() && success;  
    }
    if (UsdRelationship excludesRel = GetExcludesRel()) {
        success = excludesRel.BlockTargets() && success;
    }
    return success;    
}

bool
UsdCollectionAPI::MembershipQuery::IsPathIncluded(
    const SdfPath &path,
    TfToken *expansionRule) const
{
    // Only prims and properties can belong to a collection.
    if (!path.IsPrimPath() && !path.IsPropertyPath()) 
        return false;

    // Have separate code paths for prim and property paths as we'd like this 
    // method to be as fast as possible.
    if (path.IsPrimPath()) {
        for (SdfPath p = path; p != SdfPath::EmptyPath();  p = p.GetParentPath())     
        {
            const auto i = _pathExpansionRuleMap.find(p);
            if (i != _pathExpansionRuleMap.end()) {
                if (i->second == UsdTokens->exclude) {
                    if (expansionRule) {
                        *expansionRule = UsdTokens->exclude;
                    }
                    return false;
                } else if (i->second != UsdTokens->explicitOnly || 
                           p == path) {
                    if (expansionRule) {
                        *expansionRule = i->second;
                    }
                    return true;
                }
            }
        }
    } else {
        for (SdfPath p = path; p != SdfPath::EmptyPath();  p = p.GetParentPath()) 
        {
            const auto i = _pathExpansionRuleMap.find(p);
            if (i != _pathExpansionRuleMap.end()) {
                if (i->second == UsdTokens->exclude) {
                    if (expansionRule) {
                        *expansionRule = UsdTokens->exclude;
                    }
                    return false;
                } else if ((i->second == UsdTokens->expandPrimsAndProperties) ||
                           (i->second == UsdTokens->explicitOnly && p == path)){
                    if (expansionRule) {
                        *expansionRule = i->second;
                    }
                    return true;
                }
            }
        }
        
    }

    // Any path that's not explicitly mentioned is not included in the 
    // collection.
    return false;
}

bool 
UsdCollectionAPI::MembershipQuery::IsPathIncluded(
    const SdfPath &path,
    const TfToken &parentExpansionRule,
    TfToken *expansionRule) const
{
    // Only prims and properties can belong to a collection.
    if (!path.IsPrimPath() && !path.IsPropertyPath()) 
        return false;

    // Check if there's a direct entry in the path-expansionRule map.
    const auto i = _pathExpansionRuleMap.find(path);
    if (i != _pathExpansionRuleMap.end()) {
        if (expansionRule) {
            *expansionRule = i->second;
        }
        return i->second != UsdTokens->exclude;
    }

    // There's no direct-entry, so decide based on the parent path's 
    // expansion-rule.
    if (path.IsPrimPath()) {
        bool parentIsExcludedOrExplicitlyIncluded = 
                (parentExpansionRule == UsdTokens->exclude ||
                    parentExpansionRule == UsdTokens->explicitOnly);

        if (expansionRule) {
            *expansionRule = parentIsExcludedOrExplicitlyIncluded ? 
                UsdTokens->exclude : parentExpansionRule;
        }

        return !parentIsExcludedOrExplicitlyIncluded;

    } else {
        // If it's a property path, then the path is excluded unless its 
        // parent-path's expansionRule is "expandPrimsAndProperties".
        if (expansionRule) {
            *expansionRule = 
                (parentExpansionRule == UsdTokens->expandPrimsAndProperties) ?
                UsdTokens->expandPrimsAndProperties : UsdTokens->exclude;
        }
        return parentExpansionRule == UsdTokens->expandPrimsAndProperties;
    }
}

void 
UsdCollectionAPI::MembershipQuery::_AppendIncludedPath(
    const SdfPath &path,
    const TfToken &expansionRule)
{
    _pathExpansionRuleMap[path] = expansionRule;
}

void
UsdCollectionAPI::MembershipQuery::_AppendExcludedPath(const SdfPath &path) 
{
    _pathExpansionRuleMap[path] = UsdTokens->exclude;
    _hasExcludes = true;
}

void
UsdCollectionAPI::MembershipQuery::_MergeMembershipQuery(
    const UsdCollectionAPI::MembershipQuery &query) 
{
    // We can't just do an insert here as we need to overwrite existing 
    // entries with new values of expansion rule from 'query'.
    for (const auto &pathAndExpansionRule : query._pathExpansionRuleMap) {
        if (pathAndExpansionRule.second == UsdTokens->exclude) {
            _hasExcludes = true;
        }
        _pathExpansionRuleMap[pathAndExpansionRule.first] = 
            pathAndExpansionRule.second;
    }
}

size_t
UsdCollectionAPI::MembershipQuery::Hash::operator()( MembershipQuery const& q)
    const
{
    TRACE_FUNCTION();

    // Hashing unordered maps is costly because two maps holding the
    // same (key,value) pairs may store them in a different layout,
    // due to population history.  We must use a history-independent
    // order to compute a consistent hash value.
    //
    // If the runtime cost becomes problematic, we should consider
    // computing the hash once and storing it in the MembershipQuery,
    // as a finalization step in _ComputeMembershipQueryImpl().
    typedef std::pair<SdfPath, TfToken> _Entry;
    std::vector<_Entry> entries(q._pathExpansionRuleMap.begin(),
                                q._pathExpansionRuleMap.end());
    std::sort(entries.begin(), entries.end());
    size_t h = 0;
    for (_Entry const& entry: entries) {
        boost::hash_combine(h, entry.first);
        boost::hash_combine(h, entry.second);
    }
    // Don't hash _hasExcludes because it is derived from
    // the contents of _pathExpansionRuleMap.
    return h;
}

PXR_NAMESPACE_CLOSE_SCOPE
