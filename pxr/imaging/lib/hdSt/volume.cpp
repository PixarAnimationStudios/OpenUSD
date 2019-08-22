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
#include "pxr/imaging/hdSt/volumeShaderKey.h"
#include "pxr/imaging/hdSt/textureResourceHandle.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/glf/vdbTexture.h"
#include "pxr/imaging/glf/contextCaps.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStVolume::HdStVolume(SdfPath const& id, SdfPath const & instancerId)
    : HdVolume(id)
{
}

HdStVolume::~HdStVolume() = default;

HdDirtyBits 
HdStVolume::GetInitialDirtyBitsMask() const
{
    const int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyExtent
        | HdChangeTracker::DirtyPrimID
        | HdChangeTracker::DirtyRepr
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyInstanceIndex
        ;

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

// Fallback volume shader created from source in shaders/fallbackVolume.glslfx
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

// Helper to create GLSL code such as "HdGet_density(vec3 p)" for sampling
// a field.
struct FieldReaderCode
{
    // HdMaterialParam assumed to be of type HdMaterialParam::ParamTypeField.
    //
    // Generates function HdGet_FIELDNAME() using HdGet_TEXTURENAME().
    FieldReaderCode(TfToken const &fieldName,
                    std::string const &textureName)
        : fieldName(fieldName),
          textureName(textureName)
    {
    }

    const TfToken fieldName;
    const std::string textureName;
};

// We take a similar approach to textures here are always return vec3.
// If the field asset contains a float, we would return a vec3 padded
// with zeros. It is up to the volume shader GLSL code to consume
// only the first component of the vec3 if it expects, e.g., density.
static const std::string glType = "vec3";

// myStream << FieldReaderCode(...);
// will create the actual code.
std::ostream & operator << (std::ostream &out,
                            const FieldReaderCode &code)
{
    out << "\n// Field reader\n";
    out << "\n" << glType;
    out << " HdGet_" << code.fieldName.GetString() << "(vec3 p)\n";
    out << "{\n";
    out << "     return vec3(HdGet_" << code.textureName << "(p).xyz);\n";
    out << "}\n\n";
    
    return out;
}

// Helper to create GLSL code such as "HdGet_density(vec3 p)" to return
// fallback value when field is not available
struct FallbackFieldReaderCode
{
    // Generates function HdGet_FIELDNAME() using HdGet_FALLBACKNAME().
    FallbackFieldReaderCode(TfToken const &fieldName,
                            TfToken const &fallbackName)
        : fieldName(fieldName),
          fallbackName(fallbackName)
    {
    }
    
    const TfToken fieldName;
    const TfToken fallbackName;
};

// myStream << FallbackFieldReaderCode(...);
// will create the actual code.
std::ostream & operator << (std::ostream &out,
                            const FallbackFieldReaderCode &code)
{
    out << "\n// Field reader (using fallback)\n";
    out << "\n" << glType;
    
    out << " HdGet_" << code.fieldName.GetString() << "(vec3 p)\n";
    out << "{\n";
    out << "     return vec3(HdGet_" << code.fallbackName.GetString()
        << "().xyz);\n";
    out << "}\n\n";
    
    return out;
}

} // end namespace

// Add GLSL code such as "HdGet_density(vec3 p)" for sampling the fields
// to the volume shader code and add necessary 3d textures and other
// parameters to the result HdStSurfaceShader.
// HdMaterialParam's are consulted to figure out the names of the fields
// to sample and the names of the associated sampling functions to generate.
//
// We actually have not implemented sampling from an OpenVDB file yet.
// Instead dummy functions giving a non-constant value are generated
// to enable some amount of testing.
HdStShaderCodeSharedPtr
HdStVolume::_ComputeMaterialShader(
    HdSceneDelegate * const sceneDelegate,
    const HdStMaterial * const material,
    const HdStShaderCodeSharedPtr &volumeShader,
    const _NameToFieldResource &nameToFieldResource)
{
    HdStResourceRegistrySharedPtr resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    // Generate new shader from volume shader
    HdStSurfaceShaderSharedPtr const result =
        boost::make_shared<HdStSurfaceShader>();

    // The GLSL code for the new shader
    std::stringstream glsl;
    // The params for the new shader
    HdMaterialParamVector materialParams;
    // The sources and texture descriptors for the new shader
    HdSt_MaterialBufferSourceAndTextureHelper sourcesAndTextures;

    // Carry over existing texture descriptors... Might not be useful.
    sourcesAndTextures.textures = volumeShader->GetTextures();

    // Scan old parameters...
    for (const auto & param : volumeShader->GetParams()) {
        if (param.IsField()) {
            // Process field readers.

            // Determine the field name the field reader requests
            TfTokenVector const &samplerCoordinates =
                param.GetSamplerCoordinates();
            const TfToken fieldName =
                samplerCoordinates.empty() ? TfToken() : samplerCoordinates[0];

            // Get the field resource associated with the field name
            const auto it = nameToFieldResource.find(fieldName);
            HdStFieldResourceSharedPtr const & fieldResource =
                it == nameToFieldResource.end()
                ? HdStFieldResourceSharedPtr()
                : it->second;
                
            if (fieldResource) {
                // Create a new HdMaterialParam such that the resource binder
                // will bind the 3d texture underling the field resource and
                // codegen will give us an accessor
                //     vec3 HdGet_FIELDNAMETexture(vec3)
                // to sample it.

                const std::string textureName = fieldName.GetString() + "Texture";

                const HdMaterialParam textureParam(
                    HdMaterialParam::ParamTypeTexture,
                    TfToken(textureName),
                    VtValue(GfVec3d(0.0)),
                    SdfPath(),
                    TfTokenVector(),
                    HdTextureType::Field);

                sourcesAndTextures.ProcessTextureMaterialParam(
                    textureParam,
                    boost::make_shared<HdStTextureResourceHandle>(
                        fieldResource));

                materialParams.push_back(textureParam);

                // TODO:
                // Consume fieldResource->GetBoundingBox() to compute local space
                // to sampling coordinate transform
                // Add HdMaterialParam so that we get an accessor
                //     mat4 HdGet_FIELDNAMETransform()
                //

                // Generate GLSL function HdGet_FIELDNAME(vec3) to sample the
                // field using HdGet_FIELDNAMETexture() and
                // HdGet_FIELDNAMETransform().
                glsl << FieldReaderCode(param.GetName(), textureName);
            } else {
                // No such field, so use the fallback value authored on the
                // field reader node.
                //
                // Create a new HdMaterialParam such that codegen will give us an
                // accessor
                //     vec3 HdGet_FIELDNAMEFallback()
                // to get the fallback value.
                const TfToken fallbackName(
                    param.GetName().GetString() + "Fallback");

                const HdMaterialParam fallbackParam(
                    HdMaterialParam::ParamTypeFallback,
                    fallbackName,
                    param.GetFallbackValue());

                sourcesAndTextures.ProcessFallbackMaterialParam(
                    fallbackParam, param.GetFallbackValue());

                materialParams.push_back(fallbackParam);
                
                // Generate GLSL function HdGet_FIELDNAME(vec3) simply returning
                // the fallback value.
                glsl << FallbackFieldReaderCode(param.GetName(), fallbackName);
            }
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
                    param, sceneDelegate, material->GetId());
            }
        }
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

const VtValue &
_GetCubeVertices()
{
    static const VtValue result(
        VtVec3fArray{
                GfVec3f(0, 0, 0),
                GfVec3f(0, 0, 1),
                GfVec3f(0, 1, 0),
                GfVec3f(0, 1, 1),
                GfVec3f(1, 0, 0),
                GfVec3f(1, 0, 1),
                GfVec3f(1, 1, 0),
                GfVec3f(1, 1, 1)});
    
    return result;
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
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationConstant);
    HdStPopulateConstantPrimvars(this, &_sharedData, sceneDelegate, drawItem, 
        dirtyBits, constantPrimvars);

    /* FIELDS */
    const _NameToFieldResource nameToFieldResource =
        _ComputeNameToFieldResource(sceneDelegate);

    /* MATERIAL SHADER */
    const HdStMaterial * const material = static_cast<const HdStMaterial *>(
        sceneDelegate->GetRenderIndex().GetSprim(
            HdPrimTypeTokens->material, GetMaterialId()));

    HdStShaderCodeSharedPtr const volumeShader = _ComputeVolumeShader(material);

    // Compute the material shader by adding GLSL code such as
    // "HdGet_density(vec3 p)" for sampling the fields needed by the volume
    // shader.
    // The material shader will eventually be concatenated with
    // the geometry shader which does the raymarching and is calling into
    // GLSL functions such as "float scattering(vec3)" in the volume shader
    // to evaluate physical properties of a volume at the point p.
    drawItem->SetMaterialShader(
        _ComputeMaterialShader(sceneDelegate,
                               material,
                               volumeShader,
                               nameToFieldResource));

    HdSt_VolumeShaderKey shaderKey;
    HdStResourceRegistrySharedPtr resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());
    drawItem->SetGeometricShader(
        HdSt_GeometricShader::Create(shaderKey, resourceRegistry));

    /* VERTICES */
    {
        // XXX:
        // Always the same vertices, should they be allocated only
        // once and shared across all volumes?
        HdBufferSourceSharedPtr source(
            new HdVtBufferSource(HdTokens->points, _GetCubeVertices()));

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
