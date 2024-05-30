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
#ifndef PXR_BASE_TF_TYPE_IMPL_H
#define PXR_BASE_TF_TYPE_IMPL_H

PXR_NAMESPACE_OPEN_SCOPE

template <class DERIVED, class BASE>
inline void*
Tf_CastToParent(void* addr, bool derivedToBase);

template <typename TypeVector>
struct Tf_BaseTypeInfos;

template <>
struct Tf_BaseTypeInfos<TfType::Bases<>>
{
    static const size_t NumBases = 0;
    std::type_info const **baseTypeInfos = nullptr;
};

template <typename... Bases>
struct Tf_BaseTypeInfos<TfType::Bases<Bases...>>
{
    static const size_t NumBases = sizeof...(Bases);
    std::type_info const *baseTypeInfos[NumBases] = { &typeid(Bases)... };
};

template <class Derived, typename TypeVector>
struct Tf_TypeCastFunctions;

template <class Derived>
struct Tf_TypeCastFunctions<Derived, TfType::Bases<>>
{
    using CastFunction = void *(*)(void *, bool);
    CastFunction *castFunctions = nullptr;
};

template <class Derived, typename... Bases>
struct Tf_TypeCastFunctions<Derived, TfType::Bases<Bases...>>
{
    using CastFunction = void *(*)(void *, bool);
    CastFunction castFunctions[sizeof...(Bases)] = {
        &Tf_CastToParent<Derived, Bases>... };
};

template <class T, class BaseTypes>
TfType const &
TfType::Declare()
{
    Tf_BaseTypeInfos<BaseTypes> btis;
    return _DeclareImpl(typeid(T), btis.baseTypeInfos, btis.NumBases);
}

template <typename T, typename BaseTypes>
TfType const &
TfType::Define()
{
    Tf_BaseTypeInfos<BaseTypes> btis;
    Tf_TypeCastFunctions<T, BaseTypes> tcfs;
    return _DefineImpl(
        typeid(T), btis.baseTypeInfos, tcfs.castFunctions, btis.NumBases,
        TfSizeofType<T>::value, std::is_pod_v<T>, std::is_enum_v<T>);
}

template <typename T>
TfType const&
TfType::Define()
{
    return Define<T, Bases<>>();
}

// Helper function to implement up/down casts between TfType types.
// This was taken from the previous TfType implementation.
template <class DERIVED, class BASE>
inline void*
Tf_CastToParent(void* addr, bool derivedToBase)
{
    if (derivedToBase) {
        // Upcast -- can be done implicitly.
        DERIVED* derived = reinterpret_cast<DERIVED*>(addr);
        BASE* base = derived;
        return base;
    } else {
        // Downcast -- use static_cast.
        BASE* base = reinterpret_cast<BASE*>(addr);
        DERIVED* derived = static_cast<DERIVED*>(base);
        return derived;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_TYPE_IMPL_H
