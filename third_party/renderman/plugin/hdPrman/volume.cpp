//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/volume.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/instancer.h"
#include "hdPrman/material.h"
#include "hdPrman/rixStrings.h"
#include "hdPrman/volumeFilter.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdVol/tokens.h"
#include "pxr/usdImaging/usdRiPxrImaging/tokens.h"
#include "pxr/usdImaging/usdRiPxrImaging/version.h"
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

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((riAttrPrefix, "ri:attributes:volume:"))
    ((riPrefix, "ri:volume:"))
    (density)
    (densityGrid)
    (densityMult)
    (densityRolloff)
    (filterWidth)
    (velocityGrid)
    (velocityScale)
    (volumeFilter)
);

#if HD_API_VERSION >= 93
// XXX -- This function is a stripped-down version of the one in
//        hdPrman/light.cpp.
static bool
_PopulateNodesFromMaterialResource(HdSceneDelegate *sceneDelegate,
                                   const SdfPath &id,
                                   const TfToken &terminalName,
                                   std::vector<riley::ShadingNode> *result)
{
    VtValue hdMatVal = sceneDelegate->GetMaterialResource(id);
    if (!hdMatVal.IsHolding<HdMaterialNetworkMap>()) {
        TF_WARN("Could not get HdMaterialNetworkMap for '%s'", id.GetText());
        return false;
    }

    // Convert HdMaterial to HdMaterialNetwork2 form.
    const HdMaterialNetwork2 matNetwork2 = HdConvertToHdMaterialNetwork2(
            hdMatVal.UncheckedGet<HdMaterialNetworkMap>());

    SdfPath nodePath;
    for (auto const& terminal: matNetwork2.terminals) {
        if (terminal.first == terminalName) {
            nodePath = terminal.second.upstreamNode;
            break;
        }
    }

    if (nodePath.IsEmpty()) {
        TF_WARN("Could not find terminal '%s' in HdMaterialNetworkMap for '%s'",
                terminalName.GetText(), id.GetText());
        return false;
    }

    result->reserve(matNetwork2.nodes.size());
    if (!HdPrman_ConvertHdMaterialNetwork2ToRmanNodes(
            id, matNetwork2, nodePath, result)) {
        TF_WARN("Failed to convert HdMaterialNetwork to Renderman shading "
                "nodes for '%s'", id.GetText());
        return false;
    }

    return true;
}

// XXX -- This is similar to _PopulateLightFilterNodes() in hdPrman/light.cpp.
static void
_PopulateVolumeFilterNodes(
    const SdfPathVector &volumeFilterPaths,
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    riley::Riley *riley,
    std::vector<riley::ShadingNode> *volumeFilterNodes,
    std::vector<riley::CoordinateSystemId> *coordsysIds,
    std::vector<riley::VolumeFilterId> *volumeFilterIds)
{
    if (volumeFilterPaths.empty()) {
        return;
    }

    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);

    for (const auto& filterPath : volumeFilterPaths) {
        std::vector<riley::ShadingNode> filterNetworkNodes;
        if (!_PopulateNodesFromMaterialResource(
                sceneDelegate, filterPath,
                HdMaterialTerminalTokens->volumeFilter,
                &filterNetworkNodes)) {
            continue;
        }

        riley::ShadingNode *filter = &filterNetworkNodes.back();
        const RtUString filterPathAsString(filterPath.GetText());

        // To ensure that multiple volume filters within a volume get
        // unique names, use the full filter path for the handle.
        filter->handle = filterPathAsString;

        static const RtUString us_coordSys("coordSys");
        filter->params.SetString(us_coordSys, filterPathAsString);

        // Look up volume filter ID
        if (HdSprim* sprim = sceneDelegate->GetRenderIndex().GetSprim(
#if USD_RI_PXR_IMAGING_API_VERSION >= 3
                UsdRiPxrImagingPrimTypeTokens->volumeFilter, filterPath)) {
#else
                _tokens->volumeFilter, filterPath)) {
#endif
            if (auto* volumeFilter = dynamic_cast<HdPrman_VolumeFilter*>(sprim)) {
                volumeFilter->SyncToRiley(sceneDelegate, param, riley);
                coordsysIds->push_back(volumeFilter->GetCoordSysId());
            }
        } else {
            TF_WARN("Did not find expected volume filter <%s>",
                    filterPath.GetText());
        }

        filter->type = riley::ShadingNode::Type::k_VolumeFilter;

        const riley::ShadingNetwork filterNetwork {
            static_cast<uint32_t>(filterNetworkNodes.size()),
            filterNetworkNodes.data()
        };

        const riley::VolumeFilterId filterId = riley->CreateVolumeFilter(
            riley::UserId(stats::AddDataLocation(filterPath.GetText()).GetValue()),
            filterNetwork,
            RtParamList());
        volumeFilterIds->push_back(filterId);
        volumeFilterNodes->push_back(*filter);
    }
}
#endif

HdPrman_Field::HdPrman_Field(TfToken const& typeId, SdfPath const& id)
    : HdField(id), _typeId(typeId)
{
}

void HdPrman_Field::Sync(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdDirtyBits *dirtyBits)
{
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

void
HdPrman_Volume::Finalize(HdRenderParam *renderParam)
{
    if (!_volumeFilterIds.empty()) {
        auto* param = static_cast<HdPrman_RenderParam*>(renderParam);
        riley::Riley* riley = param->AcquireRiley();
        for (const auto& filterId : _volumeFilterIds) {
            riley->DeleteVolumeFilter(filterId);
        }
        _volumeFilterIds.clear();
    }
    _volumeFilterPaths.clear();
    _volumeFilterNodeNames.clear();

    BASE::Finalize(renderParam);
}

void
HdPrman_Volume::Sync(HdSceneDelegate *sceneDelegate,
                     HdRenderParam   *renderParam,
                     HdDirtyBits     *dirtyBits,
                     TfToken const   &reprToken)
{
#if HD_API_VERSION >= 93
    const SdfPath& id = GetId();

    auto* param = static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley* riley = param->AcquireRiley();
    HdRenderIndex& renderIndex = sceneDelegate->GetRenderIndex();
    HdChangeTracker& changeTracker = renderIndex.GetChangeTracker();

    bool dirtyFilters = false;
    SdfPathVector volumeFilterPaths;
    VtValue val = sceneDelegate->GetVolumeParamValue(id, HdTokens->filters);
    if (val.IsHolding<SdfPathVector>()) {
        volumeFilterPaths = val.UncheckedGet<SdfPathVector>();
        // XXX -- How do we know if the actual filters changed?
        //        We will just assume they did for now.
        dirtyFilters = true;
    } else if (!_volumeFilterPaths.empty()) {
        // Volume filter paths were not empty before, but are now.
        dirtyFilters = true;
    }

    if (dirtyFilters) {
        // Clear and recreate dependencies.
        if (volumeFilterPaths != _volumeFilterPaths) {
            for (const SdfPath& filterPath : _volumeFilterPaths) {
                changeTracker.RemoveSprimRprimDependency(filterPath, id);
            }
            for (const SdfPath& filterPath : volumeFilterPaths) {
                changeTracker.AddSprimRprimDependency(filterPath, id);
            }
            _volumeFilterPaths = volumeFilterPaths;
        }

        // Delete old Riley volume filters.
        for (const auto& filterId : _volumeFilterIds) {
            riley->DeleteVolumeFilter(filterId);
        }
        _volumeFilterIds.clear();
        _volumeFilterCoordSysIds.clear();

        // _PopulateVolumeFilterNodes() also gives us the coordinate systems.
        std::vector<riley::ShadingNode> filterNodes;
        _PopulateVolumeFilterNodes(volumeFilterPaths, sceneDelegate,
            renderParam, riley, &filterNodes, &_volumeFilterCoordSysIds,
            &_volumeFilterIds);

        _volumeFilterNodeNames.clear();
        for (const auto& filterNode: filterNodes) {
            _volumeFilterNodeNames.push_back(filterNode.handle);
        }
    }
#endif

    BASE::Sync(sceneDelegate, renderParam, dirtyBits, reprToken);
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

TfToken
_GetFieldName(HdSceneDelegate *sceneDelegate, const HdVolumeFieldDescriptor& field)
{
    return sceneDelegate->Get(field.fieldId, HdFieldTokens->fieldName)
        .GetWithDefault<TfToken>(field.fieldName);
}

HdPrman_Volume::FieldType
_DetermineOpenVDBFieldType(HdSceneDelegate *sceneDelegate,
                           HdVolumeFieldDescriptor const& field)
{
    SdfPath const& fieldId = field.fieldId;
    const TfToken fieldName = _GetFieldName(sceneDelegate, field);

    VtValue fieldDataTypeValue =
        sceneDelegate->Get(fieldId, UsdVolTokens->fieldDataType);
    if (!fieldDataTypeValue.IsHolding<TfToken>() ||
        fieldDataTypeValue.UncheckedGet<TfToken>().IsEmpty()) {
        TF_WARN("Missing fieldDataType attribute on volume field prim %s. "
                "Assuming float.", fieldId.GetText());
        // Cd is specific to Solaris
        if (fieldName.GetString() == "Cd" ||
            fieldName.GetString().find("color")
            != std::string::npos) {
            return HdPrman_Volume::FieldType::ColorType;
        } else if(fieldName.GetString() == "vel" ||
                  fieldName.GetString() == "velocity") {
            return HdPrman_Volume::FieldType::VectorType;
        }

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
#if PXR_VERSION <= 2302
        if (roleHint == UsdVolTokens->none_) {
            vectorDataRoleHint = UsdVolTokens->vector;
#else
        if (roleHint == UsdVolTokens->None_) {
            vectorDataRoleHint = UsdVolTokens->Vector;
#endif
        }

#if PXR_VERSION <= 2302
        if (vectorDataRoleHint == UsdVolTokens->color) {
#else
        if (vectorDataRoleHint == UsdVolTokens->Color) {
#endif
            return HdPrman_Volume::FieldType::ColorType;
#if PXR_VERSION <= 2302
        } else if (vectorDataRoleHint == UsdVolTokens->point) {
#else
        } else if (vectorDataRoleHint == UsdVolTokens->Point) {
#endif
            return HdPrman_Volume::FieldType::PointType;
#if PXR_VERSION <= 2302
        } else if (vectorDataRoleHint == UsdVolTokens->normal) {
#else
        } else if (vectorDataRoleHint == UsdVolTokens->Normal) {
#endif
            return HdPrman_Volume::FieldType::NormalType;
#if PXR_VERSION <= 2302
        } else if (vectorDataRoleHint == UsdVolTokens->vector) {
#else
        } else if (vectorDataRoleHint == UsdVolTokens->Vector) {
#endif
            return HdPrman_Volume::FieldType::VectorType;
#if PXR_VERSION <= 2302
        } else if (vectorDataRoleHint == UsdVolTokens->none_) {
#else
        } else if (vectorDataRoleHint == UsdVolTokens->None_) {
#endif
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

template <typename T>
bool _GetPrimvarValue(HdSceneDelegate *sceneDelegate,
                      const SdfPath& id,
                      const TfToken& name,
                      T* value,
                      bool reportMissing = false)
{
    float time;
    VtValue val;
    // For now, support both the "ri:attributes:volume:" and
    // "ri:volume:" namespaces.
    bool foundPrimvar = sceneDelegate->SamplePrimvar(
        id,
        TfToken(_tokens->riAttrPrefix.GetString() + name.GetString()),
        /*maxNumSamples*/ 1, &time, &val);
    if(!foundPrimvar) {
        foundPrimvar = sceneDelegate->SamplePrimvar(
            id,
            TfToken(_tokens->riPrefix.GetString() + name.GetString()),
            /*maxNumSamples*/ 1, &time, &val);
    }
    if(!foundPrimvar) {
        sceneDelegate->SamplePrimvar(id, name,
                                     /*maxNumSamples*/ 1, &time, &val);
    }
    if (foundPrimvar) {
        // Allow implicit casting for registered cases such as double to float.
        if (!val.IsHolding<T>() && val.CanCast<T>()) {
            val = val.Cast<T>();
        }
        if (val.IsHolding<T>()) {
            *value = val.UncheckedGet<T>();
            return true;
        } else {
            TF_WARN("OpenVDB Volume: %s primvar attribute for volume %s has "
                    "type %s, expected type %s.",
                    name.GetText(), id.GetText(), val.GetTypeid().name(),
                    typeid(T).name());
        }
    } else if (reportMissing) {
        TF_WARN("OpenVDB Volume: missing %s primvar attribute for volume %s.",
                name.GetText(), id.GetText());
    }

    return false;
}

void
_EmitOpenVDBVolumeField(HdSceneDelegate *sceneDelegate,
                        const SdfPath& id,
                        const HdVolumeFieldDescriptor& field,
                        RtPrimVarList* primvars,
                        std::unordered_map<
                            TfToken,
                            std::vector<int>,
                            TfToken::HashFunctor
                        >& fieldIndices,
                        TfToken& densityField,
                        TfToken& velocityField)
{
    HdPrman_Volume::FieldType fieldType = _DetermineOpenVDBFieldType(
        sceneDelegate, field
    );

    const int fieldIndex
        = sceneDelegate->Get(field.fieldId, UsdVolTokens->fieldIndex)
                .GetWithDefault<int>(0);
    const TfToken fieldName = _GetFieldName(sceneDelegate, field);

    // Set gridGroups to be the USD fieldID
    fieldIndices[fieldName].push_back(fieldIndex);

    if (fieldName == densityField) {
        densityField = TfToken(densityField.GetString() + ":fogvolume");
    }

    if (fieldName == velocityField) {
        // Force type to be vector
        fieldType = HdPrman_Volume::VectorType;
        velocityField = TfToken(velocityField.GetString() + ":fogvolume");
    }

    HdPrman_Volume::DeclareFieldPrimvar(
        primvars, RtUString(fieldName.GetText()), fieldType
    );
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

    // There is an implicit assumption that all the fields on this volume are
    // extracted from the same .vdb file, which is determined once from the
    // first field.
    const VtValue filePath = sceneDelegate->Get(fields[0].fieldId,
                                                HdFieldTokens->filePath);
    const SdfAssetPath &fileAssetPath = filePath.Get<SdfAssetPath>();

    std::string volumeAssetPath = fileAssetPath.GetResolvedPath();
    if (volumeAssetPath.empty()) {
        volumeAssetPath = fileAssetPath.GetAssetPath();
    }

    // This will be the first of the string args supplied to the blobbydso.
    std::string vdbSource;

#ifndef PXR_OPENVDB_SUPPORT_ENABLED
    vdbSource = volumeAssetPath;
#else // PXR_OPENVDB_SUPPORT_ENABLED
    // If volumeAssetPath is an actual file path, copy it into the vdbSource
    // string, prepended with a "file:" tag.
    if (TfIsFile(volumeAssetPath)) {
        vdbSource = "file:" + volumeAssetPath;
    } else {
        // volumeAssetPath is not a file path. Attempt to resolve
        // it as an ArAsset and retrieve vdb grids from that asset.
        openvdb::GridPtrVecPtr gridVecPtr = HioOpenVDBGridsFromAsset(volumeAssetPath);

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
#if PXR_VERSION >= 2602
        // XXX:
        // Disable deprecation warnings for RixStorage to enable
        // strict builds on Windows.
ARCH_PRAGMA_PUSH
ARCH_PRAGMA_DEPRECATED
#endif // PXR_VERSION >= 2602
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
    }
#if PXR_VERSION >= 2602
ARCH_PRAGMA_POP
#endif // PXR_VERSION >= 2602
#endif // PXR_OPENVDB_SUPPORT_ENABLED

    primvars->SetString(RixStr.k_Ri_type, blobbydsoImplOpenVDB);

    // Look for a density field, otherwise default to the first field
    TfToken densityField = _tokens->density;
    _GetPrimvarValue<TfToken>(sceneDelegate, id, _tokens->densityGrid, &densityField);
    if (std::find_if(fields.begin(), fields.end(),
        [&](const auto& field) {
            return _GetFieldName(sceneDelegate, field) == densityField;
        }) == fields.end()
    )
        densityField = _GetFieldName(sceneDelegate, fields[0]);

    // Look for a velocity field, otherwise default to empty string
    TfToken velocityField;
    _GetPrimvarValue<TfToken>(sceneDelegate, id, _tokens->velocityGrid, &velocityField);

    // The individual fields of this volume need to be declared as primvars
    std::unordered_map<TfToken, std::vector<int>, TfToken::HashFunctor> fieldIndices;
    for (HdVolumeFieldDescriptor const& field : fields)
        _EmitOpenVDBVolumeField(
            sceneDelegate, id, field, primvars, fieldIndices, densityField, velocityField
        );

    // Set JSON Arguments
    JsObject jsonData;

    JsArray gridGroups;
    for (const auto& gridGroup : fieldIndices) {
        JsArray indices;
        for (const auto& index : gridGroup.second)
            indices.push_back(JsValue(index));
        gridGroups.emplace_back(JsObject {
            { "name", JsValue(gridGroup.first) },
            { "indices", indices },
        });
    }
    jsonData["gridGroups"] = std::move(gridGroups);

    float densityMult = 1.f; // default for densityMult
    if (_GetPrimvarValue<float>(sceneDelegate, id, _tokens->densityMult, &densityMult))
        jsonData[_tokens->densityMult.GetText()] = JsValue(densityMult);

    float densityRolloff = 0.f; // default for densityRolloff
    if (_GetPrimvarValue<float>(sceneDelegate, id, _tokens->densityRolloff, &densityRolloff))
        jsonData[_tokens->densityRolloff.GetText()] = JsValue(densityRolloff);

    float filterWidth = 0.f; // default for filterWidth
    if (_GetPrimvarValue<float>(sceneDelegate, id, _tokens->filterWidth, &filterWidth))
        jsonData[_tokens->filterWidth.GetText()] = JsValue(filterWidth);

    float velocityScale = 1.f; // default for velocityScale
    if (_GetPrimvarValue<float>(sceneDelegate, id, _tokens->velocityScale, &velocityScale))
        jsonData[_tokens->velocityScale.GetText()] = JsValue(velocityScale);

    const std::string jsonOpts = JsWriteToString(JsValue(std::move(jsonData)));

    // Set DSO Arguments
    const unsigned nargs = 4;
    RtUString sa[nargs] = {
        RtUString(vdbSource.c_str()),
        RtUString(densityField.GetText()),
        RtUString(velocityField.GetText()),
        RtUString(jsonOpts.c_str())
    };
    primvars->SetStringArray(RixStr.k_blobbydso_stringargs, sa, nargs);
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

bool
HdPrman_Volume::_ConvertGeometry(
    HdPrman_RenderParam *renderParam,
    HdSceneDelegate *sceneDelegate,
    const SdfPath &id,
    RtUString *primType,
    RtPrimVarList *primvars,
    std::vector<HdGeomSubset> *geomSubsets,
    std::vector<RtPrimVarList> *geomSubsetPrimvars)
{
    *primType = RixStr.k_Ri_Volume;

    // Dimensions
    int32_t const dims[] = { 0, 0, 0 };
    uint64_t const dim = dims[0] * dims[1] * dims[2];

    *primvars = RtPrimVarList(1, dim, dim, dim);
    primvars->SetIntegerArray(RixStr.k_Ri_dimensions, dims, 3);

    HdPrman_ConvertPrimvars(
        sceneDelegate, id, *primvars,
        /* numUniform = */ 1,
        /* numVertex = */ 0,
        /* numVarying = */ 0,
        /* numFaceVarying = */ 0,
        renderParam->GetShutterInterval());

    // Setup the volume for Prman with the appropriate DSO and its parameters
    HdVolumeFieldDescriptorVector fields =
        sceneDelegate->GetVolumeFieldDescriptors(id);
    if (!fields.empty()) {
        TfToken fieldPrimType = _DetermineConsistentFieldPrimType(fields);
        if (fieldPrimType.IsEmpty()) {
            TF_WARN("The fields on volume %s have inconsistent types and "
                    "cannot be emitted as a single volume", id.GetText());
            return false;
        }

        // Based on the field type we determine the function to emit the
        // volume to Prman
        _VolumeEmitterMap const& volumeEmitters = _GetVolumeEmitterMap();
        auto const iter = volumeEmitters.find(fieldPrimType);
        if (iter == volumeEmitters.end()) {
            TF_WARN("No volume emitter registered for field type '%s' "
                    "on prim %s", fieldPrimType.GetText(), id.GetText());
            return false;
        }

        HdPrman_VolumeTypeEmitter emitterFunc = iter->second;
        emitterFunc(sceneDelegate, id, fields, primvars);
    } else {
        // If no fields are found, the volume will be required to
        // specify Ri:type (ex: "box") and Ri:Bounds.  We do not
        // check this here because RenderMan will already issue
        // an appropriate warning.
    }
    return true;
}

void
HdPrman_Volume::_AddPrimvars(RtPrimVarList* primvars) const
{
    if (!primvars) {
        return;
    }
    // Add volume filters to the primvars.
    static const RtUString us_volumeFilters("volume:filters");
    primvars->SetStringArray(
        us_volumeFilters,
        _volumeFilterNodeNames.data(), _volumeFilterNodeNames.size());
}

const std::vector<riley::CoordinateSystemId>& 
HdPrman_Volume::_GetAdditionalCoordSysIds() const
{
    return _volumeFilterCoordSysIds;
}

PXR_NAMESPACE_CLOSE_SCOPE
