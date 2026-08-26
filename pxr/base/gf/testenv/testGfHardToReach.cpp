//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/numericCast.h"
#include "pxr/base/gf/size2.h"
#include "pxr/base/gf/size3.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/tf/diagnostic.h"

#include <cmath>
#include <limits>
#include <type_traits>

PXR_NAMESPACE_USING_DIRECTIVE

// Each swizzle accessor is checked against the components its name gives,
// rather than against literal values, so that one macro covers every vector
// type: the expected type comes from decltype(), which makes the check
// independent of both the scalar type and the result dimension.  The literal
// checks in main() cover what these cannot -- an error in the components
// themselves.
#define CHECK_SWZ2(v, a, b)                                     \
    TF_AXIOM((v.a##b()) == decltype(v.a##b())(v.a, v.b))
#define CHECK_SWZ3(v, a, b, c)                                          \
    TF_AXIOM((v.a##b##c()) == decltype(v.a##b##c())(v.a, v.b, v.c))
#define CHECK_SWZ4(v, a, b, c, d)                                              \
    TF_AXIOM((v.a##b##c##d()) == decltype(v.a##b##c##d())(v.a, v.b, v.c, v.d))

// All 2 swizzles of a 2-dimensional vector.
template <class Vec>
static void
_CheckDim2Swizzles(Vec const &v)
{
    // 2 of length 2
    CHECK_SWZ2(v, x, y); CHECK_SWZ2(v, y, x);
}

// All 12 swizzles of a 3-dimensional vector.
template <class Vec>
static void
_CheckDim3Swizzles(Vec const &v)
{
    // 6 of length 2
    CHECK_SWZ2(v, x, y); CHECK_SWZ2(v, x, z); CHECK_SWZ2(v, y, x);
    CHECK_SWZ2(v, y, z); CHECK_SWZ2(v, z, x); CHECK_SWZ2(v, z, y);

    // 6 of length 3
    CHECK_SWZ3(v, x, y, z); CHECK_SWZ3(v, x, z, y); CHECK_SWZ3(v, y, x, z);
    CHECK_SWZ3(v, y, z, x); CHECK_SWZ3(v, z, x, y); CHECK_SWZ3(v, z, y, x);
}

// All 60 swizzles of a 4-dimensional vector.
template <class Vec>
static void
_CheckDim4Swizzles(Vec const &v)
{
    // 12 of length 2
    CHECK_SWZ2(v, x, y); CHECK_SWZ2(v, x, z); CHECK_SWZ2(v, x, w);
    CHECK_SWZ2(v, y, x); CHECK_SWZ2(v, y, z); CHECK_SWZ2(v, y, w);
    CHECK_SWZ2(v, z, x); CHECK_SWZ2(v, z, y); CHECK_SWZ2(v, z, w);
    CHECK_SWZ2(v, w, x); CHECK_SWZ2(v, w, y); CHECK_SWZ2(v, w, z);

    // 24 of length 3
    CHECK_SWZ3(v, x, y, z); CHECK_SWZ3(v, x, y, w); CHECK_SWZ3(v, x, z, y);
    CHECK_SWZ3(v, x, z, w); CHECK_SWZ3(v, x, w, y); CHECK_SWZ3(v, x, w, z);
    CHECK_SWZ3(v, y, x, z); CHECK_SWZ3(v, y, x, w); CHECK_SWZ3(v, y, z, x);
    CHECK_SWZ3(v, y, z, w); CHECK_SWZ3(v, y, w, x); CHECK_SWZ3(v, y, w, z);
    CHECK_SWZ3(v, z, x, y); CHECK_SWZ3(v, z, x, w); CHECK_SWZ3(v, z, y, x);
    CHECK_SWZ3(v, z, y, w); CHECK_SWZ3(v, z, w, x); CHECK_SWZ3(v, z, w, y);
    CHECK_SWZ3(v, w, x, y); CHECK_SWZ3(v, w, x, z); CHECK_SWZ3(v, w, y, x);
    CHECK_SWZ3(v, w, y, z); CHECK_SWZ3(v, w, z, x); CHECK_SWZ3(v, w, z, y);

    // 24 of length 4
    CHECK_SWZ4(v, x, y, z, w); CHECK_SWZ4(v, x, y, w, z);
    CHECK_SWZ4(v, x, z, y, w); CHECK_SWZ4(v, x, z, w, y);
    CHECK_SWZ4(v, x, w, y, z); CHECK_SWZ4(v, x, w, z, y);
    CHECK_SWZ4(v, y, x, z, w); CHECK_SWZ4(v, y, x, w, z);
    CHECK_SWZ4(v, y, z, x, w); CHECK_SWZ4(v, y, z, w, x);
    CHECK_SWZ4(v, y, w, x, z); CHECK_SWZ4(v, y, w, z, x);
    CHECK_SWZ4(v, z, x, y, w); CHECK_SWZ4(v, z, x, w, y);
    CHECK_SWZ4(v, z, y, x, w); CHECK_SWZ4(v, z, y, w, x);
    CHECK_SWZ4(v, z, w, x, y); CHECK_SWZ4(v, z, w, y, x);
    CHECK_SWZ4(v, w, x, y, z); CHECK_SWZ4(v, w, x, z, y);
    CHECK_SWZ4(v, w, y, x, z); CHECK_SWZ4(v, w, y, z, x);
    CHECK_SWZ4(v, w, z, x, y); CHECK_SWZ4(v, w, z, y, x);
}

#undef CHECK_SWZ2
#undef CHECK_SWZ3
#undef CHECK_SWZ4

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
        const GfVec4f v(vals);
        TF_AXIOM(v == GfVec4f(1,2,3,4));
        float const *f = v.GetArray();
        TF_AXIOM(f[0] == 1 && f[1] == 2 && f[2] == 3 && f[3] == 4);
    }

    // Swizzles.
    {
        // Components, and their agreement with operator[] and data().
        GfVec4f v(1, 2, 3, 4);
        TF_AXIOM(v.x == 1 && v.y == 2 && v.z == 3 && v.w == 4);
        TF_AXIOM(v.data() == &v.x);
        TF_AXIOM(sizeof(GfVec4f) == GfVec4f::dimension * sizeof(float));
        for (size_t i = 0; i != GfVec4f::dimension; ++i) {
            TF_AXIOM(v[i] == v.data()[i]);
        }

        // Components are writable, and alias operator[].
        v.z = 30;
        TF_AXIOM(v[2] == 30);
        v[3] = 40;
        TF_AXIOM(v.w == 40);
        TF_AXIOM(v == GfVec4f(1, 2, 30, 40));
    }
    {
        // Literal checks.  The mapping checks below are self-consistent, so
        // these are what would catch an error in the components.
        GfVec4f v(1, 2, 3, 4);
        TF_AXIOM(v.xy() == GfVec2f(1, 2));
        TF_AXIOM(v.yx() == GfVec2f(2, 1));
        TF_AXIOM(v.zxw() == GfVec3f(3, 1, 4));
        TF_AXIOM(v.wzyx() == GfVec4f(4, 3, 2, 1));
        TF_AXIOM(v.xyzw() == v);

        GfVec3d d(1.5, 2.5, 3.5);
        TF_AXIOM(d.zy() == GfVec2d(3.5, 2.5));
        TF_AXIOM(d.zyx() == GfVec3d(3.5, 2.5, 1.5));

        GfVec2i i(7, 8);
        TF_AXIOM(i.yx() == GfVec2i(8, 7));
    }
    {
        // Every swizzle of every vector type: 74 names per scalar type, 296 in
        // all, each against the components its name gives.
        _CheckDim2Swizzles(GfVec2d(1.5, 2.5));
        _CheckDim2Swizzles(GfVec2f(1.5f, 2.5f));
        _CheckDim2Swizzles(GfVec2h(1.5f, 2.5f));
        _CheckDim2Swizzles(GfVec2i(1, 2));

        _CheckDim3Swizzles(GfVec3d(1.5, 2.5, 3.5));
        _CheckDim3Swizzles(GfVec3f(1.5f, 2.5f, 3.5f));
        _CheckDim3Swizzles(GfVec3h(1.5f, 2.5f, 3.5f));
        _CheckDim3Swizzles(GfVec3i(1, 2, 3));

        _CheckDim4Swizzles(GfVec4d(1.5, 2.5, 3.5, 4.5));
        _CheckDim4Swizzles(GfVec4f(1.5f, 2.5f, 3.5f, 4.5f));
        _CheckDim4Swizzles(GfVec4h(1.5f, 2.5f, 3.5f, 4.5f));
        _CheckDim4Swizzles(GfVec4i(1, 2, 3, 4));
    }
    {
        // Composing a permutation with its inverse is the identity.
        GfVec4f v(1, 2, 3, 4);
        TF_AXIOM(v.wzyx().wzyx() == v);
        TF_AXIOM(v.yxwz().yxwz() == v);
        TF_AXIOM(v.xy().yx() == v.yx());

        GfVec3f u(1, 2, 3);
        TF_AXIOM(u.zxy().yzx() == u);
        TF_AXIOM(u.zyx().zyx() == u);
    }
    {
        // Swizzle results are ordinary vectors, so every existing operator and
        // free function applies.
        GfVec4f v(1, 2, 3, 4), w(10, 20, 30, 40);
        TF_AXIOM(v.xy() * 2.0 == GfVec2f(2, 4));
        TF_AXIOM(v.xy() / 2.0 == GfVec2f(0.5f, 1));
        TF_AXIOM(v.xy() + w.zw() == GfVec2f(31, 42));
        TF_AXIOM(v.zw() - v.xy() == GfVec2f(2, 2));
        TF_AXIOM(-v.yx() == GfVec2f(-2, -1));
        TF_AXIOM(GfDot(v.xy(), w.xy()) == 50);
        TF_AXIOM(GfCross(v.xyz(), v.xyz()) == GfVec3f(0));
        TF_AXIOM(GfCompMult(v.xy(), w.xy()) == GfVec2f(10, 40));
        TF_AXIOM(v.xy().GetLengthSq() == 5);
        // 3-4-5 triangle, so the length is exact.
        TF_AXIOM(v.zw().GetLength() == 5);

        // Comparison works in both operand orders.
        TF_AXIOM(v.xy() == GfVec2f(1, 2));
        TF_AXIOM(GfVec2f(1, 2) == v.xy());
        TF_AXIOM(v.xy() != w.xy());
    }
    {
        // Result types are the matching vector of the same scalar type.
        static_assert(
            std::is_same<decltype(GfVec4f().xy()), GfVec2f>::value, "");
        static_assert(
            std::is_same<decltype(GfVec4d().zyx()), GfVec3d>::value, "");
        static_assert(
            std::is_same<decltype(GfVec4i().wzyx()), GfVec4i>::value, "");
        static_assert(
            std::is_same<decltype(GfVec3h().yx()), GfVec2h>::value, "");
    }
    {
        // Components and swizzles work in constant expressions.  Neither is
        // possible if components are union members.
        constexpr GfVec4f c(1, 2, 3, 4);
        static_assert(c.x == 1 && c.y == 2 && c.z == 3 && c.w == 4, "");
        constexpr GfVec2f p = c.zx();
        static_assert(p.x == 3 && p.y == 1, "");
        constexpr GfVec4f q = c.wzyx();
        static_assert(q.x == 4 && q.y == 3 && q.z == 2 && q.w == 1, "");
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

        TF_AXIOM(pxr_half::hash_value(GfHalf(1.0f)) ==
                 pxr_half::hash_value(GfHalf(1.0f)));
        TF_AXIOM(pxr_half::hash_value(GfHalf(1.0f)) ==
                 hash_value(GfHalf(1.0f)));
    }

    // numeric cast
    {
        GfNumericCastFailureType failType;

        TF_AXIOM(GfNumericCast<int>(0).value() == 0);
        TF_AXIOM(GfNumericCast<int>(123).value() == 123);
        TF_AXIOM(GfNumericCast<int>(-123).value() == -123);

        TF_AXIOM(GfNumericCast<unsigned>(0).value() == 0);
        TF_AXIOM(GfNumericCast<unsigned>(123).value() == 123u);
        TF_AXIOM(!GfNumericCast<unsigned>(-123));

        TF_AXIOM(!GfNumericCast<int16_t>(100000, &failType));
        TF_AXIOM(failType == GfNumericCastPosOverflow);
        TF_AXIOM(!GfNumericCast<int16_t>(-100000, &failType));
        TF_AXIOM(failType == GfNumericCastNegOverflow);

        TF_AXIOM(!GfNumericCast<uint16_t>(-1, &failType));
        TF_AXIOM(failType == GfNumericCastNegOverflow);
        TF_AXIOM(!GfNumericCast<uint16_t>(100000, &failType));
        TF_AXIOM(failType == GfNumericCastPosOverflow);

        // signed -> unsigned
        TF_AXIOM(GfNumericCast<unsigned>(0).value() == 0);
        TF_AXIOM(!GfNumericCast<unsigned>(-1, &failType) &&
                 failType == GfNumericCastNegOverflow);

        TF_AXIOM(GfNumericCast<unsigned>(
                     std::numeric_limits<int>::max()).value() ==
                 static_cast<unsigned>(
                     std::numeric_limits<int>::max()));

        // unsigned -> signed
        TF_AXIOM(GfNumericCast<int>(0u).value() == 0);
        TF_AXIOM(!GfNumericCast<int>(
                     std::numeric_limits<unsigned>::max(), &failType) &&
                 failType == GfNumericCastPosOverflow);
        TF_AXIOM(GfNumericCast<int>(
                     static_cast<unsigned>(
                         std::numeric_limits<int>::max())).value() ==
                 std::numeric_limits<int>::max());
        TF_AXIOM(!GfNumericCast<int>(
                     static_cast<unsigned>(
                         std::numeric_limits<int>::max())+1, &failType) &&
                 failType == GfNumericCastPosOverflow);

        // float -> int
        TF_AXIOM(GfNumericCast<int16_t>(12.34f).value() == 12);
        TF_AXIOM(GfNumericCast<int16_t>(-12.34f).value() == -12);
        TF_AXIOM(GfNumericCast<int16_t>(12.99f).value() == 12);
        TF_AXIOM(GfNumericCast<int16_t>(-12.99f).value() == -12);

        TF_AXIOM(!GfNumericCast<int16_t>(100000.0f, &failType));
        TF_AXIOM(failType == GfNumericCastPosOverflow);
        TF_AXIOM(!GfNumericCast<int16_t>(-100000.0f, &failType));
        TF_AXIOM(failType == GfNumericCastNegOverflow);

        TF_AXIOM(!GfNumericCast<uint32_t>(-1.0f, &failType));
        TF_AXIOM(failType == GfNumericCastNegOverflow);

        TF_AXIOM(GfNumericCast<int16_t>(
                     static_cast<float>(
                         std::numeric_limits<int16_t>::max())
                     ).value() == std::numeric_limits<int16_t>::max());

        TF_AXIOM(!GfNumericCast<int16_t>(
                     static_cast<float>(
                         std::numeric_limits<int16_t>::max()) + 1.0f,
                     &failType) && failType == GfNumericCastPosOverflow);

        TF_AXIOM(GfNumericCast<int16_t>(
                     static_cast<float>(
                         std::numeric_limits<int16_t>::lowest())
                     ).value() == std::numeric_limits<int16_t>::lowest());

        TF_AXIOM(!GfNumericCast<int16_t>(
                     static_cast<float>(
                         std::numeric_limits<int16_t>::lowest()) - 1.0f,
                     &failType) && failType == GfNumericCastNegOverflow);

        // bool -> int
        TF_AXIOM(GfNumericCast<int>(true).value() == 1);
        TF_AXIOM(GfNumericCast<int>(false).value() == 0);

        // int -> bool
        TF_AXIOM(GfNumericCast<bool>(0).value() == false);
        TF_AXIOM(GfNumericCast<bool>(1).value() == true);

        TF_AXIOM(GfNumericCast<bool>(-1).value() == true);
        TF_AXIOM(GfNumericCast<bool>(2).value() == true);

        TF_AXIOM(GfNumericCast<bool>(
                    static_cast<unsigned>(0)).value() == false);
        TF_AXIOM(GfNumericCast<bool>(
                    static_cast<unsigned>(1)).value() == true);

        TF_AXIOM(GfNumericCast<bool>(
                    static_cast<unsigned>(2)).value() == true);
        TF_AXIOM(GfNumericCast<bool>(
                     std::numeric_limits<unsigned>::max()).value() == true);

        // bool -> float
        TF_AXIOM(GfNumericCast<float>(true) == 1.0f);
        TF_AXIOM(GfNumericCast<float>(false) == 0.0f);
        
        // float -> bool
        TF_AXIOM(GfNumericCast<bool>(0.0f).value() == false);
        TF_AXIOM(GfNumericCast<bool>(-0.0f).value() == false);
        TF_AXIOM(GfNumericCast<bool>(1.0f).value() == true);

        TF_AXIOM(GfNumericCast<bool>(-1.0f).value() == true);
        TF_AXIOM(GfNumericCast<bool>(0.5f).value() == true);
        TF_AXIOM(GfNumericCast<bool>(2.0f).value() == true);
        
        // unsigned
        TF_AXIOM(GfNumericCast<uint16_t>(
                     static_cast<float>(
                         std::numeric_limits<uint16_t>::max())
                     ).value() == std::numeric_limits<uint16_t>::max());

        TF_AXIOM(!GfNumericCast<uint16_t>(
                     static_cast<float>(
                         std::numeric_limits<uint16_t>::max()) + 1.0f,
                     &failType) && failType == GfNumericCastPosOverflow);
        
        TF_AXIOM(GfNumericCast<uint16_t>(
                     static_cast<float>(
                         std::numeric_limits<uint16_t>::lowest())
                     ).value() == std::numeric_limits<uint16_t>::lowest());
        
        TF_AXIOM(!GfNumericCast<uint16_t>(
                     static_cast<float>(
                         std::numeric_limits<uint16_t>::lowest()) - 1.0f,
                     &failType) && failType == GfNumericCastNegOverflow);
        
        // float min & denorm_min
        TF_AXIOM(GfNumericCast<int>(
                     std::numeric_limits<float>::min()).value() == 0);
        TF_AXIOM(GfNumericCast<int>(
                     std::numeric_limits<float>::denorm_min()).value() == 0);

        // float inf & nan.
        auto inf = std::numeric_limits<float>::infinity();
        auto nan = std::numeric_limits<float>::quiet_NaN();

        TF_AXIOM(!GfNumericCast<int32_t>(inf, &failType) &&
                 failType == GfNumericCastPosOverflow);

        TF_AXIOM(!GfNumericCast<int32_t>(-inf, &failType) &&
                 failType == GfNumericCastNegOverflow);

        TF_AXIOM(!GfNumericCast<int32_t>(nan, &failType) &&
                 failType == GfNumericCastNaN);

        // int -> GfHalf where the int values are out-of-range produce
        // infinities.
        TF_AXIOM(GfNumericCast<GfHalf>(1000000).value() == GfHalf::posInf());
        TF_AXIOM(GfNumericCast<GfHalf>(-1000000).value() == GfHalf::negInf());

        // double -> float where the double values are out-of-range produce
        // infinities.
        float floatHighest = std::numeric_limits<float>::max();
        float floatLowest = std::numeric_limits<float>::lowest();
        double doubleInf = std::numeric_limits<double>::infinity();

        // Interestingly in round-to-nearest ieee754 mode, a few doubles greater
        // than float max will round to float max rather than inf, so we allow
        // either behavior here.
        
        // The next double toward positive infinity after highest float.
        double testValue = std::nextafter(
            static_cast<double>(floatHighest), doubleInf);
        
        TF_AXIOM(GfNumericCast<float>(testValue).value() == inf ||
                 GfNumericCast<float>(testValue).value() == floatHighest);

        // The next double toward negative infinity after lowest float.
        testValue = std::nextafter(
            static_cast<double>(floatLowest), -doubleInf);
        TF_AXIOM(GfNumericCast<float>(testValue).value() == -inf ||
                 GfNumericCast<float>(testValue).value() == floatLowest);

        // Twice float highest & lowest.
        testValue = static_cast<double>(floatHighest) * 2.0;
        TF_AXIOM(GfNumericCast<float>(testValue).value() == inf);
        testValue = static_cast<double>(floatLowest) * 2.0;
        TF_AXIOM(GfNumericCast<float>(testValue).value() == -inf);

        // Double lowest/highest.
        TF_AXIOM(GfNumericCast<float>(
                     std::numeric_limits<double>::max()).value() == inf); 
        TF_AXIOM(GfNumericCast<float>(
                     std::numeric_limits<double>::lowest()).value() == -inf);
    }

    printf("OK\n");

    return 0;
}

template <class T>
struct _CheckTraits
{
    static_assert(std::is_trivial<T>::value, "");
};

template struct _CheckTraits<GfVec2d>;
template struct _CheckTraits<GfVec2f>;
template struct _CheckTraits<GfVec2h>;
template struct _CheckTraits<GfVec2i>;
template struct _CheckTraits<GfVec3d>;
template struct _CheckTraits<GfVec3f>;
template struct _CheckTraits<GfVec3h>;
template struct _CheckTraits<GfVec3i>;
template struct _CheckTraits<GfVec4d>;
template struct _CheckTraits<GfVec4f>;
template struct _CheckTraits<GfVec4h>;
template struct _CheckTraits<GfVec4i>;

template struct _CheckTraits<GfMatrix2d>;
template struct _CheckTraits<GfMatrix3d>;
template struct _CheckTraits<GfMatrix4d>;

template struct _CheckTraits<GfMatrix2f>;
template struct _CheckTraits<GfMatrix3f>;
template struct _CheckTraits<GfMatrix4f>;
