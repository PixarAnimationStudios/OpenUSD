//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_DRAW_TARGET_H
#define PXR_IMAGING_LOFI_DRAW_TARGET_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/plugin/LoFi/drawTargetRenderPassState.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


#define LOFI_DRAW_TARGET_TOKENS                 \
    (camera)                                    \
    (collection)                                \
    (drawTargetSet)                             \
    (enable)                                    \
    (resolution)                                \
    (aovBindings)                               \
    (depthPriority)

TF_DECLARE_PUBLIC_TOKENS(LoFiDrawTargetTokens, LOFI_API, LOFI_DRAW_TARGET_TOKENS);

class HdCamera;
class HdRenderIndex;
using LoFiDrawTargetPtrVector = std::vector<class LoFiDrawTarget *>;

/// \class LoFiDrawTarget
///
/// Represents an render to texture render pass.
///
/// \note This is a temporary API to aid transition to Storm, and is subject
/// to major changes.
///
class LoFiDrawTarget : public HdSprim
{
public:
    LOFI_API
    LoFiDrawTarget(SdfPath const & id);
    LOFI_API
    ~LoFiDrawTarget() override;

    /// Dirty bits for the LoFiDrawTarget object
    ///
    /// When GetUseStormTextureSystem() is true, "Legacy" dirty
    /// bits are ignored.
    ///
    enum DirtyBits : HdDirtyBits {
        Clean                   = 0,
        DirtyDTEnable           = 1 <<  0,
        DirtyDTCamera           = 1 <<  1,
        DirtyDTResolution       = 1 <<  2,
        DirtyDTAovBindings      = 1 <<  4,
        DirtyDTDepthPriority    = 1 <<  6,
        DirtyDTCollection       = 1 <<  7,
        AllDirty                = (DirtyDTEnable
                                   |DirtyDTCamera
                                   |DirtyDTResolution
                                   |DirtyDTAovBindings
                                   |DirtyDTDepthPriority
                                   |DirtyDTCollection)
    };

    /// Synchronizes state from the delegate to this object.
    LOFI_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;
    
    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    LOFI_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;


    // ---------------------------------------------------------------------- //
    /// \name Draw Target API
    // ---------------------------------------------------------------------- //
    bool                       IsEnabled()        const { return  _enabled;    }
    const LoFiDrawTargetRenderPassState *GetDrawTargetRenderPassState() const
    {
        return &_drawTargetRenderPassState;
    }

    /// Returns collection of rprims the draw target draws.
    LOFI_API
    HdRprimCollection const &GetCollection() const { return _collection; }

    /// returns all HdStDrawTargets in the render index
    LOFI_API
    static void GetDrawTargets(HdRenderIndex* renderIndex,
                               LoFiDrawTargetPtrVector *drawTargets);

    /// Resolution.
    ///
    /// Set during sync.
    ///
    const GfVec2i &GetResolution() const {
        return _resolution;
    }

private:
    bool                    _enabled;
    GfVec2i                 _resolution;
    HdRprimCollection       _collection;

    LoFiDrawTargetRenderPassState _drawTargetRenderPassState;

    // No copy
    LoFiDrawTarget()                                   = delete;
    LoFiDrawTarget(const LoFiDrawTarget &)             = delete;
    LoFiDrawTarget &operator =(const LoFiDrawTarget &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_LOFI_DRAW_TARGET_H
