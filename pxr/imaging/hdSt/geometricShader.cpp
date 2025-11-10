//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/geometricShader.h"

#include "pxr/imaging/hdSt/binding.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/shaderKey.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/textureHandle.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/hash.h"

#include <iostream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


HdSt_GeometricShader::HdSt_GeometricShader(std::string const &glslfxString,
                                       PrimitiveType primType,
                                       HdCullStyle cullStyle,
                                       bool useHardwareFaceCulling,
                                       bool hasMirroredTransform,
                                       bool doubleSided,
                                       bool useMetalTessellation,
                                       HdPolygonMode polygonMode,
                                       bool cullingPass,
                                       FvarPatchType fvarPatchType,
                                       SdfPath const &debugId,
                                       float lineWidth)
    : HdStShaderCode()
    , _primType(primType)
    , _cullStyle(cullStyle)
    , _useHardwareFaceCulling(useHardwareFaceCulling)
    , _hasMirroredTransform(hasMirroredTransform)
    , _doubleSided(doubleSided)
    , _useMetalTessellation(useMetalTessellation)
    , _polygonMode(polygonMode)
    , _lineWidth(lineWidth)
    , _frustumCullingPass(cullingPass)
    , _fvarPatchType(fvarPatchType)
    , _hash(0)
    , _isValidComputedTextureSourceHash(false)
    , _isValidComputedHash(false)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX
    // we will likely move this (the constructor or the entire class) into
    // the base class (HdStShaderCode) at the end of refactoring, to be able to
    // use same machinery other than geometric shaders.

    if (TfDebug::IsEnabled(HDST_DUMP_GLSLFX_CONFIG)) {
        std::cout << debugId << "\n"
                  << glslfxString << "\n";
    }

    std::stringstream ss(glslfxString);
    _glslfx.reset(new HioGlslfx(ss));
    //
    // note: Don't include polygonMode into the hash.
    //       It is independent from the GLSL program.
    //
}

HdSt_GeometricShader::~HdSt_GeometricShader() = default;

/* virtual */
HioGlslfx const *
HdSt_GeometricShader::_GetGlslfx() const
{
    return _glslfx.get();
}

// Note: The geometric shader may override the state if necessary, including
// disabling h/w culling altogether.  This is required to handle instancing
// since instanceScale / instanceTransform can flip the xform handedness.
HgiCullMode
HdSt_GeometricShader::ResolveCullMode(
    HdCullStyle const renderStateCullStyle) const
{
    if (!_useHardwareFaceCulling) {
        // Use fragment shader culling via discard.
        return HgiCullModeNone;
    }

    // If the Rprim has an opinion, that wins, else use the render state style.
    HdCullStyle const resolvedCullStyle =
        _cullStyle == HdCullStyleDontCare ? renderStateCullStyle : _cullStyle;

    HgiCullMode resolvedCullMode = HgiCullModeNone;

    switch (resolvedCullStyle) {
        case HdCullStyleFront:
            if (_hasMirroredTransform) {
                resolvedCullMode = HgiCullModeBack;
            } else {
                resolvedCullMode = HgiCullModeFront;
            }
            break;
        case HdCullStyleFrontUnlessDoubleSided:
            if (!_doubleSided) {
                if (_hasMirroredTransform) {
                    resolvedCullMode = HgiCullModeBack;
                } else {
                    resolvedCullMode = HgiCullModeFront;
                }
            }
            break;
        case HdCullStyleBack:
            if (_hasMirroredTransform) {
                resolvedCullMode = HgiCullModeFront;
            } else {
                resolvedCullMode = HgiCullModeBack;
            }
            break;
        case HdCullStyleBackUnlessDoubleSided:
            if (!_doubleSided) {
                if (_hasMirroredTransform) {
                    resolvedCullMode = HgiCullModeFront;
                } else {
                    resolvedCullMode = HgiCullModeBack;
                }
            }
            break;
        case HdCullStyleNothing:
        default:
            resolvedCullMode = HgiCullModeNone;
            break;
    }

    return resolvedCullMode;
}

/* virtual */
HdStShaderCode::ID
HdSt_GeometricShader::ComputeHash() const
{
    // All mutator methods that might affect the hash must reset this (fragile).
    if (!_isValidComputedHash) {
        _hash = _ComputeHash();
        _isValidComputedHash = true;
    }
    return _hash;
}

HdStShaderCode::ID
HdSt_GeometricShader::_ComputeHash() const
{
    size_t hash = HdSt_MaterialParam::ComputeHash(_params);

    hash = TfHash::Combine(
        hash,
        _glslfx->GetHash(),
        _frustumCullingPass,
        _primType,
        _cullStyle,
        _useMetalTessellation,
        _fvarPatchType
    );

    return hash;
}

/* virtual */
std::string
HdSt_GeometricShader::GetSource(TfToken const &shaderStageKey) const
{
    return _glslfx->GetSource(shaderStageKey);
}

/*virtual*/
HdSt_MaterialParamVector const&
HdSt_GeometricShader::GetParams() const
{
    return _params;
}

/*virtual*/
HdStShaderCode::NamedTextureHandleVector const&
HdSt_GeometricShader::GetNamedTextureHandles() const
{
    return _namedTextureHandles;
}

/*virtual*/
void
HdSt_GeometricShader::BindResources(const int program,
                                    HdSt_ResourceBinder const &binder)
{
    HdSt_TextureBinder::BindResources(binder, _namedTextureHandles);
}

/*virtual*/
void
HdSt_GeometricShader::UnbindResources(const int program,
                                      HdSt_ResourceBinder const &binder)
{
    HdSt_TextureBinder::UnbindResources(binder, _namedTextureHandles);
}

/*virtual*/
void
HdSt_GeometricShader::AddBindings(HdStBindingRequestVector *customBindings)
{
    // no-op
}

int
HdSt_GeometricShader::GetPrimitiveIndexSize() const
{
    int primIndexSize = 1;

    switch (_primType)
    {
        case PrimitiveType::PRIM_POINTS:
            primIndexSize = 1;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_LINES:
        case PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES:
            primIndexSize = 2;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
        case PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
        case PrimitiveType::PRIM_VOLUME:
        case PrimitiveType::PRIM_DASH_DOT_LINES:
            primIndexSize = 3;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES:
        case PrimitiveType::PRIM_MESH_COARSE_QUADS:
        case PrimitiveType::PRIM_MESH_REFINED_QUADS:
            primIndexSize = 4;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_TRIQUADS:
        case PrimitiveType::PRIM_MESH_REFINED_TRIQUADS:
            primIndexSize = 6;
            break;
        case PrimitiveType::PRIM_MESH_BSPLINE:
            primIndexSize = 16;
            break;
        case PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
            primIndexSize = 12;
            break;
        case PrimitiveType::PRIM_COMPUTE:
            primIndexSize = 0;
            break;
    }

    return primIndexSize;
}

int
HdSt_GeometricShader::GetNumPatchEvalVerts() const
{
    int numPatchEvalVerts = 0;

    switch (_primType)
    {
        case PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES:
            numPatchEvalVerts = 2;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES:
            numPatchEvalVerts = 4;
            break;
        case PrimitiveType::PRIM_MESH_BSPLINE:
            numPatchEvalVerts = 16;
            break;
        case PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
            numPatchEvalVerts = 15;
            break;
        default:
            numPatchEvalVerts = 0;
            break;
    }

    return numPatchEvalVerts;
}

int
HdSt_GeometricShader::GetNumPrimitiveVertsForGeometryShader() const
{
    int numPrimVerts = 1;

    switch (_primType)
    {
        case PrimitiveType::PRIM_POINTS:
            numPrimVerts = 1;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_LINES:
            numPrimVerts = 2;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
        case PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
        case PrimitiveType::PRIM_MESH_COARSE_TRIQUADS:
        case PrimitiveType::PRIM_MESH_REFINED_TRIQUADS:
        case PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES:
        case PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES:
        case PrimitiveType::PRIM_DASH_DOT_LINES:
        case PrimitiveType::PRIM_MESH_BSPLINE:
        case PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
        // for patches with tesselation, input to GS is still a series of tris
        case PrimitiveType::PRIM_VOLUME:
            numPrimVerts = 3;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_QUADS:
        case PrimitiveType::PRIM_MESH_REFINED_QUADS:
            numPrimVerts = 4;
            break;
        case PrimitiveType::PRIM_COMPUTE:
            numPrimVerts = 0;
            break;
    }

    return numPrimVerts;
}

HgiPrimitiveType
HdSt_GeometricShader::GetHgiPrimitiveType() const
{
    HgiPrimitiveType primitiveType = HgiPrimitiveTypePointList;

    switch (GetPrimitiveType())
    {
        case PrimitiveType::PRIM_POINTS:
            primitiveType = HgiPrimitiveTypePointList;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_LINES:
            primitiveType = HgiPrimitiveTypeLineList;
            break;
        case PrimitiveType::PRIM_DASH_DOT_LINES:
        case PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
        case PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
        case PrimitiveType::PRIM_MESH_COARSE_TRIQUADS:
        case PrimitiveType::PRIM_MESH_REFINED_TRIQUADS:
            if (GetUseMetalTessellation()) {
                primitiveType = HgiPrimitiveTypePatchList;
            } else {
                primitiveType = HgiPrimitiveTypeTriangleList;
            }
            break;
        case PrimitiveType::PRIM_VOLUME:
            primitiveType = HgiPrimitiveTypeTriangleList;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_QUADS:
        case PrimitiveType::PRIM_MESH_REFINED_QUADS:
            if (GetUseMetalTessellation()) {
                primitiveType = HgiPrimitiveTypePatchList;
            } else {
                primitiveType = HgiPrimitiveTypeLineListWithAdjacency;
            }
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES:
        case PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES:
        case PrimitiveType::PRIM_MESH_BSPLINE:
        case PrimitiveType::PRIM_MESH_BOXSPLINETRIANGLE:
            primitiveType = HgiPrimitiveTypePatchList;
            break;
        case PrimitiveType::PRIM_COMPUTE:
            primitiveType = HgiPrimitiveTypePointList;
            break;
    }

    return primitiveType;
}

/*virtual*/
HdStShaderCode::ID
HdSt_GeometricShader::ComputeTextureSourceHash() const
{
    if (!_isValidComputedTextureSourceHash) {
        _computedTextureSourceHash = _ComputeTextureSourceHash();
        _isValidComputedTextureSourceHash = true;
    }
    return _computedTextureSourceHash;
}

HdStShaderCode::ID
HdSt_GeometricShader::_ComputeTextureSourceHash() const
{
    TRACE_FUNCTION();

    // To avoid excessive plumbing and checking of HgiCapabilities in order to
    // determine if bindless textures are enabled, we make things a little
    // easier for ourselves by having this function check and return 0 if
    // using bindless textures.
    const bool useBindlessHandles = _namedTextureHandles.empty() ? false :
        _namedTextureHandles[0].handles[0]->UseBindlessHandles();

    if (useBindlessHandles) {
        return 0;
    }

    size_t hash = 0;

    for (const HdStShaderCode::NamedTextureHandle& namedHandle :
        _namedTextureHandles) {

        // Use name, texture object and sampling parameters.
        hash = TfHash::Combine(hash, namedHandle.name, namedHandle.hash);
    }

    return hash;
}

void
HdSt_GeometricShader::_SetNamedTextureHandles(
    const NamedTextureHandleVector& namedTextureHandles)
{
    _namedTextureHandles = namedTextureHandles;
    _isValidComputedTextureSourceHash = false;
}

void
HdSt_GeometricShader::_SetParams(const HdSt_MaterialParamVector& params)
{
    _params = params;
    _isValidComputedHash = false;
}

/*virtual*/
void
HdSt_GeometricShader::AddResourcesFromTextures(ResourceContext& ctx) const
{
    const bool doublesSupported = ctx.GetResourceRegistry()->GetHgi()->
        GetCapabilities()->IsSet(
            HgiDeviceCapabilitiesBitsShaderDoublePrecision);

    // Add buffer sources for bindless texture handles (and
    // other texture metadata such as the sampling transform for
    // a field texture).
    HdBufferSourceSharedPtrVector result;
    HdSt_TextureBinder::ComputeBufferSources(
        GetNamedTextureHandles(), &result, doublesSupported);

    if (!result.empty()) {
        ctx.AddSources(GetShaderData(), std::move(result));
    }
}

namespace {
    size_t
    _GetGeometricShaderHash(
        HdSt_ShaderKey const& shaderKey,
        HdStShaderCode::NamedTextureHandleVector const& namedTextureHandles,
        HdSt_MaterialParamVector const& params)
    {
        size_t hash = shaderKey.ComputeHash();
        for (const HdStShaderCode::NamedTextureHandle& namedHandle :
            namedTextureHandles) {

            // Use name, texture object and sampling parameters.
            hash = TfHash::Combine(hash, namedHandle.name, namedHandle.hash);
        }
        if (!params.empty())
            hash = TfHash::Combine(hash, HdSt_MaterialParam::ComputeHash(params));

        return hash;
    }
}
/*static*/
HdSt_GeometricShaderSharedPtr
HdSt_GeometricShader::Create(
    HdSt_ShaderKey const &shaderKey, 
    NamedTextureHandleVector const& namedTextureHandles,
    HdSt_MaterialParamVector const& params,
    HdStResourceRegistrySharedPtr const &resourceRegistry)
{
    // Use the shaderKey hash to deduplicate geometric shaders.
    HdInstance<HdSt_GeometricShaderSharedPtr> geometricShaderInstance =
        resourceRegistry->RegisterGeometricShader(
            _GetGeometricShaderHash(shaderKey, namedTextureHandles, params));

    if (geometricShaderInstance.IsFirstInstance()) {
        geometricShaderInstance.SetValue(
            std::make_shared<HdSt_GeometricShader>(
                shaderKey.GetGlslfxString(),
                shaderKey.GetPrimitiveType(),
                shaderKey.GetCullStyle(),
                shaderKey.UseHardwareFaceCulling(),
                shaderKey.HasMirroredTransform(),
                shaderKey.IsDoubleSided(),
                shaderKey.UseMetalTessellation(),
                shaderKey.GetPolygonMode(),
                shaderKey.IsFrustumCullingPass(),
                shaderKey.GetFvarPatchType(),
                /*debugId=*/SdfPath(),
                shaderKey.GetLineWidth()));
        if(!namedTextureHandles.empty())
            geometricShaderInstance.GetValue()->_SetNamedTextureHandles(namedTextureHandles);
        if (!params.empty())
            geometricShaderInstance.GetValue()->_SetParams(params);
    }
    return geometricShaderInstance.GetValue();
}

PXR_NAMESPACE_CLOSE_SCOPE

