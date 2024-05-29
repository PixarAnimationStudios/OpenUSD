//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// uniformBlock.cpp
//

#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/uniformBlock.h"
#include "pxr/imaging/glf/bindingMap.h"
#include "pxr/imaging/glf/glContext.h"

PXR_NAMESPACE_OPEN_SCOPE


GlfUniformBlock::GlfUniformBlock(char const *label) :
    _buffer(0), _size(0)
{
    glGenBuffers(1, &_buffer);
    if (label) {
        // Using 'glObjectLabel' is only guaranteed to work on GL resources that
        // have been created. glGenBuffers only reserves an id.
        // Postpone setting up the debug label until buffer binding.
        _debugLabel = label;
    }
}

GlfUniformBlock::~GlfUniformBlock()
{
    GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

    if (glIsBuffer(_buffer) == GL_TRUE) {
        glDeleteBuffers(1, &_buffer);
    }
}

GlfUniformBlockRefPtr
GlfUniformBlock::New(char const *label)
{
    return TfCreateRefPtr(new GlfUniformBlock(label));
}

void
GlfUniformBlock::Bind(GlfBindingMapPtr const & bindingMap,
                       std::string const & identifier)
{
    if (!bindingMap) return;
    int binding = bindingMap->GetUniformBinding(identifier);

    glBindBufferBase(GL_UNIFORM_BUFFER, binding, _buffer);

    // Binding the buffer should ensure it is created so we can assign debug lbl
    if (!_debugLabel.empty()) {
        GlfDebugLabelBuffer(_buffer, _debugLabel.c_str());
    }
}

void
GlfUniformBlock::Update(const void *data, int size)
{
    GLF_GROUP_FUNCTION();
    
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

PXR_NAMESPACE_CLOSE_SCOPE

