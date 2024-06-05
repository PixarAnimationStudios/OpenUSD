//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GLF_UNIFORM_BLOCK_H
#define PXR_IMAGING_GLF_UNIFORM_BLOCK_H

/// \file glf/uniformBlock.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/garch/glApi.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfUniformBlock);
TF_DECLARE_WEAK_PTRS(GlfBindingMap);

/// \class GlfUniformBlock
///
/// Manages a GL uniform buffer object.
///
class GlfUniformBlock : public TfRefBase, public TfWeakBase {
public:

    /// Returns a new instance.
    GLF_API
    static GlfUniformBlockRefPtr New(char const *label = nullptr);

    GLF_API
    virtual ~GlfUniformBlock();

    /// Binds the uniform buffer using a bindingMap and identifier.
    GLF_API
    void Bind(GlfBindingMapPtr const & bindingMap,
              std::string const & identifier);

    /// Updates the content of the uniform buffer. If the size
    /// is different, the buffer will be reallocated.
    GLF_API
    void Update(const void *data, int size);
    
protected:
    GLF_API
    GlfUniformBlock(char const *label);

private:
    GLuint _buffer;
    int _size;
    std::string _debugLabel;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
