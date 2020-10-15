//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hdSt/materialXFilter.h"

#include "pxr/usd/sdr/registry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hio/glslfx.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (mtlx)
);

static std::string const
_GetTempShader()
{
    return std::string(
        "-- glslfx version 0.1 \n"
        "-- configuration \n"
        "{\n"
            "\"techniques\": {\n"
            "    \"default\": {\n"
            "        \"surfaceShader\": {\n"
            "            \"source\": [ \"test.Surface\" ]\n"
            "        }\n"
            "    }\n"
            "}\n\n"
        "}\n"

        "-- glsl test.Surface \n\n"
        "vec4 \n"
        "surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord) {\n"
        "   return vec4(FallbackLighting(Peye.xyz, Neye, vec3(1.0, 0.0, 0.0)), 1.0);\n"
        "};\n\n"
    );
}

void
HdSt_ApplyMaterialXFilter(HdMaterialNetwork2 * hdNetwork)
{
    // Get the Surface Terminal
    auto const& terminalIt = 
        hdNetwork->terminals.find(HdMaterialTerminalTokens->surface);
    if (terminalIt == hdNetwork->terminals.end()) {
        return;
    }
    HdMaterialConnection2 const& connection = terminalIt->second;
    SdfPath const& terminalPath = connection.upstreamNode;
    auto const& terminalNodeIt = hdNetwork->nodes.find(terminalPath);
    HdMaterialNode2 terminalNode = terminalNodeIt->second;

    // Check if the Terminal is a MaterialX Node
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxEntry = 
        sdrRegistry.GetShaderNodeByIdentifierAndType(terminalNode.nodeTypeId, 
                                                     _tokens->mtlx);

    // XXX If found a MaterialX terminal node color everything red.
    if (mtlxEntry) {

        // Create a new terminal node with the Temp Shader
        SdrShaderNodeConstPtr sdrNode = 
            sdrRegistry.GetShaderNodeFromSourceCode(_GetTempShader(), 
                                                    HioGlslfxTokens->glslfx,
                                                    NdrTokenMap()); // metadata
        HdMaterialNode2 newTerminalNode;
        newTerminalNode.nodeTypeId = sdrNode->GetIdentifier();
        newTerminalNode.inputConnections = terminalNode.inputConnections;
        
        // Replace the original terminalNode with this newTerminalNode
        hdNetwork->nodes.erase(terminalNodeIt);
        hdNetwork->nodes.insert({terminalPath, newTerminalNode});
    }
}

PXR_NAMESPACE_CLOSE_SCOPE