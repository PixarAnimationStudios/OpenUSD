//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/lightDefParser.h"

#include "pxr/usd/usdLux/boundableLightBase.h"
#include "pxr/usd/usdLux/nonboundableLightBase.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/sdf/copyUtils.h"
#include "pxr/usd/sdr/shaderNode.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/thisPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    /* discoveryTypes */
    ((sourceType, "USD"))
    ((discoveryType, "usd-schema-gen"))

    (MeshLight)
    (MeshLightAPI)
    (LightAPI)
    (ShadowAPI)
    (ShapingAPI)
    (VolumeLight)
    (VolumeLightAPI)
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

/*static*/
const UsdLux_LightDefParserPlugin::ShaderIdToAPITypeNameMap&
UsdLux_LightDefParserPlugin::_GetShaderIdToAPITypeNameMap() {
    static const UsdLux_LightDefParserPlugin::ShaderIdToAPITypeNameMap 
        shaderIdToAPITypeNameMap = {
        {_tokens->MeshLight, _tokens->MeshLightAPI},
        {_tokens->VolumeLight, _tokens->VolumeLightAPI}
    };
    return shaderIdToAPITypeNameMap;
}

static
SdrTokenMap
_GetSdrMetadata(const UsdShadeConnectableAPI &connectable,
                const SdrTokenMap &discoveryResultMetadata) 
{
    SdrTokenMap metadata = discoveryResultMetadata;

    metadata[SdrNodeMetadata->Help] = TfStringPrintf(
        "Fallback shader node generated from the USD %s schema",
        connectable.GetPrim().GetTypeName().GetText());

    metadata[SdrNodeMetadata->Primvars] = 
        UsdShadeShaderDefUtils::GetPrimvarNamesMetadataString(
            metadata, connectable);

    return metadata;
}

static SdfLayerRefPtr
_GetGeneratedSchema()
{
    // Get the generateSchema file for this plugin and open it as a layer.
    const std::string fname = 
        PLUG_THIS_PLUGIN->FindPluginResource("generatedSchema.usda", false);
    SdfLayerRefPtr layer = SdfLayer::OpenAsAnonymous(fname);
    return layer;
}

static bool
_CopyPropertiesFromSchema(
    const SdfLayerRefPtr &schemaLayer, const TfToken &schemaName,
    const SdfPrimSpecHandle &destPrimSpec)
{
    // The path of schema prim in the generated schema layer is its schema name.
    const SdfPath schemaPath = 
        SdfPath::AbsoluteRootPath().AppendChild(schemaName);
    SdfPrimSpecHandle schemaSpec = schemaLayer->GetPrimAtPath(schemaPath);
    if (!schemaSpec) {
        TF_CODING_ERROR("The generatedSchema for UsdLux does not have a prim "
                        "spec for schema type '%s'.",
                        schemaName.GetText());
        return false;
    }
    
    const SdfLayerHandle destLayer = destPrimSpec->GetLayer();
    const SdfPath destPath = destPrimSpec->GetPath();
    // Copy all the schema's properties to the destination.
    for (const SdfPropertySpecHandle &propSpec : schemaSpec->GetProperties()) {
        if (!SdfCopySpec(schemaLayer, propSpec->GetPath(),
                destLayer, destPath.AppendProperty(propSpec->GetNameToken()))) {
            TF_CODING_ERROR("Could not copy property spec '%s' from "
                            "generatedSchema for UsdLux schema '%s' to "
                            "destination layer.",
                            propSpec->GetPath().GetText(), 
                            schemaName.GetText());
            return false;
        }
    }
    return true;
}

SdrShaderNodeUniquePtr
UsdLux_LightDefParserPlugin::ParseShaderNode(
    const SdrShaderNodeDiscoveryResult &discoveryResult)
{
    TRACE_FUNCTION();

    const UsdLux_LightDefParserPlugin::ShaderIdToAPITypeNameMap
        &shaderIdToAPITypeNameMap = _GetShaderIdToAPITypeNameMap();

    // If discoveryResult identifier is a shaderId corresponding to one of the
    // API schemas for which we are generating sdr representation, then go and
    // fetch the name of the API schema which will then be used to extract
    // properties from the generatedSchema
    const TfToken &primTypeName = 
        (shaderIdToAPITypeNameMap.find(discoveryResult.identifier) == 
         shaderIdToAPITypeNameMap.end()) ? 
            discoveryResult.identifier : 
            shaderIdToAPITypeNameMap.at(discoveryResult.identifier);

    // This parser wants to pull all the shader properties from the schema
    // defined properties of the base UsdLux light type as well as the shader 
    // properties that can be included via applying the Shadow and Shaping APIs.
    // However, it does NOT want to pull in any shader properties that could 
    // possibly come in from other plugins that may define API schemas that 
    // would auto apply to any of these lights (or to the LightAPI itself).
    // 
    // Since the UsdSchemaRegistry doesn't keep track of what built-in API 
    // schemas a type's properties come from, we have to manually figure out the
    // relevant properties from the UsdLux library's generatedSchema layer and
    // compose them into a new prim that will represent the base light 
    // definition. This prim can then be opened on a stage in order to use the
    // UsdShadeConnectableAPI to get all the inputs and outputs.

    // Find and open the generated schema layer.
    SdfLayerRefPtr schemaLayer = _GetGeneratedSchema();
    if (!schemaLayer) {
        return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);
    }

    // Since we're composing the prim ourselves create a new layer and prim
    // spec where we'll add all the properties.
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usd");
    SdfPrimSpecHandle primSpec = SdfPrimSpec::New(
        layer, primTypeName, SdfSpecifierDef);

    // All of the UsdLux intrinsic lights will directly include LightAPI so it
    // will have all the properties from LightAPI as well as any it defines 
    // itself. We also need to include the ShadowAPI and ShapingAPI properties
    // as these can be optionally applied to any light. We copy the properties 
    // from each of the schema type prim specs over to the composed prim spec.
    // Note, that the order we copy is important as the light type itself may
    // have properties that override properties that come from the LightAPI. 
    const TfTokenVector schemas({
        _tokens->LightAPI, 
        primTypeName,
        _tokens->ShadowAPI, 
        _tokens->ShapingAPI});
    for (const TfToken &schemaName : schemas) {
        // It's important that we copy just the properties. Prim fields like 
        // the typeName, apiSchemas, and the property children can affect what 
        // properties are included when we open this prim on a USD stage.
        if (!_CopyPropertiesFromSchema(schemaLayer, schemaName, primSpec)) {
            return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);
        }
    }

    // Open a stage with the layer and get the new prim as a UsdConnectableAPI
    // which we'll create the node from.
    UsdStageRefPtr stage = UsdStage::Open(layer, nullptr);
    if (!stage) {
        return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);
    }
    UsdPrim prim = stage->GetPrimAtPath(primSpec->GetPath());
    if (!prim) {
        return SdrParserPlugin::GetInvalidShaderNode(discoveryResult);
    }
    // Note that we don't check the "conformance" of this prim to the 
    // connectable API because the prim is untyped and will not conform. But 
    // conformance isn't necessary for using UsdShadeConnectableAPI in order
    // to get input and output properties from a prim as is require in the
    // functions called below.
    UsdShadeConnectableAPI connectable(prim);

    return SdrShaderNodeUniquePtr(new SdrShaderNode(
        discoveryResult.identifier,
        discoveryResult.version,
        discoveryResult.name,
        discoveryResult.family,
        SdrNodeContext->Light,
        discoveryResult.sourceType,
        /*nodeUriAssetPath=*/ std::string(),
        /*resolvedImplementationUri=*/ std::string(),
        UsdShadeShaderDefUtils::GetProperties(connectable),
        _GetSdrMetadata(connectable, discoveryResult.metadata),
        discoveryResult.sourceCode
    ));
}

const SdrTokenVec &
UsdLux_LightDefParserPlugin::GetDiscoveryTypes() const 
{
    static const SdrTokenVec discoveryTypes{_GetDiscoveryType()};
    return discoveryTypes;
}

const TfToken &
UsdLux_LightDefParserPlugin::GetSourceType() const
{
    return _GetSourceType();
}

SDR_REGISTER_PARSER_PLUGIN(UsdLux_LightDefParserPlugin);

PXR_NAMESPACE_CLOSE_SCOPE
