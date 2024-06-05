//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_STRATEGY_BASE_H
#define PXR_IMAGING_HD_ST_STRATEGY_BASE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/bufferArray.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


using HdBufferArraySharedPtr = std::shared_ptr<class HdBufferArray>;
using HdBufferArrayRangeSharedPtr = std::shared_ptr<class HdBufferArrayRange>;

/// \class HdStAggregationStrategy
///
/// Aggregation strategy base class.
///
class HdStAggregationStrategy {
public:
    /// Aggregation ID
    typedef size_t AggregationId;

    HDST_API
    virtual ~HdStAggregationStrategy();

    /// Factory for creating HdBufferArray
    virtual HdBufferArraySharedPtr CreateBufferArray(
        TfToken const &role,
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint) = 0;

    /// Factory for creating HdBufferArrayRange
    virtual HdBufferArrayRangeSharedPtr CreateBufferArrayRange() = 0;
        

    /// Returns id for given bufferSpecs to be used for aggregation
    virtual AggregationId ComputeAggregationId(
        HdBufferSpecVector const &bufferSpecs,
        HdBufferArrayUsageHint usageHint) const = 0;

    /// Returns the buffer specs from a given buffer array
    virtual HdBufferSpecVector GetBufferSpecs(
        HdBufferArraySharedPtr const &bufferArray) const = 0;

    /// Returns the accumulated GPU resource allocation
    /// for items in the BufferArray passed as parameter
    virtual size_t GetResourceAllocation(
        HdBufferArraySharedPtr const &bufferArray, 
        VtDictionary &result) const = 0;

    /// (Optional) called to Flush consolidated / staging buffers.
    HDST_API
    virtual void Flush() {}
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_STRATEGY_BASE_H
