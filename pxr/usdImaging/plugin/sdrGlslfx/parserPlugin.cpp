//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/plugin/sdrGlslfx/parserPlugin.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/imaging/hio/glslfx.h"

PXR_NAMESPACE_OPEN_SCOPE

NDR_REGISTER_PARSER_PLUGIN(SdrGlslfxParserPlugin);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Discovery and source type
    ((discoveryType, "glslfx"))
    ((sourceType, "glslfx"))
);

const NdrTokenVec& 
SdrGlslfxParserPlugin::GetDiscoveryTypes() const
{
    static const NdrTokenVec _DiscoveryTypes = {_tokens->discoveryType};
    return _DiscoveryTypes;
}

const TfToken& 
SdrGlslfxParserPlugin::GetSourceType() const
{
    return _tokens->sourceType;
}

static VtValue
ConvertToSdrCompatibleValueAndType(
    VtValue any, 
    size_t * arraySize, 
    TfToken * sdrType)
{
    // Unrecognized type by default
    *sdrType = SdrPropertyTypes->Unknown;

    // Not an array by default.
    *arraySize = 0;

    // XXX : Add support for the following sdr types:
    //       String, Struct, Terminal and Vstruct.
    // XXX : We could add some glslfx metadata to recognize if this GfVec3f
    //       is an Sdr type Vector, Color, Point or a Normal..
    if (any.IsHolding< std::vector<VtValue> >()) {

        std::vector<VtValue> const & anyVec = any.Get< std::vector<VtValue> >();

        // support for matrix
        if (anyVec.size() == 16) {
            if (anyVec[0].IsHolding<double>()) {
                GfMatrix4d retMat;
                double * m = retMat.GetArray();
                for(int i=0; i < 16; i++) {
                    m[i] = anyVec[i].UncheckedGet<double>();
                }
                *sdrType = SdrPropertyTypes->Matrix;
                return VtValue(retMat);
            }
            else if (anyVec[0].IsHolding<float>()) {
                GfMatrix4f retMat;
                float * m = retMat.GetArray();
                for(int i=0; i < 16; i++) {
                    m[i] = anyVec[i].UncheckedGet<float>();
                }
                *sdrType = SdrPropertyTypes->Matrix;
                return VtValue(retMat);
            } else {
                return VtValue();
            }
        
        // support for vectors length 1
        } else if (anyVec.size() == 1) {
            if (anyVec[0].IsHolding<double>()){
                // Sdr has no doubles, converting them to floats
                *sdrType = SdrPropertyTypes->Float; 
                return VtValue( (float)anyVec[0].UncheckedGet<double>());

            } else if (anyVec[0].IsHolding<float>()){
                *sdrType = SdrPropertyTypes->Float;
                return VtValue( anyVec[0].UncheckedGet<float>());
            }

        // support for vectors length 2
        } else if (anyVec.size() == 2) {
            VtFloatArray retVec(2);
            for (int i = 0; i < 2; i++) {
                if (anyVec[i].IsHolding<double>()) {
                    retVec[i] = anyVec[i].UncheckedGet<double>();
                } else if (anyVec[i].IsHolding<float>()) {
                    retVec[i] = anyVec[i].UncheckedGet<float>();
                } else {
                    return VtValue();
                }
            }

            *sdrType = SdrPropertyTypes->Float;
            *arraySize = 2;
            return VtValue(retVec);
        
        } else if (anyVec.size() == 3) {
            // support for vectors length 3
            GfVec3f retVec;
            for (int i = 0; i < 3; i++) {
                if (anyVec[i].IsHolding<double>()) {
                    retVec[i] = anyVec[i].UncheckedGet<double>();
                } else if (anyVec[i].IsHolding<float>()) {
                    retVec[i] = anyVec[i].UncheckedGet<float>();
                } else {
                    return VtValue();
                }
            }

            *sdrType = SdrPropertyTypes->Color;
            return VtValue(retVec);

        // support for vectors length 4
        } else if (anyVec.size() == 4) {
            VtFloatArray retVec(4);
            for (int i = 0; i < 4; i++) {
                if (anyVec[i].IsHolding<double>()) {
                    retVec[i] = anyVec[i].UncheckedGet<double>();
                } else if (anyVec[i].IsHolding<float>()) {
                    retVec[i] = anyVec[i].UncheckedGet<float>();
                } else {
                    return VtValue();
                }
            }

            *sdrType = SdrPropertyTypes->Float;
            *arraySize = 4;
            return VtValue(retVec);
        }

    } else if (any.IsHolding<double>()){
        // Sdr has no doubles, converting them to floats
        *sdrType = SdrPropertyTypes->Float; 
        return VtValue( (float)any.UncheckedGet<double>());

    } else if (any.IsHolding<float>()){
        *sdrType = SdrPropertyTypes->Float;
        return VtValue( any.UncheckedGet<float>());

    } else if (any.IsHolding<int>()){
        *sdrType = SdrPropertyTypes->Int;
        return VtValue( any.UncheckedGet<int>());

    } else if (any.IsHolding<bool>()){
        // Sdr has no bool, converting them to int
        *sdrType = SdrPropertyTypes->Int;
        return VtValue( (int)any.UncheckedGet<bool>());
    }

    return any;
}

NdrNodeUniquePtr
SdrGlslfxParserPlugin::Parse(const NdrNodeDiscoveryResult& discoveryResult)
{
    std::unique_ptr<HioGlslfx> glslfx;

    const TfToken& nodeIdentifier = discoveryResult.identifier;
    if (!discoveryResult.uri.empty()) {
        glslfx = std::make_unique<HioGlslfx>(discoveryResult.resolvedUri);

    } else if (!discoveryResult.sourceCode.empty()) {
        std::istringstream sourceCodeStream(discoveryResult.sourceCode);
        glslfx = std::make_unique<HioGlslfx>(sourceCodeStream);

    } else {
        TF_WARN("Invalid NdrNodeDiscoveryResult with identifier %s: both uri "
            "and sourceCode are empty.", nodeIdentifier.GetText());

        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    std::string errorString;
    if (!glslfx->IsValid(&errorString)){
        TF_WARN("Failed to parse glslfx at URI [%s] error [%s]",
            discoveryResult.uri.c_str(),
            errorString.c_str());
    }

    NdrPropertyUniquePtrVec nodeProperties;

    HioGlslfxConfig::Parameters params = glslfx->GetParameters();
    for (HioGlslfxConfig::Parameter const & p : params) {

        size_t arraySize = 0;
        TfToken sdrType;
        VtValue defaultValue = ConvertToSdrCompatibleValueAndType(
            p.defaultValue,
            &arraySize,
            &sdrType);

        NdrTokenMap hints;
        NdrOptionVec options;
        NdrTokenMap localMetadata;
        nodeProperties.push_back(
            std::make_unique<SdrShaderProperty>(
                TfToken(p.name),
                sdrType,
                defaultValue,
                false, 
                arraySize, 
                localMetadata, 
                hints, 
                options
            ));
    }

    HioGlslfxConfig::Textures textures = glslfx->GetTextures();
    for (HioGlslfxConfig::Texture const & t : textures) {

        size_t arraySize = 0;
        TfToken sdrType;
        VtValue defaultValue = ConvertToSdrCompatibleValueAndType(
            t.defaultValue,
            &arraySize,
            &sdrType);

        // Check for a default value, or fallback to all black.
        if (defaultValue.IsEmpty()) {
            sdrType = SdrPropertyTypes->Color;
            defaultValue = VtValue(GfVec3f(0.0,0.0,0.0));
        }

        NdrTokenMap hints;
        NdrOptionVec options;
        NdrTokenMap localMetadata;
        nodeProperties.push_back(
            std::make_unique<SdrShaderProperty>(
                TfToken(t.name),
                sdrType,
                defaultValue,
                false, 
                arraySize, 
                localMetadata, 
                hints, 
                options
            ));
    }

    NdrTokenMap metadata = discoveryResult.metadata;
    std::vector<std::string> primvarNames;
    if (metadata.count(SdrNodeMetadata->Primvars)) {
        primvarNames.push_back(metadata.at(SdrNodeMetadata->Primvars));
    }
    
    HioGlslfxConfig::Attributes attributes = glslfx->GetAttributes();
    for (HioGlslfxConfig::Attribute const & a : attributes) {
        primvarNames.push_back(a.name);
    }

    metadata[SdrNodeMetadata->Primvars] = TfStringJoin(primvarNames, "|");

    // XXX: Add support for reading metadata from glslfx and converting
    //      to node metadata

    return std::make_unique<SdrShaderNode>(
        nodeIdentifier,
        discoveryResult.version,
        discoveryResult.name,
        discoveryResult.family,
        _tokens->sourceType,
        _tokens->sourceType,
        discoveryResult.resolvedUri,
        discoveryResult.resolvedUri,
        std::move(nodeProperties),
        metadata,
        discoveryResult.sourceCode);
}

PXR_NAMESPACE_CLOSE_SCOPE
