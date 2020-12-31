//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_FIELD_H
#define PXR_IMAGING_LOFI_FIELD_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/field.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/plugin/LoFi/textureIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// Represents a Field Buffer Prim.
///
class LoFiField : public HdField {
public:
    /// For now, only fieldType LoFiTokens->openvdbAsset is supported.
    LOFI_API
    LoFiField(SdfPath const & id, TfToken const & fieldType);
    LOFI_API
    ~LoFiField() override;

    /// Loads field as 3d texture to generate GetFieldResource.
    LOFI_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    LOFI_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Initialized by Sync.
    LOFI_API
    LoFiTextureIdentifier const &GetTextureIdentifier() const {
        return _textureId;
    }

    /// Get memory request for this field
    size_t GetTextureMemory() const { return _textureMemory; }

    /// Bprim types handled by this class
    LOFI_API
    static const TfTokenVector &GetSupportedBprimTypes();

    /// Can bprim type be handled by this class
    LOFI_API
    static bool IsSupportedBprimType(const TfToken &bprimType);

private:
    const TfToken _fieldType;

    LoFiTextureIdentifier _textureId;
    size_t _textureMemory;

    bool _isInitialized : 1;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_FIELD_H
