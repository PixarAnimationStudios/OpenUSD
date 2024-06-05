//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_RESOURCEBINDINGS_H
#define PXR_IMAGING_HGIGL_RESOURCEBINDINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/resourceBindings.h"
#include "pxr/imaging/hgiGL/api.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


///
/// \class HgiGLResourceBindings
///
/// OpenGL implementation of HgiResourceBindings.
///
///
class HgiGLResourceBindings final : public HgiResourceBindings
{
public:
    HGIGL_API
    ~HgiGLResourceBindings() override;

    /// Binds the resources to GPU.
    HGIGL_API
    void BindResources();

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLResourceBindings(HgiResourceBindingsDesc const& desc);

private:
    HgiGLResourceBindings() = delete;
    HgiGLResourceBindings & operator=(const HgiGLResourceBindings&) = delete;
    HgiGLResourceBindings(const HgiGLResourceBindings&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
