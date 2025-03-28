//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticLite.h"
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

