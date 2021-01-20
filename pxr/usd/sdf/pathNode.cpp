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
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/tokens.h"
#include "pxr/usd/sdf/instantiatePool.h"
#include "pxr/base/tf/bitUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/trace/trace.h"

#include <boost/functional/hash.hpp>
#include <tbb/concurrent_hash_map.h>
#include <tbb/spin_mutex.h>

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

SDF_INSTANTIATE_POOL(Sdf_PathPrimTag, Sdf_SizeofPrimPathNode, /*regionBits=*/8);
SDF_INSTANTIATE_POOL(Sdf_PathPropTag, Sdf_SizeofPropPathNode, /*regionBits=*/8);

// Size of path nodes is important, so we want the compiler to tell us if it
// changes.
static_assert(sizeof(Sdf_PrimPathNode) == 3 * sizeof(void *), "");
static_assert(sizeof(Sdf_PrimPropertyPathNode) == 3 * sizeof(void *), "");

struct Sdf_PathNodePrivateAccess
{
    template <class Handle>
    static inline tbb::atomic<unsigned int> &
    GetRefCount(Handle h) {
        Sdf_PathNode const *p =
            reinterpret_cast<Sdf_PathNode const *>(h.GetPtr());
        return p->_refCount;
    }

    template <class T, class Pool, class ... Args>
    static inline typename Pool::Handle
    New(Sdf_PathNode const *parent, Args const & ... args) {
        typename Pool::Handle h = Pool::Allocate();
        char *p = h.GetPtr();
        T *tp = reinterpret_cast<T *>(p);
        new (tp) T(parent, args...);
        return h;
    }
};

typedef Sdf_PathNodePrivateAccess Access;

namespace {

template <class T>
struct _ParentAnd { const Sdf_PathNode *parent; T value; };

// Allow void for 'expression' path case, which has no additional data.
template <> struct _ParentAnd<void> { const Sdf_PathNode *parent; };

template <class T>
inline _ParentAnd<T>
_MakeParentAnd(const Sdf_PathNode *parent, const T &value) {
    _ParentAnd<T> ret;
    ret.parent = parent;
    ret.value = value;
    return ret;
}

inline _ParentAnd<void>
_MakeParentAnd(const Sdf_PathNode *parent) {
    _ParentAnd<void> ret;
    ret.parent = parent;
    return ret;
}

inline size_t
hash_value(const Sdf_PathNode *p)
{
    return TfHash()(p);
}

template <class T>
struct _HashParentAnd
{
    inline bool equal(const T &l, const T &r) const {
        return l.parent == r.parent && l.value == r.value;
    }

    inline size_t hash(const T &t) const {
        size_t h = reinterpret_cast<uintptr_t>(t.parent) >> 4;
        boost::hash_combine(h, t.value);
        return h;
    }
 };

template <>
struct _HashParentAnd<_ParentAnd<void> >
{
    inline bool equal(const _ParentAnd<void> &l,
                      const _ParentAnd<void> &r) const {
        return l.parent == r.parent;
    }

    inline size_t hash(const _ParentAnd<void> &t) const {
        return reinterpret_cast<uintptr_t>(t.parent) >> 4;
    }
};

template <class T>
struct _PrimTable {
    using Pool = Sdf_PathPrimPartPool;
    using PoolHandle = Sdf_PathPrimHandle;
    using NodeHandle = Sdf_PathPrimNodeHandle;
    using Type = tbb::concurrent_hash_map<
        _ParentAnd<T>, PoolHandle, _HashParentAnd<_ParentAnd<T> > >;
    
    Type map;
};

template <class T>
struct _PropTable {
    using Pool = Sdf_PathPropPartPool;
    using PoolHandle = Sdf_PathPropHandle;
    using NodeHandle = Sdf_PathPropNodeHandle;
    using Type = tbb::concurrent_hash_map<
        _ParentAnd<T>, PoolHandle, _HashParentAnd<_ParentAnd<T> > >;

    Type map;
};

using _PrimTokenTable = _PrimTable<TfToken>;
using _PropTokenTable = _PropTable<TfToken>;
using _PrimVarSelTable = _PrimTable<Sdf_PathNode::VariantSelectionType>;
using _PropTargetTable = _PropTable<SdfPath>;
using _PropVoidTable = _PropTable<void>;

template <class PathNode, class Table, class ... Args>
inline typename Table::NodeHandle
_FindOrCreate(Table &table,
              const Sdf_PathNode *parent,
              const Args & ... args)
{
    typename Table::Type::accessor accessor;
    if (table.map.insert(accessor, _MakeParentAnd(parent, args...)) ||
        Access::GetRefCount(accessor->second).fetch_and_increment() == 0) {
        // Either there was no entry in the table, or there was but it had begun
        // dying (another client dropped its refcount to 0).  We have to create
        // a new entry in the table.  When the client that is killing the other
        // node it looks for itself in the table, it will either not find itself
        // or will find a different node and so won't remove it.
        typename Table::PoolHandle newNode =
            Access::New<PathNode, typename Table::Pool>(parent, args...);
        accessor->second = newNode;
    }
    return typename Table::NodeHandle(accessor->second, /* add_ref = */ false);
}

template <class Table, class ... Args>
inline void
_Remove(const Sdf_PathNode *pathNode,
        Table &table, const Sdf_PathNodeConstRefPtr &parent,
        const Args & ... args)
{
    // If there's an entry for this key that has pathNode, erase it.  Even if
    // there's an entry present it may not be pathNode, since another node may
    // have been created since we decremented our refcount and started being
    // destroyed.  If it is this node, we remove it.
    typename Table::Type::accessor accessor;
    if (table.map.find(accessor, _MakeParentAnd(parent.get(), args...)) &&
        accessor->second.GetPtr() == reinterpret_cast<char const *>(pathNode)) {
        table.map.erase(accessor);
    }
}

} // anon


void
Sdf_PrimPartPathNode::operator delete(void *p)
{
    using PoolHandle = Sdf_PathPrimPartPool::Handle;
    Sdf_PathPrimPartPool::Free(
        PoolHandle::GetHandle(reinterpret_cast<char *>(p)));
}

void
Sdf_PropPartPathNode::operator delete(void *p)
{
    using PoolHandle = Sdf_PathPropPartPool::Handle;
    Sdf_PathPropPartPool::Free(
        PoolHandle::GetHandle(reinterpret_cast<char *>(p)));
}

// Preallocate some space in the prim and prim property tables.
TF_MAKE_STATIC_DATA(_PropTargetTable, _mapperNodes) {}
TF_MAKE_STATIC_DATA(_PropTargetTable, _targetNodes) {}
TF_MAKE_STATIC_DATA(_PropTokenTable, _mapperArgNodes) {}
TF_MAKE_STATIC_DATA(_PrimTokenTable, _primNodes) {
    _primNodes->map.rehash(32768); }
TF_MAKE_STATIC_DATA(_PropTokenTable, _primPropertyNodes) {
    _primPropertyNodes->map.rehash(32768); }
TF_MAKE_STATIC_DATA(_PropTokenTable, _relAttrNodes) {}
TF_MAKE_STATIC_DATA(_PrimVarSelTable, _primVarSelNodes) {}
TF_MAKE_STATIC_DATA(_PropVoidTable, _expressionNodes) {}

TF_MAKE_STATIC_DATA(Sdf_PathNode const *, _absoluteRootNode) {
    *_absoluteRootNode = Sdf_RootPathNode::New(true);
    TF_AXIOM((*_absoluteRootNode)->GetCurrentRefCount() == 1);
}
TF_MAKE_STATIC_DATA(Sdf_PathNode const *, _relativeRootNode) {
    *_relativeRootNode = Sdf_RootPathNode::New(false);
    TF_AXIOM((*_relativeRootNode)->GetCurrentRefCount() == 1);
}

Sdf_PathNode const *
Sdf_RootPathNode::New(bool isAbsolute)
{
    Sdf_PathPrimPartPool::Handle h = Sdf_PathPrimPartPool::Allocate();
    char *p = h.GetPtr();
    Sdf_RootPathNode *tp = reinterpret_cast<Sdf_RootPathNode *>(p);
    new (tp) Sdf_RootPathNode(isAbsolute);
    return tp;
}

Sdf_PathNode const *
Sdf_PathNode::GetAbsoluteRootNode() {
    return *_absoluteRootNode;
}

Sdf_PathNode const *
Sdf_PathNode::GetRelativeRootNode() {
    return *_relativeRootNode;
}

Sdf_PathPrimNodeHandle
Sdf_PathNode::FindOrCreatePrim(Sdf_PathNode const *parent,
                               const TfToken &name)
{
    return _FindOrCreate<Sdf_PrimPathNode>(*_primNodes, parent, name);
}
    
Sdf_PathPropNodeHandle
Sdf_PathNode::FindOrCreatePrimProperty(Sdf_PathNode const *parent, 
                                       const TfToken &name)
{
    // NOTE!  We explicitly set the parent to null here in order to create a
    // separate prefix tree for property-like paths.

    return _FindOrCreate<Sdf_PrimPropertyPathNode>(
        *_primPropertyNodes, nullptr, name);
}
    
Sdf_PathPrimNodeHandle
Sdf_PathNode::FindOrCreatePrimVariantSelection(
    Sdf_PathNode const *parent, 
    const TfToken &variantSet,
    const TfToken &variant)
{
    return _FindOrCreate<Sdf_PrimVariantSelectionNode>(
        *_primVarSelNodes, parent, VariantSelectionType(variantSet, variant));
}

Sdf_PathPropNodeHandle
Sdf_PathNode::FindOrCreateTarget(Sdf_PathNode const *parent, 
                                 SdfPath const &targetPath)
{
    return _FindOrCreate<Sdf_TargetPathNode>(*_targetNodes, parent, targetPath);
}

Sdf_PathPropNodeHandle
Sdf_PathNode
::FindOrCreateRelationalAttribute(Sdf_PathNode const *parent, 
                                  const TfToken &name)
{
    return _FindOrCreate<Sdf_RelationalAttributePathNode>(
        *_relAttrNodes, parent, name);
}

Sdf_PathPropNodeHandle
Sdf_PathNode
::FindOrCreateMapper(Sdf_PathNode const *parent, 
                     SdfPath const &targetPath)
{
    return _FindOrCreate<Sdf_MapperPathNode>(*_mapperNodes, parent, targetPath);
}

Sdf_PathPropNodeHandle
Sdf_PathNode::FindOrCreateMapperArg(Sdf_PathNode const *parent, 
                                    const TfToken &name)
{
    return _FindOrCreate<Sdf_MapperArgPathNode>(*_mapperArgNodes, parent, name);
}
    
Sdf_PathPropNodeHandle
Sdf_PathNode::FindOrCreateExpression(Sdf_PathNode const *parent)
{
    return _FindOrCreate<Sdf_ExpressionPathNode>(*_expressionNodes, parent);
}

Sdf_PathNode::Sdf_PathNode(bool isAbsolute) :
    _refCount(1),
    _elementCount(0),
    _nodeType(RootNode),
    _isAbsolute(isAbsolute),
    _containsPrimVariantSelection(false),
    _containsTargetPath(false),
    _hasToken(false)
{
}

const Sdf_PathNode::VariantSelectionType &
Sdf_PathNode::_GetEmptyVariantSelection() const {
    static VariantSelectionType _emptyVariantSelection;
    return _emptyVariantSelection;
}

namespace {
// This "table" is a thread-safe mapping from property node to path token.  Each
// entry in the _pathTokenTable points to one of these, and will have an entry
// for the path string for the prim path itself (which will be keyed with a
// nullptr property node pointer) plus all the properties that hang off it.
struct _PropToTokenTable
{
    // Note that this function returns a TfToken lvalue reference that needs to
    // live/be valid as long as this object is around.
    template <class Fn>
    TfToken const &
    FindOrCreate(Sdf_PathNode const *prop, Fn &&makeToken) {
        _Data &d = *_data;
        // We try first without creating the token -- if that fails we try
        // again.  This could be made more efficient, but getting strings for
        // paths shouldn't be a bottleneck for clients.
        tbb::spin_mutex::scoped_lock lock(d.mutex);
        auto iter = d.propsToToks.find(prop);
        if (iter == d.propsToToks.end()) {
            // No entry yet.  Drop the lock, make the token, and try to insert
            // it.  We *must* drop the lock since creating the token can
            // re-enter here (e.g. if there are embedded target paths that have
            // properties on the same prim).
            lock.release();
            TfToken tok = std::forward<Fn>(makeToken)();
            lock.acquire(d.mutex);
            // This may or may not actually insert the token, depending on
            // whether or not a concurrent caller did, but it doesn't really
            // matter.
            iter = d.propsToToks.emplace(prop, std::move(tok)).first;
        }
        return iter->second;
    }

private:
    struct _Data {
        std::map<Sdf_PathNode const *, TfToken> propsToToks;
        tbb::spin_mutex mutex;
    };

    std::shared_ptr<_Data> _data { new _Data };
};

} // anon

using _PrimToPropTokenTables =
    tbb::concurrent_hash_map<Sdf_PathNode const *, _PropToTokenTable>;

static TfStaticData<_PrimToPropTokenTables> _pathTokenTable;

const TfToken &
Sdf_PathNode::GetPathToken(Sdf_PathNode const *primPart,
                           Sdf_PathNode const *propPart)
{
    // Set the cache bit.  We only ever read this during the dtor, and that has
    // to be exclusive to all other execution.
    primPart->_hasToken = true;

    // Attempt to insert.
    TfAutoMallocTag2 tag("Sdf", "SdfPath");
    TfAutoMallocTag tag2("Sdf_PathNode::GetPathToken");

    _PrimToPropTokenTables::accessor primAccessor;
    _pathTokenTable->insert(
        primAccessor, std::make_pair(primPart, _PropToTokenTable()));
    auto &propToTokenTable = primAccessor->second;
    // Release the primAccessor here, since the call to _CreatePathToken below
    // can cause reentry here (for embedded target paths).
    primAccessor.release();

    return propToTokenTable.FindOrCreate(
        propPart, [primPart, propPart]() {
            return Sdf_PathNode::_CreatePathToken(primPart, propPart);
        });
}

TfToken
Sdf_PathNode::GetPathAsToken(Sdf_PathNode const *primPart,
                             Sdf_PathNode const *propPart)
{
    return _CreatePathToken(primPart, propPart);
}

TfToken
Sdf_PathNode::_CreatePathToken(Sdf_PathNode const *primPart,
                               Sdf_PathNode const *propPart)
{
    TRACE_FUNCTION();

    if (primPart == GetRelativeRootNode() && !propPart) {
        return SdfPathTokens->relativeRoot;
    }

    Sdf_PathNode const * const root = (primPart->IsAbsolutePath() ?
                                       Sdf_PathNode::GetAbsoluteRootNode() : 
                                       Sdf_PathNode::GetRelativeRootNode());
    
    std::vector<const Sdf_PathNode *> nodes;
    nodes.reserve(primPart->GetElementCount() +
                  (propPart ? propPart->GetElementCount() : 0));
    
    Sdf_PathNode const *curNode = propPart;
    while (curNode) {
        nodes.push_back(curNode);
        curNode = curNode->GetParentNode();
    }
    curNode = primPart;
    while (curNode && (curNode != root)) {
        nodes.push_back(curNode);
        curNode = curNode->GetParentNode();
    }

    std::string str;
    if (primPart->IsAbsolutePath()) {
        // Put the leading / on absolute
        str.append(SdfPathTokens->absoluteIndicator.GetString());
    }

    TfToken prevElem;
    Sdf_PathNode::NodeType prevNodeType = Sdf_PathNode::NumNodeTypes;
    TF_REVERSE_FOR_ALL(i, nodes) {
        const Sdf_PathNode * const node = *i;
        Sdf_PathNode::NodeType curNodeType = node->GetNodeType();
        if (prevNodeType == Sdf_PathNode::PrimNode && 
            (curNodeType == Sdf_PathNode::PrimNode ||
             // This covers cases like '../.property'
             prevElem == SdfPathTokens->parentPathElement)) {
            str.append(SdfPathTokens->childDelimiter.GetString());
        }
        TfToken elem = node->GetElement();
        str.append(elem.GetString());
        prevElem.Swap(elem);
        prevNodeType = curNodeType;
    }

    return TfToken(str);
}

void
Sdf_PathNode::_RemovePathTokenFromTable() const
{
    _pathTokenTable->erase(this);
}

// Returns true if \p identifier has at least one namespace delimiter.
static inline bool
_HasNamespaceDelimiter(const std::string& identifier)
{
    return identifier.find(SdfPathTokens->namespaceDelimiter.GetString()[0]) !=
                                                        std::string::npos;
}

bool
Sdf_PathNode::_IsNamespacedImpl() const
{
    return _HasNamespaceDelimiter(GetName().GetString());
}

void
Sdf_PathNode::AppendText(std::string *str) const
{
    switch (_nodeType) {
    case RootNode:
        return;
    case PrimNode:
        str->append(_Downcast<Sdf_PrimPathNode>()->_name.GetString());
        return;
    case PrimPropertyNode:
        str->append(SdfPathTokens->propertyDelimiter.GetString());
        str->append(_Downcast<Sdf_PrimPropertyPathNode>()->_name.GetString());
        return;
    case PrimVariantSelectionNode:
        _Downcast<Sdf_PrimVariantSelectionNode>()->_AppendText(str);
        return;
    case TargetNode:
        _Downcast<Sdf_TargetPathNode>()->_AppendText(str);
        return;
    case RelationalAttributeNode:
        str->append(SdfPathTokens->propertyDelimiter.GetString());
        str->append(_Downcast<Sdf_RelationalAttributePathNode>()->
                    _name.GetString());
        return;
    case MapperNode:
        _Downcast<Sdf_MapperPathNode>()->_AppendText(str);
        return;
    case MapperArgNode:
        _Downcast<Sdf_MapperArgPathNode>()->_AppendText(str);
        return;
    case ExpressionNode:
        _Downcast<Sdf_ExpressionPathNode>()->_AppendText(str);
        return;
    default:
        return;
    };
}

Sdf_PrimPathNode::~Sdf_PrimPathNode() {
    _Remove(this, *_primNodes, GetParentNode(), _name);
}

Sdf_PrimPropertyPathNode::~Sdf_PrimPropertyPathNode() {
    _Remove(this, *_primPropertyNodes, GetParentNode(), _name);
}

const TfToken &
Sdf_PrimVariantSelectionNode::_GetNameImpl() const
{
    return _variantSelection->second.IsEmpty()
            ? _variantSelection->first
            : _variantSelection->second;
}

void
Sdf_PrimVariantSelectionNode::_AppendText(std::string *str) const
{
    std::string const &vset = _variantSelection->first.GetString();
    std::string const &vsel = _variantSelection->second.GetString();
    str->reserve(str->size() + vset.size() + vsel.size() + 3);
    str->push_back('{');
    str->append(vset);
    str->push_back('=');
    str->append(vsel);
    str->push_back('}');
}

Sdf_PrimVariantSelectionNode::~Sdf_PrimVariantSelectionNode() {
    _Remove(this, *_primVarSelNodes, GetParentNode(), *_variantSelection);
}

void
Sdf_TargetPathNode::_AppendText(std::string *str) const {
    std::string const &open =
        SdfPathTokens->relationshipTargetStart.GetString();
    std::string const &target = _targetPath.GetString();
    std::string const &close =
        SdfPathTokens->relationshipTargetEnd.GetString();
    str->reserve(str->size() + open.size() + target.size() + close.size());
    str->append(open);
    str->append(target);
    str->append(close);
}

Sdf_TargetPathNode::~Sdf_TargetPathNode() {
    _Remove(this, *_targetNodes, GetParentNode(), _targetPath);
}

Sdf_RelationalAttributePathNode::~Sdf_RelationalAttributePathNode() {
    _Remove(this, *_relAttrNodes, GetParentNode(), _name);
}

void
Sdf_MapperPathNode::_AppendText(std::string *str) const {
    std::string const &delim = SdfPathTokens->propertyDelimiter.GetString();
    std::string const &mapperIndicator =
        SdfPathTokens->mapperIndicator.GetString();
    std::string const &open =
        SdfPathTokens->relationshipTargetStart.GetString();
    std::string const &target = _targetPath.GetString();
    std::string const &close = SdfPathTokens->relationshipTargetEnd.GetString();
    str->reserve(str->size() + delim.size() + mapperIndicator.size() +
                 open.size() + target.size() + close.size());
    str->append(delim);
    str->append(mapperIndicator);
    str->append(open);
    str->append(target);
    str->append(close);
}

Sdf_MapperPathNode::~Sdf_MapperPathNode() {
    _Remove(this, *_mapperNodes, GetParentNode(), _targetPath);
}

void
Sdf_MapperArgPathNode::_AppendText(std::string *str) const {
    std::string const &delim = SdfPathTokens->propertyDelimiter.GetString();
    std::string const &name = _name.GetString();
    str->reserve(str->size() + delim.size() + name.size());
    str->append(delim);
    str->append(name);
}

Sdf_MapperArgPathNode::~Sdf_MapperArgPathNode() {
    _Remove(this, *_mapperArgNodes, GetParentNode(), _name);
}

void
Sdf_ExpressionPathNode::_AppendText(std::string *str) const {
    std::string const &delim = SdfPathTokens->propertyDelimiter.GetString();
    std::string const &expr = SdfPathTokens->expressionIndicator.GetString();
    str->reserve(str->size() + delim.size() + expr.size());
    str->append(delim);
    str->append(expr);
}

Sdf_ExpressionPathNode::~Sdf_ExpressionPathNode() {
    _Remove(this, *_expressionNodes, GetParentNode());
}

struct Sdf_Stats {
    // Counts
    int numNodes;
    int numNodeRefs;

    // Histograms
    vector<int> lengthTable;
    vector<int> numChildrenTable;
    size_t typeTable[Sdf_PathNode::NumNodeTypes];
};

template <class Table>
static void
_GatherChildrenFrom(Sdf_PathNode const *parent,
                    Table const &table,
                    vector<Sdf_PathNodeConstRefPtr> *result)
{
    TF_FOR_ALL(i, table.map) {
        if (i->first.parent == parent)
            result->emplace_back(
                reinterpret_cast<Sdf_PathNode const *>(i->second.GetPtr()));
    }
}

static vector<Sdf_PathNodeConstRefPtr>
_GetChildren(Sdf_PathNode const *pathNode)
{
    // XXX: SLOW.  For path stats debugging only.
    vector<Sdf_PathNodeConstRefPtr> children;
    _GatherChildrenFrom(pathNode, *_mapperNodes, &children);
    _GatherChildrenFrom(pathNode, *_targetNodes, &children);
    _GatherChildrenFrom(pathNode, *_mapperArgNodes, &children);
    _GatherChildrenFrom(pathNode, *_primNodes, &children);
    _GatherChildrenFrom(pathNode, *_primPropertyNodes, &children);
    _GatherChildrenFrom(pathNode, *_relAttrNodes, &children);
    _GatherChildrenFrom(pathNode, *_primVarSelNodes, &children);
    _GatherChildrenFrom(pathNode, *_expressionNodes, &children);
    return children;
}

static void _Visit(Sdf_PathNode const *path, Sdf_Stats *stats)
{
    stats->numNodes++;
    stats->numNodeRefs += path->GetCurrentRefCount();
    stats->typeTable[path->GetNodeType()] += 1;

    // Accumulate length histogram
    const size_t len = path->GetElementCount()+1; // add 1 for abs/rel root
    while (stats->lengthTable.size() <= len)
        stats->lengthTable.push_back(0);
    stats->lengthTable[len]++;

    const std::vector<Sdf_PathNodeConstRefPtr> children = _GetChildren(path);

    // Accumulate children count histogram
    const size_t numChildren = children.size();
    while (stats->numChildrenTable.size() <= numChildren)
        stats->numChildrenTable.push_back(0);
    stats->numChildrenTable[numChildren]++;

    TF_FOR_ALL(child, children) {
        _Visit(child->get(), stats);
    }
}

void Sdf_DumpPathStats()
{
    Sdf_Stats stats;
    stats.numNodes = 0;
    stats.numNodeRefs = 0;
    memset(stats.typeTable, 0, sizeof(stats.typeTable));

    _Visit( Sdf_PathNode::GetAbsoluteRootNode(), &stats );
    _Visit( Sdf_PathNode::GetRelativeRootNode(), &stats );

    printf("Sdf_PathNode stats:\n");
    printf("\tnum node refs: %i\n", stats.numNodeRefs);
    printf("\tnum nodes:     %i\n", stats.numNodes);
    printf("\tsizeof(SdfPath), aka node ref:  %zu\n", sizeof(SdfPath));
    printf("\tsizeof(Sdf_PathNode), aka node: %zu\n", sizeof(Sdf_PathNode));

    // XXX: Use TfEnum.
    char const *enumNameMap[Sdf_PathNode::NumNodeTypes];
    enumNameMap[Sdf_PathNode::RootNode] = "RootNode";
    enumNameMap[Sdf_PathNode::PrimNode] = "PrimNode";
    enumNameMap[Sdf_PathNode::PrimPropertyNode] = "PrimPropertyNode";
    enumNameMap[Sdf_PathNode::PrimVariantSelectionNode] =
        "PrimVariantSelectionNode";
    enumNameMap[Sdf_PathNode::TargetNode] = "TargetNode";
    enumNameMap[Sdf_PathNode::RelationalAttributeNode] =
        "RelationalAttributeNode";
    enumNameMap[Sdf_PathNode::MapperNode] = "MapperNode";
    enumNameMap[Sdf_PathNode::MapperArgNode] = "MapperArgNode";
    enumNameMap[Sdf_PathNode::ExpressionNode] = "ExpressionNode";

    printf("------------------------------------------------");
    printf("-- By Type\n");
    for (size_t i = 0; i != Sdf_PathNode::NumNodeTypes; ++i) {
        printf("\t%32ss: %8zu -- %6.2f%%\n",
               enumNameMap[i], stats.typeTable[i],
               100.0 * double(stats.typeTable[i]) / double(stats.numNodes));
    }

    printf("------------------------------------------------");
    printf("-- By Length\n");
    size_t totalLen = 0;
    for (size_t i=0; i < stats.lengthTable.size();++i) {
        printf("\tnum nodes with %3zu components : %i\n",
               i, stats.lengthTable[i] );
        totalLen += i * stats.lengthTable[i];
    }
    printf("\tavg num components: %g\n",
           totalLen / float(stats.numNodes));

    printf("------------------------------------------------");
    printf("-- By Number of Children\n");
    for (size_t i=0; i < stats.numChildrenTable.size(); ++i) {
        printf("\tnum nodes with %3zu children : %i\n",
               i, stats.numChildrenTable[i] );
    }

    size_t numChildren = 0;
    for (size_t i=1; i < stats.numChildrenTable.size(); ++i) {
        numChildren += i*stats.numChildrenTable[i];
    }
    printf("\tavg num children (for nodes with any children): %g\n",
           numChildren / float(stats.numNodes - stats.numChildrenTable[0]));

    printf("\n");
}

PXR_NAMESPACE_CLOSE_SCOPE
