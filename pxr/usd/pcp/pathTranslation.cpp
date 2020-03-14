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
#include "pxr/usd/pcp/pathTranslation.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

template <bool NodeToRoot, class Mapping>
static SdfPath 
Pcp_TranslatePath(
    const Mapping &mapToRoot,
    const SdfPath& path,
    bool* pathWasTranslated)
{
    bool localPathWasTranslated;
    if (!pathWasTranslated) {
        pathWasTranslated = &localPathWasTranslated;
    }

    *pathWasTranslated = false;

    if (mapToRoot.IsNull()) {
        TF_CODING_ERROR("Null map function");
        return SdfPath();
    }

    // Special case the empty path.
    if (path.IsEmpty()) {
        *pathWasTranslated = true;
        return path;
    }

    if (!path.IsAbsolutePath()) {
        TF_CODING_ERROR("Path to translate <%s> is not absolute.",
                        path.GetText());
        return SdfPath();
    }
    if (path.ContainsPrimVariantSelection()) {
        TF_CODING_ERROR("Path to translate <%s> must not contain a variant "
                        "selection.", path.GetText());
        return SdfPath();
    }

    // Special case the identity mapping.
    if (mapToRoot.IsIdentity()) {
        *pathWasTranslated = true;
        return path;
    }

    // Translate the path and any target paths it contains from
    // the given node to the root or vice versa. If any of these paths 
    // cannot be translated (e.g., due to domain restrictions), the entire
    // translation fails and we return an empty path.
    SdfPath translatedPath = 
        NodeToRoot ? mapToRoot.MapSourceToTarget(path)
                   : mapToRoot.MapTargetToSource(path);
    if (translatedPath.IsEmpty()) {
        return SdfPath();
    }

    SdfPathVector targetPaths;
    translatedPath.GetAllTargetPathsRecursively(&targetPaths);

    TF_FOR_ALL(targetPathIt, targetPaths) {
        const SdfPath& targetPath = *targetPathIt;
        const SdfPath translatedTargetPath = 
            NodeToRoot ? mapToRoot.MapSourceToTarget(targetPath)
                       : mapToRoot.MapTargetToSource(targetPath);
        if (translatedTargetPath.IsEmpty()) {
            return SdfPath();
        }

        translatedPath = 
            translatedPath.ReplacePrefix(targetPath, translatedTargetPath);
    }

    *pathWasTranslated = true;
    return translatedPath;
}

SdfPath
PcpTranslatePathFromNodeToRoot(
    const PcpNodeRef& sourceNode, 
    const SdfPath& pathInNodeNamespace,
    bool* pathWasTranslated)
{
    TRACE_FUNCTION();

    // pathInNodeNamespace may contain a variant selection, but map functions
    // are about translations in scenegraph namespace and don't contain any
    // variant selections -- in fact, Pcp_TranslatePath throws a coding error
    // if you pass in a path with a variant selection. So, strip selections
    // out before proceeding.
    const SdfPath pathToTranslate = 
        pathInNodeNamespace.StripAllVariantSelections();

    return Pcp_TranslatePath</* NodeToRoot = */ true>(
        sourceNode.GetMapToRoot(), pathToTranslate, pathWasTranslated);
}

SdfPath
PcpTranslatePathFromRootToNode(
    const PcpNodeRef& destNode,
    const SdfPath& pathInRootNamespace,
    bool* pathWasTranslated)
{
    TRACE_FUNCTION();

    bool localPathWasTranslated = false;
    SdfPath translatedPath = Pcp_TranslatePath</* NodeToRoot = */ false>(
        destNode.GetMapToRoot(), pathInRootNamespace, &localPathWasTranslated);

    // We want to ensure that any variant selections that were made
    // between the root node and the destination node are applied to the
    // translated path. This won't occur in Pcp_TranslatePath, as map
    // functions never include variant selections. So, we do a simple
    // prefix replacement here to bring the variant selections back.
    // We don't need to fix target paths because they should never have
    // variant selections.
    if (localPathWasTranslated) {
        const SdfPath sitePath = destNode.GetPath();
        const SdfPath strippedSitePath = sitePath.StripAllVariantSelections();
        translatedPath = translatedPath.ReplacePrefix(
            strippedSitePath, sitePath, /* fixTargetPaths = */ false);
    }

    if (pathWasTranslated) {
        *pathWasTranslated = localPathWasTranslated;
    }

    return translatedPath;
}

SdfPath
PcpTranslateTargetPathFromRootToNode(
    const PcpNodeRef& destNode,
    const SdfPath& pathInRootNamespace,
    bool* pathWasTranslated)
{
    TRACE_FUNCTION();

    return Pcp_TranslatePath</* NodeToRoot = */ false>(
        destNode.GetMapToRoot(), pathInRootNamespace, pathWasTranslated);
}

SdfPath
PcpTranslatePathFromRootToNodeUsingFunction(
    const PcpMapFunction &mapToRoot,
    const SdfPath &pathInRootNamespace,
    bool *pathWasTranslated)
{
    TRACE_FUNCTION();
    return Pcp_TranslatePath</* NodeToRoot = */ false>(
        mapToRoot, pathInRootNamespace, pathWasTranslated);
}

SdfPath
PcpTranslatePathFromNodeToRootUsingFunction(
    const PcpMapFunction &mapToRoot,
    const SdfPath &pathInNodeNamespace,
    bool *pathWasTranslated)
{
    TRACE_FUNCTION();

    // pathInNodeNamespace may contain a variant selection, but map functions
    // are about translations in scenegraph namespace and don't contain any
    // variant selections -- in fact, Pcp_TranslatePath throws a coding error
    // if you pass in a path with a variant selection. So, strip selections
    // out before proceeding.
    const SdfPath pathToTranslate = 
        pathInNodeNamespace.StripAllVariantSelections();

    return Pcp_TranslatePath</* NodeToRoot = */ true>(
        mapToRoot, pathToTranslate, pathWasTranslated);
}

PXR_NAMESPACE_CLOSE_SCOPE
