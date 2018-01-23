//
// Copyright 2016 Pixar
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
#ifndef HDST_BASIS_CURVES_TOPOLOGY_H
#define HDST_BASIS_CURVES_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/basisCurvesTopology.h"

PXR_NAMESPACE_OPEN_SCOPE


typedef std::shared_ptr<class HdSt_BasisCurvesTopology>
                                              HdSt_BasisCurvesTopologySharedPtr;
typedef std::shared_ptr<class HdBufferSource> HdBufferSourceSharedPtr;


// HdSt_BasisCurvesTopology
//
// Hydra Stream implementation for basisCurves topology.
//
class HdSt_BasisCurvesTopology final : public HdBasisCurvesTopology {
public:
    static HdSt_BasisCurvesTopologySharedPtr New(const HdBasisCurvesTopology &src);

    virtual ~HdSt_BasisCurvesTopology();

    HdBufferSourceSharedPtr GetIndexBuilderComputation(bool supportSmoothCurves);

private:
    // Must be created through factory
    explicit HdSt_BasisCurvesTopology(const HdBasisCurvesTopology &src);


    // No default construction or copying.
    HdSt_BasisCurvesTopology()                                         = delete;
    HdSt_BasisCurvesTopology(const HdSt_BasisCurvesTopology &)         = delete;
    HdSt_BasisCurvesTopology &operator =(const HdSt_BasisCurvesTopology &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_BASIS_CURVES_TOPOLOGY_H
