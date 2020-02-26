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
#ifndef PXR_IMAGING_HGI_SHADERPROGRAM_H
#define PXR_IMAGING_HGI_SHADERPROGRAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgi/types.h"

#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiShaderProgramDesc
///
/// Describes the properties needed to create a GPU shader program.
///
/// <ul>
/// <li>shaderFunctions:
///   Holds handles to shader functions for each shader stage.</li>
/// </ul>
///
struct HgiShaderProgramDesc
{
    HGI_API
    HgiShaderProgramDesc();

    std::string debugName;
    HgiShaderFunctionHandleVector shaderFunctions;
};

HGI_API
inline bool operator==(
    const HgiShaderProgramDesc& lhs,
    const HgiShaderProgramDesc& rhs);

HGI_API
inline bool operator!=(
    const HgiShaderProgramDesc& lhs,
    const HgiShaderProgramDesc& rhs);


///
/// \class HgiShaderProgram
///
/// Represents a collection of shader functions.
/// This object does not take ownership of the shader functions and does not
/// destroy them automatically. The client must destroy the shader functions
/// when the program is detroyed, because only the client knows if the shader
/// functions are used by other shader programs.
///
class HgiShaderProgram
{
public:
    HGI_API
    virtual ~HgiShaderProgram();

    /// The descriptor describes the object.
    HGI_API
    HgiShaderProgramDesc const& GetDescriptor() const;

    /// Returns false if any shader compile errors occured.
    HGI_API
    virtual bool IsValid() const = 0;

    /// Returns shader compile errors.
    HGI_API
    virtual std::string const& GetCompileErrors() = 0;

    /// Returns the shader functions that are part of this program.
    HGI_API
    virtual HgiShaderFunctionHandleVector const& GetShaderFunctions() const = 0;


protected:
    HGI_API
    HgiShaderProgram(HgiShaderProgramDesc const& desc);

    HgiShaderProgramDesc _descriptor;

private:
    HgiShaderProgram() = delete;
    HgiShaderProgram & operator=(const HgiShaderProgram&) = delete;
    HgiShaderProgram(const HgiShaderProgram&) = delete;
};

/// Explicitly instantiate and define ShaderProgram handle
template class HgiHandle<class HgiShaderProgram>;
typedef HgiHandle<class HgiShaderProgram> HgiShaderProgramHandle;
typedef std::vector<HgiShaderProgramHandle> HgiShaderProgramHandleVector;


PXR_NAMESPACE_CLOSE_SCOPE

#endif
