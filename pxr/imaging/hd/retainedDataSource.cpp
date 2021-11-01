//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/assetPath.h"

#include <algorithm>
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

class Hd_EmptyContainerDataSource : public HdRetainedContainerDataSource
{
public:
    bool Has(const TfToken &name) override 
    {
        return false;
    }

    TfTokenVector GetNames() override 
    {
        return {};
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override 
    {
        return nullptr;
    }
};

//-----------------------------------------------------------------------------

// Linear storage/search for containers with small numbers of children.
template <size_t T>
class Hd_SmallRetainedContainerDataSource
    : public HdRetainedContainerDataSource
{
public:
    static const size_t capacity = T;
    HD_DECLARE_DATASOURCE(Hd_SmallRetainedContainerDataSource<T>);

    bool Has(const TfToken &name) override 
    {
        for (size_t i = 0; i < _count; ++i) {
            if (name == _names[i]) {
                return true;
            }
        }
        return false;
    }

    TfTokenVector GetNames() override 
    {
        TfTokenVector result;
        result.reserve(_count);

        for (size_t i = 0; i < _count; ++i) {
            result.push_back(_names[i]);
        }

        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override 
    {
        for (size_t i = 0; i < _count; ++i) {
            if (name == _names[i]) {
                return _values[i];
            }
        }
        return nullptr;
    }

private:
    Hd_SmallRetainedContainerDataSource(
        size_t count, 
        const TfToken *names,
        const HdDataSourceBaseHandle *values)
    {
        if (count > capacity) {
            TF_CODING_ERROR(
                "Count %zu is greater than capacity %zu. Truncating",
                    count, capacity);
            count = capacity;
        }

        size_t dstidx = 0;
        for (size_t i = 0; i < count; ++i) {
            if (values[i]) {
                _names[dstidx] = names[i];
                _values[dstidx] = values[i];
                ++dstidx;
            }
        }

        _count = dstidx;
    }

    TfToken _names[T];
    HdDataSourceBaseHandle _values[T];
    size_t _count;
};

// -------------------------------------

// Fallback any-sized container.
template <size_t T>
class Hd_MappedRetainedContainerDataSource
    : public HdRetainedContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(Hd_MappedRetainedContainerDataSource<T>);

    bool Has(const TfToken &name) override
    {
        return _values.find(name) != _values.end();
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        result.reserve(_values.size());

        for (const auto I : _values) {
            result.push_back(I.first);
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        const auto I = _values.find(name);
        if (I == _values.end()) {
            return nullptr;
        }
        return (*I).second;
    }

private:
    Hd_MappedRetainedContainerDataSource(
        size_t count, 
        const TfToken *names,
        const HdDataSourceBaseHandle *values)
    {
        _values.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            if (values[i]) {
                _values[names[i]] = values[i];
            }
        }
    }

    TfDenseHashMap<TfToken, HdDataSourceBaseHandle,
        TfToken::HashFunctor, std::equal_to<TfToken>, T> _values;
};

} //namespace anonymous

HdRetainedContainerDataSource::Handle
HdRetainedContainerDataSource::New(
    size_t count, 
    const TfToken * names,
    const HdDataSourceBaseHandle *values)
{
    switch (count)
    {
    case 0:
    {
        static const HdRetainedContainerDataSourceHandle emptyContainer(
            new Hd_EmptyContainerDataSource);
        return emptyContainer;
    }
    case 1:
        return Hd_SmallRetainedContainerDataSource<1>::New(
            count, names, values);
    case 2:
        return Hd_SmallRetainedContainerDataSource<2>::New(
            count, names, values);
    case 3:
        return Hd_SmallRetainedContainerDataSource<3>::New(
            count, names, values);
    case 4:
        return Hd_SmallRetainedContainerDataSource<4>::New(
            count, names, values);
    case 5:
        return Hd_SmallRetainedContainerDataSource<5>::New(
            count, names, values);
    case 6:
        return Hd_SmallRetainedContainerDataSource<6>::New(
            count, names, values);
    case 7:
        return Hd_SmallRetainedContainerDataSource<7>::New(
            count, names, values);
    case 8:
        return Hd_SmallRetainedContainerDataSource<8>::New(
            count, names, values);
    case 9:
        return Hd_SmallRetainedContainerDataSource<9>::New(
            count, names, values);
    case 10:
        return Hd_SmallRetainedContainerDataSource<10>::New(
            count, names, values);
    case 11:
        return Hd_SmallRetainedContainerDataSource<11>::New(
            count, names, values);
    case 12:
        return Hd_SmallRetainedContainerDataSource<12>::New(
            count, names, values);
    case 13:
        return Hd_SmallRetainedContainerDataSource<13>::New(
            count, names, values);
    case 14:
        return Hd_SmallRetainedContainerDataSource<14>::New(
            count, names, values);
    case 15:
        return Hd_SmallRetainedContainerDataSource<15>::New(
            count, names, values);
    case 16:
        return Hd_SmallRetainedContainerDataSource<16>::New(
            count, names, values);
    default:
        // any-sized via dense hash map
        return Hd_MappedRetainedContainerDataSource<32>::New(
            count, names, values);
    }
}

HdRetainedContainerDataSource::Handle
HdRetainedContainerDataSource::New()
{
    return New(0, nullptr, nullptr);
}

HdRetainedContainerDataSource::Handle
HdRetainedContainerDataSource::New(
    const TfToken &name1,
    const HdDataSourceBaseHandle &value1)
{
    TfToken names[] = {name1};
    HdDataSourceBaseHandle values[] = {value1};
    return New(1, names, values);
}

HdRetainedContainerDataSource::Handle
HdRetainedContainerDataSource::New(
    const TfToken &name1,
    const HdDataSourceBaseHandle &value1,
    const TfToken &name2,
    const HdDataSourceBaseHandle &value2)
{
    TfToken names[] = {name1, name2};
    HdDataSourceBaseHandle values[] = {value1, value2};
    return New(2, names, values);
}

HdRetainedContainerDataSource::Handle
HdRetainedContainerDataSource::New(
    const TfToken &name1,
    const HdDataSourceBaseHandle &value1,
    const TfToken &name2,
    const HdDataSourceBaseHandle &value2,
    const TfToken &name3,
    const HdDataSourceBaseHandle &value3)
{
    TfToken names[] = {name1, name2, name3};
    HdDataSourceBaseHandle values[] = {value1, value2, value3};
    return New(3, names, values);
}

HdRetainedContainerDataSource::Handle
HdRetainedContainerDataSource::New(
    const TfToken &name1,
    const HdDataSourceBaseHandle &value1,
    const TfToken &name2,
    const HdDataSourceBaseHandle &value2,
    const TfToken &name3,
    const HdDataSourceBaseHandle &value3,
    const TfToken &name4,
    const HdDataSourceBaseHandle &value4)
{
    TfToken names[] = {name1, name2, name3, name4};
    HdDataSourceBaseHandle values[] = {value1, value2, value3, value4};
    return New(4, names, values);
}

HdRetainedContainerDataSource::Handle
HdRetainedContainerDataSource::New(
    const TfToken &name1,
    const HdDataSourceBaseHandle &value1,
    const TfToken &name2,
    const HdDataSourceBaseHandle &value2,
    const TfToken &name3,
    const HdDataSourceBaseHandle &value3,
    const TfToken &name4,
    const HdDataSourceBaseHandle &value4,
    const TfToken &name5,
    const HdDataSourceBaseHandle &value5)
{
    TfToken names[] = {name1, name2, name3, name4, name5};
    HdDataSourceBaseHandle values[] =
        {value1, value2, value3, value4, value5};
    return New(5, names, values);
}

HdRetainedContainerDataSource::Handle
HdRetainedContainerDataSource::New(
    const TfToken &name1,
    const HdDataSourceBaseHandle &value1,
    const TfToken &name2,
    const HdDataSourceBaseHandle &value2,
    const TfToken &name3,
    const HdDataSourceBaseHandle &value3,
    const TfToken &name4,
    const HdDataSourceBaseHandle &value4,
    const TfToken &name5,
    const HdDataSourceBaseHandle &value5,
    const TfToken &name6,
    const HdDataSourceBaseHandle &value6)
{
    TfToken names[] = {name1, name2, name3, name4, name5, name6};
    HdDataSourceBaseHandle values[] =
        {value1, value2, value3, value4, value5, value6};
    return New(6, names, values);
}

// ----------------------------------------------------------------------------

HdRetainedSmallVectorDataSource::HdRetainedSmallVectorDataSource(
    size_t count, 
    HdDataSourceBaseHandle *values) : 
    _values(count)
{
    for (size_t i = 0; i < count; ++i) {
        if (values[i]) {
            _values[i] = values[i];
        }
    }
}

size_t 
HdRetainedSmallVectorDataSource::GetNumElements()
{
    return _values.size();
}

HdDataSourceBaseHandle 
HdRetainedSmallVectorDataSource::GetElement(size_t element)
{
    if (element >= _values.size()) return HdDataSourceBaseHandle();
    return _values[element];
}

// ----------------------------------------------------------------------------

HdSampledDataSource::Handle
HdCreateTypedRetainedDataSource(VtValue const &v)
{
    if (v.IsHolding<int>()) {
        return HdRetainedTypedSampledDataSource<int>::New(
                v.UncheckedGet<int>());
    } else if (v.IsHolding<VtArray<int>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<int>>::New(
                v.UncheckedGet<VtArray<int>>());
    } else if (v.IsHolding<float>()) {
        return HdRetainedTypedSampledDataSource<float>::New(
                v.UncheckedGet<float>());
    } else if (v.IsHolding<VtArray<float>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<float>>::New(
                v.UncheckedGet<VtArray<float>>());
    } else if (v.IsHolding<double>()) {
        return HdRetainedTypedSampledDataSource<double>::New(
                v.UncheckedGet<double>());
    } else if (v.IsHolding<VtArray<double>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<double>>::New(
                v.UncheckedGet<VtArray<double>>());
    } else if (v.IsHolding<bool>()) {
        return HdRetainedTypedSampledDataSource<bool>::New(
                v.UncheckedGet<bool>());
    } else if (v.IsHolding<VtArray<bool>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<bool>>::New(
                v.UncheckedGet<VtArray<bool>>());
    } else if (v.IsHolding<TfToken>()) {
        return HdRetainedTypedSampledDataSource<TfToken>::New(
                v.UncheckedGet<TfToken>());
    } else if (v.IsHolding<VtArray<TfToken>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<TfToken>>::New(
                v.UncheckedGet<VtArray<TfToken>>());
    } else if (v.IsHolding<SdfPath>()) {
        return HdRetainedTypedSampledDataSource<SdfPath>::New(
                v.UncheckedGet<SdfPath>());
    } else if (v.IsHolding<VtArray<SdfPath>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                v.UncheckedGet<VtArray<SdfPath>>());
    } else if (v.IsHolding<SdfAssetPath>()) {
        return HdRetainedTypedSampledDataSource<SdfAssetPath>::New(
                v.UncheckedGet<SdfAssetPath>());
    } else if (v.IsHolding<VtArray<SdfAssetPath>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<SdfAssetPath>>::New(
                v.UncheckedGet<VtArray<SdfAssetPath>>());
    } else if (v.IsHolding<GfVec2f>()) {
        return HdRetainedTypedSampledDataSource<GfVec2f>::New(
                v.UncheckedGet<GfVec2f>());
    } else if (v.IsHolding<VtArray<GfVec2f>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<GfVec2f>>::New(
                v.UncheckedGet<VtArray<GfVec2f>>());
    } else if (v.IsHolding<GfVec3f>()) {
        return HdRetainedTypedSampledDataSource<GfVec3f>::New(
                v.UncheckedGet<GfVec3f>());
    } else if (v.IsHolding<VtArray<GfVec3f>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<GfVec3f>>::New(
                v.UncheckedGet<VtArray<GfVec3f>>());
    } else if (v.IsHolding<GfVec3d>()) {
        return HdRetainedTypedSampledDataSource<GfVec3d>::New(
                v.UncheckedGet<GfVec3d>());
    } else if (v.IsHolding<VtArray<GfVec3d>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<GfVec3d>>::New(
                v.UncheckedGet<VtArray<GfVec3d>>());
    } else if (v.IsHolding<GfMatrix4d>()) {
        return HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                v.UncheckedGet<GfMatrix4d>());
    } else if (v.IsHolding<VtArray<GfMatrix4d>>()) {
        return HdRetainedTypedSampledDataSource<VtArray<GfMatrix4d>>::New(
                v.UncheckedGet<VtArray<GfMatrix4d>>());
    } else if (v.IsHolding<SdfPathVector>()) {
        return HdRetainedTypedSampledDataSource<SdfPathVector>::New(
                v.UncheckedGet<SdfPathVector>());
    } else if (v.IsHolding<std::string>()) {
        return HdRetainedTypedSampledDataSource<std::string>::New(
                v.UncheckedGet<std::string>());
    } else if (v.IsEmpty()) {
        return HdSampledDataSourceHandle(nullptr);
    } else {
        TF_CODING_ERROR("Unsupported type %s", v.GetTypeName().c_str());
        return HdSampledDataSourceHandle(nullptr);
    }
}

//-----------------------------------------------------------------------------
// HdRetainedTypedSampledDataSource<>::New specializations

template <>
HdRetainedTypedSampledDataSource<bool>::Handle
HdRetainedTypedSampledDataSource<bool>::New(const bool &value)
{
    if (value) {
        const static HdRetainedTypedSampledDataSource<bool>::Handle ds(
            new HdRetainedTypedSampledDataSource<bool>(true));
        return ds;
    } else {
        const static HdRetainedTypedSampledDataSource<bool>::Handle ds(
            new HdRetainedTypedSampledDataSource<bool>(false));
        return ds;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
