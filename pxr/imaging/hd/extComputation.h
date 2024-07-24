//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_EXT_COMPUTATION_H
#define PXR_IMAGING_HD_EXT_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// Hydra Representation of a Client defined computation.
///
/// A computation provides a way to procedurally generate a primvar.
///
/// It represents a basic Input -> Processing -> Output model.
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
class HdExtComputation : public HdSprim
{
public:
    /// Construct a new ExtComputation identified by id.
    HD_API
    HdExtComputation(SdfPath const &id);

    HD_API
    ~HdExtComputation() override;

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

        DirtyDispatchCount    = 1 << 6,  ///< The number of kernel
                                         ///  invocations to execute changed

        AllDirty              = (DirtyInputDesc
                                |DirtyOutputDesc
                                |DirtyElementCount
                                |DirtySceneInput
                                |DirtyCompInput
                                |DirtyKernel
                                |DirtyDispatchCount)
    };

    HD_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    HD_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    HD_API
    size_t GetDispatchCount() const;

    HD_API
    size_t GetElementCount() const { return _elementCount; }

    HD_API
    TfTokenVector const & GetSceneInputNames() const {
        return _sceneInputNames;
    }

    HD_API
    TfTokenVector GetOutputNames() const;

    HD_API
    HdExtComputationInputDescriptorVector const &
    GetComputationInputs() const {
        return _computationInputs;
    }

    HD_API
    HdExtComputationOutputDescriptorVector const &
    GetComputationOutputs() const {
        return _computationOutputs;
    }

    HD_API
    const std::string& GetGpuKernelSource() const { return _gpuKernelSource; }

    HD_API
    bool IsInputAggregation() const;

protected:
    HD_API
    void
    _Sync(HdSceneDelegate *sceneDelegate,
          HdRenderParam   *renderParam,
          HdDirtyBits     *dirtyBits);

    HD_API
    static bool _IsEnabledSharedExtComputationData();

private:
    size_t                                 _dispatchCount;
    size_t                                 _elementCount;
    TfTokenVector                          _sceneInputNames;
    HdExtComputationInputDescriptorVector  _computationInputs;
    HdExtComputationOutputDescriptorVector _computationOutputs;
    std::string                            _gpuKernelSource;

    // No default construction or copying
    HdExtComputation() = delete;
    HdExtComputation(const HdExtComputation &) = delete;
    HdExtComputation &operator =(const HdExtComputation &) = delete;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_EXT_COMPUTATION_H
