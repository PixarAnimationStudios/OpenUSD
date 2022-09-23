//
// Copyright 2020 Pixar
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
#include "pxr/usdImaging/usdImaging/dataSourcePointInstancer.h"

#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"
#include "pxr/usdImaging/usdImaging/dataSourceRelationship.h"
#include "pxr/usdImaging/usdImaging/primvarUtils.h"

#include "pxr/imaging/hd/lazyContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"

#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xformCache.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourcePointInstancerXforms::
UsdImagingDataSourcePointInstancerXforms(
        const UsdGeomPointInstancer &usdPI,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
  : _usdPI(usdPI)
  , _stageGlobals(stageGlobals)
{}

VtValue
UsdImagingDataSourcePointInstancerXforms::GetValue(
        HdSampledDataSource::Time shutterOffset)
{
    return VtValue(GetTypedValue(shutterOffset));
}

VtMatrix4dArray
UsdImagingDataSourcePointInstancerXforms::GetTypedValue(
        HdSampledDataSource::Time shutterOffset)
{
    TRACE_FUNCTION();

    // TODO: This implementation is here to get things going but it ultimately
    // not what we want for the following reasons.
    // - We use the UsdGeomXformCache here where we typically leave this
    //   flattening behavior to HdFlatteningSceneIndex.
    // - We ignore the shutterOffset for now - so no motion blur support.
    // - Some renderers can interpret the translate, rotate and scale primvars
    //   directly and we do not need to bake them into an instanceTransform.
    // - Write the code to compensate for the prototype transform in, e.g.,
    //   a different scene index after the flattening scene index.

    VtMatrix4dArray transforms;
    _usdPI.ComputeInstanceTransformsAtTime(
            &transforms, _stageGlobals.GetTime(), _stageGlobals.GetTime(),
            UsdGeomPointInstancer::ExcludeProtoXform,
            UsdGeomPointInstancer::IgnoreMask);

    // Compensate for prototype transforms.  The per-instance transform is
    // supposed to be:
    //   (PI part) PI root transform * TRS *
    //   (gprim part) proto root relative gprim transform ==
    //                inv(proto root transform) * gprim transform
    // ... For separation of data, gprims in imaging have their full transform,
    // so we need to tack inv(proto root transform) on each instance transform.
    VtIntArray protoIndices;
    _usdPI.GetProtoIndicesAttr().Get(&protoIndices, _stageGlobals.GetTime());
    SdfPathVector protoPaths;
    _usdPI.GetPrototypesRel().GetTargets(&protoPaths);

    std::vector<GfMatrix4d> protoXforms(protoPaths.size(), GfMatrix4d(1.0));

    UsdGeomXformCache xformCache(_stageGlobals.GetTime());
    for (size_t i = 0; i < protoPaths.size(); ++i) {
        const SdfPath &protoPath = protoPaths[i];
        if (const UsdPrim &protoPrim =
                _usdPI.GetPrim().GetPrimAtPath(protoPath)) {
            // TODO: Another implementation uses the transform of the
            // protoPrim's parent instead.
            protoXforms[i] = xformCache.GetLocalToWorldTransform(protoPrim)
                .GetInverse();
        }
    }
    for (size_t i = 0; i < transforms.size(); ++i) {
        if (i < protoIndices.size()) {
            const int protoIndex = protoIndices[i];
            if (size_t(protoIndex) < protoXforms.size()) {
                transforms[i] = protoXforms[protoIndex] * transforms[i];
            }
        }
    }

    return transforms;
}

bool
UsdImagingDataSourcePointInstancerXforms::GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> *outSampleTimes)
{
    // Above code ignores shutterOffset, so return false here.
    return false;
}    

// ----------------------------------------------------------------------------

UsdImagingDataSourcePointInstancerMask::UsdImagingDataSourcePointInstancerMask(
        const UsdGeomPointInstancer &usdPI,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
  : _usdPI(usdPI)
  , _stageGlobals(stageGlobals)
{}

VtValue
UsdImagingDataSourcePointInstancerMask::GetValue(
        HdSampledDataSource::Time shutterOffset)
{
    return VtValue(GetTypedValue(shutterOffset));
}

VtBoolArray
UsdImagingDataSourcePointInstancerMask::GetTypedValue(
        HdSampledDataSource::Time shutterOffset)
{
    // XXX: Add PI visibility here: if the PI is invisible, treat it as
    // everything masked.

    const std::vector<bool> mask = _usdPI.ComputeMaskAtTime(
        _stageGlobals.GetTime());
    VtBoolArray result(mask.begin(), mask.end());
    return result;
}


bool
UsdImagingDataSourcePointInstancerMask::GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> *outSampleTimes)
{
    return false;
}    

// ----------------------------------------------------------------------------

static
HdContainerDataSourceHandle
_PrimvarsDataSource(
    UsdGeomPointInstancer usdPI,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> values;

    UsdGeomPrimvarsAPI primvars(usdPI);

    for (const UsdGeomPrimvar &p : primvars.GetPrimvars()) {
        if (p.GetInterpolation() != UsdGeomTokens->constant &&
            p.GetInterpolation() != UsdGeomTokens->uniform) {
            const TfToken &name = p.GetPrimvarName();
            if (name != HdInstancerTokens->instanceTransform) {
                names.push_back(name);
                values.push_back(
                    UsdImagingDataSourcePrimvar::New(
                        usdPI.GetPath(),
                        name,
                        stageGlobals,
                        UsdAttributeQuery(p.GetAttr()),
                        UsdAttributeQuery(p.GetIndicesAttr()),
                        HdPrimvarSchema::BuildInterpolationDataSource(
                            UsdImagingUsdToHdInterpolationToken(
                                p.GetInterpolation())),
                        HdPrimvarSchema::BuildRoleDataSource(
                            UsdImagingUsdToHdRole(
                                p.GetAttr().GetRoleName()))));
            }
        }
    }

    names.push_back(HdInstancerTokens->instanceTransform);
    values.push_back(
        HdPrimvarSchema::Builder()
            .SetPrimvarValue(
                UsdImagingDataSourcePointInstancerXforms::New(
                    usdPI, stageGlobals))
            .SetInterpolation(
                HdPrimvarSchema::BuildInterpolationDataSource(
                    HdPrimvarSchemaTokens->instance))
            .SetRole(
                HdPrimvarSchema::BuildRoleDataSource(
                    HdPrimvarSchemaTokens->transform))
        .Build());

    return HdRetainedContainerDataSource::New(
        names.size(),
        names.data(),
        values.data());
}

// ----------------------------------------------------------------------------

UsdImagingDataSourcePointInstancerTopology::
UsdImagingDataSourcePointInstancerTopology(
        UsdGeomPointInstancer usdPI,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
  : _usdPI(usdPI)
  , _stageGlobals(stageGlobals)
{}

bool
UsdImagingDataSourcePointInstancerTopology::Has(const TfToken &name)
{
    if (name == HdInstancerTopologySchemaTokens->prototypes ||
        name == HdInstancerTopologySchemaTokens->instanceIndices ||
        name == HdInstancerTopologySchemaTokens->mask)
    {
        return true;
    }

    return false;
}

TfTokenVector
UsdImagingDataSourcePointInstancerTopology::GetNames()
{
    return {
        HdInstancerTopologySchemaTokens->prototypes,
        HdInstancerTopologySchemaTokens->instanceIndices,
        HdInstancerTopologySchemaTokens->mask,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourcePointInstancerTopology::Get(const TfToken &name)
{
    if (name == HdInstancerTopologySchemaTokens->prototypes) {
        return UsdImagingDataSourceRelationship::New(
                _usdPI.GetPrototypesRel(), _stageGlobals);
    } else if (name == HdInstancerTopologySchemaTokens->instanceIndices) {
        VtIntArray protoIndices;
        _usdPI.GetProtoIndicesAttr().Get(
            &protoIndices, _stageGlobals.GetTime());

        // We need to flip the protoIndices: [0,1,0] -> 0: [0,2], 1: [1].
        std::vector<VtIntArray> instanceIndices;
        for (size_t i = 0; i < protoIndices.size(); ++i) {
            int protoIndex = protoIndices[i];
            if (size_t(protoIndex) >= instanceIndices.size()) {
                instanceIndices.resize(protoIndex+1);
            }
            instanceIndices[protoIndex].push_back(i);
        }

        std::vector<HdDataSourceBaseHandle> indicesVec;
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            indicesVec.push_back(
                HdRetainedTypedSampledDataSource<VtArray<int>>::New(
                    instanceIndices[i]));
        }
        return HdRetainedSmallVectorDataSource::New(
            indicesVec.size(), indicesVec.data());
    } else if (name == HdInstancerTopologySchemaTokens->mask) {
        return UsdImagingDataSourcePointInstancerMask::New(
                _usdPI, _stageGlobals);
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourcePointInstancerPrim::UsdImagingDataSourcePointInstancerPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
  : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

bool
UsdImagingDataSourcePointInstancerPrim::Has(const TfToken &name)
{
    if (name == HdInstancerTopologySchemaTokens->instancerTopology ||
        name == HdPrimvarsSchemaTokens->primvars)
    {
        return true;
    }

    return UsdImagingDataSourcePrim::Has(name);
}

TfTokenVector
UsdImagingDataSourcePointInstancerPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourcePrim::GetNames();
    result.push_back(HdInstancerTopologySchemaTokens->instancerTopology);
    result.push_back(HdPrimvarsSchemaTokens->primvars);
    return result;
}

HdDataSourceBaseHandle
UsdImagingDataSourcePointInstancerPrim::Get(const TfToken &name)
{
    if (name == HdInstancerTopologySchemaTokens->instancerTopology) {
        return UsdImagingDataSourcePointInstancerTopology::New(
                UsdGeomPointInstancer(_GetUsdPrim()), _GetStageGlobals());
    } else if (name == HdPrimvarsSchemaTokens->primvars) {
        return HdLazyContainerDataSource::New(
            std::bind(
                _PrimvarsDataSource, 
                UsdGeomPointInstancer(_GetUsdPrim()),
                std::cref(_GetStageGlobals())));
    } else {
        return UsdImagingDataSourcePrim::Get(name);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
