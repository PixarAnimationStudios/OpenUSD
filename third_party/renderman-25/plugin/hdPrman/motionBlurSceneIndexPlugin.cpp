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

#include "hdPrman/motionBlurSceneIndexPlugin.h"

#include "hdPrman/debugCodes.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/xformSchema.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/visitValue.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (fps)
    ((mblur,                "ri:object:mblur"))
    ((vblur,                "ri:object:vblur"))
    ((vblur_on,             "Velocity Blur"))
    ((ablur_on,             "Acceleration Blur"))
    ((vblur_off,            "No Velocity Blur"))
    ((geosamples,           "ri:object:geosamples"))
    ((xformsamples,         "ri:object:xformsamples"))
    (angularVelocities) // XXX: Why is this not in HdTokens?
);

// XXX: These defaults are pulled from UsdMotionAPI, for which there is not yet
// a corresponding Hydra schema. 
static const int     _defaultNonlinearSampleCount = 3;
static const float   _defaultBlurScale            = 1.0f;
// There is no canonical source for these defaults. They were previously hard-
// coded in renderParam.cpp.
static const int     _defaultXformSamples         = 2;
static const int     _defaultGeoSamples           = 2;
static const bool    _defaultMblur                = true;
static const TfToken _defaultVblur                = _tokens->ablur_on;

// XXX: We need to encode the fps in the scene index (in a standard
// place). Note that fps is called timeCodesPerSecond in USD.
static const float _fps = 24.0f;

static const float _minimumShutterInterval = 1.0e-10;

// XXX: Set by HdPrman_MotionBlurSceneIndexPlugin::SetShutterInterval()
// and needed by _MotionBlurHelper. These are part of our shutter
// interval workaround. See comments on SetShutterInterval() at bottom of file.
static float _shutterOpen = 0.0f;
static float _shutterClose = 0.0f;

using TfTokenSet = std::unordered_set<TfToken, TfToken::HashFunctor>;

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdPrman_MotionBlurSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // This plug-in should be inserted *after* the extComp plug-in,
    // so that disabling of blur, etc. will also affect points from extComp
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 3;

    const HdContainerDataSourceHandle inputArgs =
        HdRetainedContainerDataSource::New(
            _tokens->fps,
            HdRetainedSampledDataSource::New(VtValue(_fps)));

    for( auto const& pluginDisplayName : HdPrman_GetPluginDisplayNames() ) {
        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            pluginDisplayName,
            HdPrmanPluginTokens->motionBlur,
            inputArgs,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart);
    }
}

namespace
{

// Get fps from input arguments data source
float _GetFps(const HdContainerDataSourceHandle& inputArgs)
{
    if (!inputArgs) {
        return _fps;
    }

    const auto source = HdSampledDataSource::Cast(inputArgs->Get(_tokens->fps));
    if (!source) {
        return _fps;
    }
    
    const VtValue &value = source->GetValue(0.0f);
    if (!value.IsHolding<float>()) {
        return _fps;
    }

    return value.UncheckedGet<float>();
}

// Unfortunately, when encountering a legacy prim, the scene index emulation
// calls GetContributingSampleTimesForInterval with startTime and endTime
// being the smallest and largest finite floating point number. It does this
// because it cannot query the scene delegate itself.
//
// We rely on the UsdImaging knowing the relevant camera and its
// shutter interval and returning a sample time for the beginning and
// end of the shutter interval.
std::pair<HdSampledDataSource::Time, HdSampledDataSource::Time>
_GetSamplingInterval(
    const HdSampledDataSourceHandle& samplesSource,
    const float shutterOpen, const float shutterClose,
    const HdSampledDataSource::Time startTime,
    const HdSampledDataSource::Time endTime)
{
    if (std::numeric_limits<HdSampledDataSource::Time>::lowest() < startTime &&
        endTime < std::numeric_limits<HdSampledDataSource::Time>::max()) {
        // Client gives us a valid shutter interval. Use it.
        return { startTime, endTime };
    }

    // Do the shutter interval reconstruction described above.

    std::vector<HdSampledDataSource::Time> sampleTimes;
    // Ignore return value - just examine sampleTimes instead
    samplesSource->GetContributingSampleTimesForInterval(
        startTime, endTime, &sampleTimes);

    // Not enough samples to reconstruct the shutter interval.
    if (sampleTimes.size() < 2) {
        // These fallback values are from the camera
        return { shutterOpen, shutterClose };
    }

    const auto iteratorPair =
        std::minmax_element(sampleTimes.cbegin(),
                            sampleTimes.cend());
    return { *iteratorPair.first, *iteratorPair.second };
}

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
        const HdContainerDataSourceHandle& primvarsSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _samplesSource(samplesSource)
      , _key(key)
      , _primPath(primPath)
      , _primType(primType)
      , _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
    { }

protected:
    bool
    _GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time>* const outSampleTimes);
    
    VtValue
    _GetValue(Time shutterOffset);

private:
    // Samples the datasource at the given locator relative to the parent prim's
    // primvars at the given shutter offset. Returns empty VtValue if not found.
    VtValue _GetPrimvarValue(
        const HdDataSourceLocator& locator,
        const Time shutterOffset = 0.0f) const;

    // Samples the underlying data source at the given shutter offset.
    VtValue _GetSourceValue(const Time shutterOffset) const;

    // Retrieves the value of ri:object:xformsamples or ri:object:geosamples
    // from the parent prim as appropriate for the kind of data source this is.
    int _GetLinearSampleCount() const;

    // Retrieves the value of nonlinearSampleCount from the parent prim.
    int _GetNonlinearSampleCount() const;

    // Retrieves the value of blurScale from the parent prim.
    float _GetBlurScale() const;

    // Retrieves the value of ri:object:mblur from the parent prim.
    bool _GetMblur() const;
    
    // Retrieves the value of ri:object:vblur from the parent prim.
    TfToken _GetVblur() const;

    // Retrieves the value of velocities (or angularVelocities if the parent
    // prim is an instancer) from the parent prim.
    VtValue _GetVelocities() const;

    // Retrieves the value of accelerations from the parent prim.
    VtValue _GetAccelerations() const;

    // Checks whether the parent prim has non-empty velocities (or 
    // angularVelocities if the parent prim is an instancer) of the
    // correct type.
    bool _HasVelocities() const;

    // Checks whether the parent prim has non-empty accelerations of the
    // correct type.
    bool _HasAccelerations() const;

    // Returns true if this data source is for instance rotations.
    bool _IsRotations() const;

    // Returns true if this data source is for a transformable prim's transform.
    bool _IsTransform() const;

    // Returns true if this data source is for a primvar whose time samples
    // should contribute to motion blur. Used to ensure that other primvars
    // only express downstream a single time sample, even if they might have
    // multiple authored samples within the interval of interest.
    bool _IsBlurablePrimvar() const;

    // Returns true if this data source is for a primvar that can undergo
    // velocity motion blur. Currently this is only points on points-based
    // prims and positions & rotations on point instancers.
    bool _IsVelocityBlurablePrimvar() const;
    
    HdSampledDataSourceHandle _samplesSource;
    TfToken _key;
    SdfPath _primPath;
    TfToken _primType;
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
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
_MotionBlurHelper::_GetLinearSampleCount() const
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

int
_MotionBlurHelper::_GetNonlinearSampleCount() const
{
    static const HdDataSourceLocator locator = {
        HdTokens->nonlinearSampleCount,
        HdPrimvarSchemaTokens->primvarValue };
    return _GetPrimvarValue(locator)
        .GetWithDefault<int>(_defaultNonlinearSampleCount);
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

TfToken
_MotionBlurHelper::_GetVblur() const
{
    static const HdDataSourceLocator locator = {
        _tokens->vblur,
        HdPrimvarSchemaTokens->primvarValue };
    return _GetPrimvarValue(locator).GetWithDefault<TfToken>(_defaultVblur);
}

VtValue
_MotionBlurHelper::_GetVelocities() const
{
    
    if (_IsRotations()) {
        static const HdDataSourceLocator locator = {
            _tokens->angularVelocities,
            HdPrimvarSchemaTokens->primvarValue };
        return _GetPrimvarValue(locator);
    } else {
        static const HdDataSourceLocator locator = {
            HdTokens->velocities,
            HdPrimvarSchemaTokens->primvarValue };
        return _GetPrimvarValue(locator);
    }
}

VtValue
_MotionBlurHelper::_GetAccelerations() const
{
    static const HdDataSourceLocator locator = {
        HdTokens->accelerations,
        HdPrimvarSchemaTokens->primvarValue };
    return _GetPrimvarValue(locator);
}

bool
_MotionBlurHelper::_HasVelocities() const
{
    const VtValue& value = _GetVelocities();
    return value.IsHolding<VtVec3fArray>() && value.GetArraySize() > 0;
}

bool
_MotionBlurHelper::_HasAccelerations() const
{
    const VtValue& value = _GetAccelerations();
    return value.IsHolding<VtVec3fArray>() && value.GetArraySize() > 0;
}

bool
_MotionBlurHelper::_IsRotations() const
{
    static const TfTokenSet rotateKeys =
    {
        HdInstancerTokens->rotate
#if HD_API_VERSION >= 56
        , HdInstancerTokens->instanceRotations
#endif
    };
    return rotateKeys.find(_key) != rotateKeys.end();
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
        HdPrimvarsSchemaTokens->points,
        HdInstancerTokens->translate,
        HdInstancerTokens->rotate,
        HdInstancerTokens->scale,
        HdInstancerTokens->instanceTransform
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
_MotionBlurHelper::_IsVelocityBlurablePrimvar() const
{
    static const TfTokenSet blurables = {
        HdPrimvarsSchemaTokens->points,
        HdInstancerTokens->translate,
        HdInstancerTokens->rotate
#if HD_API_VERSION >= 56
        , HdInstancerTokens->instanceTranslations
        , HdInstancerTokens->instanceRotations
#endif
    };
    return blurables.find(_key) != blurables.end();
}

bool
_MotionBlurHelper::_GetContributingSampleTimesForInterval(
    Time givenStartTime,
    Time givenEndTime,
    std::vector<Time>* const outSampleTimes)
{
    *outSampleTimes = { 0.0f };

    if (!_samplesSource) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): no underlying source\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        return false;
    }

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

    // Check if this sample source is blurable
    const bool isTransform = _IsTransform();
    if (!isTransform && !_IsBlurablePrimvar()) {
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

    // Check if motion blur is disabled by blurScale = 0.0f
    float blurScale = _GetBlurScale();
    if (blurScale == 0.0f) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): blurScale is 0\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        return false;
    }

    // Check if motion blur is disabled by ri:object:{geo|xform}samples < 2
    int numSamples = _GetLinearSampleCount();
    if (numSamples < 2) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): linear sample count < 2\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        return false;
    }

    // Try to get valid shutter interval
    Time startTime, endTime;
    std::tie(startTime, endTime) = _GetSamplingInterval(
        _samplesSource,
        _shutterOpen, _shutterClose,
        givenStartTime, givenEndTime);

    // Check if motion blur is disabled by too small shutter interval
    if (endTime - startTime < _minimumShutterInterval) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): interval [%f, %f] is too short\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText(),
            startTime, endTime);
        return false;
    }

    // Check for velocity blur
    if (_IsVelocityBlurablePrimvar()) {
        const TfToken vblur = _GetVblur();
        if (vblur != _tokens->vblur_off && _HasVelocities()) {
            const size_t sourceCount = _GetSourceValue(0.0f).GetArraySize();
            if (sourceCount <= _GetVelocities().GetArraySize()) {
                // Switch to nonlinear sample count if doing nonlinear velocity
                // blur. XXX: Velocity blur is always nonlinear for instance
                // rotations. It is nonlinear for points/positions when:
                //  * accelerations are present,
                //  * accelerations are of the expected type,
                //  * accelerations are at least as numerous as the source, and
                //  * ri:object:vblur = "Acceleration Blur".
                // TODO: There is an assumption here that nonlinearSampleCount
                // has meaning only when we are performing a nonlinear type
                // of motion blur. We should validate that assumption.
                if (_IsRotations() ||
                    (vblur == _tokens->ablur_on && _HasAccelerations() &&
                        sourceCount <= _GetAccelerations().GetArraySize())) {
                    numSamples = std::max(numSamples,
                        _GetNonlinearSampleCount());
                }
                // Generate sample times unaffected by blurScale.
                // XXX: blurScale is applied in GetValue when doing velocity
                // blur, so we do not include its effect here.
                const float m(numSamples - 1);
                outSampleTimes->clear();
                for (size_t i = 0; i < size_t(numSamples); ++i) {
                    outSampleTimes->push_back(
                        (float(m - i) / m) * startTime +
                        (float(i    ) / m) * endTime);
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
                        "<%s.%s> (%s): velocity motion blur sample times: "
                        "[%s]\n",
                        _primPath.GetText(), _key.GetText(),
                        _primType.GetText(), s.c_str());
                }
                return true;
            }
        }
    }

    // No velocity blur. Fall back to ordinary sampling.
    // Scale start and end times by blurScale
    if (blurScale != 1.0f) {
        // Validate interval
        if (!(std::numeric_limits<Time>::lowest() < startTime &&
            endTime < std::numeric_limits<Time>::max())) {
            static std::once_flag flag;
            std::call_once(flag, [](){
                TF_CODING_ERROR("blurScale is not supported when consumer "
                    "is not specifying interval for contributing sample "
                    "times. In particular, blurScale is not supported for "
                    "legacy prims under scene index emulation.");
            });
            blurScale = 1.0f;
        } else {
            startTime *= blurScale;
            endTime *= blurScale;
        }
    }

    outSampleTimes->clear();

    // Query the underlying samples source for sample times
    if (!_samplesSource->GetContributingSampleTimesForInterval(
        startTime, endTime, outSampleTimes)) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): underlying source has no motion\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        *outSampleTimes = { 0.0f };
        return false;
    }

    // XXX: should not be necesary but some implementations of
    // GetContributingSampleTimesForInterval() will return true with only a
    // single sample time.
    if (outSampleTimes->size() < 2) {
        TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
            "<%s.%s> (%s): underlying source has <2 samples in interval\n",
            _primPath.GetText(), _key.GetText(), _primType.GetText());
        *outSampleTimes = { 0.0f };
        return false;
    }

    // Check ordinality
    const size_t count = _GetSourceValue((*outSampleTimes)[0]).GetArraySize();
    for (size_t i = 1; i < outSampleTimes->size(); ++i) {
        if (_GetSourceValue((*outSampleTimes)[i]).GetArraySize() != count) {
            TF_DEBUG(HDPRMAN_MOTION_BLUR).Msg(
                "<%s.%s> (%s): source array size varies over interval\n",
                _primPath.GetText(), _key.GetText(), _primType.GetText());
            *outSampleTimes = { 0.0f };
            return false;
        }
    }

    // If more samples are requested than are authored, re-interpolate times
    if (static_cast<int>(outSampleTimes->size()) < numSamples) {
        const Time first = outSampleTimes->front();
        const Time last = outSampleTimes->back();
        const float m(numSamples - 1);
        outSampleTimes->clear();
        outSampleTimes->reserve(numSamples);
        for (size_t i = 0; i < size_t(numSamples); ++i) {
            outSampleTimes->push_back(
                (float(m - i) / m) * first +
                (float(i    ) / m) * last);
        }
    }

    // rescale times back to requested interval
    if (blurScale != 1.0f) {
        const float invBlurScale = 1.0f / blurScale;
        for (Time& time : *outSampleTimes) {
            time *= invBlurScale;
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
            "<%s.%s> (%s): motion blur sample times: [%s]\n",
            _primPath.GetText(), _key.GetText(),
            _primType.GetText(), s.c_str());
    }

    return true;
}

VtValue
_MotionBlurHelper::_GetValue(Time givenShutterOffset)
{
    // Shortcut for offset of 0.0f
    if (givenShutterOffset == 0.0f) {
        return _GetSourceValue(0.0f);
    }

    const float blurScale = _GetBlurScale();
    const Time shutterOffset = givenShutterOffset * blurScale;

    // Shortcut for transform blur
    if (_IsTransform()) {
        return _GetSourceValue(shutterOffset);
    }

    // Due to how GetContributingSampleTimesForInterval works, we only get non-
    // zero offset here if we have a blurable primvar and motion blur is
    // enabled. We only need to figure out whether we're doing velocity blur.
    const TfToken vblur = _GetVblur();
    if (vblur == _tokens->vblur_off ||          // cheapest
        !_IsVelocityBlurablePrimvar() ||        // slightly less cheap
        !_HasVelocities()) {                    // least cheap
        return _GetSourceValue(shutterOffset);
    } 
    
    // Do velocity blur!
    const VtValue sourceValue = _GetSourceValue(0.0f);
    const size_t sourceCount = sourceValue.GetArraySize();
    const VtValue velocitiesValue = _GetVelocities();
    const float fps = _GetFps(_inputArgs);
    const float scaledTime = shutterOffset / fps;
    const VtVec3fArray& velocities = velocitiesValue
        .UncheckedGet<VtVec3fArray>();
    
    // Check that we have enough velocities.
    if (velocities.size() != sourceCount) {
        TF_WARN("Number of angular velocity vectors (%zu) does not "
            "match number of rotations (%zu).", velocities.size(),
            sourceCount);
        // Be forgiving and only bail if we're short on velocities.
        if (velocities.size() < sourceCount) {
            return _GetSourceValue(shutterOffset);
        }
    }

    if (_IsRotations()) {
        // Check the source type
        if (!sourceValue.IsHolding<VtQuathArray>()) {
            TF_WARN("Unexpected type encountered for instance "
                "rotations. Expected VtQuathArray, got %s.",
                sourceValue.GetTypeName().c_str());
            return VtValue();
        }

        // Get the source
        const VtQuathArray& rotations = sourceValue
            .UncheckedGet<VtQuathArray>();

        // Apply the angular velocities to the rotations
        VtQuathArray result(sourceCount);
        for (size_t i = 0; i < sourceCount; ++i) {
            GfRotation rotation = GfRotation(rotations[i]);
            rotation *= GfRotation(velocities[i],
                scaledTime * velocities[i].GetLength());
            result[i] = GfQuath(rotation.GetQuat());
        }

        return VtValue(result);
    } else {
        // Check the source type
        if (!sourceValue.IsHolding<VtVec3fArray>()) {
            TF_WARN("Unexpected type encountered for points/positions. "
                "Expected VtVec3fArray, got %s.",
                sourceValue.GetTypeName().c_str());
            return VtValue();
        }

        // Get the source
        const VtVec3fArray& positions = sourceValue
            .UncheckedGet<VtVec3fArray>();
        
        // Check for acceleration blur
        bool useAccelerations = vblur == _tokens->ablur_on
            && _GetNonlinearSampleCount() >2
            && _HasAccelerations();
        static const VtVec3fArray emptyArray;
        const VtVec3fArray& accelerations = useAccelerations
            ? _GetAccelerations().UncheckedGet<VtVec3fArray>()
            : emptyArray;

        // Check that we have enough accelerations.
        if (useAccelerations && accelerations.size() != sourceCount) {
            TF_WARN("Number of acceleration vectors (%zu) does not "
                "match number of points/positions (%zu).",
                accelerations.size(), sourceCount);
            // Be forgiving and only bail on acceleration if we're
            // short on accelerations.
            if (accelerations.size() < sourceCount) {
                useAccelerations = false;
            }
        }

        // Apply the velocities and accelerations to the
        // points/positions.
        VtVec3fArray result(sourceCount);
        if (useAccelerations) {
            const float timeSqrHalf = 0.5f * scaledTime * scaledTime;
            for (size_t i = 0; i < sourceCount; i++) {
                result[i] += positions[i]
                    + scaledTime * velocities[i]
                    + timeSqrHalf * accelerations[i];
            }
        } else {
            for (size_t i = 0; i < sourceCount; i++) {
                result[i] += positions[i]
                    + scaledTime * velocities[i];
            }
        }
        return VtValue(result);
    } 
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
    /// inputArgs: data source from scene index plugin
    _MotionBlurUntypedSampledDataSource(
        const HdSampledDataSourceHandle& samplesSource,
        const TfToken& key,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _MotionBlurHelper(
            samplesSource, key, primPath, primType, primvarsSource, inputArgs)
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
    /// inputArgs: data source from scene index plugin
    _MotionBlurTypedSampledDataSource(
        const HdSampledDataSourceHandle& samplesSource,
        const TfToken& key,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _MotionBlurHelper(
            samplesSource, key, primPath, primType, primvarsSource, inputArgs)
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
        const HdContainerDataSourceHandle& primvarsSource,
        const HdContainerDataSourceHandle& inputArgs)
    {
        return _MotionBlurTypedSampledDataSource<T>::Handle(
            new _MotionBlurTypedSampledDataSource<T>(
                samplesSource, key, primPath,
                primType, primvarsSource, inputArgs));
    }
};

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
    const HdContainerDataSourceHandle inputArgs;

    /// Handler for every type except the fallback VtValue type
    template <typename T>
    HdDataSourceBaseHandle
    operator()(const T&)
    {
        return _MotionBlurTypedSampledDataSource<T>::New(
            samplesSource, key, primPath, primType, primvarsSource, inputArgs);
    }

    /// Handler for the fallback VtValue type
    HdDataSourceBaseHandle
    operator()(const VtValue&)
    {
        return _MotionBlurUntypedSampledDataSource::New(
            samplesSource, key, primPath, primType, primvarsSource, inputArgs);
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
    /// inputArgs: data source from scene index plugin
    _PrimvarDataSource(
        const HdContainerDataSourceHandle& primvarSource,
        const TfToken& primvarName,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _primvarSource(primvarSource)
      , _primvarName(primvarName)
      , _primPath(primPath)
      , _primType(primType)
      , _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
    { }

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
    TfToken _primvarName;
    SdfPath _primPath;
    TfToken _primType;
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarDataSource);

HdDataSourceBaseHandle
_PrimvarDataSource::Get(const TfToken &name)
{
    if (!_primvarSource) {
        return nullptr;
    }

    const HdDataSourceBaseHandle result = _primvarSource->Get(name);

    if (name == HdPrimvarSchemaTokens->primvarValue) {
        if (const auto source = HdSampledDataSource::Cast(result)) {
            return VtVisitValue(source->GetValue(0.0f),
                _Visitor { source, _primvarName, _primPath,
                    _primType, _primvarsSource, _inputArgs });
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
    /// inputArgs: source from scene index plugin
    _PrimvarsDataSource(
        const HdContainerDataSourceHandle& primvarsSource,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& inputArgs)
      : _primvarsSource(primvarsSource)
      , _primPath(primPath)
      , _primType(primType)
      , _inputArgs(inputArgs)
    { }

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
    SdfPath _primPath;
    TfToken _primType;
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
            _primvarsSource,
            _inputArgs);
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
    /// inputArgs: data source from scene index plugin
    _XformDataSource(
        const HdContainerDataSourceHandle& xformSource,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& primvarsSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _xformSource(xformSource)
      , _primPath(primPath)
      , _primType(primType)
      , _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
    { }

    TfTokenVector GetNames() override
    {
        if (!_xformSource) {
            return {};
        }
        return _xformSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override;

private:
    HdContainerDataSourceHandle _xformSource;
    SdfPath _primPath;
    TfToken _primType;
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_XformDataSource);

HdDataSourceBaseHandle
_XformDataSource::Get(const TfToken& name)
{
    if (!_xformSource) {
        return nullptr;
    }
    
    const HdDataSourceBaseHandle result = _xformSource->Get(name);
    
    if (name == HdXformSchemaTokens->matrix) {
        if (const auto source = HdSampledDataSource::Cast(result)) {
            return VtVisitValue(source->GetValue(0.0f), 
                _Visitor { source, name, _primPath,
                    _primType, _primvarsSource, _inputArgs });
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
    /// inputArgs: source from scene index plugin
    _PrimDataSource(
        const HdContainerDataSourceHandle& primSource,
        const SdfPath& primPath,
        const TfToken& primType,
        const HdContainerDataSourceHandle& inputArgs)
      : _primSource(primSource)
      , _primPath(primPath)
      , _primType(primType)
      , _inputArgs(inputArgs)
    { }

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
    SdfPath _primPath;
    TfToken _primType;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimDataSource);

HdDataSourceBaseHandle
_PrimDataSource::Get(const TfToken &name)
{
    if (!_primSource) {
        return nullptr;
    }

    const HdDataSourceBaseHandle result = _primSource->Get(name);

    if (name == HdPrimvarsSchemaTokens->primvars) {
        if (const auto primvarsSource = HdContainerDataSource::Cast(result)) {
            return _PrimvarsDataSource::New(
                primvarsSource, _primPath, _primType, _inputArgs);
        }
    }

    if (name == HdXformSchemaTokens->xform) {
        if (const auto xformSource = HdContainerDataSource::Cast(result)) {
            if (const auto primvarsSource = HdContainerDataSource::Cast(
                _primSource->Get(HdPrimvarsSchema::GetSchemaToken()))) {
                return _XformDataSource::New(
                    xformSource, _primPath, _primType,
                    primvarsSource, _inputArgs);
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
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const HdContainerDataSourceHandle& inputArgs)
    {
        return TfCreateRefPtr(
            new _HdPrmanMotionBlurSceneIndex(
                inputSceneIndex, inputArgs));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (_PrimIsBlurable(prim)) {
            return {
                prim.primType,
                _PrimDataSource::New(
                    prim.dataSource, primPath, prim.primType, _inputArgs) };
        }
        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _HdPrmanMotionBlurSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const HdContainerDataSourceHandle& inputArgs)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
      , _inputArgs(inputArgs)
    { }

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
    static bool _PrimIsBlurable(const HdSceneIndexPrim& prim)
    {
        // Transformables, points-based, and instancers are blurable, but
        // points-based and instancers are always also transformable so 
        // we only check for the xform schema.

        // XXX: renderSettings and integrator prim types currently leak
        // through a transformable check, so we also filter out those
        // specific prim types

        if (prim.primType == HdPrimTypeTokens->renderSettings ||
            prim.primType == HdPrimTypeTokens->integrator) {
            return false;
        }
        if (const auto container = HdContainerDataSource::Cast(
            prim.dataSource)) {
            if (HdXformSchema::GetFromParent(container)) {
                return true;
            }
        }
        return false;
    };

    HdContainerDataSourceHandle _inputArgs;
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
    const HdSceneIndexBase& sender,
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
        _GetPrimvarValueLocator(HdTokens->velocities),
        _GetPrimvarValueLocator(HdTokens->accelerations),
        _GetPrimvarValueLocator(_tokens->angularVelocities),
        _GetPrimvarValueLocator(_tokens->mblur),
        _GetPrimvarValueLocator(_tokens->geosamples),
        _GetPrimvarValueLocator(_tokens->xformsamples),
        _GetPrimvarValueLocator(HdTokens->nonlinearSampleCount),
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
    const HdContainerDataSourceHandle& inputArgs)
{
    return _HdPrmanMotionBlurSceneIndex::New(inputScene, inputArgs);
}

// TODO: Query the camera's shutter interval in a better way!
// This method is called by the camera to update
// with its shutter interval, which is necessary
// when we're doing velocity blur and UsdImaging doesn't have the sample times.
void
HdPrman_MotionBlurSceneIndexPlugin::SetShutterInterval(
    float shutterOpen, float shutterClose)
{
    _shutterOpen = shutterOpen;
    _shutterClose = shutterClose;
}

PXR_NAMESPACE_CLOSE_SCOPE
