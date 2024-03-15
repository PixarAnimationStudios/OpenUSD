//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#include "pxr/usdImaging/plugin/hdUsdWriter/material.h"

#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/usdShade/material.h"

#include <iostream>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (out)
    (_terminals)
);
// clang-format on

namespace {

    template <class UsdShadeInputOrOutput>
    SdfValueTypeName
    GetInputOrOutputType(
        const UsdShadeShader& shader,
        const UsdShadeInputOrOutput& port,
        const TfToken& name,
        SdrRegistry& registry)
    {
        if (port)
        {
            return port.GetTypeName();
        }

        // If the input/output doesn't already exist, try looking it up in the shader registry
        TfToken shaderId;
        if (!shader.GetShaderId(&shaderId))
        {
            return SdfValueTypeName();
        }
        const auto registryNode = registry.GetShaderNodeByIdentifier(shaderId);
        if (!registryNode)
        {
            return SdfValueTypeName();
        }
        NdrPropertyConstPtr registryPort;
        if constexpr (std::is_same<UsdShadeInputOrOutput, UsdShadeInput>::value) {
            registryPort = registryNode->GetInput(name);
        } else if constexpr (std::is_same<UsdShadeInputOrOutput, UsdShadeOutput>::value) {
            registryPort = registryNode->GetOutput(name);
        } else {
            // Technically, we should be able to do "static_assert(false, ...)", but
            // gcc < 13 will then unconditionally error during compilation
            static_assert(
                std::is_same<UsdShadeInputOrOutput, UsdShadeInput>::value ||
                    std::is_same<UsdShadeInputOrOutput, UsdShadeOutput>::value,
                "GetInputOrOutputType<T>: T must be UsdShadeInput or UsdShadeOutput");
        }
        if (!registryPort)
        {
            return SdfValueTypeName();
        }
        return registryPort->GetTypeAsSdfType().first;
    }

    SdfValueTypeName
    GetSourceType(
        const UsdShadeShader& shader,
        const UsdShadeOutput& output,
        const TfToken& name,
        SdrRegistry& registry)
    {
        return GetInputOrOutputType(shader, output, name, registry);
    }

    SdfValueTypeName
    GetDestType(
        const UsdShadeShader& shader,
        const UsdShadeInput& input,
        const TfToken& name,
        SdrRegistry& registry)
    {
        return GetInputOrOutputType(shader, input, name, registry);
    }

}

// As a general rule we are sorting data stored in the materialNetworkMap where the order should not affect the outcome.
// This includes:
// - Terminals names.
// - Primvar names on terminals.
// - Nodes on material network maps.
void _WriteMaterial(UsdShadeMaterial& material,
    UsdStagePtr stage,
    bool createOverrideParent,
    HdMaterialNetworkMap& materialNetworkMap)
{
    // Save the list of terminals as a custom attribute
    if (!materialNetworkMap.terminals.empty())
    {
        std::sort(materialNetworkMap.terminals.begin(), materialNetworkMap.terminals.end());
        auto terminalsAttr = material.GetPrim().CreateAttribute(
            _tokens->_terminals, SdfValueTypeNames->StringArray, true, SdfVariability::SdfVariabilityUniform);
        VtArray<std::string> terminals;
        terminals.reserve(materialNetworkMap.terminals.size());
        for (const auto& terminal : materialNetworkMap.terminals)
        {
            terminals.push_back(terminal.GetAsString());
        }
        terminalsAttr.Set(terminals);
    }
    // std::map iterators already iterate through elements in a consistent, sorted, order.
    for (auto& it : materialNetworkMap.map)
    {
        const auto& terminal = it.first;
        auto& materialNetwork = it.second;
        // Save primvars to an attribute if they exist.
        if (!materialNetwork.primvars.empty())
        {
            // Sorting primvar names, because the order of primvar names should not affect the resulting usd file.
            std::sort(materialNetwork.primvars.begin(), materialNetwork.primvars.end());
            const auto& primvarsAttr =
                material.GetPrim().CreateAttribute(TfToken{ TfStringPrintf("%s:primvars", terminal.GetText()) },
                                                   SdfValueTypeNames->StringArray, SdfVariabilityUniform);
            // std::vector<TfToken> doesn't work with UsdAttribute::Set
            primvarsAttr.Set(VtStringArray{ materialNetwork.primvars.begin(), materialNetwork.primvars.end() });
        }

        auto& registry = SdrRegistry::GetInstance();
        // Used to find the node not used as a source in any connection, to connect to the terminal
        std::vector<SdfPath> possibleTerminalNodes;
        // Sorting nodes to have a consistent order.
        std::sort(materialNetwork.nodes.begin(), materialNetwork.nodes.end(),
                  [](const auto& a, const auto& b) { return a.path < b.path; });
        for (const auto& node : materialNetwork.nodes)
        {
            possibleTerminalNodes.push_back(node.path);
            // Although this is not part of any specification, but we expect node paths to be unique and
            // equal to a sensible absolute node path.
            auto shader = UsdShadeShader::Define(stage, HdUsdWriterGetFlattenPrototypePath(node.path));
            if (createOverrideParent)
            {
                // Restore override on parent of parent
                material.GetPrim().GetParent().SetSpecifier(SdfSpecifierOver);
            }
            shader.SetShaderId(node.identifier);
            auto shaderNode = registry.GetShaderNodeByIdentifier(node.identifier);
            for (const auto& parameter : node.parameters)
            {
                const auto inputType = [&parameter, &shaderNode]() -> auto
                {
                    if (shaderNode)
                    {
                        const auto shaderNodeInput = shaderNode->GetInput(parameter.first);
                        if (shaderNodeInput)
                        {
                            return shaderNodeInput->GetTypeAsSdfType().first;
                        }
                    }
                    return SdfGetValueTypeNameForValue(parameter.second);
                }
                ();
                auto input = shader.CreateInput(parameter.first, inputType);
                // TODO: For some reason the type returned for wrapS and wrapT of the preview shader is
                //  incorrect, std::string instead of TfToken.
                // TfToken parameters might come through as std::string, so we need to do some extra
                // conversions.
                if (inputType == SdfValueTypeNames->Token && !parameter.second.template IsHolding<TfToken>())
                {
                    if (parameter.second.template IsHolding<std::string>())
                    {
                        input.Set(TfToken{ parameter.second.template UncheckedGet<std::string>() });
                    }
                    else if (parameter.second.template IsHolding<SdfAssetPath>())
                    {
                        const auto& asset = parameter.second.template UncheckedGet<SdfAssetPath>();
                        if (asset.GetResolvedPath().empty())
                        {
                            input.Set(TfToken{ asset.GetAssetPath() });
                        }
                        else
                        {
                            input.Set(TfToken{ asset.GetResolvedPath() });
                        }
                    }
                }
                else if (inputType == SdfValueTypeNames->String && !parameter.second.template IsHolding<std::string>())
                {
                    if (parameter.second.template IsHolding<TfToken>())
                    {
                        input.Set(parameter.second.template UncheckedGet<TfToken>().GetString());
                    }
                    else if (parameter.second.template IsHolding<SdfAssetPath>())
                    {
                        const auto& asset = parameter.second.template UncheckedGet<SdfAssetPath>();
                        if (asset.GetResolvedPath().empty())
                        {
                            input.Set(asset.GetAssetPath());
                        }
                        else
                        {
                            input.Set(asset.GetResolvedPath());
                        }
                    }
                }
                else if (inputType == SdfValueTypeNames->Asset && !parameter.second.template IsHolding<SdfAssetPath>())
                {
                    if (parameter.second.template IsHolding<TfToken>())
                    {
                        const auto& token = parameter.second.template UncheckedGet<TfToken>();
                        input.Set(SdfAssetPath{ token.GetString(), token.GetString() });
                    }
                    else if (parameter.second.template IsHolding<std::string>())
                    {
                        const auto& string = parameter.second.template UncheckedGet<std::string>();
                        input.Set(SdfAssetPath{ string, string });
                    }
                }
                else
                {
                    input.Set(parameter.second);
                }
            }
        }
        // A note on Connection / Relationship naming
        // --------------------------------------------------------------------
        // The naming of "input" and "output" can be confusing, as it switches
        // depending on context - ie, whether we are looking from the point of
        // view of the relationship or the shader prim.
        //
        // From the point of view of the relationship:
        //    input -> output
        // From the point of view of the shader prims:
        //    output -> input
        //
        // Or, more graphically, representing a connection from
        // `primA.outputs::foo` to `primB.inputs::bar`:
        //
        //                    |<-- Relationship viewpoint ->|
        //                    [inputs]    >>>>>>    [outputs]
        //
        //        shaderA.outputs::foo    >>>>>>    shaderB.inputs::bar
        //
        // |<-- shaderA Viewpoint -->|              |<-- shaderB Viewpoint -->|
        // [inputs]          [outputs]    >>>>>>    [inputs]        [outputs]
        //
        // Thus, the API naming for methods shifts depending on which object we
        // are calling from, resulting in confusing bits like:
        //
        //       myShader.GetOutput(myRelationship.inputName)
        //
        // To help alleviate the confusion, for these names we instead use
        // "source" and "dest" - in all APIs, the "source" is the upstream
        // connection, so we always have:
        //
        //                    source     >>>>>>>     dest
        //
        for (const auto& relationship : materialNetwork.relationships)
        {
            // We remove any node that is used as a source
            possibleTerminalNodes.erase(
                std::remove(possibleTerminalNodes.begin(),
                            possibleTerminalNodes.end(),
                            relationship.inputId),
                possibleTerminalNodes.end());

            // Now try to make the USD shader connection corresponding the HdMaterialNetwork relationship
            auto sourceShader = GetPrimAtPath<UsdShadeShader>(stage, HdUsdWriterGetFlattenPrototypePath(relationship.inputId));
            if (!sourceShader)
            {
                continue;
            }
            auto destShader = GetPrimAtPath<UsdShadeShader>(stage, HdUsdWriterGetFlattenPrototypePath(relationship.outputId));
            if (!destShader)
            {
                continue;
            }

            auto source = sourceShader.GetOutput(relationship.inputName);
            auto dest = destShader.GetInput(relationship.outputName);

            if (!source || !dest) {
                SdfValueTypeName sourceType = GetSourceType(sourceShader, source, relationship.inputName, registry);
                SdfValueTypeName destType = GetDestType(destShader, dest, relationship.outputName, registry);

                if(!sourceType && !destType) {
                    // We don't know the type of either, just default to making them both TfToken
                    sourceType = destType = SdfValueTypeNames->Token;
                } else if (!sourceType) {
                    // Set the sourceType to match the (known) destType
                    sourceType = destType;
                } else if (!destType) {
                    // Set the destType to match the (known) sourceType
                    destType = sourceType;
                }

                if (!source) {
                    source = sourceShader.CreateOutput(relationship.inputName, sourceType);
                    if (!source) {
                        continue;
                    }
                }
                if (!dest) {
                    dest = destShader.CreateInput(relationship.outputName, destType);
                    if (!dest) {
                        continue;
                    }
                }
            }

            dest.ConnectToSource(source);
        }

        if (!possibleTerminalNodes.empty())
        {
            UsdShadeShader shader;
            // See if there's a node that we can create the output for
            for (const auto& node : possibleTerminalNodes)
            {
                auto shaderCandidate = GetPrimAtPath<UsdShadeShader>(stage, HdUsdWriterGetFlattenPrototypePath(node));
                TfToken shaderId;
                shaderCandidate.GetShaderId(&shaderId);
                auto shaderNode = registry.GetShaderNodeByIdentifier(shaderId);
                const auto& outputNames = shaderNode->GetOutputNames();
                if (!outputNames.empty())
                {
                    const auto outputName = std::find(outputNames.begin(), outputNames.end(), terminal) == outputNames.end() ?
                                            TfToken() :
                                            terminal;
                    if (outputName.IsEmpty())
                    {
                        continue;
                    }
                    const auto outputProperty = shaderNode->GetOutput(outputName);
                    if (outputProperty != nullptr)
                    {
                        shader = shaderCandidate;
                        break;
                    }
                }
            }

            // If we couldn't find a suitable node above, use the first node without any outgoing connections.
            if (!shader)
            {
                shader = GetPrimAtPath<UsdShadeShader>(stage, HdUsdWriterGetFlattenPrototypePath(possibleTerminalNodes.front()));
            }

            // Sometimes the identifier can hold unusual data, like when mdl source assets are used. So we fall
            // back to `token outputs:out` as the generic output.
            auto connectFallbackOutput = [&]()
            {
                const auto output = shader.CreateOutput(_tokens->out, SdfValueTypeNames->Token);
                if (!output)
                {
                    return;
                }
                material.CreateOutput(terminal, SdfValueTypeNames->Token).ConnectToSource(output);
            };
            // HdMaterialNetwork does not have a clear association between terminal nodes and their output
            // parameters, so we are making a few guesses. First, we check if there is an output parameter
            // that matches the name of the terminal we are dealing with (like how UsdPreviewSurface has
            // surface and displacement), otherwise we are using the first available output.
            TfToken shaderId;
            shader.GetShaderId(&shaderId);
            auto shaderNode = registry.GetShaderNodeByIdentifier(shaderId);
            if (shaderNode == nullptr)
            {
                connectFallbackOutput();
                continue;
            }
            const auto& outputNames = shaderNode->GetOutputNames();
            if (outputNames.empty())
            {
                connectFallbackOutput();
                continue;
            }
            const auto outputName = std::find(outputNames.begin(), outputNames.end(), terminal) == outputNames.end() ?
                                        outputNames.front() :
                                        terminal;
            const auto outputProperty = shaderNode->GetOutput(outputName);
            if (outputProperty == nullptr)
            {
                connectFallbackOutput();
                continue;
            }
            const auto outputType = outputProperty->GetTypeAsSdfType().first;
            const auto output = shader.CreateOutput(outputName, outputType);
            if (!output)
            {
                continue;
            }
            material.CreateOutput(terminal, outputType).ConnectToSource(output);
        }
    }
}

HdUsdWriterMaterial::HdUsdWriterMaterial(const SdfPath& id) : HdMaterial(id)
{
}

void HdUsdWriterMaterial::Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
{
    TF_UNUSED(renderParam);

    if (*dirtyBits & (HdMaterial::DirtyResource | HdMaterial::DirtyParams))
    {
        const auto value = sceneDelegate->GetMaterialResource(GetId());
        if (value.IsHolding<HdMaterialNetworkMap>())
        {
            _materialNetworkMap = value.UncheckedGet<HdMaterialNetworkMap>();
        }
    }

    *dirtyBits = HdMaterial::Clean;
}

HdDirtyBits HdUsdWriterMaterial::GetInitialDirtyBitsMask() const
{
    return HdMaterial::DirtyParams | HdMaterial::DirtyResource;
}

void HdUsdWriterMaterial::SerializeToUsd(const UsdStagePtr &stage)
{
    SdfPath orgId = GetId();
    SdfPath id = HdUsdWriterGetFlattenPrototypePath(orgId);
    bool createOverrideParent = (id != orgId);
    auto material = UsdShadeMaterial::Define(stage, id);
    if (createOverrideParent)
    {
        CreateParentOverride(stage, id);
    }
    HdUsdWriterPopOptional(_materialNetworkMap, [&material, &stage, createOverrideParent](auto& materialNetworkMap)
                      { _WriteMaterial(material, stage, createOverrideParent, materialNetworkMap); });
}

PXR_NAMESPACE_CLOSE_SCOPE
