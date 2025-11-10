//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DASH_DOT_LINES_TOPOLOGY_H
#define PXR_IMAGING_HD_ST_DASH_DOT_LINES_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/dashDotLinesTopology.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_DashDotLinesTopologySharedPtr =
    std::shared_ptr<class HdSt_DashDotLinesTopology>;

using HdBufferSourceSharedPtr = std::shared_ptr<class HdBufferSource>;


// HdSt_DashDotLinesTopology
//
// Storm implementation for dashDotLines topology.
//
class HdSt_DashDotLinesTopology final : public HdDashDotLinesTopology {
public:
    HDST_API
    static HdSt_DashDotLinesTopologySharedPtr New(const HdDashDotLinesTopology &src);

    HDST_API
    virtual ~HdSt_DashDotLinesTopology();

    HdBufferSourceSharedPtr GetPointsIndexBuilderComputation();
    HdBufferSourceSharedPtr GetIndexBuilderComputation(bool forceLines);

private:
    // Must be created through factory
    explicit HdSt_DashDotLinesTopology(const HdDashDotLinesTopology &src);


    // No default construction or copying.
    HdSt_DashDotLinesTopology()                                         = delete;
    HdSt_DashDotLinesTopology(const HdSt_DashDotLinesTopology &)         = delete;
    HdSt_DashDotLinesTopology &operator =(const HdSt_DashDotLinesTopology &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_DASH_DOT_LINES_TOPOLOGY_H
