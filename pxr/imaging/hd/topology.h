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
#ifndef PXR_IMAGING_HD_TOPOLOGY_H
#define PXR_IMAGING_HD_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/arch/inttypes.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdTopology> HdTopologySharedPtr;

class HdTopology {
public:
    typedef uint64_t ID;

    HdTopology() {};
    virtual ~HdTopology() {};

    /// Returns the hash value of this topology to be used for instancing.
    virtual ID ComputeHash() const = 0;
};

HD_API
std::ostream& operator << (std::ostream &out, HdTopology const &);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_TOPOLOGY_H

