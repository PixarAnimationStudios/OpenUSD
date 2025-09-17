//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdr/parserPlugin.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    static TfToken _sourceType = TfToken("OSL");
    static SdrTokenVec _discoveryTypes = {TfToken("oso")};
}

class _SdrOslTestParserPlugin : public SdrParserPlugin
{
public:
    _SdrOslTestParserPlugin() {};
    ~_SdrOslTestParserPlugin() {};

    SdrShaderNodeUniquePtr ParseShaderNode(
        const SdrShaderNodeDiscoveryResult& discoveryResult) override
    {
        // Register some test properties
        SdrShaderPropertyUniquePtrVec properties;

        #define ADD_PROPERTY(type, suffix, arrayLen, value, metadata)   \
            properties.emplace_back(                                    \
                SdrShaderPropertyUniquePtr(                             \
                    new SdrShaderProperty(                              \
                        TfToken(#type #suffix "Property"),              \
                        SdrPropertyTypes->type,                         \
                        VtValue(value),                                 \
                        false,                                          \
                        arrayLen,                                       \
                        metadata,                                       \
                        {},                                             \
                        {})));

        SdrTokenMap arrayMetadatum = 
            {{SdrPropertyMetadata->IsDynamicArray, "true" }};

        ADD_PROPERTY(Int,      , 0, 0               , {})
        ADD_PROPERTY(String,   , 0, std::string()   , {})
        ADD_PROPERTY(Float,    , 0, 0.0f            , {})
        ADD_PROPERTY(Color,    , 0, GfVec3f(0.0f)   , {})
        ADD_PROPERTY(Point,    , 0, GfVec3f(0.0f)   , {})
        ADD_PROPERTY(Normal,   , 0, GfVec3f(0.0f)   , {})
        ADD_PROPERTY(Vector,   , 0, GfVec3f(0.0f)   , {})
        ADD_PROPERTY(Matrix,   , 0, GfMatrix4d(1.0) , {})
        ADD_PROPERTY(Struct,   , 0,                 , {})
        ADD_PROPERTY(Terminal, , 0,                 , {})
        ADD_PROPERTY(Vstruct,  , 0,                 , {})
        ADD_PROPERTY(Vstruct, _Array, 0,            , arrayMetadatum)

        // Force a float[] to act like a vstruct (e.g. multiMaterialIn)
        SdrTokenMap vstructMetadata = 
            {{SdrPropertyMetadata->IsDynamicArray, "true" },
             {SdrPropertyMetadata->Tag, "vstruct" }};
        ADD_PROPERTY(Float, _Vstruct, 0,            , vstructMetadata)

        // Add different specialized float array versions
        VtFloatArray v2 = {0.0f, 0.0f};
        ADD_PROPERTY(Float, _Vec2, 2, v2, {})
        VtFloatArray v3 = {0.0f, 0.0f, 0.0f};
        ADD_PROPERTY(Float, _Vec3, 3, v3, {})
        VtFloatArray v4 = {0.0f, 0.0f, 0.0f, 0.0f};
        ADD_PROPERTY(Float, _Vec4, 4, v4, {})

        // Add a String_Asset property
        SdrTokenMap assetMetadata =
            {{SdrPropertyMetadata->IsAssetIdentifier, std::string()}};
        ADD_PROPERTY(String, _Asset, 0, std::string(), assetMetadata)

        #undef ADD_PROPERTY

        return SdrShaderNodeUniquePtr(
            new SdrShaderNode(
                discoveryResult.identifier,
                discoveryResult.version,
                discoveryResult.name,
                discoveryResult.family,
                discoveryResult.sourceType,
                discoveryResult.sourceType,
                discoveryResult.resolvedUri,
                discoveryResult.resolvedUri,
                std::move(properties),
                discoveryResult.metadata
            )
        );
    }

    static const SdrTokenVec& DiscoveryTypes;
    static const TfToken& SourceType;

    const SdrTokenVec& GetDiscoveryTypes() const override {
        return _discoveryTypes;
    }

    const TfToken& GetSourceType() const override {
        return _sourceType;
    }
};

const SdrTokenVec& _SdrOslTestParserPlugin::DiscoveryTypes = _discoveryTypes;
const TfToken& _SdrOslTestParserPlugin::SourceType = _sourceType;

SDR_REGISTER_PARSER_PLUGIN(_SdrOslTestParserPlugin)

PXR_NAMESPACE_CLOSE_SCOPE
