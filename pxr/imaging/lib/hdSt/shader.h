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
#ifndef HDST_SHADER_H
#define HDST_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hd/shader.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdSurfaceShader> HdSurfaceShaderSharedPtr;

class HdStShader final: public HdShader {
public:
    HF_MALLOC_TAG_NEW("new HdStShader");

    HDST_API
    HdStShader(SdfPath const& id);
    HDST_API
    virtual ~HdStShader();

    /// Synchronizes state from the delegate to this object.
    HDST_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;

    /// Accessor for tasks to get the parameter cached in this sprim object.
    /// Don't communicate back to scene delegate within this function.
    HDST_API
    virtual VtValue Get(TfToken const &token) const override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HDST_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Causes the shader to be reloaded.
    HDST_API
    virtual void Reload() override;

    /// Obtains the render delegate specific representation of the shader.
    /// XXX: Should not be virtual.
    HDST_API
    virtual HdShaderCodeSharedPtr GetShaderCode() const override;

    /// Replaces the shader code object with an externally created one
    /// Used to set the fallback shader for prim.
    /// This class takes ownership of the passed in object.
    HDST_API
    void SetSurfaceShader(HdSurfaceShaderSharedPtr &shaderCode);


private:
    HdSurfaceShaderSharedPtr _surfaceShader;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_SHADER_H
