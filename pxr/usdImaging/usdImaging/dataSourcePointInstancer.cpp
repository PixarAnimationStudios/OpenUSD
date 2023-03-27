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

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

UsdImagingDataSourcePointInstancerMask::UsdImagingDataSourcePointInstancerMask(
        const SdfPath &sceneIndexPath,
        const UsdGeomPointInstancer &usdPI,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
  : _usdPI(usdPI)
  , _stageGlobals(stageGlobals)
{
    if (UsdAttribute attr = usdPI.GetInvisibleIdsAttr()) {
        if (attr.ValueMightBeTimeVarying()) {
            static const HdDataSourceLocator locator =
                HdInstancerTopologySchema::GetDefaultLocator()
                .Append(HdInstancerTopologySchemaTokens->mask);
            _stageGlobals.FlagAsTimeVarying(sceneIndexPath, locator);
        }
    }
}

VtValue
UsdImagingDataSourcePointInstancerMask::GetValue(
        const HdSampledDataSource::Time shutterOffset)
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

UsdImagingDataSourcePointInstancerTopology::
UsdImagingDataSourcePointInstancerTopology(
        const SdfPath &sceneIndexPath,
        UsdGeomPointInstancer usdPI,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
  : _sceneIndexPath(sceneIndexPath)
  , _usdPI(usdPI)
  , _stageGlobals(stageGlobals)
{}

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
            const int protoIndex = protoIndices[i];
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
                _sceneIndexPath, _usdPI, _stageGlobals);
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
                _GetSceneIndexPath(),
                UsdGeomPointInstancer(_GetUsdPrim()),
                _GetStageGlobals());
    } else if (name == HdPrimvarsSchemaTokens->primvars) {

        // Note that we are not yet handling velocities, accelerations
        // and angularVelocities.
        static const UsdImagingDataSourceCustomPrimvars::Mappings
            customPrimvarMappings =
                {
                    { HdInstancerTokens->translate,
                      UsdGeomTokens->positions,
                      HdPrimvarSchemaTokens->instance
                    },
                    { HdInstancerTokens->rotate,
                      UsdGeomTokens->orientations,
                      HdPrimvarSchemaTokens->instance
                    },
                    { HdInstancerTokens->scale,
                      UsdGeomTokens->scales,
                      HdPrimvarSchemaTokens->instance
                    }
                };

        HdContainerDataSourceHandle basePvs = HdContainerDataSource::Cast(
            UsdImagingDataSourcePrim::Get(name));

        HdContainerDataSourceHandle customPvs =
            UsdImagingDataSourceCustomPrimvars::New(
                _GetSceneIndexPath(),
                _GetUsdPrim(),
                customPrimvarMappings,
                _GetStageGlobals());

        if (basePvs) {
            return HdOverlayContainerDataSource::New(basePvs, customPvs);
        } else {
            return customPvs;
        }

    } else {
        return UsdImagingDataSourcePrim::Get(name);
    }
}

HdDataSourceLocatorSet
UsdImagingDataSourcePointInstancerPrim::Invalidate(
    UsdPrim const &prim,
    const TfToken &subprim,
    const TfTokenVector &properties)
{
    HdDataSourceLocatorSet locators =
        UsdImagingDataSourcePrim::Invalidate(
            prim, subprim, properties);

    for (const TfToken &propertyName : properties) {
        if (propertyName == UsdGeomTokens->prototypes) {
            locators.insert(
                HdInstancerTopologySchema::GetDefaultLocator());
        }
        if (propertyName == UsdGeomTokens->protoIndices) {
            static const HdDataSourceLocator locator =
                HdInstancerTopologySchema::GetDefaultLocator()
                    .Append(HdInstancerTopologySchemaTokens->instanceIndices);
            locators.insert(locator);
        }
        // inactiveIds are metadata - changing those will cause a resync
        // of the prim, that is prim remove and add to the stage scene index.
        // So we do not need to deal with them here.
        if (propertyName == UsdGeomTokens->invisibleIds) {
            static const HdDataSourceLocator locator =
                HdInstancerTopologySchema::GetDefaultLocator()
                    .Append(HdInstancerTopologySchemaTokens->mask);
            locators.insert(locator);
        }
        if (propertyName == UsdGeomTokens->positions) {
            static const HdDataSourceLocator locator =
                HdPrimvarsSchema::GetDefaultLocator()
                    .Append(HdInstancerTokens->translate)
                    .Append(HdPrimvarSchemaTokens->primvarValue);
            locators.insert(locator);
        }
        if (propertyName == UsdGeomTokens->orientations) {
            static const HdDataSourceLocator locator =
                HdPrimvarsSchema::GetDefaultLocator()
                    .Append(HdInstancerTokens->rotate)
                    .Append(HdPrimvarSchemaTokens->primvarValue);
            locators.insert(locator);
        }
        if (propertyName == UsdGeomTokens->scales) {
            static const HdDataSourceLocator locator =
                HdPrimvarsSchema::GetDefaultLocator()
                    .Append(HdInstancerTokens->scale)
                    .Append(HdPrimvarSchemaTokens->primvarValue);
            locators.insert(locator);
        }
    }

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
