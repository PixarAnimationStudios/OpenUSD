//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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


        _delegateID = _delegateID.MakeAbsolutePath(SdfPath::AbsoluteRootPath());
    }
}

HdSceneDelegate::~HdSceneDelegate() = default;

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
HdSceneDelegate::GetRenderTag(SdfPath const& id)
{
    return HdRenderTagTokens->geometry;
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
    return GfMatrix4d(1);
}

/*virtual*/
size_t
HdSceneDelegate::SampleTransform(SdfPath const & id,
                                 size_t maxSampleCount,
                                 float *sampleTimes,
                                 GfMatrix4d *sampleValues)
{
    if (maxSampleCount > 0) {
        sampleTimes[0] = 0.0;
        sampleValues[0] = GetTransform(id);
        return 1;
    }
    return 0;
}

/*virtual*/
size_t
HdSceneDelegate::SampleTransform(SdfPath const & id,
                                 float startTime,
                                 float endTime,
                                 size_t maxSampleCount,
                                 float *sampleTimes,
                                 GfMatrix4d *sampleValues)
{
    return SampleTransform(id, maxSampleCount, sampleTimes, sampleValues);
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
VtValue
HdSceneDelegate::GetIndexedPrimvar(SdfPath const& id, TfToken const& key, 
                                        VtIntArray *outIndices) 
{
    // We return an empty value here rather than returning the result of 
    // Get(id, key) since that would leave callers of this method with an 
    // empty outIndices which is semantically different than a non-indexed 
    // primvar.
    return VtValue();
}

/*virtual*/
size_t
HdSceneDelegate::SamplePrimvar(SdfPath const& id, 
                               TfToken const& key,
                               size_t maxSampleCount,
                               float *sampleTimes,
                               VtValue *sampleValues)
{
    if (maxSampleCount > 0) {
        sampleTimes[0] = 0.0;
        sampleValues[0] = Get(id, key);
        return 1;
    }
    return 0;
}

/*virtual*/
size_t
HdSceneDelegate::SamplePrimvar(SdfPath const& id, 
                               TfToken const& key,
                               float startTime,
                               float endTime,
                               size_t maxSampleCount,
                               float *sampleTimes,
                               VtValue *sampleValues)
{
    return SamplePrimvar(id, key, maxSampleCount, sampleTimes, sampleValues);
}

/*virtual*/
size_t
HdSceneDelegate::SampleIndexedPrimvar(SdfPath const& id,
                               TfToken const& key,
                               size_t maxSampleCount,
                               float *sampleTimes,
                               VtValue *sampleValues,
                               VtIntArray *sampleIndices)
{
    if (maxSampleCount > 0) {
        sampleTimes[0] = 0.0;
        sampleValues[0] = GetIndexedPrimvar(id, key, &sampleIndices[0]);
        return 1;
    }
    return 0;
}

/*virtual*/
size_t
HdSceneDelegate::SampleIndexedPrimvar(SdfPath const& id, 
                               TfToken const& key,
                               float startTime,
                               float endTime,
                               size_t maxSampleCount,
                               float *sampleTimes,
                               VtValue *sampleValues,
                               VtIntArray *sampleIndices)
{
    return
        SampleIndexedPrimvar(
            id, key, maxSampleCount,
            sampleTimes, sampleValues, sampleIndices);
}

/*virtual*/
HdReprSelector
HdSceneDelegate::GetReprSelector(SdfPath const &id)
{
    return HdReprSelector();
}

/*virtual*/
VtArray<TfToken>
HdSceneDelegate::GetCategories(SdfPath const& id)
{
    return VtArray<TfToken>();
}

std::vector<VtArray<TfToken>>
HdSceneDelegate::GetInstanceCategories(SdfPath const &instancerId)
{
    return std::vector<VtArray<TfToken>>();
}

HdIdVectorSharedPtr
HdSceneDelegate::GetCoordSysBindings(SdfPath const& id)
{
    return nullptr;
}

HdModelDrawMode
HdSceneDelegate::GetModelDrawMode(SdfPath const& id)
{
    return HdModelDrawMode();
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
HdSceneDelegate::GetInstancerTransform(SdfPath const &instancerId)
{
    return GfMatrix4d(1);
}

/*virtual*/
SdfPath
HdSceneDelegate::GetInstancerId(SdfPath const& primId)
{
    return SdfPath();
}

/*virtual*/
SdfPathVector
HdSceneDelegate::GetInstancerPrototypes(SdfPath const& instancerId)
{
    return SdfPathVector();
}

/*virtual*/
size_t
HdSceneDelegate::SampleInstancerTransform(SdfPath const &instancerId,
                                          size_t maxSampleCount,
                                          float *sampleTimes,
                                          GfMatrix4d *sampleValues)
{
    if (maxSampleCount > 0) {
        sampleTimes[0] = 0.0;
        sampleValues[0] = GetInstancerTransform(instancerId);
        return 1;
    }
    return 0;
}

/*virtual*/
size_t
HdSceneDelegate::SampleInstancerTransform(SdfPath const &instancerId,
                                          float startTime,
                                          float endTime,
                                          size_t maxSampleCount,
                                          float *sampleTimes,
                                          GfMatrix4d *sampleValues)
{
    return SampleInstancerTransform(
        instancerId, maxSampleCount, sampleTimes, sampleValues);
}

/*virtual*/
SdfPath
HdSceneDelegate::GetScenePrimPath(SdfPath const& rprimId,
                                  int instanceIndex,
                                  HdInstancerContext *instancerContext)
{
    return rprimId.ReplacePrefix(_delegateID, SdfPath::AbsoluteRootPath());
}

/*virtual*/
SdfPathVector
HdSceneDelegate::GetScenePrimPaths(SdfPath const& rprimId,
                                   std::vector<int> instanceIndices,
                                   std::vector<HdInstancerContext> *instancerContexts)
{
    return SdfPathVector(instanceIndices.size(),
            rprimId.ReplacePrefix(_delegateID, SdfPath::AbsoluteRootPath()));
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
VtValue 
HdSceneDelegate::GetMaterialResource(SdfPath const &materialId)
{
    return VtValue();
}

// -----------------------------------------------------------------------//
/// \name Renderbuffer Aspects
// -----------------------------------------------------------------------//

/*virtual*/
HdRenderBufferDescriptor
HdSceneDelegate::GetRenderBufferDescriptor(SdfPath const& id)
{
    return HdRenderBufferDescriptor();
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
VtValue 
HdSceneDelegate::GetCameraParamValue(SdfPath const &cameraId, 
                                     TfToken const &paramName) 
{
    return VtValue();
}

// -----------------------------------------------------------------------//
/// \name Volume Aspects
// -----------------------------------------------------------------------//

/*virtual*/
HdVolumeFieldDescriptorVector
HdSceneDelegate::GetVolumeFieldDescriptors(SdfPath const &volumeId)
{
    return HdVolumeFieldDescriptorVector();
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
VtValue
HdSceneDelegate::GetExtComputationInput(SdfPath const& computationId,
                                        TfToken const& input)
{
    return VtValue();
}

/*virtual*/
size_t
HdSceneDelegate::SampleExtComputationInput(SdfPath const& computationId,
                                           TfToken const& input,
                                           size_t maxSampleCount,
                                           float *sampleTimes,
                                           VtValue *sampleValues)
{
    if (maxSampleCount > 0) {
        sampleTimes[0] = 0.0;
        sampleValues[0] = GetExtComputationInput(computationId, input);
        return 1;
    }
    return 0;
}

/*virtual*/
size_t
HdSceneDelegate::SampleExtComputationInput(SdfPath const& computationId,
                                           TfToken const& input,
                                           float startTime,
                                           float endTime,
                                           size_t maxSampleCount,
                                           float *sampleTimes,
                                           VtValue *sampleValues)
{
    return
        SampleExtComputationInput(
            computationId, input, maxSampleCount, sampleTimes, sampleValues);
}

/*virtual*/
std::string
HdSceneDelegate::GetExtComputationKernel(SdfPath const& id)
{
    return std::string();
}


// -----------------------------------------------------------------------//
/// \name Task Aspects
// -----------------------------------------------------------------------//
/*virtual*/
TfTokenVector HdSceneDelegate::GetTaskRenderTags(SdfPath const& taskId)
{
    // While the empty vector can mean no filtering and let all tags
    // pass.  If any task has a non-empty render tags, the empty tags
    // means that the task isn't interested in any prims at all.
    // So the empty set use for no filtering should be limited
    // to tests.
    return TfTokenVector();
}


PXR_NAMESPACE_CLOSE_SCOPE

