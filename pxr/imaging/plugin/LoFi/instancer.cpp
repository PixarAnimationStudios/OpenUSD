//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/plugin/LoFi/instancer.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


LoFiInstancer::LoFiInstancer(HdSceneDelegate* delegate,
                                     SdfPath const& id)
    : HdInstancer(delegate, id)
{
}

LoFiInstancer::~LoFiInstancer()
{
    TF_FOR_ALL(it, _primvarMap) {
        delete it->second;
    }
    _primvarMap.clear();
}

void
LoFiInstancer::_GetPrimvarDatas(const TfToken& primvarName, const VtValue& value,
  LoFiAttributeChannel& channel, uint32_t& numElements)
{
  numElements = 0;
  channel = CHANNEL_UNDEFINED;

  if(primvarName == HdInstancerTokens->translate)
  {
    channel = CHANNEL_POSITION;
    if(value.IsHolding<VtArray<GfVec3f>>())
    {
      _positions = value.UncheckedGet<VtArray<GfVec3f>>();
      numElements = _positions.size();
    }
      
  }
  else if(primvarName == HdInstancerTokens->rotate)
  {
    channel = CHANNEL_ROTATION;
    if(value.IsHolding<VtArray<GfVec4f>>())
    {
      _rotations = value.UncheckedGet<VtArray<GfVec4f>>();
      numElements = _rotations.size();
    }
      
  }
  else if(primvarName == HdInstancerTokens->scale)
  {
    channel = CHANNEL_SCALE;
    if(value.IsHolding<VtArray<GfVec3f>>())
    {
      _scales = value.UncheckedGet<VtArray<GfVec3f>>();
      numElements = _scales.size();
    }
      
  }
   else if(primvarName == HdInstancerTokens->instanceTransform)
  {
    channel = CHANNEL_TRANSFORM;
    if(value.IsHolding<VtArray<GfMatrix4d>>())
    {
      _xforms = value.UncheckedGet<VtArray<GfMatrix4d>>();
      numElements = _xforms.size();
    }
      
  }
}

//void 
//LoFiInstancer::_SyncColors()
//{
//  TfToken colorPrimvar("colors");
//
//  VtValue value = GetDelegate()->Get(GetId(), colorPrimvar);
//  if(!value.IsEmpty() && value.IsHolding<VtArray<GfVec3f>>()) 
//  {
//    _colors = value.UncheckedGet<VtArray<GfVec3f>>();
//  }
//}

void
LoFiInstancer::_SyncPrimvars()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    HdChangeTracker &changeTracker = 
        GetDelegate()->GetRenderIndex().GetChangeTracker();
    SdfPath const& id = GetId();

    // Use the double-checked locking pattern to check if this instancer's
    // primvars are dirty.
    int dirtyBits = changeTracker.GetInstancerDirtyBits(id);
    if (HdChangeTracker::IsAnyPrimvarDirty(dirtyBits, id)) {
        std::lock_guard<std::mutex> lock(_instanceLock);

        dirtyBits = changeTracker.GetInstancerDirtyBits(id);
        if (HdChangeTracker::IsAnyPrimvarDirty(dirtyBits, id)) {

            // If this instancer has dirty primvars, get the list of
            // primvar names and then cache each one.

            TfTokenVector primvarNames;
            HdPrimvarDescriptorVector primvars = GetDelegate()
                ->GetPrimvarDescriptors(id, HdInterpolationInstance);

            for (HdPrimvarDescriptor const& pv: primvars) {
                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, pv.name)) {
                    VtValue value = GetDelegate()->Get(id, pv.name);
                    if (!value.IsEmpty()) {
                        if (_primvarMap.count(pv.name) > 0) {
                            //delete _primvarMap[pv.name];
                        }
                        LoFiAttributeChannel channel;
                        uint32_t numElements;
                        _GetPrimvarDatas(pv.name, value, channel, numElements);
                        if(channel == CHANNEL_UNDEFINED || !numElements)continue;

                        _primvarMap[pv.name] = NULL;
                          //new LoFiVertexBuffer(channel, numElements, numElements, HdInterpolationVertex);

                            //new HdVtBufferSource(pv.name, value);
                    }
                }
            }

            // Mark the instancer as clean
            changeTracker.MarkInstancerClean(id);
        }
    }
}

VtMatrix4dArray
LoFiInstancer::ComputeInstanceTransforms(SdfPath const &prototypeId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _SyncPrimvars();
    //_SyncColors();

    // The transforms for this level of instancer are computed by:
    // foreach(index : indices) {
    //     instancerTransform * translate(index) * rotate(index) *
    //     scale(index) * instanceTransform(index)
    // }
    // If any transform isn't provided, it's assumed to be the identity.

    GfMatrix4d instancerTransform =
        GetDelegate()->GetInstancerTransform(GetId());
    VtIntArray instanceIndices =
        GetDelegate()->GetInstanceIndices(GetId(), prototypeId);

    VtMatrix4dArray transforms(instanceIndices.size());
    for (size_t i = 0; i < instanceIndices.size(); ++i) {
        transforms[i] = instancerTransform;
    }

    // "translate" holds a translation vector for each index.
    if (_primvarMap.count(HdInstancerTokens->translate) > 0) {
      for (size_t i = 0; i < instanceIndices.size(); ++i) {
        if(instanceIndices[i]>=0 && instanceIndices[i]<_positions.size())
        {
          GfVec3f translate = _positions[instanceIndices[i]];
          GfMatrix4d translateMat(1);
          translateMat.SetTranslate(GfVec3d(translate));
          transforms[i] = translateMat * transforms[i];
        }
      }
    }

    // "rotate" holds a quaternion in <real, i, j, k> format for each index.
    if (_primvarMap.count(HdInstancerTokens->rotate) > 0) {
      for (size_t i = 0; i < instanceIndices.size(); ++i) {
        if(instanceIndices[i]>=0 && instanceIndices[i]<_rotations.size())
        {
          GfVec4f quat = _rotations[i];
          GfMatrix4d rotateMat(1);
          rotateMat.SetRotate(GfRotation(GfQuaternion(
              quat[0], GfVec3d(quat[1], quat[2], quat[3]))));
          transforms[i] = rotateMat * transforms[i];
        }
      }
    }
  
    // "scale" holds an axis-aligned scale vector for each index.
    if (_primvarMap.count(HdInstancerTokens->scale) > 0) {
      for (size_t i = 0; i < instanceIndices.size(); ++i) {
        if(instanceIndices[i]>=0 && instanceIndices[i]<_scales.size())
        {
          GfVec3f scale = _scales[i];
          GfMatrix4d scaleMat(1);
          scaleMat.SetScale(GfVec3d(scale));
          transforms[i] = scaleMat * transforms[i];
        }
      }
    }

    // "instanceTransform" holds a 4x4 transform matrix for each index.
    if (_primvarMap.count(HdInstancerTokens->instanceTransform) > 0) {

      for (size_t i = 0; i < instanceIndices.size(); ++i) {
        if(instanceIndices[i]>=0 && instanceIndices[i]<_xforms.size()) {
          transforms[i] = _xforms[i] * transforms[i];
        }
      }
    }

    if (GetParentId().IsEmpty()) {
        return transforms;
    }

    HdInstancer *parentInstancer =
        GetDelegate()->GetRenderIndex().GetInstancer(GetParentId());
    if (!TF_VERIFY(parentInstancer)) {
        return transforms;
    }

    // The transforms taking nesting into account are computed by:
    // parentTransforms = parentInstancer->ComputeInstanceTransforms(GetId())
    // foreach (parentXf : parentTransforms, xf : transforms) {
    //     parentXf * xf
    // }
    VtMatrix4dArray parentTransforms =
        static_cast<LoFiInstancer*>(parentInstancer)->
            ComputeInstanceTransforms(GetId());

    VtMatrix4dArray final(parentTransforms.size() * transforms.size());
    for (size_t i = 0; i < parentTransforms.size(); ++i) {
        for (size_t j = 0; j < transforms.size(); ++j) {
            final[i * transforms.size() + j] = transforms[j] *
                                               parentTransforms[i];
        }
    }
    return final;
}

PXR_NAMESPACE_CLOSE_SCOPE

