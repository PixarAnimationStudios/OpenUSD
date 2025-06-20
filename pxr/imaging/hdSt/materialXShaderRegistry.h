//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIALX_SHADER_REGISTRY_H
#define PXR_IMAGING_HD_ST_MATERIALX_SHADER_REGISTRY_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/instanceRegistry.h"
#include "pxr/imaging/hdSt/materialParam.h"

#include <MaterialXCore/Library.h>
MATERIALX_NAMESPACE_BEGIN
class Shader;
MATERIALX_NAMESPACE_END

namespace mx = MaterialX;

#include <filesystem>

PXR_NAMESPACE_OPEN_SCOPE

class JsValue;

/// \class HdSt_MaterialXCodegenResult
///
/// Encapsulates all data extracted from a generated MaterialX shader object
/// that's necessary for shader compilation, binding and rendering in Storm.
/// Unlike a MaterialX shader, this object can be serialized to disk, as a 
/// GLSLFX source file and a JSON metadata file. This makes it possible to 
/// cache such objects in a persistent cache (see HdSt_MaterialXShaderRegistry
/// below) as an optimization.
/// 
class HdSt_MaterialXCodegenResult
{
public:
    /// Initialize from a MaterialX shader, which can now be destroyed.
    HDST_API
    explicit HdSt_MaterialXCodegenResult(mx::Shader const& mxShader);

    HDST_API
    HdSt_MaterialXCodegenResult(
        std::string&&               pixelShaderSource,
        HdSt_MaterialParamVector&&  fallbackParams,
        TfTokenVector&&             textureParams
    );

    HDST_API
    HdSt_MaterialXCodegenResult(
        std::string&&   pixelShaderSource,
        JsValue const&  jsMetadata
    );

    HDST_API
    std::string const& GetPixelShaderSource() const {
        return _pixelShaderSource;
    }

    HDST_API
    HdSt_MaterialParamVector const& GetFallbackParams() const {
        return _fallbackParams;
    }

    HDST_API
    TfTokenVector const& GetTextureParams() const {
        return _textureParams;
    }

    HDST_API
    void SaveToDisk(
        std::filesystem::path const& cacheDirPath,
        HdInstanceKey key) const;

    /// Public for unit tests' sake
    HDST_API
    void SaveMetadata(std::ostream& os) const;

private:
    std::string                 _pixelShaderSource;
    HdSt_MaterialParamVector    _fallbackParams;
    TfTokenVector               _textureParams;
};

using HdSt_MaterialXCodegenResultPtr =
    std::shared_ptr<class HdSt_MaterialXCodegenResult>;

/// \class HdSt_MaterialXShaderRegistry
///
/// A specialized instance registry which caches
/// HdSt_MaterialXCodegenResult instances in memory and, optionally, on disk
/// 
class HdSt_MaterialXShaderRegistry
    : public HdInstanceRegistryBase<
        HdSt_MaterialXCodegenResultPtr,
        HdSt_MaterialXShaderRegistry
    >
{
public:
    static std::string const& GetCacheDirPathEnvSetting();

    /// \param cacheDirPath the path to the cache directory. If NULL or empty
    /// then caching to disk is disabled.
    /// 
    explicit HdSt_MaterialXShaderRegistry(
        const char* cacheDirPath);

    /// Overrides the base method in a CRTP
    void SaveToDisk(
        HdInstanceKey                           key,
        HdSt_MaterialXCodegenResultPtr const&   value);

    /// Overrides the base method in a CRTP
    HdSt_MaterialXCodegenResultPtr LoadFromDisk(
        HdInstanceKey key);

private:
    std::filesystem::path const _cacheDirPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
