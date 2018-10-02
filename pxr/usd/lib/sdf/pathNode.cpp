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

#include <atomic>
#include <utility>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

// Size of path nodes is important, so we want the compiler to tell us if it
// changes.
static_assert(sizeof(Sdf_PrimPathNode) == 3 * sizeof(void *), "");
static_assert(sizeof(Sdf_PrimPropertyPathNode) == 3 * sizeof(void *), "");

struct Sdf_PathNodePrivateAccess
{
    static inline tbb::atomic<unsigned int> &
    GetRefCount(const Sdf_PathNode *p) { return p->_refCount; }

    static inline bool IsInternedPrimPropPath(const Sdf_PathNode *p) {
        return p->_isInternedPrimPropNode;
    }

    template <class T>
    static inline Sdf_PathNodeConstRefPtr
    New(const Sdf_PathNodeConstRefPtr &parent) {
        return Sdf_PathNodeConstRefPtr(new T(parent));
    }

    template <class T, class Arg>
    static inline Sdf_PathNodeConstRefPtr
    New(const Sdf_PathNodeConstRefPtr &parent, const Arg &arg) {
        return Sdf_PathNodeConstRefPtr(new T(parent, arg));
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
    return Sdf_PathNode::Hash(p);
}

template <class T>
struct _HashParentAnd
{
    inline bool equal(const T &l, const T &r) const {
        return Sdf_PathNode::Equals(l.parent, r.parent) && l.value == r.value;
    }

    inline size_t hash(const T &t) const {
        size_t h = hash_value(t.parent);
        boost::hash_combine(h, t.value);
        return h;
    }
 };

template <>
struct _HashParentAnd<_ParentAnd<void> >
{
    inline bool equal(const _ParentAnd<void> &l,
                      const _ParentAnd<void> &r) const {
        return Sdf_PathNode::Equals(l.parent, r.parent);
    }

    inline size_t hash(const _ParentAnd<void> &t) const {
        return hash_value(t.parent);
    }
};

template <class T>
struct _Table {
    typedef tbb::concurrent_hash_map<
        _ParentAnd<T>, const Sdf_PathNode *,
        _HashParentAnd<_ParentAnd<T> > > Type;
};

typedef _Table<TfToken>::Type _TokenTable;
typedef _Table<Sdf_PathNode::VariantSelectionType>::Type _VarSelTable;
typedef _Table<SdfPath>::Type _PathTable;
typedef _Table<void>::Type _VoidTable;

template <class PathNode, class Table, class Arg>
inline Sdf_PathNodeConstRefPtr
_FindOrCreate(Table &table,
              const Sdf_PathNodeConstRefPtr &parent,
              const Arg &arg)
{
    typename Table::accessor accessor;
    if (table.insert(accessor, _MakeParentAnd(parent.get(), arg)) ||
        Access::GetRefCount(accessor->second).fetch_and_increment() == 0) {
        // Either there was no entry in the table, or there was but it had begun
        // dying (another client dropped its refcount to 0).  We have to create
        // a new entry in the table.  When the client that is killing the other
        // node it looks for itself in the table, it will either not find itself
        // or will find a different node and so won't remove it.
        Sdf_PathNodeConstRefPtr newNode = Access::New<PathNode>(parent, arg);
        accessor->second = newNode.get();
        return newNode;
    }
    return Sdf_PathNodeConstRefPtr(accessor->second, /* add_ref = */ false);
}

template <class PathNode, class Table>
inline Sdf_PathNodeConstRefPtr
_FindOrCreate(Table &table, const Sdf_PathNodeConstRefPtr &parent)
{
    typename Table::accessor accessor;
    if (table.insert(accessor, _MakeParentAnd(parent.get())) ||
        Access::GetRefCount(accessor->second).fetch_and_increment() == 0) {
        // Either there was no entry in the table, or there was but it had begun
        // dying (another client dropped its refcount to 0).  We have to create
        // a new entry in the table.  When the client that is killing the other
        // node it looks for itself in the table, it will either not find itself
        // or will find a different node and so won't remove it.
        Sdf_PathNodeConstRefPtr newNode = Access::New<PathNode>(parent);
        accessor->second = newNode.get();
        return newNode;
    }
    return Sdf_PathNodeConstRefPtr(accessor->second, /* add_ref = */ false);
}

template <class Table, class Arg>
inline void
_Remove(const Sdf_PathNode *pathNode,
        Table &table, const Sdf_PathNodeConstRefPtr &parent, const Arg &arg)
{
    // If there's an entry for this key that has pathNode, erase it.  Even if
    // there's an entry present it may not be pathNode, since another node may
    // have been created since we decremented our refcount and started being
    // destroyed.  If it is this node, we remove it.
    typename Table::accessor accessor;
    if (table.find(accessor, _MakeParentAnd(parent.get(), arg)) &&
        accessor->second == pathNode) {
        table.erase(accessor);
    }
}

template <class Table>
inline void
_Remove(const Sdf_PathNode *pathNode,
        Table &table, const Sdf_PathNodeConstRefPtr &parent)
{
    // If there's an entry for this key that has pathNode, erase it.  Even if
    // there's an entry present it may not be pathNode, since another node may
    // have been created since we decremented our refcount and started being
    // destroyed.  If it is this node, we remove it.
    typename Table::accessor accessor;
    if (table.find(accessor, _MakeParentAnd(parent.get())) &&
        accessor->second == pathNode) {
        table.erase(accessor);
    }
}

} // anon

// Preallocate some space in the prim and prim property tables.
TF_MAKE_STATIC_DATA(_PathTable, _mapperNodes) {}
TF_MAKE_STATIC_DATA(_PathTable, _targetNodes) {}
TF_MAKE_STATIC_DATA(_TokenTable, _mapperArgNodes) {}
TF_MAKE_STATIC_DATA(_TokenTable, _primNodes) {
    _primNodes->rehash(32768); }
TF_MAKE_STATIC_DATA(_TokenTable, _primPropertyNodes) {
    _primPropertyNodes->rehash(32768); }
TF_MAKE_STATIC_DATA(_TokenTable, _relAttrNodes) {}
TF_MAKE_STATIC_DATA(_VarSelTable, _primVarSelNodes) {}
TF_MAKE_STATIC_DATA(_VoidTable, _expressionNodes) {}

TF_MAKE_STATIC_DATA(Sdf_PathNodeConstRefPtr, _absoluteRootNode) {
    *_absoluteRootNode = Sdf_RootPathNode::New(true);
    TF_AXIOM((*_absoluteRootNode)->GetCurrentRefCount() == 1);
}
TF_MAKE_STATIC_DATA(Sdf_PathNodeConstRefPtr, _relativeRootNode) {
    *_relativeRootNode = Sdf_RootPathNode::New(false);
    TF_AXIOM((*_relativeRootNode)->GetCurrentRefCount() == 1);
}

const Sdf_PathNodeConstRefPtr &
Sdf_PathNode::GetAbsoluteRootNode() {
    return *_absoluteRootNode;
}

const Sdf_PathNodeConstRefPtr &
Sdf_PathNode::GetRelativeRootNode() {
    return *_relativeRootNode;
}

Sdf_PathNodeConstRefPtr
Sdf_PathNode::FindOrCreatePrim(Sdf_PathNodeConstRefPtr const &parent,
                               const TfToken &name)
{
    return _FindOrCreate<Sdf_PrimPathNode>(*_primNodes, parent, name);
}
    
Sdf_PathNodeConstRefPtr
Sdf_PathNode::FindOrCreatePrimProperty(Sdf_PathNodeConstRefPtr const &parent, 
                                       const TfToken &name)
{
    return _FindOrCreate<Sdf_PrimPropertyPathNode>(
        *_primPropertyNodes, parent, name);

    //return Sdf_PrimPropertyPathNode::NewFloatingNode(parent, name);
}
    
Sdf_PathNodeConstRefPtr
Sdf_PathNode::FindOrCreatePrimVariantSelection(
    Sdf_PathNodeConstRefPtr const &parent, 
    const TfToken &variantSet,
    const TfToken &variant)
{
    return _FindOrCreate<Sdf_PrimVariantSelectionNode>(
        *_primVarSelNodes, parent, VariantSelectionType(variantSet, variant));
}

Sdf_PathNodeConstRefPtr
Sdf_PathNode::FindOrCreateTarget(Sdf_PathNodeConstRefPtr const &parent, 
                                 Sdf_PathNodeConstRefPtr const &targetPathNode)
{
    // We must ensure that we have an interned parent node if it's a prim
    // property node.
    bool makeInternedParent =
        parent->GetNodeType() == PrimPropertyNode &&
        !parent->_isInternedPrimPropNode;

    return _FindOrCreate<Sdf_TargetPathNode>(
        *_targetNodes,
        makeInternedParent
        ? Sdf_PathNode::FindOrCreatePrimProperty(
            parent->GetParentNode(), parent->GetName())
        : parent,
        SdfPath(targetPathNode));
}

Sdf_PathNodeConstRefPtr
Sdf_PathNode
::FindOrCreateRelationalAttribute(Sdf_PathNodeConstRefPtr const &parent, 
                                  const TfToken &name)
{
    return _FindOrCreate<Sdf_RelationalAttributePathNode>(
        *_relAttrNodes, parent, name);
}

Sdf_PathNodeConstRefPtr
Sdf_PathNode
::FindOrCreateMapper(Sdf_PathNodeConstRefPtr const &parent, 
                     Sdf_PathNodeConstRefPtr const &targetPathNode)
{
    // We must ensure that we have an interned parent node if it's a prim
    // property node.
    bool makeInternedParent =
        parent->GetNodeType() == PrimPropertyNode &&
        !parent->_isInternedPrimPropNode;

    return _FindOrCreate<Sdf_MapperPathNode>(
        *_mapperNodes,
        makeInternedParent
        ? Sdf_PathNode::FindOrCreatePrimProperty(
            parent->GetParentNode(), parent->GetName())
        : parent,
        SdfPath(targetPathNode));
}

Sdf_PathNodeConstRefPtr
Sdf_PathNode::FindOrCreateMapperArg(Sdf_PathNodeConstRefPtr const &parent, 
                                    const TfToken &name)
{
    return _FindOrCreate<Sdf_MapperArgPathNode>(*_mapperArgNodes, parent, name);
}
    
Sdf_PathNodeConstRefPtr
Sdf_PathNode::FindOrCreateExpression(Sdf_PathNodeConstRefPtr const &parent)
{
    // We must ensure that we have an interned parent node if it's a prim
    // property node.
    bool makeInternedParent =
        parent->GetNodeType() == PrimPropertyNode &&
        !parent->_isInternedPrimPropNode;

    return _FindOrCreate<Sdf_ExpressionPathNode>(
        *_expressionNodes,
        makeInternedParent
        ? Sdf_PathNode::FindOrCreatePrimProperty(
            parent->GetParentNode(), parent->GetName())
        : parent);
}

Sdf_PathNode::Sdf_PathNode(bool isAbsolute) :
    _refCount(0),
    _elementCount(0),
    _nodeType(RootNode),
    _isAbsolute(isAbsolute),
    _containsPrimVariantSelection(false),
    _containsTargetPath(false),
    _isInternedPrimPropNode(false),
    _hasToken(false)
{
}

void Sdf_PathNode::GetPrefixes(SdfPathVector *prefixes, bool includeRoot) const
{
    size_t nElems = _elementCount + (includeRoot ? 1 : 0);
    prefixes->resize(nElems);
    Sdf_PathNodeConstRefPtr n(this);
    for (int i = static_cast<int>(nElems)-1; i >= 0; --i) {
        (*prefixes)[i] = SdfPath(n); 
        n = n->_parent;
    }
}

std::pair<Sdf_PathNodeConstRefPtr, Sdf_PathNodeConstRefPtr>
Sdf_PathNode::RemoveCommonSuffix(
    const Sdf_PathNodeConstRefPtr& a,
    const Sdf_PathNodeConstRefPtr& b,
    bool stopAtRootPrim)
{
    if (!a || !b) {
        return std::make_pair(a, b);
    }

    // We use raw pointers because this method is performance critical.
    // We don't want the extra overhead of TfRefPtr c'tor, d'tor and
    // dereference.

    // Scan upwards until we find a difference or a root node or child of
    // a root node.  Root nodes have element counts of 0 and their children
    // elements counts of 1.
    const Sdf_PathNode* aScan = boost::get_pointer(a);
    const Sdf_PathNode* bScan = boost::get_pointer(b);
    while (aScan->GetElementCount() > 1 && bScan->GetElementCount() > 1) {
        if (!aScan->Compare<_EqualElement>(*bScan)) {
            return std::make_pair(Sdf_PathNodeConstRefPtr(aScan),
                                  Sdf_PathNodeConstRefPtr(bScan));
        }
        aScan = boost::get_pointer(aScan->GetParentNode());
        bScan = boost::get_pointer(bScan->GetParentNode());
    }

    // If stopAtRootPrim is not true and neither path is a root then we
    // can scan upwards one more level.
    if (!stopAtRootPrim &&
        aScan->GetElementCount() >= 1 &&
        bScan->GetElementCount() >= 1 &&
        aScan->Compare<_EqualElement>(*bScan)) {
        return std::make_pair(aScan->GetParentNode(), bScan->GetParentNode());
    }
    else {
        return std::make_pair(Sdf_PathNodeConstRefPtr(aScan),
                              Sdf_PathNodeConstRefPtr(bScan));
    }
}

const Sdf_PathNode::VariantSelectionType &
Sdf_PathNode::_GetEmptyVariantSelection() const {
    static VariantSelectionType _emptyVariantSelection;
    return _emptyVariantSelection;
}

typedef tbb::concurrent_hash_map<Sdf_PathNode const *, TfToken> _PathTokenTable;
static TfStaticData<_PathTokenTable> _pathTokenTable;

const TfToken &
Sdf_PathNode::GetPathToken() const
{
    // Set the cache bit.  We only ever read this during the dtor, and that has
    // to be exclusive to all other execution.
    _hasToken = true;

    // Attempt to insert.
    TfAutoMallocTag2 tag("Sdf", "SdfPath");
    TfAutoMallocTag tag2("Sdf_PathNode::GetPathToken");

    _PathTokenTable::accessor accessor;
    if (_pathTokenTable->insert(accessor, std::make_pair(this, TfToken()))) {
        // We won the race, build and set the token.
        accessor->second = _CreatePathToken();
    }
    return accessor->second;
}

TfToken
Sdf_PathNode::_CreatePathToken() const
{
    TRACE_FUNCTION();

    if (this == boost::get_pointer(Sdf_PathNode::GetRelativeRootNode())) {
        return SdfPathTokens->relativeRoot;
    }

    const Sdf_PathNode * const root = (IsAbsolutePath() ? 
                boost::get_pointer(Sdf_PathNode::GetAbsoluteRootNode()) : 
                boost::get_pointer(Sdf_PathNode::GetRelativeRootNode()));
    const Sdf_PathNode * curNode = this;

    std::vector<const Sdf_PathNode *> nodes;
    nodes.reserve(GetElementCount());
    while (curNode && (curNode != root)) {
        nodes.push_back(curNode);
        curNode = boost::get_pointer(curNode->GetParentNode());
    }

    std::string str;
    if (IsAbsolutePath()) {
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

Sdf_PrimPathNode::~Sdf_PrimPathNode() {
    _Remove(this, *_primNodes, GetParentNode(), _name);
}

Sdf_PrimPropertyPathNode::~Sdf_PrimPropertyPathNode() {
    if (Access::IsInternedPrimPropPath(this)) {
        _Remove(this, *_primPropertyNodes, GetParentNode(), _name);
    }
}

const TfToken &
Sdf_PrimVariantSelectionNode::_GetNameImpl() const
{
    return _variantSelection.second.IsEmpty()
            ? _variantSelection.first
            : _variantSelection.second;
}

TfToken
Sdf_PrimVariantSelectionNode::_GetElementImpl() const
{
    return TfToken("{" + _variantSelection.first.GetString() + "=" +
                   _variantSelection.second.GetString() + "}");
}

Sdf_PrimVariantSelectionNode::~Sdf_PrimVariantSelectionNode() {
    _Remove(this, *_primVarSelNodes, GetParentNode(), _variantSelection);
}

TfToken
Sdf_TargetPathNode::_GetElementImpl() const {
    return TfToken(SdfPathTokens->relationshipTargetStart.GetString()
                   + _targetPath.GetString()
                   + SdfPathTokens->relationshipTargetEnd.GetString());
}

Sdf_TargetPathNode::~Sdf_TargetPathNode() {
    _Remove(this, *_targetNodes, GetParentNode(), _targetPath);
}

Sdf_RelationalAttributePathNode::~Sdf_RelationalAttributePathNode() {
    _Remove(this, *_relAttrNodes, GetParentNode(), _name);
}

TfToken
Sdf_MapperPathNode::_GetElementImpl() const {
    return TfToken(SdfPathTokens->propertyDelimiter.GetString() +
                   SdfPathTokens->mapperIndicator.GetString() +
                   SdfPathTokens->relationshipTargetStart.GetString() +
                   _targetPath.GetString() + 
                   SdfPathTokens->relationshipTargetEnd.GetString());
}

Sdf_MapperPathNode::~Sdf_MapperPathNode() {
    _Remove(this, *_mapperNodes, GetParentNode(), _targetPath);
}

TfToken
Sdf_MapperArgPathNode::_GetElementImpl() const {
    return TfToken(SdfPathTokens->propertyDelimiter.GetString() +
                   _name.GetString());
}

Sdf_MapperArgPathNode::~Sdf_MapperArgPathNode() {
    _Remove(this, *_mapperArgNodes, GetParentNode(), _name);
}

TfToken
Sdf_ExpressionPathNode::_GetElementImpl() const {
    return TfToken(SdfPathTokens->propertyDelimiter.GetString() +
                   SdfPathTokens->expressionIndicator.GetString());
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
    TF_FOR_ALL(i, table) {
        if (i->first.parent == parent)
            result->push_back(i->second);
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

    _Visit( Sdf_PathNode::GetAbsoluteRootNode().operator->(), &stats );
    _Visit( Sdf_PathNode::GetRelativeRootNode().operator->(), &stats );

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
