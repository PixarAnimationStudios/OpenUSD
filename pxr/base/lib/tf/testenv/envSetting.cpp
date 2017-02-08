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
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/arch/attributes.h"

using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_ENV_SETTING(TF_TEST_BOOL_ENV_SETTING_X, false, "bool env setting (not set by test)");
TF_DEFINE_ENV_SETTING(TF_TEST_BOOL_ENV_SETTING, false, "bool env setting");

TF_DEFINE_ENV_SETTING(TF_TEST_INT_ENV_SETTING_X, 1, "int env setting (not set by test)");
TF_DEFINE_ENV_SETTING(TF_TEST_INT_ENV_SETTING, 1, "int env setting");

TF_DEFINE_ENV_SETTING(TF_TEST_STRING_ENV_SETTING_X, "default",
		      "string env setting (not set by test)");

TF_DEFINE_ENV_SETTING(TF_TEST_STRING_ENV_SETTING, "default",
		      "string env setting");

TF_DEFINE_ENV_SETTING(TF_TEST_POST_ENV_SETTING_X, false, "post-registry-manager setting (not set by test)");

// This tests that there are no issues with getting an env setting related
// to global dynamic initialization order.  In particular, getting an env
// setting now should cause all of the TF_DEFINE_ENV_SETTING created env
// settings to be defined but we shouldn't try to define one twice.
ARCH_CONSTRUCTOR(_PostRegistryManager, 150, void)
{
    TF_AXIOM(TfGetEnvSetting(TF_TEST_POST_ENV_SETTING_X) == false);
}

static bool
Test_TfEnvSetting()
{
    TF_AXIOM(TfGetEnvSetting(TF_TEST_BOOL_ENV_SETTING_X) == false);
    TF_AXIOM(TfGetEnvSetting(TF_TEST_BOOL_ENV_SETTING) == true);

    TF_AXIOM(TfGetEnvSetting(TF_TEST_INT_ENV_SETTING_X) == 1);
    TF_AXIOM(TfGetEnvSetting(TF_TEST_INT_ENV_SETTING) == 123);

    TF_AXIOM(TfGetEnvSetting(TF_TEST_STRING_ENV_SETTING_X) == "default");
    TF_AXIOM(TfGetEnvSetting(TF_TEST_STRING_ENV_SETTING) == "alpha");
    return true;
}

TF_ADD_REGTEST(TfEnvSetting);

