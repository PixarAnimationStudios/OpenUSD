#include "pxr/usd/usdAnimX/curve.h"
#include "pxr/usd/usdAnimX/tokens.h"
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

UsdAnimXCurve::UsdAnimXCurve() 
  : _weighted(false)
  , _static(true)
{ 
};

UsdAnimXCurve::UsdAnimXCurve(const UsdAnimXCurveDesc &desc)
{
    size_t keyframeIndex = 0;
    _name = desc.name.GetString();
    _ResolveInfinityType(desc.preInfinityType, &_preInfinityType);
    _ResolveInfinityType(desc.postInfinityType, &_postInfinityType);
    for(const auto& keyframe: desc.keyframes) {
        addKeyframe(UsdAnimXKeyframe(keyframe, keyframeIndex));
        keyframeIndex++;
    }
}


bool  UsdAnimXCurve::keyframe(double time, adsk::Keyframe &key) const
{
    size_t numKeys = _keyframes.size();
    if (numKeys == 0)
        return false;

    size_t index = findClosest( time );
    if (index == numKeys)
        return last(key);

    // if found key lies before the requested time, choose next key instead
    if (_keyframes[index].time < time)
    {
        if (index != numKeys - 1)
            index++;
        else
            return last(key);
    }
    return keyframeAtIndex(index, key);
}

bool UsdAnimXCurve::keyframeAtIndex(int index, adsk::Keyframe &key) const
{
    size_t numKeys = _keyframes.size();
    if (index < 0 || index >= _keyframes.size())
        return false;

    key = _keyframes[index];
    return true;
}

bool UsdAnimXCurve::first(adsk::Keyframe &key) const
{
    return keyframeAtIndex(0, key);
}

bool UsdAnimXCurve::last(adsk::Keyframe &key) const
{
    return (keyframeCount() > 0) ?
        keyframeAtIndex(keyframeCount() - 1, key) :
        false;
}

adsk::InfinityType UsdAnimXCurve::preInfinityType() const
{
    return _preInfinityType;
}

adsk::InfinityType UsdAnimXCurve::postInfinityType() const
{
    return _postInfinityType;
}

bool UsdAnimXCurve::isWeighted() const
{
    return _weighted;
}

bool UsdAnimXCurve::isStatic() const
{
    return false;
}

unsigned int UsdAnimXCurve::keyframeCount() const
{
    return _keyframes.size();
}

bool UsdAnimXCurve::isRotation() const
{
    return _rotationInterpolationMethod == 
            adsk::CurveRotationInterpolationMethod::Quaternion ||
        _rotationInterpolationMethod == 
            adsk::CurveRotationInterpolationMethod::Slerp ||
        _rotationInterpolationMethod == 
            adsk::CurveRotationInterpolationMethod::Squad;
}

size_t UsdAnimXCurve::findClosest(double time) const
{
    if(time <= _keyframes.front().time) {
        return 0;
    } else if(time > _keyframes.back().time) {
        return _keyframes.size() - 1;
    } else {
        double t = _keyframes.front().time;
        for(const auto& key: _keyframes) {
            if(time<=key.time) {
                return key.index - ((key.time - time) > (time - t) ? 0 : 1);
            } else {
                t = key.time;
            }
        }
    }
}

void UsdAnimXCurve::addKeyframe(const UsdAnimXKeyframe& key)
{
    if(_keyframes.size()) {
        if(key.time < _keyframes.front().time)
            _keyframes.insert(_keyframes.begin(), key);
        else if(key.time > _keyframes.back().time)
            _keyframes.push_back(key);
        else {
            double t1 = std::numeric_limits<double>::max();
            for(auto it = _keyframes.begin(); it <_keyframes.end(); ++it) {
                if(it->time > t1) {
                    _keyframes.insert(it, key);
                    break;
                } else {
                    t1 = it->time;
                }
            }
        }
    } else {
        _keyframes.push_back(key);
    }
    if(_keyframes.size() > 1) {
      _static = false;
      _weighted = true;
    }

    reindexKeys();
}

void UsdAnimXCurve::addKeyframe(double time, double value)
{
    adsk::Tangent tanIn = { 
        adsk::TangentType::Auto, (adsk::seconds)-1, (adsk::seconds)0};
    adsk::Tangent tanOut = { 
        adsk::TangentType::Auto, (adsk::seconds)1, (adsk::seconds)0};

    UsdAnimXKeyframe key;
    key.time = time;                        // Time
    key.value = value;                      // Value 
    key.tanIn = tanIn;                      // In-tangent
    key.tanOut = tanOut;                    // Out-tangent
    key.quaternionW = 1.0;                  // W component of a quaternion
    key.linearInterpolation = false;        // Curve linearly interpolated

    addKeyframe(key);
}

void UsdAnimXCurve::reindexKeys()
{
    size_t keyIndex = 0;
    for(auto& keyframe: _keyframes)
        keyframe.index = keyIndex++;
}

void UsdAnimXCurve::removeKeyframe(double time)
{ 
    std::vector<UsdAnimXKeyframe>::iterator i = _keyframes.begin();
    for(;i<_keyframes.end();++i) {
        if(i->time == time)_keyframes.erase(i);
    }
}

void UsdAnimXCurve::removeKeyframe(size_t index)
{
    if(index >= 0 && index < _keyframes.size())
        _keyframes.erase(_keyframes.begin() + index);
}

void UsdAnimXCurve::setKeyframeAtIndex(size_t index, const adsk::Keyframe& k)
{
    if(index > 0 && index < _keyframes.size())
        _keyframes[index] = *(UsdAnimXKeyframe*)&k;
}

double UsdAnimXCurve::evaluate(double time) const
{
    return adsk::evaluateCurve(time, *this);
}

std::set<double> UsdAnimXCurve::computeSamples() const
{
    std::set<double> samples;
    for(const auto& keyframe: _keyframes) {
        samples.insert(keyframe.time);
    }
    return samples;
}

const std::string& UsdAnimXCurve::getName() const
{
    return _name;
}

void UsdAnimXCurve::setName(const std::string& name)
{
    _name = name;
}

PXR_NAMESPACE_CLOSE_SCOPE