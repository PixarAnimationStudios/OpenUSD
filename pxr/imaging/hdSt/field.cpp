//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/field.h"
#include "pxr/imaging/hdSt/fieldSubtextureIdentifier.h"

#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (fieldIndex)
    (fieldPurpose)
    (textureMemory)

    (openvdbAsset)
    (field3dAsset)
);

HdStField::HdStField(SdfPath const& id, TfToken const & fieldType) 
  : HdField(id)
  , _fieldType(fieldType)
  , _textureMemory(0)
  , _isInitialized(false)
{
}

HdStField::~HdStField() = default;

void
HdStField::Sync(HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits)
{
    if (*dirtyBits & DirtyParams) {

        // Get asset path from scene delegate.
        //
        const VtValue filePathValue = sceneDelegate->Get(
            GetId(), HdFieldTokens->filePath);
        const SdfAssetPath filePath = filePathValue.Get<SdfAssetPath>();

        // Resolve asset path
        //
        // Assuming that correct resolve context is set when HdStField::Sync is
        // called.
        const TfToken resolvedFilePath = TfToken(filePath.GetResolvedPath());

        const VtValue fieldNameValue = sceneDelegate->Get(
            GetId(), HdFieldTokens->fieldName);
        const TfToken &fieldName = fieldNameValue.Get<TfToken>();

        const VtValue fieldIndexValue = sceneDelegate->Get(
            GetId(), _tokens->fieldIndex);

        const int fieldIndex = fieldIndexValue.Get<int>();

        if (_fieldType == _tokens->openvdbAsset) {
            _textureId = HdStTextureIdentifier(
                resolvedFilePath,
                std::make_unique<HdStOpenVDBAssetSubtextureIdentifier>(
                    fieldName, fieldIndex));
        } else {
            const VtValue fieldPurposeValue = sceneDelegate->Get(
                GetId(), _tokens->fieldPurpose);
            const TfToken &fieldPurpose = fieldPurposeValue.Get<TfToken>();

            _textureId = HdStTextureIdentifier(
                resolvedFilePath,
                std::make_unique<HdStField3DAssetSubtextureIdentifier>(
                    fieldName, fieldIndex, fieldPurpose));
        }

        const VtValue textureMemoryValue = sceneDelegate->Get(
            GetId(), _tokens->textureMemory);
        _textureMemory =
            1048576 * textureMemoryValue.GetWithDefault<float>(0.0f);
        
        if (_isInitialized) {
            // This code is no longer needed when using scene indices
            // or scene index emulation since this dependency is now tracked
            // by the HdSt_DependencySceneIndexPlugin.
            //
            // Force volume prim to pick up the new field resource and
            // recompute bounding box.
            //
            HdChangeTracker& changeTracker =
                sceneDelegate->GetRenderIndex().GetChangeTracker();
            changeTracker.MarkAllRprimsDirty(HdChangeTracker::DirtyVolumeField);
        }
    }

    _isInitialized = true;

    *dirtyBits = Clean;
}

HdDirtyBits 
HdStField::GetInitialDirtyBitsMask() const 
{
    return DirtyBits::AllDirty;
}

const TfTokenVector &
HdStField::GetSupportedBprimTypes()
{
    static const TfTokenVector result = {
        _tokens->openvdbAsset,
        _tokens->field3dAsset
    };
    return result;
}

bool
HdStField::IsSupportedBprimType(const TfToken &bprimType)
{
    return 
        bprimType == _tokens->openvdbAsset ||
        bprimType == _tokens->field3dAsset;
}

PXR_NAMESPACE_CLOSE_SCOPE
