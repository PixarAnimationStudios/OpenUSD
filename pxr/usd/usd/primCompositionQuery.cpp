//
// Copyright 2019 Pixar
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
#include "pxr/usd/usd/primCompositionQuery.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/pcp/layerStack.h"

PXR_NAMESPACE_OPEN_SCOPE

/////////////////////////////////////////////
// UsdPrimCompositionQueryArc
//

UsdPrimCompositionQueryArc::UsdPrimCompositionQueryArc(const PcpNodeRef &node) 
    : _node(node), _originalIntroducedNode(node)
{
    // Only the query itself can construct these, so we expect the node must be 
    // valid
    if (!TF_VERIFY(_node)) {
        return;
    }

    _originalIntroducedNode = _node;

    // The root node of introduces itself
    if (_node.IsRootNode()) {
        _introducingNode = _node;
        return;
    }

    // In most cases this node's arc originates from its parent node and this
    // node is the originally introduced node for the arc. But when this node
    // has a non-parent origin it must be an implicit or copied node that has
    // not been explicitly added by its parent node. In this case the root of 
    // the origin chain is is originally introduced node of the arc that causes
    // this node to exist and therefore that node's parent is the introducing 
    // node of this arc.
    if (_node.GetOriginNode() != _node.GetParentNode()) {
        _originalIntroducedNode = _node.GetOriginRootNode();
    } 
    _introducingNode = _originalIntroducedNode.GetParentNode();
}

PcpNodeRef 
UsdPrimCompositionQueryArc::GetTargetNode() const
{
    return _node;
}

PcpNodeRef 
UsdPrimCompositionQueryArc::GetIntroducingNode() const
{
    return _introducingNode;
}

// The Pcp list op field compose functions differ only by name and result 
// vector type
template <class ResultType>
using _PcpComposeFunc = void (*)(PcpLayerStackRefPtr const &,
                                 SdfPath const &, 
                                 std::vector<ResultType> *,
                                 PcpSourceArcInfoVector *);

// Helper for getting the corresponding list entry and arc source info from
// the composed list op of an arc introducing node for all list op types.
template <class ResultType>
static
bool
_GetIntroducingComposeInfo(const UsdPrimCompositionQueryArc &arc,
                           _PcpComposeFunc<ResultType> composeFunc, 
                           PcpSourceArcInfo *arcInfo,
                           ResultType *entry)
{
    // Run the Pcp compose func to get the parallel vectors of composed list
    // entries and arc source info.
    PcpSourceArcInfoVector info;
    std::vector<ResultType> result;
    composeFunc(arc.GetIntroducingNode().GetLayerStack(), 
                arc.GetIntroducingPrimPath(), 
                &result, &info);
    if (!TF_VERIFY(result.size() == info.size())) {
        return false;
    }

    // We can use the sibling num at origin to find exactly which entry in the
    // list corresponds to our arc's target node.
    const int index = arc.GetTargetNode().GetSiblingNumAtOrigin();
    if (static_cast<size_t>(index) >= info.size()) {
        TF_CODING_ERROR("Node sibling number of target node is out of range "
                        "introducing composed list op");
        return false;
    }

    if (arcInfo) {
        *arcInfo = info[index];
    }
    if (entry) {
        *entry = result[index];
    }
    return true;
}

SdfLayerHandle 
UsdPrimCompositionQueryArc::GetIntroducingLayer() const
{
    // The arc source info returned by the various Pcp compose functions for 
    // list op fields will hold the layer whose prim spec adds this arc to the
    // list. Just need to call the correct function for each arc type.
    PcpSourceArcInfo info;
    bool foundInfo = false;
    switch (_node.GetArcType()) {
    case PcpArcTypeReference:
        foundInfo = _GetIntroducingComposeInfo<SdfReference>(
            *this, &PcpComposeSiteReferences, &info, nullptr);
        break;
    case PcpArcTypePayload:
        foundInfo = _GetIntroducingComposeInfo<SdfPayload>(
            *this, &PcpComposeSitePayloads, &info, nullptr);
        break;
    case PcpArcTypeInherit:
        foundInfo = _GetIntroducingComposeInfo<SdfPath>(
            *this, &PcpComposeSiteInherits, &info, nullptr);
        break;
    case PcpArcTypeSpecialize:
        foundInfo = _GetIntroducingComposeInfo<SdfPath>(
            *this, &PcpComposeSiteSpecializes, &info, nullptr);
        break;
    case PcpArcTypeVariant:
        foundInfo = _GetIntroducingComposeInfo<std::string>(
            *this, &PcpComposeSiteVariantSets, &info, nullptr);
        break;
    default:
        break;
    }
    if (foundInfo) {
        return info.layer;
    }
    // Empty layer for root arc and unsupported arc types.
    return SdfLayerHandle();
}

SdfPath 
UsdPrimCompositionQueryArc::GetIntroducingPrimPath() const
{
    // Special case for the root node. It doesn't have an introducing prim path.
    if (_node.IsRootNode()) {
        return SdfPath();
    }
    // We ask the introduced node for its GetIntroPath which gets its parent's
    // path when it introduced this node. Note that cannot use the introducing
    // node's GetPathAtIntroduction as that would get the introducing node's
    // path when it itself was introduced by its own parent.
    return _originalIntroducedNode.GetIntroPath();
}

// Returns the introducing prim spec for the arc given the composed source
// arc info.
static
SdfPrimSpecHandle
_GetIntroducingPrimSpec(const UsdPrimCompositionQueryArc &arc, 
                        const PcpSourceArcInfo &info)
{
    return info.layer->GetPrimAtPath(arc.GetIntroducingPrimPath());
}

bool
UsdPrimCompositionQueryArc::GetIntroducingListEditor(
    SdfReferenceEditorProxy *editor, SdfReference *ref) const
{
    if (GetArcType() != PcpArcTypeReference) {
        TF_CODING_ERROR("Cannot retrieve a reference list editor and reference "
                        "for arc types other than PcpArcTypeReference");
        return false;
    }

    // Compose the references on the introducing node.
    PcpSourceArcInfo info;
    if (!_GetIntroducingComposeInfo<SdfReference>(
        *this, &PcpComposeSiteReferences, &info, ref)) {
        return false;
    }
    // Get the refence editor from the prim spec.
    *editor = _GetIntroducingPrimSpec(*this, info)->GetReferenceList();
    // The composed reference has its asset path and layer offset resolved.
    // We want the reference we return to be the authored value in the list op
    // itself which we can get back from the source arc info.
    ref->SetAssetPath(info.authoredAssetPath);
    ref->SetLayerOffset(info.layerOffset);
    return true;    
}

bool
UsdPrimCompositionQueryArc::GetIntroducingListEditor(
    SdfPayloadEditorProxy *editor, SdfPayload *payload) const
{
    if (GetArcType() != PcpArcTypePayload) {
        TF_CODING_ERROR("Cannot retrieve a payload list editor and payload "
                        "for arc types other than PcpArcTypePayload");
        return false;
    }

    // Compose the payloads on the introducing node.
    PcpSourceArcInfo info;
    if (!_GetIntroducingComposeInfo<SdfPayload>(
        *this, &PcpComposeSitePayloads, &info, payload)) {
        return false;
    }
    // Get the payload editor from the prim spec.
    *editor = _GetIntroducingPrimSpec(*this, info)->GetPayloadList();
    // The composed payload has its asset path and layer offset resolved.
    // We want the payload we return to be the authored value in the list op
    // itself which we can get back from the source arc info.
    payload->SetAssetPath(info.authoredAssetPath);
    payload->SetLayerOffset(info.layerOffset);
    return true;
}

bool
UsdPrimCompositionQueryArc::GetIntroducingListEditor(
    SdfPathEditorProxy *editor, SdfPath *path) const
{
    if (GetArcType() != PcpArcTypeInherit && 
        GetArcType() != PcpArcTypeSpecialize) {
        TF_CODING_ERROR("Cannot retrieve a path list editor and path "
                        "for arc types other than PcpArcTypeInherit and "
                        "PcpArcTypeSpecialize");
        return false;
    }

    PcpSourceArcInfo info;
    if (GetArcType() == PcpArcTypeInherit) {
        // Compose the inherit paths on the introducing node.
        if (!_GetIntroducingComposeInfo<SdfPath>(
            *this, &PcpComposeSiteInherits, &info, path)) {
            return false;
        }
        // Get the inherit path editor from the prim spec.
        *editor = _GetIntroducingPrimSpec(*this, info)->GetInheritPathList();
    } else {
        // Compose the specialize paths on the introducing node.
        if (!_GetIntroducingComposeInfo<SdfPath>(
            *this, &PcpComposeSiteSpecializes, &info, path)) {
            return false;
        }
        // Get the specialize path editor from the prim spec.
        *editor = _GetIntroducingPrimSpec(*this, info)->GetSpecializesList();
    }

    return true;
}

bool
UsdPrimCompositionQueryArc::GetIntroducingListEditor(
    SdfNameEditorProxy *editor, std::string *name) const
{
    if (GetArcType() != PcpArcTypeVariant) {
        TF_CODING_ERROR("Cannot retrieve a name list editor and name "
                        "for arc types other than PcpArcTypeVariant");
        return false;
    }

    // Compose the variant set names on the introducing node.
    PcpSourceArcInfo info;
    if (!_GetIntroducingComposeInfo<std::string>(
        *this, &PcpComposeSiteVariantSets, &info, name)) {
        return false;
    }
    // Get the variant set name editor from the prim spec.
    *editor = _GetIntroducingPrimSpec(*this, info)->GetVariantSetNameList();
    return true;
}


PcpArcType 
UsdPrimCompositionQueryArc::GetArcType() const
{
    return _node.GetArcType();
}

bool 
UsdPrimCompositionQueryArc::IsImplicit() const
{
    // An implicit node is a node that wasn't introduced by its parent and 
    // has a different site than its origin node. This is distinguished from
    // explicit nodes (which are introduced by their parents) and copied nodes
    // (which have been copied directly from their origins for strength
    // ordering)
    return !_node.IsRootNode() &&
        _node.GetParentNode() != _introducingNode && 
        _node.GetOriginNode().GetSite() != _node.GetSite();
}

bool 
UsdPrimCompositionQueryArc::IsAncestral() const
{
    return _node.IsDueToAncestor();
}

bool 
UsdPrimCompositionQueryArc::HasSpecs() const
{
    return _node.HasSpecs();
}

bool 
UsdPrimCompositionQueryArc::IsIntroducedInRootLayerStack() const
{
    // We say the root node of the graph is always introduced in the root layer
    // stack
    if (_node.IsRootNode()) {
        return true;
    }
    // We can't just compare the introducing layer stack with the root
    // node layer stack directly as a reference or payload that specifically
    // targets the root layer by name will have a layer stack that does not
    // contain a session layer. This means that its layer stack won't
    // necessarily exactly match the root node's layer stack which may have a
    // session layer. Thus we compare just the root layers of the stacks which 
    // is semantically what we're lookin for here.
    return _introducingNode.GetLayerStack()->GetIdentifier().rootLayer ==
         _node.GetRootNode().GetLayerStack()->GetIdentifier().rootLayer;
}

bool 
UsdPrimCompositionQueryArc::IsIntroducedInRootLayerPrimSpec() const
{
    return _introducingNode.IsRootNode();
}

/////////////////////////////////////////////
// UsdPrimCompositionQuery
//

UsdPrimCompositionQuery::UsdPrimCompositionQuery(const UsdPrim & prim, 
                                                 const Filter &filter) 
    : _prim(prim), _filter(filter)
{
    // We need the unculled prim index so that we can query all possible 
    // composition dependencies even if they don't currently contribute 
    // opinions.
    _expandedPrimIndex = _prim.ComputeExpandedPrimIndex();

    // Compute the unfiltered list of composition arcs from all non-inert nodes.
    // We still skip inert nodes in the unfiltered query so we don't pick up
    // things like the original copies of specialize nodes that have been
    // moved for strength ordering purposes. 
    for(const PcpNodeRef &node: _expandedPrimIndex.GetNodeRange()) { 
        if (!node.IsInert()) {
            _unfilteredArcs.push_back(UsdPrimCompositionQueryArc(node));
        }
    }
}

/*static*/
UsdPrimCompositionQuery UsdPrimCompositionQuery::GetDirectReferences(
    const UsdPrim & prim)
{
    Filter filter;
    filter.dependencyTypeFilter = DependencyTypeFilter::Direct;
    filter.arcTypeFilter = ArcTypeFilter::Reference;
    return UsdPrimCompositionQuery(prim, filter);
}

/*static*/
UsdPrimCompositionQuery UsdPrimCompositionQuery::GetDirectInherits(
    const UsdPrim & prim)
{
    Filter filter;
    filter.dependencyTypeFilter = DependencyTypeFilter::Direct;
    filter.arcTypeFilter = ArcTypeFilter::Inherit;
    return UsdPrimCompositionQuery(prim, filter);
}

/*static*/
UsdPrimCompositionQuery UsdPrimCompositionQuery::GetDirectRootLayerArcs(
    const UsdPrim & prim)
{
    Filter filter;
    filter.dependencyTypeFilter = DependencyTypeFilter::Direct;
    filter.arcIntroducedFilter = ArcIntroducedFilter::IntroducedInRootLayerStack;
    return UsdPrimCompositionQuery(prim, filter);
}

void UsdPrimCompositionQuery::SetFilter(const Filter &filter)
{
    _filter = filter;
}

UsdPrimCompositionQuery::Filter UsdPrimCompositionQuery::GetFilter() const
{
    return _filter;
}

static bool 
_TestArcType(const UsdPrimCompositionQueryArc &compArc, 
             const UsdPrimCompositionQuery::Filter &filter)
{
    using ArcTypeFilter = UsdPrimCompositionQuery::ArcTypeFilter;
 
    // Convert to a bit mask so we filter by multiple arc types.
    int arcMask = 0;
    switch (filter.arcTypeFilter) {
    case ArcTypeFilter::All:
        return true;
    case ArcTypeFilter::Reference: 
        arcMask = 1 << PcpArcTypeReference;
        break;
    case ArcTypeFilter::Payload:
        arcMask = 1 << PcpArcTypePayload;
        break;
    case ArcTypeFilter::Inherit:
        arcMask = 1 << PcpArcTypeInherit;
        break;
    case ArcTypeFilter::Specialize:
        arcMask = 1 << PcpArcTypeSpecialize;
        break;
    case ArcTypeFilter::Variant:
        arcMask = 1 << PcpArcTypeVariant;
        break;
    case ArcTypeFilter::ReferenceOrPayload:
        arcMask = (1 << PcpArcTypeReference) | (1 << PcpArcTypePayload);
        break;
    case ArcTypeFilter::InheritOrSpecialize:
        arcMask = (1 << PcpArcTypeInherit) | (1 << PcpArcTypeSpecialize);
        break;
    case ArcTypeFilter::NotReferenceOrPayload:
        arcMask = ~((1 << PcpArcTypeReference) | (1 << PcpArcTypePayload));
        break;
    case ArcTypeFilter::NotInheritOrSpecialize:
        arcMask = ~((1 << PcpArcTypeInherit) | (1 << PcpArcTypeSpecialize));
        break;
    case ArcTypeFilter::NotVariant:
        arcMask = ~(1 << PcpArcTypeVariant);
        break;
    }

    return arcMask & (1 << compArc.GetArcType());
}


static bool 
_TestDependencyType(const UsdPrimCompositionQueryArc &compArc, 
                    const UsdPrimCompositionQuery::Filter &filter)
{
    using DependencyTypeFilter = UsdPrimCompositionQuery::DependencyTypeFilter;

    switch (filter.dependencyTypeFilter) {
    case DependencyTypeFilter::All:
        return true;
    case DependencyTypeFilter::Direct:
        return !compArc.IsAncestral();
    case DependencyTypeFilter::Ancestral:
        return compArc.IsAncestral();
    };
    return true;
}

static bool 
_TestArcIntroduced(const UsdPrimCompositionQueryArc &compArc, 
                   const UsdPrimCompositionQuery::Filter &filter)
{
    using ArcIntroducedFilter = UsdPrimCompositionQuery::ArcIntroducedFilter;

    switch (filter.arcIntroducedFilter) {
    case ArcIntroducedFilter::All:
        return true;
    case ArcIntroducedFilter::IntroducedInRootLayerStack:
        return compArc.IsIntroducedInRootLayerStack();
    case ArcIntroducedFilter::IntroducedInRootLayerPrimSpec:
        return compArc.IsIntroducedInRootLayerPrimSpec();
    };
    return true;
}

static bool 
_TestHasSpecs(const UsdPrimCompositionQueryArc &compArc, 
              const UsdPrimCompositionQuery::Filter &filter)
{
    using HasSpecsFilter = UsdPrimCompositionQuery::HasSpecsFilter;

    switch (filter.hasSpecsFilter) {
    case HasSpecsFilter::All:
        return true;
    case HasSpecsFilter::HasSpecs:
        return compArc.HasSpecs();
    case HasSpecsFilter::HasNoSpecs:
        return !compArc.HasSpecs();
    };
    return true;
}

std::vector<UsdPrimCompositionQueryArc> 
UsdPrimCompositionQuery::GetCompositionArcs()
{
    // Create a list of the filter test functions we actually need to run; 
    // there's no point in testing filters that include all.
    using _TestFunc = 
        std::function<bool (const UsdPrimCompositionQueryArc &)> ;

    std::vector<_TestFunc> filterTests;
    if (_filter.arcTypeFilter != ArcTypeFilter::All) {
        filterTests.push_back(std::bind(_TestArcType, 
            std::placeholders::_1, _filter));
    }
    if (_filter.dependencyTypeFilter != DependencyTypeFilter::All) {
        filterTests.push_back(std::bind(_TestDependencyType, 
            std::placeholders::_1, _filter));
    }
    if (_filter.arcIntroducedFilter != ArcIntroducedFilter::All) {
        filterTests.push_back(std::bind(_TestArcIntroduced, 
            std::placeholders::_1, _filter));
    }
    if (_filter.hasSpecsFilter != HasSpecsFilter::All) {
        filterTests.push_back(std::bind(_TestHasSpecs, 
            std::placeholders::_1, _filter));
    }

    // No test, return unfiltered resuslts.
    if (filterTests.empty()) {
        return _unfilteredArcs;
    }

    // Runs the filter tests on an arc, failing in any test fails
    auto _RunFilterTests = 
        [&filterTests](const UsdPrimCompositionQueryArc &compArc)
        {
            for (auto test : filterTests) {
                if (!test(compArc)) {
                    return false;
                }
            }
            return true;
        };

    // Create the filtered arc list from the unfiltered arcs.
    std::vector<UsdPrimCompositionQueryArc> result;
    for (const UsdPrimCompositionQueryArc &compArc : _unfilteredArcs) {
        if (_RunFilterTests(compArc)) {
            result.push_back(compArc);
        }
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE

