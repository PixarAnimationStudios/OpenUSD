//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_BASE_VT_VISIT_VALUE_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_BASE_VT_VISIT_VALUE_H

// XXX: Delete this file after hdPrman drops support for USD versions
// older than 22.11.
// XXX: When this file is deleted, translation units in hdPrman that use
// VtVisitValue will need to add their own include for pxr/base/vt/typeHeaders.h

#include "pxr/pxr.h" // PXR_VERSION
#if PXR_VERSION >= 2211
#include <pxr/base/vt/visitValue.h> // IWYU pragma: export
#else
#if PXR_VERSION >= 2208
#include <pxr/base/vt/typeHeaders.h> // IWYU pragma: export
#else
#include "typeHeaders.h"
#endif
#include "pxr/base/vt/value.h"

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Polyfill for VtVisitValue.
//
// 1. valueHeaders.h was private until 22.08. Since it must be included on
//    translation units that invoke VtVisitValue, we will have to polyfill it.
//
//    1a. The dual quaternion types do not exist until 22.08. We won't polyfill
//        those, so our polyfills of VtVisitValue and valueHeaders.h will have
//        to omit them.
//
// 2. VtValue::GetKnownValueTypeIndex() does not exist until 22.11, so we will
//    use an unordered_map<type_index, int> to assign fixed int indices to
//    types so we can build the switch statement.
//
// 3. Boost is not available, so we have to build the full switch table by hand.
//
// 3. The whole solution needs to be header-only, so no new source files need
//    to be added to the build script.

namespace Vt_ValueVisitDetail {

// These two overloads do SFINAE to detect whether the visitor can be invoked
// with the given held type T.  If the visitor cannot be invoked with T, it is
// instead invoked with the VtValue itself.
template <class T, class Visitor,
    class = decltype(std::declval<Visitor>()(std::declval<T>()))>
auto
Visit(VtValue const &val, Visitor &&visitor, int) {
    return std::forward<Visitor>(visitor)(val.UncheckedGet<T>());
}

template <class T, class Visitor>
auto
Visit(VtValue const &val, Visitor &&visitor, ...) {
    return std::forward<Visitor>(visitor)(val);
}

// VtValue::GetKnowValueTypeIndex() does not exist prior to 22.11 so we will
// hard-code a typeString-to-integer conversion using an unordered_map for
// fast lookup, then switch on the integer in an attempt to recreate the
// performance of the real thing.
static const std::unordered_map<std::type_index, int> types {
    { std::type_index(typeid(bool)),               0 },
    { std::type_index(typeid(char)),               1 },
    { std::type_index(typeid(unsigned char)),      2 },
    { std::type_index(typeid(short)),              3 },
    { std::type_index(typeid(unsigned short)),     4 },
    { std::type_index(typeid(int)),                5 },
    { std::type_index(typeid(unsigned int)),       6 },
    { std::type_index(typeid(int64_t)),            7 },
    { std::type_index(typeid(uint64_t)),           8 },
    { std::type_index(typeid(double)),             9 },
    { std::type_index(typeid(float)),             10 },
    { std::type_index(typeid(GfHalf)),            11 },
    { std::type_index(typeid(std::string)),       12 },
    { std::type_index(typeid(TfToken)),           13 },
    { std::type_index(typeid(VtVec4iArray)),      14 },
    { std::type_index(typeid(VtVec3iArray)),      15 },
    { std::type_index(typeid(VtVec2iArray)),      16 },
    { std::type_index(typeid(VtVec4hArray)),      17 },
    { std::type_index(typeid(VtVec3hArray)),      18 },
    { std::type_index(typeid(VtVec2hArray)),      19 },
    { std::type_index(typeid(VtVec4fArray)),      20 },
    { std::type_index(typeid(VtVec3fArray)),      21 },
    { std::type_index(typeid(VtVec2fArray)),      22 },
    { std::type_index(typeid(VtVec4dArray)),      23 },
    { std::type_index(typeid(VtVec3dArray)),      24 },
    { std::type_index(typeid(VtVec2dArray)),      25 },
    { std::type_index(typeid(VtMatrix4fArray)),   26 },
    { std::type_index(typeid(VtMatrix3fArray)),   27 },
    { std::type_index(typeid(VtMatrix2fArray)),   28 },
    { std::type_index(typeid(VtMatrix4dArray)),   29 },
    { std::type_index(typeid(VtMatrix3dArray)),   30 },
    { std::type_index(typeid(VtMatrix2dArray)),   31 },
    { std::type_index(typeid(VtRange3fArray)),    32 },
    { std::type_index(typeid(VtRange3dArray)),    33 },
    { std::type_index(typeid(VtRange2fArray)),    34 },
    { std::type_index(typeid(VtRange2dArray)),    35 },
    { std::type_index(typeid(VtRange1fArray)),    36 },
    { std::type_index(typeid(VtRange1dArray)),    37 },
    { std::type_index(typeid(VtIntervalArray)),   38 },
    { std::type_index(typeid(VtRect2iArray)),     39 },
    { std::type_index(typeid(VtQuathArray)),      40 },
    { std::type_index(typeid(VtQuatfArray)),      41 },
    { std::type_index(typeid(VtQuatdArray)),      42 },
    { std::type_index(typeid(VtQuaternionArray)), 43 },
#if PXR_VERSION >= 2208
    { std::type_index(typeid(VtDualQuathArray)),  44 },
    { std::type_index(typeid(VtDualQuatfArray)),  45 },
    { std::type_index(typeid(VtDualQuatdArray)),  46 },
#endif
    { std::type_index(typeid(VtBoolArray)),       47 },
    { std::type_index(typeid(VtCharArray)),       48 },
    { std::type_index(typeid(VtUCharArray)),      49 },
    { std::type_index(typeid(VtShortArray)),      50 },
    { std::type_index(typeid(VtUShortArray)),     51 },
    { std::type_index(typeid(VtIntArray)),        52 },
    { std::type_index(typeid(VtUIntArray)),       53 },
    { std::type_index(typeid(VtInt64Array)),      54 },
    { std::type_index(typeid(VtUInt64Array)),     55 },
    { std::type_index(typeid(VtDoubleArray)),     56 },
    { std::type_index(typeid(VtFloatArray)),      57 },
    { std::type_index(typeid(VtHalfArray)),       58 },
    { std::type_index(typeid(VtStringArray)),     59 },
    { std::type_index(typeid(VtTokenArray)),      60 },
    { std::type_index(typeid(GfVec4i)),           61 },
    { std::type_index(typeid(GfVec3i)),           62 },
    { std::type_index(typeid(GfVec2i)),           63 },
    { std::type_index(typeid(GfVec4h)),           64 },
    { std::type_index(typeid(GfVec3h)),           65 },
    { std::type_index(typeid(GfVec2h)),           66 },
    { std::type_index(typeid(GfVec4f)),           67 },
    { std::type_index(typeid(GfVec3f)),           68 },
    { std::type_index(typeid(GfVec2f)),           69 },
    { std::type_index(typeid(GfVec4d)),           70 },
    { std::type_index(typeid(GfVec3d)),           71 },
    { std::type_index(typeid(GfVec2d)),           72 },
    { std::type_index(typeid(GfMatrix4f)),        73 },
    { std::type_index(typeid(GfMatrix3f)),        74 },
    { std::type_index(typeid(GfMatrix2f)),        75 },
    { std::type_index(typeid(GfMatrix4d)),        76 },
    { std::type_index(typeid(GfMatrix3d)),        77 },
    { std::type_index(typeid(GfMatrix2d)),        78 },
    { std::type_index(typeid(GfRange3f)),         79 },
    { std::type_index(typeid(GfRange3d)),         80 },
    { std::type_index(typeid(GfRange2f)),         81 },
    { std::type_index(typeid(GfRange2d)),         82 },
    { std::type_index(typeid(GfRange1f)),         83 },
    { std::type_index(typeid(GfRange1d)),         84 },
    { std::type_index(typeid(GfInterval)),        85 },
    { std::type_index(typeid(GfRect2i)),          86 },
    { std::type_index(typeid(GfQuath)),           87 },
    { std::type_index(typeid(GfQuatf)),           88 },
    { std::type_index(typeid(GfQuatd)),           89 },
    { std::type_index(typeid(GfQuaternion)),      90 },
#if PXR_VERSION >= 2208
    { std::type_index(typeid(GfDualQuath)),       91 },
    { std::type_index(typeid(GfDualQuatf)),       92 },
    { std::type_index(typeid(GfDualQuatd)),       93 },
#endif
    { std::type_index(typeid(GfFrustum)),         94 },
    { std::type_index(typeid(GfMultiInterval)),   95 },
    // XXX: For some builds, `long` is not covered by any of the above
    // TODO: There may be more for other platforms? E.g., `long long`?
    { std::type_index(typeid(long)),              96 } };

} // namespace Vt_ValueVisitDetail

template <class Visitor>
auto VtVisitValue(VtValue const &value, Visitor &&visitor)
{
    int typeIndex = -1;
    const std::type_info& typeInfo = value.GetTypeid();
    const auto it = Vt_ValueVisitDetail::types.find(std::type_index(typeInfo));
    if (it != Vt_ValueVisitDetail::types.end()) {
        typeIndex = it->second;
    }

    // This generally gets the compiler to emit a jump table to dispatch
    // directly to the code for each known value type.
    switch (typeIndex) {
        case  0: return Vt_ValueVisitDetail::Visit<bool>( value, std::forward<Visitor>(visitor), 0); break;
        case  1: return Vt_ValueVisitDetail::Visit<char>( value, std::forward<Visitor>(visitor), 0); break;
        case  2: return Vt_ValueVisitDetail::Visit<unsigned char>( value, std::forward<Visitor>(visitor), 0); break;
        case  3: return Vt_ValueVisitDetail::Visit<short>( value, std::forward<Visitor>(visitor), 0); break;
        case  4: return Vt_ValueVisitDetail::Visit<unsigned short>( value, std::forward<Visitor>(visitor), 0); break;
        case  5: return Vt_ValueVisitDetail::Visit<int>( value, std::forward<Visitor>(visitor), 0); break;
        case  6: return Vt_ValueVisitDetail::Visit<unsigned int>( value, std::forward<Visitor>(visitor), 0); break;
        case  7: return Vt_ValueVisitDetail::Visit<int64_t>( value, std::forward<Visitor>(visitor), 0); break;
        case  8: return Vt_ValueVisitDetail::Visit<uint64_t>( value, std::forward<Visitor>(visitor), 0); break;
        case  9: return Vt_ValueVisitDetail::Visit<double>( value, std::forward<Visitor>(visitor), 0); break;
        case 10: return Vt_ValueVisitDetail::Visit<float>( value, std::forward<Visitor>(visitor), 0); break;
        case 11: return Vt_ValueVisitDetail::Visit<GfHalf>( value, std::forward<Visitor>(visitor), 0); break;
        case 12: return Vt_ValueVisitDetail::Visit<std::string>( value, std::forward<Visitor>(visitor), 0); break;
        case 13: return Vt_ValueVisitDetail::Visit<TfToken>( value, std::forward<Visitor>(visitor), 0); break;
        case 14: return Vt_ValueVisitDetail::Visit<VtVec4iArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 15: return Vt_ValueVisitDetail::Visit<VtVec3iArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 16: return Vt_ValueVisitDetail::Visit<VtVec2iArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 17: return Vt_ValueVisitDetail::Visit<VtVec4hArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 18: return Vt_ValueVisitDetail::Visit<VtVec3hArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 19: return Vt_ValueVisitDetail::Visit<VtVec2hArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 20: return Vt_ValueVisitDetail::Visit<VtVec4fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 21: return Vt_ValueVisitDetail::Visit<VtVec3fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 22: return Vt_ValueVisitDetail::Visit<VtVec2fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 23: return Vt_ValueVisitDetail::Visit<VtVec4dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 24: return Vt_ValueVisitDetail::Visit<VtVec3dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 25: return Vt_ValueVisitDetail::Visit<VtVec2dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 26: return Vt_ValueVisitDetail::Visit<VtMatrix4fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 27: return Vt_ValueVisitDetail::Visit<VtMatrix3fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 28: return Vt_ValueVisitDetail::Visit<VtMatrix2fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 29: return Vt_ValueVisitDetail::Visit<VtMatrix4dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 30: return Vt_ValueVisitDetail::Visit<VtMatrix3dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 31: return Vt_ValueVisitDetail::Visit<VtMatrix2dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 32: return Vt_ValueVisitDetail::Visit<VtRange3fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 33: return Vt_ValueVisitDetail::Visit<VtRange3dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 34: return Vt_ValueVisitDetail::Visit<VtRange2fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 35: return Vt_ValueVisitDetail::Visit<VtRange2dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 36: return Vt_ValueVisitDetail::Visit<VtRange1fArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 37: return Vt_ValueVisitDetail::Visit<VtRange1dArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 38: return Vt_ValueVisitDetail::Visit<VtIntervalArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 39: return Vt_ValueVisitDetail::Visit<VtRect2iArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 40: return Vt_ValueVisitDetail::Visit<VtQuathArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 41: return Vt_ValueVisitDetail::Visit<VtQuatfArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 42: return Vt_ValueVisitDetail::Visit<VtQuatdArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 43: return Vt_ValueVisitDetail::Visit<VtQuaternionArray>( value, std::forward<Visitor>(visitor), 0); break;
    #if PXR_VERSION >= 2208
        case 44: return Vt_ValueVisitDetail::Visit<VtDualQuathArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 45: return Vt_ValueVisitDetail::Visit<VtDualQuatfArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 46: return Vt_ValueVisitDetail::Visit<VtDualQuatdArray>( value, std::forward<Visitor>(visitor), 0); break;
    #endif
        case 47: return Vt_ValueVisitDetail::Visit<VtBoolArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 48: return Vt_ValueVisitDetail::Visit<VtCharArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 49: return Vt_ValueVisitDetail::Visit<VtUCharArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 50: return Vt_ValueVisitDetail::Visit<VtShortArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 51: return Vt_ValueVisitDetail::Visit<VtUShortArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 52: return Vt_ValueVisitDetail::Visit<VtIntArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 53: return Vt_ValueVisitDetail::Visit<VtUIntArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 54: return Vt_ValueVisitDetail::Visit<VtInt64Array>( value, std::forward<Visitor>(visitor), 0); break;
        case 55: return Vt_ValueVisitDetail::Visit<VtUInt64Array>( value, std::forward<Visitor>(visitor), 0); break;
        case 56: return Vt_ValueVisitDetail::Visit<VtDoubleArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 57: return Vt_ValueVisitDetail::Visit<VtFloatArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 58: return Vt_ValueVisitDetail::Visit<VtHalfArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 59: return Vt_ValueVisitDetail::Visit<VtStringArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 60: return Vt_ValueVisitDetail::Visit<VtTokenArray>( value, std::forward<Visitor>(visitor), 0); break;
        case 61: return Vt_ValueVisitDetail::Visit<GfVec4i>( value, std::forward<Visitor>(visitor), 0); break;
        case 62: return Vt_ValueVisitDetail::Visit<GfVec3i>( value, std::forward<Visitor>(visitor), 0); break;
        case 63: return Vt_ValueVisitDetail::Visit<GfVec2i>( value, std::forward<Visitor>(visitor), 0); break;
        case 64: return Vt_ValueVisitDetail::Visit<GfVec4h>( value, std::forward<Visitor>(visitor), 0); break;
        case 65: return Vt_ValueVisitDetail::Visit<GfVec3h>( value, std::forward<Visitor>(visitor), 0); break;
        case 66: return Vt_ValueVisitDetail::Visit<GfVec2h>( value, std::forward<Visitor>(visitor), 0); break;
        case 67: return Vt_ValueVisitDetail::Visit<GfVec4f>( value, std::forward<Visitor>(visitor), 0); break;
        case 68: return Vt_ValueVisitDetail::Visit<GfVec3f>( value, std::forward<Visitor>(visitor), 0); break;
        case 69: return Vt_ValueVisitDetail::Visit<GfVec2f>( value, std::forward<Visitor>(visitor), 0); break;
        case 70: return Vt_ValueVisitDetail::Visit<GfVec4d>( value, std::forward<Visitor>(visitor), 0); break;
        case 71: return Vt_ValueVisitDetail::Visit<GfVec3d>( value, std::forward<Visitor>(visitor), 0); break;
        case 72: return Vt_ValueVisitDetail::Visit<GfVec2d>( value, std::forward<Visitor>(visitor), 0); break;
        case 73: return Vt_ValueVisitDetail::Visit<GfMatrix4f>( value, std::forward<Visitor>(visitor), 0); break;
        case 74: return Vt_ValueVisitDetail::Visit<GfMatrix3f>( value, std::forward<Visitor>(visitor), 0); break;
        case 75: return Vt_ValueVisitDetail::Visit<GfMatrix2f>( value, std::forward<Visitor>(visitor), 0); break;
        case 76: return Vt_ValueVisitDetail::Visit<GfMatrix4d>( value, std::forward<Visitor>(visitor), 0); break;
        case 77: return Vt_ValueVisitDetail::Visit<GfMatrix3d>( value, std::forward<Visitor>(visitor), 0); break;
        case 78: return Vt_ValueVisitDetail::Visit<GfMatrix2d>( value, std::forward<Visitor>(visitor), 0); break;
        case 79: return Vt_ValueVisitDetail::Visit<GfRange3f>( value, std::forward<Visitor>(visitor), 0); break;
        case 80: return Vt_ValueVisitDetail::Visit<GfRange3d>( value, std::forward<Visitor>(visitor), 0); break;
        case 81: return Vt_ValueVisitDetail::Visit<GfRange2f>( value, std::forward<Visitor>(visitor), 0); break;
        case 82: return Vt_ValueVisitDetail::Visit<GfRange2d>( value, std::forward<Visitor>(visitor), 0); break;
        case 83: return Vt_ValueVisitDetail::Visit<GfRange1f>( value, std::forward<Visitor>(visitor), 0); break;
        case 84: return Vt_ValueVisitDetail::Visit<GfRange1d>( value, std::forward<Visitor>(visitor), 0); break;
        case 85: return Vt_ValueVisitDetail::Visit<GfInterval>( value, std::forward<Visitor>(visitor), 0); break;
        case 86: return Vt_ValueVisitDetail::Visit<GfRect2i>( value, std::forward<Visitor>(visitor), 0); break;
        case 87: return Vt_ValueVisitDetail::Visit<GfQuath>( value, std::forward<Visitor>(visitor), 0); break;
        case 88: return Vt_ValueVisitDetail::Visit<GfQuatf>( value, std::forward<Visitor>(visitor), 0); break;
        case 89: return Vt_ValueVisitDetail::Visit<GfQuatd>( value, std::forward<Visitor>(visitor), 0); break;
        case 90: return Vt_ValueVisitDetail::Visit<GfQuaternion>( value, std::forward<Visitor>(visitor), 0); break;
    #if PXR_VERSION >= 2208
        case 91: return Vt_ValueVisitDetail::Visit<GfDualQuath>( value, std::forward<Visitor>(visitor), 0); break;
        case 92: return Vt_ValueVisitDetail::Visit<GfDualQuatf>( value, std::forward<Visitor>(visitor), 0); break;
        case 93: return Vt_ValueVisitDetail::Visit<GfDualQuatd>( value, std::forward<Visitor>(visitor), 0); break;
    #endif
        case 94: return Vt_ValueVisitDetail::Visit<GfFrustum>( value, std::forward<Visitor>(visitor), 0); break;
        case 95: return Vt_ValueVisitDetail::Visit<GfMultiInterval>( value, std::forward<Visitor>(visitor), 0); break;
        case 96: return Vt_ValueVisitDetail::Visit<long>( value, std::forward<Visitor>(visitor), 0); break;
        default: return Vt_ValueVisitDetail::Visit<VtValue>(value, std::forward<Visitor>(visitor), 0); break;
    };
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2211

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_BASE_VT_VISIT_VALUE_H
