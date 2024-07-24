//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SPRIM_H
#define PXR_IMAGING_HD_SPRIM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdRenderParam;

/// \class HdSprim
///
/// Sprim (state prim) is a base class of managing state for non-drawable
/// scene entity (e.g. camera, light). Similar to Rprim, Sprim communicates
/// scene delegate and tracks the changes through change tracker, then updates
/// data cached in Hd (either on CPU or GPU).
///
/// Unlike Rprim, Sprim doesn't produce draw items. The data cached in HdSprim
/// may be used by HdTask or by HdShader.
///
/// The lifetime of HdSprim is owned by HdRenderIndex.
///
class HdSprim
{
public:
    HD_API
    HdSprim(SdfPath const & id);
    HD_API
    virtual ~HdSprim();

    /// Returns the identifier by which this state is known. This
    /// identifier is a common associative key used by the SceneDelegate,
    /// RenderIndex, and for binding to the state (e.g. camera, light)
    SdfPath const& GetId() const { return _id; }

    /// Synchronizes state from the delegate to this object.
    /// @param[in, out]  dirtyBits: On input specifies which state is
    ///                             is dirty and can be pulled from the scene
    ///                             delegate.
    ///                             On output specifies which bits are still
    ///                             dirty and were not cleaned by the sync. 
    ///                             
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits) = 0;

    /// Finalizes object resources. This function might not delete resources,
    /// but it should deal with resource ownership so that the sprim is
    /// deletable.
    HD_API
    virtual void Finalize(HdRenderParam *renderParam);

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    virtual HdDirtyBits GetInitialDirtyBitsMask() const = 0;

private:
    SdfPath _id;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_SPRIM_H
