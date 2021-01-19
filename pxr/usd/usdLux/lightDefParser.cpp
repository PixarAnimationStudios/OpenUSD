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
#include "pxr/usd/usdLux/lightDefParser.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/sdr/shaderNode.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    /* discoveryTypes */
    ((sourceType, "USD"))
    ((discoveryType, "usd-schema-gen"))
    ((context, "light"))
);

/*static*/
const TfToken &
UsdLux_LightDefParserPlugin::_GetSourceType() 
{
    return _tokens->sourceType;
}

/*static*/
const TfToken &
UsdLux_LightDefParserPlugin::_GetDiscoveryType()
{
    return _tokens->discoveryType;
}

static
NdrTokenMap
_GetSdrMetadata(const UsdShadeConnectableAPI &connectable,
                const NdrTokenMap &discoveryResultMetadata) 
{
    NdrTokenMap metadata = discoveryResultMetadata;

    metadata[SdrNodeMetadata->Help] = TfStringPrintf(
        "Fallback shader node generated from the USD %s schema",
        connectable.GetPrim().GetTypeName().GetText());

    metadata[SdrNodeMetadata->Primvars] = 
        UsdShadeShaderDefUtils::GetPrimvarNamesMetadataString(
            metadata, connectable);

    return metadata;
}

NdrNodeUniquePtr 
UsdLux_LightDefParserPlugin::Parse(
    const NdrNodeDiscoveryResult &discoveryResult)
{
    // This parser wants to pull all the shader properties from the schema
    // defined properties for light or light filter type. The simplest way to 
    // do this is to create a stage with a single prim of the type and use the
    // UsdShadeConnectableAPI to get all the inputs and outputs.

    // Note, that we create the layer and author the prim spec using the Sdf API
    // before opening a stage as opposed to creating a stage first and using 
    // the Usd DefinePrim API. This is to guard against any potential for this
    // parser being called indirectly within an SdfChangeBlock which can cause
    // DefinePrim to fail.
    static const SdfPath primPath("/Prim");
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usd");
    SdfPrimSpec::New(layer, primPath.GetName(),
                     SdfSpecifierDef, discoveryResult.identifier);
    UsdStageRefPtr stage = UsdStage::Open(layer, nullptr);
    if (!stage) {
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    UsdPrim prim = stage->GetPrimAtPath(primPath);
    if (!prim) {
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    UsdShadeConnectableAPI connectable(prim);
    if (!connectable) {
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    return NdrNodeUniquePtr(new SdrShaderNode(
        discoveryResult.identifier, 
        discoveryResult.version,
        discoveryResult.name,
        discoveryResult.family,
        _tokens->context, 
        discoveryResult.sourceType,
        /*nodeUriAssetPath=*/ std::string(), 
        /*resolvedImplementationUri=*/ std::string(),
        UsdShadeShaderDefUtils::GetShaderProperties(connectable),
        _GetSdrMetadata(connectable, discoveryResult.metadata),
        discoveryResult.sourceCode
    ));
}

const NdrTokenVec &
UsdLux_LightDefParserPlugin::GetDiscoveryTypes() const 
{
    static const NdrTokenVec discoveryTypes{_GetDiscoveryType()};
    return discoveryTypes;
}

const TfToken &
UsdLux_LightDefParserPlugin::GetSourceType() const
{
    return _GetSourceType();
}

NDR_REGISTER_PARSER_PLUGIN(UsdLux_LightDefParserPlugin);

PXR_NAMESPACE_CLOSE_SCOPE
