//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_SHADERFUNCTION_H
#define PXR_IMAGING_HGI_SHADERFUNCTION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgi/shaderFunctionDesc.h"
#include "pxr/imaging/hgi/types.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiShaderFunction
///
/// Represents one shader stage function (code snippet).
///
/// ShaderFunctions are usually passed to a ShaderProgram, however be careful
/// not to destroy the ShaderFunction after giving it to the program.
/// While this may be safe for OpenGL after the program is created, it does not
/// apply to other graphics backends, such as Vulkan, where the shader functions
/// are used during rendering.
///
class HgiShaderFunction
{
public:
    HGI_API
    virtual ~HgiShaderFunction();

    /// The descriptor describes the object.
    HGI_API
    HgiShaderFunctionDesc const& GetDescriptor() const;

    /// Returns false if any shader compile errors occured.
    HGI_API
    virtual bool IsValid() const = 0;

    /// Returns shader compile errors.
    HGI_API
    virtual std::string const& GetCompileErrors() = 0;

    /// Returns the byte size of the GPU shader function.
    /// This can be helpful if the application wishes to tally up memory usage.
    HGI_API
    virtual size_t GetByteSizeOfResource() const = 0;

    /// This function returns the handle to the Hgi backend's gpu resource, cast
    /// to a uint64_t. Clients should avoid using this function and instead
    /// use Hgi base classes so that client code works with any Hgi platform.
    /// For transitioning code to Hgi, it can however we useful to directly
    /// access a platform's internal resource handles.
    /// There is no safety provided in using this. If you by accident pass a
    /// HgiMetal resource into an OpenGL call, bad things may happen.
    /// In OpenGL this returns the GLuint resource name.
    /// In Metal this returns the id<MTLFunction> as uint64_t.
    /// In Vulkan this returns the VkShaderModule as uint64_t.
    /// In DX12 this returns the ID3D12Resource pointer as uint64_t.
    HGI_API
    virtual uint64_t GetRawResource() const = 0;

protected:
    HGI_API
    HgiShaderFunction(HgiShaderFunctionDesc const& desc);

    HgiShaderFunctionDesc _descriptor;

private:
    HgiShaderFunction() = delete;
    HgiShaderFunction & operator=(const HgiShaderFunction&) = delete;
    HgiShaderFunction(const HgiShaderFunction&) = delete;
};

using HgiShaderFunctionHandle = HgiHandle<class HgiShaderFunction>;
using HgiShaderFunctionHandleVector = std::vector<HgiShaderFunctionHandle>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
