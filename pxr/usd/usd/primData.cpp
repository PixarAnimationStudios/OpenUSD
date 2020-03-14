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

#include "pxr/usd/usd/debugCodes.h"
#include "pxr/usd/usd/instanceCache.h"
#include "pxr/usd/usd/resolver.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usd/primRange.h"

#include "pxr/usd/kind/registry.h"

#include "pxr/base/tf/stringUtils.h"

#include <algorithm>
#include <sstream>
#include <vector>
#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Wrapper around the thread safe hash map implementation used by the 
// Usd_PrimTypeInfoCache to cache prim type info
class _ThreadSafeHashMapImpl {
public:
    _ThreadSafeHashMapImpl() = default;
    _ThreadSafeHashMapImpl(const _ThreadSafeHashMapImpl&) = delete;

    // Find and return a pointer to the prim type info if it already exists.
    const Usd_PrimTypeInfo *Find(const TfToken &key) const 
    {
        _HashMap::const_accessor accessor;
        if (_hashMap.find(accessor, key)) {
            return accessor->second.get();
        }
        return nullptr;
    }

    // Inserts and takes ownership of the prim type info only if it isn't 
    // already in the hash map. Returns the pointer to the value in the map
    // after insertion regardless.
    const Usd_PrimTypeInfo *Insert(
        const TfToken &key, std::unique_ptr<Usd_PrimTypeInfo> valuePtr)
    {
        _HashMap::accessor accessor;
        if (_hashMap.insert(accessor, key)) {
            accessor->second = std::move(valuePtr);
        }
        return accessor->second.get();
    }
private:
    // Tokens hash to their pointer values but tbb::concurrent_hash_map is
    // more efficient when there is more randomness in the lower order bits of
    // the hash. Thus the shifted hash function.
    struct _TbbHashFunc {
        inline bool equal(const TfToken &l, const TfToken &r) const {
            return l == r;
        }
        inline size_t hash(const TfToken &t) const {
            return t.Hash();
        }
    };

    using _HashMap = tbb::concurrent_hash_map<
        TfToken, std::unique_ptr<Usd_PrimTypeInfo>, _TbbHashFunc>;
    _HashMap _hashMap;
};
}; // end anonymous namespace

// Class to be used as a static private singleton to cache all distinct prim 
// types used by the prim data.
class Usd_PrimTypeInfoCache {
public:
    Usd_PrimTypeInfoCache() = default;
    Usd_PrimTypeInfoCache(const Usd_PrimTypeInfoCache&) = delete;

    // Finds the cached prim type info for the given prim type and list of 
    // applied schemas, creating and caching a new one if it doesn't exist.
    const Usd_PrimTypeInfo *FindOrCreatePrimTypeInfo(
        const TfToken &primType, TfTokenVector &&appliedSchemas)
    {
        TRACE_FUNCTION();

        TfToken key = _CreatePrimTypeInfoKey(primType, appliedSchemas);
        if (key.IsEmpty()) {
            return GetEmptyPrimTypeInfo();
        }

        // Find try to find the prim type in the type info map
        if (auto primTypeInfo = _primTypeInfoMap.Find(key)) {
            return primTypeInfo;
        }

        // If it's not, create the new type info first and then try to insert 
        // it. We always return the value found in the cache which may not be
        // the type info we created if another thread happened to create the
        // same type info and managed to insert it first. In that case ours just
        // gets deleted since the hash map didn't take ownership.
        std::unique_ptr<Usd_PrimTypeInfo> newPrimTypeInfo(
            new Usd_PrimTypeInfo(primType, std::move(appliedSchemas)));
        return _primTypeInfoMap.Insert(key, std::move(newPrimTypeInfo));
    }

    // Return the single empty prim type info
    const Usd_PrimTypeInfo *GetEmptyPrimTypeInfo() const 
    {
        return &_emptyPrimTypeInfo;
    }

private:
    // Creates the unique prim type token key for the given prim type and 
    // ordered list of applied API schemas.
    static TfToken _CreatePrimTypeInfoKey(
        const TfToken &primType,
        const TfTokenVector &appliedSchemaTypes)
    {
        TRACE_FUNCTION();

        // In the common case where there are no applied schemas, we just use
        // the prim type token itself.
        if (appliedSchemaTypes.empty()) {
            return primType;
        }

        // We generate a full type string that is a comma separated list of 
        // the prim type and then each the applied schema type in order. Note
        // that it's completely valid for there to be applied schemas when the 
        // prim type is empty; they key just starts with an empty prim type.
        size_t tokenSize = appliedSchemaTypes.size() + primType.size();
        for (const TfToken &schemaType : appliedSchemaTypes) {
            tokenSize += schemaType.size();
        }
        std::string fullTypeString;
        fullTypeString.reserve(tokenSize);
        fullTypeString += primType;
        for (const TfToken &schemaType : appliedSchemaTypes) {
            fullTypeString += ",";
            fullTypeString += schemaType;
        }
        return TfToken(fullTypeString);
    }

    _ThreadSafeHashMapImpl _primTypeInfoMap;
    Usd_PrimTypeInfo _emptyPrimTypeInfo;
};

// Static prim type info cache
static TfStaticData<Usd_PrimTypeInfoCache> _primTypeInfoCache;

// Static assertion on PrimData size.  We want to be warned when its size
// changes.
static_assert(sizeof(Usd_PrimData) == 64,
              "Expected sizeof(Usd_PrimData) == 64");

Usd_PrimData::Usd_PrimData(UsdStage *stage, const SdfPath& path)
    : _stage(stage)
    , _primIndex(nullptr)
    , _path(path)
    , _primTypeInfo(_primTypeInfoCache->GetEmptyPrimTypeInfo())
    , _firstChild(nullptr)
    , _refCount(0)
{
    if (!stage)
        TF_FATAL_ERROR("Attempted to construct with null stage");

    TF_DEBUG(USD_PRIM_LIFETIMES).Msg(
        "Usd_PrimData::ctor<%s,%s,%s>\n",
        GetTypeName().GetText(), path.GetText(),
        _stage->GetRootLayer()->GetIdentifier().c_str());
}

Usd_PrimData::~Usd_PrimData() {
    TF_DEBUG(USD_PRIM_LIFETIMES).Msg(
        "~Usd_PrimData::dtor<%s,%s,%s>\n",
        GetTypeName().GetText(), _path.GetText(),
        _stage ? _stage->GetRootLayer()->GetIdentifier().c_str() :
        "prim is invalid/expired");
}

Usd_PrimDataConstPtr
Usd_PrimData::GetParent() const
{
    if (Usd_PrimDataPtr parentLink = GetParentLink())
        return parentLink;

    SdfPath parent = _path.GetParentPath();
    return parent == SdfPath::EmptyPath() ?
        nullptr : _stage->_GetPrimDataAtPath(parent);
}

const PcpPrimIndex &
Usd_PrimData::GetPrimIndex() const
{
    static const PcpPrimIndex dummyPrimIndex;
    return ARCH_UNLIKELY(IsMaster()) ? dummyPrimIndex : *_primIndex;
}

const PcpPrimIndex &
Usd_PrimData::GetSourcePrimIndex() const
{
    TF_AXIOM(_primIndex);
    return *_primIndex;
}

SdfSpecifier
Usd_PrimData::GetSpecifier() const
{
    return _stage->_GetSpecifier(this);
}

// Iterate over a prim's specs until we get a non-empty, non-any-type typeName.
static TfToken
_ComposeTypeName(const PcpPrimIndex *primIndex)
{
    for (Usd_Resolver res(primIndex); res.IsValid(); res.NextLayer()) {
        TfToken tok;
        if (res.GetLayer()->HasField(
                res.GetLocalPath(), SdfFieldKeys->TypeName, &tok)) {
            if (!tok.IsEmpty() && tok != SdfTokens->AnyTypeToken)
                return tok;
        }
    }
    return TfToken();
}

static void
_ComposeAuthoredAppliedSchemas(
    const PcpPrimIndex *primIndex, TfTokenVector *schemas)
{
    // Collect all list op opinions for the API schemas field from strongest to
    // weakest. Then we apply them from weakest to strongest.
    std::vector<SdfTokenListOp> listOps;

    SdfTokenListOp listOp;
    for (Usd_Resolver res(primIndex); res.IsValid(); res.NextLayer()) {
        if (res.GetLayer()->HasField(
                res.GetLocalPath(), UsdTokens->apiSchemas, &listOp)) {
            // The last listop was populated so add a new empty one.
            listOps.emplace_back();
            listOps.back().Swap(listOp);
            // An explicit list op overwrites anything weaker so we can just
            // stop here if it's explicit.
            if (listOps.back().IsExplicit()) {
                break;
            }
        }
    }

    // Apply the listops to our output in reverse order (weakest to strongest).
    std::for_each(listOps.crbegin(), listOps.crend(),
        [&schemas](const SdfTokenListOp& op) { op.ApplyOperations(schemas); });
}

void
Usd_PrimData::_ComposeAndCacheTypeAndFlags(Usd_PrimDataConstPtr parent, 
                                           bool isMasterPrim)
{
    // We do not have to clear _flags here since in the pseudo root or instance
    // master case the values never change, and in the ordinary prim case we set
    // every flag.

    // Special-case the root (the only prim which has no parent) and
    // instancing masters.
    if (ARCH_UNLIKELY(!parent || isMasterPrim)) {
        _flags[Usd_PrimActiveFlag] = true;
        _flags[Usd_PrimLoadedFlag] = true;
        _flags[Usd_PrimModelFlag] = true;
        _flags[Usd_PrimGroupFlag] = true;
        _flags[Usd_PrimDefinedFlag] = true;
        _flags[Usd_PrimMasterFlag] = isMasterPrim;
        _primTypeInfo = _primTypeInfoCache->GetEmptyPrimTypeInfo();
    } 
    else {
        // Compose and cache 'active'.
        UsdPrim self(Usd_PrimDataIPtr(this), SdfPath());

        const TfToken primTypeName = _ComposeTypeName(_primIndex);

        TfTokenVector appliedSchemas;
        _ComposeAuthoredAppliedSchemas(_primIndex, &appliedSchemas);

        _primTypeInfo = _primTypeInfoCache
            ->FindOrCreatePrimTypeInfo(primTypeName, std::move(appliedSchemas));

        bool active = true;
        self.GetMetadata(SdfFieldKeys->Active, &active);
        _flags[Usd_PrimActiveFlag] = active;

        // Cache whether or not this prim has a payload.
        bool hasPayload = _primIndex->HasAnyPayloads();
        _flags[Usd_PrimHasPayloadFlag] = hasPayload;

        // An active prim is loaded if it's loadable and in the load set, or
        // it's not loadable and its parent is loaded.
        _flags[Usd_PrimLoadedFlag] = active &&
            (hasPayload ?
             _stage->_GetPcpCache()->IsPayloadIncluded(_primIndex->GetPath()) :
             parent->IsLoaded());

        // According to Model hierarchy rules, only Model Groups may have Model
        // children (groups or otherwise).  So if our parent is not a Model
        // Group, then this prim cannot be a model (or a model group).
        // Otherwise we look up the kind metadata and consult the kind registry.
        bool isGroup = false, isModel = false;
        if (parent->IsGroup()) {
            static TfToken kindToken("kind");
            TfToken kind;
            self.GetMetadata(kindToken, &kind);
            // Use the kind registry to determine model/groupness.
            if (!kind.IsEmpty()) {
                isGroup = KindRegistry::IsA(kind, KindTokens->group);
                isModel = isGroup || KindRegistry::IsA(kind, KindTokens->model);
            }
        }
        _flags[Usd_PrimGroupFlag] = isGroup;
        _flags[Usd_PrimModelFlag] = isModel;

        // Get specifier.
        SdfSpecifier specifier = GetSpecifier();

        // This prim is abstract if its parent is or if it's a class.
        _flags[Usd_PrimAbstractFlag] =
            parent->IsAbstract() || specifier == SdfSpecifierClass;

        // Cache whether or not this prim has an authored defining specifier.
        const bool isDefiningSpec = SdfIsDefiningSpecifier(specifier);
        _flags[Usd_PrimHasDefiningSpecifierFlag] = isDefiningSpec;

        // This prim is defined if its parent is and its specifier is defining.
        _flags[Usd_PrimDefinedFlag] = isDefiningSpec && parent->IsDefined();

        // The presence of clips that may affect attributes on this prim
        // is computed and set in UsdStage. Default to false.
        _flags[Usd_PrimClipsFlag] = false;

        // These flags indicate whether this prim is an instance or an
        // instance master.
        _flags[Usd_PrimInstanceFlag] = active && _primIndex->IsInstanceable();
        _flags[Usd_PrimMasterFlag] = parent->IsInMaster();
    }
}

Usd_PrimDataConstPtr 
Usd_PrimData::GetPrimDataAtPathOrInMaster(const SdfPath &path) const
{
    return _stage->_GetPrimDataAtPathOrInMaster(path);
}

Usd_PrimDataConstPtr 
Usd_PrimData::GetMaster() const
{
    return _stage->_GetMasterForInstance(this);
}

bool
Usd_PrimData::_ComposePrimChildNames(TfTokenVector* nameOrder)
{
    // TODO: would be nice to not compute the name order until it is needed
    // TODO: What do we do with prohibitedNames?
    PcpTokenSet prohibitedNames;
    GetSourcePrimIndex().ComputePrimChildNames(nameOrder, &prohibitedNames);
    return true;
}

std::string
Usd_DescribePrimData(const Usd_PrimData *p, SdfPath const &proxyPrimPath)
{
    if (!p)
        return "null prim";

    bool isInstance = p->IsInstance();
    bool isInstanceProxy = Usd_IsInstanceProxy(p, proxyPrimPath);
    bool isInMaster = isInstanceProxy ?
        Usd_InstanceCache::IsPathInMaster(proxyPrimPath) : p->IsInMaster();
    bool isMaster = p->IsMaster();
    Usd_PrimDataConstPtr masterForInstance =
        isInstance && p->_stage ? p->GetMaster() : nullptr;

    return TfStringPrintf(
        "%s%s%sprim %s<%s> %s%s%s",
        Usd_IsDead(p) ? "expired " : (p->_flags[Usd_PrimActiveFlag] ?
                                      "" : "inactive "),
        p->GetTypeName().IsEmpty() ? "" :
            TfStringPrintf("'%s' ", p->GetTypeName().GetText()).c_str(),
        // XXX: Add applied schemas to this descriptor
        isInstance ? "instance " : isInstanceProxy ? "instance proxy " : "",
        isInMaster ? "in master " : "",
        isInstanceProxy ? proxyPrimPath.GetText() : p->_path.GetText(),
        (isInstanceProxy || isInstance) ? TfStringPrintf(
            "with master <%s> ", isInstance ?
            masterForInstance->GetPath().GetText() :
            p->_path.GetText()).c_str() : "",
        (isInstanceProxy || isMaster || isInMaster) ? TfStringPrintf(
            "using prim index <%s> ",
            p->GetSourcePrimIndex().GetPath().GetText()).c_str() : "",
        p->_stage ? TfStringPrintf(
            "on %s", UsdDescribe(p->_stage).c_str()).c_str() : ""
        );
}

void
Usd_IssueFatalPrimAccessError(const Usd_PrimData *p)
{
    TF_FATAL_ERROR("Used %s", Usd_DescribePrimData(p, SdfPath()).c_str());
}


PXR_NAMESPACE_CLOSE_SCOPE

