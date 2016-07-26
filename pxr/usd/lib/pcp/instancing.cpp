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
#include "pxr/usd/pcp/instancing.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tracelite/trace.h"

TF_DEFINE_ENV_SETTING(
    PCP_OVERRIDE_INSTANCEABLE, -1,
    "Overrides Pcp's default computation for whether a PrimIndex is "
    "instanceable:\n"
    " -1: (the default) computes instanceable only in USD mode\n"
    "  0: NEVER computes instanceable (always returns false)\n"
    "  1: always compute instanceable, whether in USD mode or not.");

// Visitor to determine if a prim index has instanceable data.
// This essentially checks if a prim index had a direct composition arc
// (e.g. a reference or class) that could be shared with other prims.
struct Pcp_FindInstanceableDataVisitor
{
    Pcp_FindInstanceableDataVisitor() : hasInstanceableData(false) { }
    bool Visit(PcpNodeRef node, bool nodeIsInstanceable)
    {
        if (nodeIsInstanceable) {
            hasInstanceableData = true;
        }

        // We're just looking for instanceable data anywhere in the prim
        // index, so if we've found we can return false to cut off the
        // traversal.
        return not hasInstanceableData;
    }

    bool hasInstanceableData;
};

bool
Pcp_PrimIndexIsInstanceable(
    const PcpPrimIndex& primIndex)
{
    TRACE_FUNCTION();

    // For now, instancing functionality is limited to USD mode,
    // unless the special env var is set for testing.
    static const int instancing(TfGetEnvSetting(PCP_OVERRIDE_INSTANCEABLE));

    if ((instancing == 0) or
        ((not primIndex.IsUsd() and (instancing == -1)))) {
        return false;
    }

    // Check if this prim index introduced any instanceable data.
    // This is a cheap way of determining whether this prim index 
    // *could* be instanced without reading any scene description.
    //
    // Note that this means that a prim that is tagged with 
    // 'instanceable = true' will not be considered an instance if it does
    // not introduce instanceable data.
    Pcp_FindInstanceableDataVisitor visitor;
    Pcp_TraverseInstanceableStrongToWeak(primIndex, &visitor);
    if (not visitor.hasInstanceableData) {
        return false;
    }

    // Compose the value of the 'instanceable' metadata to see if this
    // prim has been tagged as instanceable.
    struct _Helper {
        static bool
        ComposeInstance(const PcpNodeRef& node, bool* isInstance)
        {
            static const TfToken instanceField = SdfFieldKeys->Instanceable;

            if (node.CanContributeSpecs()) {
                const PcpLayerStackSite& site = node.GetSite();
                TF_FOR_ALL(layer, site.layerStack->GetLayers()) {
                    if ((*layer)->HasField(
                            site.path, instanceField, isInstance)) {
                        return true;
                    }
                }
            }

            TF_FOR_ALL(child, Pcp_GetChildrenRange(node)) {
                if (ComposeInstance(*child, isInstance)) {
                    return true;
                }
            }

            return false;
        }
    };

    bool isInstance = false;
    _Helper::ComposeInstance(primIndex.GetRootNode(), &isInstance);
    return isInstance;
}
