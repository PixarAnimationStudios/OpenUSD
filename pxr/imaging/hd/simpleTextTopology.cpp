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
#include "pxr/imaging/hd/simpleTextTopology.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/arch/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSimpleTextTopology::HdSimpleTextTopology()
    : HdTopology()
    , _pointCount(0)
    , _decorationCount(0)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->simpleTextTopology);
}
HdSimpleTextTopology::HdSimpleTextTopology(size_t pointCount, size_t decorationCount)
  : HdTopology(), _pointCount(pointCount), _decorationCount(decorationCount)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->simpleTextTopology);
}

HdSimpleTextTopology::HdSimpleTextTopology(const HdSimpleTextTopology& src)
  : HdTopology(src), _pointCount(src._pointCount), _decorationCount(src._decorationCount)
{
    HD_PERF_COUNTER_INCR(HdPerfTokens->simpleTextTopology);
}


HdSimpleTextTopology::~HdSimpleTextTopology()
{
    HD_PERF_COUNTER_DECR(HdPerfTokens->simpleTextTopology);
}

bool
HdSimpleTextTopology::operator==(HdSimpleTextTopology const &other) const
{
    HD_TRACE_FUNCTION();

    // The topology is the same if the pointCount is the same.
    return _pointCount == other._pointCount && _decorationCount == other._decorationCount;
}

bool
HdSimpleTextTopology::operator!=(HdSimpleTextTopology const &other) const
{
    HD_TRACE_FUNCTION();

    // The topology is not the same if the pointCount is not the same.
    return _pointCount != other._pointCount && _decorationCount != other._decorationCount;
}

HdTopology::ID
HdSimpleTextTopology::ComputeHash() const
{
    HD_TRACE_FUNCTION();

    HdTopology::ID hash = 0;
    // We only need to hash the point count.
    hash = ArchHash64((const char*)&_pointCount, sizeof(size_t), hash);
    hash = ArchHash64((const char*)&_decorationCount, sizeof(size_t), hash);

    // Note: We don't hash topological visibility, because it is treated as a
    // per-prim opinion, and hence, shouldn't break topology sharing.
    return hash;
}

std::ostream&
operator << (std::ostream &out, HdSimpleTextTopology const &topo)
{
    out << "(" << topo.GetPointCount() << ", " << topo.GetDecorationCount() << ")";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

