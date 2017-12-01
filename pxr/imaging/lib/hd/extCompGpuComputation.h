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
#ifndef HD_EXT_COMP_GPU_COMPUTATION_H
#define HD_EXT_COMP_GPU_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"
#include "pxr/imaging/hd/extCompGpuComputationBufferSource.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdGLSLProgram;
typedef boost::shared_ptr<class HdGLSLProgram> HdGLSLProgramSharedPtr;

/// \class HdExtCompGpuComputation
/// A Computation that represents a GPU implementation of a ExtComputation.
///
/// The computation implements the basic: input BAR -> processing -> output BAR
/// model of HdComputations where processing happens during Execute.
///
/// A companion source buffer is responsible for loading input sources into
/// the input BAR.
///
/// A GPU computation can write only to its own BAR.
class HdExtCompGpuComputation final : public HdComputation {
public:
    static const size_t INVALID_OUTPUT_INDEX;

    /// Constructs a new GPU ExtComputation computation.
    /// inputs provides a list of buffer sources that this computation
    /// requires.
    /// outputs is a list of outputs by names that the computation produces.
    ///
    /// numElements specifies the number of elements in the output.
    HdExtCompGpuComputation(
            SdfPath const &id,
            HdExtCompGpuComputationResourceSharedPtr const &resource,
            TfToken const &primvarName,
            HdBufferSpecVector const &outputBufferSpecs,
            int numElements);

    HD_API
    virtual ~HdExtCompGpuComputation() = default;

    HD_API
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const override;

    HD_API
    virtual void Execute(HdBufferArrayRangeSharedPtr const &range,
                         HdResourceRegistry *resourceRegistry) override;

    HD_API
    virtual int GetNumOutputElements() const override;
    
    HD_API
    virtual HdExtCompGpuComputationResourceSharedPtr const &GetResource() const;

private:
    SdfPath                                      _id;
    HdExtCompGpuComputationResourceSharedPtr     _resource;
    TfToken                                      _dstName;
    HdBufferSpecVector                           _outputSpecs;
    int                                          _numElements;

    std::vector<int32_t>                         _uniforms;
    
    HdExtCompGpuComputation()                                          = delete;
    HdExtCompGpuComputation(const HdExtCompGpuComputation &)           = delete;
    HdExtCompGpuComputation &operator = (const HdExtCompGpuComputation &)
                                                                       = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_EXT_COMP_CPU_COMPUTATION_H
