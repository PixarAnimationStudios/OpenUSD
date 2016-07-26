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
#ifndef TF_TRAITS_H
#define TF_TRAITS_H

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_const.hpp>

/*!
 * \file traits.h
 * \ingroup group_tf_RuntimeTyping
 */



class TfWeakPtrFacadeBase;

/*!
 * \class TfTraits Traits.h pxr/base/tf/traits.h
 * \ingroup group_tf_RuntimeTyping
 * \brief Type-querying abilities.
 *
 * \c TfTraits is a class used for compile-time type queries.
 * Types retrieved from \c TfTraits are compile-time types,
 * while values are compile-time values (i.e. constants).
 *
 * <B> Types </B>
 *
 * The following types are defined by \c TfTraits:
 * \code
 *    TfTraits::Type<T>::UnderlyingType
 * \endcode
 *
 * The above removes a single level of pointer-ness or a reference
 * from \c T, as well as any constness and returns the result.
 * For example, both \c int and \c int* are mapped to \c int,
 * but \c int** is mapped to \c int*.
 *
 * \code
 *    TfTraits::Type<T>::NoRefType;
 * \endcode
 *
 * The above transforms a reference-type into a non-reference type.
 * For example, both \c int& and \c int are mapped to \c int,
 * but \c int* is mapped to \c int*.
 *
 * \code
 *    TfTraits::Type<T>::PointerType;
 * \endcode
 *
 * The above transforms any type \c T to \c T*, except that
 * a type \c T& is transformed to \c T*.
 *
 * \code
 *     TfTraits::Type<T>::RefType
 * \endcode
 * The above transforms a non-reference-type into a reference type.
 * For example, both \c int& and \c int are mapped to \c int&. 
 * However, \c RefType is not defined if \c T is an array (either dimensioned
 * or undimensioned).
 * 
 * \code
 *     TfTraits::Type<T>::AvoidCopyingType
 * \endcode                             
 * 
 * The above maps all types \c T back to themselves, except for
 * a type that is neither a pointer nor reference; in this case,
 * the type is mapped to a reference to a const reference to itself
 * (i.e. an \c & ) is added to the type.
 *
 * \code
 *     TfTraits::Type<T>::PtrOrRefType
 * \endcode                             
 *
 * The above maps all types \c T back to themselves, except for
 * a type that is neither a pointer nor reference; in this case,
 * the type is mapped to a reference to a reference to itself
 * (i.e. an \c & ) is added to the type.  This differs from
 * \c AvoidCopyingType in that it does not produce a \c const reference.
 *
 * <B> Values </B>
 *
 * Any values retrieved using \c TfTraits are actually compile-time
 * constants; as a result, an \c if/else test with such a value usually
 * results in only one branch of the conditional actually generating code
 * (although both branches must be legal C++).
 *
 * The following boolean values are defined:
 *
 * \code
 *     TfTraits::Type<T>::isPointer
 *     TfTraits::Type<T>::isReference
 *     TfTraits::Type<T>::isConst
 *     TfTraits::TypeCompare<A,B>::isEqual
 * \endcode                             
 *
 * The \c isPointer value is true for any type \c T that is either a
 * pointer or an array; the \c isReference is true if \c T is a reference,
 * and \c isConst is true if \c T is constant qualified.
 *
 *
 * <B> Examples </B>
 *
 * \code
 * template <class T>
 * void PassThru(T arg1) {
 *     // this always avoids any significant copying:
 *     // cache1 is either a pointer or a reference
 *     TfTraits::Type<T>::AvoidCopyingType cache1 = arg1;
 *
 *     typedef TfTraits::Type<T>::UnderlyingType UT;
 *
 *     if (TfTraits::Type<UT>::isPointer) {
 *        cout << "hey, this is a pointer to a pointer!\n";
 *     }
 *     if (TfTraits::Type<T>::isConst) {
 *        cout << "original arg was constant\n";
 *     }
 * }
 * \endcode
 */

template <class T> class TfRefPtr;

struct TfTraits {
    
    template <class T, class Enable = void>
    struct Type {
        typedef T UnderlyingType;
        typedef T NoRefType;
        typedef T& RefType;
        typedef T* PointerType;
        typedef const T& AvoidCopyingType;
        typedef T& PtrOrRefType;
        
        static const bool isPointer = false;
        static const bool isReference = false;
        static const bool isConst = false;
    };

    template <class T>
    struct Type<const T> {
        typedef T UnderlyingType;
        typedef const T NoRefType;
        typedef const T& RefType;
        typedef const T* PointerType;
        typedef const T& AvoidCopyingType;
        typedef const T& PtrOrRefType;
        
        static const bool isPointer = false;
        static const bool isReference = false;
        static const bool isConst = true;
    };

    template <class T>
    struct Type<T*> {
        typedef T UnderlyingType;
        typedef T* NoRefType;
        typedef T*& RefType;
        typedef T** PointerType;
        typedef T* AvoidCopyingType;
        typedef T* PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = false;
    };

    template <class T>
    struct Type<const T*> {
        typedef T UnderlyingType;
        typedef const T* NoRefType;
        typedef const T*& RefType;
        typedef const T** PointerType;
        typedef const T* AvoidCopyingType;
        typedef const T* PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = true;
    };

    template <class T>
    struct Type<T[]> {
        typedef T UnderlyingType;
        typedef T* NoRefType;
        typedef T** PointerType;
        typedef T* AvoidCopyingType;
        typedef T* PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = false;
    };

    template <class T>
    struct Type<const T[]> {
        typedef T UnderlyingType;
        typedef const T* NoRefType;
        typedef const T** PointerType;
        typedef const T* AvoidCopyingType;
        typedef const T* PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = true;
    };

    template <class T, int n>
    struct Type<T[n]> {
        typedef T UnderlyingType;
        typedef T* NoRefType;
        typedef T** PointerType;
        typedef T* AvoidCopyingType;
        typedef T* PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = false;
    };

    template <class T, int n>
    struct Type<const T[n]> {
        typedef T UnderlyingType;
        typedef const T* NoRefType;
        typedef const T** PointerType;
        typedef const T* AvoidCopyingType;
        typedef const T* PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = true;
    };

    template <class T>
    struct Type<T&> {
        typedef T UnderlyingType;
        typedef T NoRefType;
        typedef T& RefType;
        typedef T* PointerType;
        typedef T& AvoidCopyingType;
        typedef T& PtrOrRefType;
        static const bool isPointer = false;
        static const bool isReference = true;
        static const bool isConst = false;
    };

    template <class T>
    struct Type<const T&> {
        typedef T UnderlyingType;
        typedef T NoRefType;
        typedef const T& RefType;
        typedef const T* PointerType;
        typedef const T& AvoidCopyingType;
        typedef const T& PtrOrRefType;
        static const bool isPointer = false;
        static const bool isReference = true;
        static const bool isConst = true;
    };

    
    template <template <class> class Ptr, class T>
    struct Type<Ptr<T>,
                typename boost::enable_if<
                    boost::is_base_of<TfWeakPtrFacadeBase, Ptr<T> >
                    >::type>
    {
        typedef T UnderlyingType;
        typedef Ptr<T> NoRefType;
        typedef Ptr<T> &RefType;
        typedef Ptr<T>* PointerType;
        typedef Ptr<T> AvoidCopyingType;
        typedef Ptr<T> PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = boost::is_const<T>::value;
    };
    
    template <template <class> class Ptr, class T>
    struct Type<Ptr<T> &,
                typename boost::enable_if<
                    boost::is_base_of<TfWeakPtrFacadeBase, Ptr<T> >
                    >::type>
    {
        typedef T UnderlyingType;
        typedef Ptr<T> NoRefType;
        typedef Ptr<T> &RefType;
        typedef Ptr<T>* PointerType;
        typedef Ptr<T> AvoidCopyingType;
        typedef Ptr<T> PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = boost::is_const<T>::value;
    };
    
    template <class T>
    struct Type<TfRefPtr<T> > {
        typedef T UnderlyingType;
        typedef TfRefPtr<T> NoRefType;
        typedef TfRefPtr<T>& RefType;
        typedef TfRefPtr<T>* PointerType;
        typedef TfRefPtr<T> AvoidCopyingType;
        typedef TfRefPtr<T> PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = boost::is_const<T>::value;
    };

    template <class T>
    struct Type<const TfRefPtr<T>&> {
        typedef T UnderlyingType;
        typedef TfRefPtr<T> NoRefType;
        typedef TfRefPtr<T>& RefType;
        typedef TfRefPtr<T>* PointerType;
        typedef TfRefPtr<T> AvoidCopyingType;
        typedef TfRefPtr<T> PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = boost::is_const<T>::value;
    };

    template <class T>
    struct Type<TfRefPtr<const T> > {
        typedef T UnderlyingType;
        typedef TfRefPtr<const T> NoRefType;
        typedef TfRefPtr<const T>& RefType;
        typedef TfRefPtr<const T>* PointerType;
        typedef TfRefPtr<const T> AvoidCopyingType;
        typedef TfRefPtr<const T> PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = true;
    };

    template <class T>
    struct Type<const TfRefPtr<const T>&> {
        typedef T UnderlyingType;
        typedef TfRefPtr<const T> NoRefType;
        typedef TfRefPtr<const T>& RefType;
        typedef TfRefPtr<const T>* PointerType;
        typedef TfRefPtr<const T> AvoidCopyingType;
        typedef TfRefPtr<const T> PtrOrRefType;
        static const bool isPointer = true;
        static const bool isReference = false;
        static const bool isConst = true;
    };
};

template <>
struct TfTraits::Type<void*> {
    typedef void* UnderlyingType;
    typedef void* NoRefType;
    typedef void*& RefType;
    typedef void** PointerType;
    typedef void* AvoidCopyingType;
    typedef void* PtrOrRefType;
    static const bool isPointer = true;
    static const bool isReference = false;
    static const bool isConst = false;
};

template <>
struct TfTraits::Type<const void*> {
    typedef void* UnderlyingType;
    typedef const void* NoRefType;
    typedef const void*& RefType;
    typedef const void** PointerType;
    typedef const void* AvoidCopyingType;
    typedef const void* PtrOrRefType;
    static const bool isPointer = true;
    static const bool isReference = false;
    static const bool isConst = true;
};

#endif
