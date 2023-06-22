#ifndef PXR_IMAGING_HGI_MEMORYHELPER_H
#define PXR_IMAGING_HGI_MEMORYHELPER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hd/bufferSpec.h"

PXR_NAMESPACE_OPEN_SCOPE

struct HdBufferSpec;

struct MemberMemorySpec
{
   TfToken name;
   HdTupleType tupleType;
   uint32_t offset;
   uint32_t stride;
};

struct StructMemorySpec
{
   std::vector<MemberMemorySpec> members;
   uint32_t structStride;
};

/// <summary>
/// \class HgiMemoryHelper
/// This is meant to help HDStorm calculate memory layouts
/// in a way that is Hgi dependent / compatible, 
/// e.g. account for some of the differences between OpenGL, DirectX
/// </summary>
class HgiMemoryHelper
{
public:
   HgiMemoryHelper() {};
   virtual ~HgiMemoryHelper() {};

   virtual void GetMemorySpec(const std::vector<HdBufferSpec>& structSpec, StructMemorySpec& sms) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif