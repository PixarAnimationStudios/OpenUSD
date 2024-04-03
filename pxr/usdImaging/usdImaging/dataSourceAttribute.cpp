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
#include "pxr/usdImaging/usdImaging/dataSourceAttribute.h"

#include "pxr/imaging/hd/dataSourceLocator.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

typedef HdSampledDataSourceHandle (*_DataSourceFactory)(
        const UsdAttributeQuery &usdAttrQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath,
        const HdDataSourceLocator &timeVaryingFlagLocator);

using _FactoryMap = std::unordered_map<SdfValueTypeName, _DataSourceFactory,
      SdfValueTypeNameHash>;

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

static _FactoryMap _CreateFactoryMap()
{
    _FactoryMap map;

    map[SdfValueTypeNames->Asset] = _FactoryImpl<SdfAssetPath>;
    map[SdfValueTypeNames->AssetArray] = _FactoryImpl<VtArray<SdfAssetPath>>;
    map[SdfValueTypeNames->Bool] = _FactoryImpl<bool>;
    map[SdfValueTypeNames->BoolArray] = _FactoryImpl<VtArray<bool>>;
    map[SdfValueTypeNames->Color3fArray] = _FactoryImpl<VtArray<GfVec3f>>;
    map[SdfValueTypeNames->Color3f] = _FactoryImpl<GfVec3f>;
    map[SdfValueTypeNames->Double] = _FactoryImpl<double>;
    map[SdfValueTypeNames->Double2] = _FactoryImpl<GfVec2d>;
    map[SdfValueTypeNames->DoubleArray] = _FactoryImpl<VtArray<double>>;
    map[SdfValueTypeNames->Double2Array] = _FactoryImpl<VtArray<GfVec2d>>;
    map[SdfValueTypeNames->Double3Array] = _FactoryImpl<VtArray<GfVec3d>>;
    map[SdfValueTypeNames->Float2Array] = _FactoryImpl<VtArray<GfVec2f>>;
    map[SdfValueTypeNames->Float2] = _FactoryImpl<GfVec2f>;
    map[SdfValueTypeNames->Float3Array] = _FactoryImpl<VtArray<GfVec3f>>;
    map[SdfValueTypeNames->Float3] = _FactoryImpl<GfVec3f>;
    map[SdfValueTypeNames->Float4] = _FactoryImpl<GfVec4f>;
    map[SdfValueTypeNames->Float4Array] = _FactoryImpl<VtArray<GfVec4f>>;
    map[SdfValueTypeNames->FloatArray] = _FactoryImpl<VtArray<float>>;
    map[SdfValueTypeNames->Float] = _FactoryImpl<float>;
    map[SdfValueTypeNames->IntArray] = _FactoryImpl<VtArray<int>>;
    map[SdfValueTypeNames->Int] = _FactoryImpl<int>;
    map[SdfValueTypeNames->Int2] = _FactoryImpl<GfVec2i>;
    map[SdfValueTypeNames->Matrix4dArray] = _FactoryImpl<VtArray<GfMatrix4d>>;
    map[SdfValueTypeNames->Matrix4d] = _FactoryImpl<GfMatrix4d>;
    map[SdfValueTypeNames->Normal3fArray] = _FactoryImpl<VtArray<GfVec3f>>;
    map[SdfValueTypeNames->Normal3f] = _FactoryImpl<GfVec3f>;
    map[SdfValueTypeNames->PathExpression] = _FactoryImpl<SdfPathExpression>;
    map[SdfValueTypeNames->Point3fArray] = _FactoryImpl<VtArray<GfVec3f>>;
    map[SdfValueTypeNames->Point3f] = _FactoryImpl<GfVec3f>;
    map[SdfValueTypeNames->Quath] = _FactoryImpl<GfQuath>;
    map[SdfValueTypeNames->QuathArray] = _FactoryImpl<VtArray<GfQuath>>;
    map[SdfValueTypeNames->Quatf] = _FactoryImpl<GfQuatf>;
    map[SdfValueTypeNames->QuatfArray] = _FactoryImpl<VtArray<GfQuatf>>;
    map[SdfValueTypeNames->Quatd] = _FactoryImpl<GfQuatd>;
    map[SdfValueTypeNames->QuatdArray] = _FactoryImpl<VtArray<GfQuatd>>;
    map[SdfValueTypeNames->StringArray] = _FactoryImpl<VtArray<std::string>>;
    map[SdfValueTypeNames->String] = _FactoryImpl<std::string>;
    map[SdfValueTypeNames->TexCoord2dArray] = _FactoryImpl<VtArray<GfVec2d>>;
    map[SdfValueTypeNames->TexCoord2d] = _FactoryImpl<GfVec2d>;
    map[SdfValueTypeNames->TexCoord2fArray] = _FactoryImpl<VtArray<GfVec2f>>;
    map[SdfValueTypeNames->TexCoord2f] = _FactoryImpl<GfVec2f>;
    map[SdfValueTypeNames->TexCoord2hArray] = _FactoryImpl<VtArray<GfVec2h>>;
    map[SdfValueTypeNames->TexCoord2h] = _FactoryImpl<GfVec2h>;
    map[SdfValueTypeNames->TexCoord3dArray] = _FactoryImpl<VtArray<GfVec3d>>;
    map[SdfValueTypeNames->TexCoord3d] = _FactoryImpl<GfVec3d>;
    map[SdfValueTypeNames->TexCoord3fArray] = _FactoryImpl<VtArray<GfVec3f>>;
    map[SdfValueTypeNames->TexCoord3f] = _FactoryImpl<GfVec3f>;
    map[SdfValueTypeNames->TexCoord3hArray] = _FactoryImpl<VtArray<GfVec3h>>;
    map[SdfValueTypeNames->TexCoord3h] = _FactoryImpl<GfVec3h>;
    map[SdfValueTypeNames->Token] = _FactoryImpl<TfToken>;
    map[SdfValueTypeNames->TokenArray] = _FactoryImpl<VtArray<TfToken>>;
    map[SdfValueTypeNames->Vector3fArray] = _FactoryImpl<VtArray<GfVec3f>>;
    map[SdfValueTypeNames->Vector3f] = _FactoryImpl<GfVec3f>;

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
            usdAttrQuery.GetAttribute().GetTypeName());
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
