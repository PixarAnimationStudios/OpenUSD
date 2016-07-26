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

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/resource.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

HdTexture::HdTexture(HdSceneDelegate* delegate, SdfPath const& id)
    : _delegate(delegate)
    , _id(id)
{
}

HdTexture::~HdTexture()
{
}

void
HdTexture::Sync()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();
 
    SdfPath const& id = GetID();
    HdSceneDelegate* delegate = GetDelegate();
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();
    HdChangeTracker& changeTracker = 
                                delegate->GetRenderIndex().GetChangeTracker();    
    HdChangeTracker::DirtyBits bits = changeTracker.GetTextureDirtyBits(id);

    HdTextureResource::ID texID = delegate->GetTextureResourceID(id);
    {
        HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr> texInstance;

        std::unique_lock<std::mutex> regLock =
            resourceRegistry->RegisterTextureResource(texID, &texInstance);

        // XXX : DirtyParams and DirtyTexture are currently the same but they
        //       can be separated functionally and have different 
        //       delegate methods.
        if (texInstance.IsFirstInstance() or 
            bits & HdChangeTracker::DirtyParams or
            bits & HdChangeTracker::DirtyTexture) {

            HdTextureResourceSharedPtr texResource =
                delegate->GetTextureResource(id);
#if 0
            if (texResource->GetTexelsTextureId() == 0) {
                // fail to load a texture. use fallback default

                if (texResource->IsPtex()) {
                    const size_t defaultPtexId = 0x48510398a84ebf94;
                    HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr>
                        defaultTexInstance = resourceRegistry->RegisterTextureResource(defaultPtexId);
                    if (defaultTexInstance.IsFirstInstance()) {
                        HdTextureResourceSharedPtr defaultTexResource(new Hd_DefaultPtexTextureResource());
                        defaultTexInstance.SetValue(defaultTexResource);
                    }
                    texResource = defaultTexInstance.GetValue();
                } else {
                    const size_t defaultUVId = 0x48510398a84ebf95;
                    HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr>
                        defaultTexInstance = resourceRegistry->RegisterTextureResource(defaultUVId);
                    if (defaultTexInstance.IsFirstInstance()) {
                        HdTextureResourceSharedPtr defaultTexResource(new Hd_DefaultUVTextureResource());
                        defaultTexInstance.SetValue(defaultTexResource);
                    }
                    texResource = defaultTexInstance.GetValue();
                }
            }
#endif
            texInstance.SetValue(texResource);
        }
    }
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