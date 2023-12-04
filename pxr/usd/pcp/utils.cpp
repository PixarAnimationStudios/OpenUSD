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
///
/// \file Pcp/Utils.cpp

#include "pxr/pxr.h"
#include "pxr/usd/pcp/utils.h"

#include "pxr/usd/pcp/expressionVariables.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/variableExpression.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

std::string
Pcp_EvaluateVariableExpression(
    const std::string& expression,
    const PcpExpressionVariables& expressionVars,
    const std::string& context,
    const SdfLayerHandle& sourceLayer,
    const SdfPath& sourcePath,
    std::unordered_set<std::string>* usedVariables,
    PcpErrorVector* errors)
{
    SdfVariableExpression::Result r =
        SdfVariableExpression(expression)
        .EvaluateTyped<std::string>(expressionVars.GetVariables());

    if (usedVariables) {
        usedVariables->insert(
            std::make_move_iterator(r.usedVariables.begin()),
            std::make_move_iterator(r.usedVariables.end()));
    }

    if (errors && !r.errors.empty()) {
        PcpErrorVariableExpressionErrorPtr varErr =
            PcpErrorVariableExpressionError::New();

        varErr->expression = expression;
        varErr->expressionError =
            TfStringJoin(r.errors.begin(), r.errors.end(), "; ");
        varErr->context = context;
        varErr->sourceLayer = sourceLayer;
        varErr->sourcePath = sourcePath;

        errors->push_back(std::move(varErr));
    }

    return r.value.IsHolding<std::string>() ?
        r.value.UncheckedGet<std::string>() : std::string();
}

std::string
Pcp_EvaluateVariableExpression(
    const std::string& expression,
    const PcpExpressionVariables& expressionVars)
{
    return Pcp_EvaluateVariableExpression(
        expression, expressionVars,
        std::string(), SdfLayerHandle(), SdfPath(), nullptr, nullptr);
}

bool
Pcp_IsVariableExpression(
    const std::string& str)
{
    return SdfVariableExpression::IsExpression(str);
}

static bool
_TargetIsSpecifiedInIdentifier(
    const std::string& identifier)
{
    std::string layerPath;
    SdfLayer::FileFormatArguments layerArgs;
    return SdfLayer::SplitIdentifier(identifier, &layerPath, &layerArgs)
        && layerArgs.find(SdfFileFormatTokens->TargetArg) != layerArgs.end();
}

SdfLayer::FileFormatArguments 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const std::string& target)
{
    SdfLayer::FileFormatArguments args;
    Pcp_GetArgumentsForFileFormatTarget(identifier, target, &args);
    return args;
}

void 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const std::string& target,
    SdfLayer::FileFormatArguments* args)
{
    if (!target.empty() && !_TargetIsSpecifiedInIdentifier(identifier)) {
        (*args)[SdfFileFormatTokens->TargetArg] = target;
    }
}

SdfLayer::FileFormatArguments 
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& target)
{
    if (target.empty()) {
        return {};
    }
    return {{ SdfFileFormatTokens->TargetArg, target }};
}

const SdfLayer::FileFormatArguments&
Pcp_GetArgumentsForFileFormatTarget(
    const std::string& identifier,
    const SdfLayer::FileFormatArguments* defaultArgs,
    SdfLayer::FileFormatArguments* localArgs)
{
    if (!_TargetIsSpecifiedInIdentifier(identifier)) {
        return *defaultArgs;
    }

    *localArgs = *defaultArgs;
    localArgs->erase(SdfFileFormatTokens->TargetArg);
    return *localArgs;
}

std::pair<PcpNodeRef, PcpNodeRef>
Pcp_FindStartingNodeOfClassHierarchy(const PcpNodeRef& n)
{
    TF_VERIFY(PcpIsClassBasedArc(n.GetArcType()));

    const int depth = n.GetDepthBelowIntroduction();
    PcpNodeRef instanceNode = n;
    PcpNodeRef classNode;

    while (PcpIsClassBasedArc(instanceNode.GetArcType())
           && instanceNode.GetDepthBelowIntroduction() == depth) {
        TF_VERIFY(instanceNode.GetParentNode());
        classNode = instanceNode;
        instanceNode = instanceNode.GetParentNode();
    }

    return std::make_pair(instanceNode, classNode);
}

std::pair<SdfPath, PcpNodeRef>
Pcp_TranslatePathFromNodeToRootOrClosestNode(
    const PcpNodeRef& node,
    const SdfPath& path)
{
    if (node.IsRootNode()) {
        // If the given node is already the root node, nothing to do.
        return std::make_pair(path, node);
    }

    // Start at the given node and path. We strip all variant selections
    // from the path because namespace mappings never include them.
    PcpNodeRef curNode = node;
    SdfPath curPath = path.StripAllVariantSelections();

    // First, try translating directly to the root node. If that fails,
    // walk up from the given node to the root node, translating at each
    // step until the translation fails.
    if (SdfPath pathInRootNode = node.GetMapToRoot().MapSourceToTarget(path); 
        !pathInRootNode.IsEmpty()) {
        curNode = node.GetRootNode();
        curPath = std::move(pathInRootNode);
    }
    else {
        while (!curNode.IsRootNode()) {
            SdfPath pathInParentNode = curNode.GetMapToParent()
                .MapSourceToTarget(curPath);
            if (pathInParentNode.IsEmpty()) {
                break;
            }

            curNode = curNode.GetParentNode();
            curPath = std::move(pathInParentNode);
        }
    }

    // If curNode's path contains a variant selection, do a prefix
    // replacement to apply that selection to the translated path.
    //
    // We don't check curNode.GetArcType() == PcpArcTypeVariant
    // because curNode may be the root node of a prim index that
    // is being recursively computed to pick up ancestral opinions.
    // In that case, curNode's "real" arc type once it's added to
    // main prim index being computed isn't available here.
    if (const SdfPath pathAtIntro = curNode.GetPathAtIntroduction();
        pathAtIntro.ContainsPrimVariantSelection()) {

        curPath = curPath.ReplacePrefix(
            pathAtIntro.StripAllVariantSelections(), pathAtIntro);
    }

    return std::make_pair(curPath, curNode);
}

PXR_NAMESPACE_CLOSE_SCOPE
