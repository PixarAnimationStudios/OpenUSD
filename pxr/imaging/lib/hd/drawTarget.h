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
#ifndef HD_DRAW_TARGET_H
#define HD_DRAW_TARGET_H

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/drawTargetRenderPassState.h"

#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/usd/sdf/path.h"

#include <boost/shared_ptr.hpp>

#include <vector>

class HdSceneDelegate;
class HdDrawTargetAttachmentDescArray;

typedef boost::shared_ptr<class HdDrawTarget> HdDrawTargetSharedPtr;
typedef boost::shared_ptr<class HdSprim> HdSprimSharedPtr;
typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

typedef std::vector<HdDrawTargetSharedPtr> HdDrawTargetSharedPtrVector;

/// \class HdDrawTarget
///
/// Represents an render to texture render pass.
///
/// \note This is a temporary API to aid transition to hydra, and is subject
/// to major changes.
///
class HdDrawTarget final {
public:
    HdDrawTarget(HdSceneDelegate* delegate, SdfPath const & id);
    ~HdDrawTarget(); // note: not virtual as final class

    /// Returns the HdSceneDelegate which backs this draw target.
    HdSceneDelegate* GetDelegate() const { return _delegate; }

    /// Returns the identifer by which this draw target is known. This
    /// identifier is a common associative key used by the SceneDelegate,
    /// RenderIndex, and for binding to the draw target
    SdfPath const& GetID() const { return _id; }

    /// Returns the version of the under-lying GlfDrawTarget.
    /// The version changes if the draw target attachments texture ids
    /// are changed in anyway (for example switching to a new
    /// GlfDrawTarget object or resize the resources).
    /// The version does not increment if only the contents of the
    /// texture resources change
    unsigned int GetVersion() const { return _version; }

    /// Synchronizes state from the delegate to Hydra, for example, allocating
    /// parameters into GPU memory.
    void Sync();

    // ---------------------------------------------------------------------- //
    /// \name Draw Target API
    // ---------------------------------------------------------------------- //
    bool                         IsEnabled()        const { return  _enabled;    }
    const GlfDrawTargetRefPtr   &GetGlfDrawTarget() const { return  _drawTarget; }
    HdDrawTargetRenderPassState *GetRenderPassState()     { return &_renderPassState; }

    /// Debug api to output the contents of the draw target to a png file.
    bool                WriteToFile(const std::string &attachment,
                                    const std::string &path);

private:
    HdSceneDelegate* _delegate;
    SdfPath          _id;
    unsigned int     _version;

    bool                    _enabled;
    SdfPath                 _cameraId;
    GfVec2i                 _resolution;
    HdRprimCollectionVector _collections;

    HdDrawTargetRenderPassState _renderPassState;

    /// The context which owns the draw target object.
    GlfGLContextSharedPtr  _drawTargetContext;
    GlfDrawTargetRefPtr    _drawTarget;

    void _SetAttachments(const HdDrawTargetAttachmentDescArray &attachments);
    void _SetCamera(const SdfPath &cameraPath);

    HdSprimSharedPtr _GetCamera() const;

    void _ResizeDrawTarget();

    // No copy
    HdDrawTarget()                                 = delete;
    HdDrawTarget(const HdDrawTarget &)             = delete;
    HdDrawTarget &operator =(const HdDrawTarget &) = delete;
};

#endif  // HD_DRAW_TARGET_H
