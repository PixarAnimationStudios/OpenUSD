//
// Copyright 2018 Pixar
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

#include <memory>
#include <type_traits>

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnDataBuilder.h>

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/vt/array.h"

#include "vtKatana/internalTraits.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace VtKatana_Internal {

/// Katana attribute zero copy context for a VtArray whose element type
/// is directly castable from the VtType to a Katana Value Type.
template <typename ElementType,
          typename = typename std::enable_if<
              VtKatana_IsNumericCastable<ElementType>::value>::type>
class VtKatana_Context {
    typedef
        typename VtKatana_GetNumericScalarType<ElementType>::type ScalarType;
    const VtArray<ElementType> _array;

public:
    explicit VtKatana_Context(const VtArray<ElementType>& array)
        : _array(array) {}

    const ScalarType* GetData() { return VtKatana_GetScalarPtr(_array); }

    static void Free(void* self) {
        auto context = static_cast<VtKatana_Context*>(self);
        delete context;
    }
};

/// Katana attribute zero copy context for a VtArray whose element type
/// is directly castable with multiple time samples.
template <typename ElementType,
          typename = typename std::enable_if<
              VtKatana_IsNumericCastable<ElementType>::value>::type>
class VtKatana_MultiContext {
    typedef
        typename VtKatana_GetNumericScalarType<ElementType>::type ScalarType;
    const std::vector<VtArray<ElementType>> _arrays;

public:
    explicit VtKatana_MultiContext(
        const typename std::vector<VtArray<ElementType>>& arrays)
        : _arrays(arrays) {}

    std::vector<const ScalarType*> GetData() {
        std::vector<const ScalarType*> ptrs(_arrays.size());
        std::transform(_arrays.cbegin(), _arrays.cend(), ptrs.begin(),
                       [](const VtArray<ElementType>& array) {
                           return VtKatana_GetScalarPtr(array);
                       });
        return ptrs;
    }
    static void Free(void* self) {
        auto context = static_cast<VtKatana_MultiContext*>(self);
        delete context;
    }
};

/// Convert an array of string holders to a vector of c-string pointers
/// suitable for Katana injection
template <typename StringType,
          typename = typename std::enable_if<
              VtKatana_IsOrHoldsString<StringType>::value>::type>
std::vector<const char*> VtKatana_ExtractStringVec(
    const VtArray<StringType>& array) {
    std::vector<const char*> result(array.size());
    std::transform(
        array.begin(), array.end(), result.begin(),
        [](const StringType& element) { return VtKatana_GetText(element); });
    return result;
}

/// Utilties for efficiently converting VtArrays to Katana attributes
template <typename ElementType>
class VtKatana_FromVtConversion {
    typedef typename VtKatana_GetKatanaAttrType<ElementType>::type AttrType;
    typedef
        typename VtKatana_GetKatanaAttrValueType<ElementType>::type ValueType;
    typedef typename AttrType::array_type SampleType;
    typedef typename FnKat::DataBuilder<AttrType> BuilderType;

public:
    // ZERO COPY SPECIALIZATIONS

    /// Utility constructing attributes without copies by retaining a reference
    /// to the originating VtArray
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   AttrType>::type
    ZeroCopy(const VtArray<T>& array) {
        typedef VtKatana_Context<T> ZeroCopyContext;
        TF_VERIFY(!array.empty());
        size_t size = array.size() * VtKatana_GetNumericTupleSize<T>::value;
        std::unique_ptr<ZeroCopyContext> ptr(new ZeroCopyContext(array));
        auto data = ptr->GetData();
        AttrType attr(data, size, VtKatana_GetNumericTupleSize<T>::value,
                      ptr.release(), ZeroCopyContext::Free);
        return attr;
    }

    /// Utility constructing attributes without copies by retaining references
    /// to the originating VtArrays
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   AttrType>::type
    ZeroCopy(const std::vector<float>& times,
             const std::vector<VtArray<T>>& values) {
        TF_VERIFY(times.size() == values.size() && !times.empty() &&
                  !values.front().empty());
        typedef VtKatana_MultiContext<T> ZeroCopyContext;
        size_t size =
            values.front().size() * VtKatana_GetNumericTupleSize<T>::value;
        std::unique_ptr<ZeroCopyContext> context(new ZeroCopyContext(values));
        auto data = context->GetData();
        AttrType attr(times.data(), times.size(), data.data(), size,
                      VtKatana_GetNumericTupleSize<T>::value, context.release(),
                      ZeroCopyContext::Free);
        return attr;
    }

    // COPY INTERMEDIATE TO STD::VECTOR IMPLEMENTATIONS

    /// Utility for copying numeric types to an intermediate std::vector
    /// suitable for use with Katana APIs. (ie. VtVec3hArray ->
    /// std::vector<float>)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumeric<T>::value,
                                   std::vector<ValueType>>::type
    CopyIntermediate(const VtArray<T>& array) {
        typedef typename VtKatana_GetNumericScalarType<ElementType>::type
            ScalarType;
        size_t size = array.size() * VtKatana_GetNumericTupleSize<T>::value;
        auto dataBegin = VtKatana_GetScalarPtr(array);
        auto dataEnd = dataBegin + size;
        std::vector<ValueType> intermediate(dataBegin, dataEnd);
        return intermediate;
    }

    /// Utility for copying tuple types to an intermediate std::vector
    /// suitable for use with Katana APIs. (ie. VtStringArray ->
    /// std::vector<std::string>)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_HoldsString<T>::value,
                                   std::vector<ValueType>>::type
    CopyIntermediate(const VtArray<T>& array) {
        std::vector<std::string> intermediate(array.size());
        std::transform(array.begin(), array.end(), intermediate.begin(),
                       [](const T& element) {
                           return std::string(VtKatana_GetText(element));
                       });
        return intermediate;
    }

    /// Utility for copying tuple types to an intermediate std::vector
    /// suitable for use with Katana APIs. (ie. VtStringArray ->
    /// std::vector<std::string>)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsString<T>::value,
                                   std::vector<ValueType>>::type
    CopyIntermediate(const VtArray<T>& array) {
        std::vector<std::string> intermediate(array.begin(), array.end());
        return intermediate;
    }

    // COPY FROM VTARRAY => KATANA ATTRIBUTE UTILITY

    /// Utility for copying tuple types that don't require
    /// an intermediate copy (ie. VtVec3fArray -> FloatAttribute)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   AttrType>::type
    Copy(const VtArray<T>& array) {
        typedef typename VtKatana_GetNumericScalarType<ElementType>::type
            ScalarType;
        const ScalarType* data = VtKatana_GetScalarPtr(array);
        size_t size = array.size() * VtKatana_GetNumericTupleSize<T>::value;
        AttrType attr(data, size, VtKatana_GetNumericTupleSize<T>::value);
        return attr;
    }

    /// Utility for copying types that require an intermediate
    /// translation for use with Katana APIs. (ie. VtVec3hArray ->
    /// FloatAttribute, VtUIntArray -> IntAttribute)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCopyRequired<T>::value,
                                   AttrType>::type
    Copy(const VtArray<T>& array) {
        auto intermediate = CopyIntermediate(array);
        size_t size = intermediate.size();
        auto data = intermediate.data();
        return AttrType(data, size, VtKatana_GetNumericTupleSize<T>::value);
    }

    /// Utility for copying string types that require an intermediate
    /// translation for use with Katana APIs. (ie. VtTokenArray ->
    /// StringAttribute)
    template <typename T = ElementType>
    static
        typename std::enable_if<VtKatana_HoldsString<T>::value, AttrType>::type
        Copy(const VtArray<T>& array) {
        std::vector<const char*> intermediate =
            VtKatana_ExtractStringVec(array);
        size_t size = intermediate.size();
        auto dataBegin = intermediate.data();
        AttrType attr(dataBegin, size, 1);
        return attr;
    }

    /// Utility for copying std::string types
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsString<T>::value, AttrType>::type
    Copy(const VtArray<T>& array) {
        AttrType attr(array.cdata(), array.size(), 1);
        return attr;
    }

    /// Utility for copying numeric types that don't require
    /// an intermediate copy (ie. VtVec3fArray -> FloatAttribute)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   AttrType>::type
    Copy(const std::vector<float>& times,
         const std::vector<VtArray<T>>& values) {
        typedef typename VtKatana_GetNumericScalarType<ElementType>::type
            ScalarType;
        TF_VERIFY(times.size() == values.size() && !times.empty() &&
                  !values.front().empty());
        size_t size =
            values.front().size() * VtKatana_GetNumericTupleSize<T>::value;
        std::vector<const ScalarType*> ptrs(values.size());
        std::transform(values.cbegin(), values.cend(), ptrs.begin(),
                       [](const VtArray<T>& array) {
                           return VtKatana_GetScalarPtr(array);
                       });
        AttrType attr(times.data(), times.size(), ptrs.data(), size,
                      VtKatana_GetNumericTupleSize<T>::value);
        return attr;
    }

    /// Utility for copying string holder types that don't require
    /// intermediate copies
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsOrHoldsString<T>::value,
                                   AttrType>::type
    Copy(const std::vector<float>& times,
         const std::vector<VtArray<T>>& values) {
        TF_VERIFY(times.size() == values.size() && !times.empty() &&
                  !values.front().empty());
        size_t size = values.front().size();
        std::vector<std::vector<const char*>> holders;
        std::transform(values.begin(), values.end(),
                       std::back_inserter(holders),
                       [](const VtArray<T>& array) {
                           return VtKatana_ExtractStringVec(array);
                       });
        std::vector<const char* const*> holdersHolder;
        std::transform(holders.begin(), holders.end(),
                       std::back_inserter(holdersHolder),
                       [](const std::vector<const char*>& holder) {
                           return holder.data();
                       });
        AttrType attr(times.data(), times.size(),
                      const_cast<const char***>(holdersHolder.data()), size, 1);
        return attr;
    }

    /// Utility for copying numeric types that do require intermediate copies
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCopyRequired<T>::value,
                                   AttrType>::type
    Copy(const std::vector<float>& times,
         const typename std::vector<VtArray<T>>& values) {
        BuilderType builder;
        auto timesIt = times.begin();
        for (const VtArray<T>& value : values) {
            builder.set(CopyIntermediate(value), *timesIt);
            ++timesIt;
        }
        return builder.build();
    }

    /// Iternals of map for types that are not castable, requiring an
    /// intermediate copy of values to construct an attribute (ie. uint,
    /// GfVec3h)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCopyRequired<T>::value,
                                   AttrType>::type
    MapInternal(const VtArray<T>& value) {
        return Copy(value);
    }

    /// Iternals of map for string like types
    /// We haven't found signifcant zero copy performance improvements
    /// in practice and the implementation is complicated, so we've decided
    /// against  exposing ZeroCopy for string like types for now.
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsOrHoldsString<T>::value,
                                   AttrType>::type
    MapInternal(const VtArray<T>& value) {
        return Copy(value);
    }

    /// Iternals of map for types that do not require an intermediate
    /// copy of values to construct an attribute (ie. double, GfMatrix4d,
    /// GfVec3f)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   AttrType>::type
    MapInternal(const VtArray<T>& value) {
        static const bool zeroCopyEnabled =
            TfGetEnvSetting(VTKATANA_ENABLE_ZERO_COPY_ARRAYS);
        if (zeroCopyEnabled)
            return ZeroCopy(value);
        else
            return Copy(value);
    }

    /// Iternals of map for types that are not castable, requiring an
    /// intermediate copy of values to construct an attribute (ie. uint,
    /// GfVec3h)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCopyRequired<T>::value,
                                   AttrType>::type
    MapInternalMultiple(const std::vector<float>& times,
                        const typename std::vector<VtArray<T>>& values) {
        return Copy(times, values);
    }

    /// Iternals of map for string types
    /// We haven't found signifcant zero copy performance improvements
    /// in practice and the implementation is complicated, so we've decided
    /// against  exposing ZeroCopy for string like types for now.
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsOrHoldsString<T>::value,
                                   AttrType>::type
    MapInternalMultiple(const std::vector<float>& times,
                        const typename std::vector<VtArray<T>>& values) {
        return Copy(times, values);
    }

    /// Iternals of map for types that do not require an intermediate
    /// copy of values to construct an attribute (ie. double, GfMatrix4d,
    /// GfVec3f)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   AttrType>::type
    MapInternalMultiple(const std::vector<float>& times,
                        const typename std::vector<VtArray<T>>& values) {
        static const bool zeroCopyEnabled =
            TfGetEnvSetting(VTKATANA_ENABLE_ZERO_COPY_ARRAYS);
        if (zeroCopyEnabled)
            return ZeroCopy(times, values);
        else
            return Copy(times, values);
    }

    /// Utility for copying string holder types
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsOrHoldsString<T>::value,
                                   AttrType>::type
    CopyElement(const T& value) {
        return AttrType(VtKatana_GetText<T>(value));
    }
    
    /// Utility for copying numeric scalar types
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericScalar<T>::value,
                                   AttrType>::type
    CopyElement(T value) {
        return AttrType(value);
    }

    /// Utility for copying numeric scalar types
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastableTuple<T>::value,
                                   AttrType>::type
    CopyElement(const T& value) {
        auto data = value.data();
        size_t size = VtKatana_GetNumericTupleSize<T>::value;
        return AttrType(data, size, size);
    }

    /// Utility for copying numeric tuple types
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCopyRequiredTuple<T>::value,
                                   AttrType>::type
    CopyElement(const T& value) {
        typedef typename VtKatana_GetNumericCopyTuplePeer<T>::type TupleCopyPeer;
        static_assert(
            std::is_same<typename VtKatana_GetNumericScalarType<T>::type,
                         GfHalf>::value, "");
        static_assert(VtKatana_GetNumericTupleSize<T>::value ==
                           VtKatana_GetNumericTupleSize<TupleCopyPeer>::value, "");
        TupleCopyPeer peer(value);
        auto data = peer.data();
        size_t size = VtKatana_GetNumericTupleSize<T>::value;
        return AttrType(data, size, size);
    }
};
}

PXR_NAMESPACE_CLOSE_SCOPE
