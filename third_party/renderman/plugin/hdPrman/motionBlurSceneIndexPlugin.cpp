//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/motionBlurSceneIndexPlugin.h"

#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/visitValue.h"

#include "pxr/pxr.h"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSceneIndexBase);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((mblur,                "ri:object:mblur"))
    ((geosamples,           "ri:object:geosamples"))
    ((xformsamples,         "ri:object:xformsamples"))
);

static const float   _defaultBlurScale            = 1.0f;
static const int     _defaultXformSamples         = -1;
static const int     _defaultGeoSamples           = -1;
static const bool    _defaultMblur                = true;

static const float _minimumShutterInterval = 1.0e-10;

// XXX: Set by HdPrman_MotionBlurSceneIndexPlugin::SetShutterInterval()
// and needed by _MotionBlurHelper. These are part of our shutter
// interval workaround. See comments on SetShutterInterval() at bottom of file.
static float _shutterOpen = HDPRMAN_SHUTTEROPEN_DEFAULT;
static float _shutterClose = HDPRMAN_SHUTTEROPEN_DEFAULT;

using TfTokenSet = std::unordered_set<TfToken, TfToken::HashFunctor>;

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_MotionBlurSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // This plug-in should be inserted *after* the extComp plug-in,
    // so that disabling of blur, etc. will also affect points from extComp.
    // It must also be *after* velocity motion resolving plug-in.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 3;

    for(const auto& pluginDisplayName : HdPrman_GetPluginDisplayNames()) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            HdPrmanPluginTokens->motionBlur,
            nullptr, /* inputArgs */
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

namespace
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _MotionBlurHelper
///
/// Helper base class for motion blur. This class carries the implementations
/// for methods to retrieve motion blur parameters from the prim, to compute
/// contributing time samples, and to sample the given sampled data source.
/// This class encapsulates all the logic for transform, velocity, and
/// deformation motion blur.
///
class _MotionBlurHelper
{
public:
    using Time = HdSampledDataSource::Time;

    /// samplesSource: the original data source
    /// key: identifying name for samplesSource
    /// primPath: path of sampleSource's parent prim (for diagnostics)
    /// primType: type of sampleSource's parent prim
    /// primvarsSource: data source for sampleSource's parent prim's primvars
    /// inputArgs: data source from scene index plugin
    _MotionBlurHelper(
        const HdSampledDataSourceHandle& samplesSource,
        const TfToken& key,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource)
      : _samplesSource(samplesSource)
      , _key(key)
      , _primPath(primPath)
      , _primType(primType)
      , _primvarsSource(primvarsSource)
    { }

protected:
    bool
    _GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time>* outSampleTimes);

    VtValue
    _GetValue(Time shutterOffset);

private:
    // Samples the datasource at the given locator relative to the parent prim's
    // primvars at the given shutter offset. Returns empty VtValue if not found.
    VtValue _GetPrimvarValue(
        const HdDataSourceLocator& locator,
        Time shutterOffset = 0.0f) const;

    // Samples the underlying data source at the given shutter offset.
    VtValue _GetSourceValue(Time shutterOffset) const;

    // Retrieves the value of ri:object:xformsamples or ri:object:geosamples
    // from the parent prim as appropriate for the kind of data source this is.
    int _GetRiMotionSampleCount() const;

    // Retrieves the value of ri:object:mblur from the parent prim.
    bool _GetMblur() const;

    // Retrieves the value of blurScale from the parent prim.
    float _GetBlurScale() const;

    // Returns true if this data source is for a transformable prim's transform.
    bool _IsTransform() const;

    // Returns true if this data source is for a primvar whose time samples
    // should contribute to motion blur. Used to ensure that other primvars
    // only express downstream a single time sample, even if they might have
    // multiple authored samples within the interval of interest.
    bool _IsBlurablePrimvar() const;

    HdSampledDataSourceHandle _samplesSource;
    TfToken _key;
    SdfPath _primPath;
    TfToken _primType;
    HdContainerDataSourceHandle _primvarsSource;
};

VtValue
_MotionBlurHelper::_GetPrimvarValue(
    const HdDataSourceLocator& locator,
    const Time shutterOffset) const
{
    if (const auto source = HdSampledDataSource::Cast(
        HdContainerDataSource::Get(_primvarsSource, locator))) {
        return source->GetValue(shutterOffset);
    }
    return VtValue();
}

VtValue
_MotionBlurHelper::_GetSourceValue(const Time shutterOffset) const
{
    if (!_samplesSource) {
        return VtValue();
    }
    return _samplesSource->GetValue(shutterOffset);
}

int
_MotionBlurHelper::_GetRiMotionSampleCount() const
{
    const TfToken name =
        _key == HdPrimvarsSchemaTokens->points
          ? _tokens->geosamples
          : _tokens->xformsamples;
    const HdDataSourceLocator locator = {
        name,
        HdPrimvarSchemaTokens->primvarValue };
    const VtValue value = _GetPrimvarValue(locator);
    if (value.IsHolding<VtArray<int>>() && value.GetArraySize() > 0) {
        return value.UncheckedGet<VtArray<int>>()[0];
    }
    return name == _tokens->geosamples
        ? _defaultGeoSamples
        : _defaultXformSamples;
}

float
_MotionBlurHelper::_GetBlurScale() const
{
    static const HdDataSourceLocator locator = {
        HdTokens->blurScale,
        HdPrimvarSchemaTokens->primvarValue };
    return _GetPrimvarValue(locator)
        .GetWithDefault<float>(_defaultBlurScale);
}

bool
_MotionBlurHelper::_GetMblur() const
{
    static const HdDataSourceLocator locator = {
        _tokens->mblur,
        HdPrimvarSchemaTokens->primvarValue };
    const VtValue value = _GetPrimvarValue(locator);
    if (value.IsHolding<VtArray<bool>>() && value.GetArraySize() > 0) {
        return value.UncheckedGet<VtArray<bool>>()[0];
    }
    return _defaultMblur;
}

bool
_MotionBlurHelper::_IsTransform() const
{
    return _key == HdXformSchemaTokens->matrix;
}

bool
_MotionBlurHelper::_IsBlurablePrimvar() const
{
    static const TfTokenSet blurables = {
        HdPrimvarsSchemaTokens->points
#if HD_API_VERSION < 67
        , HdInstancerTokens->translate
        , HdInstancerTokens->rotate
        , HdInstancerTokens->scale
        , HdInstancerTokens->instanceTransform
#endif
#if HD_API_VERSION >= 56
        , HdInstancerTokens->instanceTranslations
        , HdInstancerTokens->instanceRotations
        , HdInstancerTokens->instanceScales
        , HdInstancerTokens->instanceTransforms
#endif
    };
    return blurables.find(_key) != blurables.end();
}

bool
_MotionBlurHelper::_GetContributingSampleTimesForInterval(
    Time /*givenStartTime*/,   // XXX: We *ONLY* use the interval coming to
    Time /*givenEndTime*/,     //      us from RenderParam::SetRileyOptions()
    std::vector<Time>* const outSampleTimes)
{
    outSampleTimes->clear();

    // XXX: The following checks are roughly ordered from least to
    // most expensive.

    // Check the source isn't a null pointer
    if (!_samplesSource) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): no underlying source\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        return false;
    }

    // XXX: The next two checks are ordered this way for debug messaging
    // purposes. The first check could be completely subsumed by the second.

    // Check if shutterOpen and shutterClose are both 0. This is how
    // HdPrman_RenderSettings communicates that motion blur has been globally
    // disabled, either at the render product level (see
    // _ResolveShutterInterval() in renderSettings.cpp) or by environment
    // variable (see GetRileyOptionsFromEnvironment() in utils.cpp).
    if (0.0f == _shutterOpen && _shutterOpen == _shutterClose) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): motion blur globally disabled\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        return false;
    }

    // Check if motion blur is disabled by too small shutter interval
    if (_shutterClose - _shutterOpen < _minimumShutterInterval) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): interval [%f, %f] is too short\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText(),
            _shutterOpen, _shutterClose);
        return false;
    }

    // Check if this sample source is blurable
    if (!_IsTransform() && !_IsBlurablePrimvar()) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): not blurable\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        return false;
    }

    // Check if motion blur is disabled by ri:object:mblur = [0]
    if (!_GetMblur()) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): motion blur disabled by ri:object:mblur\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        return false;
    }

    // Check if motion blur is disabled by blurScale = 0.f
    float blurScale = _GetBlurScale();
    if (blurScale == 0.f) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): blurScale is 0\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        return false;
    }

    // Check if motion blur is disabled by ri:object:{geo|xform}samples < 2
    // XXX: We only care about this value if it's authored. We want to use
    // the number of samples suggested by the underlying source when this
    // has no authored value. We use -1 as a default to signal that there is
    // no authored value.
    int numSamples = _GetRiMotionSampleCount();
    if (numSamples > -1 && numSamples < 2) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): ri:object:%ssamples < 2\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText(),
            _key == HdPrimvarsSchemaTokens->points ? "geo" : "xform");
        return false;
    }

    float startTime = _shutterOpen * blurScale;
    float endTime = _shutterClose * blurScale;

    // Check that the underlying source has any motion to blur over the
    // scaled interval
    if (!_samplesSource->GetContributingSampleTimesForInterval(
        startTime, endTime, outSampleTimes)) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): underlying source has no motion\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        outSampleTimes->clear();
        return false;
    }

    // XXX: This should not be necessary but some implementations of
    // GetContributingSampleTimesForInterval() will return true with only a
    // single sample time.
    if (outSampleTimes->size() < 2) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): underlying source has <2 samples in interval\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        outSampleTimes->clear();
        return false;
    }

    // Check ordinality at source sample times
    const size_t count = _GetSourceValue(outSampleTimes->front()).GetArraySize();
    for (size_t i = 1; i < outSampleTimes->size(); ++i) {
        if (_GetSourceValue((*outSampleTimes)[i]).GetArraySize() != count) {
            TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
                "<%s.%s> (%s): source array size varies over interval\n",
                _primPath.GetText(), _key.GetText(), _primType.GetText());
            outSampleTimes->clear();
            return false;
        }
    }

    // If object:ri:{geo|xform}samples was unauthored, use source sample count
    if (numSamples == -1) {
        numSamples = int(outSampleTimes->size());
    }

    // Generate more or fewer sample times if requested. Note that asking for
    // a different number of sample times than reported by the underlying source
    // may move those times off of authored time samples and thereby fail to
    // accurately capture the underlying motion!
    if (int(outSampleTimes->size()) != numSamples) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg("<%s.%s> (%s) changing sample time "
            "count %zu -> %i; samples may no longer align with source!\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText(),
            outSampleTimes->size(), numSamples);
        startTime = outSampleTimes->front();
        endTime = outSampleTimes->back();
        outSampleTimes->clear();
        outSampleTimes->reserve(numSamples);
        const float m = float(numSamples) - 1.f;
        for (int i = 0; i < numSamples; ++i) {
            outSampleTimes->push_back(
                ((m - float(i)) / m) * startTime +
                (     float(i)  / m) * endTime);
        }
    }

    // Remove effect of blurScale
    if (blurScale != 1.f) {
        const float invBlurScale = 1.f / blurScale;
        for (Time& t : *outSampleTimes) {
            t *= invBlurScale;
        }
    }

    if (TfDebug::IsEnabled(HDPRMAN_MOTION_BLUR)) {
        std::string s;
        for (const float& t : *outSampleTimes) {
            if (!s.empty()) {
                s += ", ";
            }
            s += TfStringPrintf("%f", t);
        }
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s) motion blur sample times: [%s]\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText(),
            s.empty() ? " " : s.c_str());
    }

    return true;
}

VtValue
_MotionBlurHelper::_GetValue(Time shutterOffset)
{
    if (shutterOffset == 0.f) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s) GetValue(%f) -> GetSourceValue(%f)\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText(),
            shutterOffset, shutterOffset);
        return _GetSourceValue(shutterOffset);
    }

    float blurScale = _GetBlurScale();

    if (TfDebug::IsEnabled(HDPRMAN_MOTION_BLUR)) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s) GetValue(%f) -> GetSourceValue(%f)\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText(),
            shutterOffset, shutterOffset * blurScale);
    }
    return _GetSourceValue(shutterOffset * blurScale);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _MotionBlurUntypedSampledDataSource
///
/// Data source for untyped sampled sources
///
class _MotionBlurUntypedSampledDataSource final
  : public HdSampledDataSource
  , private _MotionBlurHelper
{
public:
    using Time = HdSampledDataSource::Time;

    HD_DECLARE_DATASOURCE(_MotionBlurUntypedSampledDataSource);

    /// samplesSource: the original data source
    /// key: identifying name for samplesSource
    /// primPath: path of sampleSource's parent prim (for diagnostics)
    /// primType: type of sampleSource's parent prim
    /// primvarsSource: data source for sampleSource's parent prim's primvars
    _MotionBlurUntypedSampledDataSource(
        const HdSampledDataSourceHandle& samplesSource,
        const TfToken& key,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource)
      : _MotionBlurHelper(
            samplesSource, key, primPath, primType, primvarsSource)
    { }

    VtValue GetValue(Time shutterOffset) override
    {
        return _GetValue(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        return _GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }
};

HD_DECLARE_DATASOURCE_HANDLES(_MotionBlurUntypedSampledDataSource);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _MotionBlurTypedSampledDataSource
///
/// Data source for typed sampled sources
///
template <typename T>
class _MotionBlurTypedSampledDataSource final
  : public HdTypedSampledDataSource<T>
  , private _MotionBlurHelper
{
public:
    using Time = HdSampledDataSource::Time;

    HD_DECLARE_DATASOURCE_ABSTRACT(_MotionBlurTypedSampledDataSource<T>);

    /// samplesSource: the original data source
    /// key: identifying name for samplesSource
    /// primPath: path of sampleSource's parent prim (for diagnostics)
    /// primType: type of sampleSource's parent prim
    /// primvarsSource: data source for sampleSource's parent prim's primvars
    _MotionBlurTypedSampledDataSource(
        const HdSampledDataSourceHandle& samplesSource,
        const TfToken& key,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource)
      : _MotionBlurHelper(
            samplesSource, key, primPath, primType, primvarsSource)
    { }

    VtValue GetValue(Time shutterOffset) override
    {
        return _GetValue(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        return _GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

    T GetTypedValue(Time shutterOffset) override
    {
        const VtValue& v = GetValue(shutterOffset);
        if (v.IsHolding<T>()) {
            return v.UncheckedGet<T>();
        }
        return T();
    }

    static typename _MotionBlurTypedSampledDataSource<T>::Handle New(
        const HdSampledDataSourceHandle& samplesSource,
        const TfToken& key,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource)
    {
        return _MotionBlurTypedSampledDataSource<T>::Handle(
            new _MotionBlurTypedSampledDataSource<T>(
                samplesSource, key, primPath,
                primType, primvarsSource));
    }
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PrimvarDataSource
///
/// Serves as data source for locator primvars>{name}
///
class _PrimvarDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarDataSource);

    /// primvarSource: original data source for locator primvars>{primvarName}
    /// primvarName: name of the primvar
    /// primPath: path of primvarSource's parent prim (for diagnostics)
    /// primType: type of primvarSource's parent prim
    /// primvarsSource: data source for primvarSource's parent prim's primvars
    _PrimvarDataSource(
        const HdContainerDataSourceHandle& primvarSource,
        const TfToken& primvarName,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource)
      : _primvarSource(primvarSource)
      , _primvarName(primvarName)
      , _primPath(primPath)
      , _primType(primType)
      , _primvarsSource(primvarsSource)
    { }

    TfTokenVector GetNames() override
    {
        if (!_primvarSource) {
            return {};
        }
        return _primvarSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

#if PXR_VERSION < 2301
    bool Has(const TfToken &name) override
    {
        const TfTokenVector names = GetNames();
        return std::find(names.begin(), names.end(), name) != names.end();
    }
#endif

private:
    HdContainerDataSourceHandle _primvarSource;
    TfToken _primvarName;
    SdfPath _primPath;
    TfToken _primType;
    HdContainerDataSourceHandle _primvarsSource;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarDataSource);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _Visitor
///
/// VtVisitValue visitor for constructing the right type of typed data source
///
struct _Visitor
{
    const HdSampledDataSourceHandle samplesSource;
    const TfToken key;
    const SdfPath primPath;
    const TfToken primType;
    const HdContainerDataSourceHandle primvarsSource;

    /// Handler for every type except the fallback VtValue type
    template <typename T>
    HdDataSourceBaseHandle
    operator()(const T&)
    {
        return _MotionBlurTypedSampledDataSource<T>::New(
            samplesSource, key, primPath, primType, primvarsSource);
    }

    /// Handler for the fallback VtValue type
    HdDataSourceBaseHandle
    operator()(const VtValue&)
    {
        return _MotionBlurUntypedSampledDataSource::New(
            samplesSource, key, primPath, primType, primvarsSource);
    }
};

HdDataSourceBaseHandle
_PrimvarDataSource::Get(const TfToken &name)
{
    if (!_primvarSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle result = _primvarSource->Get(name);

    if (name == HdPrimvarSchemaTokens->primvarValue) {
        if (const auto source = HdSampledDataSource::Cast(result)) {
            return VtVisitValue(source->GetValue(0.0f),
                _Visitor { source, _primvarName, _primPath,
                    _primType, _primvarsSource });
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PrimvarsDataSource
///
/// Serves as data source for locator primvars
///
class _PrimvarsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    /// primvarsSource: original data source for locator primvars
    /// primPath: path of primvarsSource's parent prim (for diagnostics)
    /// primType: type of primvarsSource's parent prim
    _PrimvarsDataSource(
        const HdContainerDataSourceHandle& primvarsSource,
        const SdfPath& primPath,
        const TfToken& primType)
      : _primvarsSource(primvarsSource)
      , _primPath(primPath)
      , _primType(primType)
    { }

    TfTokenVector GetNames() override
    {
        if (!_primvarsSource) {
            return {};
        }

        return _primvarsSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

#if PXR_VERSION < 2302
    bool Has(const TfToken &name) override
    {
        const TfTokenVector names = GetNames();
        return std::find(names.begin(), names.end(), name) != names.end();
    }
#endif

private:
    HdContainerDataSourceHandle _primvarsSource;
    SdfPath _primPath;
    TfToken _primType;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarsDataSource);

HdDataSourceBaseHandle
_PrimvarsDataSource::Get(const TfToken &name)
{
    if (!_primvarsSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle result = _primvarsSource->Get(name);

    // All primvars need to be handled, not just the blurable ones, because any
    // primvar might have authored time samples, and we need to make sure
    // that only a single time sample (at offset 0) makes it downstream when
    // the primvar is not blurable.
    if (const auto primvarSource = HdContainerDataSource::Cast(result)) {
        return _PrimvarDataSource::New(
            primvarSource,
            name,
            _primPath,
            _primType,
            _primvarsSource);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _XformDataSource
///
/// Serves as data source for locator xform
///
class _XformDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_XformDataSource);

    /// xformSource: the original data source for locator xform
    /// primPath: path of xformSource's parent prim (for diagnostics)
    /// primType: type of xformSource's parent prim
    /// primvarsSource: data source for xformSource's parent prim's primvars
    _XformDataSource(
        const HdContainerDataSourceHandle& xformSource,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource)
      : _xformSource(xformSource)
      , _primPath(primPath)
      , _primType(primType)
      , _primvarsSource(primvarsSource)
    { }

    TfTokenVector GetNames() override
    {
        if (!_xformSource) {
            return {};
        }
        return _xformSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override;

#if PXR_VERSION <= 2211
    bool Has(const TfToken &name) override
    {
        const TfTokenVector names = GetNames();
        return std::find(names.begin(), names.end(), name) != names.end();
    }
#endif

private:
    HdContainerDataSourceHandle _xformSource;
    SdfPath _primPath;
    TfToken _primType;
    HdContainerDataSourceHandle _primvarsSource;
};

HD_DECLARE_DATASOURCE_HANDLES(_XformDataSource);

HdDataSourceBaseHandle
_XformDataSource::Get(const TfToken& name)
{
    if (!_xformSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle result = _xformSource->Get(name);

    if (name == HdXformSchemaTokens->matrix) {
        if (const auto source = HdSampledDataSource::Cast(result)) {
            return VtVisitValue(source->GetValue(0.0f),
                _Visitor { source, name, _primPath,
                    _primType, _primvarsSource });
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PrimDataSource
///
/// Serves as data source for a prim.
///
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    /// primSource: original data source for prim
    /// primPath: path of prim (for diagnostics)
    /// primType: type of prim
    _PrimDataSource(
        const HdContainerDataSourceHandle& primSource,
        const SdfPath& primPath,
        const TfToken& primType)
      : _primSource(primSource)
      , _primPath(primPath)
      , _primType(primType)
    { }

    TfTokenVector GetNames() override
    {
        if (!_primSource) {
            return {};
        }

        return _primSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

#if PXR_VERSION < 2302
    bool Has(const TfToken &name) override
    {
        const TfTokenVector names = GetNames();
        return std::find(names.begin(), names.end(), name) != names.end();
    }
#endif

private:
    HdContainerDataSourceHandle _primSource;
    SdfPath _primPath;
    TfToken _primType;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimDataSource);

HdDataSourceBaseHandle
_PrimDataSource::Get(const TfToken &name)
{
    if (!_primSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle result = _primSource->Get(name);

    if (name == HdPrimvarsSchemaTokens->primvars) {
        if (const auto primvarsSource = HdContainerDataSource::Cast(result)) {
            return _PrimvarsDataSource::New(
                primvarsSource, _primPath, _primType);
        }
    }

    if (name == HdXformSchemaTokens->xform) {
        if (const auto xformSource = HdContainerDataSource::Cast(result)) {
            if (const auto primvarsSource = HdContainerDataSource::Cast(
                _primSource->Get(
#if PXR_VERSION < 2308
                    HdPrimvarsSchemaTokens->primvars
#else
                    HdPrimvarsSchema::GetSchemaToken()
#endif
                ))) {
                return _XformDataSource::New(
                    xformSource, _primPath, _primType, primvarsSource);
            }
        }
    }

    return result;
}

TF_DECLARE_REF_PTRS(_HdPrmanMotionBlurSceneIndex);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _HdPrmanMotionBlurSceneIndex
///
/// The scene index provided by the HdPrman_MotionBlurSceneIndexPlugin
///
class _HdPrmanMotionBlurSceneIndex final
  : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanMotionBlurSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr& inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanMotionBlurSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (_PrimIsBlurable(prim)) {
            return {
                prim.primType,
                _PrimDataSource::New(
                    prim.dataSource, primPath, prim.primType) };
        }
        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _HdPrmanMotionBlurSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    { }

    void _PrimsAdded(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase& /* sender */,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    static bool _PrimIsBlurable(const HdSceneIndexPrim& prim)
    {
        // Transformables, points-based, and instancers are blurable, but
        // points-based and instancers are always also transformable so
        // we only check for the xform schema.

        // XXX: renderSettings and integrator prim types currently leak
        // through a transformable check, so we also filter out those
        // specific prim types
#if PXR_VERSION >= 2208
        if (prim.primType == HdPrimTypeTokens->renderSettings
#if PXR_VERSION >= 2308
            || prim.primType == HdPrimTypeTokens->integrator
#endif
        ) {
            return false;
        }
#endif
        return bool(HdXformSchema::GetFromParent(prim.dataSource));
    };
};

HdDataSourceLocator
_GetPrimvarValueLocator(const TfToken &name)
{
    return HdDataSourceLocator(
        HdPrimvarsSchemaTokens->primvars,
        name,
        HdPrimvarSchemaTokens->primvarValue);
}

void
_HdPrmanMotionBlurSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    if (!_IsObserved()) {
        return;
    }

    // XXX: Invalidating all primvars is a bit heavy handed, but currently
    // hdPrman (mostly) refreshes all primvars when any primvar gets
    // invalidated anyway.

    static const auto primvarsLocator =
        HdDataSourceLocator(HdPrimvarsSchemaTokens->primvars);

    static const auto xformLocator =
        HdDataSourceLocator(HdXformSchemaTokens->xform);

    // If any of these changed, we invalidate all primvars
    static const HdDataSourceLocatorSet primvarsRelevantLocators {
        _GetPrimvarValueLocator(_tokens->mblur),
        _GetPrimvarValueLocator(_tokens->geosamples),
        _GetPrimvarValueLocator(_tokens->xformsamples),
        _GetPrimvarValueLocator(HdTokens->blurScale) };

    // If any of these changed, we invalidate the xform
    static const HdDataSourceLocatorSet xformRelevantLocators {
        _GetPrimvarValueLocator(_tokens->mblur),
        _GetPrimvarValueLocator(_tokens->xformsamples),
        _GetPrimvarValueLocator(HdTokens->blurScale) };

    std::vector<size_t> primvarIndices;
    std::vector<size_t> xformIndices;

    for (size_t i = 0; i < entries.size(); i++) {
        const HdDataSourceLocatorSet &locators = entries[i].dirtyLocators;
        if (locators.Intersects(primvarsRelevantLocators)) {
            if (!locators.Intersects(primvarsLocator)) {
                primvarIndices.push_back(i);
            }
        }
        if (locators.Intersects(xformRelevantLocators)) {
            if (!locators.Intersects(xformLocator)) {
                xformIndices.push_back(i);
            }
        }
    }

    if (primvarIndices.empty() && xformIndices.empty()) {
         _SendPrimsDirtied(entries);
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
    for (size_t i : primvarIndices) {
        newEntries[i].dirtyLocators.insert(primvarsLocator);
    }
    for (size_t i : xformIndices) {
        newEntries[i].dirtyLocators.insert(xformLocator);
    }

    _SendPrimsDirtied(newEntries);
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Implementation of HdPrman_MotionBlurSceneIndexPlugin.

HdPrman_MotionBlurSceneIndexPlugin::
HdPrman_MotionBlurSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_MotionBlurSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& /* inputArgs */)
{
    return _HdPrmanMotionBlurSceneIndex::New(inputScene);
}

// TODO: Query the camera's shutter interval in a better way!
// This method is called by the camera to update
// with its shutter interval, which is necessary
// when we're doing velocity blur and UsdImaging doesn't have the sample times.
void
HdPrman_MotionBlurSceneIndexPlugin::SetShutterInterval(
    float shutterOpen, float shutterClose)
{
    TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg("SetShutterInterval(%f, %f)\n",
        shutterOpen, shutterClose);
    _shutterOpen = shutterOpen;
    _shutterClose = shutterClose;
}

PXR_NAMESPACE_CLOSE_SCOPE
