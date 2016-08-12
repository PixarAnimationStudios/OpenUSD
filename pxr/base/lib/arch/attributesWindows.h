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
#ifndef ARCH_ATTRIBUTES_WINDOWS_H
#define ARCH_ATTRIBUTES_WINDOWS_H

#pragma section(".pxr$a", read)
#pragma section(".pxr$c", read)
#pragma section(".pxr$e", read)

typedef void(__cdecl *Arch_CtorType )(void);

template <typename T>
struct Arch_CtorA
{
    __declspec(allocate(".pxr$a")) static Arch_CtorType  x;
};
template <typename T> Arch_CtorType  Arch_CtorA<T>::x;

template <typename T>
struct Arch_CtorC
{
    __declspec(allocate(".pxr$c")) static Arch_CtorType  x;
};
template <typename T> Arch_CtorType  Arch_CtorC<T>::x;

template <typename T>
struct Arch_CtorE
{
    __declspec(allocate(".pxr$e")) static Arch_CtorType  x;
};
template <typename T> Arch_CtorType  Arch_CtorE<T>::x;

template struct Arch_CtorA<void>;
template struct Arch_CtorC<void>;
template struct Arch_CtorE<void>;

#define ARCH_PRIORITY_CAT(section, priority) ARCH_STRINGIFY(section ## priority)

#define ARCH_CONSTRUCTOR_DEFINE(priority, name, ...)                    \
    static void name(__VA_ARGS__);                                      \
    __pragma(section(ARCH_PRIORITY_CAT(.pxr$b, priority), read))        \
    __declspec(allocate(ARCH_PRIORITY_CAT(.pxr$b, priority)))           \
    Arch_CtorType  tf_ctor_ ## name = (Arch_CtorType )&name;            \
    static void name(__VA_ARGS__)

#define ARCH_CONSTRUCTOR(priority, tag, name)                           \
    __pragma(section(ARCH_PRIORITY_CAT(.pxr$b, priority), read))        \
    __declspec(allocate(ARCH_PRIORITY_CAT(.pxr$b, priority)))           \
    Arch_CtorType  tf_ctor_ ## tag = (Arch_CtorType )&name;             \

#define ARCH_DESTRUCTOR(priority, name, ...)                            \
    static void name(__VA_ARGS__);                                      \
    __pragma(section(ARCH_PRIORITY_CAT(.pxr$d, priority), read))        \
    __declspec(allocate(ARCH_PRIORITY_CAT(.pxr$d, priority)))           \
    Arch_CtorType  tf_dtor_ ## name = (Arch_CtorType )&name;            \
    static void name(__VA_ARGS__)

#define ARCH_CONSTRUCTOR_TEMPLATE_DECLARE(priority, name, ...)          \
    __pragma(section(ARCH_PRIORITY_CAT(.pxr$b, priority), read))        \
    __declspec(allocate(ARCH_PRIORITY_CAT(.pxr$b, priority)))           \
    static Arch_CtorType  tf_ctor_ ## name;                             \
    static void name(__VA_ARGS__)

#define ARCH_DESTRUCTOR_TEMPLATE_DECLARE(priority, name, ...)           \
    __pragma(section(ARCH_PRIORITY_CAT(.pxr$d, priority), read))        \
    __declspec(allocate(ARCH_PRIORITY_CAT(.pxr$d, priority)))           \
    static Arch_CtorType  tf_dtor_ ## name;                             \
    static void name(__VA_ARGS__)

#define ARCH_CONSTRUCTOR_TEMPLATE_DEFINE(priority, scope, name, ...)    \
template <> Arch_CtorType scope::tf_ctor_ ## name = (Arch_CtorType)&scope::name

#define ARCH_DESTRUCTOR_TEMPLATE_DEFINE(priority, scope, name, ...)     \
template <> Arch_CtorType scope::tf_dtor_ ## name = (Arch_CtorType)&scope::name


#define archRunConstructors()                                           \
    {                                                                   \
        Arch_CtorType * i = &Arch_CtorA<void>::x;                       \
        Arch_CtorType * n = &Arch_CtorC<void>::x;                       \
                                                                        \
        while (++i != n)                                                \
        {                                                               \
            if (*i)                                                     \
            {                                                           \
                (*i)();                                                 \
            }                                                           \
        }                                                               \
    }

#define archRunDestructors()                                            \
    {                                                                   \
        Arch_CtorType * i = &Arch_CtorE<void>::x;                       \
        Arch_CtorType * n = &Arch_CtorC<void>::x;                       \
                                                                        \
        while (--i != n)                                                \
        {                                                               \
            if (*i)                                                     \
            {                                                           \
                (*i)();                                                 \
            }                                                           \
        }                                                               \
    }
#endif // ARCH_ATTRIBUTES_WINDOWS_H 
