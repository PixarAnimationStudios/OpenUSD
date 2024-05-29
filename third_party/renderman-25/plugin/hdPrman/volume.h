//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H

#include "pxr/pxr.h"
#include "hdPrman/gprim.h"
#include "pxr/imaging/hd/field.h"
#include "pxr/imaging/hd/volume.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_Field final : public HdField
{
public:
    HdPrman_Field(TfToken const& typeId, SdfPath const& id);
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam *renderParam,
              HdDirtyBits *dirtyBits) override;
    void Finalize(HdRenderParam *renderParam) override;
    HdDirtyBits GetInitialDirtyBitsMask() const override;
private:
    TfToken const _typeId;
};

class HdPrman_Volume final : public HdPrman_Gprim<HdVolume>
{
public:
    using BASE = HdPrman_Gprim<HdVolume>;
public:

    HF_MALLOC_TAG_NEW("new HdPrman_Volume");

    HdPrman_Volume(SdfPath const& id, const bool isMeshLight);

    HdDirtyBits GetInitialDirtyBitsMask() const override;

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
                                 RtPrimVarList* primvars);

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
    static void DeclareFieldPrimvar(RtPrimVarList* primvars,
                                    RtUString const& fieldName,
                                    FieldType type);

protected:
    RtPrimVarList
    _ConvertGeometry(HdPrman_RenderParam *renderParam,
                     HdSceneDelegate *sceneDelegate,
                     const SdfPath &id,
                     RtUString *primType,
                     std::vector<HdGeomSubset> *geomSubsets) override;

    riley::MaterialId
    _GetFallbackMaterial(HdPrman_RenderParam *renderParam) override {
        return renderParam->GetFallbackVolumeMaterialId();
    }

    bool _PrototypeOnly() override;

    using _VolumeEmitterMap = std::map<TfToken, HdPrman_VolumeTypeEmitter>;
    static _VolumeEmitterMap& _GetVolumeEmitterMap();

private:
    bool _isMeshLight;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H
