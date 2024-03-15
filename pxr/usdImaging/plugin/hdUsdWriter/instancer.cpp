//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#include "pxr/usdImaging/plugin/hdUsdWriter/instancer.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"

#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/transform.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/scope.h"

#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
void _FlattenInstanceData(VtArray<T>& out, const VtArray<T>& in, const std::vector<VtIntArray>& allInstanceIndices)
{
    if (in.empty())
    {
        return;
    }
    const size_t totalNumOfInstances =
        std::accumulate(allInstanceIndices.begin(), allInstanceIndices.end(), static_cast<size_t>(0),
                        [](size_t acc, const auto& vec) -> size_t { return acc + vec.size(); });
    if (totalNumOfInstances == 0)
    {
        return;
    }
    out.reserve(totalNumOfInstances);
    for (const auto& instanceIndices : allInstanceIndices)
    {
        for (const auto& instanceIndex : instanceIndices)
        {
            if (instanceIndex < 0 || static_cast<size_t>(instanceIndex) >= in.size())
            {
                TF_WARN("Unexpected instance index %i", instanceIndex);
                continue;
            }
            out.push_back(in[instanceIndex]);
        }
    }
}

template <typename T>
bool _FlattenPrimvarIfHolding(VtValue& out, const VtValue& primvar, const std::vector<VtIntArray>& allInstanceIndices)
{
    if (!primvar.IsHolding<VtArray<T>>())
    {
        return false;
    }
    VtArray<T> outArray;
    _FlattenInstanceData(outArray, primvar.UncheckedGet<VtArray<T>>(), allInstanceIndices);
    out.Swap(outArray);
    return true;
}

template <typename T0, typename T1, typename... T>
bool _FlattenPrimvarIfHolding(VtValue& out, const VtValue& primvar, const std::vector<VtIntArray>& allInstanceIndices)
{
    return _FlattenPrimvarIfHolding<T0>(out, primvar, allInstanceIndices) ||
           _FlattenPrimvarIfHolding<T1, T...>(out, primvar, allInstanceIndices);
}

VtValue _FlattenInstancePrimvar(const VtValue& primvar, const std::vector<VtIntArray>& allInstanceIndices)
{
    VtValue out;
    if (!primvar.IsArrayValued())
    {
        return out;
    }
    _FlattenPrimvarIfHolding<bool, int, float, GfHalf, double, std::string, TfToken, SdfPath, SdfAssetPath, GfMatrix2f,
        GfMatrix2d, GfMatrix3f, GfMatrix3d, GfMatrix4f, GfMatrix4d, GfVec2f, GfVec2i, GfVec2d,
        GfVec2h, GfVec3f, GfVec3i, GfVec3d, GfVec3h, GfVec4f, GfVec4i, GfVec4d, GfVec4h, GfQuatf,
        GfQuatd, GfQuath>(out, primvar, allInstanceIndices);
    return out;
}


HdUsdWriterInstancer::HdUsdWriterInstancer(HdSceneDelegate* delegate, SdfPath const& id) : HdInstancer(delegate, id)
{
}

void HdUsdWriterInstancer::Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();

    SdfPath const& id = GetId();

    // Nested instancing support:
    _UpdateInstancer(sceneDelegate, dirtyBits);
    // Some _Sync calls will arrive with an instance selection as variant selection
    const auto parentInstancerId = GetParentId().StripAllVariantSelections();
    if (!parentInstancerId.IsEmpty())
    {
        auto& renderIndex = sceneDelegate->GetRenderIndex();
        auto* parentInstancer = static_cast<HdUsdWriterInstancer*>(renderIndex.GetInstancer(parentInstancerId));
        if (parentInstancer != nullptr)
        {
            parentInstancer->AddInstancedPrim(id.StripAllVariantSelections());
        }
    }

    if (HdChangeTracker::IsAnyPrimvarDirty(*dirtyBits, id) || HdChangeTracker::IsInstanceIndexDirty(*dirtyBits, id))
    {
        _primvars.clear();
        for (auto interpolation : { HdInterpolationConstant, HdInterpolationUniform, HdInterpolationInstance })
        {
            for (const auto& primvarDescriptor : sceneDelegate->GetPrimvarDescriptors(id, interpolation))
            {
                VtValue value = sceneDelegate->Get(id, primvarDescriptor.name);
                if (!value.IsEmpty())
                {
                    _primvars.insert(std::make_pair(primvarDescriptor.name, HdUsdWriterPrimvar(primvarDescriptor, value)));
                }
            }
        }
    }

    if (HdChangeTracker::IsTransformDirty(*dirtyBits, id))
    {
        const GfMatrix4d mtx = sceneDelegate->GetInstancerTransform(id.StripAllVariantSelections());
        _transform = mtx;
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void HdUsdWriterInstancer::AddInstancedPrim(const SdfPath& path)
{
    _rPrims.insert(path);
}

void HdUsdWriterInstancer::RemoveInstancedPrim(const SdfPath& path)
{
    _rPrims.unsafe_erase(path);
}

void HdUsdWriterInstancer::GetPrototypePath(const SdfPath& rprimId,
                                       const SdfPath& instancerPath,
                                       SdfPath& protoPathOut)
{
    if (rprimId.IsAbsoluteRootOrPrimPath())
    {
        protoPathOut = rprimId;
    }
    else
    {
        static const TfToken proto("proto");
        // Adding a proto override on top of the rprim name token.
        protoPathOut = rprimId.GetPrimOrPrimVariantSelectionPath().AppendChild(proto).AppendChild(rprimId.GetNameToken());
    }
}

void HdUsdWriterInstancer::SerializeToUsd(const UsdStagePtr &stage)
{
    SdfPath orgId = GetId();
    SdfPath id = HdUsdWriterGetFlattenPrototypePath(orgId);
    if (!id.IsAbsoluteRootOrPrimPath())
    {
        return;
    }

    const auto instancer = UsdGeomPointInstancer::Define(stage, id);
    if (!instancer)
    {
        TF_CODING_ERROR("HdUsdWriterInstancer::SerializeToUsd: Failed to create instancer %s", id.GetAsString().c_str());
        return;
    }

    HdUsdWriterPopOptional(_transform,
        [&](const auto& transform)
        {
            HdUsdWriterSetTransformOp(UsdGeomXformable(instancer), transform);
        });
    HdUsdWriterSetVisible(_visible, instancer.GetPrim());

    size_t numInstances = 0;    
    if (_primvars.count(HdInstancerTokens->instanceTransforms) != 0)
    {
        // If we find instanceTransform, that means we have a scene graph instance transforms as Matrix4d
        // and we need to convert back to PointInstancer-compatible positions/orientations/scales
        PrimvarMap::const_accessor pvAccessor;
        if (_primvars.find(pvAccessor, HdInstancerTokens->instanceTransforms) && pvAccessor->second.value.IsHolding<VtMatrix4dArray>())
        {
            VtMatrix4dArray transforms = pvAccessor->second.value.UncheckedGet<VtMatrix4dArray>();
            numInstances = transforms.size();
            VtVec3fArray translates(numInstances);
            VtQuathArray orientations(numInstances);
            VtVec3fArray scales(numInstances);

            for (size_t i = 0; i < numInstances; i++)
            {
                GfTransform t(transforms[i]);
                translates[i] = GfVec3f(t.GetTranslation());
                GfQuatd quat = t.GetRotation().GetQuat();
                orientations[i] =
                    GfQuath(quat.GetReal(), quat.GetImaginary()[0], quat.GetImaginary()[1], quat.GetImaginary()[2]);
                scales[i] = GfVec3f(t.GetScale());
            }

            // We pretend we had primvars for these 3 and let code further down remove invisible indices
            _primvars.insert(std::make_pair(
                HdInstancerTokens->instanceTranslations,
                HdUsdWriterPrimvar(HdPrimvarDescriptor(HdInstancerTokens->instanceTranslations, HdInterpolationInstance),
                               VtValue(translates))));
            _primvars.insert(std::make_pair(
                HdInstancerTokens->instanceRotations,
                HdUsdWriterPrimvar(HdPrimvarDescriptor(HdInstancerTokens->instanceRotations, HdInterpolationInstance),
                               VtValue(orientations))));
            _primvars.insert(std::make_pair(
                HdInstancerTokens->instanceScales,
                HdUsdWriterPrimvar(HdPrimvarDescriptor(HdInstancerTokens->instanceScales, HdInterpolationInstance),
                               VtValue(scales))));
        }
    }
    else if (_primvars.count(HdInstancerTokens->instanceTranslations) != 0)
    {
        PrimvarMap::const_accessor pvAccessor;
        if (_primvars.find(pvAccessor, HdInstancerTokens->instanceTranslations))
        {
            numInstances = pvAccessor->second.value.UncheckedGet<VtVec3fArray>().size();
        }
    }
    else
    {
        // There was no dirty primvar or indices this time around
        return;
    }

    auto rels = instancer.CreatePrototypesRel();

    const auto numRprims = _rPrims.size();
    // If prototypes use the same instance, the size of protoIndices could be up to numInstances * numRprims
    // However, in many (most?) cases, we may have just one prototype per instance... and in this situation,
    // numInstances * numRprims could be a MASSIVE over-allocation, considering that numInstances can get
    // VERY large.
    VtIntArray protoIndices;
    protoIndices.reserve(numInstances);

    // We need ordered traversal of the primitives or the primitives in the relationship could change ordering.
    std::vector<SdfPath> orderedRprims{std::cbegin(_rPrims), std::cend(_rPrims)};
    std::sort(orderedRprims.begin(), orderedRprims.end());

    HdSceneDelegate* sceneDelegate = GetDelegate()->GetRenderIndex().GetSceneDelegateForRprim(orgId);
    if (!sceneDelegate)
    {
        // When scene index emulation is off we can use the top-level delegate
        // otherwise the delegate we get here is missing the GetScenePrimPath(s) methods
        sceneDelegate = GetDelegate();
    }

    std::vector<VtIntArray> allInstanceIndices;
    allInstanceIndices.reserve(numRprims);
    for (int protoIndex = 0; protoIndex < numRprims; ++protoIndex)
    {
        const auto& target = orderedRprims[protoIndex];

        VtIntArray currentInstanceIndices = GetDelegate()->GetInstanceIndices(GetId(), target);
        allInstanceIndices.push_back(currentInstanceIndices);
        SdfPath protoPath;
        GetPrototypePath(HdUsdWriterGetFlattenPrototypePath(target), id, protoPath);
        rels.AddTarget(protoPath);

        // We are flattening the proto index into the list of instance indices.
        for (const auto& instanceIndex : currentInstanceIndices)
        {
            if (instanceIndex < 0 || static_cast<size_t>(instanceIndex) >= numInstances)
            {
                TF_WARN("WARNING: Found invalid instance index %i for prototype %s", instanceIndex, target.GetAsString().c_str());
                continue;
            }
            protoIndices.push_back(protoIndex);
        }

        std::vector<int> localInstanceIndices(currentInstanceIndices.size());
        std::iota(localInstanceIndices.begin(), localInstanceIndices.end(), 0);
        SdfPathVector scenePrimPaths = sceneDelegate->GetScenePrimPaths(target, localInstanceIndices);

        // Check that the scene prim paths returned by vectorized method are consistent with the single index method
        for (size_t i = 0; i < localInstanceIndices.size(); ++i)
        {
            if (scenePrimPaths[i] != sceneDelegate->GetScenePrimPath(target, static_cast<int>(i)))
            {
                TF_WARN("WARNING: GetScenePrimPaths returned different results for the same index! ( %s[%zu] %s != %s )",
                    orgId.GetAsString().c_str(), i, scenePrimPaths[i].GetAsString().c_str(), sceneDelegate->GetScenePrimPath(orgId, i).GetAsString().c_str());
                return;
            }
        }

        // Filter out any invalid path
        scenePrimPaths.erase(
            std::remove_if(scenePrimPaths.begin(), scenePrimPaths.end(), [](auto path) { return path.IsEmpty(); }),
            scenePrimPaths.end());
        if (!scenePrimPaths.empty())
        {
            std::string scenePrimPathAttrName = "scenePrimPaths";
            scenePrimPathAttrName += TfStringReplace(protoPath.GetAsString(), "/", ":");
            auto scenePrimPathRel =
                instancer.GetPrim().CreateRelationship(TfToken(scenePrimPathAttrName.c_str()), true);
            for (const auto& scenePath : scenePrimPaths)
            {
                scenePrimPathRel.AddTarget(HdUsdWriterGetFlattenPrototypePath(scenePath));
            }
        }
    }

    instancer.CreateProtoIndicesAttr().Set(protoIndices);

    _GetPrimvar<VtArray<GfVec3f>>(HdInstancerTokens->instanceTranslations,
        [&instancer, &allInstanceIndices](const auto& primvar)
        {
            VtArray<GfVec3f> translations;
            _FlattenInstanceData(translations, primvar, allInstanceIndices);
            instancer.CreatePositionsAttr().Set(translations);
        });

    _GetPrimvar<VtArray<GfQuath>>(HdInstancerTokens->instanceRotations,
        [&instancer, &allInstanceIndices](const auto& primvar)
        {
            VtArray<GfQuath> orientations;
            _FlattenInstanceData(orientations, primvar, allInstanceIndices);
            instancer.CreateOrientationsAttr().Set(orientations);
        });

    _GetPrimvar<VtVec3fArray>(HdInstancerTokens->instanceScales,
        [&instancer, &allInstanceIndices](const auto& primvar)
        {
            VtVec3fArray scales;
            _FlattenInstanceData(scales, primvar, allInstanceIndices);
            instancer.CreateScalesAttr().Set(scales);
        });

    _GetPrimvar<VtArray<GfVec3f>>(HdTokens->velocities,
        [&instancer, &allInstanceIndices](const auto& primvar)
        {
            VtVec3fArray velocities;
            _FlattenInstanceData(velocities, primvar, allInstanceIndices);
            instancer.CreateVelocitiesAttr().Set(velocities);
        });

    _GetPrimvar<VtArray<GfVec3f>>(HdTokens->accelerations,
        [&instancer, &allInstanceIndices](const auto& primvar)
        {
            VtVec3fArray accelerations;
            _FlattenInstanceData(accelerations, primvar, allInstanceIndices);
            instancer.CreateAccelerationsAttr().Set(accelerations);
        });

    _GetPrimvar<VtArray<GfVec3f>>(UsdGeomTokens->angularVelocities,
        [&instancer, &allInstanceIndices](const auto& primvar)
        {
            VtVec3fArray angularVelocities;
            _FlattenInstanceData(angularVelocities, primvar, allInstanceIndices);
            instancer.CreateAngularVelocitiesAttr().Set(angularVelocities);
        });


    static const auto instancePrimvars = std::unordered_set<TfToken, TfToken::HashFunctor>{ UsdGeomTokens->angularVelocities,
        UsdGeomTokens->invisibleIds,
        HdTokens->accelerations,
        HdTokens->velocities,
        HdInstancerTokens->instanceTransforms,
        HdInstancerTokens->instanceScales,
        HdInstancerTokens->instanceRotations,
        HdInstancerTokens->instanceTranslations };

    auto primvarsApi = UsdGeomPrimvarsAPI(instancer.GetPrim());
    // This doesn't seem to be passed along in usdImaging.
    for (const auto& kvp : _primvars)
    {
        const auto& primvar = kvp.second.value;
        if (instancePrimvars.find(kvp.first) != instancePrimvars.end())
        {
            continue;
        }

        if (!SdfValueHasValidType(primvar))
        {
            continue;
        }

        const auto sdfType = SdfGetValueTypeNameForValue(primvar);
        const auto interpolation = HdUsdWriterGetTokenFromHdInterpolation(kvp.second.descriptor.interpolation);
        auto pv = primvarsApi.CreatePrimvar(kvp.first, sdfType, interpolation);
        pv.Set((kvp.second.descriptor.interpolation == HdInterpolationConstant ||
                kvp.second.descriptor.interpolation == HdInterpolationUniform) ?
                   primvar :
                   _FlattenInstancePrimvar(primvar, allInstanceIndices));
    }
    _primvars.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
