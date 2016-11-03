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

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/drawTargetRenderPassState.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"

#include <boost/shared_ptr.hpp>

#include <vector>

#define HDX_DRAW_TARGET_TOKENS                  \
    (attachments)                               \
    (camera)                                    \
    (collection)                                \
    (depthClearValue)                           \
    (drawTargetSet)                             \
    (enable)                                    \
    (resolution)

TF_DECLARE_PUBLIC_TOKENS(HdxDrawTargetTokens, HDXLIB_API, HDX_DRAW_TARGET_TOKENS);

class HdSceneDelegate;
class HdxDrawTargetAttachmentDescArray;

typedef boost::shared_ptr<class HdxDrawTarget> HdxDrawTargetSharedPtr;
typedef boost::shared_ptr<class HdSprim> HdSprimSharedPtr;
typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

typedef std::vector<HdxDrawTargetSharedPtr> HdxDrawTargetSharedPtrVector;

/// \class HdxDrawTarget
///
/// Represents an render to texture render pass.
///
/// \note This is a temporary API to aid transition to hydra, and is subject
/// to major changes.
///
class HdxDrawTarget : public HdSprim {
public:
    HdxDrawTarget(HdSceneDelegate* delegate, SdfPath const & id);
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
    virtual void Sync();

    /// Accessor for tasks to get the parameters cached in this object.
    virtual VtValue Get(TfToken const &token) const;

    // ---------------------------------------------------------------------- //
    /// \name Draw Target API
    // ---------------------------------------------------------------------- //
    bool                         IsEnabled()        const { return  _enabled;    }
    const GlfDrawTargetRefPtr   &GetGlfDrawTarget() const { return  _drawTarget; }
    HdxDrawTargetRenderPassState *GetRenderPassState()     { return &_renderPassState; }

    /// Debug api to output the contents of the draw target to a png file.
    bool                WriteToFile(const std::string &attachment,
                                    const std::string &path);

    /// returns all HdxDrawTargets in the render index
    static void GetDrawTargets(HdSceneDelegate *delegate,
                               HdxDrawTargetSharedPtrVector *drawTargets);

private:
    unsigned int     _version;

    bool                    _enabled;
    SdfPath                 _cameraId;
    GfVec2i                 _resolution;
    HdRprimCollectionVector _collections;

    HdxDrawTargetRenderPassState _renderPassState;

    /// The context which owns the draw target object.
    GlfGLContextSharedPtr  _drawTargetContext;
    GlfDrawTargetRefPtr    _drawTarget;

    void _SetAttachments(const HdxDrawTargetAttachmentDescArray &attachments);
    void _SetCamera(const SdfPath &cameraPath);

    HdSprimSharedPtr _GetCamera() const;

    void _ResizeDrawTarget();

    // No copy
    HdxDrawTarget()                                 = delete;
    HdxDrawTarget(const HdxDrawTarget &)             = delete;
    HdxDrawTarget &operator =(const HdxDrawTarget &) = delete;
};

#endif  // HDX_DRAW_TARGET_H
