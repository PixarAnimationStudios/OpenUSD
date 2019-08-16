//
// Copyright 2019 Pixar
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
#include "valueUtils.h"

#include "error.h"
#include "USD_Utils.h"
#include "UT_TypeTraits.h"

#include "pxr/base/arch/hints.h"
#include "pxr/usd/sdf/types.h"

#include <GA/GA_AIFNumericArray.h>
#include <GA/GA_AIFSharedStringArray.h>
#include <GA/GA_AIFSharedStringTuple.h>
#include <GA/GA_AIFTuple.h>
#include <GA/GA_Attribute.h>
#include <GA/GA_ATIGroupBool.h>
#include <GA/GA_ATINumericArray.h>
#include <GA/GA_ATIString.h>
#include <GA/GA_ATIStringArray.h>
#include <GA/GA_Handle.h>
#include <GA/GA_Iterator.h>
#include <GA/GA_PageHandle.h>
#include <GA/GA_SplittableRange.h>
#include <GEO/GEO_Detail.h>
#include <SYS/SYS_TypeTraits.h>
#include <UT/UT_Interrupt.h>
#include <UT/UT_ParallelUtil.h>
#include <UT/UT_StringArray.h>
#include <UT/UT_VectorTypes.h>


PXR_NAMESPACE_OPEN_SCOPE


namespace {


/// Helper for periodically polling for interrupts.
class _InterruptPoll
{
public:
    _InterruptPoll(UT_Interrupt* boss=UTgetInterrupt()) : _boss(boss)
        { UT_ASSERT_P(boss); }

    SYS_FORCE_INLINE bool operator()()
        { return ARCH_UNLIKELY(!++_bcnt && _boss->opInterrupt()); }

private:
    UT_Interrupt* const _boss;
    unsigned char       _bcnt = 0;
};


/// Struct giving the elem type of a VtArray, or the input arg for non-arrays.
template <class T, class U=void>
struct _ElemType { using type = T; };

template <class T>
struct _ElemType<T, typename std::enable_if<VtIsArray<T>::value>::type>
{ using type = typename T::value_type; };


/// Struct giving the best matching scalar type supported by GA API,
/// for interacting with a USD scalar type.
template <class T>
struct _UsdScalarToGaScalar { using type = T; };

// Cast bools to integers for purposes of GA_AIFTuple/GA_AIFNumericArray
// queries. Group attributes are handled separately.
template <>
struct _UsdScalarToGaScalar<bool>           { using type = int32; };

template <>
struct _UsdScalarToGaScalar<unsigned char>  { using type = int32; };

// XXX: potentially lossy
template <>
struct _UsdScalarToGaScalar<unsigned int>   { using type = int32; };

// XXX: potentially lossy
template <>
struct _UsdScalarToGaScalar<unsigned long>  { using type = int64; };

template <>
struct _UsdScalarToGaScalar<GfHalf>         { using type = fpreal32; };

template <>
struct _UsdScalarToGaScalar<fpreal16>       { using type = fpreal32; };


template <class T>
constexpr bool _UsdValueIsNumeric()
{
    using ElemType = typename _ElemType<T>::type;
    return (std::is_arithmetic<ElemType>::value ||
            GfIsGfMatrix<ElemType>::value ||
            GfIsGfQuat<ElemType>::value ||
            GfIsGfVec<ElemType>::value);
}


template <class T>
constexpr bool _UsdValueIsString()
{
    using ElemType = typename _ElemType<T>::type;
    return (SYSisSame<ElemType,std::string>() ||
            SYSisSame<ElemType,TfToken>() ||
            SYSisSame<ElemType,SdfAssetPath>());
}


/// Traits of a numeric scalar/tuple types.
template <class T, class U=void>
struct _UsdNumericValueTraits
{
    using UsdTupleType = T;
    using UsdScalarType = typename GusdPodTupleTraits<T>::ValueType;
    using GaScalarType = typename _UsdScalarToGaScalar<UsdScalarType>::type;

    static const int tupleSize = GusdGetTupleSize<UsdTupleType>();

    static void     ResizeByTupleCount(const T& value, size_t count) {}

    static void     ResizeByScalarCount(const T& value, size_t count) {}

    static size_t   GetNumScalars(const T& value) { return tupleSize; }

    static size_t   GetNumTuples(const T& value)  { return 1; } 

    static UsdScalarType*       GetScalarDataPtr(T& value)
                                {
                                    return reinterpret_cast<
                                        UsdScalarType*>(&value);
                                }

    static const UsdScalarType* GetConstScalarDataPtr(const T& value)
                                {
                                    return reinterpret_cast<
                                        const UsdScalarType*>(&value);
                                }
};


/// Traits of a VtArray holding a numeric scalar/tuple.
template <class T>
struct _UsdNumericValueTraits<
    T, typename std::enable_if<VtIsArray<T>::value>::type>
{
    using UsdTupleType = typename T::value_type;
    using UsdScalarType = typename GusdPodTupleTraits<UsdTupleType>::ValueType;
    using GaScalarType = typename _UsdScalarToGaScalar<UsdScalarType>::type;

    static const int tupleSize = GusdGetTupleSize<UsdTupleType>();

    static void     ResizeByTupleCount(T& value, size_t size)
                    { value.resize(size); }

    static void     ResizeByScalarCount(T& value, size_t size)
                    { value.resize(size/tupleSize + (size%tupleSize ? 1 : 0)); }

    static size_t   GetNumScalars(const T& value)
                    { return value.size()*tupleSize; }

    static size_t   GetNumTuples(const T& value)  { return value.size();}

    static UsdScalarType*       GetScalarDataPtr(T& value)
                                {
                                    return reinterpret_cast<UsdScalarType*>(
                                        value.data());
                                }

    static const UsdScalarType* GetConstScalarDataPtr(const T& value)
                                {
                                    return reinterpret_cast<
                                        const UsdScalarType*>(value.cdata());
                                }
};


// --------------------------------------------------
// _NumericAttrToUsdValues
// --------------------------------------------------


template <class T, class U=void>
struct _NumericAttrToUsdValues
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
         { return false; }
};


/// Direct numeric value extraction.
/// Extracts to scalars, vectors, matrices and quaternions, as well as their
/// VtArray forms.
template <class T>
struct _NumericAttrToUsdValues<
    T, typename std::enable_if<
           _UsdValueIsNumeric<T>() &&
           SYSisSame<typename _UsdNumericValueTraits<T>::UsdScalarType,
                     typename _UsdNumericValueTraits<T>::GaScalarType>()>::type>
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;
        using GaScalarType = typename Traits::GaScalarType;

        const auto* aif = attr.getAIFTuple();
        if (!aif) {
            return false;
        }

        UT_AutoInterrupt task("Extract USD values from numeric attr");

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, offsets.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {
                _InterruptPoll interrupt;

                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }

                    T& value = values[i];

                    Traits::ResizeByScalarCount(value, attr.getTupleSize());

                    const int numScalarsToExtract =
                        SYSmin(attr.getTupleSize(),
                               static_cast<int>(Traits::GetNumScalars(value)));

                    aif->get(&attr, offsets[i],
                             Traits::GetScalarDataPtr(value),
                             numScalarsToExtract);
                }
            });
        return !task.wasInterrupted();
    }
};


/// Indirect numeric value extraction.
/// Extracts to scalars, vectors, matrices and quaternions, as well as their
/// VtArray forms. This form is used for values that require temporary
/// storage when extracting values, due to limited type support of the
/// GA_AIFTuple API. For example, GA_AIFTuple::get() has no method
/// using uint64 precision, so we must first get() the value with a type
/// that GA_AIFTuple supports, then cast it to our given USD type.
template <class T>
struct _NumericAttrToUsdValues<
    T, typename std::enable_if<
           _UsdValueIsNumeric<T>() &&
           !SYSisSame<
               typename _UsdNumericValueTraits<T>::UsdScalarType,
               typename _UsdNumericValueTraits<T>::GaScalarType>()>::type>
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;
        using GaScalarType = typename Traits::GaScalarType;

        const auto* aif = attr.getAIFTuple();
        if (!aif) {
            return false;
        }

        UT_AutoInterrupt task("Extract USD values from numeric attr");

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, offsets.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {
                UT_Array<GaScalarType> gaValues;
                gaValues.setSize(attr.getTupleSize());
                
                _InterruptPoll interrupt;

                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }

                    if (aif->get(&attr, offsets[i],
                                 gaValues.data(), attr.getTupleSize())) {

                        T& value = values[i];
                        Traits::ResizeByScalarCount(value, attr.getTupleSize());

                        const int numScalarsToExtract =
                            SYSmin(attr.getTupleSize(),
                                   static_cast<int>(
                                       Traits::GetNumScalars(value)));

                        UsdScalarType* dst =
                            Traits::GetScalarDataPtr(values[i]);
                        for (int si = 0; si < numScalarsToExtract; ++si) {
                            dst[si] = static_cast<UsdScalarType>(gaValues[si]);
                        }
                    }
                }
            });
        return !task.wasInterrupted();
    }
};


// --------------------------------------------------
// _GroupAttrToUsdValues
// --------------------------------------------------


template <class T, class U=void>
struct _GroupAttrToUsdValues
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
         { return false; }
};


template <class T>
struct _GroupAttrToUsdValues<
    T, typename std::enable_if<_UsdValueIsNumeric<T>()>::type>
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;

        const auto* groupAttr = GA_ATIGroupBool::cast(&attr);
        if (!groupAttr) {
            return false;
        }

        UT_AutoInterrupt task("Extract USD values from group attr");

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, offsets.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {
                _InterruptPoll interrupt;

                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }

                    T& value = values[i];

                    Traits::ResizeByScalarCount(value, 1);
                    
                    if (Traits::GetNumScalars(value) > 0) {
                        *Traits::GetScalarDataPtr(value) =
                            static_cast<UsdScalarType>(
                                groupAttr->contains(offsets[i]));
                    }
                }
            });
        return !task.wasInterrupted();
    }
};


// --------------------------------------------------
// _NumericArrayAttrToUsdValues
// --------------------------------------------------


template <class T, class U=void>
struct _NumericArrayAttrToUsdValues
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
         { return false; }
};


/// Direct numeric array extraction.
/// Extracts to scalars, vectors, matrices and quaternions, as well as their
/// VtArray forms.
template <class T>
struct _NumericArrayAttrToUsdValues<
    T, typename std::enable_if<
           _UsdValueIsNumeric<T>() &&
           SYSisSame<typename _UsdNumericValueTraits<T>::UsdScalarType,
                     typename _UsdNumericValueTraits<T>::GaScalarType>()>::type>
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;
        using GaScalarType = typename Traits::GaScalarType;

        const auto* aif = attr.getAIFNumericArray();
        if (!aif) {
            return false;
        }

        UT_AutoInterrupt task("Extract USD values from numeric array attr");

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, offsets.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {
                _InterruptPoll interrupt;

                UT_Array<GaScalarType> gaArray;

                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }

                    const exint arraySize = aif->arraySize(&attr, offsets[i]);
                    if (arraySize == 0) {
                        continue;
                    }

                    T& value = values[i];
                    Traits::ResizeByScalarCount(value, arraySize);

                    if (Traits::GetNumScalars(value) >= arraySize) {
                        // XXX: Using unsafeShareData to avoid an extra copy.
                        // This is safe for core ATIs, but could fail in custom
                        // types, depending on the implementation.
                        // If failures occur, consider white-listing types.
                        gaArray.unsafeShareData(
                            Traits::GetScalarDataPtr(value),
                            Traits::GetNumScalars(value));
                        
                        UT_ASSERT_P(gaArray.data() ==
                                    Traits::GetScalarDataPtr(value));
                        UT_ASSERT_P(gaArray.size() ==
                                    Traits::GetNumScalars(value));
                        
                        aif->get(&attr, offsets[i], gaArray);
                        
                        // Verify that the array is still referencing the
                        // same data.
                        UT_ASSERT_P(gaArray.data() ==
                                    Traits::GetScalarDataPtr(value));
                        UT_ASSERT_P(gaArray.size() ==
                                    Traits::GetNumScalars(value));
                        
                        gaArray.unsafeClearData();
                    } else {
                        // Our value can't be resized as expected for
                        // this array. Will need to extract a copy
                        // into our temporary array, then convert.
                        if (aif->get(&attr, offsets[i], gaArray)) {
                            const exint numScalarsToExtract =
                                SYSmin(gaArray.size(),
                                       static_cast<exint>(
                                           Traits::GetNumScalars(value)));

                            std::copy(gaArray.data(),
                                      gaArray.data() + numScalarsToExtract,
                                      Traits::GetScalarDataPtr(value));
                        }
                    }
                }
            });
        return !task.wasInterrupted();
    }
};


/// Indirect numeric array extraction.
/// Extracts to scalars, vectors, matrices and quaternions, as well as their
/// VtArray forms. This form is used for values that require temporary storage
/// when extracting values, due to limited type support of the
/// GA_AIFNumericArray API.
template <class T>
struct _NumericArrayAttrToUsdValues<
    T, typename std::enable_if<
           _UsdValueIsNumeric<T>() &&
           !SYSisSame<
               typename _UsdNumericValueTraits<T>::UsdScalarType,
               typename _UsdNumericValueTraits<T>::GaScalarType>()>::type>
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;
        using GaScalarType = typename Traits::GaScalarType;

        const auto* aif = attr.getAIFNumericArray();
        if (!aif) {
            return false;
        }

        UT_AutoInterrupt task("Extract USD values from numeric array attr");

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, offsets.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {
                _InterruptPoll interrupt;

                UT_Array<GaScalarType> gaArray;

                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }

                    if (aif->get(&attr, offsets[i], gaArray)) {

                        T& value = values[i];
                        Traits::ResizeByScalarCount(value, gaArray.size());
                        
                        const exint numScalarsToExtract =
                            SYSmin(gaArray.size(),
                                   static_cast<exint>(
                                       Traits::GetNumScalars(value)));

                        UsdScalarType* dst = Traits::GetScalarDataPtr(value);
                        for (exint si = 0; si < numScalarsToExtract; ++si) {
                            dst[si] = static_cast<UsdScalarType>(gaArray[si]);
                        }
                    }
                }
            });
        return !task.wasInterrupted();
    }
};


// --------------------------------------------------
// _UsdValuesToNumericAttr
// --------------------------------------------------


template <class T, class U=void>
struct _UsdValuesToNumericAttr
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
         { return false; }
};


/// Numeric value writes.
/// Writes scalars, vectors, matrices and quaternions, as well as their
/// VtArray forms.
template <class T>
struct _UsdValuesToNumericAttr<
    T, typename std::enable_if<_UsdValueIsNumeric<T>()>::type>
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;
        using GaScalarType = typename Traits::GaScalarType;
        using PageHandle = typename GA_PageHandleScalar<GaScalarType>::RWType;

        if (PageHandle(&attr).isInvalid()) {
            return false;
        }

        UT_AutoInterrupt task("Write USD values to numeric attr");

        UTparallelForLightItems(
            GA_SplittableRange(range),
            [&](const GA_SplittableRange& r)
            {
                // Bind a page handle per tuple component.
                UT_Array<PageHandle> pageHandles;
                pageHandles.setSize(attr.getTupleSize());
                for (int i = 0; i < attr.getTupleSize(); ++i) {
                    pageHandles[i].bind(&attr, i);
                }

                _InterruptPoll interrupt;

                GA_Offset o,end;
                for (GA_Iterator it(r); it.blockAdvance(o,end); ) {
                    if (interrupt()) {
                        return;
                    }

                    for (auto& pageHandle : pageHandles) {
                        pageHandle.setPage(o);
                    }

                    for ( ; o < end; ++o) {
                        const GA_Index index = rangeIndices[o];
                        
                        const T& value = values[index];
                        const UsdScalarType* src =
                            Traits::GetConstScalarDataPtr(value);

                        const int numScalarsToWrite =
                            SYSmin(attr.getTupleSize(),
                                   static_cast<int>(
                                       Traits::GetNumScalars(value)));

                        for (int si = 0; si < numScalarsToWrite; ++si) {
                            pageHandles[si].set(
                                o, static_cast<GaScalarType>(src[si]));
                        }
                    }
                }
            });
        return !task.wasInterrupted();
    }
};


// --------------------------------------------------
// _UsdValuesToGroupAttr
// --------------------------------------------------


template <class T, class U=void>
struct _UsdValuesToGroupAttr
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
         { return false; }
};


/// Numeric value writes.
/// Writes scalars or the first value of an array of scalars to a group attr.
template <class T>
struct _UsdValuesToGroupAttr<
    T, typename std::enable_if<_UsdValueIsNumeric<T>()>::type>
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;

        auto* groupAttr = GA_ATIGroupBool::cast(&attr);
        if (!groupAttr) {
            return false;
        }

        UT_AutoInterrupt task("Write USD values to group attr");

        _InterruptPoll interrupt;

        GA_Offset o,end;
        for (GA_Iterator it(range); it.blockAdvance(o,end); ) {
            if (interrupt()) {
                return false;
            }
            
            for ( ; o < end; ++o) {
                const GA_Index index = rangeIndices[o];
                const T& value = values[index];

                if (Traits::GetNumScalars(value) > 0) {
                    const bool isMember =
                        static_cast<bool>(
                            *Traits::GetConstScalarDataPtr(value));

                    groupAttr->setElement(o, isMember);
                }
           }
        }
        return true;
    }
};


// --------------------------------------------------
// _UsdValuesToNumericArrayAttr
// --------------------------------------------------


template <class T, class U=void>
struct _UsdValuesToNumericArrayAttr
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
         { return false; }
};


/// Direct numeric array writes.
/// Writes scalars, vectors, matrices and quaternions, as well as their
/// VtArray forms.
template <class T>
struct _UsdValuesToNumericArrayAttr<
    T, typename std::enable_if<
           _UsdValueIsNumeric<T>() &&
           SYSisSame<
               typename _UsdNumericValueTraits<T>::UsdScalarType,
               typename _UsdNumericValueTraits<T>::GaScalarType>()>::type>
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;
        using GaScalarType = typename Traits::GaScalarType;

        const auto* aif = attr.getAIFNumericArray();
        if (!aif) {
            return false;
        }

        UT_AutoInterrupt task("Write USD values to numeric array attr");

        // TODO: Can this be done in parallel?

        _InterruptPoll interrupt;

        UT_Array<GaScalarType> gaArray;

        GA_Offset o, end;
        for (GA_Iterator it(range); it.blockAdvance(o,end); ) {
            if (interrupt()) {
                return false;
            }

            for ( ; o < end; ++o) {
                const T& value = values[rangeIndices[o]];

                // XXX: Using unsafeShareData to avoid an extract copy.
                // This is safe for core ATIs, but could fail in custom
                // types, depending on the implementation.
                // If failures occur, consider white-listing types.
                gaArray.unsafeShareData(
                    SYSconst_cast(Traits::GetConstScalarDataPtr(value)),
                    Traits::GetNumScalars(value));

                aif->set(&attr, o, gaArray);

                gaArray.unsafeClearData();
            }
        }
        return true;
    }
};


/// Indirect numeric array writes.
/// Writes scalars, vectors, matrices and quaternions, as well as their
/// VtArray forms. This form is used for types that require temporary
/// storage when extracting values, due to limited type support of the
/// GA_AIFNumericArray API.
template <class T>
struct _UsdValuesToNumericArrayAttr<
    T, typename std::enable_if<
           _UsdValueIsNumeric<T>() &&
           !SYSisSame<
               typename _UsdNumericValueTraits<T>::UsdScalarType,
               typename _UsdNumericValueTraits<T>::GaScalarType>()>::type>
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
    {
        using Traits = _UsdNumericValueTraits<T>;
        using UsdScalarType = typename Traits::UsdScalarType;
        using GaScalarType = typename Traits::GaScalarType;

        const auto* aif = attr.getAIFNumericArray();
        if (!aif) {
            return false;
        }

        UT_AutoInterrupt task("Write USD values to numeric array attr");

        // TODO: Can this be done in parallel?

        _InterruptPoll interrupt;

        UT_Array<GaScalarType> gaArray;

        GA_Offset o, end;
        for (GA_Iterator it(range); it.blockAdvance(o,end); ) {
            if (interrupt()) {
                return false;
            }

            for ( ; o < end; ++o) {
                const T& value = values[rangeIndices[o]];

                const size_t numScalars = Traits::GetNumScalars(value);
                gaArray.setSize(numScalars);

                const UsdScalarType* src = Traits::GetConstScalarDataPtr(value);
                for (size_t si = 0; si < numScalars; ++si) {
                    gaArray[si] = static_cast<GaScalarType>(src[si]);
                }

                aif->set(&attr, o, gaArray);
            }
        }
        return true;
    }
};
            
// --------------------------------------------------
// String Types
// --------------------------------------------------


/// Traits of a string type.
template <class T, class U=void>
struct _UsdStringValueTraits
{
    using UsdStringType = T;
    
    static void                 Resize(const T& value, size_t size) {}

    static size_t               GetSize(const T& value) { return 1; }

    static UsdStringType*       GetStringDataPtr(T& value)
                                { return &value; }

    static const UsdStringType* GetConstStringDataPtr(const T& value)
                                { return &value; }
};


/// Traits of a VtArray holding a string type.
template <class T>
struct _UsdStringValueTraits<
    T, typename std::enable_if<VtIsArray<T>::value>::type>
{
    using UsdStringType = typename T::value_type;

    static void                 Resize(T& value, size_t size)
                                { value.resize(size); }

    static size_t               GetSize(const T& value)
                                { return value.size(); }

    static UsdStringType*       GetStringDataPtr(T& value)
                                { return value.data(); }

    static const UsdStringType* GetConstStringDataPtr(const T& value)
                                { return value.cdata(); }
};


template <class> struct _UsdStringOps {};

template <>
struct _UsdStringOps<std::string>
{
    
    static std::string     FromStringRef(const UT_StringRef& str)
                           { return str.toStdString(); }

    static UT_StringHolder ToStringHolder(const std::string& str)
                           { return UT_StringHolder(str); }
};

template <>
struct _UsdStringOps<TfToken>
{

    static TfToken         FromStringRef(const UT_StringRef& str)
                           { return TfToken(str.toStdString()); }
    
    static UT_StringHolder ToStringHolder(const TfToken& token)
                           { return GusdUSD_Utils::TokenToStringHolder(token); }
};

template <>
struct _UsdStringOps<SdfAssetPath>
{
    static SdfAssetPath    FromStringRef(const UT_StringRef& str)
                           { return SdfAssetPath(str.toStdString()); }

    static UT_StringHolder ToStringHolder(const SdfAssetPath& path)
                            { return UT_StringHolder(path.GetAssetPath()); }
};


// --------------------------------------------------
// _StringAttrToUsdValues
// --------------------------------------------------


template <class T, class U=void>
struct _StringAttrToUsdValues
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
         { return false; }
};


template <class T>
struct _StringAttrToUsdValues<
    T, typename std::enable_if<_UsdValueIsString<T>()>::type>
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
    {
        using Traits = _UsdStringValueTraits<T>;
        using UsdStringType = typename Traits::UsdStringType;
        using Ops = _UsdStringOps<UsdStringType>;

        const auto* aif = attr.getAIFSharedStringTuple();
        if (!aif) {
            return false;
        }

        UT_AutoInterrupt task("Extract USD values from string attr");

        // Pre-extract all unique string values.
        UT_Array<UsdStringType> uniqueValues;
        uniqueValues.setSize(aif->getTableEntries(&attr));

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, uniqueValues.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {   
                _InterruptPoll interrupt;
                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }
                    uniqueValues[i] =
                        Ops::FromStringRef(aif->getTableString(&attr, i));
                }
            });
        if (task.wasInterrupted()) {
            return false;
        }

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, offsets.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {
                _InterruptPoll interrupt;
                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }

                    T& value = values[i];
                    Traits::Resize(value, attr.getTupleSize());

                    const int numStringsToExtract =
                        SYSmin(attr.getTupleSize(),
                               static_cast<int>(Traits::GetSize(value)));

                    UsdStringType* dst = Traits::GetStringDataPtr(value);
                    for (int si = 0; si < numStringsToExtract; ++si) {
                        const GA_StringIndexType handle =
                            aif->getHandle(&attr, offsets[i], si);
                        if (handle >= 0) {
                            dst[si] = uniqueValues[handle];
                        }
                    }
                }
            });
        return !task.wasInterrupted();
    }
};


// --------------------------------------------------
// _StringArrayToUsdValues
// --------------------------------------------------


template <class T, class U=void>
struct _StringArrayAttrToUsdValues
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
         { return false; }
};


template <class T>
struct _StringArrayAttrToUsdValues<
    T, typename std::enable_if<_UsdValueIsString<T>()>::type>
{
    bool operator()(const GA_Attribute& attr,
                    const TfSpan<const GA_Offset>& offsets,
                    const TfSpan<T>& values) const
    {
        using Traits = _UsdStringValueTraits<T>;
        using UsdStringType = typename Traits::UsdStringType;
        using Ops = _UsdStringOps<UsdStringType>;

        const auto* aif = attr.getAIFSharedStringArray();
        if (!aif) {
            return false;
        }

        UT_AutoInterrupt task("Extract USD values from string array attr");

        // Pre-extract all unique string values.
        UT_Array<UsdStringType> uniqueValues;
        uniqueValues.setSize(aif->getTableEntries(&attr));

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, uniqueValues.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {   
                _InterruptPoll interrupt;
                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }
                    uniqueValues[i] =
                        Ops::FromStringRef(aif->getTableString(&attr, i));
                }
            });
        if (task.wasInterrupted()) {
            return false;
        }

        UTparallelForLightItems(
            UT_BlockedRange<size_t>(0, offsets.size()),
            [&](const UT_BlockedRange<size_t>& r)
            {
                UT_Array<GA_StringIndexType> handles;

                _InterruptPoll interrupt;
                for (size_t i = r.begin(); i < r.end(); ++i) {
                    if (interrupt()) {
                        return;
                    }

                    aif->getStringIndex(&attr, offsets[i], handles);
                    if (handles.size() > 0) {
                        T& value = values[i];
                        Traits::Resize(value, handles.size());

                        const exint numStringsToExtract =
                            SYSmin(handles.size(),  
                                   static_cast<exint>(Traits::GetSize(value)));
                        
                        UsdStringType* dst = Traits::GetStringDataPtr(value);
                        for (exint si = 0; si < numStringsToExtract; ++si) {
                            const auto handle = handles[si];
                            if (handle >= 0) {
                                dst[si] = uniqueValues[handle];
                            }
                        }
                    }
                }
            });
        return !task.wasInterrupted();
    }
};


// --------------------------------------------------
// _UsdValuesToStringAttr
// --------------------------------------------------


template <class T, class U=void>
struct _UsdValuesToStringAttr
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
         { return false; }
};


template <class T>
struct _UsdValuesToStringAttr<
    T, typename std::enable_if<_UsdValueIsString<T>()>::type>
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
    {   
        using Traits = _UsdStringValueTraits<T>;
        using UsdStringType = typename Traits::UsdStringType;
        using Ops = _UsdStringOps<UsdStringType>;

        GA_RWHandleS hnd(&attr);
        if (hnd.isInvalid()) {
            return false;
        }

        UT_AutoInterrupt task("Write USD values from string attr");

        // TODO: Can we do thsi in parallel?

        _InterruptPoll interrupt;
        GA_Offset o, end;
        for (GA_Iterator it(range); it.blockAdvance(o,end); ) {
            if (interrupt()) {
                return false;
            }

            for ( ; o < end; ++o) {
                const T& value = values[rangeIndices[o]];

                const int numStringsToWrite =
                    SYSmin(attr.getTupleSize(),
                           static_cast<int>(Traits::GetSize(value)));

                const UsdStringType* src = Traits::GetConstStringDataPtr(value);
                for (int si = 0; si < numStringsToWrite; ++si) {
                    hnd.set(o, si, Ops::ToStringHolder(src[si]));
                }
            }
        }
        return true;
    }
};


// --------------------------------------------------
// _UsdValuesToStringArrayAttr
// --------------------------------------------------


template <class T, class U=void>
struct _UsdValuesToStringArrayAttr
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
         { return false; }
};


template <class T>
struct _UsdValuesToStringArrayAttr<
    T, typename std::enable_if<_UsdValueIsString<T>()>::type>
{
    bool operator()(GA_Attribute& attr,
                    const GA_Range& range,
                    const TfSpan<const GA_Index>& rangeIndices,
                    const TfSpan<const T>& values) const
    {   
        using Traits = _UsdStringValueTraits<T>;
        using UsdStringType = typename Traits::UsdStringType;
        using Ops = _UsdStringOps<UsdStringType>;

        GA_RWHandleSA hnd(&attr);
        if (hnd.isInvalid()) {
            return false;
        }

        UT_AutoInterrupt task("Write USD values from string array attr");

        // TODO: Can we do this in parallel?

        UT_StringArray gaArray;
        _InterruptPoll interrupt;

        GA_Offset o, end;
        for (GA_Iterator it(range); it.blockAdvance(o,end); ) {
            if (interrupt()) {
                return false;
            }

            for ( ; o < end; ++o) {
                const T& value = values[rangeIndices[o]];

                gaArray.setSize(Traits::GetSize(value));

                const UsdStringType* src = Traits::GetConstStringDataPtr(value);
                for (exint si = 0; si < gaArray.size(); ++si) {
                    gaArray[si] = Ops::ToStringHolder(src[si]);
                }

                hnd.set(o, gaArray);
            }
        }
        return true;
    }
};


} // namespace


template <class T>
bool
GusdReadUsdValuesFromAttr(const GA_Attribute& attr,
                          const TfSpan<const GA_Offset>& offsets,
                          const TfSpan<T>& values)
{
    if (offsets.size() != values.size()) {
        GUSD_WARN().Msg("offsets size [%zu] != values size [%zu]",
                        offsets.size(), values.size());
        return false;
    }

    if (GA_ATIString::isType(&attr)) {
        return _StringAttrToUsdValues<T>()(attr, offsets, values);
    } else if (GA_ATIStringArray::isType(&attr)) {
        return _StringArrayAttrToUsdValues<T>()(attr, offsets, values);
    } else if (GA_ATINumericArray::isType(&attr)) {
        return _NumericArrayAttrToUsdValues<T>()(attr, offsets, values);
    } else if (GA_ATIGroupBool::isType(&attr)) {
        return _GroupAttrToUsdValues<T>()(attr, offsets, values);
    } else {
        // Try and process all other types as numerics.
        return _NumericAttrToUsdValues<T>()(attr, offsets, values);
    }
}

// Instantiate templated method for all Sdf value types.

#define GUSD_DEFINE_READ_ATTR(r, unused, elem)              \
    template bool GusdReadUsdValuesFromAttr(                \
        const GA_Attribute&,                                \
        const TfSpan<const GA_Offset>&,                     \
        const TfSpan<SDF_VALUE_CPP_TYPE(elem)>&);           \
                                                            \
    template bool GusdReadUsdValuesFromAttr(                \
        const GA_Attribute&,                                \
        const TfSpan<const GA_Offset>&,                     \
        const TfSpan<VtArray<SDF_VALUE_CPP_TYPE(elem)>>&);

BOOST_PP_SEQ_FOR_EACH(GUSD_DEFINE_READ_ATTR, ~, SDF_VALUE_TYPES);

#undef GUSD_DEFINE_READ_ATTR


template <class T>
bool
GusdWriteUsdValuesToAttr(GA_Attribute& attr,
                         const GA_Range& range,
                         const TfSpan<const GA_Index>& rangeIndices,
                         const TfSpan<const T>& values)
{
    if (rangeIndices.size() != values.size()) {
        GUSD_WARN().Msg("rangeIndices size [%zu] != values size [%zu]",
                        rangeIndices.size(), values.size());
        return false;
    }
    UT_ASSERT_P(range.getEntries() == rangeIndices.size());

    if (GA_ATIString::isType(&attr)) {
        return _UsdValuesToStringAttr<T>()(
            attr, range, rangeIndices, values);
    } else if (GA_ATIStringArray::isType(&attr)) {
        return _UsdValuesToStringArrayAttr<T>()(
            attr, range, rangeIndices, values);
    } else if (GA_ATINumericArray::isType(&attr)) {
        return _UsdValuesToNumericArrayAttr<T>()(
            attr, range, rangeIndices, values);
    } else if (GA_ATIGroupBool::isType(&attr)) {
        return _UsdValuesToGroupAttr<T>()(
            attr, range, rangeIndices, values);
    } else {
        // Try and process all other types as numerics.
        return _UsdValuesToNumericAttr<T>()(
            attr, range, rangeIndices, values);
    }
}

// Instantiate templated method for all Sdf value types.

#define GUSD_DEFINE_WRITE_ATTR(r, unused, elem)                     \
    template bool GusdWriteUsdValuesToAttr(                         \
        GA_Attribute&, const GA_Range&,                             \
        const TfSpan<const GA_Index>&,                              \
        const TfSpan<const SDF_VALUE_CPP_TYPE(elem)>&);             \
                                                                    \
    template bool GusdWriteUsdValuesToAttr(                         \
        GA_Attribute&, const GA_Range&,                             \
        const TfSpan<const GA_Index>&,                              \
        const TfSpan<const VtArray<SDF_VALUE_CPP_TYPE(elem)>>&);

BOOST_PP_SEQ_FOR_EACH(GUSD_DEFINE_WRITE_ATTR, ~, SDF_VALUE_TYPES);

#undef GUSD_DEFINE_WRITE_ATTR


namespace {


/// Returns the array-valued form of \p nonArrayType if \p isArray is true.
/// Otherwise returns the \p nonArrayType.
inline SdfValueTypeName
_GetTypeName(const SdfValueTypeName& nonArrayType, bool isArray)
{
    return isArray ? nonArrayType.GetArrayType() : nonArrayType;
}


/// Return the underlying GA_STORAGE type for \p attr.
GA_Storage
_GetAttrStorage(const GA_Attribute& attr)
{
    if (const auto* aif = attr.getAIFTuple()) {
        return aif->getStorage(&attr);
    }
    if (const auto* aif = attr.getAIFNumericArray()) {
        return aif->getStorage(&attr);
    }
    if (const auto* aif = attr.getAIFSharedStringArray()) {
        return aif->getStorage(&attr);
    }
    // String attributes do not have a GA_AIFTuple implementation.
    // For strings, refer to the storage class instead.
    if (attr.getStorageClass() == GA_STORECLASS_STRING) {
        return GA_STORE_STRING;
    }
    return GA_STORE_INVALID;
}

             
} // namespace


SdfValueTypeName
GusdGetSdfTypeNameForAttr(const GA_Attribute& attr)
{

    const bool isArray =
        GA_ATINumericArray::isType(&attr) || GA_ATIStringArray::isType(&attr);
    const int tupleSize = attr.getTupleSize();
    const GA_Storage storage = _GetAttrStorage(attr);
    const GA_TypeInfo typeInfo = attr.getTypeInfo();

    switch (storage)
    {
    case GA_STORE_BOOL:
        return _GetTypeName(SdfValueTypeNames->Bool,
                            isArray || tupleSize != 1);
    case GA_STORE_UINT8:
        return _GetTypeName(SdfValueTypeNames->UChar,
                            isArray || tupleSize != 1);
    case GA_STORE_INT8:
    case GA_STORE_INT16:
    case GA_STORE_INT32:
        if (tupleSize == 1) {
            return _GetTypeName(SdfValueTypeNames->Int, isArray);
        } else if (tupleSize == 2) {
            return _GetTypeName(SdfValueTypeNames->Int2, isArray);
        } else if (tupleSize == 3) {
            return _GetTypeName(SdfValueTypeNames->Int3, isArray);
        }
        return SdfValueTypeNames->IntArray;

    case GA_STORE_REAL16:
        if (tupleSize == 1) {
            return _GetTypeName(SdfValueTypeNames->Half, isArray);
        } else if (tupleSize == 2) {
            if (typeInfo == GA_TYPE_TEXTURE_COORD) {
                return _GetTypeName(SdfValueTypeNames->TexCoord2h, isArray);
            } else {
                return _GetTypeName(SdfValueTypeNames->Half2, isArray);
            }
        } else if (tupleSize == 3) {
            switch (typeInfo)
            {
            case GA_TYPE_POINT:
                return _GetTypeName(SdfValueTypeNames->Point3h, isArray);
            case GA_TYPE_VECTOR:
                return _GetTypeName(SdfValueTypeNames->Vector3h, isArray);
            case GA_TYPE_NORMAL:
                return _GetTypeName(SdfValueTypeNames->Normal3h, isArray);
            case GA_TYPE_COLOR:
                return _GetTypeName(SdfValueTypeNames->Color3h, isArray);
            case GA_TYPE_TRANSFORM:
                return _GetTypeName(SdfValueTypeNames->Matrix2d, isArray);
            case GA_TYPE_TEXTURE_COORD:
                return _GetTypeName(SdfValueTypeNames->TexCoord3h, isArray);
            default:
                return _GetTypeName(SdfValueTypeNames->Half3, isArray);
            }
        } else if (tupleSize == 4) {
            if (typeInfo == GA_TYPE_COLOR) {
                return _GetTypeName(SdfValueTypeNames->Color4h, isArray);
            } else {
                return _GetTypeName(SdfValueTypeNames->Half4, isArray);
            }
        } else if (tupleSize == 9 && typeInfo == GA_TYPE_TRANSFORM) {
            return _GetTypeName(SdfValueTypeNames->Matrix3d, isArray);
        } else if (tupleSize == 16 && typeInfo == GA_TYPE_TRANSFORM) {
            return _GetTypeName(SdfValueTypeNames->Matrix4d, isArray);
        }
        return SdfValueTypeNames->HalfArray;

    case GA_STORE_REAL32:
        if (tupleSize == 1) {
            return _GetTypeName(SdfValueTypeNames->Float, isArray);
        } else if (tupleSize == 2) {
            if (typeInfo == GA_TYPE_TEXTURE_COORD) {
                return _GetTypeName(SdfValueTypeNames->TexCoord2f, isArray);
            } else {
                return _GetTypeName(SdfValueTypeNames->Float2, isArray);
            }
        } else if (tupleSize == 3) {
            switch (typeInfo)
            {
            case GA_TYPE_POINT:
                return _GetTypeName(SdfValueTypeNames->Point3f, isArray);
            case GA_TYPE_VECTOR:
                return _GetTypeName(SdfValueTypeNames->Vector3f, isArray);
            case GA_TYPE_NORMAL:
                return _GetTypeName(SdfValueTypeNames->Normal3f, isArray);
            case GA_TYPE_COLOR:
                return _GetTypeName(SdfValueTypeNames->Color3f, isArray);
            case GA_TYPE_TRANSFORM:
                return _GetTypeName(SdfValueTypeNames->Matrix2d, isArray);
            case GA_TYPE_TEXTURE_COORD:
                return _GetTypeName(SdfValueTypeNames->TexCoord3f, isArray);
            default:
                return _GetTypeName(SdfValueTypeNames->Float3, isArray);
            }
        } else if (tupleSize == 4) {
            if (typeInfo == GA_TYPE_COLOR) {
                return _GetTypeName(SdfValueTypeNames->Color4f, isArray);
            } else {
                return _GetTypeName(SdfValueTypeNames->Float4, isArray);
            }
        } else if (tupleSize == 9 && typeInfo == GA_TYPE_TRANSFORM) {
            return _GetTypeName(SdfValueTypeNames->Matrix3d, isArray);
        } else if (tupleSize == 16 && typeInfo == GA_TYPE_TRANSFORM) {
            return _GetTypeName(SdfValueTypeNames->Matrix4d, isArray);
        }
        return SdfValueTypeNames->FloatArray;

    case GA_STORE_REAL64:
        if (tupleSize == 1) {
            return _GetTypeName(SdfValueTypeNames->Double, isArray);
        } else if (tupleSize == 2) {
            if (typeInfo == GA_TYPE_TEXTURE_COORD) {
                return _GetTypeName(SdfValueTypeNames->TexCoord2d, isArray);
            } else {
                return _GetTypeName(SdfValueTypeNames->Double2, isArray);
            }
        } else if (tupleSize == 3) {
            switch (typeInfo)
            {
            case GA_TYPE_POINT:
                return _GetTypeName(SdfValueTypeNames->Point3d, isArray);
            case GA_TYPE_VECTOR:
                return _GetTypeName(SdfValueTypeNames->Vector3d, isArray);
            case GA_TYPE_NORMAL:
                return _GetTypeName(SdfValueTypeNames->Normal3d, isArray);
            case GA_TYPE_COLOR:
                return _GetTypeName(SdfValueTypeNames->Color3d, isArray);
            case GA_TYPE_TRANSFORM:
                return _GetTypeName(SdfValueTypeNames->Matrix2d, isArray);
            case GA_TYPE_TEXTURE_COORD:
                return _GetTypeName(SdfValueTypeNames->TexCoord3d, isArray);
            default:
                return _GetTypeName(SdfValueTypeNames->Double3, isArray);
            }
        } else if (tupleSize == 4) {
            if (typeInfo == GA_TYPE_COLOR) {
                return _GetTypeName(SdfValueTypeNames->Color4d, isArray);
            } else {
                return _GetTypeName(SdfValueTypeNames->Double4, isArray);
            }
        } else if (tupleSize == 9 && typeInfo == GA_TYPE_TRANSFORM) {
            return _GetTypeName(SdfValueTypeNames->Matrix3d, isArray);
        } else if (tupleSize == 16 && typeInfo == GA_TYPE_TRANSFORM) {
            return _GetTypeName(SdfValueTypeNames->Matrix4d, isArray);
        }
        return SdfValueTypeNames->DoubleArray;

    case GA_STORE_STRING:
        // TODO: String, Token and Asset are all valid answers here.
        // Should the attribute store metadata telling us which type
        // to use? Should it be based on the name?
        return _GetTypeName(SdfValueTypeNames->String,
                            isArray || tupleSize != 1);
    default:
        return SdfValueTypeName();
    };
}


GA_TypeInfo
GusdGetTypeInfoForSdfRole(const TfToken& role, int tupleSize)
{
    // TODO: Determine if Houdini assumes a specific tupleSize
    // for some of these type infos. Eg., is a color always
    // assumed to have tupleSize == 3? Is it legitimate to
    // attach GA_TYPE_TRANSFORM to a float of tupleSize == 9?

    if (role == SdfValueRoleNames->Point) {
        if (tupleSize < 0 || tupleSize == 3) {
            return GA_TYPE_POINT;
        } else if (tupleSize == 4) {
            return GA_TYPE_HPOINT;
        }
    } else if (role == SdfValueRoleNames->Normal) {
        if (tupleSize < 0 || tupleSize == 3) {
            return GA_TYPE_NORMAL;
        }
    } else if (role == SdfValueRoleNames->Vector) {
        if (tupleSize < 0 || tupleSize == 3) {
            return GA_TYPE_VECTOR;
        }
    } else if (role == SdfValueRoleNames->Color) {
        if (tupleSize < 0 || tupleSize == 3 || tupleSize == 4) {
            return GA_TYPE_COLOR;
        }
    } else if (role == SdfValueRoleNames->TextureCoordinate) {
        if (tupleSize < 0 || tupleSize == 2 || tupleSize == 3) {
            return GA_TYPE_TEXTURE_COORD;
        }
    } else if (role == SdfValueRoleNames->Frame ||
               role == SdfValueRoleNames->Transform) {
        if (tupleSize < 0 || tupleSize == 16) {
            return GA_TYPE_TRANSFORM;
        }
    }

    return GA_TYPE_VOID;
}


namespace {


template <class T, class T2=void>
struct _AttrScalarType
{ using type = T; };

template <class T>
struct _AttrScalarType<T, typename std::enable_if<VtIsArray<T>::value>::type>
{ using type = typename T::value_type; };


} // namespace


template <class T>
GA_Attribute*
GusdCreateAttrForUsdValueType(GEO_Detail& gd,
                              const GA_AttributeScope scope,
                              const GA_AttributeOwner owner,
                              const UT_StringHolder& name,
                              const UT_Options* creationArgs)
{
    const GA_Storage storage = GusdGetUsdValueTypeAttrStorage<T>();
    if (storage == GA_STORE_INVALID) {
        return nullptr;
    }

    const int tupleSize = GusdGetUsdValueTypeTupleSize<T>();

    GA_Attribute* attr = nullptr;

    if (!VtIsArray<T>()) {
        attr = gd.addTuple(storage, owner, scope, name, tupleSize);
    } else {
        if (GAisFloatStorage(storage)) {
            attr = gd.addFloatArray(owner, scope, name, tupleSize, creationArgs,
                                    /*attr options*/ nullptr, storage);
        } else if (GAisIntStorage(storage)) {
            attr = gd.addIntArray(owner, scope, name, tupleSize, creationArgs,
                                  /*attr options*/ nullptr, storage);
        } else if (storage == GA_STORE_STRING) {
            attr = gd.addStringArray(owner, name, tupleSize, creationArgs);
        }
    }
    if (attr && GfIsGfQuat<typename _AttrScalarType<T>::type>::value) {
        // XXX: GA_TYPE_QUATERNION is the only type info that can be inferred
        // from the value type alone. For all other GA_TypeInfo values, the
        // caller must query the 'role' from the SdfValueTypeName of the
        // corresponding USD attribute.
        //
        // If the SdfValueTypeName was passed in as an argument to this method,
        // then we could configure the GA_AIFType for non-quaternion types at
        // this point as well. The reason we don't do that is because an
        // attribute's SdfValueTypeName is *not* cached, and must be composed
        // and potentially read from disk, so querying the type on every value
        // read introduces extra overhead.
        // Instead, the caller of this method should apply type info on the
        // resulting attribute, if necessary, using
        // GusdUsdValueTypeMayHaveRole() and GusdGetTypeInfoForUsdRole().
        attr->setTypeInfo(GA_TYPE_QUATERNION);
    }
    return attr;
}

// Instantiate templated method for all Sdf value types.

#define GUSD_CREATE_ATTR(r, unused, elem)                           \
    template GA_Attribute*                                          \
        GusdCreateAttrForUsdValueType<SDF_VALUE_CPP_TYPE(elem)>(    \
            GEO_Detail&, const GA_AttributeScope,                   \
            const GA_AttributeOwner, const UT_StringHolder&,        \
            const UT_Options*);                                     \
                                                                    \
    template GA_Attribute*                                          \
        GusdCreateAttrForUsdValueType<VtArray<SDF_VALUE_CPP_TYPE(elem)>>( \
            GEO_Detail&, const GA_AttributeScope,                   \
            const GA_AttributeOwner, const UT_StringHolder&,        \
            const UT_Options*);

BOOST_PP_SEQ_FOR_EACH(GUSD_CREATE_ATTR, ~, SDF_VALUE_TYPES);

#undef GUSD_CREATE_ATTR


PXR_NAMESPACE_CLOSE_SCOPE
