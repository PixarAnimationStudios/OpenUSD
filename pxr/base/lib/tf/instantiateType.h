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
/*
 * This header is not meant to be included in a .h file.
 * Complain if we see this header twice through.
 */

#ifdef TF_INSTANTIATETYPE_H
#error This file should only be included once in any given source (.cpp) file.
#endif

#define TF_INSTANTIATETYPE_H

#include "pxr/pxr.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/refPtr.h"

PXR_NAMESPACE_OPEN_SCOPE

template <typename T, bool AS_REF_PTR>
struct Tf_TypeFactoryType {
    struct FactoryType : public TfType::FactoryBase {
        TfRefPtr<T> New() { return T::New(); }
    };
};
template <class T>
struct TfTest_RefPtrFactory {
};

template <typename T>
struct Tf_TypeFactoryType<T, false> {
    struct FactoryType : public TfType::FactoryBase {
        T* New() { return new T; }
    };
};

// Make the type actually manufacturable.
template <typename T, bool MANUFACTURABLE>
struct Tf_MakeTypeManufacturable {
    static void Doit(TfType t) {
        typedef typename Tf_TypeFactoryType<T, TF_SUPPORTS_REFPTR(T)>::FactoryType FT;
        t.SetFactory<FT>();
    }
};

// Don't make it manufacturable.
template <typename T>
struct Tf_MakeTypeManufacturable<T, false> {
    static void Doit(TfType) {
    }
};
    
#define _TF_REMOVE_PARENS_HELPER(...) __VA_ARGS__
#define _TF_REMOVE_PARENS(parens) _TF_REMOVE_PARENS_HELPER parens

#define TF_NO_PARENT()            (TfType::Bases<>)
#define TF_1_PARENT(p1)            (TfType::Bases<p1 >)
#define TF_2_PARENT(p1,p2)  (TfType::Bases<p1, p2 >)
#define TF_INSTANTIATE_TYPE(Type, flags, Bases) \
    TF_REGISTRY_DEFINE_WITH_TYPE(TfType, Type) { \
        TfType t1 = TfType::Define<Type, _TF_REMOVE_PARENS(Bases) >(); \
        Tf_MakeTypeManufacturable<Type, flags&TfType::MANUFACTURABLE>::Doit(t1); \
    }

PXR_NAMESPACE_CLOSE_SCOPE
