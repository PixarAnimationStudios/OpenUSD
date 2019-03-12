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
#include "pxr/usd/usdSkel/animMapper.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/type.h"

#include <algorithm>
#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE

namespace {


enum _MapFlags {
    _NullMap = 0,

    _SomeSourceValuesMapToTarget = 0x1,
    _AllSourceValuesMapToTarget = 0x2,
    _SourceOverridesAllTargetValues = 0x4,
    _OrderedMap = 0x8,

    _IdentityMap = (_AllSourceValuesMapToTarget|
                    _SourceOverridesAllTargetValues|_OrderedMap),

    _NonNullMap = (_SomeSourceValuesMapToTarget|_AllSourceValuesMapToTarget)
};


} // namespace


UsdSkelAnimMapper::UsdSkelAnimMapper()
    : _targetSize(0), _offset(0), _flags(_NullMap)
{}


UsdSkelAnimMapper::UsdSkelAnimMapper(const VtTokenArray& sourceOrder,
                                     const VtTokenArray& targetOrder)
    : UsdSkelAnimMapper(sourceOrder.cdata(), sourceOrder.size(),
                        targetOrder.cdata(), targetOrder.size())
{}


UsdSkelAnimMapper::UsdSkelAnimMapper(const TfToken* sourceOrder,
                                     size_t sourceOrderSize,
                                     const TfToken* targetOrder,
                                     size_t targetOrderSize)
    : _targetSize(targetOrderSize), _offset(0)
{
    if(sourceOrderSize == 0 || targetOrderSize == 0) {
        _flags = _NullMap;
        return;
    } 

    {
        // Determine if this an ordered mapping of the source
        // onto the target, with a simple offset.
        // This includes identity maps.

        // Find where the first source element begins on the target.
        const auto it = std::find(targetOrder, targetOrder + targetOrderSize,
                                  sourceOrder[0]);
        const size_t pos = it - targetOrder;
        if((pos + sourceOrderSize) <= targetOrderSize) {
            if(std::equal(sourceOrder, sourceOrder+sourceOrderSize, it)) {
                _offset = pos;

                _flags = _OrderedMap | _AllSourceValuesMapToTarget;

                if(pos == 0 && sourceOrderSize == targetOrderSize) {
                    _flags |= _SourceOverridesAllTargetValues;
                }
                return;
            }
        }
    }

    // No ordered mapping can be produced.
    // Settle for an unordered, indexed mapping.

    // Need a map of path->targetIndex.
    std::unordered_map<TfToken,int,TfToken::HashFunctor> targetMap;
    for(size_t i = 0; i < targetOrderSize; ++i) {
        targetMap[targetOrder[i]] = static_cast<int>(i);
    }

    _indexMap.resize(sourceOrderSize);
    int* indexMap = _indexMap.data();
    size_t mappedCount = 0;
    std::vector<bool> targetMapped(targetOrderSize);
    for(size_t i = 0; i < sourceOrderSize; ++i) {
        auto it = targetMap.find(sourceOrder[i]);
        if(it != targetMap.end()) {
            indexMap[i] = it->second;
            targetMapped[it->second] = true;
            ++mappedCount;
        } else {
            indexMap[i] = -1;
        }
    }
    _flags = mappedCount == sourceOrderSize ? 
        _AllSourceValuesMapToTarget : _SomeSourceValuesMapToTarget;

    if(std::all_of(targetMapped.begin(), targetMapped.end(),
                   [](bool val) { return val; })) {
        _flags |= _SourceOverridesAllTargetValues;
    }
}


bool
UsdSkelAnimMapper::IsIdentity() const
{
    return (_flags&_IdentityMap) == _IdentityMap;
}

bool
UsdSkelAnimMapper::IsSparse() const
{
    return !(_flags&_SourceOverridesAllTargetValues);
}


bool
UsdSkelAnimMapper::IsNull() const
{
    return !(_flags&_NonNullMap);
}


bool
UsdSkelAnimMapper::_IsOrdered() const
{
    return _flags&_OrderedMap;
}


template <typename T>
bool
UsdSkelAnimMapper::_UntypedRemap(const VtValue& source,
                                 VtValue* target,
                                 int elementSize,
                                 const VtValue& defaultValue) const
{
    TF_DEV_AXIOM(source.IsHolding<VtArray<T> >());

    if (!target) {
        TF_CODING_ERROR("'target' pointer is null.");
        return false;
    }

    if (target->IsEmpty()) {
        *target = VtArray<T>();
    } else if (!target->IsHolding<VtArray<T> >()) {
        TF_CODING_ERROR("Type of 'target' [%s] did not match the type of "
                        "'source' [%s].", target->GetTypeName().c_str(),
                        source.GetTypeName().c_str());
        return false;
    }

    const T* defaultValueT = nullptr;
    if (!defaultValue.IsEmpty()) {
        if (defaultValue.IsHolding<T>()) {
            defaultValueT = &defaultValue.UncheckedGet<T>();
        } else {
            TF_CODING_ERROR("Unexpected type [%s] for defaultValue: expecting "
                            "'%s'.", defaultValue.GetTypeName().c_str(),
                            TfType::Find<T>().GetTypeName().c_str());
            return false;
        }
    }

    const auto& sourceArray = source.UncheckedGet<VtArray<T> >();
    auto targetArray = target->UncheckedGet<VtArray<T> >();
    if (Remap(sourceArray, &targetArray, elementSize, defaultValueT)) {
        *target = targetArray;
        return true;
    }
    return false;
}


bool
UsdSkelAnimMapper::Remap(const VtValue& source,
                         VtValue* target,
                         int elementSize,
                         const VtValue& defaultValue) const
{
#define _UNTYPED_REMAP(r, unused, elem)                                 \
    if(source.IsHolding<SDF_VALUE_TRAITS_TYPE(elem)::ShapedType>()) {   \
        return _UntypedRemap<SDF_VALUE_TRAITS_TYPE(elem)::Type>(        \
            source, target, elementSize, defaultValue);                 \
    }

BOOST_PP_SEQ_FOR_EACH(_UNTYPED_REMAP, ~, SDF_VALUE_TYPES);
#undef _UNTYPED_REMAP

    return false;
}


template <typename Matrix4>
bool
UsdSkelAnimMapper::RemapTransforms(const VtArray<Matrix4>& source,
                                   VtArray<Matrix4>* target,
                                   int elementSize) const
{
    static const Matrix4 identity(1);
    return Remap(source, target, elementSize, &identity);
}


template USDSKEL_API bool
UsdSkelAnimMapper::RemapTransforms(const VtMatrix4dArray&,
                                   VtMatrix4dArray*, int) const;

template USDSKEL_API bool
UsdSkelAnimMapper::RemapTransforms(const VtMatrix4fArray&,
                                   VtMatrix4fArray*, int) const;


bool
UsdSkelAnimMapper::operator==(const UsdSkelAnimMapper& o) const
{
    return _targetSize == o._targetSize &&
           _offset == o._offset &&
           _flags == o._flags &&
           _indexMap == o._indexMap;
}


PXR_NAMESPACE_CLOSE_SCOPE
