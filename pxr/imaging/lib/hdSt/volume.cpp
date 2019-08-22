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
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/rprimUtils.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdSt/volumeShaderKey.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hio/glslfx.h"

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

// Helper to create GLSL code such as "HdGet_density(vec3 p)" for sampling
// a field.
struct FieldReaderCode
{
    // HdMaterialParam assumed to be of type HdMaterialParam::ParamTypeField.
    FieldReaderCode(const HdMaterialParam &param)
        : name(param.GetName())
    {
    }

    const TfToken name;
};

// myStream << FieldReaderCode(...);
// will create the actual code.
std::ostream & operator << (std::ostream &out,
                            const FieldReaderCode &code)
{
    // We take a similar approach to textures here are always return vec3.
    // If the field asset contains a float, we would return a vec3 padded
    // with zeros. It is up to the volume shader GLSL code to consume
    // only the first component of the vec3 if it expects, e.g., density.
    static const std::string glType = "vec3";

    out << "\n// Field reader\n";
    out << "\n" << glType;
    out << " HdGet_" << code.name.GetString() << "(vec3 p)\n";
    out << "{\n";
    out << "    // Field reader reading from an actual OpenVDB file not\n";
    out << "    // implemented yet. Using non-constant function here for\n";
    out << "    // testing\n";
    out << "    return vec3(length(p) * length(p) * length(p) * 0.1);\n";
    out << "}\n\n";
    
    return out;
}

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
_ComputeMaterialShader(
    const HdStShaderCodeSharedPtr &volumeShader)
{
    // Generate new shader from volume shader
    HdStSurfaceShaderSharedPtr const result =
        boost::make_shared<HdStSurfaceShader>();

    // The GLSL code for the new shader
    std::stringstream glsl;
    // The params for the new shader
    HdMaterialParamVector materialParams;

    // Scan old parameters...
    for (const auto & param : volumeShader->GetParams()) {
        // ... for field readers
        if (param.IsField()) {
            // Generate (dummy) GLSL function to sample
            // that field.
            //
            // Eventually, we will consult the scene delegate's
            // GetVolumeFieldDescriptor to find the HdStField targeted
            // by the field- relationship named by
            // param.GetSamplerCoordinates()[0].
            // We will query the HdStField for the 3d texture and append
            // these data to the texture descriptors and also create params
            // so that codegen will produce a sampling function that we can
            // use in the GLSL function created here (node that for bindless,
            // we also need to add HdSt_BindlessSamplerBufferSource).
            // We will also query the HdStField for the OpenVDB grid transform
            // and added it to the result's buffer sources and params
            // so that our GLSL function can perform the necessary transforms
            // before sampling the 3d texture.
            glsl << FieldReaderCode(param);
        } else {
            // Do we need to copy the param.IsPrimvar() and param.IsFallback()
            // cases from HdStMaterial::Sync here to compute the respective
            // HdBufferSource's and call HdStSurfaceShader::SetBufferSources
            // below.
            materialParams.push_back(param);
        }
    }


    // Append the volume shader (calling into the GLSL functions
    // generated above)
    glsl << volumeShader->GetSource(HdShaderTokens->fragmentShader);

    result->SetFragmentSource(glsl.str());
    result->SetParams(materialParams);
    result->SetTextureDescriptors(volumeShader->GetTextures());

    // Missing: transfer or (re-)generate buffer sources.
    //
    // Note that these data are set with HdStSurfaceShader::SetBufferSources, but
    // HdStSurfaceShader::GetShaderData() gives a HdBufferArrayRange.
    // We might need to re-create the HdVtBufferSource's that HdStMaterial::Sync
    // created for the param.IsPrimvar() and param.IsFallback() case and pass them
    // to HdStSurfaceShader::SetBufferSource here.

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
    HdPrimvarDescriptorVector constantPrimvars =
        GetPrimvarDescriptors(sceneDelegate, HdInterpolationConstant);
    HdStPopulateConstantPrimvars(this, &_sharedData, sceneDelegate, drawItem, 
        dirtyBits, constantPrimvars);

    /* MATERIAL SHADER */
    const HdStMaterial *material = static_cast<const HdStMaterial *>(
        sceneDelegate->GetRenderIndex().GetSprim(
            HdPrimTypeTokens->material, GetMaterialId()));

    HdStShaderCodeSharedPtr volumeShader;
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
        volumeShader = material->GetShaderCode();
    } else {
        // Instantiate fallback volume shader only once
        //
        // Note that the default HdStMaterial provides a fallback surface
        // shader and we need a volume shader, so we create the shader here
        // ourselves.
        static const HdStShaderCodeSharedPtr fallbackVolumeShader =
            _MakeFallbackVolumeShader();
        volumeShader = fallbackVolumeShader;
    }

    // Compute the material shader by adding GLSL code such as
    // "HdGet_density(vec3 p)" for sampling the fields needed by the volume
    // shader.
    // The material shader will eventually be concatenated with
    // the geometry shader which does the raymarching and is calling into
    // GLSL functions such as "float scattering(vec3)" in the volume shader
    // to evaluate physical properties of a volume at the point p.
    drawItem->SetMaterialShader(
        _ComputeMaterialShader(volumeShader));

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
