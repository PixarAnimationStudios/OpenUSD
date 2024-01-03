//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "hdPrman/velocityMotionBlurSceneIndexPlugin.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (fps)
    ((sceneIndexPluginName, "HdPrman_VelocityMotionBlurSceneIndexPlugin"))
    ((mblur,                "ri:object:mblur"))
    ((vblur,                "ri:object:vblur"))
    ((vblur_on,             "Velocity Blur"))
    ((ablur_on,             "Acceleration Blur"))
    ((vblur_off,            "No Velocity Blur"))
    ((geosamples,           "ri:object:geosamples"))
    ((xformsamples,         "ri:object:xformsamples"))
);

static const int _defaultNonlinearSampleCount = 3;

// XXX: We need to encode the fps in the scene index (in a standard
// place). Note that fps is called timeCodesPerSecond in USD.
static const float _fps = 24.0f;

static const float _minimumShutterInterval = 1.0e-10;

static float _shutterOpen = 0.0f;
static float _shutterClose = 0.0f;


TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_VelocityMotionBlurSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // This plug-in should be inserted *after* the extComp plug-in,
    // so that disabling of blur, etc. will also affect points from extComp
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 3;

    HdContainerDataSourceHandle const inputArgs =
        HdRetainedContainerDataSource::New(
            _tokens->fps,
            HdRetainedSampledDataSource::New(VtValue(_fps)));

    for( auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames() ) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            _tokens->sceneIndexPluginName,
            inputArgs,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

namespace
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PrimvarValueDataSource
///
/// Serves as data source for locator primvars>points>primvarValue.
/// Adds the velocities at locator primvars>velocities>primvarValue
///
class _PrimvarValueDataSource final : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarValueDataSource);

    /// samplesSource: original source for locator primvars>points>primvarValue
    /// primvarsSource: original source for locator primvars
    /// inputArgs: source from scene index plugin
    _PrimvarValueDataSource(
        const HdSampledDataSourceHandle &samplesSource,
        const HdContainerDataSourceHandle &primvarsSource,
        const HdContainerDataSourceHandle &inputArgs,
        bool isPoints = true)
      : _samplesSource(samplesSource)
      , _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
      , _isPoints(isPoints)
    {
    }

    VtValue GetValue(Time givenShutterOffset) override;
    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> * outSampleTimes) override;

private: 
    VtValue _GetSourcePointsValue(const Time shutterOffset) const;
    VtValue _GetSourcePrimvarValue(const HdDataSourceLocator &locator) const;
    VtValue _GetSourceVelocitiesValue() const;
    VtValue _GetSourceAccelerationsValue() const;
    int _GetSourceNonlinearSampleCount() const;
    float _GetSourceBlurScale() const;
    bool _GetSourceEnableMotionBlur() const;
    bool _GetSourceEnableVelocityBlur() const;
    bool _GetSourceEnableAccelerationBlur() const;
    int _GetSourceGeoSamples() const;

    std::pair<Time, Time> _GetSamplingInterval(
        Time startTime,
        Time endTime) const;

    bool _HasVelocities() const;
    bool _HasAccelerations() const;

    bool _GetSamplesDeformBlur(
        Time startTime,
        Time endTime,
        float blurScale,
        std::vector<Time> * outSampleTimes);

    bool _GetSamplesVelocityBlur(
        Time givenStartTime,
        Time givenEndTime,
        std::vector<Time> * outSampleTimes);

    HdSampledDataSourceHandle _samplesSource;
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
    bool _isPoints;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarValueDataSource);

// Get fps from input arguments data source
float _GetFps(const HdContainerDataSourceHandle &inputArgs)
{
    if (!inputArgs) {
        return _fps;
    }

    HdSampledDataSourceHandle const source =
        HdSampledDataSource::Cast(inputArgs->Get(_tokens->fps));
    if (!source) {
        return _fps;
    }
    
    const VtValue &value = source->GetValue(0.0f);
    if (!value.IsHolding<float>()) {
        return _fps;
    }

    return value.UncheckedGet<float>();
}

VtValue
_PrimvarValueDataSource::_GetSourcePointsValue(const Time shutterOffset) const
{
    return _samplesSource->GetValue(shutterOffset);
}

VtValue
_PrimvarValueDataSource::_GetSourcePrimvarValue(
    const HdDataSourceLocator &locator) const
{
    HdSampledDataSourceHandle const source =
        HdSampledDataSource::Cast(
            HdContainerDataSource::Get(_primvarsSource, locator));
    if (!source) {
        return VtValue();
    }
    
    return source->GetValue(0.0f);
}

VtValue
_PrimvarValueDataSource::_GetSourceVelocitiesValue() const
{
    // Find velocities located on prim at primvars>velocities>primvarValue
    static const HdDataSourceLocator locator(
        HdTokens->velocities, HdPrimvarSchemaTokens->primvarValue);
    return _GetSourcePrimvarValue(locator);
}

VtValue
_PrimvarValueDataSource::_GetSourceAccelerationsValue() const
{
    // Find velocities located on prim at primvars>velocities>primvarValue
    static const HdDataSourceLocator locator(
        HdTokens->accelerations, HdPrimvarSchemaTokens->primvarValue);
    return _GetSourcePrimvarValue(locator);
}

int
_PrimvarValueDataSource::_GetSourceNonlinearSampleCount() const
{
    // Find count located on prim at
    // primvars>accelertionsSampleCount>primvarValue
    static const HdDataSourceLocator locator(
        HdTokens->nonlinearSampleCount, HdPrimvarSchemaTokens->primvarValue);
    const VtValue value = _GetSourcePrimvarValue(locator);
    if (!value.IsHolding<int>()) {
        // If count not available on prim, fall back to this other method
        return _GetSourceGeoSamples();
    }
    return value.GetWithDefault<int>(_defaultNonlinearSampleCount);
}

bool
_PrimvarValueDataSource::_GetSourceEnableMotionBlur() const
{
    bool blur = true;

    // When motion blur is disabled globally by the disableMotionBlur
    // render setting, _shutterOpen and _shutterClose are set to 0.
    // Need to notice that and return false here,
    // or else we'll end up sampling at shutterOpen time rather than time 0.
    if(0.0 == _shutterOpen && _shutterOpen == _shutterClose) {
        return false;
    }
    static const HdDataSourceLocator locator(
        _tokens->mblur, HdPrimvarSchemaTokens->primvarValue);
    const VtValue value = _GetSourcePrimvarValue(locator);
    if (value.IsHolding<VtArray<bool>>()) {
        blur = value.UncheckedGet<VtArray<bool>>()[0];
    }
    return blur;
}

bool
_PrimvarValueDataSource::_GetSourceEnableVelocityBlur() const
{
    // Assume velocity blur is desired when velcocities are present
    // unless explicitly disabled
    bool vblur = true;
    static const HdDataSourceLocator locator(
        _tokens->vblur, HdPrimvarSchemaTokens->primvarValue);
    const VtValue value = _GetSourcePrimvarValue(locator);
    if(value == _tokens->vblur_off) {
        vblur = false;
    }

    return vblur;
}

bool
_PrimvarValueDataSource::_GetSourceEnableAccelerationBlur() const
{
    // Assume acceleration blur is desired when accelerations are present
    // unless explicitly disabled
    bool ablur = true;
    static const HdDataSourceLocator locator(
        _tokens->vblur, HdPrimvarSchemaTokens->primvarValue);
    const VtValue value = _GetSourcePrimvarValue(locator);
    if(value == _tokens->vblur_off ||
       value == _tokens->vblur_on) {
        ablur = false;
    }

    return ablur;
}

int
_PrimvarValueDataSource::_GetSourceGeoSamples() const
{
    int sampleCount = _defaultNonlinearSampleCount;
    static const HdDataSourceLocator locator(
        _tokens->geosamples, HdPrimvarSchemaTokens->primvarValue);
    const VtValue value = _GetSourcePrimvarValue(locator);
    if (value.IsHolding<VtArray<int>>()) {
        sampleCount = value.UncheckedGet<VtArray<int>>()[0];
    }
    return sampleCount;
}

float
_PrimvarValueDataSource::_GetSourceBlurScale() const
{
    // Find velocities located on prim at primvars>blurScale>primvarValue
    static const HdDataSourceLocator locator(
        HdTokens->blurScale, HdPrimvarSchemaTokens->primvarValue);
    const VtValue value = _GetSourcePrimvarValue(locator);

    // No blur if motion blur is disbled
    if (!_GetSourceEnableMotionBlur()) {
        return 0.0f;
    }

    return std::fabs(value.GetWithDefault<float>(1.0f));
}

bool
_PrimvarValueDataSource::_HasVelocities() const
{
    // Allow for velocity blur to be disabled even when velocities are present
    if (!_GetSourceEnableVelocityBlur()) {
        return false;
    }

    const VtValue v = _GetSourceVelocitiesValue();
    if (!v.IsHolding<VtVec3fArray>()) {
        return false;
    }

    return !v.UncheckedGet<VtVec3fArray>().empty();
}

bool
_PrimvarValueDataSource::_HasAccelerations() const
{
    // Allow for acceleration blur to be disabled even if accel vals are present
    if (!_GetSourceEnableAccelerationBlur()) {
        return false;
    }

    const VtValue v = _GetSourceAccelerationsValue();
    if (!v.IsHolding<VtVec3fArray>()) {
        return false;
    }

    return !v.UncheckedGet<VtVec3fArray>().empty();
}

VtValue
_PrimvarValueDataSource::GetValue(const Time givenShutterOffset)
{
    if (!_samplesSource) {
        return VtValue();
    }

    // No math to do at time zero.
    if (givenShutterOffset == 0.0f) {
        return _GetSourcePointsValue(0.0f);
    }

    const float blurScale = _GetSourceBlurScale();
    if (blurScale == 0.0f) {
        // Motion blur disabled, always return at time zero.
        return _GetSourcePointsValue(0.0f);
    }

    const Time shutterOffset = givenShutterOffset * blurScale;

    if (!_HasVelocities()) {
        // Velocities are either not present or are disabled
        return _GetSourcePointsValue(shutterOffset);
    }

    // Check that we have velocities matching the number of points.
    //
    // If this is not the case, simply use the points value from the source.
    //
    const VtValue velocitiesValue = _GetSourceVelocitiesValue();
    if (!velocitiesValue.IsHolding<VtVec3fArray>()) {
        return _GetSourcePointsValue(shutterOffset);
    }

    const VtVec3fArray &velocitiesArray =
        velocitiesValue.UncheckedGet<VtVec3fArray>();
    if (velocitiesArray.empty()) {
        return _GetSourcePointsValue(shutterOffset);
    }

    const VtValue pointsValues = _GetSourcePointsValue(0.0f);
    if (!pointsValues.IsHolding<VtVec3fArray>() || !_isPoints) {
        // Velocities are enabled, but we're deling with some other primvar,
        // which needs to be sampled at the same time as points, ie. time 0.0
        return _GetSourcePointsValue(0.0f);
    }

    const VtVec3fArray &pointsArray = pointsValues.UncheckedGet<VtVec3fArray>();

    const size_t num = pointsArray.size();
    if (velocitiesArray.size() != num) {
        TF_WARN("Number %zu of velocity vectors does not match number %zu "
                "of points.", velocitiesArray.size(), num);

        if(velocitiesArray.size() < num) {
            // Be slightly forgiving - return values as long as there are enough
            return _GetSourcePointsValue(shutterOffset);
        }
    }
    
    // We have valid velocities, now alsocheck for valid acclerations before
    // applying them.

    static const VtVec3fArray emptyArray;
    const VtValue accelerationsValue = _GetSourceAccelerationsValue();
    const VtVec3fArray &accelerationsArray =
        accelerationsValue.IsHolding<VtVec3fArray>()
        ? accelerationsValue.UncheckedGet<VtVec3fArray>()
        : emptyArray;

    // This is also false if acclerationsArray is empty (the case where
    // points is empty doesn't matter).
    const bool useAccelerations = (accelerationsArray.size() == num);

    if (!accelerationsArray.empty()) {
        // If we have acclerations, ...
        if (!useAccelerations) {
            // but it is the wrong number.
            TF_WARN("Number %zu of accleration vectors does not match "
                    "number %zu of points - applying velocities but not "
                    "accelerations.", accelerationsArray.size(), num);
        }
    }

    const float fps = _GetFps(_inputArgs);
    const float time = shutterOffset / fps;
    
    // Apply velocities and acclerations to points
    VtVec3fArray result(num);

    if (useAccelerations) {
        const float timeSqrHalf = 0.5f * time * time;
        for (size_t i = 0; i < num; i++) {
            result[i] +=
                pointsArray[i]
                + time * velocitiesArray[i]
                + timeSqrHalf * accelerationsArray[i];
        }
    } else {
        for (size_t i = 0; i < num; i++) {
            result[i] +=
                pointsArray[i]
                + time * velocitiesArray[i];
        }
    }
    return VtValue(result);
}

// Unfortunately, the scene emulation always calls
// GetContributingSampleTimesForInterval with startTime and endTime
// being the smallest and largest finite floating point number.
//
// We rely on the UsdImaging knowing the relevant camera and its
// shutter interval and returning a sample time for the beginning and
// end of the shutter interval.
//
std::pair<HdSampledDataSource::Time, HdSampledDataSource::Time>
_PrimvarValueDataSource::_GetSamplingInterval(
    const Time startTime, const Time endTime) const
{
    if (std::numeric_limits<Time>::lowest() < startTime &&
        endTime < std::numeric_limits<Time>::max()) {
        // Client gives us a valid shutter interval. Use it.
        return { startTime, endTime };
    }

    // Do the shutter interval reconstruction described above.

    std::vector<Time> sampleTimes;
    // Ignore return value - just examine sampleTimes instead
    _samplesSource->GetContributingSampleTimesForInterval(
        startTime, endTime, &sampleTimes);

    // Not enough samples to reconstruct the shutter interval.
    if (sampleTimes.size() < 2) {
        // These fallback values are from the camera
        return {_shutterOpen, _shutterClose};
    }

    const auto iteratorPair =
        std::minmax_element(sampleTimes.begin(),
                            sampleTimes.end());
    return { *iteratorPair.first, *iteratorPair.second };
}

bool
_PrimvarValueDataSource::GetContributingSampleTimesForInterval(
    const Time startTime,
    const Time endTime,
    std::vector<Time> * const outSampleTimes)
{
    if (!_samplesSource) {
        return false;
    }

    const float blurScale = _GetSourceBlurScale();
    if (blurScale == 0.0f) {
        // Motion blur disabled, return false to indicate that
        // this is constant across shutter interval.
        *outSampleTimes = { 0.0f };
        return false;
    }

    if (_HasVelocities()) {
        // Velocities are given, forward call to source, applying
        // blurScale if non-trivial.
        if (_isPoints) {
            return _GetSamplesVelocityBlur(
                startTime, endTime, outSampleTimes);
        } else {
            // primvars other than points can't be blurred; sample at 0.0
            *outSampleTimes = { 0.0f };
            return false;
        }
    } else {
        bool result = _GetSamplesDeformBlur(
            startTime, endTime, blurScale, outSampleTimes);
        if (outSampleTimes->size() > 1) {
            std::vector<size_t> ptArraySizes;
            std::transform (outSampleTimes->begin(), outSampleTimes->end(),
                            std::back_inserter(ptArraySizes),
                            [&](float t) {return _GetSourcePointsValue(t).GetArraySize();});
            if (!std::equal(ptArraySizes.begin() + 1,
                            ptArraySizes.end(),
                            ptArraySizes.begin())) {
                // The lengths of point arrays for each sample do not match
                // so disable deforming blur.
                *outSampleTimes = { 0.0f };
                result = false;
            } else if(!_isPoints) {
                // For primvars other than points, that can't be blurred,
                // sample from the middle of the time range.
                *outSampleTimes =
                    { _shutterOpen + (_shutterClose - _shutterOpen) * 0.5f };
                result = false;
            }
        }
        return result;
    }
}
    
bool
_PrimvarValueDataSource::_GetSamplesDeformBlur(
    const Time startTime,
    const Time endTime,
    const float blurScale,
    std::vector<Time> * const outSampleTimes)
{
    // Blur scale is trivial, just forward to source.
    if (blurScale == 1.0f) {
        return
            _samplesSource->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

    // Can't do anything if given a meaningless shutter interval.
    if (!(std::numeric_limits<Time>::lowest() < startTime &&
          endTime < std::numeric_limits<Time>::max())) {
        static std::once_flag flag;
        std::call_once(flag, [](){
                TF_CODING_ERROR(
                    "blurScale is not supported when consumer is not "
                    "specifying interval for contributing sample times. "
                    "In particular, blurScale is not supported by the "
                    "scene index emulation.");
            });
        return
            _samplesSource->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
    }

    // Scale shutter interval
    if (!_samplesSource->GetContributingSampleTimesForInterval(
            blurScale * startTime,
            blurScale * endTime,
            outSampleTimes)) {
        return false;
    }
    
    // Scale time samples to fit into original shutter interval.
    //
    // _GetSamplesDeformBlur is never called with blurScale = 0.0.
    //
    const float invBlurScale = 1.0f / blurScale;
    for (Time & time : *outSampleTimes) {
        time *= invBlurScale;
    }
    
    return true;
}

bool
_PrimvarValueDataSource::_GetSamplesVelocityBlur(
    Time givenStartTime,
    Time givenEndTime,
    std::vector<Time> * outSampleTimes)
{
    // No need to take blurScale into account here.
    //
    // We apply blurScale to time in GetValue instead.

    // We have velocities!
    Time startTime, endTime;
    std::tie(startTime, endTime) = _GetSamplingInterval(
                 givenStartTime, givenEndTime);

    if (endTime - startTime < _minimumShutterInterval) {
        // Only return one time if shutter interval is tiny.
        *outSampleTimes = { startTime };
        return true;
    }

    if (!_HasAccelerations()) {
        // Velocity motion blur - linear motion described perfectly
        // by just two samples.
        *outSampleTimes = { startTime, endTime };
        return true;
    }

    const size_t nonlinearSampleCount =
        _GetSourceNonlinearSampleCount();

    if (nonlinearSampleCount < 2) {
        // Degenerate case (e.g. only one sample).
        //
        // Catch to avoid division by zero below.
        //

        *outSampleTimes = { 0.0f };

        // Just disable motion blur.
        return false;
    }

    const float m(nonlinearSampleCount - 1);

    outSampleTimes->reserve(nonlinearSampleCount);
    for (size_t i = 0; i < nonlinearSampleCount; ++i) {
        // Do floating point operations in such a way that
        // we get startTime and endTime on the nose for the first
        // and last value.
        outSampleTimes->push_back(
            (float(m - i) / m) * startTime +
            (float(i    ) / m) * endTime);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PrimvarDataSource
///
/// Serves as data source for locator primvars>points
///
class _PrimvarDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarDataSource);

    /// primvarSource: original source for locator primvars>points
    /// primvarsSource: original source for locator primvars
    /// inputArgs: source from scene index plugin
    _PrimvarDataSource(
        const HdContainerDataSourceHandle &primvarSource,
        const HdContainerDataSourceHandle &primvarsSource,
        const HdContainerDataSourceHandle &inputArgs,
        bool isPoints)
      : _primvarSource(primvarSource)
      , _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
      , _isPoints(isPoints)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_primvarSource) {
            return {};
        }
        
        return _primvarSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;
    
private:
    HdContainerDataSourceHandle _primvarSource;
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
    bool                        _isPoints;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarDataSource);

HdDataSourceBaseHandle
_PrimvarDataSource::Get(const TfToken &name)
{
    if (!_primvarSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle const result = _primvarSource->Get(name);

    if (name == HdPrimvarSchemaTokens->primvarValue) {
        // Use our own data source for primvars>points>primvarValue
        if (HdSampledDataSourceHandle const primvarValueSource =
                HdSampledDataSource::Cast(result)) {
            return _PrimvarValueDataSource::New(
                primvarValueSource, _primvarsSource, _inputArgs, _isPoints);
        }
    }

    return _primvarSource->Get(name);
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

    /// primvarsSource: original source for locator primvars
    /// inputArgs: source from scene index plugin
    _PrimvarsDataSource(
        const HdContainerDataSourceHandle &primvarsSource,
        const HdContainerDataSourceHandle &inputArgs)
      : _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_primvarsSource) {
            return {};
        }

        return _primvarsSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarsDataSource);

HdDataSourceBaseHandle
_PrimvarsDataSource::Get(const TfToken &name)
{
    if (!_primvarsSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle const result = _primvarsSource->Get(name);

    // All primvars need to be handled, not just points, because their
    // sample times are based on when the points end up being sampled,
    // which depends on various things, like whether velocities are present,
    // whether motion blur is enabled for the object, etc.
    if (HdContainerDataSourceHandle const primvarSource =
        HdContainerDataSource::Cast(result)) {
        return _PrimvarDataSource::New(
            primvarSource, _primvarsSource, _inputArgs,
            // Are there more blurrable (by rman) primvars than these?
            name == HdPrimvarsSchemaTokens->points ||
            name == HdInstancerTokens->instanceTransform
#if HD_API_VERSION >= 56
            || name == HdInstancerTokens->instanceTransforms
#endif
            );
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

    // primSource: original source for prim
    // inputArgs: source from scene index plugin
    _PrimDataSource(
        const HdContainerDataSourceHandle &primSource,
        const HdContainerDataSourceHandle &inputArgs)
      : _primSource(primSource)
      , _inputArgs(inputArgs)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_primSource) {
            return {};
        }
        
        return _primSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    HdContainerDataSourceHandle _primSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimDataSource);

HdDataSourceBaseHandle
_PrimDataSource::Get(const TfToken &name)
{
    if (!_primSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle const result = _primSource->Get(name);
    
    // Use our own data source for primvars
    if (name == HdPrimvarsSchemaTokens->primvars) {
        if (HdContainerDataSourceHandle const primvarsSource =
                HdContainerDataSource::Cast(result)) {
            return _PrimvarsDataSource::New(primvarsSource, _inputArgs);
        }
    }

    return result;
}

TF_DECLARE_REF_PTRS(_SceneIndex);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _SceneIndex
///
/// The scene index provided by the HdPrman_VelocityMotionBlurSceneIndexPlugin
///
class _SceneIndex final : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _SceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs)
    {
        return TfCreateRefPtr(
            new _SceneIndex(
                inputSceneIndex, inputArgs));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        return { prim.primType,
                 _PrimDataSource::New(
                     prim.dataSource, _inputArgs) };
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _SceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
      , _inputArgs(inputArgs)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }
        
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }
        
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    HdContainerDataSourceHandle _inputArgs;
};

HdDataSourceLocator
_GetPrimvarValueLocator(const TfToken &name)
{
    return HdDataSourceLocator(HdPrimvarsSchemaTokens->primvars,
                               name,
                               HdPrimvarSchemaTokens->primvarValue);
}

void
_SceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    static const HdDataSourceLocator pointsValueLocator =
        _GetPrimvarValueLocator(HdTokens->points);

    static const HdDataSourceLocatorSet relevantLocators{
        _GetPrimvarValueLocator(HdTokens->velocities),
        _GetPrimvarValueLocator(HdTokens->accelerations),
        _GetPrimvarValueLocator(HdTokens->nonlinearSampleCount),
        _GetPrimvarValueLocator(HdTokens->blurScale)};

    std::vector<size_t> indices;

    for (size_t i = 0; i < entries.size(); i++) {
        const HdDataSourceLocatorSet &locators = entries[i].dirtyLocators;
        if (locators.Intersects(relevantLocators)) {
            if (!locators.Intersects(pointsValueLocator)) {
                indices.push_back(i);
            }
        }
    }
    
    if (indices.empty()) {
         _SendPrimsDirtied(entries);
        return;
    }
    
    HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
    for (size_t i : indices) {
        newEntries[i].dirtyLocators.insert(pointsValueLocator);
    }

    _SendPrimsDirtied(newEntries);
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Implementation of HdPrman_VelocityBlurSceneIndexPlugin.

HdPrman_VelocityMotionBlurSceneIndexPlugin::
HdPrman_VelocityMotionBlurSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_VelocityMotionBlurSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _SceneIndex::New(inputScene, inputArgs);
}

// TODO: Query the camera's shutter interval in a better way!
// This method is called by the camera to update
// with its shutter interval, which is necessary
// when we're doing velocity blur and UsdImaging doesn't have the sample times.
void
HdPrman_VelocityMotionBlurSceneIndexPlugin::SetShutterInterval(
    float shutterOpen, float shutterClose)
{
    _shutterOpen = shutterOpen;
    _shutterClose = shutterClose;
}

PXR_NAMESPACE_CLOSE_SCOPE
