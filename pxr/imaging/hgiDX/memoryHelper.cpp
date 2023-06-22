
//
// Copyright 2023 Pixar
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


#include "pch.h"
#include "pxr/imaging/hgiDX/memoryHelper.h"

///
/// Here's some info about HLSL
/// https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
/// 

PXR_NAMESPACE_OPEN_SCOPE

namespace {
   //
   // All documentation says Dx expects data in memory to be aligned at 16 bytes 
   // but in practice I see it does not.
   // It quite possible that either I am misunderstanding something, or the documentation refers to 
   // some other case, or only some particular type of data (this would be the worst case scenario)
   //
   // Anyway I have a number of models who clearly indicate data is read in the shaders incorrectly 
   // if I try to align it at 16 bytes (2 lines, window from atrium model)


   //const size_t c_blockSize = 16;
   const size_t c_blockSize = 4;
}


HgiDXMemoryHelper::HgiDXMemoryHelper()
{
}

HgiDXMemoryHelper::~HgiDXMemoryHelper()
{
}

void 
HgiDXMemoryHelper::GetMemorySpec(const std::vector<HdBufferSpec>& structSpec, StructMemorySpec& sms)
{
   size_t lastBlockOccupancy = 0;

   size_t nStructMemberCount = structSpec.size();
   sms.members.resize(nStructMemberCount);
   sms.structStride = 0;

   for(size_t i=0; i< nStructMemberCount; i++)
   {
      const HdBufferSpec& bsCurr = structSpec[i];
      MemberMemorySpec& mbrMemSpec = sms.members[i];

      size_t currSizeInBytes = HdDataSizeOfTupleType(bsCurr.tupleType);

      mbrMemSpec.name = bsCurr.name;
      mbrMemSpec.tupleType = bsCurr.tupleType;
      mbrMemSpec.offset = sms.structStride;
      mbrMemSpec.stride = currSizeInBytes;
      sms.structStride += currSizeInBytes;

      if (lastBlockOccupancy + currSizeInBytes > c_blockSize)
      {
         if (lastBlockOccupancy > 0)
         {
            //
            // I will have to add a padding (= increase stride) of the previous member
            size_t padding = c_blockSize - lastBlockOccupancy;

            //
            // In the existing code this value is not used at all...
            //sms.members[i - 1].stride += padding; // if occupancy > 0 this cannot be at i==0
            sms.structStride += padding;
            mbrMemSpec.offset += padding;

            lastBlockOccupancy = 0;
         }
      }

      lastBlockOccupancy = (lastBlockOccupancy + currSizeInBytes) % c_blockSize;
   }

   /* Experimentally, when creating a scene in which I forgot to set the color for some ents, 
   * I notice the constantPrimvars UAV, 
   * when it ended with a single int, 
   * it loads the next set of data from directly after the first, 
   * it does not expect an offset that would allign the entire structure to 16 bytes, so...
   int nFinalOccupancy = sms.structStride % c_blockSize;
   if (nFinalOccupancy > 0)
      sms.structStride += (c_blockSize - nFinalOccupancy);*/
}

size_t 
HgiDXMemoryHelper::RoundUp(size_t neededSize)
{
   if(0 == (neededSize % 16))
      return neededSize;
   
   size_t nBlocks = neededSize / 16 + 1;
   return nBlocks * 16;
}

PXR_NAMESPACE_CLOSE_SCOPE