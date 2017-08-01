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
#include "pxr/imaging/hd/api.h"
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
    /// Used in Hd_CodeGen to generate the appropriate shader source 
    enum class PrimitiveType { 
        PRIM_POINTS, 
        PRIM_BASIS_CURVES_LINES,     // when linear (or) non-refined cubic
        PRIM_BASIS_CURVES_PATCHES,   // refined cubic curves
        PRIM_MESH_COARSE_TRIANGLES,  
        PRIM_MESH_REFINED_TRIANGLES, // e.g: loop subdiv
        PRIM_MESH_COARSE_QUADS,      // e.g: quadrangulation for ptex
        PRIM_MESH_REFINED_QUADS,     // e.g: catmark/bilinear subdiv
        PRIM_MESH_PATCHES
    };                                         

    /// static query functions for PrimitiveType
    static inline bool IsPrimTypePoints (PrimitiveType primType) {
        return primType == PrimitiveType::PRIM_POINTS;
    }

    static inline bool IsPrimTypeBasisCurves(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_BASIS_CURVES_LINES ||
                primType == PrimitiveType::PRIM_BASIS_CURVES_PATCHES);
    }

    static inline bool IsPrimTypeMesh(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_MESH_COARSE_TRIANGLES  ||
                primType == PrimitiveType::PRIM_MESH_REFINED_TRIANGLES ||
                primType == PrimitiveType::PRIM_MESH_COARSE_QUADS      ||
                primType == PrimitiveType::PRIM_MESH_REFINED_QUADS     ||
                primType == PrimitiveType::PRIM_MESH_PATCHES);
    }

    static inline bool IsPrimTypeTriangles(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_MESH_COARSE_TRIANGLES ||
                primType == PrimitiveType::PRIM_MESH_REFINED_TRIANGLES);
    }

    static inline bool IsPrimTypeQuads(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_MESH_COARSE_QUADS ||
                primType == PrimitiveType::PRIM_MESH_REFINED_QUADS);
    }

    static inline bool IsPrimTypePatches(PrimitiveType primType) {
        return primType == PrimitiveType::PRIM_MESH_PATCHES ||
               primType == PrimitiveType::PRIM_BASIS_CURVES_PATCHES;
    }

    HD_API
    Hd_GeometricShader(std::string const &glslfxString,
                       PrimitiveType primType,
                       HdCullStyle cullStyle,
                       HdPolygonMode polygonMode,
                       bool cullingPass,
                       SdfPath const &debugId=SdfPath());

    HD_API
    virtual ~Hd_GeometricShader();

    // HdShader overrides
    HD_API
    virtual ID ComputeHash() const;
    HD_API
    virtual std::string GetSource(TfToken const &shaderStageKey) const;
    HD_API
    virtual void BindResources(Hd_ResourceBinder const &binder, int program);
    HD_API
    virtual void UnbindResources(Hd_ResourceBinder const &binder, int program);
    HD_API
    virtual void AddBindings(HdBindingRequestVector *customBindings);

    /// Returns true if this geometric shader is used for GPU frustum culling.
    bool IsCullingPass() const {
        return _cullingPass;
    }

    PrimitiveType GetPrimitiveType() const {
        return _primType;
    }

    /// member query functions for PrimitiveType
     inline bool IsPrimTypePoints() const {
        return IsPrimTypePoints(_primType);
    }

    inline bool IsPrimTypeBasisCurves() const {
        return IsPrimTypeBasisCurves(_primType);
    }

    inline bool IsPrimTypeMesh() const {
        return IsPrimTypeMesh(_primType);
    }

    inline bool IsPrimTypeTriangles() const {
        return IsPrimTypeTriangles(_primType);
    }

    inline bool IsPrimTypeQuads() const {
        return IsPrimTypeQuads(_primType);
    }

    inline bool IsPrimTypePatches() const {
        return IsPrimTypePatches(_primType);
    }

    /// Return the GL primitive type of the draw item based on _primType
    GLenum GetPrimitiveMode() const;

    // Returns the primitive index size based on the primitive mode
    // 3 for triangles, 4 for quads, 16 for regular b-spline patches etc.
    int GetPrimitiveIndexSize() const;

    // Returns the primitive index size for the geometry shader shade
    // 1 for points, 2 for lines, 3 for triangles, 4 for lines_adjacency    
    int GetNumPrimitiveVertsForGeometryShader() const;

    /// template factory for convenience
    template <typename KEY>
    static Hd_GeometricShaderSharedPtr Create(
            KEY const &shaderKey, 
            HdResourceRegistrySharedPtr const &resourceRegistry) {

        HdInstance<HdShaderKey::ID, Hd_GeometricShaderSharedPtr> 
            geometricShaderInstance;

        // lookup registry
        std::unique_lock<std::mutex> regLock =
            resourceRegistry->RegisterGeometricShader(
                HdShaderKey::ComputeHash(shaderKey), &geometricShaderInstance);

        if (geometricShaderInstance.IsFirstInstance()) {
            geometricShaderInstance.SetValue(
                Hd_GeometricShaderSharedPtr(
                    new Hd_GeometricShader(
                        HdShaderKey::GetGLSLFXString(shaderKey),
                        shaderKey.GetPrimitiveType(),
                        shaderKey.GetCullStyle(),
                        shaderKey.GetPolygonMode(),
                        shaderKey.IsCullingPass())));
        }
        return geometricShaderInstance.GetValue();
    }

private:
    PrimitiveType _primType;
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
