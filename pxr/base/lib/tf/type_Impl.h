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
#ifndef TF_TYPE_IMPL_H
#define TF_TYPE_IMPL_H

#include "pxr/base/tf/mallocTag.h"

#include <initializer_list>

PXR_NAMESPACE_OPEN_SCOPE

template <class DERIVED, class BASE>
inline void*
Tf_CastToParent(void* addr, bool derivedToBase);

// Declare and register casts for all the C++ bases in the given TypeVector.
template <typename TypeVector>
struct Tf_AddBases;

template <typename... Bases>
struct Tf_AddBases<TfType::Bases<Bases...>>
{
    // Declare types in Bases as TfTypes and accumulate them into a runtime
    // vector.
    static std::vector<TfType>
    Declare()
    {
        return std::vector<TfType> {
            TfType::Declare(
                TfType::GetCanonicalTypeName( typeid(Bases) ))...
        };
    }

    // Register casts to and from Derived and each base type in Bases.
    template <typename Derived>
    static void
    RegisterCasts(TfType const* type)
    {
        struct Cast
        {
            const std::type_info *typeInfo;
            TfType::_CastFunction func;
        };

        const std::initializer_list<Cast> baseCasts = {
            { &typeid(Bases), &Tf_CastToParent<Derived, Bases> }...
        };

        for (const Cast &cast : baseCasts) {
            type->_AddCppCastFunc(*cast.typeInfo, cast.func);
        }
    }
};

template <typename T, typename BaseTypes>
TfType const&
TfType::Define()
{
    TfAutoMallocTag2 tag2("Tf", "TfType::Define");

    // Declare each of the base types.
    std::vector<TfType> baseTfTypes = Tf_AddBases<BaseTypes>::Declare();

    // Declare our type T.
    const std::type_info &typeInfo = typeid(T);
    const std::string typeName = TfType::GetCanonicalTypeName(typeInfo);
    TfType const& newType = TfType::Declare(typeName, baseTfTypes);

    // Record traits information about T.
    const bool isPodType = std::is_pod<T>::value;
    const bool isEnumType = std::is_enum<T>::value;
    const size_t sizeofType = TfSizeofType<T>::value;

    newType._DefineCppType(typeInfo, sizeofType, isPodType, isEnumType);
    Tf_AddBases<BaseTypes>::template RegisterCasts<T>(&newType);

    return newType;
}

template <typename T>
TfType const&
TfType::Define()
{
    return Define<T, Bases<> >();
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

#endif // TF_TYPE_IMPL_H
