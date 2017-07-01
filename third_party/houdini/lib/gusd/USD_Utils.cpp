//
// Copyright 2017 Pixar
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
#include "gusd/USD_Utils.h"

#include <UT/UT_Interrupt.h>
#include <UT/UT_ParallelUtil.h>
#include <UT/UT_Set.h>
#include <UT/UT_String.h>
#include <UT/UT_StringMMPattern.h>
#include <UT/UT_ThreadSpecificValue.h>
#include <UT/UT_WorkArgs.h>
#include <UT/UT_WorkBuffer.h>

#include "gusd/UT_Usd.h"

#include "pxr/base/tf/type.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace GusdUSD_Utils {


PrimIdentifier::PrimIdentifier(const UT_StringRef& primPath,
                               const UT_StringRef& variants,
                               GusdUT_ErrorContext* err)
{
    /* Identifier should be invalid (invalid prim path) if parsing
       either input fails, so handle the variants first.*/
    if(SetVariants(variants, err))
        SetPrimPath(primPath, err);
}


bool
PrimIdentifier::SetFromVariantPath(const SdfPath& variants)
{
    ExtractPathComponents(variants, _primPath, _variants);
    return (bool)*this;
}


bool
PrimIdentifier::SetFromVariantPath(const UT_StringRef& variants,
                                   GusdUT_ErrorContext* err)
{
    SdfPath sdfVariants;
    if(GusdUT_CreateSdfPath(variants, sdfVariants, err))
        return SetFromVariantPath(sdfVariants);
    return false;
}


bool
PrimIdentifier::SetPrimPath(const UT_StringRef& path, GusdUT_ErrorContext* err)
{
    return GusdUT_CreateSdfPath(path, _primPath, err);
}


bool
PrimIdentifier::SetVariants(const UT_StringRef& variants,
                            GusdUT_ErrorContext* err)
{
    return GusdUT_CreateSdfPath(variants, _variants, err);
}


bool
GetPrimAndVariantPathsFromPathList(
    const char* str,
    UT_Array<SdfPath>& primPaths,
    UT_Array<SdfPath>& variants,
    GusdUT_ErrorContext* err)
{
    UT_WorkArgs args;
    UT_String tokenStr(str);
    tokenStr.tokenize(args);

    primPaths.setCapacity(args.entries());
    variants.setCapacity(args.entries());
    for(exint i = 0; i < args.entries(); ++i) {
        UT_String arg(args(i));
        if(arg.isstring()) {
            SdfPath path;
            if(GusdUT_CreateSdfPath(arg, path, err)) {
                SdfPath primPath, variantsPath;
                ExtractPathComponents(path, primPath, variantsPath);
                primPaths.append(primPath);
                variants.append(variantsPath);
            } else {
                return false;
            }
        }
    }
    return true;
}


void
ExtractPathComponents(const SdfPath& path, SdfPath& primPath, SdfPath& variants)
{
    if(path.ContainsPrimVariantSelection()) {
        primPath = path.StripAllVariantSelections();
        for(SdfPath p = path; !p.IsEmpty(); p = p.GetParentPath()) {
            if (p.IsPrimVariantSelectionPath()) {
                variants = p;
                break;
            }
        }
    } else {
        primPath = path;
        variants = SdfPath();
    }
}


bool
SortPrims(UT_Array<UsdPrim>& prims)
{
    UTparallelStableSort<UT_Array<UsdPrim>::iterator>(
        prims.begin(), prims.end(),
        [](const UsdPrim& a, const UsdPrim& b)
        { return a.GetPath() < b.GetPath(); });
    return !UTgetInterrupt()->opInterrupt();
}


namespace {


struct _Pattern
{
    _Pattern(const char* pattern, bool caseSensitive)
        { _pattern.compile(pattern, caseSensitive); }
    
    bool    Match(const std::string& str) const
            { return UT_String(str.c_str()).multiMatch(_pattern); }
    bool    Match(const TfToken& str) const
            { return Match(str.GetString()); }
private:
    UT_StringMMPattern  _pattern;
};


void _GetSchemaTypesMatchingPattern(TfType type,
                                    const _Pattern& pattern,
                                    UT_Array<TfType>& types)
{
    if(pattern.Match(type.GetTypeName())) {
        types.append(type);
    } else {
        for(auto& derived : type.GetDirectlyDerivedTypes()) {
            _GetSchemaTypesMatchingPattern(derived, pattern, types);
        }
    }
}


void _GetModelKindsMatchingPattern(const KindNode* kind,
                                   const _Pattern& pattern,
                                   UT_Array<TfToken>& kinds)
{
    if(pattern.Match(kind->kind)) {
        kinds.append(kind->kind);
    } else {
        for(auto& derived : kind->children) {
            _GetModelKindsMatchingPattern(derived.get(), pattern, kinds);
        }
    }
}


} /*namespace*/


void
GetBaseSchemaTypesMatchingPattern(const char* pattern,
                                  UT_Array<TfType>& types,
                                  bool caseSensitive)
{
    types.clear();
    if(!UTisstring(pattern))
        return;

    TfType base = TfType::Find<UsdSchemaBase>();
    UT_ASSERT(!base.IsUnknown());
    _GetSchemaTypesMatchingPattern(
        base, _Pattern(pattern, caseSensitive), types);
}


void
GetBaseModelKindsMatchingPattern(const char* pattern,
                                 UT_Array<TfToken>& kinds,
                                 bool caseSensitive)
{
    kinds.clear();
    if(!UTisstring(pattern))
        return;

    _GetModelKindsMatchingPattern(&GetModelKindHierarchy(),
                                  _Pattern(pattern, caseSensitive), kinds);
}


void
GetPurposesMatchingPattern(const char* pattern,
                           UT_Array<TfToken>& purposes,
                           bool caseSensitive)
{
    purposes.clear();
    if(!UTisstring(pattern))
        return;

    _Pattern pat(pattern, caseSensitive);
    for(auto& purpose : {UsdGeomTokens->default_,
                         UsdGeomTokens->render,
                         UsdGeomTokens->proxy,
                         UsdGeomTokens->guide}) {
        if(pat.Match(purpose))
            purposes.append(purpose);
    }
}


namespace {

KindNode    _BuildModelKindHierarchy()
{
    static KindNode root;

    UT_Map<TfToken,KindNode::RefPtr> kindMap;

    // Create nodes for all kinds.
    for(const auto& kind : KindRegistry::GetAllKinds()) {
        KindNode::RefPtr node(new KindNode);
        node->kind = kind;
        kindMap[kind] = node;
    }

    // Map nodes onto the child map of their parent (base kind)
    for(auto& pair : kindMap) {
        auto baseKind = KindRegistry::GetBaseKind(pair.first);
        KindNode* parent = baseKind.IsEmpty() ? &root : kindMap[baseKind].get();
        parent->children.append(pair.second);
    }

    // Sort all of the children.
    for(auto& pair : kindMap) {
        pair.second->children.stdsort(
            [](const KindNode::RefPtr& a, const KindNode::RefPtr& b)
                { return a->kind < b->kind; });
    }
    root.kind = TfToken();

    return root;
}

} /*namespace*/


const KindNode&
GetModelKindHierarchy()
{
    static const KindNode root = _BuildModelKindHierarchy();
    return root;
}


void
AppendVariantSelectionString(UT_WorkBuffer& buf,
                             const SdfPath& prim,
                             const SdfPath& variants,
                             const std::string& vset,
                             const std::string& sel)
{
    if(buf.isEmpty()) {
        const auto& primStr = prim.GetString();
        if(variants.IsEmpty()) {
            buf.append(primStr.c_str(), primStr.size());
        } else {
            /* Buffer needs to hold the variant selection, as well as any part
               of the path to the prim past that variant.
               I.e., if the variant path is /a{var=sel}, and the prim is /a/b/c,
               we need to fill the buffer with '/a{var=sel}b/c */
            const auto& variantsPrimPath = variants.GetPrimPath();

            size_t variantLength = variantsPrimPath.GetString().size();

            if(prim.HasPrefix(variantsPrimPath)) {
                buf.append(variantsPrimPath.GetString().c_str(), variantLength);
                
                buf.append(primStr.c_str() + variantLength,
                           primStr.size() - variantLength);
            } else {
                /* The variant doesn't apply to this prim,
                   so ignore the variant and init the buffer to the prim path.*/
                buf.append(primStr.c_str(), primStr.size());
            }
        }
    }
    buf.append('{');
    buf.append(vset.c_str(), vset.size());
    buf.append('=');
    buf.append(sel.c_str(), sel.size());
    buf.append('}');
}


namespace {


/** Concurrent map to aid in construction of variant paths when
    appending variant selections.
    For a constant set of variant selections, this computes mappings
    of (prim+old variant path) -> (new variant path with selections).*/
struct _VariantPathMap
{
    typedef std::pair<UT_StringHolder,exint>    StringIndexPair;

    struct _Key
    {
        _Key(const UsdPrim& prim, const SdfPath& variant, exint idx=-1)
            : prim(prim), variant(variant), idx(idx) {}

        bool    operator==(const _Key& o) const
                { return prim == o.prim &&
                        idx == o.idx && variant == o.variant; }

        UsdPrim prim;
        SdfPath variant;
        exint   idx;
    };

    struct _HashKey
    {
        static size_t   hash(const _Key& o)
                        {
                            std::size_t hash = hash_value(o.prim);
                            boost::hash_combine(hash, o.variant);
                            boost::hash_combine(hash, o.idx);
                            return hash;
                        }

        static bool     equal(const _Key& a, const _Key& b)
                        { return a == b; }
    };


    exint   Append(const VariantSelArray& sels,
                   const UsdPrim& prim,
                   const SdfPath& prevVariant,
                   UT_WorkBuffer& buf)
            {
                _Key key(prim, prevVariant);
                
                {
                    _Map::const_accessor a;
                    if(_map.find(a, key))
                        return a->second.second;
                }
                
                _Map::accessor a;
                if(_map.insert(a, key)) {
                    buf.clear();
                    auto vsets = prim.GetVariantSets();

                    bool haveSel = false;
                    for(const auto& sel : sels) {
                        auto vset = vsets.GetVariantSet(sel.variantSet);
                        if(vset.HasAuthoredVariant(sel.variant)) {
                            AppendVariantSelectionString(
                                buf, prim.GetPath(), prevVariant,
                                sel.variantSet, sel.variant);
                            haveSel = true;
                        }
                    }
                    if(haveSel) {
                        a->second.first = buf.buffer();
                        a->second.second = _counter.exchangeAdd(1);
                    } else {
                        a->second.second = -1;
                    }
                }
                return a->second.second;
            }

    exint   Append(const UsdPrim& prim,
                   const SdfPath& prevVariant,
                   const std::string& variantSet,
                   const std::string& variant,
                   exint idx,
                   UT_WorkBuffer& buf)
            {
                _Key key(prim, prevVariant, idx);

                {
                    _Map::const_accessor a;
                    if(_map.find(a, key))
                        return a->second.second;
                }
                
                _Map::accessor a;
                if(_map.insert(a, key)) {
                    buf.clear();
                    AppendVariantSelectionString(
                        buf, prim.GetPath(), prevVariant, variantSet, variant);
                    a->second.first = buf.buffer();
                    a->second.second = _counter.exchangeAdd(1);
                }
                return a->second.second;
            }
                   

    void    GetOrderedPaths(UT_Array<UT_StringHolder>& paths) const
            {
                paths.setSize(_counter.relaxedLoad());
                for(const auto& pair : _map) {
                    if(pair.second.second >= paths.size())
                        paths.bumpSize(pair.second.second + 1);
                    if(pair.second.second >= 0)
                        paths(pair.second.second) = pair.second.first;
                }
            }
private:
    typedef UT_ConcurrentHashMap<_Key, StringIndexPair,_HashKey> _Map;

    _Map            _map;
    SYS_AtomicInt32 _counter;
};


/** Functor for computing new variant paths on prims.
    This populates the @a variantOverrides map, together with the
    @a variantIndices. The latter maps elements of the range into
    the @a variantOverrides map.*/
struct _AppendVariantSelectionsFn
{
    _AppendVariantSelectionsFn(const VariantSelArray& selections,
                               const UT_Array<UsdPrim>& prims,
                               const UT_Array<SdfPath>* prevVariants,
                               UT_Array<exint>& variantIndices,
                               _VariantPathMap& pathMap)
        : _selections(selections), _prims(prims),
          _prevVariants(prevVariants), _variantIndices(variantIndices),
          _pathMap(pathMap) {}

    void    operator()(const UT_BlockedRange<size_t>& r) const
            {
                // Scratch buffer reused when constructing paths.
                UT_WorkBuffer buf; 
        
                auto* boss = UTgetInterrupt();
                char bcnt = 0;
                for(size_t i = r.begin(); i < r.end(); ++i) {
                    if(!++bcnt && boss->opInterrupt())
                        return;

                    if(UsdPrim prim = _prims(i)) {
                        SdfPath variants = _prevVariants ?
                            (*_prevVariants)(i) : SdfPath();

                        _variantIndices(i) = 
                            _pathMap.Append(_selections, prim, variants, buf);
                    }
                }
            }
private:
    const VariantSelArray&          _selections;
    const UT_Array<UsdPrim>&        _prims;
    const UT_Array<SdfPath>* const  _prevVariants;
    UT_Array<exint>&                _variantIndices;
    _VariantPathMap&                _pathMap;
};


} /*namespace*/


bool
AppendVariantSelections(const UT_Array<UsdPrim>& prims,
                        const VariantSelArray& selections, 
                        UT_Array<UT_StringHolder>& orderedVariants,
                        UT_Array<exint>& indices,
                        const UT_Array<SdfPath>* prevVariants)
{
    UT_ASSERT(!prevVariants || prevVariants->size() == prims.size());

    UT_AutoInterrupt task("Append variant selections");
 
    indices.setSize(prims.size());
    
    _VariantPathMap pathMap;
    
    UTparallelFor(UT_BlockedRange<size_t>(0, prims.size()),
                  _AppendVariantSelectionsFn(selections, prims,
                                             prevVariants,
                                             indices, pathMap));
    if(task.wasInterrupted())
        return false;

    pathMap.GetOrderedPaths(orderedVariants);
    return true;
}


namespace {


typedef UT_ThreadSpecificValue<IndexPairArray*> _IndexPairArrayTLS;


struct _IndexPairArrayData
{
    ~_IndexPairArrayData()
    {
        for(auto it = indicesTLS.begin(); it != indicesTLS.end(); ++it) {
            if(auto* tdata = it.get())
                delete tdata;
        }
    }

    _IndexPairArrayTLS  indicesTLS;
};


/** Functor for computing expanded variant paths on prims.*/
struct _ExpandVariantSelectionsFn
{
    _ExpandVariantSelectionsFn(const std::string& variantSet,
                               const NameMatcher& matcher,
                               const UT_Array<UsdPrim>& prims,
                               const UT_Array<SdfPath>* prevVariants,
                               _IndexPairArrayTLS& indicesTLS,
                               _VariantPathMap& pathMap)
        : _variantSet(variantSet), _matcher(matcher), _prims(prims),
          _prevVariants(prevVariants), _indicesTLS(indicesTLS),
          _pathMap(pathMap) {}

    void    operator()(const UT_BlockedRange<size_t>& r) const
            {
                IndexPairArray*& indices = _indicesTLS.get();
                if(!indices)
                    indices = new IndexPairArray;

                // Scratch buffer reused when constructing paths.
                UT_WorkBuffer buf; 
        
                auto* boss = UTgetInterrupt();
                char bcnt = 0;
                for(size_t i = r.begin(); i < r.end(); ++i) {
                    if(!++bcnt && boss->opInterrupt())
                        return;

                    if(UsdPrim prim = _prims(i)) {
                        SdfPath prevVariant = _prevVariants ?
                            (*_prevVariants)(i) : SdfPath();
                        
                        auto vsets = prim.GetVariantSets();
                        auto vset = vsets.GetVariantSet(_variantSet);
                        exint idx = 0;
                        for(const auto& name : vset.GetVariantNames()) {
                            if(_matcher(name)) {
                                exint variantIdx =
                                    _pathMap.Append(prim, prevVariant,
                                                    _variantSet, name,
                                                    idx, buf);
                                indices->append(IndexPair(i,variantIdx));
                                ++idx;
                            }
                        }
                    }
                }
            }
private:
    const std::string&              _variantSet;
    const NameMatcher&              _matcher;
    const UT_Array<UsdPrim>&        _prims;
    const UT_Array<SdfPath>* const  _prevVariants;
    _IndexPairArrayTLS&             _indicesTLS;
    _VariantPathMap&                _pathMap;
};


bool
_GatherIndexPairsFromThreads(IndexPairArray& indices,
                             _IndexPairArrayTLS& indicesTLS)
{
    indices.clear();
    
    // Compute the full count and pre-allocate space.
    exint nPairs = 0;
    for(auto it = indicesTLS.begin(); it != indicesTLS.end(); ++it) {
        if(const auto* tdata = it.get())
            nPairs += tdata->size();
    }
    indices.setCapacity(nPairs);
    
    // Concat the per-thread arrays.
    for(auto it = indicesTLS.begin(); it != indicesTLS.end(); ++it) {
        if(const auto* tdata = it.get())
            indices.concat(*tdata);
    }

    /* Everything is currently ordered by threads, so will be non-deterministic.
       Sort to make the results deterministic.*/
    UTparallelStableSort<IndexPairArray::iterator>(
        indices.begin(), indices.end());
    return !UTgetInterrupt()->opInterrupt();
}



} /*namespace*/


bool
ExpandVariantSetPaths(const UT_Array<UsdPrim>& prims,
                      const std::string& variantSet,
                      const NameMatcher& matcher,
                      UT_Array<UT_StringHolder>& orderedVariants,
                      IndexPairArray& indices,
                      const UT_Array<SdfPath>* prevVariants)
{
    UT_ASSERT(!prevVariants || prevVariants->size() == prims.size());

    UT_AutoInterrupt task("Append variant selections");
 
    _VariantPathMap pathMap;
    
    _IndexPairArrayData data;

    UTparallelFor(UT_BlockedRange<size_t>(0, prims.size()),
                  _ExpandVariantSelectionsFn(variantSet, matcher, prims,
                                             prevVariants, data.indicesTLS,
                                             pathMap));
    if(task.wasInterrupted())
        return false;

    pathMap.GetOrderedPaths(orderedVariants);
    
    return _GatherIndexPairsFromThreads(indices, data.indicesTLS);
}


namespace {


struct _TokenIndexMap
{
    exint   Append(const TfToken& key)
            {
                {
                    _Map::const_accessor a;
                    if(_map.find(a, key))
                        return a->second;
                }
                _Map::accessor a;
                if(_map.insert(a, key))
                    a->second = _counter.exchangeAdd(1);
                return a->second;
            }

    void    GetOrderedTokens(UT_Array<UT_StringHolder>& tokens) {
                tokens.setSize(_counter.relaxedLoad());
                for(const auto& pair : _map) {
                    if(pair.second >= tokens.size())
                        tokens.bumpSize(pair.second + 1);
                    if(pair.second >= 0)
                        tokens(pair.second) = \
                            UT_StringHolder(pair.first.GetString().c_str(),
                                            UT_StringHolder::REFERENCE);
                }
            }
    
private:
    struct _HashCmp
    {
        static size_t   hash(const TfToken& t)
                        { return hash_value(t); }
        static bool     equal(const TfToken&a, const TfToken& b)
                        { return a == b; }
    };

    typedef UT_ConcurrentHashMap<TfToken,exint,_HashCmp> _Map;
    _Map            _map;
    SYS_AtomicInt32 _counter;
};


struct _GetPropertyNamesFn
{
    _GetPropertyNamesFn(const NameMatcher& matcher,
                        const UT_Array<UsdPrim>& prims,
                        _IndexPairArrayTLS& indicesTLS,
                        _TokenIndexMap& map,
                        const std::string& nameSpace)
        : _matcher(matcher), _prims(prims),
          _indicesTLS(indicesTLS), _map(map), _nameSpace(nameSpace) {}

    void    operator()(const UT_BlockedRange<size_t>& r) const
            {   
                IndexPairArray*& indices = _indicesTLS.get();
                if(!indices)
                    indices = new IndexPairArray;

                auto* boss = UTgetInterrupt();
                char bcnt = 0;

                for(size_t i = r.begin(); i < r.end(); ++i) {
                    if(!++bcnt && boss->opInterrupt())
                        return;
                    
                    if(UsdPrim prim = _prims(i)) {
                        const auto props =
                            prim.GetPropertiesInNamespace(_nameSpace);
                        for(const auto& prop : props) {
                            const auto& name = prop.GetName();
                            if(_matcher(name.GetString())) {
                                indices->append(
                                    IndexPair(i,_map.Append(name)));
                            }
                        }
                    }
                }
            }
private:
    const NameMatcher&          _matcher;
    const UT_Array<UsdPrim>&    _prims;
    _IndexPairArrayTLS&         _indicesTLS;
    _TokenIndexMap&             _map;
    const std::string&          _nameSpace;
};


} /*namespace*/


bool
GetPropertyNames(const UT_Array<UsdPrim>& prims,
                 const NameMatcher& matcher,
                 UT_Array<UT_StringHolder>& orderedNames,
                 IndexPairArray& indices,
                 const std::string& nameSpace)
{
    UT_AutoInterrupt task("Compute matching property names");

    _TokenIndexMap tokenMap;
    _IndexPairArrayData data;
    
    UTparallelFor(UT_BlockedRange<size_t>(0, prims.size()),
                  _GetPropertyNamesFn(matcher, prims, data.indicesTLS,
                                      tokenMap, nameSpace));
    if(task.wasInterrupted())
        return false;
    
    tokenMap.GetOrderedTokens(orderedNames);

    return _GatherIndexPairsFromThreads(indices, data.indicesTLS);
}


namespace {


void
_SortedStringArrayFromSet(const UT_Set<UT_StringHolder>& set,
                          UT_Array<UT_StringHolder>& array)
{
    array.setSize(set.size());
    exint idx = 0;
    for(const auto& str : set) {
        array(idx) = str;
        ++idx;
    }
    array.stdsort(std::less<UT_StringHolder>());
}


} /*namespace*/


bool
GetUniqueVariantSetNames(const UT_Array<UsdPrim>& prims,
                         UT_Array<UT_StringHolder>& names)
{
    /* This isn't used in performance sensitive contexts,
       so just loop in serial for now.*/
    UT_Set<UsdPrim> visited;
    UT_Set<UT_StringHolder> nameSet;
    for(const auto& prim : prims) {
        if(visited.find(prim) != visited.end())
            continue;

        auto vsets = prim.GetVariantSets();
        for(const auto& name : vsets.GetNames()) {
            if(nameSet.find(UT_StringRef(name.c_str())) == nameSet.end())
                nameSet.insert(UT_StringHolder(name.c_str()));
        }
        visited.insert(prim);
    }
    _SortedStringArrayFromSet(nameSet, names);
    return true;
}


bool
GetUniqueVariantNames(const UT_Array<UsdPrim>& prims,
                      const std::string& variantSet,
                      UT_Array<UT_StringHolder>& names)
{
    /* This isn't used in performance sensitive contexts,
       so just loop in serial for now.*/
    UT_Set<UsdPrim> visited;
    UT_Set<UT_StringHolder> nameSet;
    for(const auto& prim : prims) {
        if(visited.find(prim) != visited.end())
            continue;

        auto vsets = prim.GetVariantSets();
        auto vset = vsets.GetVariantSet(variantSet);
        for(const auto& name : vset.GetVariantNames()) {
            if(nameSet.find(UT_StringRef(name.c_str())) == nameSet.end())
                nameSet.insert(UT_StringHolder(name.c_str()));
        }
        visited.insert(prim);
    }
    _SortedStringArrayFromSet(nameSet, names);
    return true;
}


bool
GetUniquePropertyNames(const UT_Array<UsdPrim>& prims,
                       UT_Array<UT_StringHolder>& names,
                       const std::string& nameSpace)
{
    /* This isn't used in performance sensitive contexts,
       so just loop in serial for now.*/    
    UT_Set<UsdPrim> visited;
    std::set<TfToken> nameSet; // use std::set for sorting.
    for(const auto& prim : prims) {
        if(visited.find(prim) != visited.end())
            continue;
        for(auto p : prim.GetPropertiesInNamespace(nameSpace)) {
            const auto& name = p.GetName();
            if(nameSet.find(name) == nameSet.end())
                nameSet.insert(name);
        }
        visited.insert(prim);
    }
    names.setSize(nameSet.size());
    exint idx = 0;
    for(const auto& name : nameSet) {
        names(idx) = UT_StringHolder(name.GetString().c_str());
        ++idx;
    }
    return true;
}


} /*namespace GusdUSD_Utils */

PXR_NAMESPACE_CLOSE_SCOPE
