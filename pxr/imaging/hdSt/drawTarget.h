//
// Copyright 2017 Pixar
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
#ifndef PXR_IMAGING_HD_ST_DRAW_TARGET_H
#define PXR_IMAGING_HD_ST_DRAW_TARGET_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/drawTargetRenderPassState.h"
#include "pxr/imaging/hdSt/textureResourceHandle.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


#define HDST_DRAW_TARGET_TOKENS                 \
    (attachments)                               \
    (camera)                                    \
    (collection)                                \
    (depthClearValue)                           \
    (drawTargetSet)                             \
    (enable)                                    \
    (resolution)                                \
    (depth)

TF_DECLARE_PUBLIC_TOKENS(HdStDrawTargetTokens, HDST_API, HDST_DRAW_TARGET_TOKENS);

class HdSceneDelegate;
class HdRenderIndex;
class HdCamera;
class HdStDrawTargetAttachmentDescArray;
class HdStTextureIdentifier;
class HdStResourceRegistry;

using GlfGLContextSharedPtr = std::shared_ptr<class GlfGLContext>;

using HdStDrawTargetPtrVector = std::vector<class HdStDrawTarget *>;

using HdStDynamicUvTextureObjectSharedPtr =
    std::shared_ptr<class HdStDynamicUvTextureObject>;

/// \class HdStDrawTarget
///
/// Represents an render to texture render pass.
///
/// \note This is a temporary API to aid transition to Storm, and is subject
/// to major changes.
///
class HdStDrawTarget : public HdSprim
{
public:
    HDST_API
    HdStDrawTarget(SdfPath const & id);
    HDST_API
    ~HdStDrawTarget() override;

    /// Dirty bits for the HdStDrawTarget object
    enum DirtyBits : HdDirtyBits {
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

    /// If true, the draw target attachments are managed by the
    /// Storm texture system. This also makes the draw target task
    /// use HgiGraphicsCmdDesc to bind the attachments as render
    /// targets. Shaders who want to read the attachments can bind
    /// the textures like any other texture in the Storm texture
    /// system.
    /// Note: draw targets managed by the Storm texture system do not
    /// work when bindless textures are enabled.
    ///
    /// If false, uses GlfDrawTarget.
    ///
    HDST_API
    static bool GetUseStormTextureSystem();

    /// Returns the version of the under-lying GlfDrawTarget.
    /// The version changes if the draw target attachments texture ids
    /// are changed in anyway (for example switching to a new
    /// GlfDrawTarget object or resize the resources).
    /// The version does not increment if only the contents of the
    /// texture resources change
    unsigned int GetVersion() const { return _version; }

    /// Synchronizes state from the delegate to this object.
    HDST_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;
    
    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;


    // ---------------------------------------------------------------------- //
    /// \name Draw Target API
    // ---------------------------------------------------------------------- //
    bool                       IsEnabled()        const { return  _enabled;    }
    const GlfDrawTargetRefPtr &GetGlfDrawTarget() const { return  _drawTarget; }
    const HdStDrawTargetRenderPassState *GetRenderPassState() const
    {
        return &_renderPassState;
    }

    /// Returns collection of rprims the draw target draws.
    HDST_API
    HdRprimCollection const &GetCollection() const { return _collection; }

    /// Debug api to output the contents of the draw target to a png file.
    HDST_API
    bool WriteToFile(const HdRenderIndex &renderIndex,
                     const std::string &attachment,
                     const std::string &path) const;

    /// returns all HdStDrawTargets in the render index
    HDST_API
    static void GetDrawTargets(HdRenderIndex* renderIndex,
                               HdStDrawTargetPtrVector *drawTargets);

    /// When using the Storm texture system for draw targets, returns
    /// the texture identifier for an attachment. A shader can bind
    /// this texture using this texture identifier when allocating a
    /// texture handle with the resource registry.
    ///
    /// Note: Calling GetTextureIdentifier from within a material sync
    /// is still safe even though a draw target might be synced after
    /// the material since both are sprims.
    ///
    HDST_API
    HdStTextureIdentifier GetTextureIdentifier(
        /// Name of attachment (depth is treated the same as color attachments).
        const std::string &attachmentName,
        const HdSceneDelegate * sceneDelegate,
        /// Retrieve MSAA texture instead of resolved texture.
        bool multiSampled = false) const;

    /// Allocate the actual GPU textures used as attachments.
    ///
    /// Only call if using the Storm texture system.
    ///
    HDST_API
    void AllocateTexturesIfNecessary();

    /// The data describing the draw target attachments when using the Storm
    /// texture system. Filled during Sync.
    struct AttachmentData {
        /// Attachment name (e.g., color, depth).
        std::string name;
        /// Value used to clear attachment before rendering to it.
        GfVec4f clearValue;
        /// Format of attachment
        HdFormat format;
        /// (Resolved) texture to read from.
        HdStDynamicUvTextureObjectSharedPtr texture;
        /// MSAA texture to render to (if MSAA is enabled for draw targets).
        HdStDynamicUvTextureObjectSharedPtr textureMSAA;
    };

    using AttachmentDataVector = std::vector<AttachmentData>;
    
    /// Get data describing the darw target attachments.
    ///
    /// Only call if using the Storm texture system.
    ///
    const AttachmentDataVector &GetAttachments() const {
        return _attachmentDataVector;
    }

    /// Resolution.
    ///
    /// Set during sync.
    ///
    const GfVec2i &GetResolution() const {
        return _resolution;
    }

private:
    unsigned int     _version;

    bool                    _enabled;
    SdfPath                 _cameraId;
    GfVec2i                 _resolution;
    float                   _depthClearValue;
    HdRprimCollection       _collection;

    HdStDrawTargetRenderPassState _renderPassState;
    std::vector<HdStTextureResourceHandleSharedPtr> _colorTextureResourceHandles;
    HdStTextureResourceHandleSharedPtr              _depthTextureResourceHandle;

    /// The context which owns the draw target object.
    GlfGLContextSharedPtr  _drawTargetContext;
    GlfDrawTargetRefPtr    _drawTarget;

    AttachmentDataVector _attachmentDataVector;
    // Is it necessary to create GPU resources because they are uninitialized
    // or the attachments/resolution changed.
    bool _texturesDirty;

    void _SetAttachments(HdSceneDelegate *sceneDelegate,
                         const HdStDrawTargetAttachmentDescArray &attachments);

    // Helper used to populate attachment data.
    HdStDynamicUvTextureObjectSharedPtr _CreateTextureObject(
        const std::string &name,
        HdSceneDelegate * sceneDelegate,
        HdStResourceRegistry * resourceRegistry,
        bool multiSampled);

    // Populate attachment data from descriptor.
    void _SetAttachmentData(
        HdSceneDelegate *sceneDelegate,
        const HdStDrawTargetAttachmentDescArray &attachments);

    // Set clear value for depth attachments.
    void _SetAttachmentDataDepthClearValue();

    void _SetCamera(const SdfPath &cameraPath);

    const HdCamera *_GetCamera(const HdRenderIndex &renderIndex) const;

    void _ResizeDrawTarget();
    void _RegisterTextureResourceHandle(HdSceneDelegate *sceneDelegate,
                                  const std::string &name,
                                  HdStTextureResourceHandleSharedPtr *handlePtr);

    // No copy
    HdStDrawTarget()                                   = delete;
    HdStDrawTarget(const HdStDrawTarget &)             = delete;
    HdStDrawTarget &operator =(const HdStDrawTarget &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_DRAW_TARGET_H
