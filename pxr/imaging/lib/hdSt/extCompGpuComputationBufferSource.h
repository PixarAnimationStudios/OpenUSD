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
#ifndef HDST_EXT_COMP_GPU_COMPUTATION_BUFFER_SOURCE_H
#define HDST_EXT_COMP_GPU_COMPUTATION_BUFFER_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hdSt/extCompGpuComputationResource.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStExtCompGpuComputationBufferSource;
typedef boost::shared_ptr<class HdStExtCompGpuComputationBufferSource>
    HdStExtCompGpuComputationBufferSourceSharedPtr;

/// \class HdStExtCompGpuComputationBufferSource
///
/// A Buffer Source that represents a GPU implementation of an ExtComputation.
///
/// The computation implements the basic: input->processing->output model
/// where the inputs are other buffer sources and processing happens during
/// Execute in a companion ExtComputation.
///
/// This source is responsible for resolving the inputs that are directed
/// at the computation itself rather than coming from the rprim the computation
/// is attached to. All the inputs bound through this source are reflected
/// in the compute kernel as read-only accessors accessible via HdGet_<name>.
///
/// A GLSL example kernel using an input from a primvar computation would be:
/// \code
/// void compute(int index) {
///   // assumes the input buffer is named 'sourcePoints'
///   vec3 point = HdGet_sourcePoints(index);
///   // 'points' is an rprim primvar (HdToken->points)
///   HdSet_points(index, point * 2.0);
/// }
///
/// \see HdStExtCompGpuComputation
///
class HdStExtCompGpuComputationBufferSource final : public HdNullBufferSource {
public:
    /// Constructs a new GPU ExtComputation computation.
    /// inputs provides a list of buffer sources that this computation
    /// requires.
    /// outputs is a list of outputs by names that the computation produces.
    ///
    /// numElements specifies the number of elements in the output.
    HdStExtCompGpuComputationBufferSource(
            SdfPath const &id,
            TfToken const &primvarName,
            HdBufferSourceVector const &inputs,
            int numElements,
            HdStExtCompGpuComputationResourceSharedPtr const &resource);

    HDST_API
    virtual ~HdStExtCompGpuComputationBufferSource() = default;

    HDST_API
    virtual bool Resolve() override;
    
    virtual HdBufferSourceVector const &GetInputs() const {
        return _inputs;
    }

protected:
    virtual bool _CheckValid() const override;
    
private:
    
    SdfPath                                  _id;
    TfToken                                  _primvarName;
    
    HdBufferSourceVector                     _inputs;
    int                                      _numElements;
    
    HdStExtCompGpuComputationResourceSharedPtr _resource;
    
    HdStExtCompGpuComputationBufferSource()                = delete;
    HdStExtCompGpuComputationBufferSource(
            const HdStExtCompGpuComputationBufferSource &) = delete;
    HdStExtCompGpuComputationBufferSource &operator = (
            const HdStExtCompGpuComputationBufferSource &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_EXT_COMP_CPU_COMPUTATION_BUFFER_SOURCE_H
