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
#ifndef HD_GEOMETRIC_SHADER_H
#define HD_GEOMETRIC_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/shaderCode.h"
#include "pxr/imaging/hd/shaderKey.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/glf/glslfx.h"

#include <boost/scoped_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class Hd_GeometricShader> Hd_GeometricShaderSharedPtr;

/// \class Hd_GeometricShader
///
/// A geometric shader -- hydra internal use
///
class Hd_GeometricShader : public HdShaderCode {
public:
    Hd_GeometricShader(std::string const &glslfxString,
                       int16_t primitiveMode, /*=GLenum*/
                       int16_t primitiveIndexSize,
                       HdCullStyle cullStyle,
                       HdPolygonMode polygonMode,
                       bool cullingPass,
                       SdfPath const &debugId=SdfPath());

    virtual ~Hd_GeometricShader();

    // HdShader overrides
    virtual ID ComputeHash() const;
    virtual std::string GetSource(TfToken const &shaderStageKey) const;
    virtual void BindResources(Hd_ResourceBinder const &binder, int program);
    virtual void UnbindResources(Hd_ResourceBinder const &binder, int program);
    virtual void AddBindings(HdBindingRequestVector *customBindings);

    /// Returns true if this geometric shader is used for GPU frustum culling.
    bool IsCullingPass() const {
        return _cullingPass;
    }

    /// Return the primitive type of this draw item.
    GLenum GetPrimitiveMode() const {
        return _primitiveMode;
    }
    /// Return the index size of this draw item.
    int GetPrimitiveIndexSize() const {
        return _primitiveIndexSize;
    }

    /// template factory for convenience
    template <typename KEY>
    static Hd_GeometricShaderSharedPtr Create(KEY const &shaderKey) {
        HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();
        HdInstance<HdShaderKey::ID, Hd_GeometricShaderSharedPtr> geometricShaderInstance;

        // lookup registry
        std::unique_lock<std::mutex> regLock =
            resourceRegistry->RegisterGeometricShader(
                HdShaderKey::ComputeHash(shaderKey), &geometricShaderInstance);

        if (geometricShaderInstance.IsFirstInstance()) {
            geometricShaderInstance.SetValue(
                Hd_GeometricShaderSharedPtr(
                    new Hd_GeometricShader(
                        HdShaderKey::GetGLSLFXString(shaderKey),
                        shaderKey.GetPrimitiveMode(),
                        shaderKey.GetPrimitiveIndexSize(),
                        shaderKey.GetCullStyle(),
                        shaderKey.GetPolygonMode(),
                        shaderKey.IsCullingPass())));
        }
        return geometricShaderInstance.GetValue();
    }

private:
    // one of the following:
    // GL_POINTS          = 0x0
    // GL_LINES           = 0x1
    // GL_TRIANGLES       = 0x4
    // GL_LINES_ADJACENCY = 0xA
    // GL_PATCHES         = 0xE
    int16_t _primitiveMode;
    // 3 for triangles, 4 for quads, 16 for regular b-spline patches etc.
    int16_t _primitiveIndexSize;

    HdCullStyle _cullStyle;
    HdPolygonMode _polygonMode;
    // depth offset?

    boost::scoped_ptr<GlfGLSLFX> _glslfx;
    bool _cullingPass;
    ID _hash;

    // No copying
    Hd_GeometricShader(const Hd_GeometricShader &)                     = delete;
    Hd_GeometricShader &operator =(const Hd_GeometricShader &)         = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_GEOMETRIC_SHADER_H
