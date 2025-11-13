//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_LEGACY_RENDER_CONTROL_INTERFACE_H
#define PXR_IMAGING_HD_LEGACY_RENDER_CONTROL_INTERFACE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/command.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdRenderDelegateInfo;
class HdRenderBuffer;
using HdRenderSettingDescriptorList =
    std::vector<struct HdRenderSettingDescriptor>;
using HdCommandDescriptors =
    std::vector<struct HdCommandDescriptor>;

///
/// \class HdLegacyRenderControlInterface
///
/// Transitory Hydra-1.0-like API for HdRenderer.
///
class HdLegacyRenderControlInterface
{
public:
    /// \name Task control
    /// @{

    virtual void Execute(
        const SdfPathVector &taskPaths) = 0;

    virtual bool AreTasksConverged(
        const SdfPathVector &taskPaths) const = 0;

    virtual bool GetTaskContextData(
        const TfToken &name, VtValue *data) const = 0;

    virtual void SetTaskContextData(
        const TfToken &name, const VtValue &data) = 0;

    /// @}

    /// \name AOVs
    /// @{

    virtual HdAovDescriptor GetDefaultAovDescriptor(
        const TfToken &name) const = 0;

    virtual HdRenderBuffer * GetRenderBuffer(
        const SdfPath &path) const = 0;

    /// @}

    /// \name Render settings
    /// @{
    
    virtual HdRenderSettingDescriptorList
        GetRenderSettingDescriptors() const = 0;
    virtual VtValue GetRenderSetting(
        const TfToken &name) const = 0;
    virtual void SetRenderSetting(
        const TfToken &name, VtValue const &value) = 0;

    /// @}

    /// \name Commands
    /// @{

    virtual HdCommandDescriptors GetCommandDescriptors() = 0;

    ///
    /// Invokes the command described by the token \p command with optional
    /// \p args.
    ///
    /// If the command succeeds, returns \c true, otherwise returns \c false.
    /// A command will generally fail if it is not among those returned by
    /// GetCommandDescriptors().
    ///
    virtual bool InvokeCommand(
        TfToken const &name, const HdCommandArgs &args = {}) = 0;

    /// @}
    
    /// \name Background rendering control
    /// @{
    
    virtual bool IsPauseSupported() const = 0;
    virtual bool Pause() = 0;
    virtual bool Resume() = 0;
    virtual bool IsStopSupported() const = 0;
    virtual bool Stop(bool blocking = true) = 0;
    virtual bool Restart() = 0;

    /// @}

    /// \name Resolution information
    /// @{
    
    virtual TfToken GetMaterialBindingPurpose() const = 0;
    virtual TfTokenVector GetMaterialRenderContexts() const = 0;
    virtual TfTokenVector GetRenderSettingsNamespaces() const = 0;
    virtual bool IsPrimvarFilteringNeeded() const = 0;
    virtual TfTokenVector GetShaderSourceTypes() const = 0;
    /// This is for UsdImagingDelegate::_coordSysEnabled that has a
    /// surprising performance impact.
    virtual bool IsCoordSysSupported() const = 0;

    HD_API
    HdRenderDelegateInfo GetRenderDelegateInfo() const;
    /// @}
    
    /// \name Misc
    /// @{

    /// Provides value for HdxTaskControllerSceneIndex::Parameters::isForStorm
    /// to configure task controller scene inedx.
    virtual bool RequiresStormTasks() const = 0;

    virtual VtDictionary GetRenderStats() = 0;

    virtual SdfPath GetRprimPathFromPrimId(int primIdx) = 0;

    /// @}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
