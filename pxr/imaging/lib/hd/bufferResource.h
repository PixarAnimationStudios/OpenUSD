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
#ifndef HD_BUFFER_RESOURCE_H
#define HD_BUFFER_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/tf/token.h"

#include <boost/shared_ptr.hpp>
#include <cstddef>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class HdBufferResource;

typedef boost::shared_ptr<HdBufferResource> HdBufferResourceSharedPtr;

typedef std::vector<
    std::pair<TfToken, HdBufferResourceSharedPtr> > HdBufferResourceNamedList;

/// \class HdBufferResource
///
/// A specific type of HdResource (GPU resource) representing a buffer object.
///
class HdBufferResource : public HdResource {
public:
    HD_API
    HdBufferResource(TfToken const &role,
                     HdTupleType tupleType,
                     int offset,
                     int stride);
    HD_API
    ~HdBufferResource();

    /// Data type and count
    HdTupleType GetTupleType() const { return _tupleType; }

    /// Returns the interleaved offset (in bytes) of this data.
    int GetOffset() const {return _offset;}

    /// Returns the stride (in bytes) of underlying buffer.
    int GetStride() const {return _stride;}

protected:
    HdTupleType _tupleType;
    int _offset;
    int _stride;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_BUFFER_RESOURCE_H
