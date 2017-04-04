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

#include "pxr/pxr.h"
#include "pxr/base/arch/function.h"
#include "pxr/base/arch/error.h"

PXR_NAMESPACE_USING_DIRECTIVE

int main()
{
    // Non-member
    ARCH_AXIOM(ArchGetPrettierFunctionName("Bar", "int Bar(float)") 
             == "Bar");
 
    // Template non-member function
    ARCH_AXIOM(ArchGetPrettierFunctionName("Bar", "int Bar(C) [with C = int]") 
             == "Bar");
    ARCH_AXIOM(ArchGetPrettierFunctionName("Bar", "int Bar<C>(C)") 
             == "Bar");
 
    // Non-template class non-template member function
    ARCH_AXIOM(ArchGetPrettierFunctionName("Bar", "int Foo::Bar(float)") 
             == "Foo::Bar");
 
    // Template class member function
    ARCH_AXIOM(ArchGetPrettierFunctionName("Bar", 
                                         "int Foo<A>::Bar(float) [with A = int]")
             == "Foo<A>::Bar [with A = int]");

    // Multi-parameter template class
    ARCH_AXIOM(ArchGetPrettierFunctionName("Bar", 
                "int Foo<A,B>::Bar(float) [with A = int, B = int]")
             == "Foo<A,B>::Bar [with A = int, B = int]");

    // Template function
    ARCH_AXIOM(ArchGetPrettierFunctionName("Bar", 
                "A Foo<A, B>::Bar(C) [with C = double; B = float; A = int]")
             == "Foo<A, B>::Bar [with A = int, B = float]");

    // Linux-style nested templates
    ARCH_AXIOM(ArchGetPrettierFunctionName("foo", 
                "int X<A>::Y<B>::foo(A, B, C) [with C = bool; B = float; A = int]")
             == "X<A>::Y<B>::foo [with A = int, B = float]");

    // Linux-style nested templates with templates for template arguments
    ARCH_AXIOM(ArchGetPrettierFunctionName("foo", 
                "int X<A>::Y<B>::foo(A, B, C) [with C = bool; B = Z<char, double>::W<short int>; A = Z<char, double>]")
             == "X<A>::Y<B>::foo [with A = Z<char, double>, B = Z<char, double>::W<short int>]");

    // Windows-style nested templates
    ARCH_AXIOM(ArchGetPrettierFunctionName("foo", 
                "int __cdecl X<int>::Y<float>::foo<bool>(int,float,bool)")
             == "X<int>::Y<float>::foo");

    // Windows-style nested templates with templates for template arguments
    ARCH_AXIOM(ArchGetPrettierFunctionName("foo", 
                "int __cdecl X<Z<char,double> >::Y<Z<char,double>::W<short> >::foo<bool>(Z<char,double>,Z<char,double>::W<short>,bool)")
             == "X<Z<char,double> >::Y<Z<char,double>::W<short> >::foo");

    return 0;
}

