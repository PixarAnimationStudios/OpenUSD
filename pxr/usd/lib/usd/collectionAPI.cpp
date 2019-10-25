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
static inline
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

using PathExpansionRuleMap = UsdCollectionMembershipQuery::PathExpansionRuleMap;

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
    UsdCollectionMembershipQuery query  = ComputeMembershipQuery();
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
            // Update the query object we have by updating the map and
            // reconstructing the query
            PathExpansionRuleMap map = query.GetAsPathExpansionRuleMap();
            auto it = map.find(pathToInclude);
            if (TF_VERIFY(it != map.end())) {
                map.erase(it);
                query = UsdCollectionMembershipQuery(std::move(map));
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
    UsdCollectionMembershipQuery query = ComputeMembershipQuery();
    if (!query.GetAsPathExpansionRuleMap().empty() &&
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
            PathExpansionRuleMap map = query.GetAsPathExpansionRuleMap();
            auto it = map.find(pathToExclude);
            if (TF_VERIFY(it != map.end())) {
                map.erase(it);
                query = UsdCollectionMembershipQuery(std::move(map));
            }
        }
    }

    // Now that we've removed the explicit include if there was one, 
    // we can add an explicit exclude, if required.
    if (query.GetAsPathExpansionRuleMap().empty() ||
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

UsdCollectionMembershipQuery 
UsdCollectionAPI::ComputeMembershipQuery() const
{
    UsdCollectionMembershipQuery query;
    ComputeMembershipQuery(&query);
    return query;
}

void 
UsdCollectionAPI::ComputeMembershipQuery(
    UsdCollectionMembershipQuery *query) const
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
    UsdCollectionMembershipQuery *query,
    const SdfPathSet &chainedCollectionPaths,
    bool *foundCircularDependency) const
{
    if (!TF_VERIFY(query)) {
        return;
    }

    // Get the map from the query
    PathExpansionRuleMap map = query->GetAsPathExpansionRuleMap();

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
            UsdCollectionMembershipQuery includedQuery;
            includedCollection._ComputeMembershipQueryImpl(&includedQuery,
                    seenCollectionPaths, foundCircularDependency);

            const PathExpansionRuleMap& includedMap =
                includedQuery.GetAsPathExpansionRuleMap();

            // Merge path expansion rule maps
            // We can't just do an insert here as we need to overwrite existing
            // entries with new values of expansion rule from other map
            for (const auto &pathAndExpansionRule : includedMap) {
                map[pathAndExpansionRule.first] = pathAndExpansionRule.second;
            }
        } else {
            // Append included path
            map[includedPath] = expRule;
        }
    }

    // Process the excludes after the includes.
    for (const auto &p: excludes) {
        // Append excluded path
        map[p] = UsdTokens->exclude;
    }

    *query = UsdCollectionMembershipQuery(std::move(map));
}

/* static */
std::set<UsdObject> 
UsdCollectionAPI::ComputeIncludedObjects(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred)
{
    return UsdComputeIncludedObjectsFromCollection(query, stage, pred);
}

/* static */
SdfPathSet
UsdCollectionAPI::ComputeIncludedPaths(
    const UsdCollectionMembershipQuery &query,
    const UsdStageWeakPtr &stage,
    const Usd_PrimFlagsPredicate &pred)
{
    return UsdComputeIncludedPathsFromCollection(query, stage, pred);
}

static bool
_AllRootmostRulesPassFilter(
    UsdCollectionMembershipQuery::PathExpansionRuleMap const& ruleMap,
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
    UsdCollectionMembershipQuery query;
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
            query.GetAsPathExpansionRuleMap(),
            [](std::pair<SdfPath, TfToken> const& rule) -> bool {
                return rule.second == UsdTokens->exclude;
            });
        bool allIncludes = _AllRootmostRulesPassFilter(
            query.GetAsPathExpansionRuleMap(),
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

PXR_NAMESPACE_CLOSE_SCOPE
