//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/listEditImpl.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

// ------------------------------------------------------------------------- //
// UsdInherits
// ------------------------------------------------------------------------- //

using _ListEditImpl = 
    Usd_ListEditImpl<UsdInherits, SdfInheritsProxy>;

// The implementation doesn't define this function as it needs to be specialized
// so we implement it here.
template <>
SdfInheritsProxy 
_ListEditImpl::_GetListEditorForSpec(const SdfPrimSpecHandle &spec)
{
    return spec->GetInheritPathList();
}

bool
UsdInherits::AddInherit(const SdfPath &primPathIn, UsdListPosition position)
{
    return _ListEditImpl::Add(*this, primPathIn, position);
}

bool
UsdInherits::RemoveInherit(const SdfPath &primPathIn)
{
    return _ListEditImpl::Remove(*this, primPathIn);
}

bool
UsdInherits::ClearInherits()
{
    return _ListEditImpl::Clear(*this);
}

bool 
UsdInherits::SetInherits(const SdfPathVector& itemsIn)
{
    return _ListEditImpl::Set(*this, itemsIn);
}

SdfPathVector
UsdInherits::GetAllDirectInherits() const
{
    SdfPathVector ret;
    if (!_prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(_prim).c_str());
        return ret;
    }

    std::unordered_set<SdfPath, SdfPath::Hash> seen;

    auto addIfDirectInheritFn = [&](const PcpNodeRef &node) {
        if (node.GetArcType() == PcpArcTypeInherit &&
            node.GetLayerStack() == node.GetRootNode().GetLayerStack() &&
            !node.GetOriginRootNode().IsDueToAncestor() &&
            seen.insert(node.GetPath()).second) {
            ret.push_back(node.GetPath());
        }
    };

    // All class based arcs (inherits and specializes) get propagated up the 
    // prim index graph to the root node regardless of the where they're 
    // introduced. So we just have to look for the direct inherit nodes in the
    // subtrees started by inherit and specialize arcs under the root node.
    // Looking at only the propagated inherits has the advantage that these 
    // inherits are guaranteed to be correctly mapped across any references that
    // introduce them (which is important for local inherits)
    // 
    // When a specialized class inherits other classes (or vice versa),
    // those classes form a hierarchy and are propagated together. This means
    // that any inherit arcs introduced under a specializes arc will not break
    // the encapsulation of the class hierarchy and will not be found under the 
    // root's inherits arcs when the class hierarchy is introduces by a 
    // specializes. Thus, we have to search under both the root's inherits 
    // and its specializes to find all propagated inherit arcs.
    //
    // We search the expanded prim index to ensure that we pick up all possible
    // sources of opinions even if they currently do not produce specs. These
    // locations may be culled from the index returned by _prim.GetPrimIndex().
    PcpPrimIndex fullPrimIndex = _prim.ComputeExpandedPrimIndex();

    for (const PcpNodeRef &node: 
             fullPrimIndex.GetNodeRange(PcpRangeTypeInherit)) {
        addIfDirectInheritFn(node);
    }
    for (const PcpNodeRef &node:
             fullPrimIndex.GetNodeRange(PcpRangeTypeSpecialize)) {
        addIfDirectInheritFn(node);
    }
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE

