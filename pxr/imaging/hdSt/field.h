//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_FIELD_H
#define PXR_IMAGING_HD_ST_FIELD_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/field.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// Represents a Field Buffer Prim.
///
class HdStField : public HdField {
public:
    /// For now, only fieldType HdStTokens->openvdbAsset is supported.
    HDST_API
    HdStField(SdfPath const & id, TfToken const & fieldType);
    HDST_API
    ~HdStField() override;

    /// Loads field as 3d texture to generate GetFieldResource.
    HDST_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Initialized by Sync.
    HDST_API
    HdStTextureIdentifier const &GetTextureIdentifier() const {
        return _textureId;
    }

    /// Get memory request for this field
    size_t GetTextureMemory() const { return _textureMemory; }

    /// Bprim types handled by this class
    HDST_API
    static const TfTokenVector &GetSupportedBprimTypes();

    /// Can bprim type be handled by this class
    HDST_API
    static bool IsSupportedBprimType(const TfToken &bprimType);

private:
    const TfToken _fieldType;

    HdStTextureIdentifier _textureId;
    size_t _textureMemory;

    bool _isInitialized : 1;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_FIELD_H
