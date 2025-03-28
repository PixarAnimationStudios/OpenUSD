//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_COPY_COMPUTATION_H
#define PXR_IMAGING_HD_ST_COPY_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/computation.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdStCopyComputationGPU
///
/// A GPU computation which transfers a vbo range specified by src and name to
/// the given range.
///
class HdStCopyComputationGPU : public HdStComputation
{
public:
    HDST_API
    HdStCopyComputationGPU(HdBufferArrayRangeSharedPtr const &src,
                           TfToken const &name);

    HDST_API
    void Execute(HdBufferArrayRangeSharedPtr const &range,
                 HdResourceRegistry *resourceRegistry) override;

    HDST_API
    int GetNumOutputElements() const override;

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

private:
    HdBufferArrayRangeSharedPtr _src;
    TfToken _name;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_COMPUTATION_H
