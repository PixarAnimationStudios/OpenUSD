//
// Copyright 2023 Pixar
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

#ifndef PXR_BASE_TS_MATH_UTILS_H
#define PXR_BASE_TS_MATH_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class GfMatrix4d;

// Solve the cubic polynomial time=f(u) where
// f(u) = c[0] + u * c[1] + u^2 * c[2] + u^3 * c[3].
// XXX: exported because used by templated functions starting from TsEvaluator
TS_API
double Ts_SolveCubic(const TsTime c[4], TsTime time);

// Take the first derivative of a cubic polynomial:
//   3*c[3]*u^2 + 2*c[2] + c[1]
template <typename T>
void
Ts_CubicDerivative( const T poly[4], double deriv[3] )
{
    deriv[2] = 3. * poly[3];
    deriv[1] = 2. * poly[2];
    deriv[0] =      poly[1];
}

// Solve f(x) = y for x in the given bounds where f is a cubic polynomial.
double
Ts_SolveCubicInInterval(
    const TsTime poly[4], const TsTime polyDeriv[3],
    TsTime y, const GfInterval& bounds);

// Solve for the roots of a quadratic equation.
bool
Ts_SolveQuadratic( const double poly[3], double *root0, double *root1 );

// Evaluate the quadratic polynomial in c[] at u.
template <typename T>
T
Ts_EvalQuadratic(const T c[3], double u)
{
    return u * (u * c[2] + c[1]) + c[0];
}

// Evaluate the cubic polynomial in c[] at u.
template <typename T>
T
Ts_EvalCubic(const T c[4], double u)
{
    return u * (u * (u * c[3] + c[2]) + c[1]) + c[0];
}

template <typename T>
T
Ts_EvalCubicDerivative(const T c[4], double u)
{
    return u * (u * 3.0 * c[3] + 2.0 * c[2]) + c[1];
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
