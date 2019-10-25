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
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/payloads.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/specializes.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/variantSets.h"

#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/singularTask.h"

#include "pxr/base/tf/ostreamMethods.h"

#include <boost/functional/hash.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/parallel_sort.h>

#include <algorithm>
#include <functional>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

UsdPrim
UsdPrim::GetChild(const TfToken &name) const
{
    return GetStage()->GetPrimAtPath(GetPath().AppendChild(name));
}

SdfPrimSpecHandle
UsdPrim::GetPrimDefinition() const
{
    const TfToken &typeName = GetTypeName();
    SdfPrimSpecHandle definition;

    if (!typeName.IsEmpty()) {
        // Look up definition from prim type name.
        definition = UsdSchemaRegistry::GetPrimDefinition(typeName);

        if (!definition) {
            // Issue a diagnostic for unknown prim types.
            TF_WARN("No definition for prim type '%s', <%s>",
                    typeName.GetText(), GetPath().GetText());
        }
    }
    return definition;
}

bool
UsdPrim::_IsA(const TfType& schemaType, bool validateSchemaType) const
{
    if (validateSchemaType) {
        // Check Schema TfType
        if (schemaType.IsUnknown()) {
            TF_CODING_ERROR("Unknown schema type (%s) is invalid for IsA query",
                            schemaType.GetTypeName().c_str());
            return false;
        }
    }

    // Get Prim TfType
    const std::string &typeName = GetTypeName().GetString();

    return !typeName.empty() &&
        PlugRegistry::FindDerivedTypeByName<UsdSchemaBase>(typeName).
        IsA(schemaType);
}

bool
UsdPrim::IsA(const TfType& schemaType) const
{
    return _IsA(schemaType, true);
}

bool
UsdPrim::_HasAPI(
    const TfType& schemaType, 
    bool validateSchemaType, 
    const TfToken &instanceName) const 
{
    TRACE_FUNCTION();

    static const auto apiSchemaBaseType = 
            TfType::Find<UsdAPISchemaBase>();

    const bool isMultipleApplyAPISchema = 
        UsdSchemaRegistry::GetInstance().IsMultipleApplyAPISchema(schemaType);

    // Note that this block of code is only hit in python code paths,
    // C++ clients would hit the static_asserts defined inline in 
    // UsdPrim::HasAPI().
    if (validateSchemaType) {
        if (schemaType.IsUnknown()) {
            TF_CODING_ERROR("HasAPI: Invalid unknown schema type (%s) ",
                            schemaType.GetTypeName().c_str());
            return false;
        }

        if (UsdSchemaRegistry::GetInstance().IsTyped(schemaType)) {
            TF_CODING_ERROR("HasAPI: provided schema type ( %s ) is typed.",
                            schemaType.GetTypeName().c_str());
            return false;
        }

        if (!UsdSchemaRegistry::GetInstance().IsAppliedAPISchema(schemaType)) {
            TF_CODING_ERROR("HasAPI: provided schema type ( %s ) is not an "
                "applied API schema type.", schemaType.GetTypeName().c_str());
            return false;
        }

        if (!schemaType.IsA(apiSchemaBaseType) || 
            schemaType == apiSchemaBaseType) {
            TF_CODING_ERROR("HasAPI: provided schema type ( %s ) does not "
                "derive from UsdAPISchemaBase.", 
                schemaType.GetTypeName().c_str());
            return false;
        }

        if (!isMultipleApplyAPISchema && !instanceName.IsEmpty()) {
            TF_CODING_ERROR("HasAPI: single application API schemas like %s do "
                "not contain an application instanceName ( %s ).",
                schemaType.GetTypeName().c_str(), instanceName.GetText());
            return false;
        }
    }

    // Get our composed set of all applied schemas.
    auto appliedSchemas = GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return false;
    }

    auto foundMatch = [&appliedSchemas, isMultipleApplyAPISchema, &instanceName]
            (const std::string &alias) {
        // If instanceName is not empty, look for an exact match in the 
        // apiSchemas list.
        if (!instanceName.IsEmpty()) {
            const TfToken apiName(SdfPath::JoinIdentifier(
                    alias, instanceName.GetString()));
            return std::find(appliedSchemas.begin(), appliedSchemas.end(), 
                             apiName) != appliedSchemas.end();
        } 
        // If we're looking for a multiple-apply API schema, then we return 
        // true if we find an applied schema name that starts with "<alias>:".
        else if (isMultipleApplyAPISchema) {
            return std::any_of(appliedSchemas.begin(), appliedSchemas.end(), 
                [&alias](const TfToken &appliedSchema) {
                    return TfStringStartsWith(appliedSchema, 
                            alias + UsdObject::GetNamespaceDelimiter());
                });
        } else {
            // If instanceName is empty and if schemaType is not a multiple 
            // apply API schema, then we can look for an exact match.
            return std::find(appliedSchemas.begin(), appliedSchemas.end(), 
                             alias) != appliedSchemas.end();
        }
    };

    // See if our schema is directly authored
    static const auto schemaBaseType = TfType::Find<UsdSchemaBase>();
    for (const auto& alias : schemaBaseType.GetAliases(schemaType)) {
        if (foundMatch(alias)) {
            return true; 
        }
    }

    // If we couldn't find it directly authored in apiSchemas, 
    // consider derived types. For example, if a user queries
    // prim.HasAPI<UsdModelAPI>() on a prim with 
    // apiSchemas = ["UsdGeomModelAPI"], we should return true
    std::set<TfType> derivedTypes;
    schemaType.GetAllDerivedTypes(&derivedTypes);
    for (const auto& derived : derivedTypes) {
        for (const auto& alias : schemaBaseType.GetAliases(derived)) {
            if (foundMatch(alias)) { 
                return true; 
            }
        }
    }

    return false;
}

bool
UsdPrim::HasAPI(const TfType& schemaType, const TfToken& instanceName) const{
    return _HasAPI(schemaType, true, instanceName);
}

std::vector<UsdProperty>
UsdPrim::_MakeProperties(const TfTokenVector &names) const
{
    std::vector<UsdProperty> props;
    UsdStage *stage = _GetStage();
    props.reserve(names.size());
    for (auto const &propName : names) {
        SdfSpecType specType =
            stage->_GetDefiningSpecType(get_pointer(_Prim()), propName);
        if (specType == SdfSpecTypeAttribute) {
            props.push_back(GetAttribute(propName));
        } else if (TF_VERIFY(specType == SdfSpecTypeRelationship)) {
            props.push_back(GetRelationship(propName));
        }
    }
    return props;
}

// Change the order of items in 'names' so that all the things in 'order' that
// are also in 'names' are at the beginning in the order that they appear in
// 'order', followed by any remaining items in 'names' in their existing order.
static void
_ApplyOrdering(const TfTokenVector &order, TfTokenVector *names)
{
    // If order is empty or names is empty, nothing to do.
    if (order.empty() || names->empty())
        return;

    // Perf note: this walks 'order' and linear searches 'names' to find each
    // element, for O(M*N) operations, where M and N are the lengths of 'order'
    // and 'names'.  We hope 1) that propertyOrder stmts are relatively rare and
    // 2) that property lists are relatively short.  If those assumptions fail,
    // this may need revisiting.  In some quick microbenchmarking, this linear
    // search seems to outperform binary search up to about 5000 names.  We
    // suspect this is because linear search does TfToken pointer comparisons,
    // while binary search has to dereference and do string comparisons.

    typedef TfTokenVector::iterator Iter;

    Iter namesRest = names->begin(), namesEnd = names->end();
    for (const TfToken &oName: order) {
        // Look for this name from 'order' in the rest of 'names'.
        Iter i = std::find(namesRest, namesEnd, oName);
        if (i != namesEnd) {
            // Found.  Move to the front by rotating the sub-range.  Using
            // std::rotate invokes swap(), which avoids TfToken refcounting.
            // Also advance 'namesRest' to the next element.
            std::rotate(namesRest++, i, i+1);
        }
    }
}

bool 
UsdPrim::RemoveProperty(const TfToken &propName) 
{
    SdfPath propPath = GetPath().AppendProperty(propName);
    return _GetStage()->_RemoveProperty(propPath);
}

UsdProperty
UsdPrim::GetProperty(const TfToken &propName) const
{
    SdfSpecType specType =
        _GetStage()->_GetDefiningSpecType(get_pointer(_Prim()), propName);
    if (specType == SdfSpecTypeAttribute) {
        return GetAttribute(propName);
    }
    else if (specType == SdfSpecTypeRelationship) {
        return GetRelationship(propName);
    }
    return UsdProperty(UsdTypeProperty, _Prim(), _ProxyPrimPath(), propName);
}

bool
UsdPrim::HasProperty(const TfToken &propName) const 
{
    return static_cast<bool>(GetProperty(propName));
}

TfTokenVector
UsdPrim::GetPropertyOrder() const
{
    TfTokenVector order;
    GetMetadata(SdfFieldKeys->PropertyOrder, &order);
    return order;
}

void
UsdPrim::SetPropertyOrder(const TfTokenVector &order) const
{
    SetMetadata(SdfFieldKeys->PropertyOrder, order);
}


static void
_ComposePrimPropertyNames( 
    const PcpPrimIndex& primIndex,
    const PcpNodeRef& node,
    const UsdPrim::PropertyPredicateFunc &predicate,
    TfTokenVector *names,
    TfTokenVector *localNames)
{
    if (node.IsCulled()) {
        return;
    }

    // Strength-order does not matter here, since we're just collecting all 
    // names.
    TF_FOR_ALL(child, node.GetChildrenRange()) {
        _ComposePrimPropertyNames(primIndex, *child, predicate, names,
                                  localNames);
    }

    // Compose the site's local names over the current result.
    if (node.CanContributeSpecs()) {
        for (auto &layer : node.GetLayerStack()->GetLayers()) {
            if (layer->HasField<TfTokenVector>(node.GetPath(), 
                    SdfChildrenKeys->PropertyChildren, localNames)) {
                // If predicate is valid, then append only the names that pass 
                // the predicate. If not, add all names (including duplicates).
                if (predicate) {
                    for(auto &name: *localNames) {
                        if (predicate(name)) {
                            names->push_back(name);
                        }
                    }    
                } else {
                    names->insert(names->end(), localNames->begin(), 
                                  localNames->end());
                }
            }
        }
    }
}

// This function and the one above (_ComposePrimPropertyNames) were copied 
// from Pcp/{PrimIndex,ComposeSite} and optimized for Usd.
static void
_ComputePrimPropertyNames(
    const PcpPrimIndex &primIndex,
    const UsdPrim::PropertyPredicateFunc &predicate,
    TfTokenVector *names)
{
    if (!primIndex.IsValid()) {
        return;
    }

    TRACE_FUNCTION();

    // Temporary shared vector for collecting local property names.
    // This is used to re-use storage allocated for the local property 
    // names in each layer.
    TfTokenVector localNames;

    // Walk the graph to compose prim child names.
    _ComposePrimPropertyNames(primIndex, primIndex.GetRootNode(), predicate, 
                              names, &localNames);
}

TfTokenVector
UsdPrim::_GetPropertyNames(
    bool onlyAuthored, 
    bool applyOrder,
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    TfTokenVector names;

    // If we're including unauthored properties, take names from definition, if
    // present.
    if (!onlyAuthored) {   
        if (predicate) {
            TfTokenVector builtInNames;
            UsdSchemaRegistry::HasField(GetTypeName(), TfToken(),
                    SdfChildrenKeys->PropertyChildren, &builtInNames);

            for (const auto &builtInName : builtInNames) {
                if (predicate(builtInName)) {
                    names.push_back(builtInName);
                }
            }
        } else {
            UsdSchemaRegistry::HasField(GetTypeName(), TfToken(),
                SdfChildrenKeys->PropertyChildren, &names);
        }
    }

    // Add authored names, then sort and apply ordering.
    _ComputePrimPropertyNames(GetPrimIndex(), predicate, &names);

    if (!names.empty()) {
        // Sort and uniquify the names.
        sort(names.begin(), names.end(), TfDictionaryLessThan());
        names.erase(std::unique(names.begin(), names.end()), names.end());
        if (applyOrder) {
            _ApplyOrdering(GetPropertyOrder(), &names);
        }
    }

    return names;
}

TfTokenVector
UsdPrim::GetAppliedSchemas() const
{
    SdfTokenListOp appliedSchemas;
    GetMetadata(UsdTokens->apiSchemas, &appliedSchemas);
    TfTokenVector result;
    appliedSchemas.ApplyOperations(&result);
    return result;
}

TfTokenVector
UsdPrim::GetPropertyNames(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _GetPropertyNames(/*onlyAuthored=*/ false, 
                             /*applyOrder*/ true, 
                             predicate);
}

TfTokenVector
UsdPrim::GetAuthoredPropertyNames(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _GetPropertyNames(/*onlyAuthored=*/ true, 
                             /*applyOrder*/ true, 
                             predicate);
}

std::vector<UsdProperty>
UsdPrim::GetProperties(const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _MakeProperties(GetPropertyNames(predicate));
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredProperties(
    const UsdPrim::PropertyPredicateFunc &predicate) const
{
    return _MakeProperties(GetAuthoredPropertyNames(predicate));
}

std::vector<UsdProperty>
UsdPrim::_GetPropertiesInNamespace(
    const std::string &namespaces,
    bool onlyAuthored) const
{
    if (namespaces.empty())
        return onlyAuthored ? GetAuthoredProperties() : GetProperties();

    const char delim = UsdObject::GetNamespaceDelimiter();

    // Set terminator to the expected position of the delimiter after all the
    // supplied namespaces.  We perform an explicit test for this char below
    // so that we don't need to allocate a new string if namespaces does not
    // already end with the delimiter
    const size_t terminator = namespaces.size() -
        (*namespaces.rbegin() == delim);

    TfTokenVector names = _GetPropertyNames(onlyAuthored, 
        /*applyOrder=*/ true,
        /*predicate*/ [&namespaces, terminator, delim](const TfToken &name) {
            const std::string &s = name.GetString();
            return s.size() > terminator &&
                   TfStringStartsWith(s, namespaces)   && 
                   s[terminator] == delim;
        });

    return _MakeProperties(names);
}

std::vector<UsdProperty>
UsdPrim::GetPropertiesInNamespace(const std::string &namespaces) const
{
    return _GetPropertiesInNamespace(namespaces, /*onlyAuthored=*/false);
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredPropertiesInNamespace(const std::string &namespaces) const
{
    return _GetPropertiesInNamespace(namespaces, /*onlyAuthored=*/true);
}

std::vector<UsdProperty>
UsdPrim::GetPropertiesInNamespace(const std::vector<std::string> &namespaces) const
{
    return GetPropertiesInNamespace(SdfPath::JoinIdentifier(namespaces));
}

std::vector<UsdProperty>
UsdPrim::GetAuthoredPropertiesInNamespace(
    const std::vector<std::string> &namespaces) const
{
    return GetAuthoredPropertiesInNamespace(
        SdfPath::JoinIdentifier(namespaces));
}

UsdAttribute
UsdPrim::CreateAttribute(const TfToken& name,
                         const SdfValueTypeName &typeName,
                         bool custom,
                         SdfVariability variability) const
{
    UsdAttribute attr = GetAttribute(name);
    attr._Create(typeName, custom, variability);
    return attr;
}

UsdAttribute
UsdPrim::CreateAttribute(const TfToken& name,
                         const SdfValueTypeName &typeName,
                         SdfVariability variability) const
{
    return CreateAttribute(name, typeName, /*custom=*/true, variability);
}

UsdAttribute
UsdPrim::CreateAttribute(const std::vector<std::string> &nameElts,
                         const SdfValueTypeName &typeName,
                         bool custom,
                         SdfVariability variability) const
{
    return CreateAttribute(TfToken(SdfPath::JoinIdentifier(nameElts)),
                           typeName, custom, variability);
}

UsdAttribute
UsdPrim::CreateAttribute(const std::vector<std::string> &nameElts,
                         const SdfValueTypeName &typeName,
                         SdfVariability variability) const
{
    return CreateAttribute(nameElts, typeName, /*custom=*/true, variability);
}

UsdAttributeVector
UsdPrim::_GetAttributes(bool onlyAuthored, bool applyOrder) const
{
    const TfTokenVector names = _GetPropertyNames(onlyAuthored, applyOrder);
    UsdAttributeVector attrs;

    // PERFORMANCE: This is sloppy, since property names are a superset of
    // attribute names, however this vector is likely short lived and worth the
    // trade off of repeated reallocation.
    attrs.reserve(names.size());
    for (const auto& propName : names) {
        if (UsdAttribute attr = GetAttribute(propName)) {
            attrs.push_back(attr);
        }
    }

    return attrs;
}

UsdAttributeVector
UsdPrim::GetAttributes() const
{
    return _GetAttributes(/*onlyAuthored=*/false, /*applyOrder=*/true);
}

UsdAttributeVector
UsdPrim::GetAuthoredAttributes() const
{
    return _GetAttributes(/*onlyAuthored=*/true, /*applyOrder=*/true);
}

UsdAttribute
UsdPrim::GetAttribute(const TfToken& attrName) const
{
    // An invalid prim will present a coding error, and then return an
    // invalid attribute
    return UsdAttribute(_Prim(), _ProxyPrimPath(), attrName);
}

bool
UsdPrim::HasAttribute(const TfToken& attrName) const
{
    return static_cast<bool>(GetAttribute(attrName));
}

UsdRelationship
UsdPrim::CreateRelationship(const TfToken& name, bool custom) const
{
    UsdRelationship rel = GetRelationship(name);
    rel._Create(custom);
    return rel;
}

UsdRelationship
UsdPrim::CreateRelationship(const std::vector<std::string> &nameElts, 
                            bool custom) const
{
    return CreateRelationship(TfToken(SdfPath::JoinIdentifier(nameElts)),
                              custom);
}

UsdRelationshipVector
UsdPrim::_GetRelationships(bool onlyAuthored, bool applyOrder) const
{
    const TfTokenVector names = _GetPropertyNames(onlyAuthored, applyOrder);
    UsdRelationshipVector rels;

    // PERFORMANCE: This is sloppy, since property names are a superset of
    // relationship names, however this vector is likely short lived and worth
    // the trade off of repeated reallocation.
    rels.reserve(names.size());
    for (const auto& propName : names) {
        if (UsdRelationship rel = GetRelationship(propName)) {
            rels.push_back(rel);
        }
    }

    return rels;
}

UsdRelationshipVector
UsdPrim::GetRelationships() const
{
    return _GetRelationships(/*onlyAuthored=*/false, /*applyOrder=*/true);
}

UsdRelationshipVector
UsdPrim::GetAuthoredRelationships() const
{
    return _GetRelationships(/*onlyAuthored=*/true, /*applyOrder=*/true);
}

UsdRelationship
UsdPrim::GetRelationship(const TfToken& relName) const
{
    return UsdRelationship(_Prim(), _ProxyPrimPath(), relName);
}

bool
UsdPrim::HasRelationship(const TfToken& relName) const
{
    return static_cast<bool>(GetRelationship(relName));
} 

template <class PropertyType, class Derived>
struct UsdPrim_TargetFinder
{
    using Predicate = std::function<bool (PropertyType const &)>;
    
    static SdfPathVector
    Find(UsdPrim const &prim, Predicate const &pred, bool recurse) {
        UsdPrim_TargetFinder tf(prim, pred, recurse);
        tf._Find();
        return std::move(tf._result);
    }

private:
    explicit UsdPrim_TargetFinder(
        UsdPrim const &prim, Predicate const &pred, bool recurse)
        : _prim(prim)
        , _consumerTask(_dispatcher, [this]() { _ConsumerTask(); })
        , _predicate(pred)
        , _recurse(recurse) {}

    void _Visit(UsdRelationship const &rel) {
        SdfPathVector targets;
        rel._GetForwardedTargets(&targets,
                                 /*includeForwardingRels=*/true);
        _VisitImpl(targets);
    }
    
    void _Visit(UsdAttribute const &attr) {
        SdfPathVector sources;
        attr.GetConnections(&sources);
        _VisitImpl(sources);
    }
    
    void _VisitImpl(SdfPathVector const &paths) {
        if (!paths.empty()) {
            for (SdfPath const &p: paths) {
                _workQueue.push(p);
            }
            _consumerTask.Wake();
        }
        
        if (_recurse) {
            WorkParallelForEach(
                paths.begin(), paths.end(),
                [this](SdfPath const &path) {
                    if (!path.HasPrefix(_prim.GetPath())) {
                        if (UsdPrim owningPrim = _prim.GetStage()->
                            GetPrimAtPath(path.GetPrimPath())) {
                            _VisitSubtree(owningPrim);
                        }
                    }
                });
        }
    }    

    void _VisitPrim(UsdPrim const &prim) {
        if (_seenPrims.insert(prim).second) {
            auto props = static_cast<Derived *>(this)->_GetProperties(prim);
            for (auto const &prop: props) {
                if (!_predicate || _predicate(prop)) {
                    _dispatcher.Run([this, prop]() { _Visit(prop); });
                }
            }
        }
    };

    void _VisitSubtree(UsdPrim const &prim) {
        _VisitPrim(prim);
        auto range = prim.GetDescendants();
        WorkParallelForEach(range.begin(), range.end(),
                            [this](UsdPrim const &desc) { _VisitPrim(desc); });
    }

    void _Find() {
        TF_PY_ALLOW_THREADS_IN_SCOPE();

        _dispatcher.Run([this]() { _VisitSubtree(_prim); });
        _dispatcher.Wait();

        // (We run this parallel_sort in the arena dispatcher to avoid the TBB
        // deadlock issue).
        _dispatcher.Run([this]() {
                tbb::parallel_sort(_result.begin(), _result.end(),
                                   SdfPath::FastLessThan());
            });
        _dispatcher.Wait();

        _result.erase(unique(_result.begin(), _result.end()), _result.end());
    }

    void _ConsumerTask() {
        SdfPath path;
        while (_workQueue.try_pop(path)) {
            _result.push_back(path);
        }
    }

    UsdPrim _prim;
    WorkArenaDispatcher _dispatcher;
    WorkSingularTask _consumerTask;
    Predicate const &_predicate;
    tbb::concurrent_queue<SdfPath> _workQueue;
    tbb::concurrent_unordered_set<UsdPrim, boost::hash<UsdPrim> > _seenPrims;
    SdfPathVector _result;
    bool _recurse;
};
          
struct UsdPrim_RelTargetFinder
    : public UsdPrim_TargetFinder<UsdRelationship, UsdPrim_RelTargetFinder>
{
    std::vector<UsdRelationship> _GetProperties(UsdPrim const &prim) const {
        return prim._GetRelationships(/*onlyAuthored=*/true,
                                      /*applyOrder=*/false);
    }
};

struct UsdPrim_AttrConnectionFinder
    : public UsdPrim_TargetFinder<UsdAttribute, UsdPrim_AttrConnectionFinder>
{
    std::vector<UsdAttribute> _GetProperties(UsdPrim const &prim) const {
        return prim._GetAttributes(/*onlyAuthored=*/true,
                                   /*applyOrder=*/false);
    }
};

USD_API
SdfPathVector
UsdPrim::FindAllAttributeConnectionPaths(
    std::function<bool (UsdAttribute const &)> const &predicate,
    bool recurseOnSources) const
{
    return UsdPrim_AttrConnectionFinder
        ::Find(*this, predicate, recurseOnSources);
}
    
SdfPathVector
UsdPrim::FindAllRelationshipTargetPaths(
    std::function<bool (UsdRelationship const &)> const &predicate,
    bool recurseOnTargets) const
{
    return UsdPrim_RelTargetFinder::Find(*this, predicate, recurseOnTargets);
}

bool
UsdPrim::HasVariantSets() const
{
    return HasMetadata(SdfFieldKeys->VariantSetNames);
}

UsdVariantSets
UsdPrim::GetVariantSets() const
{
    return UsdVariantSets(*this);
}

UsdVariantSet
UsdPrim::GetVariantSet(const std::string& variantSetName) const
{
    return UsdVariantSet(*this, variantSetName);
}


UsdInherits
UsdPrim::GetInherits() const
{
    return UsdInherits(*this);
}

bool
UsdPrim::HasAuthoredInherits() const
{
    return HasMetadata(SdfFieldKeys->InheritPaths);
}

UsdSpecializes
UsdPrim::GetSpecializes() const
{
    return UsdSpecializes(*this);
}

bool
UsdPrim::HasAuthoredSpecializes() const
{
    return HasMetadata(SdfFieldKeys->Specializes);
}

UsdReferences
UsdPrim::GetReferences() const
{
    return UsdReferences(*this);
}

bool
UsdPrim::HasAuthoredReferences() const
{
    return HasMetadata(SdfFieldKeys->References);
}

// --------------------------------------------------------------------- //
/// \name Payloads, Load and Unload
// --------------------------------------------------------------------- //

bool
UsdPrim::HasPayload() const
{
    return HasAuthoredPayloads();
}

bool
UsdPrim::SetPayload(const SdfPayload& payload) const
{
    UsdPayloads payloads = GetPayloads();
    payloads.ClearPayloads();
    return payloads.SetPayloads(SdfPayloadVector{payload});
}

bool
UsdPrim::SetPayload(const std::string& assetPath, const SdfPath& primPath) const
{
    return SetPayload(SdfPayload(assetPath, primPath));
}

bool
UsdPrim::SetPayload(const SdfLayerHandle& layer, const SdfPath& primPath) const
{
    return SetPayload(SdfPayload(layer->GetIdentifier(), primPath));
}

bool
UsdPrim::ClearPayload() const
{
    return GetPayloads().ClearPayloads();
}

UsdPayloads
UsdPrim::GetPayloads() const
{
    return UsdPayloads(*this);
}

bool
UsdPrim::HasAuthoredPayloads() const
{
    // Unlike the equivalent function for references, we query the prim data
    // for the cached value of HasPayload computed by Pcp instead of querying
    // the composed metadata. This is necessary as this function is called by 
    // _IncludeNewlyDiscoveredPayloadsPredicate in UsdStage which can't safely
    // call back into the querying the composed metatdata.
    return _Prim()->HasPayload();
}

void
UsdPrim::Load(UsdLoadPolicy policy) const
{
    if (IsInMaster()) {
        TF_CODING_ERROR("Attempted to load a prim in a master <%s>",
                        GetPath().GetText());
        return;
    }
    _GetStage()->Load(GetPath(), policy);
}

void
UsdPrim::Unload() const
{
    if (IsInMaster()) {
        TF_CODING_ERROR("Attempted to unload a prim in a master <%s>",
                        GetPath().GetText());
        return;
    }
    _GetStage()->Unload(GetPath());
}

UsdPrim
UsdPrim::GetNextSibling() const
{
    return GetFilteredNextSibling(UsdPrimDefaultPredicate);
}

UsdPrim
UsdPrim::GetFilteredNextSibling(const Usd_PrimFlagsPredicate &inPred) const
{
    Usd_PrimDataConstPtr sibling = get_pointer(_Prim());
    SdfPath siblingPath = _ProxyPrimPath();
    const Usd_PrimFlagsPredicate pred = 
        Usd_CreatePredicateForTraversal(sibling, siblingPath, inPred);

    if (Usd_MoveToNextSiblingOrParent(sibling, siblingPath, pred)) {
        return UsdPrim();
    }
    return UsdPrim(sibling, siblingPath);
}

bool
UsdPrim::IsPseudoRoot() const
{
    return GetPath() == SdfPath::AbsoluteRootPath();
}

UsdPrim
UsdPrim::GetMaster() const
{
    Usd_PrimDataConstPtr masterPrimData = 
        _GetStage()->_GetMasterForInstance(get_pointer(_Prim()));
    return UsdPrim(masterPrimData, SdfPath());
}

bool 
UsdPrim::_PrimPathIsInMaster() const
{
    return Usd_InstanceCache::IsPathInMaster(GetPrimPath());
}

SdfPrimSpecHandleVector 
UsdPrim::GetPrimStack() const
{
    SdfPrimSpecHandleVector primStack;

    for (Usd_Resolver resolver(&(_Prim()->GetPrimIndex())); 
                      resolver.IsValid(); resolver.NextLayer()) {

        auto primSpec = resolver.GetLayer()
            ->GetPrimAtPath(resolver.GetLocalPath());

        if (primSpec) { 
            primStack.push_back(primSpec); 
        }
    }

    return primStack;
}

PcpPrimIndex 
UsdPrim::ComputeExpandedPrimIndex() const
{
    // Get the prim index path to compute from the index stored in the prim
    // data. This ensures we get consistent behavior when dealing with 
    // instancing and instance proxies.
    const PcpPrimIndex& cachedPrimIndex = _Prim()->GetPrimIndex();
    if (!cachedPrimIndex.IsValid()) {
        return PcpPrimIndex();
    }
    
    const SdfPath& primIndexPath = cachedPrimIndex.GetPath();
    PcpCache* cache = _GetStage()->_GetPcpCache();
    
    PcpPrimIndexOutputs outputs;
    PcpComputePrimIndex(
        primIndexPath, cache->GetLayerStack(),
        cache->GetPrimIndexInputs().Cull(false), 
        &outputs);

    _GetStage()->_ReportPcpErrors(
        outputs.allErrors, 
        TfStringPrintf(
            "computing expanded prim index for <%s>", GetPath().GetText()));
    
    return outputs.primIndex;
}

PXR_NAMESPACE_CLOSE_SCOPE

