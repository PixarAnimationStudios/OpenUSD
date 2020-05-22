//
// Copyright 2019 Pixar
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
#include "pxr/imaging/hdSt/volumeShader.h"

#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/volume.h"
#include "pxr/imaging/hdSt/field.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/textureBinder.h"
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (stepSize)
    (stepSizeLighting)

    (volumeBBoxInverseTransform)
    (volumeBBoxLocalMin)
    (volumeBBoxLocalMax)
);


HdSt_VolumeShader::HdSt_VolumeShader(HdRenderDelegate * const renderDelegate)
  : _renderDelegate(renderDelegate),
    _lastRenderSettingsVersion(0),
    _stepSize(HdStVolume::defaultStepSize),
    _stepSizeLighting(HdStVolume::defaultStepSizeLighting),
    _fillsPointsBar(false)
{
}


HdSt_VolumeShader::~HdSt_VolumeShader() = default;

void 
HdSt_VolumeShader::AddBindings(HdBindingRequestVector * const customBindings)
{
    HdStSurfaceShader::AddBindings(customBindings);
    customBindings->push_back(
        HdBindingRequest(
            HdBinding::UNIFORM,
            _tokens->stepSize,
            HdTypeFloat));
    customBindings->push_back(
        HdBindingRequest(
            HdBinding::UNIFORM,
            _tokens->stepSizeLighting,
            HdTypeFloat));
}

void 
HdSt_VolumeShader::BindResources(const int program,
                                 HdSt_ResourceBinder const &binder,
                                 HdRenderPassState const &state)
{
    HdStSurfaceShader::BindResources(program, binder, state);
    
    const int currentRenderSettingsVersion =
        _renderDelegate->GetRenderSettingsVersion();
    
    if (_lastRenderSettingsVersion != currentRenderSettingsVersion) {
        _lastRenderSettingsVersion = currentRenderSettingsVersion;
        _stepSize = _renderDelegate->GetRenderSetting<float>(
            HdStRenderSettingsTokens->volumeRaymarchingStepSize,
            HdStVolume::defaultStepSize);
        _stepSizeLighting = _renderDelegate->GetRenderSetting<float>(
            HdStRenderSettingsTokens->volumeRaymarchingStepSizeLighting,
            HdStVolume::defaultStepSizeLighting);
    }
    
    binder.BindUniformf(_tokens->stepSize, 1, &_stepSize);
    binder.BindUniformf(_tokens->stepSizeLighting, 1, &_stepSizeLighting);
}

void
HdSt_VolumeShader::UnbindResources(const int program,
                                   HdSt_ResourceBinder const &binder,
                                   HdRenderPassState const &state)
{
    HdStSurfaceShader::UnbindResources(program, binder, state);
}

void
HdSt_VolumeShader::SetPointsBar(HdBufferArrayRangeSharedPtr const &pointsBar)
{
    _pointsBar = pointsBar;
}

void
HdSt_VolumeShader::SetFillsPointsBar(const bool fillsPointsBar)
{
    _fillsPointsBar = fillsPointsBar;
}

static
TfToken
_ConcatFallback(const TfToken &token)
{
    return TfToken(
        token.GetString()
        + HdSt_ResourceBindingSuffixTokens->fallback.GetString());
}

void
HdSt_VolumeShader::GetParamsAndBufferSpecsForBBox(
    HdSt_MaterialParamVector * const params,
    HdBufferSpecVector * const specs)
{
    {
        params->emplace_back(
            HdSt_MaterialParam::ParamTypeFallback,
            _tokens->volumeBBoxInverseTransform,
            VtValue(GfMatrix4d()));

        static const TfToken sourceName(
            _ConcatFallback(_tokens->volumeBBoxInverseTransform));
        specs->emplace_back(
            sourceName,
            HdTupleType{HdTypeDoubleMat4, 1});
    }
     
    {
        params->emplace_back(
            HdSt_MaterialParam::ParamTypeFallback,
            _tokens->volumeBBoxLocalMin,
            VtValue(GfVec3d()));

        static const TfToken sourceName(
            _ConcatFallback(_tokens->volumeBBoxLocalMin));
        specs->emplace_back(
            sourceName,
            HdTupleType{HdTypeDoubleVec3, 1});
    }
     
    {
        params->emplace_back(
            HdSt_MaterialParam::ParamTypeFallback,
            _tokens->volumeBBoxLocalMax,
            VtValue(GfVec3d()));

        static const TfToken sourceName(
            _ConcatFallback(_tokens->volumeBBoxLocalMax));
        specs->emplace_back(
            sourceName,
            HdTupleType{HdTypeDoubleVec3, 1});
    }
}

void
HdSt_VolumeShader::GetBufferSourcesForBBox(
    const GfBBox3d &bbox,
    HdBufferSourceSharedPtrVector * const sources)
{
    const GfRange3d &range = bbox.GetRange();

    {
        static const TfToken sourceName(
            _ConcatFallback(_tokens->volumeBBoxInverseTransform));
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                sourceName,
                VtValue(bbox.GetInverseMatrix())));
    }

    {
        static const TfToken sourceName(
            _ConcatFallback(_tokens->volumeBBoxLocalMin));
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                sourceName,
                VtValue(GetSafeMin(range))));
    }

    {
        static const TfToken sourceName(
            _ConcatFallback(_tokens->volumeBBoxLocalMax));
        sources->push_back(
            std::make_shared<HdVtBufferSource>(
                sourceName,
                VtValue(GetSafeMax(range))));
    }
}

GfVec3d
HdSt_VolumeShader::GetSafeMin(const GfRange3d &range)
{
    if (range.IsEmpty()) {
        return GfVec3d(0.0, 0.0, 0.0);
    }
    return range.GetMin();
}

GfVec3d
HdSt_VolumeShader::GetSafeMax(const GfRange3d &range)
{
    if (range.IsEmpty()) {
        return GfVec3d(0.0, 0.0, 0.0);
    }
    return range.GetMax();
}

namespace {

// Compute the bounding box from all the fields in this volume.
GfBBox3d
_ComputeBBox(const HdStShaderCode::NamedTextureHandleVector &textures)
{
    GfBBox3d result;

    for (const HdStShaderCode::NamedTextureHandle &texture : textures) {
        HdStTextureObjectSharedPtr const &textureObject =
            texture.handle->GetTextureObject();

        if (const HdStFieldTextureObject * const fieldTex =
                dynamic_cast<HdStFieldTextureObject *>(
                    textureObject.get())) {
            result = GfBBox3d::Combine(result, fieldTex->GetBoundingBox());
        }
    }

    return result;
}

// Compute 8 vertices of a bounding box.

VtValue
_ComputePoints(const GfBBox3d &bbox)
{
    VtVec3fArray points(8);

    size_t i = 0;

    const GfMatrix4d &transform = bbox.GetMatrix();
    const GfRange3d &range = bbox.GetRange();

    // Use vertices of a cube shrunk to point for empty bounding box
    // (to avoid min and max being large floating point numbers).
    const GfVec3d min = HdSt_VolumeShader::GetSafeMin(range);
    const GfVec3d max = HdSt_VolumeShader::GetSafeMax(range);

    for (const double x : { min[0], max[0] }) {
        for (const double y : { min[1], max[1] }) {
            for (const double z : { min[2], max[2] }) {
                points[i] = GfVec3f(transform.Transform(GfVec3d(x, y, z)));
                i++;
            }
        }
    }
    
    return VtValue(points);
}

}  // end anonymous namespace

void
HdSt_VolumeShader::AddResourcesFromTextures(ResourceContext &ctx) const
{
    const bool bindlessTextureEnabled
        = GlfContextCaps::GetInstance().bindlessTextureEnabled;

    HdBufferSourceSharedPtrVector shaderBarSources;

    // Fills in sampling transforms for textures and also texture
    // handles for bindless textures.
    HdSt_TextureBinder::ComputeBufferSources(
        GetNamedTextureHandles(), bindlessTextureEnabled, &shaderBarSources);

    if (_fillsPointsBar) {
        // Compute volume bounding box from field bounding boxes
        const GfBBox3d bbox = _ComputeBBox(GetNamedTextureHandles());

        // Use as points
        ctx.AddSource(
            _pointsBar,
            std::make_shared<HdVtBufferSource>(
                HdTokens->points,
                _ComputePoints(bbox)));

        // And let the shader know for raymarching bounds.
        GetBufferSourcesForBBox(bbox, &shaderBarSources);
    }

    if (!shaderBarSources.empty()) {
        ctx.AddSources(GetShaderData(), std::move(shaderBarSources));
    }
}

void
HdSt_VolumeShader::SetFieldDescriptors(
    const HdVolumeFieldDescriptorVector & fieldDescs)
{
    _fieldDescriptors = fieldDescs;
}

void
HdSt_VolumeShader::UpdateTextureHandles(
    HdSceneDelegate * const sceneDelegate)
{
    TRACE_FUNCTION();

    HdStResourceRegistrySharedPtr const resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    NamedTextureHandleVector textureHandles = GetNamedTextureHandles();

    if (!TF_VERIFY(textureHandles.size() == _fieldDescriptors.size())) {
        return;
    }

    const bool bindlessTextureEnabled
        = GlfContextCaps::GetInstance().bindlessTextureEnabled;

    // Walk through the vector of named texture handles and field descriptors
    // simultaneously.
    for (size_t i = 0; i < textureHandles.size(); i++) {
        // To allocate the texture and update it in the vector ...

        // use field descriptor to find field prim, ...
        const HdVolumeFieldDescriptor &fieldDesc = _fieldDescriptors[i];

        const HdStField * const fieldPrim =
            dynamic_cast<HdStField*>(
                sceneDelegate->GetRenderIndex().GetBprim(
                    fieldDesc.fieldPrimType, fieldDesc.fieldId));

        // ask field prim for texture information, ...
        const HdStTextureIdentifier &textureId =
            TF_VERIFY(fieldPrim) ?
            fieldPrim->GetTextureIdentifier() : HdStTextureIdentifier();
        const HdTextureType textureType = textureHandles[i].type;
        const size_t textureMemory =
            TF_VERIFY(fieldPrim) ?
            fieldPrim->GetTextureMemory() : 0;
        static const HdSamplerParameters samplerParams{
            HdWrapBlack, HdWrapBlack, HdWrapBlack,
            HdMinFilterLinear, HdMagFilterLinear };
        
        // allocate texture handle and assign it.
        textureHandles[i].handle =
            resourceRegistry->AllocateTextureHandle(
                textureId,
                textureType,
                samplerParams,
                textureMemory,
                bindlessTextureEnabled,
                shared_from_this());
    }

    // And update!
    SetNamedTextureHandles(textureHandles);
}


PXR_NAMESPACE_CLOSE_SCOPE
