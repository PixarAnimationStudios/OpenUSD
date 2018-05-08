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
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSceneDelegate::HdSceneDelegate(HdRenderIndex *parentIndex,
                                 SdfPath const& delegateID)
    : _index(parentIndex)
    , _delegateID(delegateID)
{
    if (!_delegateID.IsAbsolutePath()) {
        TF_CODING_ERROR("Scene Delegate Id must be an absolute path: %s",
                        delegateID.GetText());


        _delegateID.MakeAbsolutePath(SdfPath::AbsoluteRootPath());
    }
}

HdSceneDelegate::~HdSceneDelegate()
{
}

/*virtual*/
void
HdSceneDelegate::Sync(HdSyncRequestVector* request)
{

}

void
HdSceneDelegate::PostSyncCleanup()
{
}

/*virtual*/
bool
HdSceneDelegate::IsEnabled(TfToken const& option) const
{
    if (option == HdOptionTokens->parallelRprimSync) {
        return true;
    }

    return false;
}

/*virtual*/
TfToken
HdSceneDelegate::GetRenderTag(SdfPath const& id, TfToken const& reprName)
{
    return HdTokens->geometry;
}

// -----------------------------------------------------------------------//
/// \name Rprim Aspects
// -----------------------------------------------------------------------//

/*virtual*/
HdMeshTopology
HdSceneDelegate::GetMeshTopology(SdfPath const& id)
{
    return HdMeshTopology();
}

/*virtual*/
HdBasisCurvesTopology
HdSceneDelegate::GetBasisCurvesTopology(SdfPath const& id)
{
    return HdBasisCurvesTopology();
}

/*virtual*/
PxOsdSubdivTags
HdSceneDelegate::GetSubdivTags(SdfPath const& id)
{
    return PxOsdSubdivTags();
}

/*virtual*/
GfRange3d
HdSceneDelegate::GetExtent(SdfPath const & id)
{
    return GfRange3d();
}

/*virtual*/
GfMatrix4d
HdSceneDelegate::GetTransform(SdfPath const & id)
{
    return GfMatrix4d();
}

/*virtual*/
size_t
HdSceneDelegate::SampleTransform(SdfPath const & id,
                                 size_t maxSampleCount,
                                 float *times,
                                 GfMatrix4d *samples)
{
    if (maxSampleCount > 0) {
        times[0] = 0;
        samples[0] = GetTransform(id);
        return 1;
    }
    return 0;
}

/*virtual*/
bool
HdSceneDelegate::GetVisible(SdfPath const & id)
{
    return true;
}

/*virtual*/
bool
HdSceneDelegate::GetDoubleSided(SdfPath const & id)
{
    return false;
}

/*virtual*/
HdCullStyle
HdSceneDelegate::GetCullStyle(SdfPath const &id)
{
    return HdCullStyleDontCare;
}

/*virtual*/
VtValue
HdSceneDelegate::GetShadingStyle(SdfPath const &id)
{
    return VtValue();
}

/*virtual*/
HdDisplayStyle
HdSceneDelegate::GetDisplayStyle(SdfPath const& id)
{
    return HdDisplayStyle();
}

/*virtual*/
VtValue
HdSceneDelegate::Get(SdfPath const& id, TfToken const& key)
{
    return VtValue();
}

/*virtual*/
size_t
HdSceneDelegate::SamplePrimvar(SdfPath const& id, TfToken const& key,
                               size_t maxSampleCount,
                               float *times,
                               VtValue *samples)
{
    if (maxSampleCount > 0) {
        times[0] = 0;
        samples[0] = Get(id, key);
        return 1;
    }
    return 0;
}

/*virtual*/
TfToken
HdSceneDelegate::GetReprName(SdfPath const &id)
{
    return TfToken();
}

// -----------------------------------------------------------------------//
/// \name Instancer prototypes
// -----------------------------------------------------------------------//

/*virtual*/
VtIntArray
HdSceneDelegate::GetInstanceIndices(SdfPath const &instancerId,
                                      SdfPath const &prototypeId)
{
    return VtIntArray();
}

/*virtual*/
GfMatrix4d
HdSceneDelegate::GetInstancerTransform(SdfPath const &instancerId,
                                         SdfPath const &prototypeId)
{
    return GfMatrix4d();
}

/*virtual*/
size_t
HdSceneDelegate::SampleInstancerTransform(SdfPath const &instancerId,
                                          SdfPath const &prototypeId,
                                          size_t maxSampleCount,
                                          float *times,
                                          GfMatrix4d *samples)
{
    if (maxSampleCount > 0) {
        times[0] = 0;
        samples[0] = GetInstancerTransform(instancerId, prototypeId);
        return 1;
    }
    return 0;
}

/*virtual*/
SdfPath
HdSceneDelegate::GetPathForInstanceIndex(const SdfPath &protoPrimPath,
                                         int instanceIndex,
                                         int *absoluteInstanceIndex,
                                         SdfPath * rprimPath,
                                         SdfPathVector *instanceContext)
{
    return SdfPath();
}



// -----------------------------------------------------------------------//
/// \name Material Aspects
// -----------------------------------------------------------------------//

/*virtual*/
SdfPath 
HdSceneDelegate::GetMaterialId(SdfPath const &rprimId)
{
    return SdfPath();
}

/*virtual*/
std::string
HdSceneDelegate::GetSurfaceShaderSource(SdfPath const &materialId)
{
    return std::string("");
}

/*virtual*/
std::string
HdSceneDelegate::GetDisplacementShaderSource(SdfPath const &materialId)
{
    return std::string("");
}

/*virtual*/
VtValue
HdSceneDelegate::GetMaterialParamValue(SdfPath const &materialId, 
                                       TfToken const &paramName)
{
    return VtValue();
}

/*virtual*/
HdMaterialParamVector
HdSceneDelegate::GetMaterialParams(SdfPath const &materialId)
{
    return HdMaterialParamVector();
}

/*virtual*/
VtValue 
HdSceneDelegate::GetMaterialResource(SdfPath const &materialId)
{
    return VtValue();
}

/*virtual*/
TfTokenVector 
HdSceneDelegate::GetMaterialPrimvars(SdfPath const &materialId)
{
    return TfTokenVector();
}


// -----------------------------------------------------------------------//
/// \name Texture Aspects
// -----------------------------------------------------------------------//

/*virtual*/
HdTextureResource::ID
HdSceneDelegate::GetTextureResourceID(SdfPath const& textureId)
{
    return HdTextureResource::ID();
}

/*virtual*/
HdTextureResourceSharedPtr
HdSceneDelegate::GetTextureResource(SdfPath const& textureId)
{
    return HdTextureResourceSharedPtr();
}

// -----------------------------------------------------------------------//
/// \name Light Aspects
// -----------------------------------------------------------------------//

/*virtual*/
VtValue 
HdSceneDelegate::GetLightParamValue(SdfPath const &id, 
                                    TfToken const &paramName) 
{
    return VtValue();
}

// -----------------------------------------------------------------------//
/// \name Camera Aspects
// -----------------------------------------------------------------------//

/*virtual*/
std::vector<GfVec4d>
HdSceneDelegate::GetClipPlanes(SdfPath const& cameraId)
{
    return std::vector<GfVec4d>();
}

// -----------------------------------------------------------------------//
/// \name ExtComputation Aspects
// -----------------------------------------------------------------------//

/*virtual*/
void
HdSceneDelegate::InvokeExtComputation(SdfPath const& computationId,
                                      HdExtComputationContext *context)
{
}

/*virtual*/
TfTokenVector
HdSceneDelegate::GetExtComputationSceneInputNames(SdfPath const& computationid)
{
    return TfTokenVector();
}

/*virtual*/
HdExtComputationInputDescriptorVector
HdSceneDelegate::GetExtComputationInputDescriptors(
                                        SdfPath const& computationid)
{
    return HdExtComputationInputDescriptorVector();
}

/*virtual*/
HdExtComputationOutputDescriptorVector
HdSceneDelegate::GetExtComputationOutputDescriptors(
                                        SdfPath const& computationid)
{
    return HdExtComputationOutputDescriptorVector();
}


// -----------------------------------------------------------------------//
/// \name Primitive Variables
// -----------------------------------------------------------------------//

/*virtual*/
HdPrimvarDescriptorVector
HdSceneDelegate::GetPrimvarDescriptors(SdfPath const& id,
                                       HdInterpolation interpolation)
{
    return HdPrimvarDescriptorVector();
}

/*virtual*/
HdExtComputationPrimvarDescriptorVector
HdSceneDelegate::GetExtComputationPrimvarDescriptors(
                                        SdfPath const& rprimId,
                                        HdInterpolation interpolationMode)
{
    return HdExtComputationPrimvarDescriptorVector();
}


/*virtual*/
std::string
HdSceneDelegate::GetExtComputationKernel(SdfPath const& id)
{
    return std::string();
}

PXR_NAMESPACE_CLOSE_SCOPE

