//
// Copyright 2021 Pixar
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

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/base/tf/errorMark.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

// NB: This test requires HD_USE_DEPRECATED_INSTANCER_PRIMVAR_NAMES to be set.
// It will fail if it is not set, but checking whether it is set in the test is
// counterproductive, since the purpose of the test is to check hydra's behavior
// when it is set, not to check whether it has been set.

bool TestDeprecatedPrimvarNames()
{
    static const TfTokenVector expected = {
        HdInstancerTokens->instanceTransform,
        HdInstancerTokens->rotate,
        HdInstancerTokens->scale,
        HdInstancerTokens->translate
    };

    const TfTokenVector& names = HdInstancer::GetBuiltinPrimvarNames();
    if (names.size() != expected.size()) {
        TF_WARN("Unexpected response size from GetBuiltinPrimvarNames; "
            "expected %zu, got %zu", expected.size(), names.size());
        return false;
    }

    for (size_t i = 0; i < expected.size(); ++i) {
        if (names[i] != expected[i]) {
            TF_WARN("names[%zu]: expected %s, got %s",
                i, expected[i].GetText(), names[i].GetText());
            return false;
        }
    }
    return true;
}

int main()
{
    TfErrorMark mark;
    bool success = TestDeprecatedPrimvarNames();
    TF_VERIFY(mark.IsClean());
    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
