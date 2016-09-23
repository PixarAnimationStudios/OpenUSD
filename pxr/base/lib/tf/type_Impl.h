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
#include "pxr/base/tf/singleton.h"

#include <boost/bind.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/remove.hpp>
#include <boost/mpl/empty.hpp>

// Recursive template for declaring the types in TypeVector as
// TfTypes and pushing them onto the result vector.
template <typename TypeVector,
          bool empty = boost::mpl::empty<TypeVector>::value >
struct Tf_DeclareTypes
{
    static void
    Apply( std::vector<TfType> *results )
    {
        typedef typename boost::mpl::front<TypeVector>::type ThisBase;
        typedef typename boost::mpl::pop_front<TypeVector>::type Rest;

        const std::string typeName =
            TfType::GetCanonicalTypeName( typeid(ThisBase) );

        results->push_back( TfType::Declare(typeName) );

        Tf_DeclareTypes<Rest>::Apply(results);
    }
};
template <typename TypeVector >
struct Tf_DeclareTypes<TypeVector, true /* empty vector */>
{
    static void
    Apply( std::vector<TfType> * )
    {
        // Base case; nothing left to do.
    }
};

// Recursive template for taking each of the types in TypeVector
// and adding them as BaseCppTypes for the given TypeDefiner.
template <typename DERIVED, typename TypeVector,
          bool empty = boost::mpl::empty<TypeVector>::value >
struct Tf_AddBases
{
    static void
    Apply( TfType::_TypeDefiner<DERIVED> & definer )
    {
        typedef typename boost::mpl::front<TypeVector>::type ThisBase;
        typedef typename boost::mpl::pop_front<TypeVector>::type Rest;

        // Apply a single entry of Bases
        definer.template _AddBaseCppType<ThisBase>();
        // Recurse
        Tf_AddBases<DERIVED, Rest>::Apply(definer);
    }
};
template <typename DERIVED, typename TypeVector >
struct Tf_AddBases<DERIVED, TypeVector, true /* empty vector */>
{
    static void
    Apply( TfType::_TypeDefiner<DERIVED> & )
    {
        // Base case; nothing left to do.
    }
};

template <typename T, typename B>
TfType const&
TfType::Define()
{
    TfAutoMallocTag2 tag2("Tf", "TfType::Define");
    
    // Convert B to an MPL typelist by filtering out our sentinel.
    typedef typename boost::mpl::remove< B, Unspecified >::type BaseTypes;

    // Declare each of the types.
    std::vector<TfType> baseTfTypes;
    Tf_DeclareTypes< BaseTypes >::Apply( &baseTfTypes );

    // Declare our type T.
    const std::string typeName = TfType::GetCanonicalTypeName( typeid(T) );
    TfType const& newType = TfType::Declare(typeName, baseTfTypes);

    // Declare base C++ types.
    _TypeDefiner<T> def(newType);
    def.template _AddBaseCppTypes<BaseTypes>();
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
static void*
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

// Add all the C++ bases in the given TypeVector.
template < typename DERIVED >
template < typename TypeVector >
TfType::_TypeDefiner<DERIVED> &
TfType::_TypeDefiner<DERIVED>::_AddBaseCppTypes()
{
    // icc needs to have the optional 3rd argument specified explicitly
    // e.g., add a 3rd template param, boost::mpl::empty<TypeVector>::value
    Tf_AddBases< DERIVED, TypeVector >::Apply(*this);
    return *this;
}

// Add a single C++ base type.
template < typename DERIVED >
template <typename BASE>
TfType::_TypeDefiner<DERIVED> &
TfType::_TypeDefiner<DERIVED>::_AddBaseCppType()
{
    _type->_AddCppCastFunc( typeid(BASE), Tf_CastToParent<DERIVED, BASE> );
    return *this;
}

#endif // TF_TYPE_IMPL_H
