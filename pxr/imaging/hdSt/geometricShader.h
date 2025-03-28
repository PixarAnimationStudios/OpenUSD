//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_GEOMETRIC_SHADER_H
#define PXR_IMAGING_HD_ST_GEOMETRIC_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/usd/sdf/path.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdSt_GeometricShaderSharedPtr =
    std::shared_ptr<class HdSt_GeometricShader>;
using HdStResourceRegistrySharedPtr =
    std::shared_ptr<class HdStResourceRegistry>;
struct HdSt_ShaderKey;
class HioGlslfx;

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
        PRIM_MESH_COARSE_TRIQUADS,   // e.g: triangulated quadrangulation
        PRIM_MESH_REFINED_TRIQUADS,  // e.g: triangulated catmark/bilinear
        PRIM_MESH_BSPLINE,           // e.g. catmark limit surface patches
        PRIM_MESH_BOXSPLINETRIANGLE, // e.g. loop limit surface patches
        PRIM_VOLUME,                 // Triangles of bounding box of a volume.
        PRIM_COMPUTE                 // A compute shader, e.g frustum culling
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
                primType == PrimitiveType::PRIM_MESH_COARSE_TRIQUADS   ||
                primType == PrimitiveType::PRIM_MESH_REFINED_TRIQUADS  ||
                primType == PrimitiveType::PRIM_MESH_BSPLINE           ||
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

    static inline bool IsPrimTypeTriQuads(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_MESH_COARSE_TRIQUADS ||
                primType == PrimitiveType::PRIM_MESH_REFINED_TRIQUADS);
    }

    static inline bool IsPrimTypeRefinedMesh(PrimitiveType primType) {
        return (primType == PrimitiveType::PRIM_MESH_REFINED_TRIANGLES ||
                primType == PrimitiveType::PRIM_MESH_REFINED_QUADS     ||
                primType == PrimitiveType::PRIM_MESH_REFINED_TRIQUADS  ||
                primType == PrimitiveType::PRIM_MESH_BSPLINE           ||
                primType == PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE);
    }

    static inline bool IsPrimTypePatches(PrimitiveType primType) {
        return primType == PrimitiveType::PRIM_MESH_BSPLINE ||
               primType == PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE ||
               primType == PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES ||
               primType == PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES;
    }

    static inline bool IsPrimTypeCompute(PrimitiveType primType) {
        return primType == PrimitiveType::PRIM_COMPUTE;
    }

    // Face-varying patch type
    enum class FvarPatchType { 
        PATCH_COARSE_TRIANGLES,  
        PATCH_REFINED_TRIANGLES,
        PATCH_COARSE_QUADS,
        PATCH_REFINED_QUADS,
        PATCH_BSPLINE,
        PATCH_BOXSPLINETRIANGLE,
        PATCH_NONE
    }; 

    HDST_API
    HdSt_GeometricShader(std::string const &glslfxString,
                       PrimitiveType primType,
                       HdCullStyle cullStyle,
                       bool useHardwareFaceCulling,
                       bool hasMirroredTransform,
                       bool doubleSided,
                       bool useMetalTessellation,
                       HdPolygonMode polygonMode,
                       bool cullingPass,
                       FvarPatchType fvarPatchType,
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
                       HdSt_ResourceBinder const &binder) override;
    
    HDST_API
    void UnbindResources(int program,
                         HdSt_ResourceBinder const &binder) override;
    HDST_API
    void AddBindings(HdStBindingRequestVector *customBindings) override;

    /// Returns true if this geometric shader is used for GPU frustum culling.
    bool IsFrustumCullingPass() const {
        return _frustumCullingPass;
    }

    PrimitiveType GetPrimitiveType() const {
        return _primType;
    }

    bool GetUseMetalTessellation() const {
        return _useMetalTessellation;
    }

    float GetLineWidth() const {
        return _lineWidth;
    }

    HdPolygonMode GetPolygonMode() const {
        return _polygonMode;
    }

    /// member query functions for PrimitiveType
    bool IsPrimTypePoints() const {
        return IsPrimTypePoints(_primType);
    }

    bool IsPrimTypeBasisCurves() const {
        return IsPrimTypeBasisCurves(_primType);
    }

    bool IsPrimTypeMesh() const {
        return IsPrimTypeMesh(_primType);
    }

    bool IsPrimTypeTriangles() const {
        return IsPrimTypeTriangles(_primType);
    }

    bool IsPrimTypeQuads() const {
        return IsPrimTypeQuads(_primType);
    }

    bool IsPrimTypeTriQuads() const {
        return IsPrimTypeTriQuads(_primType);
    }

    bool IsPrimTypeRefinedMesh() const {
        return IsPrimTypeRefinedMesh(_primType);
    }

    bool IsPrimTypePatches() const {
        return IsPrimTypePatches(_primType);
    }

    bool IsPrimTypeCompute() const {
        return IsPrimTypeCompute(_primType);
    }

    FvarPatchType GetFvarPatchType() const {
        return _fvarPatchType;
    }

    // Returns the primitive index size based on the primitive type
    // 3 for triangles, 4 for quads, 16 for regular b-spline patches etc.
    HDST_API
    int GetPrimitiveIndexSize() const;

    // Returns the number of vertices output for patch evaluation,
    // i.e. the number of tessellation control shader invocations.
    HDST_API
    int GetNumPatchEvalVerts() const;

    // Returns the primitive index size for the geometry shader shade
    // 1 for points, 2 for lines, 3 for triangles, 4 for lines_adjacency    
    HDST_API
    int GetNumPrimitiveVertsForGeometryShader() const;

    // Returns the HgiPrimitiveType for the primitive type.
    HDST_API
    HgiPrimitiveType GetHgiPrimitiveType() const;

    // Resolve the cull mode from the cull style in the render state.
    HDST_API
    HgiCullMode ResolveCullMode(HdCullStyle const renderStateCullStyle) const;

    // Factory for convenience.
    HDST_API
    static HdSt_GeometricShaderSharedPtr Create(
            HdSt_ShaderKey const &shaderKey, 
            HdStResourceRegistrySharedPtr const &resourceRegistry);

private:
    PrimitiveType _primType;
    HdCullStyle _cullStyle;
    bool _useHardwareFaceCulling;
    bool _hasMirroredTransform;
    bool _doubleSided;
    bool _useMetalTessellation;
    HdPolygonMode _polygonMode;
    float _lineWidth;

    std::unique_ptr<HioGlslfx> _glslfx;
    bool _frustumCullingPass;
    FvarPatchType _fvarPatchType;
    ID _hash;

    // No copying
    HdSt_GeometricShader(const HdSt_GeometricShader &) = delete;
    HdSt_GeometricShader &operator =(const HdSt_GeometricShader &) = delete;

    HioGlslfx const * _GetGlslfx() const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_GEOMETRIC_SHADER_H
