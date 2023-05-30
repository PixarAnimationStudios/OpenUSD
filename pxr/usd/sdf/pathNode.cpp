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
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/trace/trace.h"

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
    static inline std::atomic<unsigned int> &
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

template <class T=void>
struct _ParentAnd { const Sdf_PathNode *parent; T value; };

// Allow void for 'expression' path case, which has no additional data.
template <> struct _ParentAnd<void> { const Sdf_PathNode *parent; };

template <class T=void>
struct _ParentAndRef {
    const Sdf_PathNode *parent; T const &value;
    operator _ParentAnd<T>() const {
        return { parent, value };
    }
};

// Allow void for 'expression' path case, which has no additional data.
template <> struct _ParentAndRef<void>
{
    const Sdf_PathNode *parent;
    operator _ParentAnd<void>() const {
        return { parent };
    }
};



template <class T>
inline bool operator==(_ParentAnd<T> const &x, _ParentAnd<T> const &y)
{
    return x.parent == y.parent && x.value == y.value;
}
template <class T>
inline bool operator==(_ParentAndRef<T> const &x, _ParentAndRef<T> const &y)
{
    return x.parent == y.parent && x.value == y.value;
}
template <class T>
inline bool operator==(_ParentAndRef<T> const &x, _ParentAnd<T> const &y)
{
    return x.parent == y.parent && x.value == y.value;
}
template <class T>
inline bool operator==(_ParentAnd<T> const &x, _ParentAndRef<T> const &y)
{
    return x.parent == y.parent && x.value == y.value;
}

inline bool operator==(_ParentAnd<void> const &x, _ParentAnd<void> const &y)
{
    return x.parent == y.parent;
}
inline bool operator==(_ParentAndRef<void> const &x,
                       _ParentAndRef<void> const &y)
{
    return x.parent == y.parent;
}
inline bool operator==(_ParentAndRef<void> const &x, _ParentAnd<void> const &y)
{
    return x.parent == y.parent;
}
inline bool operator==(_ParentAnd<void> const &x, _ParentAndRef<void> const &y)
{
    return x.parent == y.parent;
}

inline size_t
hash_value(const Sdf_PathNode *p)
{
    return TfHash()(p);
}

template <class PaT>
inline size_t _OuterHash(PaT const &pat)
{
    return TfHash::Combine(pat.parent, pat.value);
}

inline size_t _OuterHash(_ParentAnd<void> const &pat)
{
    return TfHash::Combine(pat.parent);
};

inline size_t _OuterHash(_ParentAndRef<void> const &pat)
{
    return TfHash::Combine(pat.parent);
};

template <class HashState, class T>
inline void TfHashAppend(HashState &h, _ParentAnd<T> const &pat)
{
    h.Append(pat.parent, pat.value);
}

template <class HashState, class T>
inline void TfHashAppend(HashState &h, _ParentAndRef<T> const &pat)
{
    h.Append(pat.parent, pat.value);
}

template <class HashState>
inline void TfHashAppend(HashState &h, _ParentAnd<void> const &pat)
{
    h.Append(pat.parent);
}

template <class HashState>
inline void TfHashAppend(HashState &h, _ParentAndRef<void> const &pat)
{
    h.Append(pat.parent);
}

static constexpr unsigned NumNodeMaps = 128;

template <class T>
struct _PrimTable {
    using Pool = Sdf_PathPrimPartPool;
    using PoolHandle = Sdf_PathPrimHandle;
    using NodeHandle = Sdf_PathPrimNodeHandle;

    struct _MapAndMutex {
        pxr_tsl::robin_map<_ParentAnd<T>, PoolHandle, TfHash> map;
        mutable tbb::spin_mutex mutex;
    };

    _MapAndMutex &GetMapAndMutexFor(_ParentAndRef<T> const &pat) {
        size_t z = _OuterHash(pat);
        return _mapsAndMutexes[z & (NumNodeMaps-1)];
    }
    
    _MapAndMutex _mapsAndMutexes[NumNodeMaps];
     
};

template <class T>
struct _PropTable {
    using Pool = Sdf_PathPropPartPool;
    using PoolHandle = Sdf_PathPropHandle;
    using NodeHandle = Sdf_PathPropNodeHandle;

    struct _MapAndMutex {
        pxr_tsl::robin_map<_ParentAnd<T>, PoolHandle, TfHash> map;
        mutable tbb::spin_mutex mutex;
    };

    _MapAndMutex &GetMapAndMutexFor(_ParentAndRef<T> const &pat) {
        size_t z = _OuterHash(pat);
        return _mapsAndMutexes[z & (NumNodeMaps-1)];
    }
    
    _MapAndMutex _mapsAndMutexes[NumNodeMaps];
};

using _PrimTokenTable = _PrimTable<TfToken>;
using _PropTokenTable = _PropTable<TfToken>;
using _PrimVarSelTable = _PrimTable<Sdf_PathNode::VariantSelectionType>;
using _PropTargetTable = _PropTable<SdfPath>;
using _PropVoidTable = _PropTable<void>;

template <class PathNode, class Table, class ... Args>
inline typename Table::NodeHandle
_FindOrCreate(Table &table,
              const TfFunctionRef<bool ()> &isValid,
              const Sdf_PathNode *parent,
              const Args & ... args)
{
    std::pair<_ParentAndRef<Args...>, typename Table::PoolHandle>
        newItem { { parent, args... }, {} };
    auto &mapAndMutex = table.GetMapAndMutexFor(newItem.first);
    tbb::spin_mutex::scoped_lock lock(mapAndMutex.mutex);

    auto iresult = mapAndMutex.map.insert(newItem);
    if (iresult.second) {
        // There was no entry in the table, check for validity, and back out the
        // insertion if it's invalid.
        if (ARCH_UNLIKELY(!isValid())) {
            mapAndMutex.map.erase(iresult.first);
            return typename Table::NodeHandle();
        }
    }
    if (iresult.second ||
        (Table::NodeHandle::IsCounted &&
         (Access::GetRefCount(
             iresult.first->second).fetch_add(1) &
          Sdf_PathNode::RefCountMask) == 0)) {
        // There was either no entry, or there was one but it had begun dying
        // (another client dropped its refcount to 0).  We have to create a new
        // entry in the table.  When the client that is deleting the other node
        // it looks for itself in the table.  It will either not find itself or
        // will find a different node and so won't remove it.
        iresult.first.value() =
            Access::New<PathNode, typename Table::Pool>(parent, args...);
    }
    return typename Table::NodeHandle(
        iresult.first->second, /* add_ref = */ false);
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
    _ParentAndRef<Args...> pat { parent.get(), args... };
    auto &mapAndMutex = table.GetMapAndMutexFor(pat);
    tbb::spin_mutex::scoped_lock lock(mapAndMutex.mutex);

    auto iter = mapAndMutex.map.find(pat);
    if (iter != mapAndMutex.map.end() &&
        iter->second.GetPtr() == reinterpret_cast<char const *>(pathNode)) {
        mapAndMutex.map.erase(iter);
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

TF_MAKE_STATIC_DATA(_PropTargetTable, _mapperNodes) {}
TF_MAKE_STATIC_DATA(_PropTargetTable, _targetNodes) {}
TF_MAKE_STATIC_DATA(_PropTokenTable, _mapperArgNodes) {}
TF_MAKE_STATIC_DATA(_PrimTokenTable, _primNodes) {}
TF_MAKE_STATIC_DATA(_PropTokenTable, _primPropertyNodes) {}
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
                               const TfToken &name,
                               TfFunctionRef<bool ()> isValid)
{
    return _FindOrCreate<Sdf_PrimPathNode>(*_primNodes, isValid, parent, name);
}
    
Sdf_PathPropNodeHandle
Sdf_PathNode::FindOrCreatePrimProperty(Sdf_PathNode const *parent, 
                                       const TfToken &name,
                                       TfFunctionRef<bool ()> isValid)
{
    // NOTE!  We explicitly set the parent to null here in order to create a
    // separate prefix tree for property-like paths.

    return _FindOrCreate<Sdf_PrimPropertyPathNode>(
        *_primPropertyNodes, isValid, nullptr, name);
}
    
Sdf_PathPrimNodeHandle
Sdf_PathNode::FindOrCreatePrimVariantSelection(
    Sdf_PathNode const *parent, 
    const TfToken &variantSet,
    const TfToken &variant,
    TfFunctionRef<bool ()> isValid)
{
    return _FindOrCreate<Sdf_PrimVariantSelectionNode>(
        *_primVarSelNodes, isValid, parent,
        VariantSelectionType(variantSet, variant));
}

Sdf_PathPropNodeHandle
Sdf_PathNode::FindOrCreateTarget(Sdf_PathNode const *parent, 
                                 SdfPath const &targetPath,
                                 TfFunctionRef<bool ()> isValid)
{
    return _FindOrCreate<Sdf_TargetPathNode>(
        *_targetNodes, isValid, parent, targetPath);
}

Sdf_PathPropNodeHandle
Sdf_PathNode
::FindOrCreateRelationalAttribute(Sdf_PathNode const *parent, 
                                  const TfToken &name,
                                  TfFunctionRef<bool ()> isValid)
{
    return _FindOrCreate<Sdf_RelationalAttributePathNode>(
        *_relAttrNodes, isValid, parent, name);
}

Sdf_PathPropNodeHandle
Sdf_PathNode
::FindOrCreateMapper(Sdf_PathNode const *parent, 
                     SdfPath const &targetPath,
                     TfFunctionRef<bool ()> isValid)
{
    return _FindOrCreate<Sdf_MapperPathNode>(
        *_mapperNodes, isValid, parent, targetPath);
}

Sdf_PathPropNodeHandle
Sdf_PathNode::FindOrCreateMapperArg(Sdf_PathNode const *parent, 
                                    const TfToken &name,
                                    TfFunctionRef<bool ()> isValid)
{
    return _FindOrCreate<Sdf_MapperArgPathNode>(
        *_mapperArgNodes, isValid, parent, name);
}
    
Sdf_PathPropNodeHandle
Sdf_PathNode::FindOrCreateExpression(Sdf_PathNode const *parent,
                                     TfFunctionRef<bool ()> isValid)
{
    return _FindOrCreate<Sdf_ExpressionPathNode>(
        *_expressionNodes, isValid, parent);
}

Sdf_PathNode::Sdf_PathNode(bool isAbsolute) :
    _refCount(1),
    _elementCount(0),
    _nodeType(RootNode),
    _nodeFlags(isAbsolute ? IsAbsoluteFlag : 0)
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
    primPart->_refCount.fetch_or(HasTokenBit, std::memory_order_relaxed);

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

namespace {

struct _StringBuffer
{
    template <class ... Ts>
    void WriteText(Ts ... args) {
        _eltStart = _str.size();
        _WriteTextImpl(args ...);
    }

    std::string
    GetString() const {
        return std::string(_str.crbegin(), _str.crend());
    }

private:
    template <class ... Ts>
    void _WriteTextImpl(char const *a0, Ts ... args) {
        _str.append(a0);
        _WriteTextImpl(args ...);
    }

    // Base case.
    void _WriteTextImpl(char const *aN) {
        _str.append(aN);
        std::reverse(_str.begin() + _eltStart, _str.end());
    }

    std::string _str;
    size_t _eltStart = 0;
};

static constexpr size_t _DebugPathBufferSize = 1024*8;
static char _debugPathBuffer[_DebugPathBufferSize];
static char *_debugPathCur = _debugPathBuffer;
static char _debugPathOverrunMsg[] =
    "<< path text exceeds debug buffer size >>";

struct _DebugBuffer
{
    template <class ... Ts>
    void WriteText(Ts ... args) {
        _eltStart = _debugPathCur;
        _WriteTextImpl(args ...);
    }

    char const *
    GetText() const {
        if (_pathTextTooLong) {
            return _debugPathOverrunMsg;
        }
        std::reverse(_debugPathBuffer, _debugPathCur);
        _debugPathCur = _debugPathBuffer;
        return _debugPathBuffer;
    }

private:
    template <class ... Ts>
    void _WriteTextImpl(char const *a0, Ts ... args) {
        if (_Write(a0)) {
            _WriteTextImpl(args ...);
        }
    }

    // Base case.
    void _WriteTextImpl(char const *aN) {
        if (_Write(aN)) {
            std::reverse(_eltStart, _debugPathCur);
        }
    }

    // Write text to buffer if it fits.
    bool _Write(char const *a) {
        size_t len = strlen(a);
        char const *end = _debugPathBuffer + _DebugPathBufferSize;
        if (_debugPathCur + len >= end) { // >= accounts for null terminator.
            _pathTextTooLong = true;
            return false;
        }
        strcpy(_debugPathCur, a);
        _debugPathCur += len; // leave this pointing at the null.
        return true;
    }
    
    char *_eltStart = nullptr;
    bool _pathTextTooLong = false;
};

} // anon

TfToken
Sdf_PathNode::_GetElementImpl() const
{
    _StringBuffer buf;
    _WriteText(buf);
    return TfToken(buf.GetString());
}
        
TfToken
Sdf_PathNode::_CreatePathToken(Sdf_PathNode const *primPart,
                               Sdf_PathNode const *propPart)
{
    _StringBuffer buf;
    _WriteTextToBuffer(primPart, propPart, buf);
    return TfToken(buf.GetString());
}

template <class Buffer>
void
Sdf_PathNode::_WriteTextToBuffer(Sdf_PathNode const *primPart,
                                 Sdf_PathNode const *propPart,
                                 Buffer &out)
{
    if (primPart == GetRelativeRootNode() && !propPart) {
        out.WriteText(SDF_PATH_RELATIVE_ROOT_STR);
        return;
    }

    // Write all the nodes in reverse order, reversing each element as we go.
    // Then at the end we reverse the entire string, to produce the correct
    // output.  We do it this way so we do not have to do any heap
    // allocation/recursion/etc because we use this function to produce string
    // representations in debuggers, and if, say, another thread is in the
    // middle of a malloc/free and holds a lock, or if we're in a signal
    // handler, we're in trouble.
    
    Sdf_PathNode const * const root = (primPart->IsAbsolutePath() ?
                                       Sdf_PathNode::GetAbsoluteRootNode() : 
                                       Sdf_PathNode::GetRelativeRootNode());

    Sdf_PathNode const *curNode = propPart;
    while (curNode) {
        curNode->_WriteText(out);
        curNode = curNode->GetParentNode();
    }
    // This covers cases like '../.property'
    if (propPart && primPart->GetNodeType() == Sdf_PathNode::PrimNode &&
        primPart->GetName() == SdfPathTokens->parentPathElement) {
        out.WriteText(SDF_PATH_CHILD_DELIMITER_STR);
    }
    curNode = primPart;
    while (curNode && (curNode != root)) {
        curNode->_WriteText(out);
        Sdf_PathNode const *parent = curNode->GetParentNode();
        if (curNode->GetNodeType() == Sdf_PathNode::PrimNode &&
            parent && parent->GetNodeType() == Sdf_PathNode::PrimNode) {
            out.WriteText(SDF_PATH_CHILD_DELIMITER_STR);
        }            
        curNode = parent;
    }

    if (primPart->IsAbsolutePath()) {
        // Put the leading / on absolute
        out.WriteText(SDF_PATH_ABSOLUTE_INDICATOR_STR);
    }
}

template <class Buffer>
void
Sdf_PathNode::_WriteTextToBuffer(SdfPath const &path, Buffer &out)
{
    _WriteTextToBuffer(path._primPart.get(),
                       path._propPart.get(), out);
}

char const *
Sdf_PathNode::GetDebugText(Sdf_PathNode const *primPart,
                           Sdf_PathNode const *propPart)
{
    _DebugBuffer buf;
    _WriteTextToBuffer(primPart, propPart, buf);
    return buf.GetText();
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

template <class Buffer>
void
Sdf_PathNode::_WriteText(Buffer &out) const
{
    switch (_nodeType) {
    case RootNode:
        return;
    case PrimNode:
        out.WriteText(_Downcast<Sdf_PrimPathNode>()->_name.GetText());
        return;
    case PrimPropertyNode:
        out.WriteText(SDF_PATH_PROPERTY_DELIMITER_STR,
                      _Downcast<Sdf_PrimPropertyPathNode>()->_name.GetText());
        return;
    case PrimVariantSelectionNode:
        _Downcast<Sdf_PrimVariantSelectionNode>()->_WriteTextImpl(out);
        return;
    case TargetNode:
        _Downcast<Sdf_TargetPathNode>()->_WriteTextImpl(out);
        return;
    case RelationalAttributeNode:
        out.WriteText(
            SDF_PATH_PROPERTY_DELIMITER_STR,
            _Downcast<Sdf_RelationalAttributePathNode>()->_name.GetText());
        return;
    case MapperNode:
        _Downcast<Sdf_MapperPathNode>()->_WriteTextImpl(out);
        return;
    case MapperArgNode:
        _Downcast<Sdf_MapperArgPathNode>()->_WriteTextImpl(out);
        return;
    case ExpressionNode:
        _Downcast<Sdf_ExpressionPathNode>()->_WriteTextImpl(out);
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

template <class Buffer>
void
Sdf_PrimVariantSelectionNode::_WriteTextImpl(Buffer &out) const
{
    char const *vset = _variantSelection->first.GetText();
    char const *vsel = _variantSelection->second.GetText();
    out.WriteText("{", vset, "=", vsel, "}");
}

Sdf_PrimVariantSelectionNode::~Sdf_PrimVariantSelectionNode() {
    _Remove(this, *_primVarSelNodes, GetParentNode(), *_variantSelection);
}

template <class Buffer>
void
Sdf_TargetPathNode::_WriteTextImpl(Buffer &out) const {
    out.WriteText(SDF_PATH_RELATIONSHIP_TARGET_END_STR);
    _WriteTextToBuffer(_targetPath, out);
    out.WriteText(SDF_PATH_RELATIONSHIP_TARGET_START_STR);
}

Sdf_TargetPathNode::~Sdf_TargetPathNode() {
    _Remove(this, *_targetNodes, GetParentNode(), _targetPath);
}

Sdf_RelationalAttributePathNode::~Sdf_RelationalAttributePathNode() {
    _Remove(this, *_relAttrNodes, GetParentNode(), _name);
}

template <class Buffer>
void
Sdf_MapperPathNode::_WriteTextImpl(Buffer &out) const {
    out.WriteText(SDF_PATH_RELATIONSHIP_TARGET_END_STR);
    _WriteTextToBuffer(_targetPath, out);
    out.WriteText(SDF_PATH_RELATIONSHIP_TARGET_START_STR);
    out.WriteText(SdfPathTokens->mapperIndicator.GetText());
    out.WriteText(SDF_PATH_PROPERTY_DELIMITER_STR);
}

Sdf_MapperPathNode::~Sdf_MapperPathNode() {
    _Remove(this, *_mapperNodes, GetParentNode(), _targetPath);
}

template <class Buffer>
void
Sdf_MapperArgPathNode::_WriteTextImpl(Buffer &out) const {
    out.WriteText(SDF_PATH_PROPERTY_DELIMITER_STR, _name.GetText());
}

Sdf_MapperArgPathNode::~Sdf_MapperArgPathNode() {
    _Remove(this, *_mapperArgNodes, GetParentNode(), _name);
}

template <class Buffer>
void
Sdf_ExpressionPathNode::_WriteTextImpl(Buffer &out) const {
    out.WriteText(SDF_PATH_PROPERTY_DELIMITER_STR,
                  SdfPathTokens->expressionIndicator.GetText());
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
    for (size_t outerIndex = 0; outerIndex != NumNodeMaps; ++outerIndex) {
        auto &mapAndMutex = table._mapsAndMutexes[outerIndex];
        tbb::spin_mutex::scoped_lock lock(mapAndMutex.mutex);
        TF_FOR_ALL(i, mapAndMutex.map) {
            if (i->first.parent == parent)
                result->emplace_back(
                    reinterpret_cast<Sdf_PathNode const *>(i->second.GetPtr()));
        }
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
