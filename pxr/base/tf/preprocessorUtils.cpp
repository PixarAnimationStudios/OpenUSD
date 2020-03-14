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
#include "pxr/base/tf/preprocessorUtils.h"

#if !defined(ARCH_OS_WINDOWS)

#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/config/limits.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/comparison/greater_equal.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/paren.hpp>
#include <boost/preprocessor/tuple/eat.hpp>

#define _MAX_ARGS 64

/* Python code for generating The TF_ARG_N() macros:
for i in range(1, 32+1):
    line = '#define TF_ARG_%d(' % i
    for j in range(1, i):
        line += '_%d,' % j
    line += '_%d,...) _%d' % (i, i)
    print(line)
*/

// Make sure TF_ARG_N() supports at least BOOST_PP_LIMIT_TUPLE arguments.

#define _TF_ARG_ERROR(a, b, c)
#define _TF_ARG_TUPLE_OVERFLOW(a, b, c)

BOOST_PP_IIF(BOOST_PP_EQUAL(1,                                          \
    BOOST_PP_EXPAND(BOOST_PP_CAT(TF_ARG_, _MAX_ARGS) BOOST_PP_LPAREN()  \
    _TF                                                                 \
    BOOST_PP_REPEAT(BOOST_PP_SUB(_MAX_ARGS, 2), _TF_NUM_ARGS_REP, _TF)  \
    BOOST_PP_COMMA() 1 BOOST_PP_RPAREN())),                             \
    BOOST_PP_TUPLE_EAT(1), _TF_ARG_ERROR)(...)

BOOST_PP_IIF(BOOST_PP_GREATER_EQUAL(_MAX_ARGS, BOOST_PP_LIMIT_TUPLE), \
             BOOST_PP_TUPLE_EAT(1), _TF_ARG_TUPLE_OVERFLOW)(...)
 
#endif
