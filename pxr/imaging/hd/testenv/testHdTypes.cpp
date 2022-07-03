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
