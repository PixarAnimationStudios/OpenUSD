//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

    // Edge cases.
    ARCH_AXIOM(ArchGetPrettierFunctionName("operator<", "bool operator<(X, Y)") 
             == "operator<");
    ARCH_AXIOM(ArchGetPrettierFunctionName("operator<", "bool Z<W>::operator<(Y) const [with W = int]") 
             == "Z<W>::operator< [with W = int]");
    ARCH_AXIOM(ArchGetPrettierFunctionName("operator<<", "int operator<<(X, int)") 
             == "operator<<");

    return 0;
}

