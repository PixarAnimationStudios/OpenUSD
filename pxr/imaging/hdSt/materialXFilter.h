//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MATERIALX_FILTER_H
#define PXR_IMAGING_HD_ST_MATERIALX_FILTER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hdSt/materialNetwork.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/usd/sdf/path.h"
#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenShader/Shader.h>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
struct HdSt_MaterialFilterTask;

using HdStResourceRegistrySharedPtr =
    std::shared_ptr<class HdStResourceRegistry>;

using HdSt_MaterialFilterTaskSharedPtr =
    std::shared_ptr<HdSt_MaterialFilterTask>;

/// Storing MaterialX-Hydra counterparts and other Hydra-specific information
struct HdSt_MxShaderGenInfo {
    HdSt_MxShaderGenInfo() 
        : textureNames(MaterialX::StringVec()),
          primvarMap(MaterialX::StringMap()), 
          primvarDefaultValueMap(MaterialX::StringMap()), 
          defaultTexcoordName("st"),
          materialTag(HdStMaterialTagTokens->defaultMaterialTag.GetString()),
          bindlessTexturesEnabled(false) {}
    MaterialX::StringVec textureNames;
    MaterialX::StringMap primvarMap;
    MaterialX::StringMap primvarDefaultValueMap;
    std::string defaultTexcoordName;
    std::string materialTag;
    bool bindlessTexturesEnabled;
};

/// Encapsulates the input data for MaterialX codegen for an individual shader.
///
class HdSt_MaterialXGeneratorTask final
{
public:
    /// Modifies the input filter task - stores mappings between original and
    /// anonymized Sdf paths.
    ///
    HdSt_MaterialXGeneratorTask(
        HdSt_MaterialFilterTaskSharedPtr filterTask,
        SdfPath const& materialPath,
        Hgi const& hgi);

    /// The hash that uniquely identifies the shader to be generated. This is
    /// based on the anonymized shader network.
    size_t GetShaderHash() const
    {
        return _shaderHash;
    }

    MaterialX::ShaderPtr Generate() const;

    HdSt_MaterialFilterTaskSharedPtr GetFilterTask() {
        return _filterTask;
    }

private:
    // To keep input data such as the material network alive for the duration
    // of the codegen process
    HdSt_MaterialFilterTaskSharedPtr _filterTask;

    HdMaterialNetwork2 _hdNetwork;
    SdfPath _terminalNodePath;

    std::string const& _strMaterialTag;

    size_t _shaderHash = 0;

    HdMaterialNode2 const& _terminalNode;
    SdfPath const _materialPath;

    TfToken const& _apiName;
    bool const _bindlessTexturesEnabled;
};

/// MaterialX Filter
/// Converts a MaterialX node to one with a generated MaterialX shader
HDST_API
MaterialX::ShaderPtr HdSt_ApplyMaterialXFilter(
    HdSt_MaterialFilterTaskSharedPtr filterTask,
    SdfPath const& materialPath,
    HdSt_MaterialParamVector* materialParams,
    HdStResourceRegistry *resourceRegistry);

/// Create a MaterialX shader codegen task object for parallel execution
std::unique_ptr<HdSt_MaterialXGeneratorTask>
HdSt_CreateMaterialXGeneratorTask(
    SdfPath const& materialPath,
    VtValue vtMat,
    Hgi const& hgi);

/// Generate the shader for the given MaterialX document
HDST_API
MaterialX::ShaderPtr HdSt_GenMaterialXShader(
    MaterialX::DocumentPtr const& mxDoc,
    MaterialX::DocumentPtr const& stdLibraries,
    MaterialX::FileSearchPath const& searchPath,
    HdSt_MxShaderGenInfo const& mxHdInfo=HdSt_MxShaderGenInfo(),
    TfToken const& apiName=TfToken());

PXR_NAMESPACE_CLOSE_SCOPE

#endif
