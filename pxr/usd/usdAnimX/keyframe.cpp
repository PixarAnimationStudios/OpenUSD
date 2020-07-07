#include "keyframe.h"
#include <iostream>
PXR_NAMESPACE_OPEN_SCOPE

UsdAnimXKeyframe::UsdAnimXKeyframe(const UsdAnimXKeyframeDesc &desc, 
    size_t idx)
{
    time = desc.time;
    index = index;
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

std::ostream&
operator<<(std::ostream &out, const UsdAnimXKeyframe &k)
{
    return out
        << "("
        << k.time << ", "
        << k.value << ", "
        << (short)k.tanIn.type << ", "
        << k.tanIn.x << ", "
        << k.tanIn.y << ", "
        << (short)k.tanOut.type << ", "
        << k.tanOut.x << ", "
        << k.tanOut.y << ", "
        << k.quaternionW
        << ")";
}

PXR_NAMESPACE_CLOSE_SCOPE