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

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/vt/array.h"

#include "vtKatana/internalTraits.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace VtKatana_Internal {

/// The VtKatana_SampleSource holds a reference to the katana
/// attribute to preserve the lifetime of all its samples.
template <typename AttributeType>
class VtKatana_SampleSource : public Vt_ArrayForeignDataSource {
    typedef typename AttributeType::array_type SampleType;

public:
    explicit VtKatana_SampleSource(const AttributeType& attribute)
        : Vt_ArrayForeignDataSource(VtKatana_SampleSource::_Detached),
          _dataAttribute(attribute) {}
    SampleType _GetNearestSample(float sample) {
        return _dataAttribute.getNearestSample(sample);
    }

private:
    static void _Detached(Vt_ArrayForeignDataSource* selfBase) {
        auto* self = static_cast<VtKatana_SampleSource*>(selfBase);
        delete self;
    }

private:
    AttributeType _dataAttribute;
};

/// Conversion utilities for Katana attributes to VtArrays containing type T
template <typename ElementType>
class VtKatana_ToVtConversion {
    typedef typename VtKatana_GetKatanaAttrType<ElementType>::type AttrType;
    typedef
        typename VtKatana_GetKatanaAttrValueType<ElementType>::type ValueType;
    typedef typename AttrType::array_type SampleType;
    typedef VtKatana_SampleSource<AttrType> ZeroCopySource;

    template <typename T = ElementType>
    static
        typename std::enable_if<VtKatana_IsNumericScalar<T>::value, void>::type
        _CheckAndWarnSize(size_t size) {
        static_assert(VtKatana_GetNumericTupleSize<T>::value == 1, "");
    }

    template <typename T = ElementType>
    static
        typename std::enable_if<VtKatana_IsNumericTuple<T>::value, void>::type
        _CheckAndWarnSize(size_t size) {
        if (size % VtKatana_GetNumericTupleSize<T>::value != 0) {
            TF_WARN(
                "Elements sequence will be truncated because size '%zu' is "
                "not divisible by element tuple size '%zu'.",
                size, VtKatana_GetNumericTupleSize<T>::value);
        }
    }

    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastableTuple<T>::value,
                                   const VtArray<T>>::type
    _CopyUnaligned(const SampleType& sample) {
        size_t size = sample.size() / VtKatana_GetNumericTupleSize<T>::value;
        VtArray<T> result(size);
        for (size_t i = 0; i < size;
             i += VtKatana_GetNumericTupleSize<T>::value) {
            result[i] =
                T(sample.data()[i * VtKatana_GetNumericTupleSize<T>::value]);
        }
        return result;
    }

    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastableScalar<T>::value,
                                   const VtArray<T>>::type
    _CopyUnaligned(const SampleType& sample) {
        VtArray<T> result;
        result.assign(sample.begin(), sample.end());
        return result;
    }

public:
    /// Zero copy utility for numeric types (GfVec3f, GfMatrix4d, ...).
    /// The resulting VtArray retains a reference to the attribute
    /// to avoid the copy.
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   const VtArray<T>>::type
    ZeroCopy(const AttrType& attribute, float time) {
        typedef typename VtKatana_GetNumericScalarType<ElementType>::type
            ScalarType;
        std::unique_ptr<ZeroCopySource> source(new ZeroCopySource(attribute));
        auto sample = source->_GetNearestSample(time);
        _CheckAndWarnSize<T>(sample.size());
        T* data = NULL;
        size_t size = sample.size() / VtKatana_GetNumericTupleSize<T>::value;
        if (VtKatana_IsSampleAligned<T>(sample)) {
            // GetVtPtr's reinterpret cast requires aligned data.
            data = const_cast<T*>(VtKatana_GetVtPtr<T>(sample));
            VtArray<T> result(source.release(), data, size);
            return result;
        } else {
            return _CopyUnaligned(sample);
        }
    }

    /// Copy utility for numeric types that don't require an intermediate
    /// copy to construct a VtArray  (ie. FloatAttr->VtVec3fArray)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   VtArray<T>>::type
    Copy(const SampleType& sample) {
        _CheckAndWarnSize<T>(sample.size());
        size_t size = sample.size() / VtKatana_GetNumericTupleSize<T>::value;
        if (VtKatana_IsSampleAligned<T>(sample)) {
            auto dataBegin = VtKatana_GetVtPtr<T>(sample);
            auto dataEnd = dataBegin + size;
            VtArray<T> result;
            result.assign(dataBegin, dataEnd);
            return result;
        } else {
            return _CopyUnaligned(sample);
        }
    }

    /// Copy utility for numeric scalar types require an intermediate copy
    /// to construct a VtArray (ie. IntAttr->VtBoolArray)
    template <typename T = ElementType>
    static
        typename std::enable_if<VtKatana_IsNumericCopyRequiredScalar<T>::value,
                                VtArray<T>>::type
        Copy(const SampleType& array) {
        typedef typename VtKatana_GetNumericScalarType<ElementType>::type
            ScalarType;
        VtArray<ScalarType> result;
        result.assign(array.begin(), array.end());
        return result;
    }

    /// Copy utility for numeric tuple types require an intermediate copy
    /// to construct a VtArray (ie. FloatAttr->VtVec3hArray)
    template <typename T = ElementType>
    static
        typename std::enable_if<VtKatana_IsNumericCopyRequiredTuple<T>::value,
                                VtArray<T>>::type
        Copy(const SampleType& array) {
        typedef
            typename VtKatana_GetNumericCopyTuplePeer<T>::type TupleCopyPeer;
        typedef typename VtKatana_GetNumericScalarType<ElementType>::type
            ScalarType;
        // This only works on GfHalf based types currently.
        // See VtKatana_GetNumericCopyTuplePeer for more info.
        static_assert(std::is_same<ScalarType, GfHalf>::value, "");
        static_assert(VtKatana_GetNumericTupleSize<T>::value ==
                          VtKatana_GetNumericTupleSize<TupleCopyPeer>::value,
                      "");
        _CheckAndWarnSize<T>(array.size());
        size_t size = array.size() / VtKatana_GetNumericTupleSize<T>::value;
        auto data = reinterpret_cast<const TupleCopyPeer*>(array.data());
        VtArray<T> result(size);
        std::transform(data, data + size, result.begin(),
                       [](const TupleCopyPeer& peer) { return T(peer); });
        return result;
    }

    /// Copy utility for std::string
    template <typename T = ElementType>
    static
        typename std::enable_if<VtKatana_IsString<T>::value, VtArray<T>>::type
        Copy(const SampleType& sample) {
        VtArray<T> result;
        result.assign(sample.begin(), sample.end());
        return result;
    }

    /// Copy utility for holders of std::string
    /// (This assumes that the holder can be constructed
    ///  from a string. This is trivially true for TfToken,
    ///  but there's some amibguity here with what the right
    ///  thing to do with SdfAssetPath which takes one or two
    ///  strings as an input.)
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_HoldsString<T>::value,
                                   VtArray<T>>::type
    Copy(const SampleType& sample) {
        VtArray<T> result(sample.size());
        std::transform(sample.begin(), sample.end(), result.begin(),
                       [](const std::string& element) { return T(element); });
        return result;
    }

    /// Maps a given \p attribute \p sample to a VtArray.
    /// Specialization for types that require an intermediate copy
    /// for translation.
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCopyRequired<T>::value ||
                                       VtKatana_IsOrHoldsString<T>::value,
                                   const VtArray<T>>::type
    MapInternal(const AttrType& attribute, float sample = 0.0f) {
        auto nearestSample = attribute.getNearestSample(sample);
        return Copy(nearestSample);
    }

    /// Maps a given \p attribute \p sample to a VtArray.
    /// Specialization for types that DO NOT require an intermediate copy
    /// for translation.
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                                   const VtArray<T>>::type
    MapInternal(const AttrType& attribute, float sample = 0.0f) {
        static bool zeroCopyEnabled =
            TfGetEnvSetting(VTKATANA_ENABLE_ZERO_COPY_ARRAYS);
        if (zeroCopyEnabled) {
            return ZeroCopy(attribute, sample);
        } else {
            auto nearestSample = attribute.getNearestSample(sample);
            return Copy(nearestSample);
        }
    }
    
    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsNumericScalar<T>::value, T>::type
    CopyElement(const AttrType& attr, float time) {
        auto nearestSample = attr.getNearestSample(time); 
        return nearestSample.data()[0]; 
    }

    template <typename T = ElementType>
    static typename std::enable_if<GfIsGfVec<T>::value && VtKatana_IsNumericCastable<T>::value, T>::type
    CopyElement(const AttrType& attr, float time) {
        auto nearestSample = attr.getNearestSample(time);
        return T(nearestSample.data()); 
    }

    template <typename T = ElementType>
    static typename std::enable_if<GfIsGfVec<T>::value && !VtKatana_IsNumericCastable<T>::value, T>::type
    CopyElement(const AttrType& attr) {
        typedef
            typename VtKatana_GetNumericCopyTuplePeer<T>::type TupleCopyPeer;
        typedef typename VtKatana_GetNumericScalarType<ElementType>::type
            ScalarType; 
        // This only works on GfHalf based types currently.
        // See VtKatana_GetNumericCopyTuplePeer for more info.
        static_assert(std::is_same<ScalarType, GfHalf>::value, "");
        static_assert(VtKatana_GetNumericTupleSize<T>::value ==
                          VtKatana_GetNumericTupleSize<TupleCopyPeer>::value,
                      "");  
        auto nearestSample = attr.getNearestSample(0.0f);
        const TupleCopyPeer* peer = reinterpret_cast<const TupleCopyPeer*>(nearestSample.data());
        return T(*peer); 
   }

    template <typename T = ElementType>
    static typename std::enable_if<GfIsGfMatrix<T>::value, T>::type
    CopyElement(const AttrType& attr) {
        typedef typename VtKatana_GetNumericScalarType<ElementType>::type
            ScalarType;
        auto nearestSample = attr.getNearestSample(0.0f);
        auto data = reinterpret_cast<const ScalarType(*)[T::numRows]>(
            nearestSample.data());
        return T(data); 
    }

    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_IsString<T>::value, T>::type
    CopyElement(const AttrType& attr) {
        static const std::string defValue;
        return attr.getValue(defValue, false);
    }

    template <typename T = ElementType>
    static typename std::enable_if<VtKatana_HoldsString<T>::value, T>::type
    CopyElement(const AttrType& attr) {
        static const std::string defValue;
	std::string value = attr.getValue(defValue, false);
        return T(value);
    }
};
}

PXR_NAMESPACE_CLOSE_SCOPE
