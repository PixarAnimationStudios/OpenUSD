//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/velocityMotionResolvingSceneIndexPlugin.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"

#include "pxr/pxr.h"

#include "pxr/imaging/hd/version.h"
#if HD_API_VERSION >= 106
#include "pxr/imaging/hdsi/velocityMotionResolvingSceneIndex.h"
#else
#include "hdPrman/pxr/imaging/hdsi/velocityMotionResolvingSceneIndex.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (fps)
    ((vblur, "ri:object:vblur"))
    ((vblur_enable, "Acceleration Blur"))
    ((vblur_ignore, "No Velocity Blur"))
    ((vblur_noAcceleration, "Velocity Blur"))
);

static float _fallbackFps = 24.f;

// -----------------------------------------------------------------------------

namespace {

class _VelocityMotionModeDataSource final
  : public HdTokenDataSource
{
public:
    HD_DECLARE_DATASOURCE(_VelocityMotionModeDataSource);

    _VelocityMotionModeDataSource(
        const HdContainerDataSourceHandle& primSource)
      : _primSource(primSource)
    { }

    bool
    GetContributingSampleTimesForInterval(
        Time /* startTime */,
        Time /* endTime */,
        std::vector<Time>* outSampleTimes) override
    {
        *outSampleTimes = { };
        return false;
    }

    TfToken
    GetTypedValue(Time shutterOffset) override
    {
        static const HdDataSourceLocator vblurLocator =
            HdPrimvarsSchema::GetDefaultLocator()
                .Append(_tokens->vblur)
                .Append(HdPrimvarSchemaTokens->primvarValue);
        if (const auto& vblurDs = HdSampledDataSource::Cast(
            HdContainerDataSource::Get(_primSource, vblurLocator))) {
            const TfToken& vblur = vblurDs->GetValue(shutterOffset)
                .GetWithDefault<TfToken>(_tokens->vblur_enable);
            if (vblur == _tokens->vblur_ignore) {
#if HD_API_VERSION >= 106 
                return HdsiVelocityMotionResolvingSceneIndexTokens->ignore;
#else
                return HdPrmanVelocityMotionResolvingSceneIndexTokens->ignore;
#endif
            }
            if (vblur == _tokens->vblur_noAcceleration) {
#if HD_API_VERSION >= 106 
                return HdsiVelocityMotionResolvingSceneIndexTokens->noAcceleration;
#else
                return HdPrmanVelocityMotionResolvingSceneIndexTokens->noAcceleration;
#endif
            }
        }
        static const auto modeLocator = HdDataSourceLocator(
#if HD_API_VERSION >= 106 
            HdsiVelocityMotionResolvingSceneIndexTokens->velocityMotionMode);
#else
            HdPrmanVelocityMotionResolvingSceneIndexTokens->velocityMotionMode);
#endif
        if (const auto modeDS = HdTokenDataSource::Cast(
            HdContainerDataSource::Get(_primSource, modeLocator))) {
            return modeDS->GetTypedValue(shutterOffset);
        }
#if HD_API_VERSION >= 106
        return HdsiVelocityMotionResolvingSceneIndexTokens->enable;
#else
        return HdPrmanVelocityMotionResolvingSceneIndexTokens->enable;
#endif
    }

    VtValue
    GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

private:
    HdContainerDataSourceHandle _primSource;
};

HD_DECLARE_DATASOURCE_HANDLES(_VelocityMotionModeDataSource);

TF_DECLARE_REF_PTRS(_VblurInterpretingSceneIndex);

class _VblurInterpretingSceneIndex
  : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _VblurInterpretingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _VblurInterpretingSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim
    GetPrim(const SdfPath& primPath) const override
    {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
#if HD_API_VERSION >= 106 
        if (HdsiVelocityMotionResolvingSceneIndex::
            PrimTypeSupportsVelocityMotion(prim.primType)) {
#else
        if (HdPrmanVelocityMotionResolvingSceneIndex::
            PrimTypeSupportsVelocityMotion(prim.primType)) {
#endif
#if PXR_VERSION < 2302
            std::vector<HdContainerDataSourceHandle> sources {
                HdRetainedContainerDataSource::New(
#if HD_API_VERSION >= 106 
                    HdsiVelocityMotionResolvingSceneIndexTokens->velocityMotionMode,
#else
                    HdPrmanVelocityMotionResolvingSceneIndexTokens->velocityMotionMode,
#endif
                    _VelocityMotionModeDataSource::New(prim.dataSource)),
                prim.dataSource };
            prim.dataSource = HdOverlayContainerDataSource::New(
                sources.size(), sources.data());
#else
            prim.dataSource = HdOverlayContainerDataSource::New(
                HdRetainedContainerDataSource::New(
#if HD_API_VERSION >= 106 
                    HdsiVelocityMotionResolvingSceneIndexTokens->velocityMotionMode,
#else
                    HdPrmanVelocityMotionResolvingSceneIndexTokens->velocityMotionMode,
#endif
                    _VelocityMotionModeDataSource::New(prim.dataSource)),
                prim.dataSource);
#endif
        }
        return prim;
    }

    SdfPathVector
    GetChildPrimPaths(const SdfPath& primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _VblurInterpretingSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    { }

    void
    _PrimsAdded(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override
    {
        _SendPrimsAdded(entries);
    }

    void
    _PrimsRemoved(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override
    {
        _SendPrimsRemoved(entries);
    }

    void
    _PrimsDirtied(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override
    {
        static const HdDataSourceLocator vblurLocator {
            HdPrimvarsSchema::GetDefaultLocator()
                .Append(_tokens->vblur) };
        static const HdDataSourceLocator modeLocator {
#if HD_API_VERSION >= 106
            HdsiVelocityMotionResolvingSceneIndexTokens->velocityMotionMode
#else
            HdPrmanVelocityMotionResolvingSceneIndexTokens->velocityMotionMode
#endif
        };

        HdSceneIndexObserver::DirtiedPrimEntries newEntries;
        for (const auto& entry : entries) {
            if (entry.dirtyLocators.Intersects(vblurLocator)) {
                newEntries.emplace_back(entry.primPath, modeLocator);
            }
        }
        if (newEntries.empty()) {
            return _SendPrimsDirtied(entries);
        }
        newEntries.insert(newEntries.begin(), entries.cbegin(), entries.cend());
        _SendPrimsDirtied(newEntries);
    }
};

} // anonymous namespace

// -----------------------------------------------------------------------------

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry
        ::Define<HdPrman_VelocityMotionResolvingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 2;

    for (const auto& rendererDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            rendererDisplayName,
            HdPrmanPluginTokens->velocityMotion,
            /* inputArgs = */ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
    }
}

/* static */
void
HdPrman_VelocityMotionResolvingSceneIndexPlugin::SetFPS(float fps)
{
    _fallbackFps = fps;
}

HdPrman_VelocityMotionResolvingSceneIndexPlugin::
HdPrman_VelocityMotionResolvingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_VelocityMotionResolvingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    TF_UNUSED(inputArgs);

    HdSceneIndexBaseRefPtr si = inputScene;
    si = _VblurInterpretingSceneIndex::New(si);

    // Define inputArgs here instead of in the TF_REGISTRY_FUNCTION block.
    // In the future, we may consider renaming the inputArgs parameter to
    // something like "sceneIndexGraphCreateArgs" to allow the app and renderer
    // plugin to provide arguments for scene indices instantiated via the
    // scene index plugin system.
    const HdContainerDataSourceHandle localInputArgs =
        HdRetainedContainerDataSource::New(
            // TODO: Get the real framerate!
            _tokens->fps,
            HdRetainedTypedSampledDataSource<float>::New(
                _fallbackFps));

#if HD_API_VERSION >= 106 
    si = HdsiVelocityMotionResolvingSceneIndex::New(si, localInputArgs);
#else
    si = HdPrmanVelocityMotionResolvingSceneIndex::New(si, localInputArgs);
#endif

    return si;
}

PXR_NAMESPACE_CLOSE_SCOPE
