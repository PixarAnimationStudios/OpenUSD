//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_COMP_CPU_COMPUTATION_H
#define PXR_IMAGING_HD_ST_EXT_COMP_CPU_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/bufferSource.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;
class HdExtComputation;

using HdStExtCompCpuComputationSharedPtr =
    std::shared_ptr<class HdStExtCompCpuComputation>;
using HdSt_ExtCompInputSourceSharedPtr =
    std::shared_ptr<class HdSt_ExtCompInputSource>;
using HdSt_ExtCompInputSourceSharedPtrVector =
    std::vector<HdSt_ExtCompInputSourceSharedPtr>;

///
/// A Buffer Source that represents a CPU implementation of a ExtComputation.
///
/// The computation implements the basic: input->processing->output model
/// where the inputs are other buffer sources and processing happens during
/// resolve.
///
/// As a computation may have many outputs, the outputs from the CPU
/// Computation can not be directly associated with a BAR.  Instead
/// other buffer source computation bind the output to sources that can
/// be used in a BAR.
///
/// Outputs to a computation are in SOA form, so a computation may have
/// many outputs, but each output has the same number of elements in it.
class HdStExtCompCpuComputation final : public HdNullBufferSource
{
public:
    HDST_API
    static const size_t INVALID_OUTPUT_INDEX;

    /// Constructs a new Cpu ExtComputation source.
    /// inputs provides a list of buffer sources that this computation
    /// requires.
    /// outputs is a list of outputs by names that the computation produces.
    ///
    /// Num elements specifies the number of elements in the output.
    ///
    /// sceneDelegate and id are used to callback to the scene delegate
    /// in order to invoke computation processing.
    HdStExtCompCpuComputation(
        const SdfPath &id,
        const HdSt_ExtCompInputSourceSharedPtrVector &inputs,
        const TfTokenVector &outputs,
        int numElements,
        HdSceneDelegate *sceneDelegate);

    HDST_API
    ~HdStExtCompCpuComputation() override;

    /// Create a CPU computation implementing the given abstract computation.
    /// The scene delegate identifies which delegate to pull scene inputs from.
    HDST_API
    static HdStExtCompCpuComputationSharedPtr
    CreateComputation(HdSceneDelegate *sceneDelegate,
                      const HdExtComputation &computation,
                      HdBufferSourceSharedPtrVector *computationSources);

    /// Returns the id for this computation as a token.
    HDST_API
    TfToken const &GetName() const override;

    /// Ask the scene delegate to run the computation and captures the output
    /// signals.
    HDST_API
    bool Resolve() override;

    HDST_API
    size_t GetNumElements() const override;


    /// Converts a output name token into an index.
    HDST_API
    size_t GetOutputIndex(const TfToken &outputName) const;

    /// Returns the value of the specified output
    /// (after the computations been Resolved).
    HDST_API
    const VtValue &GetOutputByIndex(size_t index) const;

protected:
    /// Returns if the computation is specified correctly.
    HDST_API
    bool _CheckValid() const override;

private:
    SdfPath                                 _id;
    HdSt_ExtCompInputSourceSharedPtrVector  _inputs;
    TfTokenVector                           _outputs;
    size_t                                  _numElements;
    HdSceneDelegate                        *_sceneDelegate;

    std::vector<VtValue>                    _outputValues;

    HdStExtCompCpuComputation() = delete;
    HdStExtCompCpuComputation(
        const HdStExtCompCpuComputation &) = delete;
    HdStExtCompCpuComputation &operator = (
        const HdStExtCompCpuComputation &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_COMP_CPU_COMPUTATION_H
