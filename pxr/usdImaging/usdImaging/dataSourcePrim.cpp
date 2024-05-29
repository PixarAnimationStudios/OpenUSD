//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"
#include "pxr/usdImaging/usdImaging/dataSourceUsdPrimInfo.h"
#include "pxr/usdImaging/usdImaging/extentsHintSchema.h"
#include "pxr/usdImaging/usdImaging/geomModelSchema.h"
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
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/modelAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceVisibility::UsdImagingDataSourceVisibility(
        const UsdAttributeQuery &visibilityQuery,
        const SdfPath &sceneIndexPath,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _visibilityQuery(visibilityQuery)
    , _stageGlobals(stageGlobals)
{
    if (_visibilityQuery.ValueMightBeTimeVarying()) {
        _stageGlobals.FlagAsTimeVarying(
            sceneIndexPath, HdVisibilitySchema::GetDefaultLocator());
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
        TfToken purpose;
        // Purpose is uniform, so just use a retained data source.
        if (_purposeQuery.Get<TfToken>(&purpose)) {
            if (purpose == UsdGeomTokens->default_) {
                static HdDataSourceBaseHandle const ds =
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdTokens->geometry);
                return ds;
            }
            return HdRetainedTypedSampledDataSource<TfToken>::New(purpose);
        }
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceExtentCoordinate::UsdImagingDataSourceExtentCoordinate(
        const HdVec3fArrayDataSourceHandle &extentDs,
        const SdfPath &attrPath,
        unsigned int index)
    : _extentDs(extentDs)
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
    const VtVec3fArray raw = _extentDs->GetTypedValue(shutterOffset);
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
    return _extentDs->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceExtent::UsdImagingDataSourceExtent(
        const UsdAttributeQuery &extentQuery,
        const SdfPath &sceneIndexPath,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (extentQuery.ValueMightBeTimeVarying()) {
        stageGlobals.FlagAsTimeVarying(
            sceneIndexPath, HdExtentSchema::GetDefaultLocator());
    }

    _attrPath = extentQuery.GetAttribute().GetPath();
    _extentDs = HdVec3fArrayDataSource::Cast(
            UsdImagingDataSourceAttributeNew(extentQuery, stageGlobals));
}

TfTokenVector
UsdImagingDataSourceExtent::GetNames()
{
    if (!_extentDs) {
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
    if (!_extentDs) {
        return nullptr;
    }

    if (name == HdExtentSchemaTokens->min) {
        return UsdImagingDataSourceExtentCoordinate::New(
                _extentDs, _attrPath, 0);
    } else if (name == HdExtentSchemaTokens->max) {
        return UsdImagingDataSourceExtentCoordinate::New(
                _extentDs, _attrPath, 1);
    }

    return nullptr;
}

// ----------------------------------------------------------------------------

static
TfTokenVector _UsdToHdPurposes(const TfTokenVector &v)
{
    TfTokenVector result;
    for (const TfToken &usdPurpose : v) {
        if (usdPurpose == UsdGeomTokens->default_) {
            result.push_back(HdTokens->geometry);
        } else {
            result.push_back(usdPurpose);
        }
    }
    return result;
}

static
const TfTokenVector &_OrderedPurposes()
{
    static const TfTokenVector result =
        _UsdToHdPurposes(UsdGeomImageable::GetOrderedPurposeTokens());
    return result;
}

UsdImagingDataSourceExtentsHint::UsdImagingDataSourceExtentsHint(
        const UsdAttributeQuery &extentQuery,
        const SdfPath &sceneIndexPath,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (extentQuery.ValueMightBeTimeVarying()) {
        stageGlobals.FlagAsTimeVarying(
            sceneIndexPath,
            UsdImagingExtentsHintSchema::GetDefaultLocator());
    }

    _attrPath = extentQuery.GetAttribute().GetPath();
    _extentDs = HdVec3fArrayDataSource::Cast(
            UsdImagingDataSourceAttributeNew(extentQuery, stageGlobals));
}

TfTokenVector
UsdImagingDataSourceExtentsHint::GetNames()
{
    if (!_extentDs) {
        return {};
    }

    const TfTokenVector &orderedPurposes = _OrderedPurposes();

    const size_t n0 = _extentDs->GetTypedValue(0.0f).size() / 2;
    const size_t n1 = orderedPurposes.size();
    const size_t n = std::min(n0, n1);

    return TfTokenVector(orderedPurposes.begin(), orderedPurposes.begin() + n);
}

HdDataSourceBaseHandle
UsdImagingDataSourceExtentsHint::Get(const TfToken &name)
{
    if (!_extentDs) {
        return nullptr;
    }

    const TfTokenVector &orderedPurposes = _OrderedPurposes();

    const size_t n0 = _extentDs->GetTypedValue(0.0f).size() / 2;
    const size_t n1 = orderedPurposes.size();
    const size_t n = std::min(n0, n1);

    for (size_t i = 0; i < n; i++) {
        if (name == orderedPurposes[i]) {
            return
                HdExtentSchema::Builder()
                    .SetMin(
                        UsdImagingDataSourceExtentCoordinate::New(
                            _extentDs, _attrPath, 2 * i))
                    .SetMax(
                        UsdImagingDataSourceExtentCoordinate::New(
                            _extentDs, _attrPath, 2 * i + 1))
                    .Build();
        }
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
    if (timeSamples.empty() || timeSamples[0] > interval.GetMin()) {
        timeSamples.insert(timeSamples.begin(), interval.GetMin());
    }
    if (timeSamples.back() < interval.GetMax()) {
        timeSamples.push_back(interval.GetMax());
    }

    // We need to convert the time array because usd uses double and
    // hydra (and prman) use float :/.
    outSampleTimes->resize(timeSamples.size());
    for (size_t i = 0; i < timeSamples.size(); ++i) {
        (*outSampleTimes)[i] = timeSamples[i] - time.GetValue();
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
        _stageGlobals.FlagAsTimeVarying(
            sceneIndexPath, HdXformSchema::GetDefaultLocator());
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

/// \class UsdImagingDataSourcePrim_ModelAPI
///
/// Data source representing UsdModelAPI.
///
class UsdImagingDataSourcePrim_ModelAPI : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePrim_ModelAPI);
    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourcePrim_ModelAPI(const UsdModelAPI &model);
    const UsdModelAPI _model;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePrim_ModelAPI);

UsdImagingDataSourcePrim_ModelAPI::UsdImagingDataSourcePrim_ModelAPI(
        const UsdModelAPI &model)
  : _model(model)
{
}

TfTokenVector
UsdImagingDataSourcePrim_ModelAPI::GetNames()
{
    return {
        UsdImagingModelSchemaTokens->modelPath,
        UsdImagingModelSchemaTokens->assetIdentifier,
        UsdImagingModelSchemaTokens->assetName,
        UsdImagingModelSchemaTokens->assetVersion,
    };
}

HdDataSourceBaseHandle
UsdImagingDataSourcePrim_ModelAPI::Get(const TfToken &name)
{ 
    TRACE_FUNCTION();
    if (name == UsdImagingModelSchemaTokens->modelPath) {
        return HdRetainedTypedSampledDataSource<SdfPath>::New(
            _model.GetPrim().GetPath());
    } else if (name == UsdImagingModelSchemaTokens->assetIdentifier) {
        SdfAssetPath assetIdentifier;
        if (_model.GetAssetIdentifier(&assetIdentifier)) {
            return HdRetainedTypedSampledDataSource<SdfAssetPath>
                ::New(assetIdentifier);
        }
        return nullptr;
    } else if (name == UsdImagingModelSchemaTokens->assetName) {
        std::string assetName;
        if (_model.GetAssetName(&assetName)) {
            return HdRetainedTypedSampledDataSource<std::string>
                ::New(assetName);
        }
        return nullptr;
    } else if (name == UsdImagingModelSchemaTokens->assetVersion) {
        std::string assetVersion;
        if (_model.GetAssetVersion(&assetVersion)) {
            return HdRetainedTypedSampledDataSource<std::string>
                ::New(assetVersion);
        }
        return nullptr;
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
        vec.push_back(HdVisibilitySchema::GetSchemaToken());
        vec.push_back(HdPurposeSchema::GetSchemaToken());
    }

    if (_GetUsdPrim().IsA<UsdGeomXformable>()) {
        vec.push_back(HdXformSchema::GetSchemaToken());
    }

    if (_GetUsdPrim().IsA<UsdGeomBoundable>()) {
        vec.push_back(HdExtentSchema::GetSchemaToken());
    }

    if (_GetUsdPrim().IsModel()) {
        vec.push_back(UsdImagingModelSchema::GetSchemaToken());
    }
    
    if (UsdAttributeQuery(UsdGeomModelAPI(_GetUsdPrim()).GetExtentsHintAttr())
                        .HasAuthoredValue()) {
        vec.push_back(UsdImagingExtentsHintSchema::GetSchemaToken());
    }


    vec.push_back(UsdImagingUsdPrimInfoSchema::GetSchemaToken());
    vec.push_back(HdPrimOriginSchema::GetSchemaToken());
    vec.push_back(HdPrimvarsSchema::GetSchemaToken());

    return vec;
}

HdDataSourceBaseHandle
UsdImagingDataSourcePrim::Get(const TfToken &name)
{
    TRACE_FUNCTION();

    if (!_sceneIndexPath.IsPrimPath()) {
        return nullptr;
    }

    if (name == HdXformSchema::GetSchemaToken()) {
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
    } else if (name == HdPrimvarsSchema::GetSchemaToken()) {
        return UsdImagingDataSourcePrimvars::New(
                _GetSceneIndexPath(),
                _GetUsdPrim(),
                UsdGeomPrimvarsAPI(_GetUsdPrim()),
                _GetStageGlobals());
    } else if (name == HdVisibilitySchema::GetSchemaToken()) {
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
    } else if (name == HdPurposeSchema::GetSchemaToken()) {
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
    } else if (name == HdExtentSchema::GetSchemaToken()) {
        UsdGeomBoundable boundable(_GetUsdPrim());
        if (!boundable) {
            return nullptr;
        }

        UsdAttributeQuery extentQuery(boundable.GetExtentAttr());
        if (extentQuery.HasAuthoredValue()) {
            return UsdImagingDataSourceExtent::New(
                extentQuery,
                _sceneIndexPath,
                _GetStageGlobals());
        } else {
            return nullptr;
        }
    } else if (name == UsdImagingExtentsHintSchema::GetSchemaToken()) {
        // For compatibility with UsdImagingDelegate, we read the extentsHint
        // even if the prim is not a Usd model or UsdGeomModelAPI.
        UsdAttributeQuery extentsHintQuery(
            UsdGeomModelAPI(_GetUsdPrim()).GetExtentsHintAttr());
        if (extentsHintQuery.HasAuthoredValue()) {
            return UsdImagingDataSourceExtentsHint::New(
                extentsHintQuery,
                _sceneIndexPath,
                _GetStageGlobals());
        } else {
            return nullptr;
        }
    } else if (name == UsdImagingModelSchema::GetSchemaToken()) {
        if (UsdModelAPI model = UsdModelAPI(_GetUsdPrim())) {
            if (model.IsModel()) {
                return UsdImagingDataSourcePrim_ModelAPI::New(model);
            }
        }
        return nullptr;
    } else if (name == UsdImagingUsdPrimInfoSchema::GetSchemaToken()) {
        return UsdImagingDataSourceUsdPrimInfo::New(
            _GetUsdPrim());
    } else if (name == HdPrimOriginSchema::GetSchemaToken()) {
        return UsdImagingDataSourcePrimOrigin::New(
            _GetUsdPrim());
    }
    return nullptr;
}

/*static*/ HdDataSourceLocatorSet
UsdImagingDataSourcePrim::Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim, 
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType)
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

        if (propertyName == UsdGeomTokens->extentsHint) {
            locators.insert(UsdImagingExtentsHintSchema::GetDefaultLocator());
        }

        if (UsdGeomPrimvarsAPI::CanContainPropertyName(propertyName)) {
            if (invalidationType == UsdImagingPropertyInvalidationType::Resync) {
                locators.insert(
                    HdPrimvarsSchema::GetDefaultLocator());
            } else {
                static const int prefixLength = 9; // "primvars:"
                locators.insert(
                    HdPrimvarsSchema::GetDefaultLocator()
                        .Append(TfToken(propertyName.data() + prefixLength)));
            }
        }
    }

    return locators;
}

PXR_NAMESPACE_CLOSE_SCOPE
