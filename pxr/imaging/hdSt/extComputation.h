//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_COMPUTATION_H
#define PXR_IMAGING_HD_ST_EXT_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;

/// \class HdStExtComputation
///
/// Specialization of HdExtComputation which manages inputs as GPU resources.
///
class HdStExtComputation : public HdExtComputation
{
public:
    /// Construct a new ExtComputation identified by id.
    HDST_API
    HdStExtComputation(SdfPath const &id);

    HDST_API
    ~HdStExtComputation() override;

    HDST_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    HDST_API
    void Finalize(HdRenderParam *renderParam) override;

    HDST_API
    HdBufferArrayRangeSharedPtr const & GetInputRange() const {
        return _inputRange;
    }

private:
    // No default construction or copying
    HdStExtComputation() = delete;
    HdStExtComputation(const HdStExtComputation &) = delete;
    HdStExtComputation &operator =(const HdStExtComputation &) = delete;

    HdBufferArrayRangeSharedPtr _inputRange;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_COMPUTATION_H
