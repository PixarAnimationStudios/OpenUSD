//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
