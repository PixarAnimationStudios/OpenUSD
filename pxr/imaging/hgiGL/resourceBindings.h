//
// Copyright 2020 Pixar
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
