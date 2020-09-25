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
#include "pxr/usd/sdf/pathNode.h"
#include "pxr/usd/sdf/pathParser.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/trace/trace.h"

#include <algorithm>
#include <ostream>

using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

static inline bool _IsValidIdentifier(TfToken const &name);

// XXX: Enable this define to make bad path strings
// cause runtime errors.  This can be useful when trying to track down cases
// of bad path strings originating from python code.
// #define PARSE_ERRORS_ARE_ERRORS

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SdfPath>();
    TfType::Define< vector<SdfPath> >()
        .Alias(TfType::GetRoot(), "vector<SdfPath>");    
}

SdfPath::SdfPath(const std::string &path) {
    TfAutoMallocTag2 tag("Sdf", "SdfPath::SdfPath(string)");
    TRACE_FUNCTION();

    Sdf_PathParserContext context;

    // Initialize the scanner, allowing it to be reentrant.
    pathYylex_init(&context.scanner);

    yy_buffer_state *b = pathYy_scan_bytes(path.c_str(), path.size(),
                                           context.scanner);
    if( pathYyparse(&context) != 0 ) {
#ifdef PARSE_ERRORS_ARE_ERRORS
        TF_RUNTIME_ERROR("Ill-formed SdfPath <%s>: %s",
                         path.c_str(), context.errStr.c_str());
#else
        TF_WARN("Ill-formed SdfPath <%s>: %s",
                path.c_str(), context.errStr.c_str());
#endif
    } else {
        *this = std::move(context.path);
    }

    // Clean up.
    pathYy_delete_buffer(b, context.scanner);
    pathYylex_destroy(context.scanner);
}

const SdfPath &
SdfPath::EmptyPath()
{
    static SdfPath theEmptyPath;
    return theEmptyPath;
}

const SdfPath &
SdfPath::AbsoluteRootPath()
{
    static SdfPath *theAbsoluteRootPath =
        new SdfPath(Sdf_PathNode::GetAbsoluteRootNode(), nullptr);
    return *theAbsoluteRootPath;
}

const SdfPath &
SdfPath::ReflexiveRelativePath()
{
    static SdfPath *theReflexiveRelativePath =
        new SdfPath(Sdf_PathNode::GetRelativeRootNode(), nullptr);
    return *theReflexiveRelativePath;
}

size_t
SdfPath::GetPathElementCount() const
{
    size_t primElems = _primPart ? _primPart->GetElementCount() : 0;
    size_t propElems = _propPart ? _propPart->GetElementCount() : 0;
    return primElems + propElems;
}

bool
SdfPath::IsAbsolutePath() const
{
    return _primPart && _primPart->IsAbsolutePath();
}

bool
SdfPath::IsAbsoluteRootPath() const
{
    return !_propPart && _primPart && _primPart->IsAbsoluteRoot();
}

bool
SdfPath::IsPrimPath() const
{
    return !_propPart && _primPart &&
        (_primPart->GetNodeType() == Sdf_PathNode::PrimNode ||
         *this == ReflexiveRelativePath());
}

bool
SdfPath::IsAbsoluteRootOrPrimPath() const
{
    return !_propPart && _primPart &&
        (_primPart->GetNodeType() == Sdf_PathNode::PrimNode ||
         *this == AbsoluteRootPath() ||
         *this == ReflexiveRelativePath());
}

bool
SdfPath::IsRootPrimPath() const {
    if (_propPart)
        return false;
    Sdf_PathNode const *primNode = _primPart.get();
    return primNode && primNode->IsAbsolutePath() &&
        primNode->GetElementCount() == 1;
}

bool
SdfPath::IsPropertyPath() const
{
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        auto nodeType = propNode->GetNodeType();
        return nodeType == Sdf_PathNode::PrimPropertyNode ||
            nodeType == Sdf_PathNode::RelationalAttributeNode;
    }
    return false;
}

bool
SdfPath::IsPrimPropertyPath() const
{
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        return propNode->GetNodeType() ==  Sdf_PathNode::PrimPropertyNode;
    }
    return false;
}

bool
SdfPath::IsNamespacedPropertyPath() const
{
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        return propNode->IsNamespaced() &&
            // Currently this subexpression is always true if IsNamespaced() is.
            ((propNode->GetNodeType() ==
              Sdf_PathNode::PrimPropertyNode) ||
             (propNode->GetNodeType() ==
              Sdf_PathNode::RelationalAttributeNode));
    }
    return false;
}

bool
SdfPath::IsPrimVariantSelectionPath() const
{
    if (_propPart)
        return false;
    if (Sdf_PathNode const *primNode = _primPart.get()) {
        return primNode->GetNodeType() ==
            Sdf_PathNode::PrimVariantSelectionNode;
    }
    return false;
}

bool
SdfPath::IsPrimOrPrimVariantSelectionPath() const
{
    if (_propPart)
        return false;
    if (Sdf_PathNode const *primNode = _primPart.get()) {
        auto nodeType = primNode->GetNodeType();
        return
            nodeType == Sdf_PathNode::PrimNode ||
            nodeType == Sdf_PathNode::PrimVariantSelectionNode || 
            *this == ReflexiveRelativePath();
    }
    return false;
}

bool
SdfPath::ContainsPrimVariantSelection() const
{
    if (Sdf_PathNode const *primNode = _primPart.get()) {
        return primNode->ContainsPrimVariantSelection();
    }
    return false;
}

bool
SdfPath::ContainsTargetPath() const
{
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        return propNode->ContainsTargetPath();
    }
    return false;
}

bool
SdfPath::IsRelationalAttributePath() const {
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        return propNode->GetNodeType() == Sdf_PathNode::RelationalAttributeNode;
    }
    return false;
}

bool
SdfPath::IsTargetPath() const {
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        return propNode->GetNodeType() == Sdf_PathNode::TargetNode;
    }
    return false;
}

bool
SdfPath::IsMapperPath() const {
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        return propNode->GetNodeType() == Sdf_PathNode::MapperNode;
    }
    return false;
}

bool
SdfPath::IsMapperArgPath() const {
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        return propNode->GetNodeType() == Sdf_PathNode::MapperArgNode;
    }
    return false;
}

bool
SdfPath::IsExpressionPath() const {
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        return propNode->GetNodeType() == Sdf_PathNode::ExpressionNode;
    }
    return false;
}

TfToken
SdfPath::GetAsToken() const
{
    if (_primPart) {
        return Sdf_PathNode::GetPathAsToken(_primPart.get(), _propPart.get());
    }
    return TfToken();
}

std::string
SdfPath::GetAsString() const
{
    return GetAsToken().GetString();
}

TfToken const &
SdfPath::GetToken() const
{
    if (_primPart) {
        return Sdf_PathNode::GetPathToken(_primPart.get(), _propPart.get());
    }
    return SdfPathTokens->empty;
}

const std::string &
SdfPath::GetString() const
{
    return GetToken().GetString();
}

const char *
SdfPath::GetText() const
{
    return GetToken().GetText();
}

SdfPathVector
SdfPath::GetPrefixes() const {
    SdfPathVector result;
    GetPrefixes(&result);
    return result;
}

void
SdfPath::GetPrefixes(SdfPathVector *prefixes) const
{
    Sdf_PathNode const *prop = _propPart.get();
    Sdf_PathNode const *prim = _primPart.get();

    size_t elemCount = GetPathElementCount();
    prefixes->resize(elemCount);
    
    SdfPathVector::reverse_iterator iter = prefixes->rbegin();
    while (prop && elemCount--) {
        *iter++ = SdfPath(prim, prop);
        prop = prop->GetParentNode();
    }
    while (prim && elemCount--) {
        *iter++ = SdfPath(prim, prop);
        prim = prim->GetParentNode();
    }
}

SdfPathAncestorsRange
SdfPath::GetAncestorsRange() const
{
    return SdfPathAncestorsRange(*this);
}

const std::string &
SdfPath::GetName() const
{
    return GetNameToken().GetString();
}

const TfToken &
SdfPath::GetNameToken() const
{
    if (_propPart) {
        return _propPart.get()->GetName();
    }
    return _primPart ? _primPart.get()->GetName() : SdfPathTokens->empty;
}

string
SdfPath::GetElementString() const
{
    return GetElementToken().GetString();
}

TfToken
SdfPath::GetElementToken() const
{
    if (_propPart)
        return _propPart.get()->GetElement();
    return _primPart ? _primPart.get()->GetElement() : TfToken();
}    

SdfPath
SdfPath::ReplaceName(TfToken const &newName) const
{
    if (IsPrimPath())
        return GetParentPath().AppendChild(newName);
    else if (IsPrimPropertyPath())
        return GetParentPath().AppendProperty(newName);
    else if (IsRelationalAttributePath())
        return GetParentPath().AppendRelationalAttribute(newName);

    TF_CODING_ERROR("%s is not a prim, property, "
                    "or relational attribute path", GetText());
    return SdfPath();
}

static Sdf_PathNode const *
_GetNextTargetNode(Sdf_PathNode const *curNode)
{
    if (!curNode || !curNode->ContainsTargetPath())
        return nullptr;

    // Find nearest target or mapper node.
    while (curNode
           && curNode->GetNodeType() != Sdf_PathNode::TargetNode
           && curNode->GetNodeType() != Sdf_PathNode::MapperNode) {
        curNode = curNode->GetParentNode();
    }
    return curNode;
}

const SdfPath &
SdfPath::GetTargetPath() const
{
    if (!_propPart)
        return EmptyPath();
    Sdf_PathNode const *targetNode = _GetNextTargetNode(_propPart.get());
    return targetNode ? targetNode->GetTargetPath() : EmptyPath();
}

void
SdfPath::GetAllTargetPathsRecursively(SdfPathVector *result) const
{
    if (!_propPart)
        return;
    for (Sdf_PathNode const *targetNode = _GetNextTargetNode(_propPart.get());
         targetNode;
         targetNode = _GetNextTargetNode(targetNode->GetParentNode())) {
        SdfPath const &targetPath = targetNode->GetTargetPath();
        result->push_back(targetPath);
        targetPath.GetAllTargetPathsRecursively(result);
    }
}

pair<string, string>
SdfPath::GetVariantSelection() const
{
    pair<string, string> result;
    if (IsPrimVariantSelectionPath()) {
        const Sdf_PathNode::VariantSelectionType& sel =
            _primPart.get()->GetVariantSelection();
        result.first = sel.first.GetString();
        result.second = sel.second.GetString();
    }
    return result;
}

bool
SdfPath::HasPrefix(const SdfPath &prefix) const
{
    if (prefix.IsEmpty() || IsEmpty())
        return false;

    if (prefix._propPart) {
        // The prefix is a property-like path, in order for it to be a prefix of
        // this path, we must also have a property part, and our prim part must
        // be the same as the prefix's prim part.
        if (_primPart != prefix._primPart || !_propPart) {
            return false;
        }

        // Now walk up property parts until we hit prefix._propPart or we
        // recurse above its depth.
        Sdf_PathNode const *propNode = _propPart.get();
        Sdf_PathNode const *prefixPropNode = prefix._propPart.get();
        while (propNode && propNode != prefixPropNode) {
            propNode = propNode->GetParentNode();
        }
        return propNode == prefixPropNode;
    }
    else {
        // The prefix is a prim-like path.  Walk up nodes until we achieve the
        // same depth as the prefix, then just check for equality.
        Sdf_PathNode const *primNode = _primPart.get();

        if (primNode->IsAbsolutePath() &&
            prefix == SdfPath::AbsoluteRootPath()) {
            return true;
        }

        Sdf_PathNode const *prefixPrimNode = prefix._primPart.get();
        
        int prefixDepth = prefixPrimNode->GetElementCount();
        int curDepth = primNode->GetElementCount();

        if (curDepth < prefixDepth) {
            return false;
        }
        while (curDepth > prefixDepth) {
            primNode = primNode->GetParentNode();
            --curDepth;
        }
        return primNode == prefixPrimNode;
    }
}

SdfPath
SdfPath::GetParentPath() const {
    if (IsEmpty()) {
        return *this;
    }

    // If this is a property-like path, trim that first.
    if (_propPart) {
        Sdf_PathNode const *propNode = _propPart.get()->GetParentNode();
        return SdfPath(_primPart, Sdf_PathPropNodeHandle(propNode));
    }
    
    // This is a prim-like path.  If this is an absolute path (most common case)
    // then it's just the parent path node.  On the other hand if this path is a
    // relative path, and is '.' or ends with '..', the logical parent path is
    // made by appending a '..' component.
    //
    // XXX: NOTE that this is NOT the way that that Sdf_PathNode::GetParentNode
    // works, and note that most of the code in SdfPath uses GetParentNode
    // intentionally.
    Sdf_PathNode const *primNode = _primPart.get();
    if (ARCH_LIKELY(
            primNode->IsAbsolutePath() ||
            (primNode != Sdf_PathNode::GetRelativeRootNode() && 
             primNode->GetName() != SdfPathTokens->parentPathElement))) {
        return SdfPath(primNode->GetParentNode(), nullptr);
    }
    // Is relative root, or ends with '..'.
    return SdfPath(Sdf_PathNode::FindOrCreatePrim(
                       primNode, SdfPathTokens->parentPathElement),
                   Sdf_PathPropNodeHandle());
}

SdfPath
SdfPath::GetPrimPath() const {
    
    Sdf_PathNode const *primNode = _primPart.get();
    // Walk up looking for a prim node.
    while (primNode && primNode->GetNodeType() != Sdf_PathNode::PrimNode) {
        primNode = primNode->GetParentNode();
    }
    return SdfPath(primNode, nullptr);
}

SdfPath
SdfPath::GetPrimOrPrimVariantSelectionPath() const
{
    Sdf_PathNode const *primNode = _primPart.get();
    // Walk up looking for a prim or prim variant selection node.
    while (primNode &&
           (primNode->GetNodeType() != Sdf_PathNode::PrimNode &&
            primNode->GetNodeType() != Sdf_PathNode::PrimVariantSelectionNode)){
        primNode = primNode->GetParentNode();
    }
    return SdfPath(primNode, nullptr);
}

SdfPath
SdfPath::GetAbsoluteRootOrPrimPath() const {
    return (*this == AbsoluteRootPath()) ? *this : GetPrimPath();
}

static inline SdfPath
_AppendNode(const SdfPath &path, Sdf_PathNode const *node) {

    switch (node->GetNodeType()) {
        case Sdf_PathNode::PrimNode:
            return path.AppendChild(node->GetName());
        case Sdf_PathNode::PrimPropertyNode:
            return path.AppendProperty(node->GetName());
        case Sdf_PathNode::PrimVariantSelectionNode:
        {
            const Sdf_PathNode::VariantSelectionType& selection =
                node->GetVariantSelection();
            return path.AppendVariantSelection(selection.first.GetString(),
                                               selection.second.GetString());
        }
        case Sdf_PathNode::TargetNode:
            return path.AppendTarget( node->GetTargetPath());
        case Sdf_PathNode::RelationalAttributeNode:
            return path.AppendRelationalAttribute(node->GetName());
        case Sdf_PathNode::MapperNode:
            return path.AppendMapper(node->GetTargetPath());
        case Sdf_PathNode::MapperArgNode:
            return path.AppendMapperArg(node->GetName());
        case Sdf_PathNode::ExpressionNode:
            return path.AppendExpression();
        default:
            // CODE_COVERAGE_OFF
            // Should never get here.  All reasonable cases are
            // handled above.
            TF_CODING_ERROR("Unexpected node type %i", node->GetNodeType());
            return SdfPath::EmptyPath();
            // CODE_COVERAGE_ON
    }
}

SdfPath
SdfPath::StripAllVariantSelections() const {
    if (!ContainsPrimVariantSelection())
        return *this;
    TRACE_FUNCTION();
    std::vector<Sdf_PathNode const *> primNodes;
    Sdf_PathNode const *curNode = _primPart.get();
    while (curNode) {
        if (curNode->GetNodeType() != Sdf_PathNode::PrimVariantSelectionNode)
            primNodes.push_back(curNode);
        curNode = curNode->GetParentNode();
    }

    SdfPath stripPath(*(primNodes.rbegin()), nullptr);
    // Step through all primNodes except the last (which is the root node):
    for (auto it = ++(primNodes.rbegin()); it != primNodes.rend(); ++it) {
        stripPath = _AppendNode(stripPath, *it);
    }
    // Tack on any property portion.
    stripPath._propPart = _propPart;
    return stripPath;
}


SdfPath
SdfPath::AppendPath(const SdfPath &newSuffix) const {
    if (*this == EmptyPath()) {
        TF_CODING_ERROR("Cannot append to invalid path");
        return EmptyPath();
    }
    if (newSuffix == EmptyPath()) {
        TF_CODING_ERROR("Cannot append invalid path to <%s>",
                        GetAsString().c_str());
        return EmptyPath();
    }
    if (newSuffix.IsAbsolutePath()) {
        TF_WARN("Cannot append absolute path <%s> to another path <%s>.",
                newSuffix.GetAsString().c_str(), GetAsString().c_str());
        return EmptyPath();
    }
    if (newSuffix == ReflexiveRelativePath()) {
        return *this;
    }

    Sdf_PathNode::NodeType primNodeType = _primPart->GetNodeType();
    if (_propPart || (primNodeType != Sdf_PathNode::RootNode &&
                      primNodeType != Sdf_PathNode::PrimNode &&
                      primNodeType != Sdf_PathNode::PrimVariantSelectionNode)) {
        TF_WARN("Cannot append a path to another path that is not "
                    "a root or a prim path.");
        return EmptyPath();
    }

    // This list winds up in reverse order to what one might at first expect.
    vector<Sdf_PathNode const *> tailNodes;

    // Walk up to top of newSuffix.
    Sdf_PathNode const *curNode = newSuffix._propPart.get();
    while (curNode) {
        tailNodes.push_back(curNode);
        curNode = curNode->GetParentNode();
    }
    curNode = newSuffix._primPart.get();
    while (curNode != Sdf_PathNode::GetRelativeRootNode()) {
        tailNodes.push_back(curNode);
        curNode = curNode->GetParentNode();
    }

    if ((tailNodes.back()->GetNodeType() == Sdf_PathNode::PrimPropertyNode) && 
        *this == AbsoluteRootPath()) {
        TF_WARN("Cannot append a property path to the absolute root path.");
        return EmptyPath();
    }

    SdfPath result = *this;

    // We have a list of new nodes (in reverse order) to append to our node.
    for (auto it = tailNodes.rbegin();
         (it != tailNodes.rend()) && (result != EmptyPath()); ++it) {
        result = _AppendNode(result, *it);
    }
    return result;
}

// Use a simple per-thread cache for appending children to prim paths.  This
// lets us avoid hitting the global table, reducing thread contention, for
// appending children repeatedly to a node.
namespace {
struct _PerThreadPrimPathCache
{
    static constexpr unsigned Shift = 14;
    static constexpr unsigned Size = 1 << Shift;
    static constexpr unsigned ProbeShift = 1;
    static constexpr unsigned Probes = 1 << ProbeShift;

    struct _Entry {
        Sdf_PathPrimNodeHandle parent;
        Sdf_PathPrimNodeHandle primPart;
        TfToken childName;
    };

    inline Sdf_PathPrimNodeHandle
    Find(Sdf_PathPrimNodeHandle const &parent, TfToken const &childName,
         int *outIndex) const {
        // Hash and shift to find table index.
        size_t h = childName.Hash();
        uint32_t parentAsInt;
        memcpy(&parentAsInt, &parent, sizeof(uint32_t));
        boost::hash_combine(h, parentAsInt >> 8);
        unsigned index = (h & (Size-1));

        for (unsigned probe = 0; probe != Probes; ++probe) {
            _Entry const &e = cache[(index + probe) & (Size - 1)];
            if (e.parent == parent && e.childName == childName) {
                // Cache hit.
                return e.primPart;
            }
            if (!e.parent)
                break;
        }
        
        // Not found -- arrange to replace original hash index.
        *outIndex = index;
        return Sdf_PathPrimNodeHandle();
    }

    inline void
    Store(Sdf_PathPrimNodeHandle const &parent, TfToken const &childName,
          Sdf_PathPrimNodeHandle primPart, int index) {
        cache[index] = { parent, primPart, childName };
    }

    _Entry cache[Size];
};
}

namespace {
// XXX: Workaround for Windows issue USD-5306 -- this avoids destroying the
// per-thread caches to deal with static destruction order problems.
template <class T>
struct _FastThreadLocalBase
{
    static T &Get() {
        static thread_local T *theTPtr = nullptr;
        if (ARCH_LIKELY(theTPtr)) {
            return *theTPtr;
        }
        static thread_local
            typename std::aligned_storage<sizeof(T)>::type storage;
        void *addr = &storage;
        T *p = new (addr) T;
        theTPtr = p;
        return *p;
    }
};
}
using _PrimPathCache = _FastThreadLocalBase<_PerThreadPrimPathCache>;
static _PrimPathCache _primPathCache;

SdfPath
SdfPath::AppendChild(TfToken const &childName) const {
    if (ARCH_UNLIKELY(_propPart)) {
        TF_WARN("Cannot append child '%s' to path '%s'.",
                childName.GetText(), GetText());
        return EmptyPath();
    }
    auto &cache = _primPathCache.Get();
    int storeIndex = 0;
    Sdf_PathPrimNodeHandle primPart =
        cache.Find(_primPart, childName, &storeIndex);
    SdfPath ret { primPart, {} };
    if (primPart) {
        return ret;
    }
    if (!IsAbsoluteRootOrPrimPath()
        && !IsPrimVariantSelectionPath()
        && (*this != ReflexiveRelativePath())) {
        TF_WARN("Cannot append child '%s' to path '%s'.",
                childName.GetText(), GetText());
        return EmptyPath();
    }
    if (ARCH_UNLIKELY(childName == SdfPathTokens->parentPathElement)) {
        return GetParentPath();
    } else {
        if (ARCH_UNLIKELY(!_IsValidIdentifier(childName))) {
            TF_WARN("Invalid prim name '%s'", childName.GetText());
            return EmptyPath();
        }
        ret._primPart =
            Sdf_PathNode::FindOrCreatePrim(_primPart.get(), childName);
        cache.Store(_primPart, childName, ret._primPart, storeIndex);
        return ret;
    }
}


// Use a simple per-thread cache for appending prim properties.  This lets us
// avoid hitting the global table, reducing thread contention and increasing
// speed.  We don't do this for the other property-type paths, like target paths
// or relational attribute paths because those operations are done much less
// frequently than appending properties to prim paths.
namespace {
struct _PerThreadPropertyPathCache
{
    static constexpr unsigned Shift = 10;
    static constexpr unsigned Size = 1 << Shift;
    static constexpr unsigned ProbeShift = 1;
    static constexpr unsigned Probes = 1 << ProbeShift;

    struct _Entry {
        TfToken propName;
        Sdf_PathPropNodeHandle propPart;
    };

    inline Sdf_PathPropNodeHandle
    Find(TfToken const &propName, int *outIndex) const {
        // Hash and shift to find table index.
        size_t h = propName.Hash();
        unsigned index = (h >> (8*sizeof(h) - Shift));

        for (unsigned probe = 0; probe != Probes; ++probe) {
            _Entry const &e = cache[(index + probe) & (Size - 1)];
            if (e.propName == propName) {
                // Cache hit.
                return e.propPart;
            }
            if (e.propName.IsEmpty())
                break;
        }
        
        // Not found -- arrange to replace original hash index.
        *outIndex = index;
        return Sdf_PathPropNodeHandle();
    }

    inline void
    Store(TfToken const &propName, Sdf_PathPropNodeHandle propPart, int index) {
        cache[index] = { propName, propPart };
    }

    _Entry cache[Size];
};
}

using _PropPathCache = Sdf_FastThreadLocalBase<_PerThreadPropertyPathCache>;
static _PropPathCache _propPathCache;

SdfPath
SdfPath::AppendProperty(TfToken const &propName) const {
    if (ARCH_UNLIKELY(_propPart)) {
        TF_WARN("Can only append a property '%s' to a prim path (%s)",
                propName.GetText(), GetText());
        return EmptyPath();
    }
    auto &cache = _propPathCache.Get();
    int storeIndex = 0;
    Sdf_PathPropNodeHandle propPart = cache.Find(propName, &storeIndex);
    SdfPath ret { _primPart, propPart };
    if (propPart) {
        return ret;
    }
    if (!IsValidNamespacedIdentifier(propName.GetString())) {
        //TF_WARN("Invalid property name.");
        return EmptyPath();
    }
    if (!IsPrimVariantSelectionPath() && 
        !IsPrimPath() && (*this != ReflexiveRelativePath())) {
        TF_WARN("Can only append a property '%s' to a prim path (%s)",
                propName.GetText(), GetText());
        return EmptyPath();
    }
    ret._propPart =
        Sdf_PathNode::FindOrCreatePrimProperty(_primPart.get(), propName);
    cache.Store(propName, ret._propPart, storeIndex);
    return ret;
}

SdfPath
SdfPath::AppendVariantSelection(const string &variantSet,
                                const string &variant) const
{
    if (!IsPrimOrPrimVariantSelectionPath()) {
        TF_CODING_ERROR("Cannot append variant selection %s = %s to <%s>; "
                        "can only append a variant selection to a prim or "
                        "prim variant selection path.",
                        variantSet.c_str(), variant.c_str(),
                        GetText());
        return EmptyPath();
    }
    return SdfPath(Sdf_PathNode::
                   FindOrCreatePrimVariantSelection(_primPart.get(),
                                                    TfToken(variantSet),
                                                    TfToken(variant)));
}

SdfPath
SdfPath::AppendTarget(const SdfPath &targetPath) const {
    if (!IsPropertyPath()) {
        TF_WARN("Can only append a target to a property path.");
        return EmptyPath();
    }
    if (targetPath == EmptyPath()) {
        TF_WARN("Target path cannot be invalid.");
        return EmptyPath();
    }
    return SdfPath(_primPart, Sdf_PathNode::
                   FindOrCreateTarget(_propPart.get(), targetPath));
}

SdfPath
SdfPath::AppendRelationalAttribute(TfToken const &attrName) const {
    if (!IsValidNamespacedIdentifier(attrName)) {
        TF_WARN("Invalid property name.");
        return EmptyPath();
    }
    if (!IsTargetPath()) {
        TF_WARN("Can only append a relational attribute to a target path.");
        return EmptyPath();
    }
    return SdfPath(_primPart,
                   Sdf_PathNode::FindOrCreateRelationalAttribute(
                       _propPart.get(), attrName));
}

SdfPath
SdfPath::AppendMapper(const SdfPath &targetPath) const {
    if (!IsPropertyPath()) {
        TF_WARN("Cannnot append mapper '%s' to non-property path <%s>.",
                targetPath.GetAsString().c_str(), GetAsString().c_str());
        return EmptyPath();
    }
    if (targetPath == EmptyPath()) {
        TF_WARN("Cannot append an empty mapper target path to <%s>",
                GetAsString().c_str());
        return EmptyPath();
    }
    return SdfPath { _primPart, 
            Sdf_PathNode::FindOrCreateMapper(_propPart.get(), targetPath) };
}

SdfPath
SdfPath::AppendMapperArg(TfToken const &argName) const {
    if (!_IsValidIdentifier(argName)) {
        TF_WARN("Invalid arg name.");
        return EmptyPath();
    }
    if (!IsMapperPath()) {
        TF_WARN("Can only append a mapper arg to a mapper path.");
        return EmptyPath();
    }
    return SdfPath { _primPart,
            Sdf_PathNode::FindOrCreateMapperArg(_propPart.get(), argName) };
}

SdfPath
SdfPath::AppendExpression() const {
    if (!IsPropertyPath()) {
        TF_WARN("Can only append an expression to a property path.");
        return EmptyPath();
    }
    return SdfPath { _primPart,
            Sdf_PathNode::FindOrCreateExpression(_propPart.get()) };
}

SdfPath
SdfPath::AppendElementString(const std::string &element) const
{
    return AppendElementToken(TfToken(element));
}

SdfPath
SdfPath::AppendElementToken(const TfToken &elementTok) const
{
    std::string const &element = elementTok.GetString();

    if (ARCH_UNLIKELY(IsEmpty() || element.empty())) {
        if (IsEmpty()) {
            TF_CODING_ERROR("Cannot append element \'%s\' to the EmptyPath.",
                            element.c_str());
        }
        else {
            TF_CODING_ERROR("Cannot append EmptyPath as a path element.");
        }
        return EmptyPath();
    }
    /* This is a somewhat unfortunate replication of a subset of the
     * logic contained in the full-path-parser.  We can't invoke the
     * parser on just a single element out of context (and probably
     * wouldn't want to for cost reasons if we could).  Can't think of
     * a more elegant way to do this.  1/13 */
    char const *txt = element.c_str();
    // No static tokens for variant chars...
    if (txt[0] == '{') {
        
        vector<string> tokens = TfStringTokenize(element, "{=}");
        TfToken variantSel;
        if (tokens.size() == 2){
            variantSel = TfToken(tokens[1]);
        } 
        else if (tokens.size() != 1){
            return EmptyPath();
        }
        return AppendVariantSelection(TfToken(tokens[0]),
                                      variantSel);
        
    } 
    else if (txt[0] == SdfPathTokens->relationshipTargetStart.GetString()[0]) {
        SdfPath  target(element.substr(1, element.length()-2));
        return AppendTarget(target);
    } 
    else if (txt[0] == SdfPathTokens->propertyDelimiter.GetString()[0]) {
        // This is the ambiguous one.  First check for the special symbols,
        // and if it looks like a "plain old property", consult parent type
        // to determine what the property sub-type should be.
        static string mapperStr = SdfPathTokens->propertyDelimiter.GetString() +
            SdfPathTokens->mapperIndicator.GetString() +
            SdfPathTokens->relationshipTargetStart.GetString();
        static string expressionStr =
            SdfPathTokens->propertyDelimiter.GetString()
            + SdfPathTokens->expressionIndicator.GetString();
        
        if ( element == expressionStr ){
            return IsPropertyPath() ? AppendExpression() : 
                AppendProperty(SdfPathTokens->expressionIndicator);
        } 
        else if (TfStringStartsWith(element, mapperStr)){
            const size_t prefixSz(mapperStr.length());
            SdfPath  target(element.substr(prefixSz, 
                                          element.length()-(prefixSz+1)));
            return AppendMapper(target);
        }
        else {
            TfToken  property(element.substr(1));
            
            if (IsMapperPath()){
                return AppendMapperArg(property);
            }
            else if (IsTargetPath()){
                return AppendRelationalAttribute(property);
            }
            else {
                return AppendProperty(property);
            }
        }
    } 
    else {
        return AppendChild(elementTok);
    }
        
}

SdfPath
SdfPath::_ReplacePrimPrefix(SdfPath const &oldPrefix,
                            SdfPath const &newPrefix) const
{
    using Sdf_PathNodeConstPtr = Sdf_PathNode const *;

    // Walk up the prim part of this path until we have the same depth as
    // oldPrefix, recording tail elements along the way.  If we find oldPrefix
    // is in fact a prefix, then tack the tail elements onto newPrefix and
    // return the resulting path with our property element (if any).  Otherwise
    // return this path unchanged.
    
    Sdf_PathNodeConstPtr primNode = _primPart.get();
    Sdf_PathNodeConstPtr prefixPrimNode = oldPrefix._primPart.get();
        
    int prefixDepth = prefixPrimNode->GetElementCount();
    int curDepth = primNode->GetElementCount();

    if (curDepth < prefixDepth) {
        return *this;
    }

    // Make space for temporary tail nodes.  Use stack if small enough.
    constexpr size_t MaxLocalNodes = 16;
    Sdf_PathNodeConstPtr localNodes[MaxLocalNodes];
    std::unique_ptr<Sdf_PathNodeConstPtr []> remoteNodes;
    Sdf_PathNodeConstPtr *tmpNodes = localNodes;
    size_t requiredTmpNodes = curDepth - prefixDepth;
    if (requiredTmpNodes > MaxLocalNodes) {
        remoteNodes.reset(new Sdf_PathNodeConstPtr[requiredTmpNodes]);
        tmpNodes = remoteNodes.get();
    }

    size_t i = 0;
    while (curDepth > prefixDepth) {
        tmpNodes[i++] = primNode;
        primNode = primNode->GetParentNode();
        --curDepth;
    }
    
    if (primNode != prefixPrimNode) {
        return *this;
    }

    // Tack the prim elements onto newPrefix.
    SdfPath newPath = newPrefix;
    while (i--) {
        switch (tmpNodes[i]->GetNodeType()) {
        case Sdf_PathNode::PrimNode:
            newPath._primPart = Sdf_PathNode::FindOrCreatePrim(
                newPath._primPart.get(), tmpNodes[i]->GetName());
            break;
        default:
            newPath = _AppendNode(newPath, tmpNodes[i]);
        }
    }

    // Add our property element.
    newPath._propPart = _propPart;

    return newPath;
}

SdfPath
SdfPath::_ReplaceTargetPathPrefixes(SdfPath const &oldPrefix,
                                    SdfPath const &newPrefix) const
{
    using Sdf_PathNodeConstPtr = Sdf_PathNode const *;

    // Go through all the target paths in this path, and replace their prefixes.
    Sdf_PathNode const *propNode = _propPart.get();
    if (!propNode->ContainsTargetPath()) {
        return *this;
    }

    // Make space for temporary tail nodes.  Use stack if small enough.
    constexpr size_t MaxLocalNodes = 16;
    Sdf_PathNodeConstPtr localNodes[MaxLocalNodes];
    std::unique_ptr<Sdf_PathNodeConstPtr []> remoteNodes;
    Sdf_PathNodeConstPtr *tmpNodes = localNodes;
    size_t requiredTmpNodes = propNode->GetElementCount();
    if (requiredTmpNodes > MaxLocalNodes) {
        remoteNodes.reset(new Sdf_PathNodeConstPtr[requiredTmpNodes]);
        tmpNodes = remoteNodes.get();
    }

    size_t i = 0;
    while (propNode && propNode->ContainsTargetPath()) {
        tmpNodes[i++] = propNode;
        propNode = propNode->GetParentNode();
    }
    
    // Tack the prop elements onto newPrefix's prop part.
    SdfPath newPath(_primPart.get(), propNode);
    while (i--) {
        switch (tmpNodes[i]->GetNodeType()) {
        case Sdf_PathNode::PrimPropertyNode:
            newPath._propPart = Sdf_PathNode::FindOrCreatePrimProperty(
                nullptr, tmpNodes[i]->GetName());
            break;
        case Sdf_PathNode::TargetNode:
            newPath = newPath.AppendTarget(
                tmpNodes[i]->GetTargetPath().ReplacePrefix(
                    oldPrefix, newPrefix, /*fixTargetPaths=*/true));
            break;
        case Sdf_PathNode::MapperNode:
            newPath = newPath.AppendMapper(
                tmpNodes[i]->GetTargetPath().ReplacePrefix(
                    oldPrefix, newPrefix, /*fixTargetPaths=*/true));
            break;
        default:
            newPath = _AppendNode(newPath, tmpNodes[i]);
        }
    }

    return newPath;
}

SdfPath
SdfPath::_ReplacePropPrefix(SdfPath const &oldPrefix,
                            SdfPath const &newPrefix,
                            bool fixTargetPaths) const
{ 
    using Sdf_PathNodeConstPtr = Sdf_PathNode const *;

    // Walk up the prop part of this path until we have the same depth as
    // oldPrefix's prop part, recording tail elements along the way.  If we find
    // oldPrefix is in fact a prefix, then tack the tail elements onto newPrefix
    // (replacing prefixes in target paths if \p fixTargetPaths is true).  If
    // oldPrefix is not found, then just replace target paths in all the
    // elements.
    
    Sdf_PathNodeConstPtr propNode = _propPart.get();
    Sdf_PathNodeConstPtr prefixPropNode = oldPrefix._propPart.get();
        
    int prefixDepth = prefixPropNode->GetElementCount();
    int curDepth = propNode->GetElementCount();

    if (curDepth < prefixDepth) {
        return (fixTargetPaths && propNode->ContainsTargetPath()) ? 
            _ReplaceTargetPathPrefixes(oldPrefix, newPrefix) : *this;
    }

    // Make space for temporary tail nodes.  Use stack if small enough.
    constexpr size_t MaxLocalNodes = 16;
    Sdf_PathNodeConstPtr localNodes[MaxLocalNodes];
    std::unique_ptr<Sdf_PathNodeConstPtr []> remoteNodes;
    Sdf_PathNodeConstPtr *tmpNodes = localNodes;
    size_t requiredTmpNodes = curDepth - prefixDepth;
    if (requiredTmpNodes > MaxLocalNodes) {
        remoteNodes.reset(new Sdf_PathNodeConstPtr[requiredTmpNodes]);
        tmpNodes = remoteNodes.get();
    }

    size_t i = 0;
    while (curDepth > prefixDepth) {
        tmpNodes[i++] = propNode;
        propNode = propNode->GetParentNode();
        --curDepth;
    }
    
    if (propNode != prefixPropNode) {
        return (fixTargetPaths && ContainsTargetPath()) ?
            _ReplaceTargetPathPrefixes(oldPrefix, newPrefix) : *this;
    }

    // Tack the prop elements onto newPrefix's prop part.
    SdfPath newPath = newPrefix;
    while (i--) {
        switch (tmpNodes[i]->GetNodeType()) {
        case Sdf_PathNode::PrimPropertyNode:
            newPath._propPart = Sdf_PathNode::FindOrCreatePrimProperty(
                nullptr, tmpNodes[i]->GetName());
            break;
        case Sdf_PathNode::TargetNode:
            if (fixTargetPaths) {
                newPath = newPath.AppendTarget(
                    tmpNodes[i]->GetTargetPath().ReplacePrefix(
                        oldPrefix, newPrefix, fixTargetPaths));
            } else {
                newPath = _AppendNode(newPath, tmpNodes[i]);
            }
            break;
        case Sdf_PathNode::MapperNode:
            if (fixTargetPaths) {
                newPath = newPath.AppendMapper(
                    tmpNodes[i]->GetTargetPath().ReplacePrefix(
                        oldPrefix, newPrefix, fixTargetPaths));
            } else {
                newPath = _AppendNode(newPath, tmpNodes[i]);
            }
            break;
        default:
            newPath = _AppendNode(newPath, tmpNodes[i]);
        }
    }

    return newPath;
}


SdfPath
SdfPath::ReplacePrefix(const SdfPath &oldPrefix, const SdfPath &newPrefix,
                       bool fixTargetPaths) const
{
    // Perhaps surprisingly, this path need not have oldPrefix as a prefix.  For
    // example, '/a.rel[/target]'.ReplacePrefix('/target', '/other/target') ->
    // '/a.rel[/other/target]' when fixTargetPaths == true.

    TRACE_FUNCTION();

    if (IsEmpty() || oldPrefix == newPrefix) {
        return *this;
    }
    if (oldPrefix.IsEmpty() || newPrefix.IsEmpty()) {
        return EmptyPath();
    }
    if (*this == oldPrefix) {
        return newPrefix;
    }

    using Sdf_PathNodeConstPtr = Sdf_PathNode const *;

    Sdf_PathNodeConstPtr primNode = _primPart.get();
    Sdf_PathNodeConstPtr propNode = _propPart.get();

    SdfPath newPath;
    
    if (!oldPrefix._propPart) {
        // oldPrefix is a prim-like path.  Replace the prefix in the prim part,
        // if it has oldPrefix as a prefix.
        newPath = _ReplacePrimPrefix(oldPrefix, newPrefix);

        if (fixTargetPaths && propNode && propNode->ContainsTargetPath()) {
            // This path is property-like and contains targets that we need to
            // fix, so fix them up.
            newPath = newPath._ReplaceTargetPathPrefixes(oldPrefix, newPrefix);
        }
    }
    else {
        // oldPrefix is a property-like path.  If this path is a prim-like path
        // then oldPrefix cannot be a prefix of this path and we do not have
        // targets to fix.
        if (!propNode) {
            return *this;
        }

        // Both oldPrefix and this are property-like paths.  If the prim parts
        // do not match, then we just replace targets (or do nothing).  If they
        // do match, then we walk up prop nodes to same depth (or as long as we
        // need to fix targets) and replace.  But crucially, we have to search
        // for the property part of oldPrefix as a prefix of this path's
        // property part.  If we find it then the resulting path has newPrefix's
        // prim part, otherwise it has this path's prim part.
        
        if (primNode != oldPrefix._primPart.get()) {
            if (fixTargetPaths && propNode->ContainsTargetPath()) {
                newPath = _ReplaceTargetPathPrefixes(oldPrefix, newPrefix);
            }
            else {
                return *this;
            }
        }
        else {
            newPath = _ReplacePropPrefix(oldPrefix, newPrefix, fixTargetPaths);
        }
    }

    return newPath;
}

SdfPath
SdfPath::GetCommonPrefix(const SdfPath &path2) const {

    if (path2.IsEmpty()) {
        TF_WARN("GetCommonPrefix(): invalid path.");
        return SdfPath();
    }

    SdfPath const &path1 = *this;
    
    // Skip as much as we can based on whether or not the paths have property
    // elements, etc.  We either start in the prim area (if one or both paths
    // have no property elements, or if they both do but the leafmost prim
    // elements differ) or we stay fully in the property area (up to the
    // leafmost prim element).
    Sdf_PathNode const *path1Node;
    Sdf_PathNode const *path2Node;

    bool isPrimLike = true;
    if (ARCH_LIKELY(!path1._propPart || !path2._propPart ||
                    (path1._primPart != path2._primPart))) {
        path1Node = path1._primPart.get();
        path2Node = path2._primPart.get();
    }
    else {
        isPrimLike = false;
        path1Node = path1._propPart.get();
        path2Node = path2._propPart.get();
    }        

    size_t count1 = path1Node->GetElementCount();
    size_t count2 = path2Node->GetElementCount();

    while (count1 > count2) {
        path1Node = path1Node->GetParentNode();
        --count1;
    }
    while (count2 > count1) {
        path2Node = path2Node->GetParentNode();
        --count2;
    }

    while (path1Node != path2Node) {
        path1Node = path1Node->GetParentNode();
        path2Node = path2Node->GetParentNode();
    }
    
    SdfPath ret;
    if (ARCH_LIKELY(isPrimLike)) {
        ret._primPart = path1Node;
    }
    else {
        ret._primPart = path1._primPart;
        ret._propPart = path1Node;
    }
    return ret;
}

namespace {
struct _NodeEqual
{
    template <class T>
    inline bool operator()(T const &a, T const &b) const {
        return a == b;
    }
};
}

std::pair<SdfPath, SdfPath>
SdfPath::RemoveCommonSuffix(const SdfPath& otherPath,
                            bool stopAtRootPrim) const {

    if (IsEmpty() || otherPath.IsEmpty() ||
        (static_cast<bool>(_propPart) ^
         static_cast<bool>(otherPath._propPart))) {
        return std::make_pair(*this, otherPath);
    }

    // Scan upwards until we find a difference or a root node or child of
    // a root node.  Root nodes have element counts of 0 and their children
    // elements counts of 1.

    if (_propPart) {
        Sdf_PathNode const *thisProp = _propPart.get();
        Sdf_PathNode const *otherProp = otherPath._propPart.get();
        while (thisProp && otherProp) {
            if (!thisProp->Compare<_NodeEqual>(*otherProp)) {
                return std::make_pair(
                    SdfPath(_primPart,
                            Sdf_PathPropNodeHandle(thisProp)),
                    SdfPath(otherPath._primPart,
                            Sdf_PathPropNodeHandle(otherProp)));
            }
            thisProp = thisProp->GetParentNode();
            otherProp = otherProp->GetParentNode();
        }
        if (thisProp || otherProp) {
            return std::make_pair(
                SdfPath(_primPart, Sdf_PathPropNodeHandle(thisProp)),
                SdfPath(otherPath._primPart,
                        Sdf_PathPropNodeHandle(otherProp)));
        }
    }

    Sdf_PathNode const *thisPrim = _primPart.get();
    Sdf_PathNode const *otherPrim = otherPath._primPart.get();
    
    while (thisPrim->GetElementCount() > 1 &&
           otherPrim->GetElementCount() > 1) {
        if (!thisPrim->Compare<_NodeEqual>(*otherPrim)) {
            return std::make_pair(SdfPath(thisPrim, nullptr),
                                  SdfPath(otherPrim, nullptr));
        }
        thisPrim = thisPrim->GetParentNode();
        otherPrim = otherPrim->GetParentNode();
    }

    // If stopAtRootPrim is not true and neither path is a root then we
    // can scan upwards one more level.
    if (!stopAtRootPrim &&
        thisPrim->GetElementCount() >= 1 &&
        otherPrim->GetElementCount() >= 1 &&
        thisPrim->Compare<_NodeEqual>(*otherPrim)) {
        thisPrim = thisPrim->GetParentNode();
        otherPrim = otherPrim->GetParentNode();
    }
    return std::make_pair(SdfPath(thisPrim, nullptr),
                          SdfPath(otherPrim, nullptr));
}

SdfPath
SdfPath::ReplaceTargetPath(const SdfPath &newTargetPath) const {

    if (IsEmpty()) {
        return SdfPath();
    }

    if (newTargetPath == SdfPath()) {
        TF_WARN("ReplaceTargetPath(): invalid new target path.");
        return SdfPath();
    }

    if (_propPart) {
        Sdf_PathNode const *propNode = _propPart.get();
        Sdf_PathNode::NodeType type = _propPart->GetNodeType();
        if (type == Sdf_PathNode::TargetNode) {
            return GetParentPath().AppendTarget(newTargetPath);
        } else if (type == Sdf_PathNode::RelationalAttributeNode) {
            return GetParentPath().ReplaceTargetPath(newTargetPath).
                AppendRelationalAttribute(propNode->GetName());
        } else if (type == Sdf_PathNode::MapperNode) {
            return GetParentPath().AppendMapper(newTargetPath);
        } else if (type == Sdf_PathNode::MapperArgNode) {
            return GetParentPath().ReplaceTargetPath(newTargetPath).
                AppendMapperArg(propNode->GetName());
        } else if (type == Sdf_PathNode::ExpressionNode) {
            return GetParentPath().ReplaceTargetPath(newTargetPath).
                AppendExpression();
        }
    }

    // no target to replace
    // return path unchanged
    return *this;
}

SdfPath
SdfPath::MakeAbsolutePath(const SdfPath & anchor) const {

    SdfPath result;

    if (anchor == SdfPath()) {
        TF_WARN("MakeAbsolutePath(): anchor is the empty path.");
        return result;
    }

    // Check that anchor is an absolute path
    if (!anchor.IsAbsolutePath()) {
        TF_WARN("MakeAbsolutePath() requires an absolute path as an argument.");
        return result;
    }

    // Check that anchor is a prim-like path
    if (!anchor.IsAbsoluteRootOrPrimPath() && 
        !anchor.IsPrimVariantSelectionPath()) {
        TF_WARN("MakeAbsolutePath() requires a prim path as an argument.");
        return result;
    }

    // If we're empty, just return empty.
    if (IsEmpty()) {
        return result;
    }

    // If we're not already absolute, do our own path using anchor as the
    // relative base.
    if (ARCH_LIKELY(!IsAbsolutePath())) {

        // Collect all the ancestral path nodes.
        Sdf_PathNode const *curNode = _primPart.get();
        size_t numNodes = curNode->GetElementCount();
        vector<Sdf_PathNode const *> relNodes(numNodes);
        while (numNodes--) {
            relNodes[numNodes] = curNode;
            curNode = curNode->GetParentNode();
        }

        // Now append all the nodes to the anchor to produce the absolute path.
        result = anchor;
        for (Sdf_PathNode const *node: relNodes) {
            result = _AppendNode(result, node);
            if (result.IsEmpty()) {
                break;
            }
        }
    }
    else {
        result = *this;
    }

    // If we failed to produce a path, return empty.  This happens in cases
    // where we try to make paths like '../../..' absolute with anchors that are
    // shorter, causing us to try to go "outside the root".
    if (result.IsEmpty()) {
        return result;
    }

    // Tack on any property path.
    result._propPart = _propPart;

    // Now make target paths absolute (recursively) if we need to.
    // We need to use result's prim path as the anchor for the target path.
    SdfPath const &targetPath = result.GetTargetPath();
    if (!targetPath.IsEmpty()) {
        SdfPath primPath = result.GetPrimPath();
        SdfPath newTargetPath = targetPath.MakeAbsolutePath(primPath);
        result = result.ReplaceTargetPath(newTargetPath);
    }

    return result;
}

SdfPath
SdfPath::MakeRelativePath(const SdfPath & anchor) const
{
    TRACE_FUNCTION();

    // Check that anchor is a valid path
    if ( anchor == SdfPath() ) {
        TF_WARN("MakeRelativePath(): anchor is the invalid path.");
        return SdfPath();
    }

    // Check that anchor is an absolute path
    if (!anchor.IsAbsolutePath()) {
        TF_WARN("MakeRelativePath() requires an absolute path as an argument.");
        return SdfPath();
    }

    // Check that anchor is a prim-like path
    if (!anchor.IsAbsoluteRootOrPrimPath() &&
        !anchor.IsPrimVariantSelectionPath()) {
        TF_WARN("MakeRelativePath() requires a prim, prim variant selection, "
                "or absolute root path as an anchor (got '%s').",
                 anchor.GetAsString().c_str());
        return SdfPath();
    }

    // If we're invalid, just return a copy of ourselves.
    if (IsEmpty()) {
        return SdfPath();
    }

    if (!IsAbsolutePath()) {
        // Canonicalize... make sure the relative path has the
        // fewest possible dot-dots.
        SdfPath absPath = MakeAbsolutePath(anchor);
        return absPath.MakeRelativePath(anchor);
    }

    // We are absolute, we want to be relative

    // This list winds up in reverse order to what one might at first expect.
    vector<Sdf_PathNode const *> relNodes;

    // We need to crawl up the this path until we are the same length as
    // the anchor.
    // Then we crawl up both till we find the matching nodes.
    // As we crawl, we build the relNodes vector.
    size_t thisCount = _primPart->GetElementCount();
    size_t anchorCount = anchor._primPart->GetElementCount();

    Sdf_PathNode const *curThisNode = _primPart.get();
    Sdf_PathNode const *curAnchorNode = anchor._primPart.get();

    // walk to the same depth
    size_t dotdotCount = 0;

    while (thisCount > anchorCount) {
        relNodes.push_back(curThisNode);
        curThisNode = curThisNode->GetParentNode();
        --thisCount;
    }

    while (thisCount < anchorCount) {
        ++dotdotCount;
        curAnchorNode = curAnchorNode->GetParentNode();
        --anchorCount;
    }

    // now we're at the same depth
    TF_AXIOM(thisCount == anchorCount);

    // walk to a common prefix
    while (curThisNode != curAnchorNode) {
        ++dotdotCount;
        relNodes.push_back(curThisNode);
        curThisNode   = curThisNode->GetParentNode();
        curAnchorNode = curAnchorNode->GetParentNode();
    }

    // Now relNodes the nodes of this path after the prefix
    // common to anchor and this path.
    SdfPath result = ReflexiveRelativePath();

    // Start by adding dotdots
    while (dotdotCount--) {
        result = result.GetParentPath();
    }

    // Now add nodes similar to relNodes to the ReflexiveRelativePath()
    // relNodes needs to be iterated in reverse since the closest ancestor
    // node was pushed on last.
    for (auto it = relNodes.rbegin(); it != relNodes.rend(); ++it) {
        result = _AppendNode(result, *it);
    }

    // Tack on any property part.
    result._propPart = _propPart;

    return result;
}

static inline bool _IsValidIdentifier(TfToken const &name)
{
    return TfIsValidIdentifier(name.GetString());
}

bool
SdfPath::IsValidIdentifier(const std::string &name)
{
    return TfIsValidIdentifier(name);
}

// We use our own _IsAlpha and _IsAlnum here for two reasons.  One, we want to
// ensure that they follow C/Python identifier rules and are not subject to
// various locale differences.  And two, since we are not consulting a locale,
// it is faster.
static constexpr bool _IsAlpha(int x) {
    return ('a' <= (x|32)) && ((x|32) <= 'z');
}
static constexpr bool _IsAlnum(int x) {
    return _IsAlpha(x) || (('0' <= x) && (x <= '9'));
}

bool
SdfPath::IsValidNamespacedIdentifier(const std::string &name)
{
    // A valid C/Python identifier except we also allow the namespace delimiter
    // and if we tokenize on that delimiter then all tokens are valid C/Python
    // identifiers.  That means following a delimiter there must be an '_' or
    // alphabetic character.
    constexpr char delim = SDF_PATH_NS_DELIMITER_CHAR;
    for (char const *p = name.c_str(); *p; ++p) {
        if (!_IsAlpha(*p) && *p != '_') {
            return false;
        }
        for (++p; _IsAlnum(*p) ||*p == '_'; ++p) {
            /* consume identifier */
        }
        if (*p != delim) {
            return !*p;
        }
    }
    return false;
}

std::vector<std::string>
SdfPath::TokenizeIdentifier(const std::string &name)
{
    std::vector<std::string> result;

    // This code currently assumes the namespace delimiter is one character.
    const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];

    std::string::const_iterator first = name.begin();
    std::string::const_iterator last = name.end();

    // Not empty and first character is alpha or '_'.
    if (first == last || !(isalpha(*first) || (*first == '_')))
        return result;
    // Last character is not the namespace delimiter.
    if (*(last - 1) == namespaceDelimiter)
        return result;

    // Count delimiters and reserve space in result.
    result.reserve(1 + std::count(first, last, namespaceDelimiter));

    std::string::const_iterator anchor = first;
    for (++first; first != last; ++first) {
        // Allow a namespace delimiter.
        if (*first == namespaceDelimiter) {
            // Record token.
            result.push_back(std::string(anchor, first));

            // Skip delimiter.  We know we will not go beyond the end of
            // the string because we checked before the loop that the
            // last character was not the delimiter.
            anchor = ++first;

            // First character.
            if (!(isalpha(*first) || (*first == '_'))) {
                TfReset(result);
                return result;
            }
        }
        else {
            // Next character 
            if (!(isalnum(*first) || (*first == '_'))) {
                TfReset(result);
                return result;
            }
        }
    }

    // Record the last token.
    result.push_back(std::string(anchor, first));

    return result;
}

TfTokenVector
SdfPath::TokenizeIdentifierAsTokens(const std::string &name)
{
    std::vector<std::string> tmp = TokenizeIdentifier(name);
    TfTokenVector result(tmp.size());
    for (size_t i = 0, n = tmp.size(); i != n; ++i) {
        TfToken(tmp[i]).Swap(result[i]);
    }
    return result;
}

std::string
SdfPath::JoinIdentifier(const std::vector<std::string> &names)
{
    if (std::any_of(names.begin(), names.end(), 
                    [](const std::string &s){return s.empty();})) 
    {
        // Create a new vector with just the non-empty names.
        std::vector<std::string> nonEmptyNames;
        nonEmptyNames.reserve(names.size());
        std::copy_if(names.begin(), names.end(), 
                     std::back_inserter(nonEmptyNames),
                     [](const std::string &s){return !s.empty();});
        return TfStringJoin(nonEmptyNames, 
                            SdfPathTokens->namespaceDelimiter.GetText());
    }
    return TfStringJoin(names, SdfPathTokens->namespaceDelimiter.GetText());
}

std::string
SdfPath::JoinIdentifier(const TfTokenVector& names)
{
    std::vector<std::string> tmp;
    tmp.reserve(names.size());
    for (size_t i = 0, n = names.size(); i != n; ++i) {
        if (!names[i].IsEmpty()) {
            tmp.push_back(names[i].GetString());
        }
    }
    return TfStringJoin(tmp, SdfPathTokens->namespaceDelimiter.GetText());
}

std::string
SdfPath::JoinIdentifier(const std::string &lhs, const std::string &rhs)
{
    if (lhs.empty()) {
        return rhs;
    }
    else if (rhs.empty()) {
        return lhs;
    }
    else {
        return lhs + SdfPathTokens->namespaceDelimiter.GetText() + rhs;
    }
}

std::string
SdfPath::JoinIdentifier(const TfToken &lhs, const TfToken &rhs)
{
    return JoinIdentifier(lhs.GetString(), rhs.GetString());
}

std::string
SdfPath::StripNamespace(const std::string &name)
{
    // This code currently assumes the namespace delimiter is one character.
    const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];
    const std::string::size_type n = name.rfind(namespaceDelimiter);
    return n == std::string::npos ? name : name.substr(n + 1);
}

TfToken
SdfPath::StripNamespace(const TfToken &name)
{
    return TfToken(StripNamespace(name.GetString()));
}

std::pair<std::string, bool> 
SdfPath::StripPrefixNamespace(const std::string &name, 
                              const std::string &matchNamespace)
{
    static const char namespaceDelimiter =
        SdfPathTokens->namespaceDelimiter.GetText()[0];

    if (matchNamespace.empty()) {
        return std::make_pair(name, false);
    }

    if (TfStringStartsWith(name, matchNamespace)) {

        size_t matchNamespaceLen = matchNamespace.size();

        // Now check to make sure the next character is the namespace delimiter
        if (matchNamespace[matchNamespaceLen - 1] == namespaceDelimiter) {

            // The matched namespace already contained the end delimiter,
            // nothing more to do.
            return std::make_pair(name.substr(matchNamespaceLen), true);

        } else {

            // The matched namespace needs an extra delimiter ':' so check for
            // it now.
            if (name[matchNamespaceLen] == namespaceDelimiter) {
                return std::make_pair(name.substr(matchNamespaceLen + 1), true);
            }

        }
    }

    return std::make_pair(name, false);
}

bool
SdfPath::IsValidPathString(const std::string &pathString,
                          std::string *errMsg)
{
    Sdf_PathParserContext context;

    // Initialize the scanner, allowing it to be reentrant.
    pathYylex_init(&context.scanner);

    yy_buffer_state *b =
        pathYy_scan_bytes(pathString.c_str(), pathString.size(), 
                          context.scanner);

    bool valid = (pathYyparse(&context) == 0);

    if (!valid && errMsg)
        *errMsg = context.errStr;

    // Clean up.
    pathYy_delete_buffer(b, context.scanner);
    pathYylex_destroy(context.scanner);

    return valid;
}

// Caller ensures both absolute or both relative.  We need to crawl up the
// longer path until both are the same length.  Then we crawl up both until we
// find the nodes whose parents match.  Then we can compare those nodes.
static inline bool
_LessThanCompareNodes(Sdf_PathNode const *l, Sdf_PathNode const *r)
{
    // Internally element counts are 'short', so this cast is safe.
    int lCount = static_cast<int>(l->GetElementCount());
    int rCount = static_cast<int>(r->GetElementCount());
    
    // Since caller ensures both absolute or both relative, then if either has
    // no elements, it's the root node.  l is less than r if l is the root and r
    // is not.
    if (!lCount || !rCount) {
        return !lCount && rCount;
    }

    int diff = rCount - lCount;

    // walk up to the same depth
    while (diff < 0) {
        l = l->GetParentNode();
        ++diff;
    }
    while (diff > 0) {
        r = r->GetParentNode();
        --diff;
    }
    
    // Now the cur nodes are at the same depth in the node tree
    if (l == r) {
        // They differ only in the tail.  If r has the tail, then this is
        // less, otherwise r is less.
        return lCount < rCount;
    }
    
    Sdf_PathNode const *lp = l->GetParentNode();
    Sdf_PathNode const *rp = r->GetParentNode();
    while (lp != rp) {
        l = lp, r = rp;
        lp = l->GetParentNode(), rp = r->GetParentNode();
    }
    
    // Now parents are equal, compare the current child nodes.
    return l->Compare<Sdf_PathNode::LessThan>(*r);
}

bool
SdfPath::_LessThanInternal(SdfPath const &lhs, SdfPath const &rhs)
{
    Sdf_PathNode const *lNode = lhs._primPart.get();
    Sdf_PathNode const *rNode = rhs._primPart.get();

    bool lIsAbs = lNode->IsAbsolutePath();
    bool rIsAbs = rNode->IsAbsolutePath();

    // Absolute paths are less than all relative paths.
    if (lIsAbs != rIsAbs) {
        return lIsAbs;
    }

    // If there is a difference in prim part, it's more significant than the
    // property part.
    if (ARCH_LIKELY(lNode != rNode)) {
        return _LessThanCompareNodes(lNode, rNode);
    }

    lNode = lhs._propPart.get(), rNode = rhs._propPart.get();
    if (!lNode || !rNode) {
        return !lNode;
    }
    return _LessThanCompareNodes(lNode, rNode);
}

std::ostream & operator<<( std::ostream &out, const SdfPath &path ) {
    return out << path.GetString();
}

SdfPathVector
SdfPath::GetConciseRelativePaths(const SdfPathVector& paths) {

    SdfPathVector primPaths;
    SdfPathVector anchors;
    SdfPathVector labels;

    // initialize the vectors
    TF_FOR_ALL(iter, paths) {

        if(!iter->IsAbsolutePath()) {
            TF_WARN("argument to GetConciseRelativePaths contains a relative path.");
            return paths;
        }

        // first, get the prim paths
        SdfPath primPath = iter->GetPrimPath();
        SdfPath anchor = primPath.GetParentPath();

        primPaths.push_back(primPath);
        anchors.push_back(anchor);

        // we have to special case root anchors, since MakeRelativePath can't handle them
        if(anchor == SdfPath::AbsoluteRootPath())
          labels.push_back(primPath);
        else
          labels.push_back(primPath.MakeRelativePath(anchor));

    }

    // each ambiguous path must be raised to its parent
    bool ambiguous;
    do {

        ambiguous = false;

        // the next iteration of labels
        SdfPathVector newAnchors;
        SdfPathVector newLabels;

        // find ambiguous labels
        for(size_t i=0;i<labels.size();++i) {

           int ok = true;

           // search for some other path that makes this one ambiguous
           for(size_t j=0;j<labels.size();++j) {
              if(i != j && labels[i] == labels[j] && primPaths[i] != primPaths[j]) {
                  ok = false;
                  break;
              }
           }

           if(!ok) {

               // walk the anchor up one node
               SdfPath newAnchor = anchors[i].GetParentPath();

               newAnchors.push_back(newAnchor);
               newLabels.push_back( newAnchor == SdfPath::AbsoluteRootPath() ? primPaths[i]
                                       : primPaths[i].MakeRelativePath( newAnchor ) );
               ambiguous = true;

           } else {
               newAnchors.push_back(anchors[i]);
               newLabels.push_back(labels[i]);
           }

        }

        anchors = newAnchors;
        labels = newLabels;

    } while(ambiguous);

    // generate the final set from the anchors
    SdfPathVector result;

    for(size_t i=0; i<anchors.size();++i) {

       if(anchors[i] == SdfPath::AbsoluteRootPath()) {
          result.push_back( paths[i] );
       } else {
          result.push_back( paths[i].MakeRelativePath( anchors[i] ));
       }

    }

    return result;

}

void
SdfPath::RemoveDescendentPaths(SdfPathVector *paths)
{
    // To remove descendents, first partition paths into prefix-related groups
    // via sort.
    std::sort(paths->begin(), paths->end());

    // Now unique and erase all descendents.  The equivalence predicate returns
    // true if rhs has lhs as a prefix.
    paths->erase(std::unique(paths->begin(), paths->end(),
                             [](SdfPath const &l, SdfPath const &r) {
                                 return r.HasPrefix(l);
                             }),
                 paths->end());
}

void
SdfPath::RemoveAncestorPaths(SdfPathVector *paths)
{
    // To remove ancestors, first parition paths into prefix-related groups
    // via sort.
    std::sort(paths->begin(), paths->end());

    // Now unique and erase ancestors.  The equivalence predicate returns true
    // if lhs has rhs as a prefix.
    paths->erase(paths->begin(),
                 std::unique(paths->rbegin(), paths->rend(),
                             [](SdfPath const &l, SdfPath const &r) {
                                 return l.HasPrefix(r);
                             }).base());
}

typename std::set<SdfPath>::const_iterator
SdfPathFindLongestPrefix(std::set<SdfPath> const &set, SdfPath const &path)
{
    return Sdf_PathFindLongestPrefixImpl<
        typename std::set<SdfPath>::const_iterator,
        std::set<SdfPath> const &>(set, path, /*strictPrefix=*/false);
}

typename std::set<SdfPath>::const_iterator
SdfPathFindLongestStrictPrefix(std::set<SdfPath> const &set,
                               SdfPath const &path)
{
    return Sdf_PathFindLongestPrefixImpl<
        typename std::set<SdfPath>::const_iterator,
        std::set<SdfPath> const &>(set, path, /*strictPrefix=*/true);
}

SdfPathAncestorsRange::iterator&
SdfPathAncestorsRange::iterator::operator++()
{
    if (!_path.IsEmpty()) {
        const Sdf_PathNode* propPart = nullptr;
        const Sdf_PathNode* primPart = nullptr;
        if (ARCH_UNLIKELY(_path._propPart)) {
            propPart = _path._propPart->GetParentNode();
            primPart = _path._primPart.get();
        } else if (_path._primPart && _path._primPart->GetElementCount() > 1) {
            primPart = _path._primPart->GetParentNode();
        }
        _path = SdfPath(primPart, propPart);
    }
    return *this;
}

SDF_API std::ptrdiff_t
distance(const SdfPathAncestorsRange::iterator& first,
         const SdfPathAncestorsRange::iterator& last)
{
    return (static_cast<std::ptrdiff_t>(first->GetPathElementCount()) -
            static_cast<std::ptrdiff_t>(last->GetPathElementCount()));
}


PXR_NAMESPACE_CLOSE_SCOPE
