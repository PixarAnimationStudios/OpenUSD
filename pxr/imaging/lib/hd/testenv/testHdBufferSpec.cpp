#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

void BufferSpecTest()
{
    // test comparison operators
    {
        TF_VERIFY(HdBufferSpec(HdTokens->points, GL_FLOAT, 3)
                  == HdBufferSpec(HdTokens->points, GL_FLOAT, 3));
        TF_VERIFY(HdBufferSpec(HdTokens->points, GL_FLOAT, 3)
                  != HdBufferSpec(HdTokens->points, GL_FLOAT, 4));
        TF_VERIFY(HdBufferSpec(HdTokens->points, GL_FLOAT, 3)
                  != HdBufferSpec(HdTokens->normals, GL_FLOAT, 3));
        TF_VERIFY(HdBufferSpec(HdTokens->points, GL_FLOAT, 3)
                  != HdBufferSpec(HdTokens->points, GL_DOUBLE, 3));

        TF_VERIFY(not (HdBufferSpec(HdTokens->points, GL_FLOAT, 3)
                       < HdBufferSpec(HdTokens->points, GL_FLOAT, 3)));
        TF_VERIFY(HdBufferSpec(HdTokens->normals, GL_FLOAT, 3)
                  < HdBufferSpec(HdTokens->points, GL_FLOAT, 3));
        TF_VERIFY(HdBufferSpec(HdTokens->points, GL_FLOAT, 3)
                  < HdBufferSpec(HdTokens->points, GL_DOUBLE, 3));
        TF_VERIFY(HdBufferSpec(HdTokens->points, GL_FLOAT, 3)
                  < HdBufferSpec(HdTokens->points, GL_FLOAT, 4));
    }

    // test set operations
    {
        HdBufferSpecVector spec1;
        HdBufferSpecVector spec2;

        spec1.push_back(HdBufferSpec(HdTokens->points, GL_FLOAT, 3));
        spec1.push_back(HdBufferSpec(HdTokens->color, GL_FLOAT, 4));

        spec2.push_back(HdBufferSpec(HdTokens->points, GL_FLOAT, 3));

        TF_VERIFY(HdBufferSpec::IsSubset(spec2, spec1) == true);
        TF_VERIFY(HdBufferSpec::IsSubset(spec1, spec2) == false);

        spec2.push_back(HdBufferSpec(HdTokens->normals, GL_FLOAT, 4));

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
