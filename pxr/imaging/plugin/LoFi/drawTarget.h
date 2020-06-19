//
// Copyright 2020 benmalartre
//
// Unlicensed 
// 
#ifndef PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_H
#define PXR_IMAGING_PLUGIN_LOFI_DRAW_TARGET_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/plugin/LoFi/drawTargetRenderPassState.h"
#include "pxr/imaging/plugin/LoFi/textureResourceHandle.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/glf/drawTarget.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


#define LOFI_DRAW_TARGET_TOKENS                 \
    (attachments)                               \
    (camera)                                    \
    (collection)                                \
    (depthClearValue)                           \
    (drawTargetSet)                             \
    (enable)                                    \
    (resolution)

TF_DECLARE_PUBLIC_TOKENS(LoFiDrawTargetTokens, LOFI_API, LOFI_DRAW_TARGET_TOKENS);

class HdSceneDelegate;
class HdRenderIndex;
class HdCamera;
class LoFiDrawTargetAttachmentDescArray;


typedef std::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

typedef std::vector<class LoFiDrawTarget const *> LoFiDrawTargetPtrConstVector;

/// \class LoFiDrawTarget
///
/// Represents an render to texture render pass.
///
/// \note This is a temporary API to aid transition to Storm, and is subject
/// to major changes.
///
class LoFiDrawTarget : public HdSprim {
public:
    LOFI_API
    LoFiDrawTarget(SdfPath const & id);
    LOFI_API
    virtual ~LoFiDrawTarget();

    /// Dirty bits for the LoFiDrawTarget object
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

    /// Returns the version of the under-lying GlfDrawTarget.
    /// The version changes if the draw target attachments texture ids
    /// are changed in anyway (for example switching to a new
    /// GlfDrawTarget object or resize the resources).
    /// The version does not increment if only the contents of the
    /// texture resources change
    unsigned int GetVersion() const { return _version; }

    /// Synchronizes state from the delegate to this object.
    LOFI_API
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    LOFI_API
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;


    // ---------------------------------------------------------------------- //
    /// \name Draw Target API
    // ---------------------------------------------------------------------- //
    bool                       IsEnabled()        const { return  _enabled;    }
    const GlfDrawTargetRefPtr &GetGlfDrawTarget() const { return  _drawTarget; }
    const LoFiDrawTargetRenderPassState *GetRenderPassState() const
    {
        return &_renderPassState;
    }

    /// Returns collection of rprims the draw target draws.
    LOFI_API
    HdRprimCollection const &GetCollection() const { return _collection; }

    /// Debug api to output the contents of the draw target to a png file.
    LOFI_API
    bool WriteToFile(const HdRenderIndex &renderIndex,
                     const std::string &attachment,
                     const std::string &path) const;

    /// returns all LoFiDrawTargets in the render index
    LOFI_API
    static void GetDrawTargets(HdRenderIndex* renderIndex,
                               LoFiDrawTargetPtrConstVector *drawTargets);


private:
    unsigned int     _version;

    bool                    _enabled;
    SdfPath                 _cameraId;
    GfVec2i                 _resolution;
    HdRprimCollection       _collection;

    LoFiDrawTargetRenderPassState _renderPassState;
    std::vector<LoFiTextureResourceHandleSharedPtr> _colorTextureResourceHandles;
    LoFiTextureResourceHandleSharedPtr              _depthTextureResourceHandle;

    /// The context which owns the draw target object.
    GlfGLContextSharedPtr  _drawTargetContext;
    GlfDrawTargetRefPtr    _drawTarget;

    void _SetAttachments(HdSceneDelegate *sceneDelegate,
                         const LoFiDrawTargetAttachmentDescArray &attachments);

    void _SetCamera(const SdfPath &cameraPath);

    const HdCamera *_GetCamera(const HdRenderIndex &renderIndex) const;

    void _ResizeDrawTarget();
    void _RegisterTextureResourceHandle(HdSceneDelegate *sceneDelegate,
                                  const std::string &name,
                                  LoFiTextureResourceHandleSharedPtr *handlePtr);

    // No copy
    LoFiDrawTarget()                                   = delete;
    LoFiDrawTarget(const LoFiDrawTarget &)             = delete;
    LoFiDrawTarget &operator =(const LoFiDrawTarget &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_DRAW_TARGET_H
