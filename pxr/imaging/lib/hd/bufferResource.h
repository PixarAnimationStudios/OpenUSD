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
#include "pxr/imaging/hd/conversions.h"

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
/// A specific type of HdResource (GPU resource) representing an OpenGL buffer
/// object.
///
class HdBufferResource : public HdResource {
public:
    HD_API
    HdBufferResource(TfToken const &role,
                     int glDataType,
                     short numComponents,
                     int arraySize,
                     int offset,
                     int stride);
    HD_API
    ~HdBufferResource();

    /// OpenGL data type; GL_UNSIGNED_INT, etc
    int GetGLDataType() const {return _glDataType;}

    /// Returns the number of components in a single element.
    /// This value is always in the range [1,4].
    short GetNumComponents() const {return _numComponents;}

    /// Returns the size of a single component.
    /// For example: sizeof(GLuint)
    size_t GetComponentSize() const {
        return HdConversions::GetComponentSize(_glDataType);
    }

    /// Returns the interleaved offset (in bytes) of this data
    int GetOffset() const { return _offset; }

    /// Returns the stride (in bytes) of underlying buffer
    int GetStride() const { return _stride; }

    /// Returns the size of array if this resource is a static-sized array.
    /// returns 1 for non-array resource.
    int GetArraySize() const { return _arraySize; }

    /// Sets the OpenGL name/identifier for this resource and its size.
    /// also caches the gpu address of the buffer.
    virtual void SetAllocation(GLuint id, ptrdiff_t size);

    /// Returns the gpu address (if available. otherwise returns 0).
    uint64_t GetGPUAddress() const { return _gpuAddr; }

    /// Returns the texture buffer view
    HD_API
    GLuint GetTextureBuffer();

    /// Returns the GLSL type name string of this resource
    /// to be used in codegen.
    HD_API
    TfToken GetGLTypeName() const;

private:
    int _glDataType;
    short _numComponents;
    int _arraySize;
    int _offset;
    int _stride;
    uint64_t _gpuAddr;
    GLuint _texId;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_BUFFER_RESOURCE_H
