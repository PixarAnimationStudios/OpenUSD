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
#ifndef HD_EXT_COMPUATION_H
#define HD_EXT_COMPUATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include <vector>
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdExtCompCpuComputation;
class HdExtCompGpuComputation;
class HdExtCompGpuComputationBufferSource;

typedef boost::shared_ptr<HdExtCompCpuComputation>
                                HdExtCompCpuComputationSharedPtr;
typedef boost::shared_ptr<HdExtCompGpuComputation>
                                HdExtCompGpuComputationSharedPtr;
typedef boost::shared_ptr<HdExtCompGpuComputationBufferSource>
                                HdExtCompGpuComputationBufferSourceSharedPtr;

///
/// Hydra Representation of a Client defined computation.
///
/// A computation provides a way to procedurally generate a primVar.
///
/// In represents a basic Input -> Processing -> Output model.
///
/// Primarily inputs are provided by the scene delegate via the Get()
/// mechanism.
///
/// Computations can also be chained together, such that the output from
/// one computation can be an input to another.
///
/// The results of a computation is designed to be in SOA form (structure or
/// array), where each output is a member of the "structure" producing several
/// parallel arrays.  While the type of the elements of the array is defined
/// by the output member, the number of elements in each array is the same
/// across all outputs.
///
/// ExtComputations use a pull model, so processing is only triggered if
/// a downstream computation or prim pulls on one the computations outputs.
///
class HdExtComputation final {
public:
    /// Construct a new ExtComputation identified by id.
    HdExtComputation(SdfPath const &id);

    HD_API
    ~HdExtComputation() = default;

    ///
    /// Change tracking
    ///
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyInputDesc        = 1 << 0,  ///< The list of inputs or input
                                         ///  bindings changed
        DirtyOutputDesc       = 1 << 1,  ///< The list of outputs changed
        DirtyElementCount     = 1 << 2,  ///< The number of elements in the
                                         ///  output arrays changed
        DirtySceneInput       = 1 << 3,  ///< A scene input changed value
        DirtyCompInput        = 1 << 4,  ///< A computation input changed value
        DirtyKernel           = 1 << 5,  ///< The compute kernel binding changed
        AllDirty              = (DirtyInputDesc
                                |DirtyOutputDesc
                                |DirtyElementCount
                                |DirtySceneInput
                                |DirtyCompInput
                                |DirtyKernel)
    };

    ///
    /// Update the object with new data from the scene delegate
    /// using dirtyBits to specify what needs to be updated.
    ///
    /// It is expected that the routine clears any dirty bits that
    /// were synced.
    ///
    HD_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdDirtyBits     *dirtyBits);

    ///
    /// Gets the buffer source object for the computation.
    /// The returned source has already been added to the resource registry.
    ///
    /// The scene delegate identifies which delegate to pull scene inputs from.
    HD_API
    HdExtCompCpuComputationSharedPtr GetComputation(
                                HdSceneDelegate *sceneDelegate,
                                HdBufferSourceVector *computationSources) const;

    ///
    /// Gets the computation object for the computation to be performed on GPU.
    /// The returned computation has already been added to the resource registry.
    ///
    ///@param sceneDelegate identifies which delegate to pull scene inputs from.
    ///
    HD_API
    std::pair<HdExtCompGpuComputationSharedPtr,
              HdExtCompGpuComputationBufferSourceSharedPtr>
                                     GetGpuComputation(
                                HdSceneDelegate *sceneDelegate,
                                HdBufferSourceVector *computationSources,
                                TfToken const &primvarName,
                                HdBufferSpecVector const &outputBufferSpecs,
                                HdBufferSpecVector const &primInputSpecs) const;
    ///
    /// Returns the set of dirty bits required for the first-sync of the
    /// object.
    ///
    HD_API
    HdDirtyBits GetInitialDirtyBits() const;

private:
    struct SourceComputationDesc {
        SdfPath computationId;
        TfToken computationOutput;
    };
    typedef std::vector<SourceComputationDesc> SourceComputationDescVector;

    SdfPath _id;

    size_t                      _elementCount;
    TfTokenVector               _sceneInputs;
    TfTokenVector               _computationInputs;
    SourceComputationDescVector _computationSourceDescs;
    TfTokenVector               _outputs;
    std::string                 _kernel;

    // Creates a buffer source objects that bind the inputs and
    // for processing the computation.
    // These buffer sources are added to the resource registry.
    //
    // The scene delegate identifies which delegate to pull scene inputs from.
    //
    // The return value is the processing buffer source..
    HD_API
    HdExtCompCpuComputationSharedPtr _CreateCpuComputation(
                                HdSceneDelegate *sceneDelegate,
                                HdBufferSourceVector *computationSources) const;

    HD_API
    std::pair<HdExtCompGpuComputationSharedPtr,
              HdExtCompGpuComputationBufferSourceSharedPtr>
                                     _CreateGpuComputation(
                                HdSceneDelegate *sceneDelegate,
                                HdBufferSourceVector *computationSources,
                                TfToken const &primvarName,
                                HdBufferSpecVector const &outputBufferSpecs,
                                HdBufferSpecVector const &primInputSpecs) const;

    // No default construction or copying
    HdExtComputation() = delete;
    HdExtComputation(const HdExtComputation &) = delete;
    HdExtComputation &operator =(const HdExtComputation &) = delete;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_EXT_COMPUATION_H
