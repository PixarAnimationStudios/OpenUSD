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
#include "pxr/imaging/hdSt/volume.h"

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/field.h"
#include "pxr/imaging/hdSt/materialBufferSourceAndTextureHelper.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/rprimUtils.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/volumeShader.h"
#include "pxr/imaging/hdSt/volumeShaderKey.h"
#include "pxr/imaging/hdSt/textureResourceHandle.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hf/diagnostic.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/glf/vdbTexture.h"
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (volumeBBoxInverseTransform)
    (volumeBBoxLocalMin)
    (volumeBBoxLocalMax)
);

TF_DEFINE_PRIVATE_TOKENS(
    _fallbackShaderTokens,

    (density)
    (emission)
);

const float HdStVolume::defaultStepSize         = 1.0f;
const float HdStVolume::defaultStepSizeLighting = 2.0f;

HdStVolume::HdStVolume(SdfPath const& id, SdfPath const & instancerId)
    : HdVolume(id)
{
}

HdStVolume::~HdStVolume() = default;

// Dirty bits requiring recomputing the material shader and the
// bounding box.
static const int _shaderAndBBoxComputationDirtyBitsMask =
    HdChangeTracker::Clean 
    | HdChangeTracker::DirtyExtent
    | HdChangeTracker::DirtyMaterialId
    | HdChangeTracker::DirtyRepr
    | HdChangeTracker::DirtyVolumeField;

static const int _initialDirtyBitsMask =
    _shaderAndBBoxComputationDirtyBitsMask
    | HdChangeTracker::DirtyPrimID
    | HdChangeTracker::DirtyPrimvar
    | HdChangeTracker::DirtyTransform
    | HdChangeTracker::DirtyVisibility;

HdDirtyBits 
HdStVolume::GetInitialDirtyBitsMask() const
{
    int mask = _initialDirtyBitsMask;

    if (!GetInstancerId().IsEmpty()) {
        mask |= HdChangeTracker::DirtyInstancer;
    }

    return (HdDirtyBits)mask;
}

HdDirtyBits 
HdStVolume::_PropagateDirtyBits(HdDirtyBits bits) const
{
    return bits;
}

void 
HdStVolume::_InitRepr(TfToken const &reprToken, HdDirtyBits* dirtyBits)
{
    // All representations point to _volumeRepr.
    if (!_volumeRepr) {
        _volumeRepr = HdReprSharedPtr(new HdRepr());
        HdDrawItem * const drawItem = new HdStDrawItem(&_sharedData);
        _volumeRepr->AddDrawItem(drawItem);
        *dirtyBits |= HdChangeTracker::NewRepr;
    }
    
    _ReprVector::iterator it = std::find_if(_reprs.begin(), _reprs.end(),
                                            _ReprComparator(reprToken));
    bool isNew = it == _reprs.end();
    if (isNew) {
        // add new repr
        it = _reprs.insert(_reprs.end(),
                std::make_pair(reprToken, _volumeRepr));
    }
}

void
HdStVolume::Sync(HdSceneDelegate *delegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits,
                 TfToken const   &reprToken)
{
    TF_UNUSED(renderParam);

    if (*dirtyBits & HdChangeTracker::DirtyMaterialId) {
        _SetMaterialId(delegate->GetRenderIndex().GetChangeTracker(),
                       delegate->GetMaterialId(GetId()));

        _sharedData.materialTag = _GetMaterialTag(delegate->GetRenderIndex());
    }

    _UpdateRepr(delegate, reprToken, dirtyBits);

    // This clears all the non-custom dirty bits. This ensures that the rprim
    // doesn't have pending dirty bits that add it to the dirty list every
    // frame.
    // XXX: GetInitialDirtyBitsMask sets certain dirty bits that aren't
    // reset (e.g. DirtyExtent, DirtyPrimID) that make this necessary.
    *dirtyBits &= ~HdChangeTracker::AllSceneDirtyBits;
}

const TfToken&
HdStVolume::_GetMaterialTag(const HdRenderIndex &renderIndex) const
{
    return HdStMaterialTagTokens->volume;
}

void
HdStVolume::_UpdateRepr(HdSceneDelegate *sceneDelegate,
                        TfToken const &reprToken,
                        HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdReprSharedPtr const &curRepr = _volumeRepr;

    if (TfDebug::IsEnabled(HD_RPRIM_UPDATED)) {
        HdChangeTracker::DumpDirtyBits(*dirtyBits);
    }

    HdStDrawItem * const drawItem = static_cast<HdStDrawItem*>(
        curRepr->GetDrawItem(0));

    if (HdChangeTracker::IsDirty(*dirtyBits)) {
        _UpdateDrawItem(sceneDelegate, drawItem, dirtyBits);
    }

    *dirtyBits &= ~HdChangeTracker::NewRepr;
}

HdStVolume::_NameToFieldResource
HdStVolume::_ComputeNameToFieldResource(
    HdSceneDelegate * const sceneDelegate)
{
    _NameToFieldResource result;

    const HdVolumeFieldDescriptorVector & fields =
        sceneDelegate->GetVolumeFieldDescriptors(GetId());

    for (const HdVolumeFieldDescriptor &field : fields) {
        if (const HdStField * const fieldPrim = 
                    dynamic_cast<HdStField*>(
                        sceneDelegate->GetRenderIndex().GetBprim(
                            field.fieldPrimType, field.fieldId))) {
            if (const HdStFieldResourceSharedPtr fieldResource =
                        fieldPrim->GetFieldResource()) {
                result[field.fieldName] = fieldResource;
            }
        }
    }

    return result;
}

namespace {

// Fallback volume shader created from shaders/fallbackVolume.glslfx
HdStShaderCodeSharedPtr
_MakeFallbackVolumeShader()
{
    using HioGlslfxSharedPtr = boost::shared_ptr<class HioGlslfx>;

    const HioGlslfx glslfx(HdStPackageFallbackVolumeShader());

    // Note that we use HdStSurfaceShader for a volume shader.
    // Despite its name, HdStSurfaceShader is really just a pair of
    // GLSL code and bindings and not specific to surface shading.
    HdStSurfaceShaderSharedPtr const result =
        boost::make_shared<HdStSurfaceShader>();
    
    result->SetFragmentSource(glslfx.GetVolumeSource());
    result->SetParams(
        {
            HdMaterialParam(
                HdMaterialParam::ParamTypeField,
                _fallbackShaderTokens->density,
                VtValue(GfVec3f(0.0, 0.0, 0.0)),
                SdfPath(),
                { _fallbackShaderTokens->density }),
            HdMaterialParam(
                HdMaterialParam::ParamTypeField,
                _fallbackShaderTokens->emission,
                VtValue(GfVec3f(0.0, 0.0, 0.0)),
                SdfPath(),
                { _fallbackShaderTokens->emission })});

    return result;
}

HdStShaderCodeSharedPtr
_ComputeVolumeShader(const HdStMaterial * const material)
{
    if (material) {
        // Use the shader from the HdStMaterial as volume shader.
        //
        // Note that rprims should query the material whether they want
        // a surface or volume shader instead of just asking for "some"
        // shader with HdStMaterial::GetShaderCode().
        // We can use HdStMaterial::GetShaderCode() here because the
        // UsdImagingGLHydraMaterialAdapter is following the outputs:volume
        // input of a material if the outputs:surface is unconnected.
        //
        // We should revisit the API an rprim is using to ask HdStMaterial
        // for a shader once we switched over to HdMaterialNetworkMap's.
        return material->GetShaderCode();
    } else {
        // Instantiate fallback volume shader only once
        //
        // Note that the default HdStMaterial provides a fallback surface
        // shader and we need a volume shader, so we create the shader here
        // ourselves.
        static const HdStShaderCodeSharedPtr fallbackVolumeShader =
            _MakeFallbackVolumeShader();
        return fallbackVolumeShader;
    }
}

// Compute transform mapping GfRange3d to unit box [0,1]^3
GfMatrix4d
_ComputeSamplingTransform(const GfRange3d &range)
{
    const GfVec3d size(range.GetSize());

    const GfVec3d scale(1.0 / size[0], 1.0 / size[1], 1.0 / size[2]);

    return
        // First map range so that min becomes (0,0,0)
        GfMatrix4d(1.0).SetTranslateOnly(-range.GetMin()) *
        // Then scale to unit box
        GfMatrix4d(1.0).SetScale(scale);
}

// Compute transform mapping bounding box to unit box [0,1]^3
GfMatrix4d
_ComputeSamplingTransform(const GfBBox3d &bbox)
{
    return
        // First map so that bounding box goes to its GfRange3d
        bbox.GetInverseMatrix() *
        // Then scale to unit box [0,1]^3
        _ComputeSamplingTransform(bbox.GetRange());
}

} // end namespace

// Add GLSL code such as "HdGet_density(vec3 p)" for sampling the fields
// to the volume shader code and add necessary 3d textures and other
// parameters to the result HdStSurfaceShader.
// HdMaterialParam's are consulted to figure out the names of the fields
// to sample and the names of the associated sampling functions to generate.
//
HdStShaderCodeSharedPtr
HdStVolume::_ComputeMaterialShaderAndBBox(
    HdSceneDelegate * const sceneDelegate,
    const HdStMaterial * const material,
    const HdStShaderCodeSharedPtr &volumeShader,
    const _NameToFieldResource &nameToFieldResource,
    GfBBox3d * const localVolumeBBox)
{
    // The bounding box containing all fields.
    GfBBox3d totalFieldBbox;
    // Does the material have any field readers.
    bool hasField = false;

    HdStResourceRegistrySharedPtr resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // Generate new shader from volume shader
    HdSt_VolumeShaderSharedPtr const result =
        boost::make_shared<HdSt_VolumeShader>(
            sceneDelegate->GetRenderIndex().GetRenderDelegate());

    // The GLSL code for the new shader
    std::stringstream glsl;
    // The params for the new shader
    HdMaterialParamVector materialParams;
    // The sources and texture descriptors for the new shader
    HdSt_MaterialBufferSourceAndTextureHelper sourcesAndTextures;

    // Carry over existing texture descriptors... Might not be useful.
    sourcesAndTextures.textures = volumeShader->GetTextures();

    std::set<TfToken> processedFieldNames;

    // Scan old parameters...
    for (const auto & param : volumeShader->GetParams()) {
        if (param.IsField()) {
            // Process field readers.
            hasField = true;

            // Determine the field name the field reader requests
            TfTokenVector const &samplerCoordinates =
                param.samplerCoords;
            const TfToken fieldName =
                samplerCoordinates.empty() ? TfToken() : samplerCoordinates[0];

            const TfToken textureName(
                fieldName.GetString() + "Texture");
            const TfToken samplingTransformName(
                fieldName.GetString() + "SamplingTransform");
            const TfToken fallbackName(
                param.name.GetString() + "Fallback");

            // Get the field resource associated with the field name if
            // field name was not seen before (two field readers could
            // access the same field).
            const auto it = nameToFieldResource.find(fieldName);
            if (it != nameToFieldResource.end()) {
                if (processedFieldNames.insert(fieldName).second) {
                    HdStFieldResourceSharedPtr const & fieldResource =
                        it->second;

                    // Add HdMaterialParam such that the resource binder
                    // will bind the 3d texture underling the field resource
                    // and codegen will give us an accessor
                    //     vec3 HdGet_FIELDNAMETexture(vec3)
                    // to sample it.
                    const HdMaterialParam textureParam(
                        HdMaterialParam::ParamTypeTexture,
                        textureName,
                        param.fallbackValue,
                        SdfPath(),
                        TfTokenVector(),
                        HdTextureType::Uvw);
                    
                    sourcesAndTextures.ProcessTextureMaterialParam(
                        textureParam,
                        boost::make_shared<HdStTextureResourceHandle>(
                        fieldResource));
                    
                    materialParams.push_back(textureParam);
                    
                    // Query the field for its bounding box.  Note
                    // that this contains both a GfRange3d and a
                    // matrix.
                    //
                    // For a grid in an OpenVDB file, the range is the
                    // bounding box of the active voxels of the tree
                    // and the matrix is the grid transform.
                    const GfBBox3d fieldBoundingBox =
                        fieldResource->GetBoundingBox();
                    
                    // Transform to map the bounding box to [0,1]^3.
                    const VtValue samplingTransform(
                        _ComputeSamplingTransform(fieldBoundingBox));
                    
                    // Add HdMaterialParam so that we get an accessor
                    //     mat4 HdGet_FIELDNAMESamplingTransform()
                    // converting local space to the coordinate at which
                    // we need to sample the 3d texture.
                    const HdMaterialParam samplingTransformParam(
                        HdMaterialParam::ParamTypeFallback,
                        samplingTransformName,
                        samplingTransform);
                    
                    sourcesAndTextures.ProcessFallbackMaterialParam(
                    samplingTransformParam, samplingTransform);
                    
                    materialParams.push_back(samplingTransformParam);
                    
                    // Update the bounding box containing all fields
                    totalFieldBbox = GfBBox3d::Combine(totalFieldBbox,
                                                       fieldBoundingBox);
                }
            }

            // Add HdMaterialParam such that codegen will give us an
            // accessor
            //     vec3 HdGet_NAMEFallback()
            // to get the fallback value.
            const HdMaterialParam fallbackParam(
                HdMaterialParam::ParamTypeFallback,
                fallbackName,
                param.fallbackValue);

            sourcesAndTextures.ProcessFallbackMaterialParam(
                fallbackParam, param.fallbackValue);

            materialParams.push_back(fallbackParam);
            
            // Add HdMaterialParam 
            
            const HdMaterialParam fieldRedirectParam(
                HdMaterialParam::ParamTypeFieldRedirect,
                param.name,
                param.fallbackValue,
                SdfPath(),
                { fieldName });
                
            materialParams.push_back(fieldRedirectParam);
        } else {
            // Push non-field params so that codegen will do generate
            // the respective code for them.
            materialParams.push_back(param);
            
            // Process non-field params similar to how they are handled in
            // HdStMaterial::Sync.
            if (param.IsPrimvar()) {
                sourcesAndTextures.ProcessPrimvarMaterialParam(param);
            } else if (param.IsFallback()) {
                sourcesAndTextures.ProcessFallbackMaterialParam(
                    param, param.fallbackValue);
            }
        }
    }

    // If there was a field reader, update the local volume bbox to be
    // the bounding box containing all fields.
    if (hasField) {
        *localVolumeBBox = totalFieldBbox;
    }

    // Use it to create HdGet_volumeBBoxInverseTransform(),
    // HdGet_volumeBBoxLocalMin() and HdGet_volumeBBoxLocalMax().
    //
    // These are used by the volume fragment shader to determine when
    // to stop ray marching (localVolumeBBox also gives the vertices
    // of the box that we draw to invoke the volume fragment shader).
    {
        // volume bounding box transform

        const HdMaterialParam transformParam(
            HdMaterialParam::ParamTypeFallback,
            _tokens->volumeBBoxInverseTransform,
            VtValue(localVolumeBBox->GetMatrix().GetInverse()));
        
        sourcesAndTextures.ProcessFallbackMaterialParam(
            transformParam, transformParam.fallbackValue);
        
        materialParams.push_back(transformParam);
    }

    {
        // volume bounding box min

        const HdMaterialParam minParam(
            HdMaterialParam::ParamTypeFallback,
            _tokens->volumeBBoxLocalMin,
            VtValue(localVolumeBBox->GetRange().GetMin()));
        
        sourcesAndTextures.ProcessFallbackMaterialParam(
            minParam, minParam.fallbackValue);
        
        materialParams.push_back(minParam);
    }

    {
        // volume bounding box max

        const HdMaterialParam maxParam(
            HdMaterialParam::ParamTypeFallback,
            _tokens->volumeBBoxLocalMax,
            VtValue(localVolumeBBox->GetRange().GetMax()));
        
        sourcesAndTextures.ProcessFallbackMaterialParam(
            maxParam, maxParam.fallbackValue);
        
        materialParams.push_back(maxParam);
    }

    // Append the volume shader (calling into the GLSL functions
    // generated above)
    glsl << volumeShader->GetSource(HdShaderTokens->fragmentShader);

    result->SetFragmentSource(glsl.str());
    result->SetParams(materialParams);
    result->SetTextureDescriptors(sourcesAndTextures.textures);
    result->SetBufferSources(sourcesAndTextures.sources, resourceRegistry);

    return result;
}

namespace {

VtValue
_GetCubeVertices(GfBBox3d const &bbox)
{
    const GfMatrix4d &transform = bbox.GetMatrix();
    const GfRange3d &range = bbox.GetRange();
    const bool isEmpty = range.IsEmpty();
    
    // Use vertices of a cube shrunk to point for empty bounding box
    // (to avoid min and max being large floating point numbers).

    const GfVec3d &min = isEmpty ? GfVec3d(0,0,0) : range.GetMin();
    const GfVec3d &max = isEmpty ? GfVec3d(0,0,0) : range.GetMax();

    const float minX = min[0];
    const float minY = min[1];
    const float minZ = min[2];

    const float maxX = max[0];
    const float maxY = max[1];
    const float maxZ = max[2];

    return VtValue(
        VtVec3fArray{
            transform.Transform(GfVec3f(minX, minY, minZ)),
            transform.Transform(GfVec3f(minX, minY, maxZ)),
            transform.Transform(GfVec3f(minX, maxY, minZ)),
            transform.Transform(GfVec3f(minX, maxY, maxZ)),
            transform.Transform(GfVec3f(maxX, minY, minZ)),
            transform.Transform(GfVec3f(maxX, minY, maxZ)),
            transform.Transform(GfVec3f(maxX, maxY, minZ)),
            transform.Transform(GfVec3f(maxX, maxY, maxZ))});
}

const VtValue &
_GetCubeTriangleIndices()
{
    static const VtValue result(
        VtVec3iArray{
                GfVec3i(2,3,1),
                GfVec3i(2,1,0),
                    
                GfVec3i(4,5,7),
                GfVec3i(4,7,6),

                GfVec3i(0,1,5),
                GfVec3i(0,5,4),
        
                GfVec3i(6,7,3),
                GfVec3i(6,3,2),

                GfVec3i(4,6,2),
                GfVec3i(4,2,0),

                GfVec3i(1,3,7),
                GfVec3i(1,7,5)});
    
    return result;
}

} // end namespace

void
HdStVolume::_UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                            HdStDrawItem *drawItem,
                            HdDirtyBits *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    /* VISIBILITY */
    _UpdateVisibility(sceneDelegate, dirtyBits);

    /* CONSTANT PRIMVARS, TRANSFORM AND EXTENT */
    const HdPrimvarDescriptorVector constantPrimvars =
        HdStGetPrimvarDescriptors(this, drawItem, sceneDelegate,
                                  HdInterpolationConstant);
    HdStPopulateConstantPrimvars(this, &_sharedData, sceneDelegate, drawItem, 
        dirtyBits, constantPrimvars);

    // The rest of this method is computing the material shader and the vertices
    // and topology of the bounding box. We can skip it unless the material, any
    // of the parameters or the fields have changed.
    //
    // XXX:
    // We might separate the material shader and bounding box computation and
    // do a finer grained sync.
    if (!((*dirtyBits) & _shaderAndBBoxComputationDirtyBitsMask)) {
        return;
    }

    /* FIELDS */
    const _NameToFieldResource nameToFieldResource =
        _ComputeNameToFieldResource(sceneDelegate);

    /* MATERIAL SHADER (may affect subsequent primvar population) */
    const HdStMaterial * const material = static_cast<const HdStMaterial *>(
        sceneDelegate->GetRenderIndex().GetSprim(
            HdPrimTypeTokens->material, GetMaterialId()));

    HdStShaderCodeSharedPtr const volumeShader = _ComputeVolumeShader(material);

    // The bounding box of the volume in the local frame (but not necessarily
    // aligned with the local frame).
    // 
    // It will be computed by _ComputeMaterialShader from the bounding boxes
    // of the fields. But if there is no field, it falls back to the extents
    // provided by the scene delegate for the volume prim.
    //
    const GfRange3d &extents = _sharedData.bounds.GetRange();
    GfBBox3d localVolumeBBox(extents);

    // Compute the material shader by adding GLSL code such as
    // "HdGet_density(vec3 p)" for sampling the fields needed by the volume
    // shader.
    // The material shader will eventually be concatenated with
    // the geometry shader which does the raymarching and is calling into
    // GLSL functions such as "float scattering(vec3)" in the volume shader
    // to evaluate physical properties of a volume at the point p.
    drawItem->SetMaterialShader(
        _ComputeMaterialShaderAndBBox(sceneDelegate,
                                      material,
                                      volumeShader,
                                      nameToFieldResource,
                                      &localVolumeBBox));

    // Note that then extents on the volume are with respect to the volume's
    // prim space but the localVolumeBBox might have an additional transform
    // (from the field).
    if (!(extents.IsEmpty() || extents.Contains(
              localVolumeBBox.ComputeAlignedRange()))) {
        HF_VALIDATION_WARN(
            GetId(),
            "Authored extents on volume prim should be updated since they do "
            "not contain volume (more precisely, they do not contain the "
            "bounding box computed from the fields associated with the "
            "volume)");
    }

    // Question: Should we transform localVolumeBBox to world space to update
    // update _sharedData.bounds if there was a field?
    
    HdSt_VolumeShaderKey shaderKey;
    HdStResourceRegistrySharedPtr resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());
    drawItem->SetGeometricShader(
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry));

    /* VERTICES */
    {
        HdBufferSourceSharedPtr const source =
            boost::make_shared<HdVtBufferSource>(
                HdTokens->points, _GetCubeVertices(localVolumeBBox));

        HdBufferSourceVector sources = { source };

        if (!drawItem->GetVertexPrimvarRange() ||
            !drawItem->GetVertexPrimvarRange()->IsValid()) {
            HdBufferSpecVector bufferSpecs;
            HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
            
            HdBufferArrayRangeSharedPtr range =
                resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());
            _sharedData.barContainer.Set(
                drawItem->GetDrawingCoord()->GetVertexPrimvarIndex(), range);
        }
        
        resourceRegistry->AddSources(drawItem->GetVertexPrimvarRange(),
                                     sources);
    }

    /* TRIANGLE INDICES */
    {
        // XXX:
        // Always the same triangle indices, should they be allocated only
        // once and shared across all volumes?
        HdBufferSourceSharedPtr source(
            new HdVtBufferSource(HdTokens->indices, _GetCubeTriangleIndices()));

        HdBufferSourceVector sources = { source };

        if (!drawItem->GetTopologyRange() ||
            !drawItem->GetTopologyRange()->IsValid()) {
            HdBufferSpecVector bufferSpecs;
            HdBufferSpec::GetBufferSpecs(sources, &bufferSpecs);
            
            HdBufferArrayRangeSharedPtr range =
                resourceRegistry->AllocateNonUniformBufferArrayRange(
                    HdTokens->primvar, bufferSpecs, HdBufferArrayUsageHint());
            _sharedData.barContainer.Set(
                drawItem->GetDrawingCoord()->GetTopologyIndex(), range);
        }
        
        resourceRegistry->AddSources(drawItem->GetTopologyRange(),
                                     sources);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
