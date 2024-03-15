//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#include "pxr/usdImaging/plugin/hdUsdWriter/openvdbasset.h"

#include "pxr/usd/usdVol/openVDBAsset.h"

PXR_NAMESPACE_OPEN_SCOPE

HdUsdWriterOpenvdbAsset::HdUsdWriterOpenvdbAsset(const SdfPath& id) : HdField(id)
{
}

void HdUsdWriterOpenvdbAsset::Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits)
{
    const auto& id = GetId();

    if (*dirtyBits & HdField::DirtyTransform)
    {
        _transform = sceneDelegate->GetTransform(id);
    }

    if (*dirtyBits & HdField::DirtyParams)
    {
        _filePath = HdUsdWriterGet<SdfAssetPath>(sceneDelegate, id, UsdVolTokens->filePath);
        _fieldName = HdUsdWriterGet<TfToken>(sceneDelegate, id, UsdVolTokens->fieldName);
        _fieldIndex = HdUsdWriterGet<int>(sceneDelegate, id, UsdVolTokens->fieldIndex);
        _fieldDataType = HdUsdWriterGet<TfToken>(sceneDelegate, id, UsdVolTokens->fieldDataType);
        _vectorDataRoleHint = HdUsdWriterGet<TfToken>(sceneDelegate, id, UsdVolTokens->vectorDataRoleHint);
        _fieldClass = HdUsdWriterGet<TfToken>(sceneDelegate, id, UsdVolTokens->fieldClass);
    }

    *dirtyBits = HdField::Clean;
}

HdDirtyBits HdUsdWriterOpenvdbAsset::GetInitialDirtyBitsMask() const
{
    return HdField::DirtyTransform | HdField::DirtyParams;
}

void HdUsdWriterOpenvdbAsset::SerializeToUsd(const UsdStagePtr &stage)
{
    const auto& id = GetId();
    auto openvdbAsset = UsdVolOpenVDBAsset::Define(stage, id);

    HdUsdWriterPopOptional(_transform,
        [&](const auto& transform)
        {
            UsdGeomXformable xform(openvdbAsset.GetPrim());
            auto transformOp = xform.AddTransformOp();
            transformOp.Set(transform);
        });

    HdUsdWriterPopOptional(_filePath,
        [&](const auto& filePath)
        {
            // We are using the non-resolved asset path to avoid testing absolute paths.
            openvdbAsset.CreateFilePathAttr().Set(SdfAssetPath(filePath.GetAssetPath()));
        });

    HdUsdWriterPopOptional(_fieldName, [&](const auto& fieldName) { openvdbAsset.CreateFieldNameAttr().Set(fieldName); });
    HdUsdWriterPopOptional(_fieldIndex, [&](const auto& fieldIndex) { openvdbAsset.CreateFieldIndexAttr().Set(fieldIndex); });
    HdUsdWriterPopOptional(
        _fieldDataType, [&](const auto& fieldDataType) { openvdbAsset.CreateFieldDataTypeAttr().Set(fieldDataType); });
    HdUsdWriterPopOptional(_vectorDataRoleHint, [&](const auto& vectorDataRoleHint)
                      { openvdbAsset.CreateVectorDataRoleHintAttr().Set(vectorDataRoleHint); });
    HdUsdWriterPopOptional(_fieldClass, [&](const auto& fieldClass) { openvdbAsset.CreateFieldClassAttr().Set(fieldClass); });
}

PXR_NAMESPACE_CLOSE_SCOPE
