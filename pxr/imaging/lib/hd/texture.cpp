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
#include "pxr/imaging/hd/texture.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


HdTexture::HdTexture(SdfPath const& id)
  : HdBprim(id)
  , _textureResource()
{
}

HdTexture::~HdTexture()
{
}

void
HdTexture::Sync(HdSceneDelegate *sceneDelegate,
                HdRenderParam   *renderParam,
                HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    SdfPath const& id = GetId();
    HdDirtyBits bits = *dirtyBits;

    // XXX : DirtyParams and DirtyTexture are currently the same but they
    //       can be separated functionally and have different 
    //       delegate methods.
    if ((bits & (DirtyParams | DirtyTexture)) != 0) {
       HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

        HdResourceRegistrySharedPtr const &resourceRegistry = 
                                              renderIndex.GetResourceRegistry();

        HdTextureResource::ID texID = sceneDelegate->GetTextureResourceID(id);

        // Has the texture really changed.
        // The safest thing to do is assume it has, so that's the default used
        bool isNewTexture = true;

        if (texID != HdTextureResource::ID(-1)) {
            HdInstance<HdResourceRegistry::TextureKey,
                       HdTextureResourceSharedPtr> texInstance;

            // Use render index to convert local texture id into global
            // texture key
            HdResourceRegistry::TextureKey texKey =
                                               renderIndex.GetTextureKey(texID);

            std::unique_lock<std::mutex> regLock =
                resourceRegistry->RegisterTextureResource(texKey,
                                                          &texInstance);

            if (texInstance.IsFirstInstance()) {
                _textureResource = _GetTextureResource(sceneDelegate,
                                                       id, texID);
                texInstance.SetValue(_textureResource);
            } else {
                // Take a reference to the texture to ensure it lives as long
                // as this class.
                HdTextureResourceSharedPtr textureResource
                                                       = texInstance.GetValue();

                if (_textureResource == textureResource) {
                    isNewTexture = false;
                } else {
                    _textureResource = textureResource;
                }
            }
        } else {
            _textureResource.reset();
        }

        // The texture resource may have been cleared, so we need to release the
        // old one.
        //
        // This is particularly important if the update is on the memory
        // request.
        // As the cache may be still holding on to the resource with a larger
        // memory request.
        if (isNewTexture) {
            renderIndex.GetChangeTracker().SetBprimGarbageCollectionNeeded();
        }
    }

    *dirtyBits = Clean;
}

HdDirtyBits
HdTexture::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

bool
HdTexture::IsPtex() const
{
    return false;
}

bool
HdTexture::ShouldGenerateMipMaps() const
{
    return true;
}

HdTextureResourceSharedPtr
HdTexture::_GetTextureResource( HdSceneDelegate *sceneDelegate,
                                const SdfPath &sceneId,
                                HdTextureResource::ID texID) const
{
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

