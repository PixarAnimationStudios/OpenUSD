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
#ifndef HDST_EXT_COMP_GPU_COMPUTATION_H
#define HDST_EXT_COMP_GPU_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/extCompGpuComputationBufferSource.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdExtComputation;
typedef boost::shared_ptr<class HdStGLSLProgram> HdStGLSLProgramSharedPtr;

typedef boost::shared_ptr<class HdStExtCompGpuComputation>
                                HdStExtCompGpuComputationSharedPtr;

/// \class HdStExtCompGpuComputation
/// A Computation that represents a GPU implementation of a ExtComputation.
///
/// The computation implements the basic:
///    input HdBufferArrayRange -> processing -> output HdBufferArrayRange
/// model of HdComputations where processing happens in Execute during the
/// Execute phase of HdResourceRegistry::Commit.
///
/// The computation is performed in three stages by three companion classes:
/// 
/// 1. HdStExtCompGpuComputationBufferSource is responsible for loading
/// input HdBuffersources into the input HdBufferArrayRange during the Resolve
/// phase of the HdResourceRegistry::Commit processing.
///
/// 2. HdStExtCompGpuComputationResource holds the committed GPU resident
/// resources along with the compiled compute shading kernel to execute.
/// The values of the HdBufferArrayRanges for the inputs are stored in this
/// object. The resource can store heterogenous sources with differing number
/// of elements as may be required by computations.
///
/// 3. HdStExtCompGpuComputation executes the kernel using the committed GPU
/// resident resources and stores the results to the destination
/// HdBufferArrayRange given in Execute. The destination HdBufferArrayRange is
/// allocated by the owning HdRprim that registers the computation with the
/// HdResourceRegistry by calling HdResourceRegistry::AddComputation.
/// 
/// \see HdStExtCompGpuComputationBufferSource
/// \see HdStExtCompGpuComputationResource
/// \see HdRprim
/// \see HdComputation
/// \see HdResourceRegistry
/// \see HdExtComputation
/// \see HdBufferArrayRange
class HdStExtCompGpuComputation final : public HdComputation {
public:
    /// Constructs a new GPU ExtComputation computation.
    /// inputs provides a list of buffer sources that this computation
    /// requires.
    /// outputs is a list of outputs by names that the computation produces.
    ///
    /// numElements specifies the number of elements in the output.
    HdStExtCompGpuComputation(
            SdfPath const &id,
            HdStExtCompGpuComputationResourceSharedPtr const &resource,
            TfToken const &primvarName,
            TfToken const &computationOutputName,
            int numElements);

    /// Creates a GPU computation implementing the given abstract computation.
    /// When created this allocates HdStExtCompGpuComputationResource to be
    /// shared with the HdStExtCompGpuComputationBufferSource. Nothing
    /// is assigned GPU resources unless the source is subsequently added to 
    /// the hdResourceRegistry and the registry is committed.
    /// 
    /// This delayed allocation allow Rprims to share computed primvar data and
    /// avoid duplicate allocations GPU resources for computation inputs and
    /// outputs.
    ///
    /// \param[in] sceneDelegate the delegate to pull scene inputs from.
    /// \param[in] sourceComp the abstract computation in the HdRenderIndex
    /// this instance actually implements.
    /// \param[in] computationOutputName the name of the output value for
    /// this computation.
    /// \param[in] destinationPrimvar the buffer source representing the
    /// output value in the destination
    /// HdBufferArrayRange that this computation will output results to.
    /// \see HdExtComputation
    HDST_API
    static HdStExtCompGpuComputationSharedPtr
    CreateGpuComputation(
        HdSceneDelegate *sceneDelegate,
        HdExtComputation const *sourceComp,
        TfToken const &computationOutputName,
        HdBufferSourceSharedPtr const &destinationPrimvar);

    HDST_API
    virtual ~HdStExtCompGpuComputation() = default;

    /// Adds the output buffer specs generated by this computation to the passed
    /// in vector of buffer specs.
    /// \param[out] specs the vector of HdBufferSpec to add this computation
    /// output buffer layout requirements to.
    HDST_API
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const override;

    /// Executes the computation on the GPU.
    /// Called by HdResourceRegistry::Commit with the HdBufferArrayRange given
    /// to the HdResourceRegistry when the computation was added to the registry.
    /// \param[inout] range the buffer array range to save the computation
    /// result to.
    /// \param[in] resourceRegistry the registry that is current committing
    /// resources to the GPU.
    HDST_API
    virtual void Execute(HdBufferArrayRangeSharedPtr const &range,
                         HdResourceRegistry *resourceRegistry) override;

    /// Gets the number of elements in the output primvar.
    /// The number of elements produced by the computation must be known before
    /// doing the computation. The allocation of GPU resources needs to know
    /// the size to allocate before the kernel can run.
    HDST_API
    virtual int GetNumOutputElements() const override;
    
    /// Gets the shared GPU resource holder for the computation.
    /// HdStExtCompGPUComputationBufferSource will copy its data into this if
    /// it had been added to the HdResourceRegistry.
    HDST_API
    virtual HdStExtCompGpuComputationResourceSharedPtr const &GetResource() const;

private:
    SdfPath                                      _id;
    HdStExtCompGpuComputationResourceSharedPtr   _resource;
    TfToken                                      _primvarName;
    TfToken                                      _computationOutputName;
    int                                          _numElements;

    std::vector<int32_t>                         _uniforms;
    
    HdStExtCompGpuComputation()                                        = delete;
    HdStExtCompGpuComputation(const HdStExtCompGpuComputation &)       = delete;
    HdStExtCompGpuComputation &operator = (const HdStExtCompGpuComputation &)
                                                                       = delete;
};


/// For a given interpolation mode, obtains a set of ExtComputation primVar
/// source computations needed for this Rprim.
///
/// The list of primVars that are obtained through an ExtComputation
/// for the given interpolationMode is obtained from the scene delegate.
///
/// The scene delegate also provides information about which output on
/// which computation is providing the source of the primVar.
///
/// Based on the information, the function creates the necessary
/// computations and appends them on to the sources list (the sources vector
/// need not be empty).
///
/// The caller is expected to pass these computation on these computations
/// onto the resource registry (associating them with BARs if it is
/// expected the primvar will be downloaded)  Additional sources that
/// should be associated with BARs but do not otherwise need to be scheduled
/// for commit will be returned in reserveOnlySources.
/// 
/// The computation may also need to add sources that are resolved against
/// internal BARs that are not to be associated with the primvar BAR. Those
/// are returned in the separateComputationSources vector.
/// The caller is expected to add them to the resource registry if the
/// computation is needed.
HDST_API
void HdSt_GetExtComputationPrimVarsComputations(
    const SdfPath &id,
    HdSceneDelegate *sceneDelegate,
    HdInterpolation interpolationMode,
    HdDirtyBits dirtyBits,
    HdBufferSourceVector *sources,
    HdBufferSourceVector *reserveOnlySources,
    HdBufferSourceVector *separateComputationSources,
    HdComputationVector *computations);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_EXT_COMP_GPU_COMPUTATION_H
