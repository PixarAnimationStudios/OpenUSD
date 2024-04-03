//
// Copyright 2023 Pixar
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
