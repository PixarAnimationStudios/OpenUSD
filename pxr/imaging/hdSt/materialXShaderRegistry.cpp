//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/js/json.h"

#include "pxr/imaging/hdSt/materialXShaderRegistry.h"
#include "pxr/imaging/hdSt/materialXShaderGen.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"

#include <MaterialXGenShader/Shader.h>

#include <fstream>

namespace mx = MaterialX;
namespace fs = std::filesystem;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDST_MTLX_CODEGEN_CACHE_DIR_PATH, "",
    "Path to the directory of the persistent MaterialX codegen cache");

namespace
{
    TF_DEFINE_PRIVATE_TOKENS(
        _tokens,

        // Extensions of cached files
        (glslfx)
        (json)

        // JSON fields per material
        (texture_parameters)
        (fallback_parameters)

        // JSON fields per material parameter
        (name)
        (type)
        (value)
    );

    // Construct a valid filesystem path from the setting string and create
    // the directory if necessary. If anything goes wrong, we print an error
    // message and return an empty path, which disables disk caching.
    //
    fs::path _ValidateCacheDirPath(const char* cacheDirPath)
    {
        HD_TRACE_FUNCTION();

        if (!cacheDirPath) {
            return {};
        }

        fs::path path = cacheDirPath;
        if (path.empty()) {
            return path;
        }

        if (fs::exists(path)) {
            if ( TF_VERIFY(fs::is_directory(path),
                "%s exists but is not a directory",
                path.string().c_str()) ) {

                return path;
            }
            else {
                return {};
            }
        }
        else {
            std::error_code ec;
            if (TF_VERIFY(fs::create_directory(path, ec),
                "Failed to create directory %s",
                path.string().c_str())) {

                return path;
            }
            else {
                return {};
            }
        }
    }

    std::string _GenerateCacheFilePath(
        fs::path const& cacheDirPath,
        HdInstanceKey   key,
        TfToken         extension)
    {
        std::ostringstream osFileName;
        osFileName << std::hex << key << '.' << extension;
        fs::path path = cacheDirPath / osFileName.str();

        return path.string();
    }

    using _DeserializeFunc = std::function<
        VtValue(                    // the return value
            std::istringstream&     // the input stream to deserialize
        )
    >;

    // These function objects implement deserialization of values of
    // particular data types. They're used in two cases:
    // 1. Importing values from MaterialX (the values are first serialized to
    //    strings with MaterialX methods).
    // 2. Deserializing values serialized to JSON files with `VtValue` 
    //    streaming methods as part of the persistent cache.
    //
    struct _Deserializer
    {
        bool                isVectorType;
        _DeserializeFunc    Deserialize;
    };

    // Deserializer implementations indexed by the USD type name. This is 
    // convenient because the types names are read from JSON files as-is.
    //
    using _DeserializerMap = std::unordered_map<std::string, _Deserializer>;
    static const _DeserializerMap _deserializerMap{
        {
            "bool",
            {
                false,
                [](std::istringstream& issValue) -> VtValue
                {
                    bool val = false;
                    issValue >> val;
                    return VtValue(val);
                }
            }
        },
        {
            "float",
            {
                false,
                [](std::istringstream& issValue) -> VtValue
                {
                    float val;
                    issValue >> val;
                    return VtValue(val);
                }
            }
        },
        {
            "GfVec2f",
            {
                true,
                [](std::istringstream& issValue) -> VtValue
                {
                    GfVec2f val;
                    std::string separator;
                    issValue >> val[0] >> separator >> val[1];
                    return VtValue(val);
                }
            }
        },
        {
            "GfVec3f",
            {
                true,
                [](std::istringstream& issValue) -> VtValue
                {
                    GfVec3f val;
                    std::string separator;
                    issValue >> val[0] >> separator >> val[1] >> separator
                        >> val[2];
                    return VtValue(val);
                }
            }
        },
        {
            "GfVec4f",
            {
                true,
                [](std::istringstream& issValue) -> VtValue
                {
                    GfVec4f val;
                    std::string separator;
                    issValue >> val[0] >> separator >> val[1] >> separator
                        >> val[2] >> separator >> val[3];
                    return VtValue(val);
                }
            }
        },
        {
            "int",
            {
                false,
                [](std::istringstream& issValue) -> VtValue
                {
                    int val;
                    issValue >> val;
                    return VtValue(val);
                }
            }
        },
        {
            "GfVec2i",
            {
                true,
                [](std::istringstream& issValue) -> VtValue
                {
                    GfVec2i val;
                    std::string separator;
                    issValue >> val[0] >> separator >> val[1];
                    return VtValue(val);
                }
            }
        },
        {
            "GfVec3i",
            {
                true,
                [](std::istringstream& issValue) -> VtValue
                {
                    GfVec3i val;
                    std::string separator;
                    issValue >> val[0] >> separator >> val[1] >> separator
                        >> val[2];
                    return VtValue(val);
                }
            }
        },
        {
            "GfVec4i",
            {
                true,
                [](std::istringstream& issValue) -> VtValue
                {
                    GfVec4i val;
                    std::string separator;
                    issValue >> val[0] >> separator >> val[1] >> separator
                        >> val[2] >> separator >> val[3];
                    return VtValue(val);
                }
            }
        }
    };
}

std::string const&
HdSt_MaterialXShaderRegistry::GetCacheDirPathEnvSetting()
{
    return TfGetEnvSetting(HDST_MTLX_CODEGEN_CACHE_DIR_PATH);
}

HdSt_MaterialXShaderRegistry::HdSt_MaterialXShaderRegistry(
    const char* cacheDirPath)
    : _cacheDirPath(_ValidateCacheDirPath(cacheDirPath))
{
}

void
HdSt_MaterialXShaderRegistry::SaveToDisk(
    HdInstanceKey                           key,
    HdSt_MaterialXCodegenResultPtr const&   value)
{
    if (!value || _cacheDirPath.empty()) {
        return;
    }

    value->SaveToDisk(_cacheDirPath, key);
}

HdSt_MaterialXCodegenResultPtr
HdSt_MaterialXShaderRegistry::LoadFromDisk(HdInstanceKey key)
{
    HD_TRACE_FUNCTION();
    std::ostringstream ossPixelShaderSource;

    {
        TRACE_FUNCTION_SCOPE("Load shader code")
        
        std::ifstream ifsGlslfx(_GenerateCacheFilePath(
            _cacheDirPath, key, _tokens->glslfx).c_str());

        if (!ifsGlslfx.is_open()) {
            return nullptr;
        }

        ossPixelShaderSource << ifsGlslfx.rdbuf();
    }

    JsValue jsMetadata;

    {
        TRACE_FUNCTION_SCOPE("Load metadata from JSON")

        std::string const& jsonFileName =
            _GenerateCacheFilePath(_cacheDirPath, key, _tokens->json);

        std::ifstream ifsJson(jsonFileName.c_str());

        if (!ifsJson.is_open()) {
            return nullptr;
        }

        jsMetadata = JsParseStream(ifsJson);
    }

    return std::make_shared<HdSt_MaterialXCodegenResult>(
        ossPixelShaderSource.str(), jsMetadata);
}

HdSt_MaterialXCodegenResult::HdSt_MaterialXCodegenResult(
    mx::Shader const& mxShader)
{
    HD_TRACE_FUNCTION();

    const mx::ShaderStage& pixelShaderStage =
        mxShader.getStage(mx::Stage::PIXEL);

    _pixelShaderSource = pixelShaderStage.getSourceCode();

    mx::VariableBlock const& uniformBlock =
        pixelShaderStage.getUniformBlock(mx::HW::PUBLIC_UNIFORMS);

    for (size_t iUniform = 0; iUniform < uniformBlock.size(); ++iUniform) {

        // MaterialX parameter Information
        const mx::ShaderPort* mxShaderPort = uniformBlock[iUniform];
        const mx::TypeDesc mxTypeDesc =
            HdStMaterialXHelpers::GetMxTypeDesc(mxShaderPort);

        const std::string& mxVarName = mxShaderPort->getVariable();

        // Create a corresponding HdSt_MaterialParam
        HdSt_MaterialParam param;
        param.paramType = HdSt_MaterialParam::ParamTypeFallback;
        param.name = TfToken(mxVarName);

        const char* fallbackParamType = nullptr;

        switch (mxTypeDesc.getBaseType())
        {
            case mx::TypeDesc::SEMANTIC_FILENAME:
                _textureParams.push_back(param.name);
                break;
            case mx::TypeDesc::BASETYPE_BOOLEAN:
                fallbackParamType = "bool";
                break;
            case mx::TypeDesc::BASETYPE_FLOAT: {
                switch (mxTypeDesc.getSize()) {
                    case 1:
                        fallbackParamType = "float";
                        break;
                    case 2:
                        fallbackParamType = "GfVec2f";
                        break;
                    case 3:
                        fallbackParamType = "GfVec3f";
                        break;
                    case 4:
                        fallbackParamType = "GfVec4f";
                        break;
                }
                break;
            }
            case mx::TypeDesc::BASETYPE_INTEGER: {
                switch (mxTypeDesc.getSize()) {
                    case 1:
                        fallbackParamType = "int";
                        break;
                    case 2:
                        fallbackParamType = "GfVec2i";
                        break;
                    case 3:
                        fallbackParamType = "GfVec3i";
                        break;
                    case 4:
                        fallbackParamType = "GfVec4i";
                        break;
                }
                break;
            }
        }

        if (fallbackParamType) {
            auto const& itDeserializer = _deserializerMap.find(fallbackParamType);
            if (TF_VERIFY(itDeserializer != _deserializerMap.end())) {

                const mx::ValuePtr mxVarValue = mxShaderPort->getValue();
                std::istringstream issValue(mxVarValue
                    ? mxVarValue->getValueString() : std::string());
                
                issValue.imbue(std::locale::classic());
                
                // MaterialX serializes booleans as "true" and "false",
                // while our cache serializes them as "1" and "0", in line with
                // the default C++ stream behavior and the logic hard-coded in 
                // `VtValue` serialization
                issValue >> std::boolalpha;

                param.fallbackValue =
                    itDeserializer->second.Deserialize(issValue);

                if (!param.fallbackValue.IsEmpty()) {
                    _fallbackParams.push_back(std::move(param));
                }
            }
        }
    }
}

HdSt_MaterialXCodegenResult::HdSt_MaterialXCodegenResult(
    std::string&&               pixelShaderSource,
    HdSt_MaterialParamVector&&  fallbackParams,
    TfTokenVector&&             textureParams
)
    : _pixelShaderSource(std::move(pixelShaderSource))
    , _fallbackParams(std::move(fallbackParams))
    , _textureParams(std::move(textureParams))
{
}

HdSt_MaterialXCodegenResult::HdSt_MaterialXCodegenResult(
    std::string&&   pixelShaderSource,
    JsValue const&  jsMetadata
)
    : _pixelShaderSource(std::move(pixelShaderSource))
{
    TRACE_FUNCTION_SCOPE("Deserialize metadata")

    JsObject jsMetadataObj = jsMetadata.GetJsObject();
    JsArray const& jsTextureParams =
        jsMetadataObj[_tokens->texture_parameters].GetJsArray();

    for (JsValue name : jsTextureParams) {
        _textureParams.push_back(TfToken(name.GetString()));
    }

    JsArray const& jsFallbackParamsArray =
        jsMetadataObj[_tokens->fallback_parameters].GetJsArray();

    for (JsValue const& jsFallbackParam : jsFallbackParamsArray) {
        JsObject jsFallbackParamObj = jsFallbackParam.GetJsObject();

        HdSt_MaterialParam param;

        param.name = TfToken(jsFallbackParamObj[_tokens->name].GetString());
        param.paramType = HdSt_MaterialParam::ParamTypeFallback;

        std::string const& valueTypeName =
            jsFallbackParamObj[_tokens->type].GetString();

        auto const& itDeserializer = _deserializerMap.find(valueTypeName);
        if (TF_VERIFY(itDeserializer != _deserializerMap.end())) {

            std::string const& strValue =
                jsFallbackParamObj[_tokens->value].GetString();

            std::istringstream issValue(strValue);
            issValue.imbue(std::locale::classic());

            if (itDeserializer->second.isVectorType) {
                // Skip the leading parenthesis
                issValue.seekg(1, std::ios::cur);
            }

            param.fallbackValue =
                itDeserializer->second.Deserialize(issValue);

            if (!param.fallbackValue.IsEmpty()) {
                _fallbackParams.push_back(std::move(param));
            }
        }
    }
}

void
HdSt_MaterialXCodegenResult::SaveMetadata(std::ostream& osFile) const
{
    HD_TRACE_FUNCTION();

    JsArray jsFallbackParams;
    jsFallbackParams.reserve(_fallbackParams.size());

    for (HdSt_MaterialParam const& param : _fallbackParams) {

        std::ostringstream osValue;
        osValue.imbue(std::locale::classic());
        osValue << param.fallbackValue;
        
        JsObject jsParam{
            { _tokens->name,    JsValue(param.name) },
            { _tokens->type,    JsValue(param.fallbackValue.GetTypeName()) },
            { _tokens->value,   JsValue(osValue.str()) } };

        jsFallbackParams.push_back(jsParam);
    }

    JsArray jsTextureParams;
    jsTextureParams.reserve(_textureParams.size());

    for (TfToken name : _textureParams) {
        jsTextureParams.emplace_back(name);
    }

    JsObject jsMetadataObj{
        { _tokens->fallback_parameters, JsValue(jsFallbackParams) },
        { _tokens->texture_parameters,  JsValue(jsTextureParams) } };

    JsWriteToStream(JsValue(jsMetadataObj), osFile);
}

void
HdSt_MaterialXCodegenResult::SaveToDisk(
    std::filesystem::path const&    cacheDirPath,
    HdInstanceKey                   key) const
{
    HD_TRACE_FUNCTION();

    {
        TRACE_FUNCTION_SCOPE("Load shader code")

        std::ofstream ofsGlslfx(
            _GenerateCacheFilePath(cacheDirPath, key, _tokens->glslfx).c_str() );

        if (ofsGlslfx.is_open()) {
            ofsGlslfx.write(
                _pixelShaderSource.c_str(), _pixelShaderSource.length());
        }
    }

    {
        std::ofstream ofsJson(
            _GenerateCacheFilePath(cacheDirPath, key, _tokens->json).c_str() );

        if (ofsJson.is_open()) {
            SaveMetadata(ofsJson);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
