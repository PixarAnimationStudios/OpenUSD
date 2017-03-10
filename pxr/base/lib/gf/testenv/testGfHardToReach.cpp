//
// Copyright 2016 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/size2.h"
#include "pxr/base/gf/size3.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_USING_DIRECTIVE

int
main(int argc, char *argv[])
{
    // GfVec2f
    {
        float vals[] = { 1.0f, 2.0f };
        GfVec2f v(vals);
        TF_AXIOM(v == GfVec2f(1,2));
        float const *f = v.GetArray();
        TF_AXIOM(f[0] == 1 && f[1] == 2);
    }

    // GfVec2i
    {
        int vals[] = { 1, 2 };
        GfVec2i v(vals);
        TF_AXIOM(v == GfVec2i(1,2));
        int const *i = v.GetArray();
        TF_AXIOM(i[0] == 1 && i[1] == 2);
        v.Set(0, 1);
        TF_AXIOM(v == GfVec2i(0,1));
    }

    // GfVec3i
    {
        int vals[] = { 1, 2, 3 };
        GfVec3i v(vals);
        TF_AXIOM(v == GfVec3i(1,2,3));
        int const *i = v.GetArray();
        TF_AXIOM(i[0] == 1 && i[1] == 2 && i[2] == 3);
        v.Set(0, 1, 2);
        TF_AXIOM(v == GfVec3i(0,1,2));
    }

    // GfVec4i
    {
        int vals[] = { 1, 2, 3, 4 };
        GfVec4i v(vals);
        TF_AXIOM(v == GfVec4i(1,2,3,4));
        int const *i = v.GetArray();
        TF_AXIOM(i[0] == 1 && i[1] == 2 && i[2] == 3 && i[3] == 4);
        v.Set(0, 1, 2, 3);
        TF_AXIOM(v == GfVec4i(0,1,2,3));
    }

    // GfVec3f
    {
        float vals[] = { 1.0f, 2.0f, 3.0f };
        GfVec3f v(vals);
        TF_AXIOM(v == GfVec3f(1,2,3));
        float const *f = v.GetArray();
        TF_AXIOM(f[0] == 1 && f[1] == 2 && f[2] == 3);
    }

    // GfVec4f
    {
        float vals[] = { 1.0f, 2.0f, 3.0f, 4.0f };
        GfVec4f v(vals);
        TF_AXIOM(v == GfVec4f(1,2,3,4));
        float const *f = v.GetArray();
        TF_AXIOM(f[0] == 1 && f[1] == 2 && f[2] == 3 && f[3] == 4);
    }

    // GfSize2, GfSize3
    {
        size_t vals[] = {1, 2, 3};
        TF_AXIOM(GfSize2(vals) == GfSize2(1,2));
        TF_AXIOM(GfSize3(vals) == GfSize3(1,2,3));
    }

    // GfMatrix2d
    {
        double vals[2][2] = {{1, 0},
                             {0, 1}};
        TF_AXIOM(GfMatrix2d(vals) == GfMatrix2d(1));
        GfMatrix2d m(vals);
        double const *d = m.GetArray();
        TF_AXIOM(d[0] == 1 && d[1] == 0 &&
                 d[2] == 0 && d[3] == 1);
    }

    // GfMatrix2f
    {
        float vals[2][2] = {{1, 0},
                             {0, 1}};
        TF_AXIOM(GfMatrix2f(vals) == GfMatrix2f(1));
        GfMatrix2f m(vals);
        float const *f = m.GetArray();
        TF_AXIOM(f[0] == 1 && f[1] == 0 &&
                 f[2] == 0 && f[3] == 1);
    }

    // GfMatrix3d
    {
        double vals[3][3] = {{1, 0, 0},
                             {0, 1, 0},
                             {0, 0, 1}};
        TF_AXIOM(GfMatrix3d(vals) == GfMatrix3d(1));
        GfMatrix3d m(vals);
        double const *d = m.GetArray();
        TF_AXIOM(d[0] == 1 && d[1] == 0 && d[2] == 0 &&
                 d[3] == 0 && d[4] == 1 && d[5] == 0 &&
                 d[6] == 0 && d[7] == 0 && d[8] == 1);
    }

    // GfMatrix4d
    {
        double vals[4][4] = {{1, 0, 0, 0},
                             {0, 1, 0, 0},
                             {0, 0, 1, 0},
                             {0, 0, 0, 1}};
        TF_AXIOM(GfMatrix4d(vals) == GfMatrix4d(1));
        GfMatrix4d m(vals);
        double const *d = m.GetArray();
        TF_AXIOM(d[ 0] == 1 && d[ 1] == 0 && d[ 2] == 0 && d[ 3] == 0 &&
                 d[ 4] == 0 && d[ 5] == 1 && d[ 6] == 0 && d[ 7] == 0 &&
                 d[ 8] == 0 && d[ 9] == 0 && d[10] == 1 && d[11] == 0 &&
                 d[12] == 0 && d[13] == 0 && d[14] == 0 && d[15] == 1);
    }
    
    // half
    {
        float halfPosInf = GfHalf::posInf();
        TF_AXIOM(!std::isfinite(halfPosInf));
        TF_AXIOM(std::isinf(halfPosInf));

        float halfNegInf = GfHalf::negInf();
        TF_AXIOM(!std::isfinite(halfNegInf));
        TF_AXIOM(std::isinf(halfNegInf));

        float halfqNan = GfHalf::qNan();
        TF_AXIOM(std::isnan(halfqNan));

        float halfsNan = GfHalf::sNan();
        TF_AXIOM(std::isnan(halfsNan));
    }
    
    return 0;
}
