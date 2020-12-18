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
#ifndef PXR_IMAGING_HD_ST_GEOMETRIC_SHADER_H
#define PXR_IMAGING_HD_ST_GEOMETRIC_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/garch/glApi.h"
#include "pxr/imaging/hio/glslfx.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_GeometricShaderSharedPtr =
    std::shared_ptr<class HdSt_GeometricShader>;
struct HdSt_ShaderKey;

/// \class HdSt_GeometricShader
///
/// Storm breaks down the concept of a shader program into distinct
/// conceptual pieces that are then stitched together during code generation.
/// The pieces are:
/// (i)   geometric shader
/// (ii)  material shader 
/// (iii) lighting shader
/// (iv)  render pass shader
///
/// The geometric shader contains the entry points for the relevant shader
/// stages and uses geometry opinions (such as cullstyle, double sided, etc)
/// to generate shader code variants via mixins.
///
class HdSt_GeometricShader : public HdStShaderCode {
public:
    /// Used in HdSt_CodeGen to generate the appropriate shader source 
    enum class PrimitiveType { 
        PRIM_POINTS, 
        PRIM_BASIS_CURVES_LINES,     // when linear (or) non-refined cubic
        PRIM_BASIS_CURVES_LINEAR_PATCHES,  // refined linear curves
        PRIM_BASIS_CURVES_CUBIC_PATCHES,   // refined cubic curves
        PRIM_MESH_COARSE_TRIANGLES,  
        PRIM_MESH_REFINED_TRIANGLES, // e.g: loop subdiv
        PRIM_MESH_COARSE_QUADS,      // e.g: quadrangulation for ptex
        PRIM_MESH_REFINED_QUADS,     // e.g: catmark/bilinear subdiv
        PRIM_MESH_BSPLINE,           // e.g. catmark limit surface patches
        PRIM_MESH_BOXSPLINETRIANGLE, // e.g. loop limit surface patches
        PRIM_VOLUME                  // Simply draws triangles of bounding
                                     // box of a volume.
    };                                         

    /// static query functions for PrimitiveType
    static inline bool IsPrimTypePoints (PrimitiveType primType) {
        return primType == PrimitiveType::PRIM_POINTS;
    }

    static inline bool IsPrimTypeBasisCurves(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_BASIS_CURVES_LINES ||
                primType == PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES ||
                primType == PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES);
    }

    static inline bool IsPrimTypeMesh(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_MESH_COARSE_TRIANGLES  ||
                primType == PrimitiveType::PRIM_MESH_REFINED_TRIANGLES ||
                primType == PrimitiveType::PRIM_MESH_COARSE_QUADS      ||
                primType == PrimitiveType::PRIM_MESH_REFINED_QUADS     ||
                primType == PrimitiveType::PRIM_MESH_BSPLINE   ||
                primType == PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE);
    }

    static inline bool IsPrimTypeTriangles(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_MESH_COARSE_TRIANGLES ||
                primType == PrimitiveType::PRIM_MESH_REFINED_TRIANGLES ||
                primType == PrimitiveType::PRIM_VOLUME);
    }

    static inline bool IsPrimTypeQuads(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_MESH_COARSE_QUADS ||
                primType == PrimitiveType::PRIM_MESH_REFINED_QUADS);
    }

    static inline bool IsPrimTypePatches(PrimitiveType primType) {
        return primType == PrimitiveType::PRIM_MESH_BSPLINE ||
               primType == PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE ||
               primType == PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES ||
               primType == PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES;
    }

    HDST_API
    HdSt_GeometricShader(std::string const &glslfxString,
                       PrimitiveType primType,
                       HdCullStyle cullStyle,
                       bool useHardwareFaceCulling,
                       bool hasMirroredTransform,
                       bool doubleSided,
                       HdPolygonMode polygonMode,
                       bool cullingPass,
                       SdfPath const &debugId = SdfPath(),
                       float lineWidth = 0);

    HDST_API
    ~HdSt_GeometricShader() override;

    // HdShader overrides
    HDST_API
    ID ComputeHash() const override;
    HDST_API
    std::string GetSource(TfToken const &shaderStageKey) const override;
    HDST_API
    void BindResources(int program,
                               HdSt_ResourceBinder const &binder,
                               HdRenderPassState const &state) override;
    
    HDST_API
    void UnbindResources(int program,
                                 HdSt_ResourceBinder const &binder,
                                 HdRenderPassState const &state) override;
    HDST_API
    void AddBindings(HdBindingRequestVector *customBindings) override;

    /// Returns true if this geometric shader is used for GPU frustum culling.
    bool IsFrustumCullingPass() const {
        return _frustumCullingPass;
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
    HDST_API
    GLenum GetPrimitiveMode() const;

    // Returns the primitive index size based on the primitive mode
    // 3 for triangles, 4 for quads, 16 for regular b-spline patches etc.
    HDST_API
    int GetPrimitiveIndexSize() const;

    // Returns the primitive index size for the geometry shader shade
    // 1 for points, 2 for lines, 3 for triangles, 4 for lines_adjacency    
    HDST_API
    int GetNumPrimitiveVertsForGeometryShader() const;

    // Factory for convenience.
    static HdSt_GeometricShaderSharedPtr Create(
            HdSt_ShaderKey const &shaderKey, 
            HdStResourceRegistrySharedPtr const &resourceRegistry);

private:
    PrimitiveType _primType;
    HdCullStyle _cullStyle;
    bool _useHardwareFaceCulling;
    bool _hasMirroredTransform;
    bool _doubleSided;
    HdPolygonMode _polygonMode;
    float _lineWidth;

    std::unique_ptr<HioGlslfx> _glslfx;
    bool _frustumCullingPass;
    ID _hash;

    // No copying
    HdSt_GeometricShader(const HdSt_GeometricShader &) = delete;
    HdSt_GeometricShader &operator =(const HdSt_GeometricShader &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_GEOMETRIC_SHADER_H
