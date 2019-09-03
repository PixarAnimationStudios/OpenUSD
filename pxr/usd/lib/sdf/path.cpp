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

static VtValue
_CastFromSdfPathToTfToken(const VtValue &val)
{
    return VtValue(val.Get<SdfPath>().GetToken());
}

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

// Register with VtValue that SdfPaths can be cast to TfTokens.  The only
// reason we need this is because we need to cast AnimSplines that contain
// SdfPaths to ones that contain TfTokens, and we need that to succeed.  The
// only reason we need that, is in execution we can't use SdfPaths directly
// due to performance and threadsafety reasons.
TF_REGISTRY_FUNCTION(VtValue)
{
    VtValue::RegisterCast<SdfPath, TfToken>(&_CastFromSdfPathToTfToken);
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
    if (Sdf_PathNode const *propNode = _propPart.get()) {
        propNode = propNode->GetParentNode();
        return SdfPath(_primPart, Sdf_PathPropNodeHandle(propNode));
    }
    
    // This is a prim-like path.  If this prim path is '.' or ends with '..',
    // the "parent" path is made by appending a '..' component.
    //
    // XXX: NOTE that this is NOT the way that that Sdf_PathNode::GetParentNode
    // works, and note that most of the code in SdfPath uses GetParentNode
    // intentionally.
    Sdf_PathNode const *primNode = _primPart.get();
    if (ARCH_UNLIKELY(
            primNode == Sdf_PathNode::GetRelativeRootNode() || 
            primNode->GetName() == SdfPathTokens->parentPathElement)) {
        return SdfPath(Sdf_PathNode::FindOrCreatePrim(
                           primNode, SdfPathTokens->parentPathElement),
                       Sdf_PathPropNodeHandle());
    } else {
        return SdfPath(primNode->GetParentNode(), nullptr);
    }
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
                        GetString().c_str());
        return EmptyPath();
    }
    if (newSuffix.IsAbsolutePath()) {
        TF_WARN("Cannot append absolute path <%s> to another path <%s>.",
                newSuffix.GetString().c_str(), GetString().c_str());
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
                targetPath.GetString().c_str(), GetString().c_str());
        return EmptyPath();
    }
    if (targetPath == EmptyPath()) {
        TF_WARN("Cannot append an empty mapper target path to <%s>",
                GetString().c_str());
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
SdfPath::ReplacePrefix(const SdfPath &oldPrefix, const SdfPath &newPrefix,
                       bool fixTargetPaths) const
{
    // Perhaps surprisingly, this path need not have oldPrefix as a prefix.  For
    // example, '/a.rel[/target]'.ReplacePrefix('/target', '/other/target') ->
    // '/a.rel[/other/target]' when fixTargetPaths == true.

    TRACE_FUNCTION();

    if (oldPrefix == newPrefix) {
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

    size_t thisElemCount = GetPathElementCount();
    size_t oldPfxElemCount = oldPrefix.GetPathElementCount();

    // We only have target paths to fix if the property part contains target
    // paths to fix.
    fixTargetPaths =
        fixTargetPaths && _propPart && _propPart->ContainsTargetPath();

    if (thisElemCount == 0 ||
        (oldPfxElemCount >= thisElemCount && !fixTargetPaths)) {
        return *this;
    }

    // Get temporary node storage -- stack if small enough, heap otherwise.
    constexpr size_t MaxLocalNodes = 16;
    Sdf_PathNodeConstPtr localNodes[MaxLocalNodes];
    std::unique_ptr<Sdf_PathNodeConstPtr []> remoteNodes;
    Sdf_PathNodeConstPtr *tmpNodes = localNodes;

    // If we're fixing target paths, we may need to examine the entirety of the
    // property part of this path, not just the tail elements.
    size_t requiredTmpNodes;
    if (fixTargetPaths) {
        requiredTmpNodes = thisElemCount > oldPfxElemCount ?
            std::max(thisElemCount - oldPfxElemCount,
                     _propPart->GetElementCount()) :
            _propPart->GetElementCount();
    }
    else {
        requiredTmpNodes = thisElemCount - oldPfxElemCount;
    }
    if (requiredTmpNodes > MaxLocalNodes) {
        remoteNodes.reset(new Sdf_PathNodeConstPtr[requiredTmpNodes]);
        tmpNodes = remoteNodes.get();
    }
    // Now tmpNodes is our temporary node storage.

    // Walk up this path until we do not need to check anymore.  If we're not
    // fixing target paths, this is just until we hit the same elem count as
    // oldPrefix.  If we are doing target paths it's additionally until there
    // are no more target path elements to examine.
    size_t i = 0;
    tmpNodes[i++] = propNode ? propNode : primNode;
    size_t numTailNodes =
        thisElemCount > oldPfxElemCount ? thisElemCount - oldPfxElemCount : 0;
    bool foundOldPrefix = false;
    bool foundTargetPaths = fixTargetPaths;
    bool moreTargetPaths = foundTargetPaths;
    Sdf_PathNodeConstPtr oldPrefixPrimNode = oldPrefix._primPart.get();
    Sdf_PathNodeConstPtr oldPrefixPropNode = oldPrefix._propPart.get();
    bool inPrimPart = !propNode;
    while (numTailNodes || moreTargetPaths) {
        Sdf_PathNodeConstPtr tmp = tmpNodes[i-1]->GetParentNode();
        if (!tmp) {
            tmp = primNode;
            inPrimPart = true;
        }
        if (numTailNodes) {
            --numTailNodes;
            foundOldPrefix = (
                (inPrimPart && tmp == oldPrefixPrimNode) ||
                (primNode == oldPrefixPrimNode && tmp == oldPrefixPropNode));
            if (foundOldPrefix)
                break;
        }
        moreTargetPaths =
            moreTargetPaths && tmpNodes[i-1]->ContainsTargetPath();
        tmpNodes[i++] = tmp;
    }

    // Now tmpNodes[i-1]->GetParentNode() is either equal to oldPrefix or we
    // never hit oldPrefix and we're just fixing target paths above.

    // If we didn't find the old prefix and we're not fixing up embedded target
    // paths, then oldPrefix is not a prefix of this path so we just return this
    // path.
    if (!foundOldPrefix && (!fixTargetPaths || !foundTargetPaths)) {
        return *this;
    }

    --i;

    // Append the tail component.  Use _AppendNode() except in these cases:
    // - For prims and properties, we construct child nodes directly
    //   so as to not expand out ".." components and to avoid the cost
    //   of unnecessarily re-validating identifiers.
    // - For embedded target paths, translate the target path.

    SdfPath newPath;
    if (foundOldPrefix) {
        newPath = newPrefix;
    }
    else {
        Sdf_PathNodeConstPtr newStart = tmpNodes[i]->GetParentNode();
        if (!newStart) {
            // In this case, tmpNodes[i] was a root property node, so we know to
            // just start with our prim component.
            newPath._primPart = _primPart;
        }
        else {
            // Start with newStart, if it's prim-like, use it, otherwise use our
            // primPart and it as the property part.
            Sdf_PathNode::NodeType newNodeType = newStart->GetNodeType();
            if (newNodeType == Sdf_PathNode::RootNode ||
                newNodeType == Sdf_PathNode::PrimNode ||
                newNodeType == Sdf_PathNode::PrimVariantSelectionNode) {
                newPath._primPart = newStart;
            } else {
                newPath._primPart = _primPart;
                newPath._propPart = newStart;
            }
        }
    }

    do {
        switch (tmpNodes[i]->GetNodeType()) {
        case Sdf_PathNode::PrimNode:
            newPath._primPart = Sdf_PathNode::FindOrCreatePrim(
                newPath._primPart.get(), tmpNodes[i]->GetName());
            break;
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
    } while (i--);
    
    return newPath;
}

SdfPath
SdfPath::GetCommonPrefix(const SdfPath &path) const {

    if (path == SdfPath()) {
        TF_WARN("GetCommonPrefix(): invalid path.");
        return SdfPath();
    }

    SdfPath path1 = *this;
    SdfPath path2 = path;

    size_t count1 = path1.GetPathElementCount();
    size_t count2 = path2.GetPathElementCount();

    if (count1 > count2) {
        for (size_t i=0; i < (count1-count2); ++i) {
            path1 = path1.GetParentPath();
        }
    } else {
        for (size_t i=0; i < (count2-count1); ++i) {
            path2 = path2.GetParentPath();
        }
    }

    while (path1 != path2) {
        path1 = path1.GetParentPath();
        path2 = path2.GetParentPath();
    }

    return path1;
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

    if (anchor == SdfPath()) {
        TF_WARN("MakeAbsolutePath(): anchor is the empty path.");
        return SdfPath();
    }

    // Check that anchor is an absolute path
    if (!anchor.IsAbsolutePath()) {
        TF_WARN("MakeAbsolutePath() requires an absolute path as an argument.");
        return SdfPath();
    }

    // Check that anchor is a prim-like path
    if (!anchor.IsAbsoluteRootOrPrimPath() && 
        !anchor.IsPrimVariantSelectionPath()) {
        TF_WARN("MakeAbsolutePath() requires a prim path as an argument.");
        return SdfPath();
    }

    // If we're invalid, just return a copy of ourselves.
    if (IsEmpty())
        return *this;

    SdfPath result = *this;

    // If we're not already absolute, do our own path using anchor as the
    // relative base.
    if (!IsAbsolutePath()) {
        // This list winds up in reverse order to what one might at
        // first expect.
        vector<Sdf_PathNode const *> relNodes;

        Sdf_PathNode const *relRoot = Sdf_PathNode::GetRelativeRootNode();
        Sdf_PathNode const *curNode = _primPart.get();
        // Walk up looking for oldPrefix node.
        while (curNode) {
            if (curNode == relRoot) {
                break;
            }
            relNodes.push_back(curNode);
            curNode = curNode->GetParentNode();
        }
        if (!curNode) {
            // Didn't find relative root
            // should never get here since all relative paths should have a
            // relative root node
            // CODE_COVERAGE_OFF
            TF_CODING_ERROR("Didn't find relative root");
            return SdfPath();
            // CODE_COVERAGE_ON
        }

        result = anchor;

        // Got the list, now add nodes similar to relNodes to anchor
        // relNodes needs to be iterated in reverse since the closest ancestor
        // node was pushed on last.
        for (auto it = relNodes.rbegin(); it != relNodes.rend(); ++it) {
            result = _AppendNode(result, *it);
        }
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
                 anchor.GetString().c_str());
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

bool
SdfPath::_LessThanInternal(SdfPath const &lhs, SdfPath const &rhs)
{
    SdfPath const &absRoot = SdfPath::AbsoluteRootPath();

    Sdf_PathNode const *lNode = lhs._primPart.get();
    Sdf_PathNode const *rNode = rhs._primPart.get();

    if (lNode->IsAbsolutePath() != rNode->IsAbsolutePath()) {
        return lNode->IsAbsolutePath();
    } else if (lhs == absRoot) {
        return true;
    } else if (rhs == absRoot) {
        return false;
    }

    // Both absolute or both relative.  We need to crawl up the longer path
    // until both are the same length.  Then we crawl up both till we find the
    // nodes whose parents match.  Then we can compare those nodes.
    auto compareNodes = [](Sdf_PathNode const *l, Sdf_PathNode const *r) {
        size_t lCount = l->GetElementCount();
        size_t rCount = r->GetElementCount();

        // walk up to the same depth
        size_t upSteps = lCount > rCount ? lCount - rCount : 0;
        while (upSteps--) {
            l = l->GetParentNode();
        }
        upSteps = rCount > lCount ? rCount - lCount : 0;
        while (upSteps--) {
            r = r->GetParentNode();
        }
        
        // Now the cur nodes are at the same depth in the node tree
        if (l == r) {
            // They differ only in the tail.  If r has the tail, then this is
            // less, otherwise r is less.
            return lCount < rCount;
        }

        while (l->GetParentNode() != r->GetParentNode()) {
            l = l->GetParentNode(), r = r->GetParentNode();
        }

        // Now parents are equal, compare the current child nodes.
        return l->Compare<Sdf_PathNode::LessThan>(*r);
    };

    // If there is a difference in prim part, it's more significant than the
    // property part.
    if (ARCH_LIKELY(lNode != rNode)) {
        return compareNodes(lNode, rNode);
    }

    lNode = lhs._propPart.get(), rNode = rhs._propPart.get();
    if (!lNode || !rNode) {
        return !lNode;
    }
    return compareNodes(lNode, rNode);
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

PXR_NAMESPACE_CLOSE_SCOPE
