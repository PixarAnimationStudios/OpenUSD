//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/types.h"
#include "pxr/base/gf/vec3f.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

bool testHdVec4f_2_10_10_10_REV()
{
    // Test round tripping between GfVec3f and HdVec4f_2_10_10_10_REV.
    GfVec3f a(-0.1617791586913686, -0.2533272416818153, 0.9537572083266245);
    GfVec3f b(0.12954827567352645, -0.8348099306719063, 0.5350790819323653);

    GfVec3f a_rt = HdVec4f_2_10_10_10_REV(a).GetAsVec<GfVec3f>();
    GfVec3f b_rt = HdVec4f_2_10_10_10_REV(b).GetAsVec<GfVec3f>();

    const float eps = 0.01;
    bool a_ok = (fabsf(a[0] - a_rt[0]) < eps) &&
                (fabsf(a[1] - a_rt[1]) < eps) &&
                (fabsf(a[2] - a_rt[2]) < eps);
    bool b_ok = (fabsf(b[0] - b_rt[0]) < eps) &&
                (fabsf(b[1] - b_rt[1]) < eps) &&
                (fabsf(b[2] - b_rt[2]) < eps);

    std::cout << "Vec3 -> HdVec4f_2_10_10_10_REV -> Vec3:\n"
              << "\t" << a << " -> " << a_rt << (a_ok ? " OK" : " FAIL") << "\n"
              << "\t" << b << " -> " << b_rt << (b_ok ? " OK" : " FAIL") << "\n";

    return a_ok && b_ok;
}

int main()
{
    if (!testHdVec4f_2_10_10_10_REV()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
