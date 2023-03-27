//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdVolImaging/dataSourceFieldAsset.h"
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"
#include "pxr/usd/usdVol/openVDBAsset.h"
#include "pxr/usd/usdVol/field3DAsset.h"

#include "pxr/imaging/hd/volumeFieldSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceFieldAsset::UsdImagingDataSourceFieldAsset(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : _sceneIndexPath(sceneIndexPath)
    , _usdPrim(usdPrim)
    , _stageGlobals(stageGlobals)
{
}

UsdImagingDataSourceFieldAsset::~UsdImagingDataSourceFieldAsset() = default;

static
TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}

template<typename FieldSubclass>
static
const TfTokenVector &
_GetStaticUsdAttributeNames()
{
    // Get names from the subclass of UsdVolFieldAsset and
    // names inherited from FieldAsset schema but stop there
    // and do not pick up, e.g., the xform attributes which are
    // handled elsewhere and under a different data source
    // locator.
    //
    static TfTokenVector result = _ConcatenateAttributeNames(
        UsdVolFieldAsset::GetSchemaAttributeNames(
            /* includeInherited = */ false),
        FieldSubclass::GetSchemaAttributeNames(
            /* includeInherited = */ false));
    return result;
}

static
const TfTokenVector &
_GetUsdAttributeNames(UsdPrim usdPrim)
{
    if (usdPrim.IsA<UsdVolOpenVDBAsset>()) {
        return _GetStaticUsdAttributeNames<UsdVolOpenVDBAsset>();
    }

    if (usdPrim.IsA<UsdVolField3DAsset>()) {
        return _GetStaticUsdAttributeNames<UsdVolField3DAsset>();
    }

    TF_CODING_ERROR("Unsupported field type.");

    static TfTokenVector empty;
    return empty;
}

TfTokenVector
UsdImagingDataSourceFieldAsset::GetNames()
{
    return _GetUsdAttributeNames(_usdPrim);
}

HdDataSourceBaseHandle
UsdImagingDataSourceFieldAsset::Get(const TfToken &name)
{
    if (UsdAttribute attr = _usdPrim.GetAttribute(name)) {
        return UsdImagingDataSourceAttributeNew(
            attr,
            _stageGlobals,
            _sceneIndexPath,
            HdVolumeFieldSchema::GetDefaultLocator().Append(name));
    } else {
        return nullptr;
    }
}

// ----------------------------------------------------------------------------

UsdImagingDataSourceFieldAssetPrim::UsdImagingDataSourceFieldAssetPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
    : UsdImagingDataSourcePrim(sceneIndexPath, usdPrim, stageGlobals)
{
}

TfTokenVector 
UsdImagingDataSourceFieldAssetPrim::GetNames()
{
    TfTokenVector result = UsdImagingDataSourcePrim::GetNames();
    result.push_back(HdVolumeFieldSchemaTokens->volumeField);
    return result;
}

HdDataSourceBaseHandle 
UsdImagingDataSourceFieldAssetPrim::Get(const TfToken & name)
{
    if (name == HdVolumeFieldSchemaTokens->volumeField) {
        return UsdImagingDataSourceFieldAsset::New(
                _GetSceneIndexPath(),
                _GetUsdPrim(),
                _GetStageGlobals());
    } 

    return UsdImagingDataSourcePrim::Get(name);
}


PXR_NAMESPACE_CLOSE_SCOPE
