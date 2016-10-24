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
#include "pxr/base/arch/defines.h"

#if !defined(ARCH_OS_WINDOWS)
#include "pxr/base/tf/patternMatcher.h"
#include "pxr/base/tf/regTest.h"

static bool
Test_TfPatternMatcher()
{
    bool status = true;
    std::string toast = "i like toast";
    std::string toast2 = "i like ToaST";
    TfPatternMatcher pm;
    pm.SetPattern("oast");
    pm.SetIsGlobPattern(true);
    pm.SetIsCaseSensitive(true);
    std::string err = "foo";
    status &= pm.Match(toast, &err);
    err = "foo";
    status &= !pm.Match(toast2, &err);
    pm.SetPattern("oast\\");
    status &= !pm.Match(toast, &err);

    // A loose date/time match regexp.
    TfPatternMatcher dt(
        "^[0-9]{4}/[0-9]{2}/[0-9]{2}(:[0-9]{2}:[0-9]{2}:[0-9]{2})?");
    status &= dt.Match("2009/01/01");
    status &= dt.Match("2009/01/01:12:34:56");
    status &= !dt.Match("01/01/2009");

    return status;
}

TF_ADD_REGTEST(TfPatternMatcher);

#endif // #if !defined(ARCH_OS_WINDOWS)