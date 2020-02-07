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
#ifndef PXR_IMAGING_HGI_SHADERFUNCTION_H
#define PXR_IMAGING_HGI_SHADERFUNCTION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"

#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

struct HgiShaderFunctionDesc;


///
/// \class HgiShaderFunction
///
/// Represents one shader stage function (code snippet).
///
class HgiShaderFunction {
public:
    HGI_API
    virtual ~HgiShaderFunction();

    /// Returns false if any shader compile errors occured.
    HGI_API
    virtual bool IsValid() const = 0;

    /// Returns shader compile errors.
    HGI_API
    virtual std::string const& GetCompileErrors() = 0;

protected:
    HGI_API
    HgiShaderFunction(HgiShaderFunctionDesc const& desc);

private:
    HgiShaderFunction() = delete;
    HgiShaderFunction & operator=(const HgiShaderFunction&) = delete;
    HgiShaderFunction(const HgiShaderFunction&) = delete;
};

typedef HgiShaderFunction* HgiShaderFunctionHandle;
typedef std::vector<HgiShaderFunctionHandle> HgiShaderFunctionHandleVector;


/// \struct HgiShaderFunctionDesc
///
/// Describes the properties needed to create a GPU shader function.
///
/// <ul>
/// <li>shaderCode:
///   The ascii shader code.</li>
/// </ul>
///
struct HgiShaderFunctionDesc {
    HGI_API
    HgiShaderFunctionDesc();

    std::string debugName;
    HgiShaderStage shaderStage;
    std::string shaderCode;
};

HGI_API
inline bool operator==(
    const HgiShaderFunctionDesc& lhs,
    const HgiShaderFunctionDesc& rhs);

HGI_API
inline bool operator!=(
    const HgiShaderFunctionDesc& lhs,
    const HgiShaderFunctionDesc& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
