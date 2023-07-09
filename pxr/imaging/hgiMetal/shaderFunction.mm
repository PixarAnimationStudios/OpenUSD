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
        id<MTLDevice> device = hgi->GetPrimaryDevice();

        HgiMetalShaderGenerator shaderGenerator {desc, device};
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
