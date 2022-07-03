//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_DATASOURCETYPEDEFS_H
#define PXR_IMAGING_HD_DATASOURCETYPEDEFS_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/array.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Numeric
using HdIntDataSource = HdTypedSampledDataSource<int>;
using HdIntDataSourceHandle = HdIntDataSource::Handle;
using HdIntArrayDataSource = HdTypedSampledDataSource<VtArray<int>>;
using HdIntArrayDataSourceHandle = HdIntArrayDataSource::Handle;

using HdSizetDataSource = HdTypedSampledDataSource<size_t>;
using HdSizetDataSourceHandle = HdSizetDataSource::Handle;

using HdFloatDataSource = HdTypedSampledDataSource<float>;
using HdFloatDataSourceHandle = HdFloatDataSource::Handle;
using HdFloatArrayDataSource = HdTypedSampledDataSource<VtArray<float>>;
using HdFloatArrayDataSourceHandle = HdFloatArrayDataSource::Handle;

// Bool
using HdBoolDataSource = HdTypedSampledDataSource<bool>;
using HdBoolDataSourceHandle = HdBoolDataSource::Handle;
using HdBoolArrayDataSource = HdTypedSampledDataSource<VtArray<bool>>;
using HdBoolArrayDataSourceHandle = HdBoolArrayDataSource::Handle;

// String
using HdTokenDataSource = HdTypedSampledDataSource<TfToken>;
using HdTokenDataSourceHandle = HdTokenDataSource::Handle;
using HdTokenArrayDataSource = HdTypedSampledDataSource<VtArray<TfToken>>;
using HdTokenArrayDataSourceHandle = HdTokenArrayDataSource::Handle;

using HdPathDataSource = HdTypedSampledDataSource<SdfPath>;
using HdPathDataSourceHandle = HdPathDataSource::Handle;
using HdPathArrayDataSource = HdTypedSampledDataSource<VtArray<SdfPath>>;
using HdPathArrayDataSourceHandle = HdPathArrayDataSource::Handle;

using HdStringDataSource = HdTypedSampledDataSource<std::string>;
using HdStringDataSourceHandle = HdStringDataSource::Handle;

using HdAssetPathDataSource = HdTypedSampledDataSource<SdfAssetPath>;
using HdAssetPathDataSourceHandle = HdAssetPathDataSource::Handle;

// Linear algebra
using HdVec2iDataSource = HdTypedSampledDataSource<GfVec2i>;
using HdVec2iDataSourceHandle = HdVec2iDataSource::Handle;
using HdVec2fDataSource = HdTypedSampledDataSource<GfVec2f>;
using HdVec2fDataSourceHandle = HdVec2fDataSource::Handle;
using HdVec2fArrayDataSource = HdTypedSampledDataSource<VtArray<GfVec2f>>;
using HdVec2fArrayDataSourceHandle = HdVec2fArrayDataSource::Handle;

using HdVec3iDataSource = HdTypedSampledDataSource<GfVec3i>;
using HdVec3iDataSourceHandle = HdVec3iDataSource::Handle;
using HdVec3fDataSource = HdTypedSampledDataSource<GfVec3f>;
using HdVec3fDataSourceHandle = HdVec3fDataSource::Handle;
using HdVec3fArrayDataSource = HdTypedSampledDataSource<VtArray<GfVec3f>>;
using HdVec3fArrayDataSourceHandle = HdVec3fArrayDataSource::Handle;

using HdVec4iDataSource = HdTypedSampledDataSource<GfVec4i>;
using HdVec4iDataSourceHandle = HdVec4iDataSource::Handle;
using HdVec3dDataSource = HdTypedSampledDataSource<GfVec3d>;
using HdVec3dDataSourceHandle = HdVec3dDataSource::Handle;
using HdVec3dArrayDataSource = HdTypedSampledDataSource<VtArray<GfVec3d>>;
using HdVec3dArrayDataSourceHandle = HdVec3dArrayDataSource::Handle;

using HdMatrixDataSource = HdTypedSampledDataSource<GfMatrix4d>;
using HdMatrixDataSourceHandle = HdMatrixDataSource::Handle;
using HdMatrixArrayDataSource = HdTypedSampledDataSource<VtArray<GfMatrix4d>>;
using HdMatrixArrayDataSourceHandle = HdMatrixArrayDataSource::Handle;

// Locator
using HdLocatorDataSource = HdTypedSampledDataSource<HdDataSourceLocator>;
using HdLocatorDataSourceHandle = HdLocatorDataSource::Handle;

// Enum
using HdFormatDataSource = HdTypedSampledDataSource<HdFormat>;
using HdFormatDataSourceHandle = HdFormatDataSource::Handle;

using HdTupleTypeDataSource = HdTypedSampledDataSource<HdTupleType>;
using HdTupleTypeDataSourceHandle = HdTupleTypeDataSource::Handle;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_DATASOURCETYPEDEFS_H
