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
// uniformBlock.cpp
//

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/uniformBlock.h"
#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/glContext.h"

GlfUniformBlock::GlfUniformBlock() :
    _buffer(0), _size(0)
{
    glGenBuffers(1, &_buffer);
}

GlfUniformBlock::~GlfUniformBlock()
{
    GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

    if (_buffer) glDeleteBuffers(1, &_buffer);
}

GlfUniformBlockRefPtr
GlfUniformBlock::New()
{
    return TfCreateRefPtr(new GlfUniformBlock());
}

void
GlfUniformBlock::Bind(GlfBindingMapPtr const & bindingMap,
                       std::string const & identifier)
{
    if (not bindingMap) return;
    int binding = bindingMap->GetUniformBinding(identifier);

    glBindBufferBase(GL_UNIFORM_BUFFER, binding, _buffer);
}

void
GlfUniformBlock::Update(const void *data, int size)
{
    glBindBuffer(GL_UNIFORM_BUFFER, _buffer);
    if (_size != size) {
        glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW);
        _size = size;
    }
    if (size > 0) {
        // Bug 95969 BufferSubData w/ size == 0 should be a noop but
        // raises errors on some NVIDIA drivers.
        glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
