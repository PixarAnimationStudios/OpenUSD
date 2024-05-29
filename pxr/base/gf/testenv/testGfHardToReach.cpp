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
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"

#include "pxr/base/tf/diagnostic.h"

#include <cmath>
#include <limits>
#include <type_traits>

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
