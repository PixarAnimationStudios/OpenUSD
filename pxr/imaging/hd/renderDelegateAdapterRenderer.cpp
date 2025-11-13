//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/renderDelegateAdapterRenderer.h"

#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hd/legacyRenderControlInterface.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/rendererCreateArgsSchema.h"
#include "pxr/imaging/hd/renderIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (renderDriver)
);

std::vector<HdDriver>
_ComputeDrivers(HdContainerDataSourceHandle const &rendererCreateArgs)
{
    const HdRendererCreateArgsSchema argsSchema(rendererCreateArgs);
    const HdSampledDataSourceContainerSchema driverSchema =
        argsSchema.GetDrivers();

    std::vector<HdDriver> drivers;
    for (const TfToken &name : driverSchema.GetNames()) {
        HdSampledDataSourceHandle const ds = driverSchema.Get(name);
        if (!ds) {
            continue;
        }
        drivers.push_back({ name, ds->GetValue(0.0f)});
        // For concreteness, we now use Hgi rather than renderDriver as key.
        if (name == HdRendererCreateArgsSchemaTokens->hgi) {
            drivers.push_back({ _tokens->renderDriver, ds->GetValue(0.0f)});
        }
    }

    return drivers;
}

/// HdDriverVector is a std::vector<HdDriver*> (and the argument to
/// HdRenderDelegate::SetDrivers) - even though HdDriver is not intended
/// as an (abstract) base class but a struct of a TfToken and VtValue
/// and it is the VtValue that can hold a pointer to an abstract base
/// class such as Hgi.
///
/// Creating a std::vector<HdDriver*> here.
/// HdRenderDelegateAdapterRenderer::_drivers owns the HdDriver struct's.
///
class HdRenderDelegateAdapterRenderer::_LegacyRenderControl
    : public HdLegacyRenderControlInterface
{
public:
    _LegacyRenderControl(
        HdRenderDelegate * const renderDelegate,
        HdRenderIndex * const renderIndex,
        HdEngine * const engine)
      : _renderDelegate(renderDelegate)
      , _renderIndex(renderIndex)
      , _engine(engine)
    {
    }

    void Execute(const SdfPathVector &taskPaths) override {
        _engine->Execute(_renderIndex, taskPaths);
    }

    bool AreTasksConverged(const SdfPathVector &taskPaths) const override {
        return _engine->AreTasksConverged(_renderIndex, taskPaths);
    }

    bool GetTaskContextData(const TfToken &name, VtValue *data) const override {
        return _engine->GetTaskContextData(name, data);
    }

    void SetTaskContextData(const TfToken &name, const VtValue &data) override {
        return _engine->SetTaskContextData(name, data);
    }

    HdAovDescriptor
    GetDefaultAovDescriptor(const TfToken &name) const override {
        return _renderDelegate->GetDefaultAovDescriptor(name);
    }

    HdRenderBuffer * GetRenderBuffer(const SdfPath &path) const override
    {
        return
            dynamic_cast<HdRenderBuffer*>(
                _renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer, path));
    }

    HdRenderSettingDescriptorList GetRenderSettingDescriptors() const override {
        return _renderDelegate->GetRenderSettingDescriptors();
    }

    VtValue GetRenderSetting(const TfToken &name) const override {
        return _renderDelegate->GetRenderSetting(name);
    }

    void SetRenderSetting(const TfToken &name, VtValue const &value) override {
        _renderDelegate->SetRenderSetting(name, value);
    }

    HdCommandDescriptors GetCommandDescriptors() override {
        return _renderDelegate->GetCommandDescriptors();
    }

    bool InvokeCommand(
        TfToken const &name, const HdCommandArgs &args) override {
        return _renderDelegate->InvokeCommand(name, args);
    }

    bool IsPauseSupported() const override {
        return _renderDelegate->IsPauseSupported();
    }

    bool Pause() override {
        return _renderDelegate->Pause();
    }

    bool Resume() override {
        return _renderDelegate->Resume();
    }

    bool IsStopSupported() const override {
        return _renderDelegate->IsStopSupported();
    }

    bool Stop(bool blocking = true) override {
        return _renderDelegate->Stop(blocking);
    }

    bool Restart() override {
        return _renderDelegate->Restart();
    }

    TfToken GetMaterialBindingPurpose() const override {
        return _renderDelegate->GetMaterialBindingPurpose();
    }

    TfTokenVector GetMaterialRenderContexts() const override {
        return _renderDelegate->GetMaterialRenderContexts();
    }

    TfTokenVector GetRenderSettingsNamespaces() const override {
        return _renderDelegate->GetRenderSettingsNamespaces();
    }

    bool IsPrimvarFilteringNeeded() const override {
        return _renderDelegate->IsPrimvarFilteringNeeded();
    }

    TfTokenVector GetShaderSourceTypes() const override {
        return _renderDelegate->GetShaderSourceTypes();
    }

    bool IsCoordSysSupported() const override {
        return _renderIndex->IsSprimTypeSupported(
            HdPrimTypeTokens->coordSys);
    }

    bool RequiresStormTasks() const override {
        return _renderDelegate->RequiresStormTasks();
    }

    VtDictionary GetRenderStats() override {
        return _renderDelegate->GetRenderStats();
    }

    SdfPath GetRprimPathFromPrimId(const int primIdx) override {
        return _renderIndex->GetRprimPathFromPrimId(primIdx);
    }

private:
    HdRenderDelegate * const _renderDelegate;
    HdRenderIndex * const _renderIndex;
    HdEngine * const _engine;
};

static
HdDriverVector
_ToPointers(const std::vector<HdDriver> &drivers)
{
    HdDriverVector result;
    result.reserve(drivers.size());
    for (const HdDriver &driver : drivers) {
        result.push_back(const_cast<HdDriver*>(&driver));
    }
    return result;
}

HdRenderDelegateAdapterRenderer::HdRenderDelegateAdapterRenderer(
    HdPluginRenderDelegateUniqueHandle renderDelegate,
    HdSceneIndexBaseRefPtr const &terminalSceneIndex,
    HdContainerDataSourceHandle const &rendererCreateArgs)
 : _drivers(_ComputeDrivers(rendererCreateArgs))
 , _renderDelegate(std::move(renderDelegate))
 , _renderIndex(
     HdRenderIndex::New(
         _renderDelegate.Get(),
         _ToPointers(_drivers),
         terminalSceneIndex))
 , _engine(std::make_unique<HdEngine>())
 , _legacyRenderControl(
     std::make_unique<_LegacyRenderControl>(
         _renderDelegate.Get(),
         _renderIndex.get(),
         _engine.get()))
{
}

HdRenderDelegateAdapterRenderer::~HdRenderDelegateAdapterRenderer() = default;

HdLegacyRenderControlInterface *
HdRenderDelegateAdapterRenderer::GetLegacyRenderControl()
{
    return _legacyRenderControl.get();
}

PXR_NAMESPACE_CLOSE_SCOPE
