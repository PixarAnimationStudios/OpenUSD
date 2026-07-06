//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/profileRegistry.h"
#include "pxr/usd/usdProfiles/claimsAPI.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"

#include <cstdio>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE
USDPROFILES_API void Usd_ProfilesRegistryTestClear();
USDPROFILES_API bool Usd_ProfilesRegistryTestLoadFromFile(
    const std::string& filePath,
    std::vector<std::string>* errors = nullptr);
PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

using CR = UsdProfileRegistry::CapabilityResult;
using QS = UsdProfileRegistry::QueryStatus;

static bool
_ResultContains(const std::vector<CR>& v, const TfToken& t)
{
    for (const auto& r : v) {
        if (r.capability == t) return true;
    }
    return false;
}

static QS
_ResultStatus(const std::vector<CR>& v, const TfToken& t)
{
    for (const auto& r : v) {
        if (r.capability == t) return r.status;
    }
    TF_AXIOM(false);
    return QS::NoPath;
}

//----------------------------------------------------------------------------
// TestIsProfile
//
// Factored from testUsdProfilesRegistry: verifies IsProfile() correctly
// reflects the "isProfile": true flag and leaves non-profile nodes false.
//----------------------------------------------------------------------------

static void
TestIsProfile()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testIsProfile.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    TF_AXIOM( UsdProfileRegistry::IsProfile(TfToken("usd.core.26.03")));
    TF_AXIOM( UsdProfileRegistry::IsProfile(TfToken("usd.core.26.11")));
    TF_AXIOM(!UsdProfileRegistry::IsProfile(TfToken("usd")));
    TF_AXIOM(!UsdProfileRegistry::IsProfile(TfToken("usd.geom")));
    TF_AXIOM(!UsdProfileRegistry::IsProfile(TfToken("usd.geom.mesh")));
    TF_AXIOM(!UsdProfileRegistry::IsProfile(TfToken("no.such.capability")));
    TF_AXIOM(!UsdProfileRegistry::IsProfile(TfToken()));

    // IsProfile doesn't affect normal capability queries
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd.core.26.03")));
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd.core.26.11")));
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("usd.core.26.03"), TfToken("usd")) == QS::ValidPath);
    // Profile supersession: 26.11 deprecates 26.03
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("usd.core.26.11"), TfToken("usd.core.26.03")) == QS::Deprecated);

    printf("  TestIsProfile passed\n");
}

//----------------------------------------------------------------------------
// TestCoversCapabilities
//
// Factored from testUsdProfilesRegistry: verifies the full contract of
// CoversCapabilities — aggregate status, per-result population, excepted caps.
//
// Graph (testProfileVersions.json):
//   usd <- usd.geom.mesh <- usd.geom.nurbs
//                        <- profile.25.11
//   profile.26.08 <- usd.geom.nurbs (valid)
//                 <- profile.25.11  (deprecated)
//----------------------------------------------------------------------------

static void
TestCoversCapabilities()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(
        "testProfileVersions.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    // All required caps reachable via valid paths
    {
        std::vector<TfToken> required = {
            TfToken("usd.geom.mesh"), TfToken("usd"),
        };
        std::vector<CR> results;
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("profile.25.11"), required, {}, &results);
        TF_AXIOM(status == QS::ValidPath);
        TF_AXIOM(results.size() == 2);
        TF_AXIOM(results[0].capability == TfToken("usd.geom.mesh"));
        TF_AXIOM(results[0].status == QS::ValidPath);
        TF_AXIOM(results[1].capability == TfToken("usd"));
        TF_AXIOM(results[1].status == QS::ValidPath);
    }

    // Mixed: valid + deprecated → DeprecationConflict aggregate
    {
        std::vector<TfToken> required = {
            TfToken("usd.geom.nurbs"), TfToken("usd.geom.mesh"),
        };
        std::vector<CR> results;
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("profile.26.08"), required, {}, &results);
        TF_AXIOM(status == QS::DeprecationConflict);
        TF_AXIOM(results[0].capability == TfToken("usd.geom.nurbs"));
        TF_AXIOM(results[0].status == QS::ValidPath);
        TF_AXIOM(results[1].capability == TfToken("usd.geom.mesh"));
        TF_AXIOM(results[1].status == QS::DeprecationConflict);
    }

    // One cap unreachable → NoPath aggregate, results still fully populated
    {
        std::vector<TfToken> required = {
            TfToken("usd.geom.mesh"), TfToken("no.such.cap"),
        };
        std::vector<CR> results;
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("profile.25.11"), required, {}, &results);
        TF_AXIOM(status == QS::NoPath);
        TF_AXIOM(results.size() == 2);
        TF_AXIOM(results[0].status == QS::ValidPath);
        TF_AXIOM(results[1].status == QS::NoPath);
    }

    // Empty required → NoPath
    {
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("profile.25.11"), {});
        TF_AXIOM(status == QS::NoPath);
    }

    // Unknown perspective → NoPath
    {
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("no.such.profile"), {TfToken("usd.geom.mesh")});
        TF_AXIOM(status == QS::NoPath);
    }

    // Deprecated path only → Deprecated aggregate
    {
        std::vector<TfToken> required = {TfToken("profile.25.11")};
        std::vector<CR> results;
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("profile.26.08"), required, {}, &results);
        TF_AXIOM(status == QS::Deprecated);
        TF_AXIOM(results[0].status == QS::Deprecated);
    }

    // Excepted cap: per-result → Excepted, aggregate → NoPath
    {
        std::vector<TfToken> required = {
            TfToken("usd.geom.mesh"), TfToken("usd"),
        };
        std::vector<TfToken> ex = {TfToken("usd.geom.mesh")};
        std::vector<CR> results;
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("profile.25.11"), required, ex, &results);
        TF_AXIOM(status == QS::NoPath);
        TF_AXIOM(results[0].capability == TfToken("usd.geom.mesh"));
        TF_AXIOM(results[0].status == QS::Excepted);
        TF_AXIOM(results[1].capability == TfToken("usd"));
        TF_AXIOM(results[1].status == QS::ValidPath);
    }

    // Excepted cap not in required → no effect
    {
        std::vector<TfToken> required = {TfToken("usd.geom.mesh")};
        std::vector<TfToken> ex = {TfToken("usd.geom.nurbs")};
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("profile.25.11"), required, ex);
        TF_AXIOM(status == QS::ValidPath);
    }

    printf("  TestCoversCapabilities passed\n");
}

//----------------------------------------------------------------------------
// TestDeprecationPerspective
//
// Factored from testUsdProfilesRegistry: deprecation is perspective-aware.
// profile.26.08 sees usd.geom.mesh as DeprecationConflict because it reaches
// it via both the deprecated profile.25.11 edge and the valid nurbs→mesh path.
//----------------------------------------------------------------------------

static void
TestDeprecationPerspective()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(
        "testProfileVersions.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("profile.25.11"), TfToken("usd.geom.mesh")) == QS::ValidPath);
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("profile.26.08"), TfToken("profile.25.11")) == QS::Deprecated);
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("profile.26.08"), TfToken("usd.geom.nurbs")) == QS::ValidPath);
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("profile.26.08"), TfToken("usd.geom.mesh")) == QS::DeprecationConflict);

    printf("  TestDeprecationPerspective passed\n");
}

//----------------------------------------------------------------------------
// TestIsCompatibleWith
//
// End-to-end test of ClaimsAPI::IsCompatibleWith:
//   1. Load a registry (testProfileVersions.json).
//   2. Apply ClaimsAPI to an in-memory prim, record capability usages.
//   3. Declare profile compatibility (with and without exceptions).
//   4. Call IsCompatibleWith and check the QueryStatus and per-cap results.
//----------------------------------------------------------------------------

static void
TestIsCompatibleWith()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(
        "testProfileVersions.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim prim = stage->DefinePrim(SdfPath("/TestPrim"));
    UsdProfilesClaimsAPI api = UsdProfilesClaimsAPI::Apply(prim);
    TF_AXIOM(api);

    // Record capability usages
    api.SetCapabilityUsage(TfToken("usd.geom.mesh"), TfToken("hard"));
    api.SetCapabilityUsage(TfToken("usd"),           TfToken("hard"));

    // Profile not declared → NoPath without consulting the registry
    TF_AXIOM(api.IsCompatibleWith(TfToken("profile.25.11")) == QS::NoPath);

    // Declare fully compatible; profile.25.11 covers both caps validly
    api.SetProfileCompatible(TfToken("profile.25.11"));
    {
        std::vector<CR> results;
        QS status = api.IsCompatibleWith(TfToken("profile.25.11"), &results);
        TF_AXIOM(status == QS::ValidPath);
        TF_AXIOM(results.size() == 2);
        TF_AXIOM(_ResultContains(results, TfToken("usd.geom.mesh")));
        TF_AXIOM(_ResultContains(results, TfToken("usd")));
        TF_AXIOM(_ResultStatus(results, TfToken("usd.geom.mesh")) == QS::ValidPath);
        TF_AXIOM(_ResultStatus(results, TfToken("usd"))           == QS::ValidPath);
    }

    // Declare compatible with profile.26.08 with usd.geom.mesh excepted.
    // profile.26.08 reaches usd.geom.mesh via DeprecationConflict; excepted
    // caps are reported as Excepted and make the aggregate NoPath.
    api.SetProfileCompatibleWithExceptions(
        TfToken("profile.26.08"), VtArray<TfToken>{TfToken("usd.geom.mesh")});
    {
        std::vector<CR> results;
        QS status = api.IsCompatibleWith(TfToken("profile.26.08"), &results);
        TF_AXIOM(status == QS::NoPath);
        TF_AXIOM(_ResultStatus(results, TfToken("usd.geom.mesh")) == QS::Excepted);
    }

    // Prim with no capability usages: IsCompatibleWith returns NoPath
    {
        UsdPrim empty = stage->DefinePrim(SdfPath("/NoCaps"));
        UsdProfilesClaimsAPI api2 = UsdProfilesClaimsAPI::Apply(empty);
        api2.SetProfileCompatible(TfToken("profile.25.11"));
        TF_AXIOM(api2.IsCompatibleWith(TfToken("profile.25.11")) == QS::NoPath);
    }

    printf("  TestIsCompatibleWith passed\n");
}

//----------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------

int
main(int argc, char* argv[])
{
    printf("testUsdProfilesProfiles\n");

    Usd_ProfilesRegistryTestClear();
    TestIsProfile();

    Usd_ProfilesRegistryTestClear();
    TestCoversCapabilities();

    Usd_ProfilesRegistryTestClear();
    TestDeprecationPerspective();

    Usd_ProfilesRegistryTestClear();
    TestIsCompatibleWith();

    Usd_ProfilesRegistryTestClear();
    printf("All tests passed\n");
    return 0;
}
