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
        UsdImagingUsdPrimInfoSchemaTokens->specifier,
        UsdImagingUsdPrimInfoSchemaTokens->typeName,
        UsdImagingUsdPrimInfoSchemaTokens->isLoaded,
        UsdImagingUsdPrimInfoSchemaTokens->apiSchemas,
        UsdImagingUsdPrimInfoSchemaTokens->kind,
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
    using BoolDataSource = HdRetainedTypedSampledDataSource<bool>;
    using TokenDataSource = HdRetainedTypedSampledDataSource<TfToken>;
    using TokenArrayDataSource =
        HdRetainedTypedSampledDataSource<VtArray<TfToken>>;

    if (name == UsdImagingUsdPrimInfoSchemaTokens->specifier) {
        return _SpecifierToDataSource(_usdPrim.GetSpecifier());
    }
    if (name == UsdImagingUsdPrimInfoSchemaTokens->typeName) {
        return TokenDataSource::New(_usdPrim.GetTypeName());
    }
    if (name == UsdImagingUsdPrimInfoSchemaTokens->isLoaded) {
        return BoolDataSource::New(_usdPrim.IsLoaded());
    }
    if (name == UsdImagingUsdPrimInfoSchemaTokens->apiSchemas) {
        const TfTokenVector appliedSchemas = _usdPrim.GetAppliedSchemas();
        if (!appliedSchemas.empty()) {
            return TokenArrayDataSource::New(
                VtArray<TfToken>(appliedSchemas.begin(), appliedSchemas.end()));
        }
        return nullptr;
    }
    if (name == UsdImagingUsdPrimInfoSchemaTokens->kind) {
        TfToken kind;
        if (_usdPrim.GetKind(&kind)) {
            return TokenDataSource::New(kind);
        }
        return nullptr;
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
        return BoolDataSource::New(true);
    }
    return nullptr;
}
        
PXR_NAMESPACE_CLOSE_SCOPE
