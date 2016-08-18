#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>

int main()
{
    TfErrorMark mark;

    VtValue v;
    HdVtBufferSource b(HdTokens->points, v);

    // Above could throw errors.
    mark.Clear();

    bool valid = b.IsValid();

    std::cout << "Buffer is ";
    std::cout << (valid ? "Valid" : "Invalid");
    std::cout << "\n";


    if (mark.IsClean() and (not valid)) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}

