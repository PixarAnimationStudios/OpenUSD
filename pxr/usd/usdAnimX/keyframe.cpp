#include "pxr/usd/usdAnimX/keyframe.h"
#include <iostream>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/value.h>

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<UsdAnimXKeyframe>();
}

UsdAnimXKeyframe::UsdAnimXKeyframe()
{
    time = 1.0;
    index = 0;
    value = 0.0;
    tanIn = {adsk::TangentType::Auto, (adsk::seconds)1.f, (adsk::seconds)0.f};
    tanOut = {adsk::TangentType::Auto, (adsk::seconds)-1.f, (adsk::seconds)0.f};
    quaternionW = 1.0;
    linearInterpolation = false;
}

UsdAnimXKeyframe::UsdAnimXKeyframe(const UsdAnimXKeyframeDesc &desc, 
    size_t idx)
{
    time = desc.time;
    index = idx;
    value = desc.data[0];
    tanIn = {
        (adsk::TangentType)(int)desc.data[1], 
        (adsk::seconds)desc.data[2],
        (adsk::seconds)desc.data[3]};
    tanOut = {
        (adsk::TangentType)(int)desc.data[4], 
        (adsk::seconds)desc.data[5], 
        (adsk::seconds)desc.data[6]};
    quaternionW = desc.data[7];
    linearInterpolation = false;
}

UsdAnimXKeyframe::UsdAnimXKeyframe(double t, VtValue &val, size_t idx)
{
    VtArray<double> a = val.UncheckedGet<VtArray<double>>();
    time = t;
    index = idx;
    value = a[0];
    tanIn = {
        (adsk::TangentType)(int)a[1], 
        (adsk::seconds)a[2], 
        (adsk::seconds)a[3]};
    tanOut = {
        (adsk::TangentType)(int)a[4], 
        (adsk::seconds)a[5], 
        (adsk::seconds)a[6]};
    quaternionW = a[7];
    linearInterpolation = false;
}

UsdAnimXKeyframeDesc 
UsdAnimXKeyframe::GetDesc()
{
    UsdAnimXKeyframeDesc desc;
    desc.time = (double)time;
    desc.data[0] = (double)value;
    desc.data[1] = (double)tanIn.type;
    desc.data[2] = (double)tanIn.x;
    desc.data[3] = (double)tanIn.y;
    desc.data[4] = (double)tanOut.type;
    desc.data[5] = (double)tanOut.x;
    desc.data[6] = (double)tanOut.y;
    desc.data[7] = quaternionW;
    return desc;
}

VtValue
UsdAnimXKeyframe::GetAsSample()
{
    VtArray<double> result(8);
    result[0] = value;
    result[1] = (double)tanIn.type;
    result[2] = (double)tanIn.x;
    result[3] = (double)tanIn.y;
    result[4] = (double)tanOut.type;
    result[5] = (double)tanOut.x;
    result[6] = (double)tanOut.y;
    result[7] = (double)quaternionW;
    return VtValue(result);
}

std::ostream&
operator<<(std::ostream &out, const UsdAnimXKeyframe &k)
{
    return out
        << "("
        << TfStreamDouble(k.time) << ", "
        << TfStreamDouble(k.value) << ", "
        << (short)k.tanIn.type << ", "
        << TfStreamDouble(k.tanIn.x) << ", "
        << TfStreamDouble(k.tanIn.y) << ", "
        << (short)k.tanOut.type << ", "
        << TfStreamDouble(k.tanOut.x) << ", "
        << TfStreamDouble(k.tanOut.y) << ", "
        << TfStreamDouble(k.quaternionW)
        << ")";
}

std::ostream&
operator<<(std::ostream &out, const UsdAnimXKeyframeDesc &k)
{
    return out
        << "("
        << TfStreamDouble(k.time) << ", "
        << TfStreamDouble(k.data[0]) << ", "
        << k.data[1] << ", "
        << TfStreamDouble(k.data[2]) << ", "
        << TfStreamDouble(k.data[3]) << ", "
        << k.data[4] << ", "
        << TfStreamDouble(k.data[5]) << ", "
        << TfStreamDouble(k.data[6]) << ", "
        << TfStreamDouble(k.data[7])
        << ")";
}

PXR_NAMESPACE_CLOSE_SCOPE