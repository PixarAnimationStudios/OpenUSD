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

#ifndef PXR_BASE_TS_DIFF_H
#define PXR_BASE_TS_DIFF_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class TsSpline;

/// Returns the interval in which the splines \p s1 and \p s2 will
/// evaluate to different values or in which knots in the splines have
/// different values.
///
/// In particular, if the rightmost changed knot is a dual-valued knot
/// where the left value has changed and the right value is unchanged,
/// the returned interval will be closed on the right, even though the
/// value of the spline *at* the rightmost time does not change.
TS_API
GfInterval
TsFindChangedInterval(
    const TsSpline &s1,
    const TsSpline &s2);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
