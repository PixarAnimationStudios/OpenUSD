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

#ifndef PXR_IMAGING_GLF_TEXTURE_HANDLE_H
#define PXR_IMAGING_GLF_TEXTURE_HANDLE_H

/// \file glf/textureHandle.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/texture.h"

#include "pxr/imaging/garch/gl.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"

#include <string>
#include <map>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfTextureHandle);

class GlfTextureHandle : public TfRefBase, public TfWeakBase {
public:
    GLF_API
    static GlfTextureHandleRefPtr New(GlfTextureRefPtr texture);

    GLF_API
    virtual ~GlfTextureHandle();

    GlfTexturePtr GetTexture() {
        return _texture;
    }

    GLF_API
    void AddMemoryRequest(size_t targetMemory);

    GLF_API
    void DeleteMemoryRequest(size_t targetMemory);

protected:
    GLF_API
    GlfTextureHandle(GlfTextureRefPtr texture);

    GlfTextureRefPtr _texture;

    GLF_API
    void _ComputeMemoryRequirement();

    // requested memory map
    std::map<size_t, size_t> _requestedMemories;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
