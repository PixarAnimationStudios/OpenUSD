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
#include "pxr/base/gf/rect2i.h"

#include "pxr/base/gf/ostreamHelpers.h"

#include "pxr/base/tf/type.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

// CODE_COVERAGE_OFF_GCOV_BUG
TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfRect2i>();
}
// CODE_COVERAGE_ON_GCOV_BUG

GfRect2i
GfRect2i::GetNormalized() const
{
    GfVec2i lower, higher;

    if (_higher[0] < _lower[0]) {
        lower[0] = _higher[0];
        higher[0] = _lower[0];
    }
    else {
        lower[0] = _lower[0];
        higher[0] = _higher[0];
    }

    if (_higher[1] < _lower[1]) {
        lower[1] = _higher[1];
        higher[1] = _lower[1];
    }
    else {
        lower[1] = _lower[1];
        higher[1] = _higher[1];
    }

    return GfRect2i(lower, higher);
}

std::ostream &
operator<<(std::ostream& out, const GfRect2i& r)
{
    return out << '[' << Gf_OstreamHelperP(r.GetLower()) << ":" 
        << Gf_OstreamHelperP(r.GetHigher()) << ']';
}

PXR_NAMESPACE_CLOSE_SCOPE
