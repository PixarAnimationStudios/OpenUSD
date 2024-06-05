//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"
#include "pxr/imaging/hgiMetal/shaderGenerator.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/diagnostic.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalShaderFunction::HgiMetalShaderFunction(
    HgiMetal *hgi,
    HgiShaderFunctionDesc const& desc)
  : HgiShaderFunction(desc)
  , _shaderId(nil)
{
    if (desc.shaderCode) {
        HgiMetalShaderGenerator shaderGenerator(hgi, desc);
        shaderGenerator.Execute();
        const char *shaderCode = shaderGenerator.GetGeneratedShaderCode();

        MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
        options.fastMathEnabled = YES;

        if (@available(macOS 10.15, ios 13.0, *)) {
            options.languageVersion = MTLLanguageVersion2_2;
        } else {
            options.languageVersion = MTLLanguageVersion2_1;
        }

        options.preprocessorMacros = @{
                @"ARCH_GFX_METAL": @1,
        };

        NSError *error = NULL;
        id<MTLLibrary> library =
            [hgi->GetPrimaryDevice() newLibraryWithSource:@(shaderCode)
                                                        options:options
                                                        error:&error];

        [options release];
        options = nil;

        NSString *entryPoint = nullptr;
        switch (_descriptor.shaderStage) {
            case HgiShaderStageVertex:
                entryPoint = @"vertexEntryPoint";
                break;
            case HgiShaderStageFragment:
                entryPoint = @"fragmentEntryPoint";
                break;
            case HgiShaderStageCompute:
                entryPoint = @"computeEntryPoint";
                break;
            case HgiShaderStagePostTessellationControl:
                entryPoint = @"vertexEntryPoint";
                break;
            case HgiShaderStagePostTessellationVertex:
                entryPoint = @"vertexEntryPoint";
                break;
            case HgiShaderStageTessellationControl:
            case HgiShaderStageTessellationEval:
            case HgiShaderStageGeometry:
                TF_CODING_ERROR("Todo: Unsupported shader stage");
                break;
        }

        // Load the function into the library
        _shaderId = [library newFunctionWithName:entryPoint];
        if (!_shaderId) {
            NSString *err = [error localizedDescription];
            _errors = [err UTF8String];
        }
        else {
            HGIMETAL_DEBUG_LABEL(_shaderId, _descriptor.debugName.c_str());
        }
        
        [library release];
    }

    // Clear these pointers in our copy of the descriptor since we
    // have to assume they could become invalid after we return.
    _descriptor.shaderCodeDeclarations = nullptr;
    _descriptor.shaderCode = nullptr;
    _descriptor.generatedShaderCodeOut = nullptr;
}

HgiMetalShaderFunction::~HgiMetalShaderFunction()
{
    [_shaderId release];
    _shaderId = nil;
}

bool
HgiMetalShaderFunction::IsValid() const
{
    return _errors.empty();
}

std::string const&
HgiMetalShaderFunction::GetCompileErrors()
{
    return _errors;
}

size_t
HgiMetalShaderFunction::GetByteSizeOfResource() const
{
    return 0;
}

uint64_t
HgiMetalShaderFunction::GetRawResource() const
{
    return (uint64_t) _shaderId;
}

id<MTLFunction>
HgiMetalShaderFunction::GetShaderId() const
{
    return _shaderId;
}

PXR_NAMESPACE_CLOSE_SCOPE
