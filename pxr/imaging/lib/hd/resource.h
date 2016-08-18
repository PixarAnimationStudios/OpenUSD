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
#ifndef HD_RESOURCE_H
#define HD_RESOURCE_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/imaging/garch/glext.h"
#include "pxr/base/tf/token.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<class HdResource> HdResourceSharedPtr;

/// \class HdResource
///
/// Base class for all GPU resource objects.
///
class HdResource : boost::noncopyable {
public:
    HdResource(TfToken const & role);
    virtual ~HdResource();

    /// Returns the role of the GPU data in this resource.
    TfToken const & GetRole() const {return _role;}

    /// The OpenGL name/identifier for this resource and its size
    virtual void SetAllocation(GLuint id, GLsizeiptr size);

    GLuint GetId() const {return _id;}
    GLsizeiptr GetSize() const {return _size;}

private:
    const TfToken _role;
    GLuint _id;
    GLsizeiptr _size;
};

#endif //HD_RESOURCE_H
