//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeLimits.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using std::string;

// Tests to validate UsdAttributeLimits's templated limits API. See
// testUsdAttributeLimits.py for more limits test cases.

static void
TestBasicUsage()
{
    printf("TestBasicUsage...\n");

    const UsdStageRefPtr stage = UsdStage::CreateInMemory();
    const UsdPrim prim =
        stage->DefinePrim(SdfPath("/TestBasicUsage"));
    const UsdAttribute attr =
        prim.CreateAttribute(TfToken("attr"), SdfValueTypeNames->Int);

    const TfToken customKey("customKey");

    UsdAttributeLimits hard = attr.GetHardLimits();
    UsdAttributeLimits soft = attr.GetSoftLimits();
    UsdAttributeLimits custom = attr.GetLimits(TfToken("customSubDict"));

    std::vector<UsdAttributeLimits> limitses{ hard, soft, custom };

    for (UsdAttributeLimits& limits : limitses) {
        printf("  %s\n", limits.GetSubDictKey().GetText());

        // Limits dict is empty, simple getters should return empty
        TF_AXIOM(!limits.Get<int>(UsdLimitsKeys->Minimum).has_value());
        TF_AXIOM(!limits.Get<int>(UsdLimitsKeys->Maximum).has_value());
        TF_AXIOM(!limits.Get<string>(customKey).has_value());

        // "Or" getters should return the passed-in defaults
        TF_AXIOM(limits.GetOr(UsdLimitsKeys->Minimum, 3) == 3);
        TF_AXIOM(limits.GetOr(UsdLimitsKeys->Maximum, 7) == 7);
        TF_AXIOM(limits.GetOr(customKey, string("foo")) == "foo");

        // Set and confirm values
        TF_AXIOM(limits.SetMinimum(5));
        TF_AXIOM(limits.SetMaximum(10));
        TF_AXIOM(limits.Set(customKey, string("custom")));

        TF_AXIOM(limits.GetMinimumOr(3) == 5);
        TF_AXIOM(limits.GetMaximumOr(7) == 10);
        TF_AXIOM(limits.GetOr(customKey, string()) == "custom");

        const std::optional<int> min = limits.GetMinimum<int>();
        TF_AXIOM(min);
        TF_AXIOM(*min == 5);

        const std::optional<int> max = limits.GetMaximum<int>();
        TF_AXIOM(max);
        TF_AXIOM(*max == 10);

        const std::optional<string> customVal = limits.Get<string>(customKey);
        TF_AXIOM(customVal);
        TF_AXIOM(*customVal == "custom");
    }
}

static void
TestWrongTypes()
{
    printf("TestWrongTypes...\n");

    const UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->GetRootLayer()->ImportFromString(R"(
        #usda 1.0

        def "TestWrongTypes"
        {
            custom int attr = 7 (
                limits = {
                    dictionary soft = {
                        int minimum = 5
                        int maximum = 10
                        string customKey = "bleep"
                    }
                }
            )

            custom double badLimits = 7.0 (
                limits = {
                    dictionary hard = {
                        int minimumValue = 5
                        string maximumValue = "ten"
                    }
                }
            )
        }
        )");

    const UsdAttribute attr =
        stage->GetAttributeAtPath(SdfPath("/TestWrongTypes.attr"));

    UsdAttributeLimits soft = attr.GetSoftLimits();

    fprintf(stderr, "=== EXPECTED ERRORS ===\n");

    // Setting min/max with a type other than the attribute's value type
    // should fail and post an error
    {
        TfErrorMark mark;
        TF_AXIOM(!soft.SetMinimum(5.5));
        TF_AXIOM(!mark.IsClean());

        // Value should be unchanged
        const std::optional<int> min = soft.GetMinimum<int>();
        TF_AXIOM(min);
        TF_AXIOM(*min == 5);

        TF_AXIOM(soft.GetMinimumOr(3) == 5);
    }

    {
        TfErrorMark mark;
        TF_AXIOM(!soft.SetMaximum("foo"));
        TF_AXIOM(!mark.IsClean());

        // Value should be unchanged
        const std::optional<int> max = soft.GetMaximum<int>();
        TF_AXIOM(max);
        TF_AXIOM(*max == 10);

        TF_AXIOM(soft.GetMaximumOr(7) == 10);
    }

    fprintf(stderr, "=== END EXPECTED ERRORS ===\n");

    // Getting with the wrong type will fail but not post errors
    {
        TfErrorMark mark;
        TF_AXIOM(!soft.GetMinimum<bool>().has_value());
        TF_AXIOM(soft.GetMinimumOr(false) == false);
        TF_AXIOM(mark.IsClean());
    }

    {
        TfErrorMark mark;
        TF_AXIOM(!soft.GetMaximum<string>().has_value());
        TF_AXIOM(soft.GetMinimumOr(string("str")) == "str");
        TF_AXIOM(mark.IsClean());
    }

    {
        TfErrorMark mark;
        const TfToken customKey("customKey");

        TF_AXIOM(!soft.Get<int>(customKey).has_value());
        TF_AXIOM(soft.GetOr(customKey, 10) == 10);
        TF_AXIOM(mark.IsClean());
    }

    // Getting min/max with the right type when the stored value is of the
    // wrong type will also fail and also not post errors.
    //
    // Note that testUsdAttributeLimits (the Python version) uses the
    // VtValue-based "get" API, which _will_ return the wrong-typed values.
    const UsdAttribute badLimitsAttr =
        stage->GetAttributeAtPath(
            SdfPath("/TestWrongTypes.badLimits"));

    const UsdAttributeLimits badHard = badLimitsAttr.GetHardLimits();

    {
        TfErrorMark mark;
        TF_AXIOM(!badHard.GetMinimum<double>().has_value());
        TF_AXIOM(badHard.GetMinimumOr(10.5) == 10.5);
        TF_AXIOM(mark.IsClean());
    }

    {
        TfErrorMark mark;
        TF_AXIOM(!badHard.GetMaximum<double>().has_value());
        TF_AXIOM(badHard.GetMaximumOr(15.5) == 15.5);
        TF_AXIOM(mark.IsClean());
    }
}

int main()
{
    TestBasicUsage();
    TestWrongTypes();

    printf("\n\n>>> Test SUCCEEDED\n");
    return 0;
}
