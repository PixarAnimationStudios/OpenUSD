//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_BASIS_CURVES_TOPOLOGY_H
#define PXR_IMAGING_HD_ST_BASIS_CURVES_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/basisCurvesTopology.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_BasisCurvesTopologySharedPtr =
    std::shared_ptr<class HdSt_BasisCurvesTopology>;

using HdBufferSourceSharedPtr = std::shared_ptr<class HdBufferSource>;


// HdSt_BasisCurvesTopology
//
// Storm implementation for basisCurves topology.
//
class HdSt_BasisCurvesTopology final : public HdBasisCurvesTopology {
public:
    HDST_API
    static HdSt_BasisCurvesTopologySharedPtr New(const HdBasisCurvesTopology &src);

    HDST_API
    virtual ~HdSt_BasisCurvesTopology();

    HdBufferSourceSharedPtr GetPointsIndexBuilderComputation();
    HdBufferSourceSharedPtr GetIndexBuilderComputation(bool forceLines);

private:
    // Must be created through factory
    explicit HdSt_BasisCurvesTopology(const HdBasisCurvesTopology &src);


    // No default construction or copying.
    HdSt_BasisCurvesTopology()                                         = delete;
    HdSt_BasisCurvesTopology(const HdSt_BasisCurvesTopology &)         = delete;
    HdSt_BasisCurvesTopology &operator =(const HdSt_BasisCurvesTopology &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_BASIS_CURVES_TOPOLOGY_H
