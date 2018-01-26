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
#ifndef USDSKEL_ANIMMAPPER_H
#define USDSKEL_ANIMMAPPER_H

/// \file usdSkel/animMapper.h

#include "pxr/pxr.h"
#include "pxr/usd/usdSkel/api.h"

#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/types.h"

#include <type_traits>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


using UsdSkelAnimMapperRefPtr = std::shared_ptr<class UsdSkelAnimMapper>;


/// \class UsdSkelAnimMap
///
/// Helper class for remapping vectorized animation data from
/// one ordering of tokens to another.
class UsdSkelAnimMapper {
public:
    /// Construct a null mapper.
    USDSKEL_API
    UsdSkelAnimMapper();
    
    /// Construct a mapper for mapping data from \p sourceOrder to
    /// \p targetOrder.
    USDSKEL_API
    UsdSkelAnimMapper(const VtTokenArray& sourceOrder,
                      const VtTokenArray& targetOrder);

    /// Typed remapping of data from \p source into \p target.
    /// The \p source array provides a run of \p elementSize elements for each
    /// path in the \\em sourceOrder. These elements are remapped and copied
    /// over the \p target array.
    /// Prior to remapping, the \p target array is resized to the size of the
    /// \\em targetOrder (as given at mapper construction time) multiplied by
    /// the \p elementSize. New elements created in the array are initialized
    /// to \p defaultValue, if provided.
    template <typename T>
    bool Remap(const VtArray<T>& source,
               VtArray<T>* target,
               int elementSize=1,
               const T* defaultValue=nullptr) const {
        static_assert(!std::is_pointer<T>::value, "");
        static_assert(SdfValueTypeTraits<T>::IsValueType, "");
        static_assert(!VtIsArray<T>::value, "");
        return _Remap(source, target, elementSize);
    }

    /// Type-erased remapping of data from \p source into \p target.
    /// The \p source array provides a run of \p elementSize elements for each
    /// path in the \\em sourceOrder. These elements are remapped and copied
    /// over the \p target array.
    /// Prior to remapping, the \p target array is resized to the size of the
    /// \\em targetOrder (as given at mapper construction time) multiplied by
    /// the \p elementSize. New elements created in the array are initialized
    /// to \p defaultValue, if provided.
    /// Remapping is supported for registered Sdf array value types only.
    USDSKEL_API
    bool Remap(const VtValue& source, VtValue* target,
               int elementSize=1, const VtValue& defaultValue=VtValue()) const;

    /// Convenience method for the common task of remapping transform arrays.
    /// This performs the same operation as Remap(), but sets the matrix
    /// identity as the default value.
    USDSKEL_API
    bool RemapTransforms(const VtMatrix4dArray& source,
                         VtMatrix4dArray* target,
                         int elementSize=1) const;

    /// Returns true if this is an identity map.
    /// The source and target orders of an identity map are identical.
    USDSKEL_API
    bool IsIdentity() const;

    /// Returns true if this is a sparse mapping.
    /// A sparse mapping means that not all target values will be overridden
    /// by source values, when mapped with Remap().
    USDSKEL_API
    bool IsSparse() const;

    /// Returns true if this is a null mapping.
    /// No source elements of a null map are mapped to the target.
    USDSKEL_API
    bool IsNull() const;

    bool operator==(const UsdSkelAnimMapper& o) const;

    bool operator!=(const UsdSkelAnimMapper& o) const {
        return !(*this == o);
    }

private:
    template <typename T>
    bool _Remap(const VtArray<T>& source, VtArray<T>* target,
                int elementSize, const T* defaultValue=nullptr) const;

    template <typename T>
    bool _UntypedRemap(const VtValue& source, VtValue* target,
                       int elementSize, const VtValue& defaultValue) const;

    /// Size of the output map.
    size_t _targetSize;

    /// For ordered mappings, an offset into the output array at which 
    /// to map the source data.
    size_t _offset;
    
    /// For unordered mappings, an index map, mapping from source
    /// indices to target indices.
    VtIntArray _indexMap;
    int _flags;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSKEL_ANIMMAPPER_H
