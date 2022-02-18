//
// Copyright 2021 Pixar
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

#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

void BufferSpecTest()
{
    // test comparison operators
    {
        TF_VERIFY(HdBufferSpec(HdTokens->points,
                               HdTupleType { HdTypeFloatVec3, 1 })
                  == HdBufferSpec(HdTokens->points,
                                  HdTupleType { HdTypeFloatVec3, 1 }));
        TF_VERIFY(HdBufferSpec(HdTokens->points,
                               HdTupleType { HdTypeFloatVec3, 1 })
                  != HdBufferSpec(HdTokens->points,
                                  HdTupleType { HdTypeFloatVec4, 1 }));
        TF_VERIFY(HdBufferSpec(HdTokens->points,
                               HdTupleType { HdTypeFloatVec3, 1 })
                  != HdBufferSpec(HdTokens->normals,
                                  HdTupleType { HdTypeFloatVec3, 1 }));
        TF_VERIFY(HdBufferSpec(HdTokens->points,
                               HdTupleType { HdTypeFloatVec3, 1 })
                  != HdBufferSpec(HdTokens->points,
                                  HdTupleType { HdTypeDoubleVec3, 1 }));

        TF_VERIFY(!(HdBufferSpec(HdTokens->points,
                                 HdTupleType { HdTypeFloatVec3, 1 })
                    < HdBufferSpec(HdTokens->points,
                                   HdTupleType { HdTypeFloatVec3, 1 })));
        TF_VERIFY(HdBufferSpec(HdTokens->normals,
                               HdTupleType { HdTypeFloatVec3, 1 })
                  < HdBufferSpec(HdTokens->points,
                                 HdTupleType { HdTypeFloatVec3, 1 }));
        TF_VERIFY(HdBufferSpec(HdTokens->points,
                               HdTupleType { HdTypeFloatVec3, 1 })
                  < HdBufferSpec(HdTokens->points,
                               HdTupleType { HdTypeDoubleVec3, 1 }));
        TF_VERIFY(HdBufferSpec(HdTokens->points,
                               HdTupleType { HdTypeFloatVec3, 1 })
                  < HdBufferSpec(HdTokens->points,
                                 HdTupleType { HdTypeFloatVec4, 1 }));
        }

    // test set operations
    {
        HdBufferSpecVector spec1;
        HdBufferSpecVector spec2;

        spec1.push_back(HdBufferSpec(HdTokens->points,
                                     HdTupleType { HdTypeFloatVec3, 1 }));
        spec1.push_back(HdBufferSpec(HdTokens->displayColor,
                                     HdTupleType { HdTypeFloatVec3, 1 }));

        spec2.push_back(HdBufferSpec(HdTokens->points,
                                     HdTupleType { HdTypeFloatVec3, 1 }));

        TF_VERIFY(HdBufferSpec::IsSubset(spec2, spec1) == true);
        TF_VERIFY(HdBufferSpec::IsSubset(spec1, spec2) == false);

        spec2.push_back(HdBufferSpec(HdTokens->normals,
                                     HdTupleType { HdTypeFloatVec4, 1 }));

        TF_VERIFY(HdBufferSpec::IsSubset(spec2, spec1) == false);
        TF_VERIFY(HdBufferSpec::IsSubset(spec1, spec2) == false);

        HdBufferSpecVector spec3 = HdBufferSpec::ComputeUnion(spec1, spec2);

        TF_VERIFY(HdBufferSpec::IsSubset(spec1, spec3) == true);
        TF_VERIFY(HdBufferSpec::IsSubset(spec2, spec3) == true);
    }
}

int main()
{
    TfErrorMark mark;

    BufferSpecTest();

    TF_VERIFY(mark.IsClean());

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
