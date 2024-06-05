//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/instanceKey.h"
#include "pxr/usd/pcp/composeSite.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/instancing.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

struct PcpInstanceKey::_Collector
{
    bool Visit(const PcpNodeRef& node, bool nodeIsInstanceable)
    {
        if (nodeIsInstanceable) {
            instancingArcs.push_back(_Arc(node));
            // We can stop immediately if we know there is no payload
            // arc in the node graph -- but otherwise we must continue,
            // since payload arcs can be optionally included, and
            // therefore affect instance sharing.
            if (!indexHasPayload) {
                return false;
            }
        }
        return true;
    }

    std::vector<_Arc> instancingArcs;
    bool indexHasPayload;
};

PcpInstanceKey::PcpInstanceKey()
    : _hash(0)
{
}

PcpInstanceKey::PcpInstanceKey(const PcpPrimIndex& primIndex)
    : _hash(0)
{
    TRACE_FUNCTION();

    // Instance keys only apply to instanceable prim indexes.
    if (!primIndex.IsInstanceable()) {
        return;
    }

    // Collect all composition arcs that contribute to the instance key.
    _Collector collector;
    collector.indexHasPayload = primIndex.HasAnyPayloads();
    Pcp_TraverseInstanceableStrongToWeak(primIndex, &collector);
    _arcs.swap(collector.instancingArcs);

    // Collect all authored variant selections in strong-to-weak order.
    SdfVariantSelectionMap variantSelection;
    for (const PcpNodeRef &node: primIndex.GetNodeRange()) {
        if (node.CanContributeSpecs()) {
            PcpComposeSiteVariantSelections(node, &variantSelection);
        }
    }
    _variantSelection.assign(variantSelection.begin(), variantSelection.end());
    _hash = TfHash::Combine(_hash, _arcs, _variantSelection);
}

bool 
PcpInstanceKey::operator==(const PcpInstanceKey& rhs) const
{
    return _variantSelection == rhs._variantSelection &&
        _arcs == rhs._arcs;
}

bool 
PcpInstanceKey::operator!=(const PcpInstanceKey& rhs) const
{
    return !(*this == rhs);
}

std::string 
PcpInstanceKey::GetString() const
{
    std::string s;
    s += "Arcs:\n";
    if (_arcs.empty()) {
        s += "  (none)\n";
    }
    else {
        for (const auto& arc : _arcs) {
            s += TfStringPrintf("  %s%s : %s\n",
                TfEnum::GetDisplayName(arc._arcType).c_str(),
                (arc._timeOffset.IsIdentity() ? 
                    "" :
                    TfStringPrintf(" (offset: %f scale: %f)",
                        arc._timeOffset.GetOffset(),
                        arc._timeOffset.GetScale()).c_str()),
                Pcp_FormatSite(arc._sourceSite).c_str());
        }
    }

    s += "Variant selections:\n";
    if (_variantSelection.empty()) {
        s += "  (none)";
    }
    else {
        for (const auto& vsel : _variantSelection) {
            s += TfStringPrintf("  %s = %s\n", 
                vsel.first.c_str(), vsel.second.c_str());
        }
        // Kill the last newline.
        s.erase(s.length() - 1);
    }

    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
