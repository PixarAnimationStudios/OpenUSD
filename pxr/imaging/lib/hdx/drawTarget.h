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
#ifndef HDX_DRAW_TARGET_H
#define HDX_DRAW_TARGET_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/drawTargetRenderPassState.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/textureResource.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"

#include <boost/shared_ptr.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


#define HDX_DRAW_TARGET_TOKENS                  \
    (attachments)                               \
    (camera)                                    \
    (collection)                                \
    (depthClearValue)                           \
    (drawTargetSet)                             \
    (enable)                                    \
    (resolution)

TF_DECLARE_PUBLIC_TOKENS(HdxDrawTargetTokens, HDX_API, HDX_DRAW_TARGET_TOKENS);

class HdSceneDelegate;
class HdRenderIndex;
class HdxCamera;
class HdxDrawTargetAttachmentDescArray;


typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

typedef std::vector<class HdxDrawTarget const *> HdxDrawTargetPtrConstVector;

/// \class HdxDrawTarget
///
/// Represents an render to texture render pass.
///
/// \note This is a temporary API to aid transition to hydra, and is subject
/// to major changes.
///
class HdxDrawTarget : public HdSprim {
public:
    HDX_API
    HdxDrawTarget(SdfPath const & id);
    HDX_API
    virtual ~HdxDrawTarget();

    /// Dirty bits for the HdxDrawTarget object
    enum DirtyBits {
        Clean                   = 0,
        DirtyDTEnable           = 1 <<  0,
        DirtyDTCamera           = 1 <<  1,
        DirtyDTResolution       = 1 <<  2,
        DirtyDTAttachment       = 1 <<  3,
        DirtyDTDepthClearValue  = 1 <<  4,
        DirtyDTCollection       = 1 <<  5,
        AllDirty                = (DirtyDTEnable
                                   |DirtyDTCamera
                                   |DirtyDTResolution
                                   |DirtyDTAttachment
                                   |DirtyDTDepthClearValue
                                   |DirtyDTCollection)
    };

    /// Returns the version of the under-lying GlfDrawTarget.
    /// The version changes if the draw target attachments texture ids
    /// are changed in anyway (for example switching to a new
    /// GlfDrawTarget object or resize the resources).
    /// The version does not increment if only the contents of the
    /// texture resources change
    unsigned int GetVersion() const { return _version; }

    /// Synchronizes state from the delegate to this object.
    HDX_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;

    /// Accessor for tasks to get the parameters cached in this object.
    HDX_API
    virtual VtValue Get(TfToken const &token) const override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HDX_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;


    // ---------------------------------------------------------------------- //
    /// \name Draw Target API
    // ---------------------------------------------------------------------- //
    bool                         IsEnabled()        const { return  _enabled;    }
    const GlfDrawTargetRefPtr   &GetGlfDrawTarget() const { return  _drawTarget; }
    const HdxDrawTargetRenderPassState *GetRenderPassState() const
    {
        return &_renderPassState;
    }

    /// Debug api to output the contents of the draw target to a png file.
    HDX_API
    bool WriteToFile(const HdRenderIndex &renderIndex,
                     const std::string &attachment,
                     const std::string &path) const;

    /// returns all HdxDrawTargets in the render index
    HDX_API
    static void GetDrawTargets(HdSceneDelegate *sceneDelegate,
                               HdxDrawTargetPtrConstVector *drawTargets);


private:
    unsigned int     _version;

    bool                    _enabled;
    SdfPath                 _cameraId;
    GfVec2i                 _resolution;
    HdRprimCollectionVector _collections;

    HdxDrawTargetRenderPassState _renderPassState;
    std::vector<HdTextureResourceSharedPtr> _colorTextureResources;
    HdTextureResourceSharedPtr              _depthTextureResource;

    /// The context which owns the draw target object.
    GlfGLContextSharedPtr  _drawTargetContext;
    GlfDrawTargetRefPtr    _drawTarget;

    void _SetAttachments(HdSceneDelegate *sceneDelegate,
                         const HdxDrawTargetAttachmentDescArray &attachments);

    void _SetCamera(const SdfPath &cameraPath);

    const HdxCamera *_GetCamera(const HdRenderIndex &renderIndex) const;

    void _ResizeDrawTarget();
    void _RegisterTextureResource(HdSceneDelegate *sceneDelegate,
                                  const std::string &name,
                                  HdTextureResourceSharedPtr *resourcePtr);

    // No copy
    HdxDrawTarget()                                 = delete;
    HdxDrawTarget(const HdxDrawTarget &)             = delete;
    HdxDrawTarget &operator =(const HdxDrawTarget &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDX_DRAW_TARGET_H
