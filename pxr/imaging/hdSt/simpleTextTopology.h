//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_ST_SIMPLE_TEXT_TOPOLOGY_H
#define PXR_IMAGING_HD_ST_SIMPLE_TEXT_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/simpleTextTopology.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_SimpleTextTopologySharedPtr =
    std::shared_ptr<class HdSt_SimpleTextTopology>;

using HdBufferSourceSharedPtr = std::shared_ptr<class HdBufferSource>;

// HdSt_SimpleTextTopology
//
// Storm implementation for simpleText topology.
//
class HdSt_SimpleTextTopology final : public HdSimpleTextTopology {
public:
    static HdSt_SimpleTextTopologySharedPtr New(const HdSimpleTextTopology &src);

    virtual ~HdSt_SimpleTextTopology();

    // Get the indices buffer for the simpleText.
    HdBufferSourceSharedPtr GetTriangleIndexBuilderComputation();

private:
    // Must be created through factory
    explicit HdSt_SimpleTextTopology(const HdSimpleTextTopology&src);


    // No default construction or copying.
    HdSt_SimpleTextTopology()                                         = delete;
    HdSt_SimpleTextTopology(const HdSt_SimpleTextTopology&)         = delete;
    HdSt_SimpleTextTopology&operator =(const HdSt_SimpleTextTopology&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_SIMPLE_TEXT_TOPOLOGY_H
