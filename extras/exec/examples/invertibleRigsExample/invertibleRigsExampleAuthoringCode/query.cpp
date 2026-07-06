//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "query.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"

#include <tbb/concurrent_vector.h>

#include <set>

PXR_NAMESPACE_OPEN_SCOPE

InvertibleRigsExample_Query::InvertibleRigsExample_Query(
    const UsdStageRefPtr &stage)
    : _stage(stage)
{
}
        
// Traverses along connections to find the unique source attribute whose value
// flows to initialAttribute.
//
// If any attribute along the way has 0 or > 1 connections, the traversal is
// terminated.
//
static UsdAttribute
_FindUniqueSourceAvar(
    const UsdAttribute &initialAttribute)
{
    UsdAttribute attribute = initialAttribute;
    while(1) {
        SdfPathVector sources;
        if (!attribute.GetConnections(&sources) || sources.size() != 1) {
            return attribute;
        }

        UsdAttribute source =
            attribute.GetStage()->GetAttributeAtPath(sources[0]);
        if (!source) {
            return attribute;
        }

        attribute = source;
    }

    TF_CODING_ERROR(
        "Failed to find source avar for <%s>",
        initialAttribute.GetPath().GetText());
}

UsdAttributeVector
InvertibleRigsExample_Query::FindSwitchAvars(
    const TfToken &attributeName) const
{
    TF_DESCRIBE_SCOPE("Searching for source switch avars.");
    TRACE_FUNCTION();

    // TODO: ExecIr should publish tokens for the irRole and isInvertible
    // metadata keys and for valid roles.
    static const TfToken irRoleToken("irRole");
    static const TfToken switchAttributeRoleToken("SwitchAttribute");

    // Traverse the stage to find all switch attributes, based on IrRole
    // metadata, and follow connections to find switch avars that are unique
    // sources for the switch attributes.

    tbb::concurrent_vector<UsdAttribute> switchAvars;

    const auto range = _stage->GetPseudoRoot().GetDescendants();
    WorkParallelForEach(
        range.begin(), range.end(),
        [&attributeName, &switchAvars]
        (const UsdPrim &prim)
    {
        const UsdAttribute attr = prim.GetAttribute(attributeName);
        if (!attr) {
            return;
        }

        TfToken irRole;
        if (!attr.GetMetadata<TfToken>(irRoleToken, &irRole) ||
            irRole != switchAttributeRoleToken) {
            return;
        }

        switchAvars.push_back(_FindUniqueSourceAvar(attr));
    });

    std::set<UsdAttribute> unique(switchAvars.begin(), switchAvars.end());
    return UsdAttributeVector(unique.begin(), unique.end());
}

PXR_NAMESPACE_CLOSE_SCOPE
