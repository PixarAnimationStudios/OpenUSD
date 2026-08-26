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
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/rendererCreateArgsSchema.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (renderDriver)
);

namespace {

bool
_IsNamespaced(const TfToken &name)
{
    return name.GetString().find(':') != std::string::npos;
}

}

std::vector<HdDriver>
_ComputeDrivers(const HdRendererCreateArgsSchema &rendererCreateArgs)
{
    const HdSampledDataSourceContainerSchema driverSchema =
        rendererCreateArgs.GetDrivers();

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

    void RemoveTaskContextData(const TfToken &name) override {
        _engine->RemoveTaskContextData(name);
    }

    void ClearTaskContextData() override {
        _engine->ClearTaskContextData();
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

// Observes the terminal scene index for render settings and translates them
// into HdRenderDelegate::SetRenderSetting calls.
//
// More precisely, it uses the render settings from the active rendersettings
// prim pointed to by the scene globals.
//
// Note that only render settings advertised by
// HdRenderDelegate::GetRenderSettingDescriptors are observed.
//
class HdRenderDelegateAdapterRenderer::_RenderSettingsObserver final
    : public HdSceneIndexObserver
{
public:
    _RenderSettingsObserver(
        HdSceneIndexBaseRefPtr const &terminalSceneIndex,
        HdRenderDelegate * const renderDelegate)
      : _sceneIndex(terminalSceneIndex)
      , _renderDelegate(renderDelegate)
    {
        _activeRenderSettingsPath = _ComputeActiveRenderSettingsPath();

        _SetRenderSettings(HdDataSourceLocatorSet::UniversalSet());

        if (_sceneIndex) {
            _sceneIndex->AddObserver(HdSceneIndexObserverPtr(this));
        }
    }

    ~_RenderSettingsObserver() override
    {
        if (_sceneIndex) {
            _sceneIndex->RemoveObserver(HdSceneIndexObserverPtr(this));
        }
    }

    void PrimsAdded(
        const HdSceneIndexBase &,
        const AddedPrimEntries &entries) override
    {
        bool reapply = false;

        for (const AddedPrimEntry &entry : entries) {
            // The scene globals (root) prim may (re-)appear, changing which
            // prim is the active render settings prim; recompute the path.
            if (entry.primPath == HdSceneGlobalsSchema::GetDefaultPrimPath()) {
                _activeRenderSettingsPath =
                    _ComputeActiveRenderSettingsPath();
                reapply = true;
                break;
            }
            // The active render settings prim itself (re-)appeared.
            if (entry.primPath == _activeRenderSettingsPath) {
                reapply = true;
            }
        }
        if (!reapply) {
            return;
        }
        _SetRenderSettings(HdDataSourceLocatorSet::UniversalSet());
    }

    void PrimsRemoved(
        const HdSceneIndexBase &,
        const RemovedPrimEntries &entries) override
    {
        // Does not handle the case where scene globals prim gets removed.
        // But that should never happen.

        if (_activeRenderSettingsPath.IsEmpty()) {
            return;
        }

        for (const RemovedPrimEntry &entry : entries) {
            if (_activeRenderSettingsPath.HasPrefix(entry.primPath)) {
                _SetRenderSettings(HdDataSourceLocatorSet::UniversalSet());
                return;
            }
        }
    }

    void PrimsRenamed(
        const HdSceneIndexBase &,
        const RenamedPrimEntries &entries) override
    {
        // Does not handle the case where scene globals get removed.
        // But that should never happen.

        if (_activeRenderSettingsPath.IsEmpty()) {
            return;
        }

        for (const RenamedPrimEntry &entry : entries) {
            if (_activeRenderSettingsPath.HasPrefix(entry.oldPrimPath)) {
                _SetRenderSettings(HdDataSourceLocatorSet::UniversalSet());
                return;
            }
        }
    }

    void PrimsDirtied(
        const HdSceneIndexBase &,
        const DirtiedPrimEntries &entries) override
    {
        // (a) The active render settings prim path may have changed; if so,
        //     re-push the entire set.
        for (const DirtiedPrimEntry &entry : entries) {
            if (entry.primPath == HdSceneGlobalsSchema::GetDefaultPrimPath() &&
                entry.dirtyLocators.Intersects(
                    HdSceneGlobalsSchema::
                        GetActiveRenderSettingsPrimLocator())) {
                const SdfPath newPath = _ComputeActiveRenderSettingsPath();
                if (newPath != _activeRenderSettingsPath) {
                    _activeRenderSettingsPath = newPath;
                    _SetRenderSettings(HdDataSourceLocatorSet::UniversalSet());
                    return;
                }
            }
            if (entry.primPath == _activeRenderSettingsPath) {
                _SetRenderSettings(entry.dirtyLocators);
            }
        }
    }

private:
    // The path of the scene's active render settings prim (from scene globals),
    // or an empty path if there is none.
    SdfPath _ComputeActiveRenderSettingsPath() const
    {
        if (!_sceneIndex) {
            return {};
        }
        HdPathDataSourceHandle const ds =
            HdSceneGlobalsSchema::GetFromSceneIndex(_sceneIndex)
                .GetActiveRenderSettingsPrim();
        if (!ds) {
            return {};
        }
        return ds->GetTypedValue(0.0);
    }

    // The 'renderSettings' container of the active render settings prim, or
    // null.
    HdRenderSettingsSchema _GetActiveRenderSettingsSchema() const
    {
        if (!_sceneIndex || _activeRenderSettingsPath.IsEmpty()) {
            return {nullptr};
        }
        const HdSceneIndexPrim prim =
            _sceneIndex->GetPrim(_activeRenderSettingsPath);
        return HdRenderSettingsSchema::GetFromParent(prim.dataSource);
    }

    static
    HdDataSourceLocator _GetRenderSettingLocator(
        const TfToken &name)
    {
        if (_IsNamespaced(name)) {
            return HdRenderSettingsSchema::GetNamespacedSettingsLocator()
                .Append(name);
        } else {
            return HdRenderSettingsSchema::GetDefaultLocator()
                .Append(name);
        }
    }

    static
    HdSampledDataSourceHandle _GetRenderSettingDataSource(
        const TfToken &name,
        const HdRenderSettingsSchema &renderSettings)
    {
        if (_IsNamespaced(name)) {
            return renderSettings.GetNamespacedSettings().Get(name);
        } else {
            HdContainerDataSourceHandle const container =
                renderSettings.GetContainer();
            if (!container) {
                return nullptr;
            }
            return HdSampledDataSource::Cast(container->Get(name));
        }
    }

    // Pushes render settings that are known to the render delegate
    // and that correspond to data source locators in the given \p
    // locators.
    void _SetRenderSettings(const HdDataSourceLocatorSet &locators)
    {
        if (!locators.Intersects(
                HdRenderSettingsSchema::GetDefaultLocator())) {
            return;
        }

        const HdRenderSettingsSchema renderSettings =
            _GetActiveRenderSettingsSchema();

        // Only push render settings to the render delegate via the legacy
        // HdRenderDelegate::SetRenderSetting API if the active render settings
        // prim opts in via useForLegacyRenderDelegateSettings.
        HdBoolDataSourceHandle const useForLegacyDs =
            renderSettings.GetUseForLegacyRenderDelegateSettings();
        if (!(useForLegacyDs && useForLegacyDs->GetTypedValue(0.0f))) {
            return;
        }

        for (const HdRenderSettingDescriptor &descriptor :
                 _renderDelegate->GetRenderSettingDescriptors()) {
            if (!locators.Intersects(
                    _GetRenderSettingLocator(descriptor.key))) {
                continue;
            }
            HdSampledDataSourceHandle const ds =
                _GetRenderSettingDataSource(descriptor.key, renderSettings);
            _renderDelegate->SetRenderSetting(
                descriptor.key,
                ds ? ds->GetValue(0.0f) : descriptor.defaultValue);
        }
    }

    HdSceneIndexBaseRefPtr const _sceneIndex;
    HdRenderDelegate * const _renderDelegate;
    SdfPath _activeRenderSettingsPath;
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
    const HdRendererCreateArgsSchema &rendererCreateArgs)
 : _drivers(_ComputeDrivers(rendererCreateArgs))
 , _renderDelegate(std::move(renderDelegate))
 , _renderIndex(
     HdRenderIndex::NewForBackendEmulation(
         _renderDelegate.Get(),
         _ToPointers(_drivers),
         terminalSceneIndex))
 , _engine(std::make_unique<HdEngine>())
 , _legacyRenderControl(
     std::make_unique<_LegacyRenderControl>(
         _renderDelegate.Get(),
         _renderIndex.get(),
         _engine.get()))
 , _renderSettingsObserver(
     std::make_unique<_RenderSettingsObserver>(
         terminalSceneIndex,
         _renderDelegate.Get()))
{
}

HdRenderDelegateAdapterRenderer::HdRenderDelegateAdapterRenderer(
    HdPluginRenderDelegateUniqueHandle renderDelegate,
    HdSceneIndexBaseRefPtr const &terminalSceneIndex,
    HdContainerDataSourceHandle const &rendererCreateArgs)
 : HdRenderDelegateAdapterRenderer(
     std::move(renderDelegate),
     terminalSceneIndex,
     HdRendererCreateArgsSchema(rendererCreateArgs))
{
}

HdRenderDelegateAdapterRenderer::~HdRenderDelegateAdapterRenderer() = default;

HdLegacyRenderControlInterface *
HdRenderDelegateAdapterRenderer::GetLegacyRenderControl()
{
    return _legacyRenderControl.get();
}

PXR_NAMESPACE_CLOSE_SCOPE
