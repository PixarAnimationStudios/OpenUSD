//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"

#include "pxr/imaging/hd/dataSourceLocator.h"

#include "pxr/usd/usdShade/udimUtils.h"
#include "pxr/usd/sdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// We need to find the first layer that changes the value
// of the parameter so that we anchor relative paths to that.
static
SdfLayerHandle
_FindLayerHandle(const UsdAttribute& attr, const UsdTimeCode& time) {
    for (const auto& spec: attr.GetPropertyStack(time)) {
        if (spec->HasDefaultValue() ||
            spec->GetLayer()->GetNumTimeSamplesForPath(
                spec->GetPath()) > 0) {
            return spec->GetLayer();
        }
    }
    return TfNullPtr;
}

class UsdImagingDataSourceAssetPathAttribute :
    public UsdImagingDataSourceAttribute<SdfAssetPath>
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceAssetPathAttribute);

    /// Returns the extracted SdfAssetPath value of the attribute at
    /// \p shutterOffset, with proper handling for UDIM paths.
    SdfAssetPath
    GetTypedValue(const HdSampledDataSource::Time shutterOffset) override
    {
        using Parent = UsdImagingDataSourceAttribute<SdfAssetPath>;
        SdfAssetPath result = Parent::GetTypedValue(shutterOffset);
        if (!UsdShadeUdimUtils::IsUdimIdentifier(result.GetAssetPath())) {
            return result;
        }
        UsdTimeCode time = _stageGlobals.GetTime();
        if (time.IsNumeric()) {
            time = UsdTimeCode(time.GetValue() + shutterOffset);
        }
        const std::string resolvedPath = UsdShadeUdimUtils::ResolveUdimPath(
            result.GetAssetPath(),
            _FindLayerHandle(_usdAttrQuery.GetAttribute(), time));
        if (!resolvedPath.empty()) {
            result = SdfAssetPath(result.GetAssetPath(), resolvedPath);
        }
        return result;
    }

protected:
    UsdImagingDataSourceAssetPathAttribute(
            const UsdAttribute &usdAttr,
            const UsdImagingDataSourceStageGlobals &stageGlobals,
            const SdfPath &sceneIndexPath = SdfPath::EmptyPath(),
            const HdDataSourceLocator &timeVaryingFlagLocator =
                    HdDataSourceLocator::EmptyLocator())
        : UsdImagingDataSourceAttribute<SdfAssetPath>(
            usdAttr, stageGlobals, sceneIndexPath, timeVaryingFlagLocator)
    { }

    UsdImagingDataSourceAssetPathAttribute(
            const UsdAttributeQuery &usdAttrQuery,
            const UsdImagingDataSourceStageGlobals &stageGlobals,
            const SdfPath &sceneIndexPath = SdfPath::EmptyPath(),
            const HdDataSourceLocator &timeVaryingFlagLocator =
                    HdDataSourceLocator::EmptyLocator())
        : UsdImagingDataSourceAttribute<SdfAssetPath>(
            usdAttrQuery, stageGlobals, sceneIndexPath, timeVaryingFlagLocator)
    { }
};

typedef HdSampledDataSourceHandle (*_DataSourceFactory)(
        const UsdAttributeQuery &usdAttrQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &timeVaryingFlagLocator);

template <typename T>
HdSampledDataSourceHandle _FactoryImpl(
        const UsdAttributeQuery &usdAttrQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &timeVaryingFlagLocator)
{
    return UsdImagingDataSourceAttribute<T>::New(
            usdAttrQuery, stageGlobals, sceneIndexPath, timeVaryingFlagLocator);
}

template <>
HdSampledDataSourceHandle _FactoryImpl<SdfAssetPath>(
        const UsdAttributeQuery &usdAttrQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &timeVaryingFlagLocator)
{
    return UsdImagingDataSourceAssetPathAttribute::New(
            usdAttrQuery, stageGlobals, sceneIndexPath, timeVaryingFlagLocator);
}

using _FactoryMap = std::unordered_map<TfType, _DataSourceFactory, TfHash>;

static _FactoryMap _CreateFactoryMap()
{
    _FactoryMap map;

    // Instantiate _FactoryImpl for all Sdf value types.
    // See usd/sdf/types.h for details on these macros.
    SdfValueTypeName tn;
#define _REGISTER_FACTORY(unused, tuple) \
    tn = SdfValueTypeNames->TF_PP_TUPLE_ELEM(0, tuple); \
    map[tn.GetType()] = _FactoryImpl< SDF_VALUE_CPP_TYPE(tuple) >; \
    /* Not all Sdf value types have corresponding array types */ \
    if (const SdfValueTypeName arrayTn = tn.GetArrayType()) { \
        map[arrayTn.GetType()] = \
            _FactoryImpl< SDF_VALUE_CPP_ARRAY_TYPE(tuple) >; \
    }
    TF_PP_SEQ_FOR_EACH(_REGISTER_FACTORY, ~, SDF_VALUE_TYPES);
#undef _REGISTER_FACTORY

    return map;
}

}

HdSampledDataSourceHandle
UsdImagingDataSourceAttributeNew(
        const UsdAttributeQuery &usdAttrQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &timeVaryingFlagLocator)
{
    if (!TF_VERIFY(usdAttrQuery.GetAttribute())) {
        return nullptr;
    }

    static const _FactoryMap _factoryMap = _CreateFactoryMap();

    const _FactoryMap::const_iterator i = _factoryMap.find(
            usdAttrQuery.GetAttribute().GetTypeName().GetType());
    if (i != _factoryMap.end()) {
        _DataSourceFactory factory = i->second;
        return factory(usdAttrQuery, stageGlobals,
                sceneIndexPath, timeVaryingFlagLocator);
    } else {
        TF_WARN("<%s> Unable to create attribute datasource for type '%s'",
            usdAttrQuery.GetAttribute().GetPath().GetText(),
            usdAttrQuery.GetAttribute().GetTypeName().GetAsToken().GetText());
        return nullptr;
    }
}

HdSampledDataSourceHandle
UsdImagingDataSourceAttributeNew(
        const UsdAttribute &usdAttr,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &timeVaryingFlagLocator)
{
    return UsdImagingDataSourceAttributeNew(
            UsdAttributeQuery(usdAttr),
            stageGlobals,
            sceneIndexPath,
            timeVaryingFlagLocator);
}

PXR_NAMESPACE_CLOSE_SCOPE
