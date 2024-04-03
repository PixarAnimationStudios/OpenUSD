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

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/preprocessorUtils.h"
#include <string.h>

PXR_NAMESPACE_USING_DIRECTIVE

static bool
TestTF_NUM_ARGS()
{
    static_assert(TF_NUM_ARGS() == 0, "");
    static_assert(TF_NUM_ARGS( ) == 0, "");
    static_assert(TF_NUM_ARGS(/**/) == 0, "");
    static_assert(TF_NUM_ARGS(/*Test*/) == 0, "");
    static_assert(TF_NUM_ARGS(()) == 1, "");
    static_assert(TF_NUM_ARGS(f()) == 1, "");
    static_assert(TF_NUM_ARGS(f()()) == 1, "");
    static_assert(TF_NUM_ARGS((a)) == 1, "");
    static_assert(TF_NUM_ARGS(((a))) == 1, "");
    static_assert(TF_NUM_ARGS((()())) == 1, "");

    static_assert(TF_NUM_ARGS(a) == 1, "");
    static_assert(TF_NUM_ARGS(a, b) == 2, "");
    static_assert(TF_NUM_ARGS(a, b, c) == 3, "");
    static_assert(TF_NUM_ARGS(a, b, c, d) == 4, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e) == 5, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f) == 6, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g) == 7, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h) == 8, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i) == 9, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j) == 10, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k) == 11, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l) == 12, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m) == 13, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n) == 14, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o) == 15, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) == 16, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q) == 17, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r) == 18, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s) == 19, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t) == 20, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u) == 21, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v) == 22, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w) == 23, "");
    static_assert(TF_NUM_ARGS(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x) == 24, "");

    return true;
}

static bool
Test_TfPreprocessorUtils()
{
    return TestTF_NUM_ARGS();
}

TF_ADD_REGTEST(TfPreprocessorUtils);
