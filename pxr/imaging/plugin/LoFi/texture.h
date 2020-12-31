//
// Copyright 2020 benmalartre
//
// Unlicensed 
// 
#ifndef PXR_IMAGING_LOFI_TEXTURE_H
#define PXR_IMAGING_LOFI_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/enums.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;
class HdRenderIndex;

///
/// Represents a Texture Buffer Prim.
///
/// Implements no behaviors.
///
class LoFiTexture : public HdTexture
{
public:
    LOFI_API
    LoFiTexture(SdfPath const & id);
    LOFI_API
    ~LoFiTexture() override;

    /// Synchronizes state from the delegate to Hydra, for example, allocating
    /// parameters into GPU memory.
    LOFI_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    LOFI_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_LOFI_TEXTURE_H

