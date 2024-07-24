//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXTRAS_IMAGING_EXAMPLES_HD_TINY_RENDER_DELEGATE_H
#define EXTRAS_IMAGING_EXAMPLES_HD_TINY_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdTinyRenderDelegate
///
/// Render delegates provide renderer-specific functionality to the render
/// index, the main hydra state management structure. The render index uses
/// the render delegate to create and delete scene primitives, which include
/// geometry and also non-drawable objects. The render delegate is also
/// responsible for creating renderpasses, which know how to draw this
/// renderer's scene primitives.
///
class HdTinyRenderDelegate final : public HdRenderDelegate 
{
public:
    /// Render delegate constructor. 
    HdTinyRenderDelegate();
    /// Render delegate constructor. 
    HdTinyRenderDelegate(HdRenderSettingsMap const& settingsMap);
    /// Render delegate destructor.
    virtual ~HdTinyRenderDelegate();

    /// Supported types
    const TfTokenVector &GetSupportedRprimTypes() const override;
    const TfTokenVector &GetSupportedSprimTypes() const override;
    const TfTokenVector &GetSupportedBprimTypes() const override;

    // Basic value to return from the RD
    HdResourceRegistrySharedPtr GetResourceRegistry() const override;

    // Prims
    HdRenderPassSharedPtr CreateRenderPass(
        HdRenderIndex *index,
        HdRprimCollection const& collection) override;

    HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id) override;
    void DestroyInstancer(HdInstancer *instancer) override;

    HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId) override;
    void DestroyRprim(HdRprim *rPrim) override;

    HdSprim *CreateSprim(TfToken const& typeId,
                         SdfPath const& sprimId) override;
    HdSprim *CreateFallbackSprim(TfToken const& typeId) override;
    void DestroySprim(HdSprim *sprim) override;

    HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override;
    HdBprim *CreateFallbackBprim(TfToken const& typeId) override;
    void DestroyBprim(HdBprim *bprim) override;

    void CommitResources(HdChangeTracker *tracker) override;

    HdRenderParam *GetRenderParam() const override;

private:
    static const TfTokenVector SUPPORTED_RPRIM_TYPES;
    static const TfTokenVector SUPPORTED_SPRIM_TYPES;
    static const TfTokenVector SUPPORTED_BPRIM_TYPES;

    void _Initialize();

    HdResourceRegistrySharedPtr _resourceRegistry;

    // This class does not support copying.
    HdTinyRenderDelegate(const HdTinyRenderDelegate &) = delete;
    HdTinyRenderDelegate &operator =(const HdTinyRenderDelegate &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXTRAS_IMAGING_EXAMPLES_HD_TINY_RENDER_DELEGATE_H
