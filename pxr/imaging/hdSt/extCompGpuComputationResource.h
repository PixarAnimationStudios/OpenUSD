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
#ifndef PXR_IMAGING_HD_ST_EXT_COMP_GPU_COMPUTATION_RESOURCE_H
#define PXR_IMAGING_HD_ST_EXT_COMP_GPU_COMPUTATION_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/computeShader.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdStExtCompGpuComputationResource> 
    HdStExtCompGpuComputationResourceSharedPtr;
typedef boost::shared_ptr<class HdStGLSLProgram> HdStGLSLProgramSharedPtr;
typedef std::vector<HdBufferArrayRangeSharedPtr> HdBufferArrayRangeSharedPtrVector;

/// \class HdStExtCompGpuComputationResource
///
/// A resource that represents the persistent GPU resources of an ExtComputation.
///
/// The persistent resources are shared between the ephemeral
/// HdStExtCompGpuComputationBufferSource and the actual
/// HdStExtCompGpuComputation. Once the buffer source is resolved the resource
/// is configured for the computation and it will then persist until the
/// computation is released.
///
/// All program and binding data required for compiling and loading HdRprim and
/// internal primvar data is held by this object. The companion source and
/// computation appeal to this object to access the GPU resources.
///
/// \see HdStExtCompGpuComputation
/// \see HdStExtCompGpuComputationBufferSource
class HdStExtCompGpuComputationResource final {
public:
    /// Creates a GPU computation resource that can bind resources matching
    /// the layout of the compute kernel.
    /// The registry passed is the registry that the kernel program will
    /// be shared amongst. De-duplication of the compiled and linked program
    /// for runtime execution happens on a per-registry basis.
    ///
    /// Memory for the input computation buffers must be provided
    /// This must be done prior to a HdResourceRegistry::Commit in
    /// which the computation has been added.
    /// Note that the Resource allocates no memory on its own and can be
    /// speculatively created and later de-duplicated, or discarded,
    /// without wasting resources.
    ///
    /// \param[in] outputBufferSpecs the buffer specs that the computation is
    /// expecting to output.
    /// \param[in] kernel the compute kernel source to run as the computation.
    /// \param[in] registry the registry that the internal computation
    /// will cache and de-duplicate its compute shader instance with.
    HdStExtCompGpuComputationResource(
        HdBufferSpecVector const &outputBufferSpecs,
        HdStComputeShaderSharedPtr const &kernel,
        HdBufferArrayRangeSharedPtrVector const &inputs,
        HdStResourceRegistrySharedPtr const &registry
    );

    virtual ~HdStExtCompGpuComputationResource() = default;

    /// Gets the HdBufferArrayRange that inputs should be loaded into using the
    /// resource binder.
    HdBufferArrayRangeSharedPtrVector const &GetInputs() const {
        return _inputs;
    }

    /// Gets the GPU HdStGLSLProgram to run to execute the computation.
    /// This may have been shared with many other instances in the same
    /// registry.
    /// The program is only valid for execution after Resolve has been called.
    HdStGLSLProgramSharedPtr const &GetProgram() const {
        return _computeProgram;
    }

    /// Gets the resource binder that matches the layout of the compute program.
    /// The binder is only valid for resolving layouts after Resolve has been
    /// called.
    HdSt_ResourceBinder const &GetResourceBinder() const {
        return _resourceBinder;
    }

    /// Resolve the resource bindings and program for use by a computation.
    /// The compute program is resolved and linked against the input and output
    /// resource bindings and the kernel source in this step.
    HDST_API
    bool Resolve();

private:
    HdBufferSpecVector                    _outputBufferSpecs;
    HdStComputeShaderSharedPtr            _kernel;
    HdStResourceRegistrySharedPtr         _registry;
    
    size_t                                _shaderSourceHash;
    HdBufferArrayRangeSharedPtrVector     _inputs;  
    HdStGLSLProgramSharedPtr              _computeProgram;
    HdSt_ResourceBinder                   _resourceBinder;
    
    HdStExtCompGpuComputationResource()                = delete;
    HdStExtCompGpuComputationResource(
            const HdStExtCompGpuComputationResource &) = delete;
    HdStExtCompGpuComputationResource &operator = (
            const HdStExtCompGpuComputationResource &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_COMP_GPU_COMPUTATION_RESOURCE_H

