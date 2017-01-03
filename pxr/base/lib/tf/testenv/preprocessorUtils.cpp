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
#define TF_MAX_ARITY 24

#include <boost/preprocessor/stringize.hpp>
#include <string.h>
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/preprocessorUtils.h"


static bool
TestTF_NUM_ARGS()
{
    TF_AXIOM(TF_NUM_ARGS() == 0);
    TF_AXIOM(TF_NUM_ARGS( ) == 0);
    TF_AXIOM(TF_NUM_ARGS(/**/) == 0);
    TF_AXIOM(TF_NUM_ARGS(/*Test*/) == 0);
    TF_AXIOM(TF_NUM_ARGS(()) == 1);
    TF_AXIOM(TF_NUM_ARGS(f()) == 1);
    TF_AXIOM(TF_NUM_ARGS(f()()) == 1);
    TF_AXIOM(TF_NUM_ARGS((a)) == 1);
    TF_AXIOM(TF_NUM_ARGS(((a))) == 1);
    TF_AXIOM(TF_NUM_ARGS((()())) == 1);

    //XXX: Figure out why these doesn't work.  See bug 8584.
    //TF_AXIOM(TF_NUM_ARGS(()f) == 1);
    //TF_AXIOM(TF_NUM_ARGS(()()) == 1);

    TF_AXIOM(TF_NUM_ARGS(a) == 1);
    TF_AXIOM(TF_NUM_ARGS(a, b) == 2);
    TF_AXIOM(TF_NUM_ARGS(a, b, c) == 3);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d) == 4);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e) == 5);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f) == 6);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g) == 7);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h) == 8);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i) == 9);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j) == 10);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k) == 11);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l) == 12);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m) == 13);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n) == 14);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) == 15);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) == 16);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q) == 17);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) == 18);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s) == 19);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t) == 20);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u) == 21);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v) == 22);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w) == 23);
    TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x) == 24);

    //XXX: TF_MAX_ARITY of 25 should work... See bug 8584.
    //TF_AXIOM(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y) == 25);

    return true;
}


#include <iostream>
static bool
TestTF_PP_EAT_PARENS()
{
    #define _STR BOOST_PP_STRINGIZE
    #define _EAT TF_PP_EAT_PARENS

    TF_AXIOM(!strcmp(_STR(), ""));
    TF_AXIOM(!strcmp(_STR(_EAT()), ""));
    TF_AXIOM(!strcmp(_STR(_EAT(())), ""));
    TF_AXIOM(!strcmp(_STR(_EAT(a)), "a"));
    TF_AXIOM(!strcmp(_STR(_EAT(a)), "a"));
    TF_AXIOM(!strcmp(_STR(_EAT((a))), "a"));
    TF_AXIOM(!strcmp(_STR(_EAT(((a)))), "(a)"));
    TF_AXIOM(!strcmp(_STR(_EAT(_EAT(((a))))), "a"));
    TF_AXIOM(!strcmp(_STR(_EAT(_EAT((((a)))))), "(a)"));
    TF_AXIOM(!strcmp(_STR(_EAT(_EAT(_EAT((((a))))))), "a"));

    TF_AXIOM(!strcmp(_STR((_EAT((<a, b>)))), "(<a, b>)"));
    TF_AXIOM(!strcmp(_STR((_EAT(_EAT(((<a, b>)))))), "(<a, b>)"));
    TF_AXIOM(!strcmp(_STR((_EAT(_EAT(_EAT((((<a, b>)))))))), "(<a, b>)"));

    TF_AXIOM(!strcmp(_STR(_EAT(f(a))), "f(a)"));

    //XXX: This isn't quite what we want; we would only expect _EAT()
    //     to remove the outermost _matching_ parentheses.
    //     See bug 8584.
    TF_AXIOM(!strcmp(_STR(_EAT((x)(x))), "x(x)"));
    TF_AXIOM(!strcmp(_STR(_EAT((x)f(x))), "xf(x)"));
    TF_AXIOM(!strcmp(_STR(_EAT((x)(x)(x))), "x(x)(x)"));
    //TF_AXIOM(!strcmp(_STR(_EAT((x)(x))), "(x)(x)"));
    //TF_AXIOM(!strcmp(_STR(_EAT((x)f(x))), "(x)f(x)"));
    //TF_AXIOM(!strcmp(_STR(_EAT((x)(x)(x))), "(x)(x)(x)"));

    #undef _STR
    #undef _EAT

    return true;
}


static bool
TestTF_PP_IS_TUPLE()
{
    #define _STR BOOST_PP_STRINGIZE

    TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE(())), "1"));
    TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE((a))), "1"));
    TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE(((a)))), "1"));
    TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE((a, b))), "1"));
    TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE((a, b, c))), "1"));

    TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE(a)), "0"));
    TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE(f(a))), "0"));
    TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE(This is a test)), "0"));

    // XXX: TF_PP_IS_TUPLE() should expand to 0 for sequences of size
    //      greater than 1...  See bug 8584.
    //TF_AXIOM(!strcmp(_STR(TF_PP_IS_TUPLE(()())), "0"));

    #undef _STR

    return true;
}


static bool
Test_TfPreprocessorUtils()
{
    return TestTF_NUM_ARGS() &&
           TestTF_PP_EAT_PARENS() &&
           TestTF_PP_IS_TUPLE()
           ;
}

TF_ADD_REGTEST(TfPreprocessorUtils);
