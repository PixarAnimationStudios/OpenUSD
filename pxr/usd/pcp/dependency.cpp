//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file Dependency.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/dependency.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/base/tf/enum.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(PcpDependencyTypeNone, "non-dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypeRoot, "root dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypePurelyDirect, "purely-direct dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypePartlyDirect, "partly-direct dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypeDirect, "direct dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypeAncestral, "ancestral dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypeVirtual, "virtual dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypeNonVirtual, "non-virtual dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypeAnyNonVirtual,
                     "any non-virtual dependency");
    TF_ADD_ENUM_NAME(PcpDependencyTypeAnyIncludingVirtual, "any dependency");
}

bool 
PcpNodeIntroducesDependency(const PcpNodeRef &node)
{
    if (node.IsInert()) {
        switch(node.GetArcType()) {
        case PcpArcTypeInherit:
        case PcpArcTypeSpecialize:
            // Special case: inert, propagated class-based arcs do not
            // represent dependencies.
            if (node.GetOriginNode() != node.GetParentNode()) {
                return false;
            }
            // Fall through
        default:
            break;
        }
    }
    return true;
}

PcpDependencyFlags
PcpClassifyNodeDependency(const PcpNodeRef &node)
{
    if (node.GetArcType() == PcpArcTypeRoot) {
        return PcpDependencyTypeRoot;
    }

    int flags = 0;
    
    // Inert nodes can represent virtual dependencies even though
    // they do not contribute the scene description at their site.
    //
    // Examples:
    // - relocates
    // - arcs whose target prims are (currently) private
    // - references/payloads without a prim or defaultPrim
    //
    // Tracking these dependencies is crucial for processing scene
    // edits in the presence of spooky ancestral opinions, and for
    // edits that resolve the condition causing the node to be inert,
    // such as permissions.
    //
    if (node.IsInert()) {
        if (!PcpNodeIntroducesDependency(node)) {
            return PcpDependencyTypeNone;
        }
        flags |= PcpDependencyTypeVirtual;
    }

    // Classify as ancestral or direct: if there is any non-ancestral
    // arc in the path to the root node, the node is considered a
    // direct dependency.
    bool anyDirect = false;
    bool anyAncestral = false;
    for (PcpNodeRef p = node; p.GetParentNode(); p = p.GetParentNode()) {
        if (p.IsDueToAncestor()) {
            anyAncestral = true;
        } else {
            anyDirect = true;
        }
        if (anyAncestral && anyDirect) {
            break;
        }
    }
    if (anyDirect) {
        if (anyAncestral) {
            flags |= PcpDependencyTypePartlyDirect;
        } else {
            flags |= PcpDependencyTypePurelyDirect;
        }
    } else {
        if (anyAncestral) {
            flags |= PcpDependencyTypeAncestral;
        }
    }

    if (!(flags & PcpDependencyTypeVirtual)) {
        flags |= PcpDependencyTypeNonVirtual;
    }

    return flags;
}

std::string
PcpDependencyFlagsToString(const PcpDependencyFlags depFlags)
{
    std::set<std::string> tags;
    if (depFlags == PcpDependencyTypeNone) {
        tags.insert("none");
    }
    if (depFlags == PcpDependencyTypeRoot) {
        tags.insert("root");
    }
    if (depFlags & PcpDependencyTypePurelyDirect) {
        tags.insert("purely-direct");
    }
    if (depFlags & PcpDependencyTypePartlyDirect) {
        tags.insert("partly-direct");
    }
    if (depFlags & PcpDependencyTypeAncestral) {
        tags.insert("ancestral");
    }
    if (depFlags & PcpDependencyTypeVirtual) {
        tags.insert("virtual");
    }
    if (depFlags & PcpDependencyTypeNonVirtual) {
        tags.insert("non-virtual");
    }
    return TfStringJoin(tags, ", ");
}

PXR_NAMESPACE_CLOSE_SCOPE
