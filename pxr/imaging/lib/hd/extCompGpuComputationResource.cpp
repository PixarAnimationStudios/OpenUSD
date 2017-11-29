//
// Copyright 2017 Pixar
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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/extCompGpuComputationResource.h"
#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/codeGen.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/glUtils.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

static size_t _Hash(HdBufferSpecVector const &specs) {
    size_t result = 0;
    for (HdBufferSpec const &spec : specs) {
        size_t const params[] = { 
            spec.name.Hash(),
            spec.glDataType,
            (size_t)spec.numComponents
        };
        boost::hash_combine(result,
                ArchHash((char const*)params, sizeof(size_t) * 3));
    }
    return result;
}

HdExtCompGpuComputationResource::HdExtCompGpuComputationResource(
        HdBufferSpecVector const &outputBufferSpecs,
        HdComputeShaderSharedPtr const &kernel,
        HdResourceRegistrySharedPtr const &registry)
 : _outputBufferSpecs(outputBufferSpecs)
 , _kernel(kernel)
 , _registry(registry)
 , _shaderSourceHash()
 , _internalRange()
 , _computeProgram()
 , _resourceBinder()
{
    
}

bool
HdExtCompGpuComputationResource::Resolve()
{
    // Non-in-place sources should have been registered as resource registry
    // sources already and Resolved. They go to an internal buffer range that
    // was allocated in AllocateInternalRange
    HdBufferSpecVector inputBufferSpecs;
    if (_internalRange) {
        _internalRange->AddBufferSpecs(&inputBufferSpecs);
    }
    // Once we know the names and sizes of all outputs and inputs and the kernel
    // to use we can codeGen the compute shader to use.
    
    // We can shortcut the codegen by using a heuristic for determining that
    // the output source would be identical given a certain destination buffer
    // range.
    size_t shaderSourceHash = 0;
    boost::hash_combine(shaderSourceHash, _kernel->ComputeHash());
    boost::hash_combine(shaderSourceHash, _Hash(_outputBufferSpecs));
    boost::hash_combine(shaderSourceHash, _Hash(inputBufferSpecs));
    
    // XXX we'll need to test for hash collisions as they could be fatal in the
    // case of shader sources. Adjust based on pref vs correctness needs.
    // The new specs and the old specs as well as the new vs old kernel
    // source should be compared for equality if that is the case.
    //if (_shaderSourceHash == shaderSourceHash) {
    //    -- if hash equal but not content equal resolve hash collision --
    //}
    
    // If the source hash mismatches the saved program from previous executions
    // we are going to have to recompile it here.
    // We save the kernel for future runs to not have to incur the
    // compilation cost each time.
    if (!_computeProgram || _shaderSourceHash != shaderSourceHash) {
        HdShaderCodeSharedPtrVector shaders;
        shaders.push_back(_kernel);
        Hd_CodeGen codeGen(shaders);
        
        // let resourcebinder resolve bindings and populate metadata
        // which is owned by codegen.
        _resourceBinder.ResolveComputeBindings(_outputBufferSpecs,
                                              inputBufferSpecs,
                                              shaders,
                                              codeGen.GetMetaData());

        HdGLSLProgram::ID registryID = codeGen.ComputeHash();

        {
            HdInstance<HdGLSLProgram::ID, HdGLSLProgramSharedPtr> programInstance;

            // ask registry to see if there's already compiled program
            std::unique_lock<std::mutex> regLock =
                _registry->RegisterGLSLProgram(registryID, &programInstance);

            if (programInstance.IsFirstInstance()) {
                HdGLSLProgramSharedPtr glslProgram = codeGen.CompileComputeProgram();
                if (!TF_VERIFY(glslProgram)) {
                    return false;
                }
                
                if (!glslProgram->Link()) {
                    std::string logString;
                    HdGLUtils::GetProgramLinkStatus(
                        glslProgram->GetProgram().GetId(),
                        &logString);
                    TF_WARN("Failed to link compute shader:\n%s\n",
                            logString.c_str());
                    return false;
                }
                
                // store the program into the program registry.
                programInstance.SetValue(glslProgram);
            }

            _computeProgram = programInstance.GetValue();
        }
        
        if (!TF_VERIFY(_computeProgram)) {
            return false;
        }
        
        _shaderSourceHash = shaderSourceHash;
    }
    return true;
}

void
HdExtCompGpuComputationResource::AllocateInternalRange(
    HdBufferSourceVector const &inputs,
    HdBufferSourceVector *internalSources,
    HdResourceRegistrySharedPtr const &resourceRegistry)
{
    TF_VERIFY(internalSources);

    for (size_t i = 0; i < inputs.size(); ++i) {
        HdBufferSourceSharedPtr const &source = inputs[i];
        bool inPlace = false;
        TF_FOR_ALL(it, _outputBufferSpecs) {
            if (it->name == source->GetName()) {
                // XXX upload in-place to not waste buffer space
                //resourceRegistry->AddSource(prim's range, source);
                inPlace = true;
                break;
            }
        }
        if (!inPlace) {
            // upload to SSBO allocated input range.
            internalSources->push_back(source);
        }
    }

    if (!_internalRange && internalSources->size() > 0) {
        HdBufferSpecVector bufferSpecs;
        for (size_t i = 0; i < internalSources->size(); ++i) {
            HdBufferSourceSharedPtr const &source = (*internalSources)[i];
            // This currently needs the element count as the array size as the
            // SSBO allocator needs all data in one stripe.
            const int arraySize = source->GetNumElements();
            bufferSpecs.push_back(
                HdBufferSpec(source->GetName(),
                             source->GetGLComponentDataType(),
                             source->GetNumComponents(),
                             arraySize));
        }

        _internalRange = boost::static_pointer_cast<HdBufferArrayRangeGL>(
            resourceRegistry->AllocateShaderStorageBufferArrayRange(
                HdTokens->primVar, bufferSpecs));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
