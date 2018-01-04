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
class HdStExtCompGpuComputation;
class HdStGLSLProgram;
typedef boost::shared_ptr<class HdStGLSLProgram> HdStGLSLProgramSharedPtr;

typedef boost::shared_ptr<HdStExtCompGpuComputation>
                                HdStExtCompGpuComputationSharedPtr;

/// \class HdStExtCompGpuComputation
/// A Computation that represents a GPU implementation of a ExtComputation.
///
/// The computation implements the basic: input BAR -> processing -> output BAR
/// model of HdComputations where processing happens during Execute.
///
/// A companion source buffer is responsible for loading input sources into
/// the input BAR.
///
/// A GPU computation can write only to its own BAR.
class HdStExtCompGpuComputation final : public HdComputation {
public:
    static const size_t INVALID_OUTPUT_INDEX;

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
            HdBufferSpecVector const &outputBufferSpecs,
            int numElements);

    /// Create a GPU computation implementing the given abstract computation. 
    /// The scene delegate identifies which delegate to pull scene inputs from.
    HDST_API
    static std::pair<
        HdStExtCompGpuComputationSharedPtr,
        HdStExtCompGpuComputationBufferSourceSharedPtr>
    CreateComputation(
        HdSceneDelegate *sceneDelegate,
        const HdExtComputation &computation,
        HdBufferSourceVector *computationSources,
        TfToken const &primvarName,
        HdBufferSpecVector const &outputBufferSpecs,
        HdBufferSpecVector const &primInputSpecs);

    HDST_API
    virtual ~HdStExtCompGpuComputation() = default;

    HDST_API
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const override;

    HDST_API
    virtual void Execute(HdBufferArrayRangeSharedPtr const &range,
                         HdResourceRegistry *resourceRegistry) override;

    HDST_API
    virtual int GetNumOutputElements() const override;
    
    HDST_API
    virtual HdStExtCompGpuComputationResourceSharedPtr const &GetResource() const;

private:
    SdfPath                                      _id;
    HdStExtCompGpuComputationResourceSharedPtr     _resource;
    TfToken                                      _dstName;
    HdBufferSpecVector                           _outputSpecs;
    int                                          _numElements;

    std::vector<int32_t>                         _uniforms;
    
    HdStExtCompGpuComputation()                                          = delete;
    HdStExtCompGpuComputation(const HdStExtCompGpuComputation &)           = delete;
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
/// expected the primvar will be downloaded)
/// 
/// The computation may also need to add sources that are resolved against
/// internal BARs that are not to be associated with the primvar BAR. Those
/// are returned in the computationSources vector.
/// The caller is expected to add them to the resource registry if the
/// computation is needed.
HDST_API
void HdSt_GetExtComputationPrimVarsComputations(
    const SdfPath &id,
    HdSceneDelegate *sceneDelegate,
    HdInterpolation interpolationMode,
    HdDirtyBits dirtyBits,
    HdBufferSourceVector *sources,
    HdComputationVector *computations,
    HdBufferSourceVector *computationSources);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_EXT_COMP_CPU_COMPUTATION_H
