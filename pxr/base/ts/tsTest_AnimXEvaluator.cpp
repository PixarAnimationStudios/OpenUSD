//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_AnimXEvaluator.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/diagnostic.h"

// This flag is turned on in all of our AnimX builds.
// We just hard-code it here.
// It is apparently necessary in order to make AnimX match Maya precisely.
#define MAYA_64BIT_TIME_PRECISION
#include <animx.h>

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

using Evaluator = TsTest_AnimXEvaluator;
using STimes = TsTest_SampleTimes;
using SData = TsTest_SplineData;

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(Evaluator::AutoTanAuto);
    TF_ADD_ENUM_NAME(Evaluator::AutoTanSmooth);
}

TsTest_AnimXEvaluator::TsTest_AnimXEvaluator(
    const AutoTanType autoTanType)
    : _autoTanType(autoTanType)
{
}

namespace
{
    class _Curve : public adsk::ICurve
    {
    public:
        _Curve(
            const SData &data,
            const Evaluator::AutoTanType autoTanType)
            : _autoTanType(autoTanType)
        {
            _isWeighted = !data.GetIsHermite();
            _preInfinity = _GetInfinity(data.GetPreExtrapolation());
            _postInfinity = _GetInfinity(data.GetPostExtrapolation());

            if (data.GetKnots().empty())
                return;

            int i = 0;
            adsk::TangentType tanType = adsk::TangentType::Global;

            // Translate TsTestUtils knots to AnimX keyframes.
            for (const SData::Knot &kf : data.GetKnots())
            {
                adsk::Keyframe axKf;
                axKf.time = kf.time;
                axKf.value = kf.value;
                axKf.index = i++;

                double preLen = 0, postLen = 0;
                if (data.GetIsHermite())
                {
                    // Hermite spline.  Tan lengths may be zero and are ignored.
                    // Any nonzero length will allow us to establish a slope in
                    // X and Y, so use length 1.
                    preLen = 1;
                    postLen = 1;
                }
                else
                {
                    // Non-Hermite spline.  Use tan lengths as specified.
                    // Multiply by 3.
                    preLen = kf.preLen * 3;
                    postLen = kf.postLen * 3;
                }

                // Use previous segment type as in-tan type.
                axKf.tanIn.type = _GetTanType(tanType, kf.preAuto);
                axKf.tanIn.x = preLen;
                axKf.tanIn.y = kf.preSlope * preLen;

                // Determine new out-tan type and record that for the next
                // in-tan.
                tanType = _GetBaseTanType(kf.nextSegInterpMethod);
                axKf.tanOut.type = _GetTanType(tanType, kf.postAuto);
                axKf.tanOut.x = postLen;
                axKf.tanOut.y = kf.postSlope * postLen;

                // XXX: unimplemented for now.
                axKf.quaternionW = 0;

                axKf.linearInterpolation =
                    (kf.nextSegInterpMethod == SData::InterpLinear);

                // Store new AnimX keyframe.
                _kfs[kf.time] = axKf;
            }

            // Implement linear pre-extrap with explicit linear tangents.
            if (data.GetPreExtrapolation().method == SData::ExtrapLinear)
            {
                adsk::Keyframe &axKf = _kfs.begin()->second;
                axKf.tanIn.type = adsk::TangentType::Linear;
                if (_kfs.size() == 1
                    || axKf.tanOut.type == adsk::TangentType::Step)
                {
                    // Mirror a held (flat) segment.
                    axKf.tanIn.x = 1.0;
                    axKf.tanIn.y = 0.0;
                }
                else if (axKf.tanOut.type == adsk::TangentType::Linear)
                {
                    // Mirror a linear segment.
                    const adsk::Keyframe &axNextKf =
                        (++_kfs.begin())->second;
                    axKf.tanIn.x = axNextKf.time - axKf.time;
                    axKf.tanIn.y = axNextKf.value - axKf.value;
                }
                else
                {
                    // Mirror the tangent to a curved segment.
                    axKf.tanIn.x = axKf.tanOut.x;
                    axKf.tanIn.y = axKf.tanOut.y;
                }
            }

            // Implement linear post-extrap with explicit linear tangents.
            if (data.GetPostExtrapolation().method == SData::ExtrapLinear)
            {
                adsk::Keyframe &axKf = _kfs.rbegin()->second;
                axKf.tanOut.type = adsk::TangentType::Linear;
                if (_kfs.size() == 1
                    || axKf.tanIn.type == adsk::TangentType::Step)
                {
                    // Mirror a held (flat) segment.
                    axKf.tanOut.x = 1.0;
                    axKf.tanOut.y = 0.0;
                }
                else if (axKf.tanIn.type == adsk::TangentType::Linear)
                {
                    // Mirror a linear segment.
                    const adsk::Keyframe &axPrevKf =
                        (++_kfs.rbegin())->second;
                    axKf.tanOut.x = axKf.time - axPrevKf.time;
                    axKf.tanOut.y = axKf.value - axPrevKf.value;
                }
                else
                {
                    // Mirror the tangent to a curved segment.
                    axKf.tanOut.x = axKf.tanIn.x;
                    axKf.tanOut.y = axKf.tanIn.y;
                }
            }
        }

        virtual ~_Curve() = default;

        bool keyframeAtIndex(
            const int index, adsk::Keyframe &keyOut) const override
        {
            if (index < 0 || size_t(index) >= _kfs.size())
                return false;

            auto it = _kfs.begin();
            for (int i = 0; i < index; i++)
                it++;
            keyOut = it->second;
            return true;
        }

        // If there is a keyframe at the specified time, return that.  If the
        // time is after the last keyframe, return the last.  Otherwise return
        // the next keyframe after the specified time.
        bool keyframe(
            const double time, adsk::Keyframe &keyOut) const override
        {
            const auto it = _kfs.lower_bound(time);
            if (it == _kfs.end())
            {
                return last(keyOut);
            }
            else
            {
                keyOut = it->second;
                return true;
            }
        }

        bool first(adsk::Keyframe &keyOut) const override
        {
            if (_kfs.empty())
                return false;

            keyOut = _kfs.begin()->second;
            return true;
        }

        bool last(adsk::Keyframe &keyOut) const override
        {
            if (_kfs.empty())
                return false;

            keyOut = _kfs.rbegin()->second;
            return true;
        }

        adsk::InfinityType preInfinityType() const override
        {
            return _preInfinity;
        }

        adsk::InfinityType postInfinityType() const override
        {
            return _postInfinity;
        }

        bool isWeighted() const override
        {
            return _isWeighted;
        }

        unsigned int keyframeCount() const override
        {
            return _kfs.size();
        }

        bool isStatic() const override
        {
            // XXX: betting this is just an optimization.
            return false;
        }

    private:
        static adsk::InfinityType _GetInfinity(
            const SData::Extrapolation &extrap)
        {
            // Non-looped modes.
            if (extrap.method == SData::ExtrapHeld)
                return adsk::InfinityType::Constant;
            if (extrap.method == SData::ExtrapLinear)
                return adsk::InfinityType::Linear;

            // Looped modes.
            if (extrap.loopMode == SData::LoopRepeat)
                return adsk::InfinityType::CycleRelative;
            if (extrap.loopMode == SData::LoopReset)
                return adsk::InfinityType::Cycle;
            if (extrap.loopMode == SData::LoopOscillate)
                return adsk::InfinityType::Oscillate;

            TF_CODING_ERROR("Unexpected extrapolation");
            return adsk::InfinityType::Constant;
        }

        static adsk::TangentType _GetBaseTanType(
            const SData::InterpMethod method)
        {
            switch (method)
            {
                case SData::InterpHeld: return adsk::TangentType::Step;
                case SData::InterpLinear: return adsk::TangentType::Linear;
                case SData::InterpCurve: return adsk::TangentType::Global;
            }

            TF_CODING_ERROR("Unexpected base tangent type");
            return adsk::TangentType::Global;
        }

        adsk::TangentType _GetTanType(
            const adsk::TangentType tanType,
            const bool isAuto) const
        {
            if (tanType != adsk::TangentType::Global
                || !isAuto)
            {
                return tanType;
            }

            switch (_autoTanType)
            {
                case Evaluator::AutoTanAuto: return adsk::TangentType::Auto;
                case Evaluator::AutoTanSmooth: return adsk::TangentType::Smooth;
            }

            TF_CODING_ERROR("Unexpected tangent type");
            return adsk::TangentType::Global;
        }

    private:
        Evaluator::AutoTanType _autoTanType = Evaluator::AutoTanAuto;
        bool _isWeighted = false;
        adsk::InfinityType _preInfinity = adsk::InfinityType::Constant;
        adsk::InfinityType _postInfinity = adsk::InfinityType::Constant;
        std::map<double, adsk::Keyframe> _kfs;
    };
}

TsTest_SampleVec
TsTest_AnimXEvaluator::Eval(
    const SData &data,
    const STimes &times) const
{
    static const SData::Features supportedFeatures =
        SData::FeatureHeldSegments |
        SData::FeatureLinearSegments |
        SData::FeatureBezierSegments |
        SData::FeatureHermiteSegments |
        SData::FeatureAutoTangents |
        SData::FeatureExtrapolatingLoops;
    if (data.GetRequiredFeatures() & ~supportedFeatures)
    {
        TF_CODING_ERROR("Unsupported spline features for AnimX");
        return {};
    }

    const _Curve curve(data, _autoTanType);
    TsTest_SampleVec result;

    for (const STimes::SampleTime &sampleTime : times.GetTimes())
    {
        // Emulate pre-values by subtracting an arbitrary tiny time delta (but
        // large enough to avoid Maya snapping it to a knot time).  We will
        // return a sample with a differing time, which will allow the result to
        // be understood as a small delta rather than an instantaneous change.
        double time = sampleTime.time;
        if (sampleTime.pre)
            time -= 1e-5;

        const double value = adsk::evaluateCurve(time, curve);
        result.push_back(TsTest_Sample(time, value));
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
