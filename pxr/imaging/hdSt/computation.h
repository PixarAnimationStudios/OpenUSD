//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_COMPUTATION_H
#define PXR_IMAGING_HD_ST_COMPUTATION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdResourceRegistry;

using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;
using HdBufferSpecVector = std::vector<struct HdBufferSpec>;

using HdStComputationSharedPtr = std::shared_ptr<class HdStComputation>;
using HdStComputationSharedPtrVector = std::vector<HdStComputationSharedPtr>;

/// \class HdStComputation
///
/// An interface class for GPU computation.
///
/// GPU computation fills the result into range, which has to be allocated
/// using buffer specs determined by GetBufferSpecs, and registered as a pair
/// of computation and range.
///
class HdStComputation
{
public:
    HDST_API
    virtual ~HdStComputation();

    /// Execute computation.
    virtual void Execute(
        HdBufferArrayRangeSharedPtr const &range,
        HdResourceRegistry *resourceRegistry) = 0;

    /// Returns the size of its destination buffer (located by range argument
    /// of Execute()). This function will be called after all HdBufferSources
    /// have been resolved and commited, so it can use the result of those
    /// buffer source results.
    /// Returning 0 means it doesn't need to resize.
    virtual int GetNumOutputElements() const = 0;

    /// Add the buffer spec for this computation into given bufferspec vector.
    /// Caller has to allocate the destination buffer with respect to the
    /// BufferSpecs, and passes the range when registering the computation.
    virtual void GetBufferSpecs(HdBufferSpecVector *specs) const = 0;

    /// This function is needed as HdStComputation shares a templatized
    /// interface with HdBufferSource.
    ///
    /// It is a check to see if the GetBufferSpecs would produce a valid result.
    bool IsValid() { return true; }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_COMPUTATION_H
