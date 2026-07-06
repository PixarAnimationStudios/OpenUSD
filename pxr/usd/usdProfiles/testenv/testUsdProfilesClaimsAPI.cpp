//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/claimsAPI.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"

#include <cstdio>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------

static UsdStageRefPtr
_MakeStage(UsdProfilesClaimsAPI *apiOut)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim prim = stage->DefinePrim(SdfPath("/TestPrim"));
    *apiOut = UsdProfilesClaimsAPI::Apply(prim);
    TF_AXIOM(*apiOut);
    return stage;
}

//----------------------------------------------------------------------------
// TestSchemaApply
//----------------------------------------------------------------------------

static void
TestSchemaApply()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim prim = stage->DefinePrim(SdfPath("/P"));

    // CanApply succeeds on a plain prim
    std::string whyNot;
    TF_AXIOM(UsdProfilesClaimsAPI::CanApply(prim, &whyNot));
    TF_AXIOM(whyNot.empty());

    // Apply returns a valid API object
    UsdProfilesClaimsAPI api = UsdProfilesClaimsAPI::Apply(prim);
    TF_AXIOM(api);

    // Get round-trips to the same prim
    UsdProfilesClaimsAPI got = UsdProfilesClaimsAPI::Get(stage, SdfPath("/P"));
    TF_AXIOM(got);
    TF_AXIOM(got.GetPrim() == prim);

    printf("  TestSchemaApply passed\n");
}

//----------------------------------------------------------------------------
// TestCapabilityUsages
//----------------------------------------------------------------------------

static void
TestCapabilityUsages()
{
    UsdProfilesClaimsAPI api;
    UsdStageRefPtr stage = _MakeStage(&api);

    // Initially empty
    TF_AXIOM(api.GetCapabilityUsages().empty());
    TF_AXIOM(api.GetCapabilityUsage(TfToken("usd.geom.mesh")).IsEmpty());

    // Set single capability
    api.SetCapabilityUsage(TfToken("usd.geom.mesh"), TfToken("hard"));
    TF_AXIOM(api.GetCapabilityUsage(TfToken("usd.geom.mesh")) == TfToken("hard"));

    // Set a second capability
    api.SetCapabilityUsage(TfToken("usd.shading"), TfToken("soft"));
    TF_AXIOM(api.GetCapabilityUsage(TfToken("usd.shading")) == TfToken("soft"));

    // Bulk read
    VtDictionary usages = api.GetCapabilityUsages();
    TF_AXIOM(usages.size() == 2);
    TF_AXIOM(usages.count("usd.geom.mesh"));
    TF_AXIOM(usages["usd.geom.mesh"].Get<TfToken>() == TfToken("hard"));

    // Overwrite existing entry
    api.SetCapabilityUsage(TfToken("usd.geom.mesh"), TfToken("enhancement"));
    TF_AXIOM(api.GetCapabilityUsage(TfToken("usd.geom.mesh")) == TfToken("enhancement"));

    // Bulk replace via SetCapabilityUsages wipes previous entries
    VtDictionary replacement;
    replacement["usd.physics"] = VtValue(TfToken("hard"));
    api.SetCapabilityUsages(replacement);
    VtDictionary after = api.GetCapabilityUsages();
    TF_AXIOM(after.size() == 1);
    TF_AXIOM(after.count("usd.physics"));
    TF_AXIOM(!after.count("usd.geom.mesh"));

    // Unknown capability returns empty token
    TF_AXIOM(api.GetCapabilityUsage(TfToken("no.such.cap")).IsEmpty());

    printf("  TestCapabilityUsages passed\n");
}

//----------------------------------------------------------------------------
// TestProfileCompatibilityDeclarations
//----------------------------------------------------------------------------

static void
TestProfileCompatibilityDeclarations()
{
    UsdProfilesClaimsAPI api;
    UsdStageRefPtr stage = _MakeStage(&api);

    // Initially no profiles declared
    TF_AXIOM(api.GetCompatibleProfiles().empty());
    TF_AXIOM(api.GetProfileExceptions(TfToken("profile.a")).empty());

    // Declare fully compatible (no exceptions)
    api.SetProfileCompatible(TfToken("profile.a"));
    {
        VtArray<TfToken> profiles = api.GetCompatibleProfiles();
        TF_AXIOM(profiles.size() == 1);
        TF_AXIOM(profiles[0] == TfToken("profile.a"));
        TF_AXIOM(api.GetProfileExceptions(TfToken("profile.a")).empty());
    }

    // Declare compatible with exceptions
    VtArray<TfToken> exceptions = {
        TfToken("usd.geom.mesh"), TfToken("usd.shading"),
    };
    api.SetProfileCompatibleWithExceptions(TfToken("profile.b"), exceptions);
    {
        VtArray<TfToken> profiles = api.GetCompatibleProfiles();
        TF_AXIOM(profiles.size() == 2);

        VtArray<TfToken> ex = api.GetProfileExceptions(TfToken("profile.b"));
        TF_AXIOM(ex.size() == 2);
        bool hasMesh = false, hasShading = false;
        for (const auto& t : ex) {
            if (t == TfToken("usd.geom.mesh"))  hasMesh    = true;
            if (t == TfToken("usd.shading"))    hasShading = true;
        }
        TF_AXIOM(hasMesh && hasShading);
    }

    // Overwrite profile.a with exceptions
    api.SetProfileCompatibleWithExceptions(
        TfToken("profile.a"), {TfToken("usd.geom.hair")});
    {
        VtArray<TfToken> ex = api.GetProfileExceptions(TfToken("profile.a"));
        TF_AXIOM(ex.size() == 1);
        TF_AXIOM(ex[0] == TfToken("usd.geom.hair"));
    }

    // Clear one profile; the other remains
    api.ClearProfileCompatibility(TfToken("profile.a"));
    {
        VtArray<TfToken> profiles = api.GetCompatibleProfiles();
        TF_AXIOM(profiles.size() == 1);
        TF_AXIOM(profiles[0] == TfToken("profile.b"));
        TF_AXIOM(api.GetProfileExceptions(TfToken("profile.a")).empty());
    }

    // GetProfileExceptions on an undeclared profile returns empty
    TF_AXIOM(api.GetProfileExceptions(TfToken("never.declared")).empty());

    printf("  TestProfileCompatibilityDeclarations passed\n");
}

//----------------------------------------------------------------------------
// TestProfilesInfoRoundTrip
//----------------------------------------------------------------------------

static void
TestProfilesInfoRoundTrip()
{
    UsdProfilesClaimsAPI api;
    UsdStageRefPtr stage = _MakeStage(&api);

    TF_AXIOM(api.GetProfilesInfo().empty());

    // Write a raw dictionary and read it back
    VtDictionary usages;
    usages["usd.geom.mesh"] = VtValue(TfToken("hard"));
    VtDictionary info;
    info["capabilityUsages"] = VtValue(usages);
    api.SetProfilesInfo(info);

    VtDictionary got = api.GetProfilesInfo();
    TF_AXIOM(got.count("capabilityUsages"));

    // The high-level API reads through the same storage
    TF_AXIOM(api.GetCapabilityUsage(TfToken("usd.geom.mesh")) == TfToken("hard"));

    printf("  TestProfilesInfoRoundTrip passed\n");
}

//----------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------

int
main(int argc, char* argv[])
{
    printf("testUsdProfilesClaimsAPI\n");

    TestSchemaApply();
    TestCapabilityUsages();
    TestProfileCompatibilityDeclarations();
    TestProfilesInfoRoundTrip();

    printf("All tests passed\n");
    return 0;
}
