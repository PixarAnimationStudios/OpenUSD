//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hdSt/field.h"
#include "pxr/imaging/hdSt/fieldSubtextureIdentifier.h"

#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (fieldName)
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
            GetId(), _tokens->fieldName);
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
            // Force volume prim to pick up the new field resource and
            // recompute bounding box.
            //
            // XXX:-matthias
            // Ideally, this would be more fine-grained than blasting all
            // rprims.
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
