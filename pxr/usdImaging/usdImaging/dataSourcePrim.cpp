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
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"
#include "pxr/usdImaging/usdImaging/dataSourceUsdPrimInfo.h"
#include "pxr/usdImaging/usdImaging/modelSchema.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/primOriginSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceVisibility::UsdImagingDataSourceVisibility(
        const UsdAttributeQuery &visibilityQuery,
        const SdfPath &sceneIndexPath,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _visibilityQuery(visibilityQuery)
    , _stageGlobals(stageGlobals)
{
    if (_visibilityQuery.ValueMightBeTimeVarying()) {
        _stageGlobals.FlagAsTimeVarying(sceneIndexPath,
                HdDataSourceLocator(HdVisibilitySchemaTokens->visibility));
    }
}

TfTokenVector
UsdImagingDataSourceVisibility::GetNames()
{
    return {
        HdVisibilitySchemaTokens->visibility,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceVisibility::Get(const TfToken &name)
{
    // Note, we need to do a bit of a dance here.
    //
    // Hydra has tri-state visibility (visible, invisible, inherited), which
    // is indicated by presence of the boolean visibility attribute (vis/invis),
    // or absence of the visibility attribute (inherited). For inherited
    // visibility, the flattening scene index will compute a resolved boolean
    // value.
    //
    // USD has bi-state visibility (invisible, inherited). Absence of a value
    // indicates inherited.
    //
    // If the USD attribute isn't authored, the hydra attribute isn't present,
    // and the value is "inherited".  If the USD attribute is "invisible", we
    // can return "invisible" (boolean false) here, and everything's cool.
    // However, if the USD attribute is authored as "inherited" here, we need
    // to map that to hydra not having a value.
    //
    // We do this by mapping "invisible" to constant "false", and
    // "inherited" to nullptr.
    //
    // Note that this mapping doesn't allow for visibility to vary across
    // a shutter window; that would require a hydra schema change, but it's
    // probably not a useful feature.
    if (name == HdVisibilitySchemaTokens->visibility) {
        TfToken vis;
        _visibilityQuery.Get(&vis, _stageGlobals.GetTime());
        if (vis == UsdGeomTokens->invisible) {
            return HdRetainedTypedSampledDataSource<bool>::New(false);
        }
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourcePurpose::UsdImagingDataSourcePurpose(
        const UsdAttributeQuery &purposeQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _purposeQuery(purposeQuery)
    , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourcePurpose::GetNames()
{
    return {
        HdPurposeSchemaTokens->purpose,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourcePurpose::Get(const TfToken &name)
{
    if (name == HdPurposeSchemaTokens->purpose) {
        return UsdImagingDataSourceAttributeNew(_purposeQuery, _stageGlobals);
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceExtentCoordinate::UsdImagingDataSourceExtentCoordinate(
        const HdVec3fArrayDataSourceHandle &extentAttr,
        const SdfPath &attrPath,
        unsigned int index)
    : _extentAttr(extentAttr)
    , _attrPath(attrPath)
    , _index(index)
{
}

VtValue
UsdImagingDataSourceExtentCoordinate::GetValue(
        HdSampledDataSource::Time shutterOffset)
{
    return VtValue(GetTypedValue(shutterOffset));
}

GfVec3d
UsdImagingDataSourceExtentCoordinate::GetTypedValue(
        HdSampledDataSource::Time shutterOffset)
{
    // XXX: Note: this class would make for a nice utility in the core
    // datasource code if it were only doing the indexing.  Here, we're jumping
    // through some extra hoops to cast it up to vec3d rather than vec3f,
    // since that's what hydra expects.
    VtVec3fArray raw = _extentAttr->GetTypedValue(shutterOffset);
    if (_index >= raw.size()) {
        TF_WARN("<%s> Attribute does not have expected index entry %d",
                _attrPath.GetText(), _index);
        return GfVec3d(0);
    }

    GfVec3d result = raw[_index];
    return result;
}

bool
UsdImagingDataSourceExtentCoordinate::GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> * outSampleTimes)
{
    return _extentAttr->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceExtent::UsdImagingDataSourceExtent(
        const UsdAttributeQuery &extentQuery,
        const SdfPath &sceneIndexPath,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (extentQuery.ValueMightBeTimeVarying()) {
        stageGlobals.FlagAsTimeVarying(sceneIndexPath,
                HdDataSourceLocator(HdExtentSchemaTokens->extent));
    }

    _attrPath = extentQuery.GetAttribute().GetPath();
    _extentAttr = HdVec3fArrayDataSource::Cast(
            UsdImagingDataSourceAttributeNew(extentQuery, stageGlobals));
}

TfTokenVector
UsdImagingDataSourceExtent::GetNames()
{
    if (!_extentAttr) {
        return {};
    }

    return {
        HdExtentSchemaTokens->min,
        HdExtentSchemaTokens->max,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceExtent::Get(const TfToken &name)
{
    // If the extents attr hasn't been defined, this prim has no extents.
    if (!_extentAttr) {
        return nullptr;
    }

    if (name == HdExtentSchemaTokens->min) {
        return UsdImagingDataSourceExtentCoordinate::New(
                _extentAttr, _attrPath, 0);
    } else if (name == HdExtentSchemaTokens->max) {
        return UsdImagingDataSourceExtentCoordinate::New(
                _extentAttr, _attrPath, 1);
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceXformResetXformStack::UsdImagingDataSourceXformResetXformStack(
        const UsdGeomXformable::XformQuery &xformQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _xformQuery(xformQuery)
    , _stageGlobals(stageGlobals)
{
}

VtValue
UsdImagingDataSourceXformResetXformStack::GetValue(
        HdSampledDataSource::Time shutterOffset)
{
    return VtValue(GetTypedValue(shutterOffset));
}

bool
UsdImagingDataSourceXformResetXformStack::GetTypedValue(
        HdSampledDataSource::Time shutterOffset)
{
    return _xformQuery.GetResetXformStack();
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceXformMatrix::UsdImagingDataSourceXformMatrix(
        const UsdGeomXformable::XformQuery &xformQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _xformQuery(xformQuery)
    , _stageGlobals(stageGlobals)
{
}

VtValue
UsdImagingDataSourceXformMatrix::GetValue(
        HdSampledDataSource::Time shutterOffset)
{
    return VtValue(GetTypedValue(shutterOffset));
}

GfMatrix4d
UsdImagingDataSourceXformMatrix::GetTypedValue(
        HdSampledDataSource::Time shutterOffset)
{
    GfMatrix4d transform;
    UsdTimeCode time = _stageGlobals.GetTime();
    if (time.IsNumeric()) {
        time = UsdTimeCode(time.GetValue() + shutterOffset);
    }
    _xformQuery.GetLocalTransformation(&transform, time);
    return transform;
}

bool
UsdImagingDataSourceXformMatrix::GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> * outSampleTimes)
{
    // XXX: Note: this code is very similar to the code in dataSourceAttribute.h
    // and the two should be kept in sync.  They are separate implementations
    // because transform values are composed from attributes, rather than being
    // a single attribute.
    UsdTimeCode time = _stageGlobals.GetTime();
    if (!_xformQuery.TransformMightBeTimeVarying() ||
        !time.IsNumeric()) {
        return false;
    }

    GfInterval interval(
        time.GetValue() + startTime,
        time.GetValue() + endTime);
    std::vector<double> timeSamples;
    _xformQuery.GetTimeSamplesInInterval(interval, &timeSamples);

    // Add boundary timesamples, if necessary.
    if (timeSamples[0] > interval.GetMin()) {
        timeSamples.insert(timeSamples.begin(), interval.GetMin());
    }
    if (timeSamples.back() < interval.GetMax()) {
        timeSamples.push_back(interval.GetMax());
    }

    // We need to convert the time array because usd uses double and
    // hydra (and prman) use float :/.
    outSampleTimes->resize(timeSamples.size());
    for (size_t i = 0; i < timeSamples.size(); ++i) {
        (*outSampleTimes)[i] = timeSamples[i];
    }

    return true;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceXform::UsdImagingDataSourceXform(
        const UsdGeomXformable::XformQuery &xformQuery,
        const SdfPath &sceneIndexPath,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _xformQuery(xformQuery)
    , _stageGlobals(stageGlobals)
{
    if (_xformQuery.TransformMightBeTimeVarying()) {
        _stageGlobals.FlagAsTimeVarying(sceneIndexPath,
                HdDataSourceLocator(HdXformSchemaTokens->xform));
    }
}

TfTokenVector
UsdImagingDataSourceXform::GetNames()
{
    return {
        HdXformSchemaTokens->matrix,
        HdXformSchemaTokens->resetXformStack,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceXform::Get(const TfToken &name)
{
    if (name == HdXformSchemaTokens->matrix) {
        return UsdImagingDataSourceXformMatrix::New(
                _xformQuery, _stageGlobals);
    } else if (name == HdXformSchemaTokens->resetXformStack) {
        return UsdImagingDataSourceXformResetXformStack::New(
                _xformQuery, _stageGlobals);
    }

    return nullptr;
}


// ----------------------------------------------------------------------------

UsdImagingDataSourceModel::UsdImagingDataSourceModel(
        const UsdGeomModelAPI &model,
        const SdfPath &sceneIndexPath,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
  : _model(model)
  , _sceneIndexPath(sceneIndexPath)
  , _stageGlobals(stageGlobals)
{
}

TfTokenVector
UsdImagingDataSourceModel::GetNames()
{
    return {
        UsdImagingModelSchemaTokens->drawMode,
        UsdImagingModelSchemaTokens->applyDrawMode,
        UsdImagingModelSchemaTokens->drawModeColor,
        UsdImagingModelSchemaTokens->cardGeometry,
        UsdImagingModelSchemaTokens->cardTextureXPos,
        UsdImagingModelSchemaTokens->cardTextureYPos,
        UsdImagingModelSchemaTokens->cardTextureZPos,
        UsdImagingModelSchemaTokens->cardTextureXNeg,
        UsdImagingModelSchemaTokens->cardTextureYNeg,
        UsdImagingModelSchemaTokens->cardTextureZNeg
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourceModel::Get(const TfToken &name)
{
    if (name == UsdImagingModelSchemaTokens->drawMode) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->drawMode);
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _model.GetModelDrawModeAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->applyDrawMode) {
        UsdPrim prim = _model.GetPrim();
        if (prim.IsModel()) {
            if (UsdModelAPI(prim).IsKind(KindTokens->component)) {
                return HdRetainedTypedSampledDataSource<bool>::New(true);
            }
        }

        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->applyDrawMode);
        return UsdImagingDataSourceAttribute<bool>::New(
            _model.GetModelApplyDrawModeAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->drawModeColor) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->drawModeColor);
        return UsdImagingDataSourceAttribute<GfVec3f>::New(
            _model.GetModelDrawModeColorAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->cardGeometry) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardGeometry);
        return UsdImagingDataSourceAttribute<TfToken>::New(
            _model.GetModelCardGeometryAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->cardTextureXPos) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureXPos);
        return UsdImagingDataSourceAttribute<SdfAssetPath>::New(
            _model.GetModelCardTextureXPosAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->cardTextureYPos) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureYPos);
        return UsdImagingDataSourceAttribute<SdfAssetPath>::New(
            _model.GetModelCardTextureYPosAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->cardTextureZPos) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureZPos);
        return UsdImagingDataSourceAttribute<SdfAssetPath>::New(
            _model.GetModelCardTextureZPosAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->cardTextureXNeg) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureXNeg);
        return UsdImagingDataSourceAttribute<SdfAssetPath>::New(
            _model.GetModelCardTextureXNegAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->cardTextureYNeg) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureYNeg);
        return UsdImagingDataSourceAttribute<SdfAssetPath>::New(
            _model.GetModelCardTextureYNegAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }
    if (name == UsdImagingModelSchemaTokens->cardTextureZNeg) {
        static const HdDataSourceLocator locator =
            UsdImagingModelSchema::GetDefaultLocator().Append(
                UsdImagingModelSchemaTokens->cardTextureZNeg);
        return UsdImagingDataSourceAttribute<SdfAssetPath>::New(
            _model.GetModelCardTextureZNegAttr(),
            _stageGlobals,
            _sceneIndexPath,
            locator);
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourcePrimOrigin::UsdImagingDataSourcePrimOrigin(
        const UsdPrim &usdPrim)
  : _usdPrim(usdPrim)
{
}

TfTokenVector
UsdImagingDataSourcePrimOrigin::GetNames()
{
    return {
        HdPrimOriginSchemaTokens->scenePath
    };
}

// If prim, say /__Prototype_1/MyXform/MySphere is inside a Usd
// Prototype (here /__Prototype_1), return the path relative to
// the prototype root (here MyXform/MySphere).
// If prim is not inside a Usd Prototype, just return (absolute) prim path.
//
// Assumes that all Usd prototype roots are children of the pseudo root.
//
static
SdfPath
_ComputePrototypeRelativePath(const UsdPrim &prim)
{
    const SdfPath path = prim.GetPath();

    const SdfPathVector prefixes = path.GetPrefixes();
    if (prefixes.empty()) {
        return path;
    }

    // Get path of potential prototype containing the prim.
    const SdfPath prototypePath = prefixes[0];
    UsdPrim prototype = prim.GetStage()->GetPrimAtPath(prototypePath);
    if (!prototype) {
        return path;
    }
    if (!prototype.IsPrototype()) {
        return path;
    }
    return path.MakeRelativePath(prototypePath);
}

HdDataSourceBaseHandle
UsdImagingDataSourcePrimOrigin::Get(const TfToken &name)
{ 
    if (name == HdPrimOriginSchemaTokens->scenePath) {
        if (!_usdPrim) {
            return nullptr;
        }

        using OriginPath = HdPrimOriginSchema::OriginPath;
        using DataSource = HdRetainedTypedSampledDataSource<OriginPath>;
        return
            DataSource::New(
                OriginPath(
                    _ComputePrototypeRelativePath(_usdPrim)));
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourcePrim::UsdImagingDataSourcePrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdPrim(usdPrim)
    , _stageGlobals(stageGlobals)
    , _primvars(nullptr)
{
}

TfTokenVector
UsdImagingDataSourcePrim::GetNames()
{
    TfTokenVector vec;

    if (!_sceneIndexPath.IsPrimPath()) {
        return vec;
    }
    
    if (_GetUsdPrim().IsA<UsdGeomImageable>()) {
        vec.push_back(HdVisibilitySchemaTokens->visibility);
        vec.push_back(HdPurposeSchemaTokens->purpose);
    }

    if (_GetUsdPrim().IsA<UsdGeomXformable>()) {
        vec.push_back(HdXformSchemaTokens->xform);
    }

    if (_GetUsdPrim().IsA<UsdGeomBoundable>()) {
        vec.push_back(HdExtentSchemaTokens->extent);
    }

    if (_GetUsdPrim().HasAPI<UsdGeomModelAPI>()) {
        vec.push_back(UsdImagingModelSchemaTokens->model);
    }

    vec.push_back(UsdImagingUsdPrimInfoSchemaTokens->__usdPrimInfo);
    vec.push_back(HdPrimOriginSchemaTokens->primOrigin);
    vec.push_back(HdPrimvarsSchemaTokens->primvars);

    return vec;
}

HdDataSourceBaseHandle
UsdImagingDataSourcePrim::Get(const TfToken &name)
{
    TRACE_FUNCTION();

    if (!_sceneIndexPath.IsPrimPath()) {
        return nullptr;
    }

    if (name == HdXformSchemaTokens->xform) {
        UsdGeomXformable xformable(_GetUsdPrim());
        if (!xformable) {
            return nullptr;
        }

        UsdGeomXformable::XformQuery xformQuery(xformable);
        if (xformQuery.HasNonEmptyXformOpOrder()) {
            return UsdImagingDataSourceXform::New(
                    xformQuery, _sceneIndexPath, _GetStageGlobals());
        } else {
            return nullptr;
        }
    } else if (name == HdPrimvarsSchemaTokens->primvars) {
        auto primvars = UsdImagingDataSourcePrimvars::AtomicLoad(_primvars);
        if (!primvars) {
            primvars = UsdImagingDataSourcePrimvars::New(
                _GetSceneIndexPath(),
                _GetUsdPrim(),
                UsdGeomPrimvarsAPI(_GetUsdPrim()),
                _GetStageGlobals());
            UsdImagingDataSourcePrimvars::AtomicStore(_primvars, primvars);
        }
        return primvars;
    } else if (name == HdVisibilitySchemaTokens->visibility) {
        UsdGeomImageable imageable(_GetUsdPrim());
        if (!imageable) {
            return nullptr;
        }

        UsdAttributeQuery visibilityQuery(imageable.GetVisibilityAttr());
        if (visibilityQuery.HasAuthoredValue()) {
            return UsdImagingDataSourceVisibility::New(
                    visibilityQuery, _sceneIndexPath, _GetStageGlobals());
        } else {
            return nullptr;
        }
    } else if (name == HdPurposeSchemaTokens->purpose) {
        UsdGeomImageable imageable(_GetUsdPrim());
        if (!imageable) {
            return nullptr;
        }

        UsdAttributeQuery purposeQuery(imageable.GetPurposeAttr());
        if (purposeQuery.HasAuthoredValue()) {
            return UsdImagingDataSourcePurpose::New(
                    purposeQuery, _GetStageGlobals());
        } else {
            return nullptr;
        }
    } else if (name == HdExtentSchemaTokens->extent) {
        UsdGeomBoundable boundable(_GetUsdPrim());
        if (!boundable) {
            return nullptr;
        }

        UsdAttributeQuery extentQuery(boundable.GetExtentAttr());
        if (extentQuery.HasAuthoredValue()) {
            return UsdImagingDataSourceExtent::New(
                extentQuery, _sceneIndexPath, _GetStageGlobals());
        } else {
            return nullptr;
        }
    } else if (name == UsdImagingModelSchemaTokens->model) {
        UsdGeomModelAPI model(_GetUsdPrim());
        if (!model) {
            return nullptr;
        }
        return UsdImagingDataSourceModel::New(
            model, _sceneIndexPath, _GetStageGlobals());
    } else if (name == UsdImagingUsdPrimInfoSchemaTokens->__usdPrimInfo) {
        return UsdImagingDataSourceUsdPrimInfo::New(
            _GetUsdPrim());
    } else if (name == HdPrimOriginSchemaTokens->primOrigin) {
        return UsdImagingDataSourcePrimOrigin::New(
            _GetUsdPrim());
    }
    return nullptr;
}

/*static*/ HdDataSourceLocatorSet
UsdImagingDataSourcePrim::Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim, 
        const TfTokenVector &properties)
{
    HdDataSourceLocatorSet locators;

    for (const TfToken &propertyName : properties) {
        if (propertyName == UsdGeomTokens->visibility) {
            locators.insert(HdVisibilitySchema::GetDefaultLocator());
        }

        if (propertyName == UsdGeomTokens->purpose) {
            locators.insert(HdPurposeSchema::GetDefaultLocator());
        }

        if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(
                    propertyName)) {
            locators.insert(HdXformSchema::GetDefaultLocator());
        }

        if (propertyName == UsdGeomTokens->extent) {
            locators.insert(HdExtentSchema::GetDefaultLocator());
        }

        // TODO: Should all this model stuff go somewhere else?

        if (propertyName == UsdGeomTokens->modelDrawMode) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->drawMode);
            locators.insert(locator);  
        }

        if (propertyName == UsdGeomTokens->modelApplyDrawMode) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->applyDrawMode);
            locators.insert(locator);
        }

        if (propertyName == UsdGeomTokens->modelDrawModeColor) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->drawModeColor);
            locators.insert(locator);
        }

        if (propertyName == UsdGeomTokens->modelCardGeometry) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->cardGeometry);
            locators.insert(locator);
        }

        if (propertyName == UsdGeomTokens->modelCardTextureXPos) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->cardTextureXPos);
            locators.insert(locator);
        }

        if (propertyName == UsdGeomTokens->modelCardTextureXNeg) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->cardTextureXNeg);
            locators.insert(locator);
        }

        if (propertyName == UsdGeomTokens->modelCardTextureYPos) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->cardTextureYPos);
            locators.insert(locator);
        }

        if (propertyName == UsdGeomTokens->modelCardTextureYNeg) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->cardTextureYNeg);
            locators.insert(locator);
        }

        if (propertyName == UsdGeomTokens->modelCardTextureZPos) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->cardTextureZPos);
            locators.insert(locator);
        }

        if (propertyName == UsdGeomTokens->modelCardTextureZNeg) {
            static const HdDataSourceLocator locator(
                UsdImagingModelSchemaTokens->model,
                UsdImagingModelSchemaTokens->cardTextureZNeg);
            locators.insert(locator);
        }

        if (UsdGeomPrimvarsAPI::CanContainPropertyName(propertyName)) {
            static const int prefixLength = 9; // "primvars:"
            locators.insert(
                HdPrimvarsSchema::GetDefaultLocator().Append(TfToken(
                    propertyName.data() + prefixLength)));
        }
    }

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
