#include "pxr/usd/plugin/usdAnimX/keyframe.h"
#include <iostream>
PXR_NAMESPACE_OPEN_SCOPE

std::ostream&
operator<<(std::ostream& out, const adsk::Keyframe& k)
{
    return out
        << k.time << ": "
        << "("
        << k.value << ", "
        << "(" << (short)k.tanIn.type << ", "
        << k.tanIn.x << ", "
        << k.tanIn.y << "), "
        << "(" << (short)k.tanOut.type << ", "
        << k.tanOut.x << ", "
        << k.tanOut.y << "), "
        << k.quaternionW
        << ")";
}

void 
GetKeyframeDescription(adsk::Keyframe const& k, UsdAnimXKeyframeDesc* d)
{
  d->time = k.time;
  d->data.resize(8);
  d->data[0] = k.value;
  d->data[1] = (double)k.tanIn.type;
  d->data[2] = k.tanIn.x;
  d->data[3] = k.tanIn.y;
  d->data[4] = (double)k.tanOut.type;
  d->data[5] = k.tanOut.x;
  d->data[6] = k.tanOut.y;
  d->data[7] = k.quaternionW;
}

void 
GetKeyframeDescription(VtValue const& k, UsdAnimXKeyframeDesc* d)
{
  std::cout << "ARRAY VALUED : " << k.IsArrayValued() << std::endl;
  std::cout << "ELEMENT TYPE : " << k.GetElementTypeid().name() << std::endl;
  std::cout << "Is EMPTY : " << k.IsEmpty() << std::endl;
  if(k.IsArrayValued() && !k.IsEmpty()) {
    std::cout << "ELEMENT : " << k.GetElementTypeid().name() << std::endl;
  }
  /*
  std::cout << "HOLDING : " << k.GetTypeName() << std::endl;
  if(k.IsHolding<VtArray<double>>()) {
    VtArray<double> data = k.UncheckedGet<VtArray<double>>();
    if(data.size() == 9) {
      d->time = data[0];
      d->data.resize(8);
      for(size_t i=0;i<8;++i)d->data[i] = data[i+1];
    }
  }
  else std::cout << "NOT HOLDING DOUBLE ARRAY !!" << std::endl;
  */
}

adsk::Keyframe 
GetKeyframeFromVtValue(double time, VtValue const& value, size_t index)
{
  std::cout << "### GET KEYFRAME FROM VTVALUE !!!" << std::endl;
  VtArray<double> a = value.UncheckedGet<VtArray<double>>();
  adsk::Keyframe keyframe;
  keyframe.time = time;
  keyframe.value = a[0];
  keyframe.index = index;
  adsk::Tangent tanIn;
  tanIn.type = (adsk::TangentType)(int)a[1];
  tanIn.x = (adsk::seconds)a[2];
  tanIn.y = (adsk::seconds)a[3];
  keyframe.tanIn = tanIn;
  adsk::Tangent tanOut;
  tanOut.type = (adsk::TangentType)(int)a[4];
  tanOut.x = (adsk::seconds)a[5];
  tanOut.y = (adsk::seconds)a[6];
  keyframe.tanOut = tanOut;
  keyframe.quaternionW = a[7];
  keyframe.linearInterpolation = false;
  std::cout << keyframe << std::endl;
   std::cout << keyframe.time << ": "<< keyframe.value  << std::endl;
  return keyframe;

 
}

PXR_NAMESPACE_CLOSE_SCOPE