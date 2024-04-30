//
// Copyright 2019 Pixar
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
#include "hdPrman/volume.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdVol/tokens.h"
#include "pxr/usdImaging/usdVolImaging/tokens.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/js/json.h"
#include "pxr/base/js/types.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/fileUtils.h"

#ifdef PXR_OPENVDB_SUPPORT_ENABLED
#include "pxr/imaging/hioOpenVDB/utils.h"
#endif

#include "Riley.h"
#include "RiTypesHelper.h"
#include "RixShadingUtils.h"
#include "RixPredefinedStrings.hpp"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_Field::HdPrman_Field(TfToken const& typeId, SdfPath const& id)
    : HdField(id), _typeId(typeId)
{
}

void HdPrman_Field::Sync(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdDirtyBits *dirtyBits)
{
    if (*dirtyBits & DirtyParams) {
        // Force volume prim to pick up the new field resources -
        // in the same way as in HdStField::Sync.
        //
        // Ideally, this would be more fine-grained than blasting all
        // rprims.
        HdChangeTracker& changeTracker =
            sceneDelegate->GetRenderIndex().GetChangeTracker();
        changeTracker.MarkAllRprimsDirty(HdChangeTracker::DirtyVolumeField);
    }

    *dirtyBits = Clean;
}

void HdPrman_Field::Finalize(HdRenderParam *renderParam)
{
}

HdDirtyBits HdPrman_Field::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtVolume(), so it should list every data item
    // that _PopluateRtVolume requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyMaterialId
        ;
    return (HdDirtyBits)mask;
}

HdPrman_Volume::HdPrman_Volume(SdfPath const& id, const bool isMeshLight)
    : BASE(id)
    , _isMeshLight(isMeshLight)
{
}

bool HdPrman_Volume::_PrototypeOnly()
{
    return _isMeshLight;
}

HdDirtyBits
HdPrman_Volume::GetInitialDirtyBitsMask() const
{
    // The initial dirty bits control what data is available on the first
    // run through _PopulateRtVolume(), so it should list every data item
    // that _PopluateRtVolume requests.
    int mask = HdChangeTracker::Clean
        | HdChangeTracker::DirtyTransform
        | HdChangeTracker::DirtyVisibility
        | HdChangeTracker::DirtyPrimvar
        | HdChangeTracker::DirtyMaterialId
        | HdChangeTracker::DirtyInstancer
        ;

    return (HdDirtyBits)mask;
}

namespace { // anonymous namespace

HdPrman_Volume::FieldType
_DetermineOpenVDBFieldType(HdSceneDelegate *sceneDelegate,
                           SdfPath const& fieldId)
{
    VtValue fieldDataTypeValue =
        sceneDelegate->Get(fieldId, UsdVolTokens->fieldDataType);
    if (!fieldDataTypeValue.IsHolding<TfToken>()) {
        TF_WARN("Missing fieldDataType attribute on volume field prim %s. "
                "Assuming float.", fieldId.GetText());
        return HdPrman_Volume::FieldType::FloatType;
    }
    const TfToken& fieldDataType = fieldDataTypeValue.UncheckedGet<TfToken>();

    if (fieldDataType == UsdVolTokens->half ||
        fieldDataType == UsdVolTokens->float_ ||
        fieldDataType == UsdVolTokens->double_) {
        return HdPrman_Volume::FieldType::FloatType;
    }

    if (fieldDataType == UsdVolTokens->int_ ||
        fieldDataType == UsdVolTokens->uint ||
        fieldDataType == UsdVolTokens->int64) {
        // Not yet supported by impl_openvdb plugin
        return HdPrman_Volume::FieldType::IntType;
    }

    if (fieldDataType == UsdVolTokens->half2 ||
        fieldDataType == UsdVolTokens->float2 ||
        fieldDataType == UsdVolTokens->double2) {
        // Not yet supported by impl_openvdb plugin
        return HdPrman_Volume::FieldType::Float2Type;
    }

    if (fieldDataType == UsdVolTokens->int2) {
        // Not yet supported by impl_openvdb plugin
        return HdPrman_Volume::FieldType::Int2Type;
    }

    if (fieldDataType == UsdVolTokens->half3 ||
        fieldDataType == UsdVolTokens->float3 ||
        fieldDataType == UsdVolTokens->double3) {

        // The role hint for vector data is optional
        TfToken vectorDataRoleHint;
        VtValue roleHint =
            sceneDelegate->Get(fieldId, UsdVolTokens->vectorDataRoleHint);
        if (roleHint.IsHolding<TfToken>()) {
            vectorDataRoleHint = roleHint.UncheckedGet<TfToken>();
        }

        if (vectorDataRoleHint == UsdVolTokens->Color) {
            return HdPrman_Volume::FieldType::ColorType;
        } else if (vectorDataRoleHint == UsdVolTokens->Point) {
            return HdPrman_Volume::FieldType::PointType;
        } else if (vectorDataRoleHint == UsdVolTokens->Normal) {
            return HdPrman_Volume::FieldType::NormalType;
        } else if (vectorDataRoleHint == UsdVolTokens->Vector) {
            return HdPrman_Volume::FieldType::VectorType;
        } else if (vectorDataRoleHint == UsdVolTokens->None_) {
            // Fall through
        } else if (!vectorDataRoleHint.IsEmpty()) {
            TF_WARN("Unknown vectorDataRoleHint value '%s' on volume field prim"
                    " %s. Treating it as a regular float3 field.",
                    vectorDataRoleHint.GetText(), fieldId.GetText());
        }

        return HdPrman_Volume::FieldType::Float3Type;
    }

    if (fieldDataType == UsdVolTokens->int3) {
        // Not yet supported by impl_openvdb plugin
        return HdPrman_Volume::FieldType::Int3Type;
    }

    if (fieldDataType == UsdVolTokens->matrix3d ||
        fieldDataType == UsdVolTokens->matrix4d) {
        // Not yet supported by impl_openvdb plugin
        return HdPrman_Volume::FieldType::MatrixType;
    }

    if (fieldDataType == UsdVolTokens->quatd) {
        // Not yet supported by impl_openvdb plugin
        return HdPrman_Volume::FieldType::Float4Type;
    }

    if (fieldDataType == UsdVolTokens->bool_ ||
        fieldDataType == UsdVolTokens->mask) {
        // Not yet supported by impl_openvdb plugin
        return HdPrman_Volume::FieldType::IntType;
    }

    if (fieldDataType == UsdVolTokens->string) {
        // Not yet supported by impl_openvdb plugin
        return HdPrman_Volume::FieldType::StringType;
    }

    TF_WARN("Unsupported OpenVDB fieldDataType value '%s' on volume field "
            "prim %s. Assuming float.",
            fieldDataType.GetText(), fieldId.GetText());

    return HdPrman_Volume::FieldType::FloatType;
}

void
_EmitOpenVDBVolume(HdSceneDelegate *sceneDelegate,
                   const SdfPath& id,
                   const HdVolumeFieldDescriptorVector& fields,
                   RtPrimVarList* primvars)
{
    static const RtUString blobbydsoImplOpenVDB("blobbydso:impl_openvdb");

    if (fields.empty()) {
        return;
    }

    auto const& firstfield = fields[0];

    // There is an implicit assumption that all the fields on this volume are
    // extracted from the same .vdb file, which is determined once from the
    // first field.
    const VtValue filePath = sceneDelegate->Get(firstfield.fieldId,
                                                HdFieldTokens->filePath);
    const SdfAssetPath &fileAssetPath = filePath.Get<SdfAssetPath>();

    std::string volumeAssetPath = fileAssetPath.GetResolvedPath();
    if (volumeAssetPath.empty()) {
        volumeAssetPath = fileAssetPath.GetAssetPath();
    }

    // This will be the first of the string args supplied to the blobbydso. 
    std::string vdbSource;
    // JSON args.
    JsObject jsonData;

#ifndef PXR_OPENVDB_SUPPORT_ENABLED
    vdbSource = volumeAssetPath;
#else
    // If volumeAssetPath is an actual file path, copy it into the vdbSource
    // string, prepended with a "file:" tag.
    if (TfIsFile(volumeAssetPath)) {
        vdbSource = "file:" + volumeAssetPath;

    } else {
        // volumeAssetPath is not a file path. Attempt to resolve
        // it as an ArAsset and retrieve vdb grids from that asset.
        openvdb::GridPtrVecPtr gridVecPtr =
            HioOpenVDBGridsFromAsset(volumeAssetPath);

        if (!gridVecPtr) {
            TF_WARN("Failed to retrieve VDB grids from %s.",
                    volumeAssetPath.c_str());
            return;
        }

        // Allocate a new vector of vdb grid pointers on the heap. The contents
        // are copied from gridVecPtr. (This copy should be fairly cheap since
        // the elements are just shared pointers).
        openvdb::GridPtrVec* grids = new openvdb::GridPtrVec(*gridVecPtr);

        // Ownership of this new vector is given to RixStorage, which
        // will take care of clean-up when rendering is complete.
        RixContext* context = RixGetContext();
        RixStorage* storage =
            static_cast<RixStorage*>(context->GetRixInterface(k_RixGlobalData));
        if (!storage) {
            TF_WARN("Failed to access RixStorage interface.");
            return;
        }

        // Create a unique RixStorage key by combining the id
        // and the raw pointer address of the grids vector.
        std::stringstream stream;
        stream << id << "@" << static_cast<void*>(grids);
        const std::string key(stream.str());

        // Store the grids vector in RixStorage.
        // This will allow the impl_openvdb blobbydso to retrieve it.
        storage->Lock();
        storage->Set(RtUString(key.c_str()), grids,
            [](RixContext* context, void* data) // clean-up function
            {
                if (auto grids = static_cast<openvdb::GridPtrVec*>(data)) {
                    grids->clear();
                    delete grids;
                }
            });
        storage->Unlock();

        // Copy key into the vdbSource string, prepended with a "key:" tag.
        vdbSource = "key:" + key;

        // Build up JSON args for grid groups. For now we assume all grids in
        // the VDB provided should be included.
        std::map<std::string, JsArray> indexMap;
        for (const auto& grid : *grids) {
            if (auto meta = grid->getMetadata<openvdb::TypedMetadata<int>>("index")) {
                indexMap[grid->getName()].emplace_back(meta->value());
            }
        }

        if (!indexMap.empty()) {
            JsArray gridGroups;
            for (const auto& entry : indexMap) {
                gridGroups.emplace_back(JsObject {
                    { "name", JsValue(entry.first) },
                    { "indices", entry.second },
                });
            }

            jsonData["gridGroups"] = std::move(gridGroups);
        }
    }
#endif

    const VtValue fieldNameVal = sceneDelegate->Get(firstfield.fieldId,
                                                    HdFieldTokens->fieldName);
    const TfToken &fieldName = fieldNameVal.Get<TfToken>();

    const std::string jsonOpts = JsWriteToString(JsValue(std::move(jsonData)));

    primvars->SetString(RixStr.k_Ri_type, blobbydsoImplOpenVDB);

    const std::array<RtUString, 4> sa = {
        RtUString(vdbSource.c_str()),
        RtUString(fieldName.GetText()),
        RtUString(""),
        RtUString(jsonOpts.c_str())
    };
    primvars->SetStringArray(RixStr.k_blobbydso_stringargs, sa.data(), sa.size());

    // The individual fields of this volume need to be declared as primvars
    for (HdVolumeFieldDescriptor const& field : fields) {
        HdPrman_Volume::DeclareFieldPrimvar(
            primvars,
            RtUString(field.fieldName.GetText()),
            _DetermineOpenVDBFieldType(sceneDelegate, field.fieldId));
    }
}

// Returns the prim type token of a list of fields, if all the fields have the
// same type. If there are no fields or the types are inconsistent it returns
// the empty token.
TfToken
_DetermineConsistentFieldPrimType(const HdVolumeFieldDescriptorVector& fields)
{
    if (fields.empty()) {
        return TfToken();
    }

    HdVolumeFieldDescriptorVector::const_iterator iter = fields.begin();
    TfToken const& fieldPrimType = iter->fieldPrimType;
    ++iter;

    for (; iter != fields.end(); ++iter) {
        if (iter->fieldPrimType != fieldPrimType) {
            return TfToken();
        }
    }

    return fieldPrimType;
}

} // end anonymous namespace

/* static */
HdPrman_Volume::_VolumeEmitterMap&
HdPrman_Volume::_GetVolumeEmitterMap()
{
    // Note, the volumeEmitters map is static inside of this method to ensure
    // that it will be initialized the first time this method is called.
    // Initialization of static members of classes or globals is less clearly
    // defined.
    static _VolumeEmitterMap volumeEmitters = {
        // Pre-populate the map with the default implementation for OpenVDB
        {UsdVolImagingTokens->openvdbAsset, _EmitOpenVDBVolume}
    };

    return volumeEmitters;
}

/* static */
bool
HdPrman_Volume::AddVolumeTypeEmitter(TfToken const& fieldPrimType,
                                     HdPrman_VolumeTypeEmitter emitterFunc,
                                     bool overrideExisting)
{
    auto pair = _GetVolumeEmitterMap().insert({fieldPrimType, emitterFunc});
    // Set entry if overriding and there was a previous entry
    if (overrideExisting || pair.second) {
        pair.first->second = emitterFunc;
        return true;
    }

    return false;
}

/* static */
void
HdPrman_Volume::DeclareFieldPrimvar(RtPrimVarList* primvars,
                                    RtUString const& fieldName,
                                    FieldType type)
{
    RtDetailType detailType = RtDetailType::k_varying;

    // Note, the Set*Detail calls below declare a primvar for each field,
    // but do not provide the data. The data itself has to be provided by
    // the plugin that extracts the actual data from the volume files.
    switch (type) {
        case FloatType:
            primvars->SetFloatDetail(fieldName, nullptr, detailType);
            break;
        case IntType:
            primvars->SetIntegerArrayDetail(fieldName, nullptr, 1, detailType);
            break;
        case Float2Type:
            primvars->SetFloatArrayDetail(fieldName, nullptr, 2, detailType);
            break;
        case Int2Type:
            primvars->SetIntegerArrayDetail(fieldName, nullptr, 2, detailType);
            break;
        case Float3Type:
            primvars->SetFloatArrayDetail(fieldName, nullptr, 3, detailType);
            break;
        case Int3Type:
            primvars->SetIntegerArrayDetail(fieldName, nullptr, 3, detailType);
            break;
        case ColorType:
            primvars->SetColorDetail(fieldName, nullptr, detailType);
            break;
        case PointType:
            primvars->SetPointDetail(fieldName, nullptr, detailType);
            break;
        case NormalType:
            primvars->SetNormalDetail(fieldName, nullptr, detailType);
            break;
        case VectorType:
            primvars->SetVectorDetail(fieldName, nullptr, detailType);
            break;
        case Float4Type:
            primvars->SetFloatArrayDetail(fieldName, nullptr, 4, detailType);
            break;
        case MatrixType:
            primvars->SetMatrixDetail(fieldName, nullptr, detailType);
            break;
        case StringType:
            primvars->SetStringDetail(fieldName, nullptr, detailType);
            break;
    }
}

RtPrimVarList
HdPrman_Volume::_ConvertGeometry(HdPrman_RenderParam *renderParam,
                                 HdSceneDelegate *sceneDelegate,
                                 const SdfPath &id,
                                 RtUString *primType,
                                 std::vector<HdGeomSubset> *geomSubsets)
{
    HdVolumeFieldDescriptorVector fields =
        sceneDelegate->GetVolumeFieldDescriptors(id);

    if (fields.empty()) {
        return RtPrimVarList();
    }

    TfToken fieldPrimType = _DetermineConsistentFieldPrimType(fields);
    if (fieldPrimType.IsEmpty()) {
        TF_WARN("The fields on volume %s have inconsistent types and can't be "
                "emitted as a single volume", id.GetText());
        return RtPrimVarList();
    }

    // Based on the field type we determine the function to emit the volume to
    // Prman
    _VolumeEmitterMap const& volumeEmitters = _GetVolumeEmitterMap();
    auto const iter = volumeEmitters.find(fieldPrimType);
    if (iter == volumeEmitters.end()) {
        TF_WARN("No volume emitter registered for field type '%s' "
                "on prim %s", fieldPrimType.GetText(), id.GetText());
        return RtPrimVarList();
    }

    int32_t const dims[] = { 0, 0, 0 };
    uint64_t const dim = dims[0] * dims[1] * dims[2];

    RtPrimVarList primvars(1, dim, dim, dim);
    primvars.SetIntegerArray(RixStr.k_Ri_dimensions, dims, 3);

    *primType = RixStr.k_Ri_Volume;

    // Setup the volume for Prman with the appropriate DSO and its parameters
    HdPrman_VolumeTypeEmitter emitterFunc = iter->second;
    emitterFunc(sceneDelegate, id, fields, &primvars);

    HdPrman_ConvertPrimvars(sceneDelegate, id, primvars, 1, dim, dim, dim);

    return primvars;
}

PXR_NAMESPACE_CLOSE_SCOPE
