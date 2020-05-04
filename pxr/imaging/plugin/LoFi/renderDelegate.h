//
// Copyright 2020 Pixar
//
// Unlicensed 
//
#ifndef EXTRAS_IMAGING_EXAMPLES_HD_LoFi_RENDER_DELEGATE_H
#define EXTRAS_IMAGING_EXAMPLES_HD_LoFi_RENDER_DELEGATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

class LoFiRenderParam;

#define LOFI_RENDER_SETTINGS_TOKENS \
    (enableLights)                  \
    (enableShadows)                 \
    (enableLines)                   \
    

// Also: HdRenderSettingsTokens->convergedSamplesPerPixel

TF_DECLARE_PUBLIC_TOKENS(LoFiRenderSettingsTokens, LOFI_RENDER_SETTINGS_TOKENS);

///
/// \class LoFiRenderDelegate
///
/// Render delegates provide renderer-specific functionality to the render
/// index, the main hydra state management structure. The render index uses
/// the render delegate to create and delete scene primitives, which include
/// geometry and also non-drawable objects. The render delegate is also
/// responsible for creating renderpasses, which know how to draw this
/// renderer's scene primitives.
///
class LoFiRenderDelegate final : public HdRenderDelegate 
{
public:
  /// Render delegate constructor. 
  LoFiRenderDelegate();
  /// Render delegate constructor. 
  LoFiRenderDelegate(HdRenderSettingsMap const& settingsMap);
  /// Render delegate destructor.
  virtual ~LoFiRenderDelegate();

  /// Supported types
  const TfTokenVector &GetSupportedRprimTypes() const override;
  const TfTokenVector &GetSupportedSprimTypes() const override;
  const TfTokenVector &GetSupportedBprimTypes() const override;

  // Basic value to return from the RD
  HdResourceRegistrySharedPtr GetResourceRegistry() const override;

  /// Returns a list of user-configurable render settings.
  /// This is a reflection API for the render settings dictionary; it need
  /// not be exhaustive, but can be used for populating application settings
  /// UI.
  virtual HdRenderSettingDescriptorList
    GetRenderSettingDescriptors() const override;

  // Prims
  HdRenderPassSharedPtr CreateRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const& collection) override;

  HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                        SdfPath const& id,
                                        SdfPath const& instancerId) override;
  void DestroyInstancer(HdInstancer *instancer) override;

  HdRprim *CreateRprim(TfToken const& typeId,
                                SdfPath const& rprimId,
                                SdfPath const& instancerId) override;
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

  //HdRenderParam* GetRenderParam() const override;

private:
  static const TfTokenVector SUPPORTED_RPRIM_TYPES;
  static const TfTokenVector SUPPORTED_SPRIM_TYPES;
  static const TfTokenVector SUPPORTED_BPRIM_TYPES;

  /// Resource registry used in this render delegate
  static std::mutex _mutexResourceRegistry;
  static std::atomic_int _counterResourceRegistry;
  static LoFiResourceRegistrySharedPtr _resourceRegistry;

  // LoFi initialization routine.
  void _Initialize();

  // Handle to the render param to pass
  //LoFiRenderParam* _renderParams;

  // Handle to the renderPassState
  HdRenderPassStateSharedPtr _renderPassState;

  // A list of render setting exports.
  HdRenderSettingDescriptorList _settingDescriptors;

  // This class does not support copying.
  LoFiRenderDelegate(const LoFiRenderDelegate &) = delete;
  LoFiRenderDelegate &operator =(const LoFiRenderDelegate &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXTRAS_IMAGING_EXAMPLES_HD_LoFi_RENDER_DELEGATE_H
