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
#ifndef EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H
#define EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H

#include "pxr/pxr.h"
#include "hdPrman/gprim.h"
#include "pxr/imaging/hd/field.h"
#include "pxr/imaging/hd/volume.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/base/gf/matrix4f.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_Field final : public HdField {
public:
    HdPrman_Field(TfToken const& typeId, SdfPath const& id);
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam *renderParam,
                      HdDirtyBits *dirtyBits) override;
    virtual void Finalize(HdRenderParam *renderParam) override;
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;
private:
    TfToken const _typeId;
};

class HdPrman_Volume final : public HdPrman_Gprim<HdVolume> {
public:
    typedef HdPrman_Gprim<HdVolume> BASE;
public:
    HF_MALLOC_TAG_NEW("new HdPrman_Volume");
    HdPrman_Volume(SdfPath const& id);
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// The types of volumes that can be emitted to Prman are extensible, since
    /// volumes are emitted via blobbydsos, which themselves are plugins to
    /// Prman. So we allow the registration of handlers for volumes of different
    /// types. The volumes are identified by the prim type of their fields.
    /// Currently Hydra knows of two such types:
    ///
    ///    UsdVolImagingTokens->openvdbAsset
    ///    UsdVolImagingTokens->field3dAsset
    ///
    /// Note, since a Volume prim can have multiple fields associated with it,
    /// we require that all associated fields are of the same type. The code
    /// rejects a volume if that is not the case and issues a warning.
    ///
    /// The emitter functions that can be registered are responsible to fill in
    /// the RtParamList list with the k_Ri_type (name of the blobbydso) and any
    /// parameters to this plugin (k_blobbydso_stringargs). The function is also
    /// responsible for declaring the primvar for each field.
    using HdPrman_VolumeTypeEmitter =
                        void (*)(HdSceneDelegate *sceneDelegate,
                                 SdfPath const& id,
                                 HdVolumeFieldDescriptorVector const& fields,
                                 RtParamList* primvars);

    /// Registers a new volume emitter. Returns true if the handler was
    /// registered as the new handler. When \p overrideExisting is false, then
    /// a new handler for a previously registered emitter will not be accepted.
    static bool AddVolumeTypeEmitter(TfToken const& fieldPrimType,
                                     HdPrman_VolumeTypeEmitter emitterFunc,
                                     bool overrideExisting = false);

    /// Specialized subset of primvar types for volume fields
    enum FieldType {
        FloatType = 0,
        IntType,
        Float2Type,
        Int2Type,
        Float3Type,
        Int3Type,
        ColorType,
        PointType,
        NormalType,
        VectorType,
        Float4Type,
        MatrixType,
        StringType
    };

    /// Helper method for emitter functions to declare a primvar for a field
    static void DeclareFieldPrimvar(RtParamList* primvars,
                                    RtUString const& fieldName,
                                    FieldType type);

protected:
    virtual RtParamList
    _ConvertGeometry(HdPrman_Context *context,
                      HdSceneDelegate *sceneDelegate,
                      const SdfPath &id,
                      RtUString *primType,
                      std::vector<HdGeomSubset> *geomSubsets) override;

    virtual riley::MaterialId
    _GetFallbackMaterial(HdPrman_Context *context) override {
        return context->fallbackVolumeMaterial;
    }

    using _VolumeEmitterMap = std::map<TfToken, HdPrman_VolumeTypeEmitter>;
    static _VolumeEmitterMap& _GetVolumeEmitterMap();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_23_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H
