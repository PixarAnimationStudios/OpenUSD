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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/predicateLibrary.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    // SdfPredicateFunctionResult::Constancy
    TF_ADD_ENUM_NAME(SdfPredicateFunctionResult::ConstantOverDescendants);
    TF_ADD_ENUM_NAME(SdfPredicateFunctionResult::MayVaryOverDescendants);
}

size_t
SdfPredicateParamNamesAndDefaults::_CountDefaults() const
{
    size_t count = 0;
    for (Param const &p: _params) {
        if (!p.val.IsEmpty()) {
            ++count;
        }
    }
    return count;
}

bool
SdfPredicateParamNamesAndDefaults::CheckValidity() const
{
    // Basic check -- names cannot be empty, once an arg with a default appears,
    // all subsequent args must have defaults.
    TfErrorMark m;
    Param const *firstDefault = nullptr;
    for (Param const &param: _params) {
        if (param.name.empty()) {
            TF_CODING_ERROR(
                "Specified empty predicate expression parameter name");
        }
        bool hasDefault = !param.val.IsEmpty();
        if (firstDefault) {
            if (!hasDefault) {
                TF_CODING_ERROR("Non-default predicate function parameter "
                                "'%s' follows default parameter '%s'",
                                param.name.c_str(),
                                firstDefault->name.c_str());
            }
        }
        else if (hasDefault) {
            firstDefault = &param;
        }
    }
    return m.IsClean();
}

PXR_NAMESPACE_CLOSE_SCOPE
