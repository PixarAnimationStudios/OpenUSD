//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/dataSourceUsdPrimInfo.h"

#include "pxr/usdImaging/usdImaging/usdPrimInfoSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceUsdPrimInfo::UsdImagingDataSourceUsdPrimInfo(
    UsdPrim usdPrim)
  : _usdPrim(usdPrim)
{
}

UsdImagingDataSourceUsdPrimInfo::~UsdImagingDataSourceUsdPrimInfo() = default;

static
HdDataSourceBaseHandle
_SpecifierToDataSource(const SdfSpecifier specifier)
{
    struct DataSources {
        using DataSource = HdRetainedTypedSampledDataSource<TfToken>;

        DataSources()
          : def(DataSource::New(UsdImagingUsdPrimInfoSchemaTokens->def))
          , over(DataSource::New(UsdImagingUsdPrimInfoSchemaTokens->over))
          , class_(DataSource::New(UsdImagingUsdPrimInfoSchemaTokens->class_))
        {
        }

        HdDataSourceBaseHandle def;
        HdDataSourceBaseHandle over;
        HdDataSourceBaseHandle class_;
    };

    static const DataSources dataSources;

    switch(specifier) {
    case SdfSpecifierDef:
        return dataSources.def;
    case SdfSpecifierOver:
        return dataSources.over;
    case SdfSpecifierClass:
        return dataSources.class_;
    case SdfNumSpecifiers:
        break;
    }
    
    return nullptr;
}

TfTokenVector
UsdImagingDataSourceUsdPrimInfo::GetNames()
{
    TfTokenVector result = { 
        UsdImagingUsdPrimInfoSchemaTokens->isLoaded,
        UsdImagingUsdPrimInfoSchemaTokens->specifier
    };

    if (_usdPrim.IsInstance()) {
        result.push_back(UsdImagingUsdPrimInfoSchemaTokens->niPrototypePath);
    }

    if (_usdPrim.IsPrototype()) {
        result.push_back(UsdImagingUsdPrimInfoSchemaTokens->isNiPrototype);
    }

    return result;
}

HdDataSourceBaseHandle
UsdImagingDataSourceUsdPrimInfo::Get(const TfToken &name)
{
    if (name == UsdImagingUsdPrimInfoSchemaTokens->isLoaded) {
        return HdRetainedTypedSampledDataSource<bool>::New(
            _usdPrim.IsLoaded());
    }
    if (name == UsdImagingUsdPrimInfoSchemaTokens->specifier) {
        return _SpecifierToDataSource(_usdPrim.GetSpecifier());
    }
    if (name == UsdImagingUsdPrimInfoSchemaTokens->niPrototypePath) {
        if (!_usdPrim.IsInstance()) {
            return nullptr;
        }
        const UsdPrim prototype(_usdPrim.GetPrototype());
        if (!prototype) {
            return nullptr;
        }
        return HdRetainedTypedSampledDataSource<SdfPath>::New(
            prototype.GetPath());
    }
    if (name == UsdImagingUsdPrimInfoSchemaTokens->isNiPrototype) {
        if (!_usdPrim.IsPrototype()) {
            return nullptr;
        }
        return HdRetainedTypedSampledDataSource<bool>::New(true);
    }
    return nullptr;
}
        
PXR_NAMESPACE_CLOSE_SCOPE
