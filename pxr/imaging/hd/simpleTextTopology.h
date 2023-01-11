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
#ifndef PXR_IMAGING_HD_SIMPLE_TEXT_TOPOLOGY_H
#define PXR_IMAGING_HD_SIMPLE_TEXT_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/topology.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSimpleTextTopology
///
/// Topology data for simpleText.
///
/// HdSimpleTextTopology holds the raw input topology data for simpleText.
///
/// The geometries of the text render items are always triangles, and we provide separate position
/// for each point. So the indices are always from zero to the count of points. The topology only
/// diffs by pointCount.
///
class HdSimpleTextTopology : public HdTopology {
public:
    HD_API HdSimpleTextTopology();
    HD_API HdSimpleTextTopology(size_t pointCount, size_t decorationCount);
    HD_API HdSimpleTextTopology(const HdSimpleTextTopology& src);
    HD_API virtual ~HdSimpleTextTopology();

    /// Returns the hash value of this topology to be used for instancing.
    HD_API virtual ID ComputeHash() const;

    /// Returns point count of the text geometry.
    size_t GetPointCount() const {
        return _pointCount;
    }

    /// Returns point count of the text geometry.
    size_t GetDecorationCount() const {
        return _decorationCount;
    }

    /// Equality check between two simpleText topologies.
    HD_API bool operator==(HdSimpleTextTopology const &other) const;
    HD_API bool operator!=(HdSimpleTextTopology const &other) const;
private:
    size_t _pointCount;
    size_t _decorationCount;
};

HD_API
std::ostream& operator << (std::ostream &out, HdSimpleTextTopology const &topo);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SIMPLE_TEXT_TOPOLOGY_H
