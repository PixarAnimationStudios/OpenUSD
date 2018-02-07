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

#include "pxr/base/tf/type.h"

#include <algorithm>

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
    : _targetSize(targetOrder.size()), _offset(0)
{
    if(sourceOrder.size() == 0 || targetOrder.size() == 0) {
        _flags = _NullMap;
        return;
    } 

    {
        // Determine if this an ordered mapping of the source
        // onto the target, with a simple offset.
        // This includes identity maps.

        // Find where the first source element begins on the target.
        auto it = std::find(targetOrder.begin(), targetOrder.end(),
                            sourceOrder[0]);
        if(it != targetOrder.end()) {
            size_t pos = std::distance(targetOrder.begin(), it);
            size_t compareCount = std::min(sourceOrder.size(),
                                           targetOrder.size()-pos);
            if(std::equal(sourceOrder.data(),
                          sourceOrder.data()+compareCount, it)) {
                _offset = pos;

                _flags = _OrderedMap;

                if(pos == 0 && compareCount == targetOrder.size()) {
                    _flags |= _SourceOverridesAllTargetValues;
                }
                _flags |= compareCount == sourceOrder.size() ?
                    _AllSourceValuesMapToTarget : _SomeSourceValuesMapToTarget;
                return;
            }
        }
    }

    // No ordered mapping can be produced.
    // Settle for an unordered, indexed mapping.

    // Need a map of path->targetIndex.
    std::map<TfToken,int> targetMap;
    for(size_t i = 0; i < targetOrder.size(); ++i) {
        targetMap[targetOrder[i]] = static_cast<int>(i);
    }

    _indexMap.resize(sourceOrder.size());
    int* indexMap = _indexMap.data();
    size_t mappedCount = 0;
    std::vector<bool> targetMapped(targetOrder.size());
    for(size_t i = 0; i < sourceOrder.size(); ++i) {
        auto it = targetMap.find(sourceOrder[i]);
        if(it != targetMap.end()) {
            indexMap[i] = it->second;
            targetMapped[it->second] = true;
            ++mappedCount;
        } else {
            indexMap[i] = -1;
        }
    }
    _flags = mappedCount == sourceOrder.size() ? 
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


namespace {


template <typename T>
void _ResizeArray(VtArray<T>* array, size_t size, const T* defaultValue)
{
    if(defaultValue) {
        size_t prevSize = array->size();
        array->resize(size);
        T* data = array->data();
        for(size_t i = prevSize; i < size; ++i) {
            data[i] = *defaultValue;
        }
    } else {
        array->resize(size);
    }
}


} // namespace


template <typename T>
bool
UsdSkelAnimMapper::_Remap(const VtArray<T>& source,
                          VtArray<T>* target,
                          int elementSize,
                          const T* defaultValue) const
{
    if(!target) {
        TF_CODING_ERROR("'target' pointer is invalid.");
        return false;
    }
    if(elementSize <= 0) {
        TF_WARN("Invalid elementSize [%d]: "
                "size must be greater than zero.", elementSize);
        return false;
    }

    const size_t targetArraySize = _targetSize*elementSize;

    if(IsIdentity() && source.size() == targetArraySize) {
        // Can make a copy of the array (will share a reference to the source)
        *target = source;
        return true;
    }

    // Resize the target array to the expected size.
    _ResizeArray(target, targetArraySize, defaultValue);

    if(IsNull()) {
        return true;
    } else if(_flags&_OrderedMap) {

        _ResizeArray(target, _targetSize*elementSize, defaultValue);

        size_t copyCount =
            std::min(source.size(), targetArraySize - _offset*elementSize);
        std::copy(source.cdata(), source.cdata()+copyCount,
                  target->data() + _offset*elementSize);
    } else {

        const T* sourceData = source.cdata();
        T* targetData = target->data();
        size_t copyCount = std::min(source.size()/elementSize,
                                    _indexMap.size());

        const int* indexMap = _indexMap.data();

        for(size_t i = 0; i < copyCount; ++i) {
            int targetIdx = indexMap[i];
            if(targetIdx >= 0 && targetIdx < target->size()) {
                TF_DEV_AXIOM(i*elementSize < source.size());
                TF_DEV_AXIOM((i+1)*elementSize <= source.size());
                TF_DEV_AXIOM((targetIdx+1)*elementSize <= target->size());
                std::copy(sourceData + i*elementSize,
                          sourceData + (i+1)*elementSize,
                          targetData + targetIdx*elementSize);
            }
        }
    }
    return true;
}


// Explicitly instantiate templated Remap methods for all array-value sdf types.
#define _INSTANTIATE_REMAP(r, unused, elem)                     \
    template bool UsdSkelAnimMapper::Remap(                     \
        const SDF_VALUE_TRAITS_TYPE(elem)::ShapedType&,         \
        SDF_VALUE_TRAITS_TYPE(elem)::ShapedType*,               \
        int, const SDF_VALUE_TRAITS_TYPE(elem)::Type*) const;

BOOST_PP_SEQ_FOR_EACH(_INSTANTIATE_REMAP, ~, SDF_VALUE_TYPES);
#undef _INSTANTIATE_REMAP


template <typename T>
bool
UsdSkelAnimMapper::_UntypedRemap(const VtValue& source,
                                 VtValue* target,
                                 int elementSize,
                                 const VtValue& defaultValue) const
{
    TF_DEV_AXIOM(source.IsHolding<VtArray<T> >());

    if(!target) {
        TF_CODING_ERROR("'target' pointer is null.");
        return false;
    }

    if(target->IsEmpty()) {
        *target = VtArray<T>();
    } else if(!target->IsHolding<VtArray<T> >()) {
        TF_CODING_ERROR("Type of 'target' [%s] did not match the type of "
                        "'source' [%s].", target->GetTypeName().c_str(),
                        source.GetTypeName().c_str());
        return false;
    }

    const T* defaultValueT = nullptr;
    if(!defaultValue.IsEmpty()) {
        if(defaultValue.IsHolding<T>()) {
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
    if(_Remap(sourceArray, &targetArray, elementSize, defaultValueT)) {
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


bool
UsdSkelAnimMapper::RemapTransforms(const VtMatrix4dArray& source,
                                   VtMatrix4dArray* target,
                                   int elementSize) const
{
    static const GfMatrix4d identity(1);
    return Remap(source, target, elementSize, &identity);
}


bool
UsdSkelAnimMapper::operator==(const UsdSkelAnimMapper& o) const
{
    return _targetSize == o._targetSize &&
           _offset == o._offset &&
           _flags == o._flags &&
           _indexMap == o._indexMap;
}


PXR_NAMESPACE_CLOSE_SCOPE
