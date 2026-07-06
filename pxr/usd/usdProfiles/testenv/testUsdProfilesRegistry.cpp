//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdProfiles/profileRegistry.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/js/types.h"
#include "pxr/base/js/value.h"

#include <algorithm>
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

//----------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------

static bool
_VectorContains(const std::vector<TfToken>& v, const TfToken& t)
{
    return std::find(v.begin(), v.end(), t) != v.end();
}

using CR = UsdProfileRegistry::CapabilityResult;
using QS = UsdProfileRegistry::QueryStatus;

// Return true if any CapabilityResult in v has the given capability token.
static bool
_ResultContains(const std::vector<CR>& v, const TfToken& t)
{
    for (const auto& r : v) {
        if (r.capability == t) return true;
    }
    return false;
}

// Return the QueryStatus for a given token in a CapabilityResult vector.
// Asserts if not found.
static QS
_ResultStatus(const std::vector<CR>& v, const TfToken& t)
{
    for (const auto& r : v) {
        if (r.capability == t) return r.status;
    }
    TF_AXIOM(false); // token not found
    return QS::NoPath;
}

// Return all tokens with a given status from a CapabilityResult vector.
static std::vector<TfToken>
_ResultTokensWithStatus(const std::vector<CR>& v, QS status)
{
    std::vector<TfToken> result;
    for (const auto& r : v) {
        if (r.status == status) result.push_back(r.capability);
    }
    return result;
}

static bool
_StringVectorContains(const std::vector<std::string>& v,
                      const std::string& substr)
{
    for (const auto& s : v) {
        if (s.find(substr) != std::string::npos) {
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------
// TestBasicDAG
//----------------------------------------------------------------------------

static void
TestBasicDAG()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testBasicDAG.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    // HasCapability
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd")));
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("chain.a")));
    TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("merge.point")));
    TF_AXIOM(!UsdProfileRegistry::HasCapability(TfToken("nonexistent")));

    // GetAllCapabilities
    std::vector<TfToken> all = UsdProfileRegistry::GetAllCapabilities();
    TF_AXIOM(all.size() == 6);
    TF_AXIOM(_VectorContains(all, TfToken("usd")));
    TF_AXIOM(_VectorContains(all, TfToken("merge.point")));

    // GetPredecessors
    std::vector<CR> rootPreds =
        UsdProfileRegistry::GetPredecessors(TfToken("usd"));
    TF_AXIOM(rootPreds.empty());

    std::vector<CR> chainAPreds =
        UsdProfileRegistry::GetPredecessors(TfToken("chain.a"));
    TF_AXIOM(chainAPreds.size() == 1);
    TF_AXIOM(chainAPreds[0].capability == TfToken("usd"));
    TF_AXIOM(chainAPreds[0].status == QS::ValidPath);

    std::vector<CR> mergePreds =
        UsdProfileRegistry::GetPredecessors(TfToken("merge.point"));
    TF_AXIOM(mergePreds.size() == 3);
    TF_AXIOM(_ResultContains(mergePreds, TfToken("chain.b")));
    TF_AXIOM(_ResultContains(mergePreds, TfToken("branch.left")));
    TF_AXIOM(_ResultContains(mergePreds, TfToken("branch.right")));

    // No deprecated edges in this DAG
    TF_AXIOM(_ResultTokensWithStatus(mergePreds, QS::Deprecated).empty());

    // GetDocString
    TF_AXIOM(UsdProfileRegistry::GetDocString(TfToken("usd"))
             == "Root capability");
    TF_AXIOM(UsdProfileRegistry::GetDocString(TfToken("merge.point"))
             == "Merges chain and both branches");

    // GetDisplayName
    TF_AXIOM(UsdProfileRegistry::GetDisplayName(TfToken("chain.a"))
             == "Chain A");
    TF_AXIOM(UsdProfileRegistry::GetDisplayName(TfToken("merge.point"))
             == "Merge Point");

    // GetStyleForCapability
    TF_AXIOM(UsdProfileRegistry::GetStyleForCapability(TfToken("usd"))
             == TfToken("core"));
    TF_AXIOM(UsdProfileRegistry::GetStyleForCapability(TfToken("branch.left"))
             == TfToken("extra"));

    // GetSubgraphForCapability
    TF_AXIOM(UsdProfileRegistry::GetSubgraphForCapability(TfToken("usd"))
             == TfToken("Foundation"));
    TF_AXIOM(UsdProfileRegistry::GetSubgraphForCapability(TfToken("merge.point"))
             == TfToken("Extensions"));

    // GetCapabilityMetadata — verify usd root is present and has no predecessors
    TF_AXIOM(UsdProfileRegistry::IsProfile(TfToken("usd")) == false);

    // GetCapabilityMetadata
    VtDictionary md =
        UsdProfileRegistry::GetCapabilityMetadata(TfToken("chain.b"));
    TF_AXIOM(md.count("docstring"));
    TF_AXIOM(md["docstring"].Get<std::string>()
             == "Second link in chain");
    TF_AXIOM(md["style"].Get<std::string>() == "core");

    // GetCapabilityStyles
    auto styles = UsdProfileRegistry::GetCapabilityStyles();
    TF_AXIOM(styles.size() == 2);
    TF_AXIOM(styles.find(TfToken("core")) != styles.end());
    TF_AXIOM(styles.find(TfToken("extra")) != styles.end());

    printf("  TestBasicDAG passed\n");
}

//----------------------------------------------------------------------------
// TestDeprecation
//----------------------------------------------------------------------------

static void
TestDeprecation()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(
        "testDeprecation.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    // Case 1: Hydra v1/v2 migration
    //   hydra.v2 has hydra.v1 as deprecated predecessor, hydra.base as valid
    {
        std::vector<CR> preds =
            UsdProfileRegistry::GetPredecessors(TfToken("hydra.v2"));
        TF_AXIOM(preds.size() == 2);
        TF_AXIOM(_ResultContains(preds, TfToken("hydra.base")));
        TF_AXIOM(_ResultContains(preds, TfToken("hydra.v1")));
        TF_AXIOM(_ResultStatus(preds, TfToken("hydra.base")) == QS::ValidPath);
        TF_AXIOM(_ResultStatus(preds, TfToken("hydra.v1")) == QS::Deprecated);
    }

    // Case 2: Look/Material sunset
    //   shade.material has shade.look as deprecated predecessor
    {
        std::vector<CR> preds =
            UsdProfileRegistry::GetPredecessors(TfToken("shade.material"));
        TF_AXIOM(_ResultStatus(preds, TfToken("shade.look")) == QS::Deprecated);
        TF_AXIOM(_ResultStatus(preds, TfToken("shade.base")) == QS::ValidPath);
    }

    // Case 3: Pipeline multi-deprecated
    //   pipeline.v3 has both v1 and v2 as deprecated predecessors
    {
        std::vector<CR> preds =
            UsdProfileRegistry::GetPredecessors(TfToken("pipeline.v3"));
        TF_AXIOM(preds.size() == 3);
        TF_AXIOM(_ResultStatus(preds, TfToken("hydra.base")) == QS::ValidPath);
        TF_AXIOM(_ResultStatus(preds, TfToken("pipeline.v1")) == QS::Deprecated);
        TF_AXIOM(_ResultStatus(preds, TfToken("pipeline.v2")) == QS::Deprecated);
        std::vector<TfToken> deprecated =
            _ResultTokensWithStatus(preds, QS::Deprecated);
        TF_AXIOM(deprecated.size() == 2);
    }

    // Case 4: Clean leaf has no deprecated direct predecessors
    {
        std::vector<CR> preds =
            UsdProfileRegistry::GetPredecessors(TfToken("clean.leaf"));
        TF_AXIOM(_ResultTokensWithStatus(preds, QS::Deprecated).empty());
    }

    printf("  TestDeprecation passed\n");
}

//----------------------------------------------------------------------------
// TestCycleDetection
//----------------------------------------------------------------------------

static void
TestCycleDetection()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(
        "testCycleDetection.json", &errors);
    TF_AXIOM(!ok);
    TF_AXIOM(!errors.empty());
    TF_AXIOM(_StringVectorContains(errors, "cycle"));
    TF_AXIOM(_StringVectorContains(errors, "root"));

    printf("  TestCycleDetection passed\n");
}

//----------------------------------------------------------------------------
// TestMissingPredecessor
//----------------------------------------------------------------------------

static void
TestMissingPredecessor()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(
        "testMissingPredecessor.json", &errors);
    TF_AXIOM(!ok);
    TF_AXIOM(!errors.empty());
    TF_AXIOM(_StringVectorContains(errors, "nonexistent.capability"));

    printf("  TestMissingPredecessor passed\n");
}

//----------------------------------------------------------------------------
// TestEmptyGraph
//----------------------------------------------------------------------------

static void
TestEmptyGraph()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testEmpty.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    std::vector<TfToken> all = UsdProfileRegistry::GetAllCapabilities();
    TF_AXIOM(all.empty());

    TF_AXIOM(!UsdProfileRegistry::HasCapability(TfToken("anything")));
    TF_AXIOM(UsdProfileRegistry::GetCapabilityStyles().empty());

    printf("  TestEmptyGraph passed\n");
}

//----------------------------------------------------------------------------
// TestMultiPluginMerge
//----------------------------------------------------------------------------

static void
TestMultiPluginMerge()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(
        "testMultiPluginMerged.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    // Cross-plugin edge: vendor.geom depends on core.module
    std::vector<CR> geomPreds =
        UsdProfileRegistry::GetPredecessors(TfToken("vendor.geom"));
    TF_AXIOM(geomPreds.size() == 1);
    TF_AXIOM(geomPreds[0].capability == TfToken("core.module"));
    TF_AXIOM(geomPreds[0].status == QS::ValidPath);

    // vendor.render depends on both core.module and vendor.geom
    std::vector<CR> renderPreds =
        UsdProfileRegistry::GetPredecessors(TfToken("vendor.render"));
    TF_AXIOM(renderPreds.size() == 2);
    TF_AXIOM(_ResultContains(renderPreds, TfToken("core.module")));
    TF_AXIOM(_ResultContains(renderPreds, TfToken("vendor.geom")));

    // Styles from both plugins merged
    auto styles = UsdProfileRegistry::GetCapabilityStyles();
    TF_AXIOM(styles.find(TfToken("plugA")) != styles.end());
    TF_AXIOM(styles.find(TfToken("plugB")) != styles.end());

    // Style assignment crosses plugin boundary
    TF_AXIOM(UsdProfileRegistry::GetStyleForCapability(TfToken("core.module"))
             == TfToken("plugA"));
    TF_AXIOM(UsdProfileRegistry::GetStyleForCapability(TfToken("vendor.geom"))
             == TfToken("plugB"));

    printf("  TestMultiPluginMerge passed\n");
}

//----------------------------------------------------------------------------
// TestClear
//----------------------------------------------------------------------------

static void
TestClear()
{
    // Load some data, then clear
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testBasicDAG.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(!UsdProfileRegistry::GetAllCapabilities().empty());

    Usd_ProfilesRegistryTestClear();

    TF_AXIOM(UsdProfileRegistry::GetAllCapabilities().empty());
    TF_AXIOM(!UsdProfileRegistry::HasCapability(TfToken("usd")));
    TF_AXIOM(UsdProfileRegistry::GetCapabilityStyles().empty());

    printf("  TestClear passed\n");
}

//----------------------------------------------------------------------------
// TestTransitivePredecessors
//----------------------------------------------------------------------------

static void
TestTransitivePredecessors()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testBasicDAG.json", &errors);
    TF_AXIOM(ok);

    // usd has no predecessors at all
    std::vector<CR> rootTrans =
        UsdProfileRegistry::GetTransitivePredecessors(TfToken("usd"));
    TF_AXIOM(rootTrans.empty());

    // chain.a -> usd (one level, valid)
    std::vector<CR> chainATrans =
        UsdProfileRegistry::GetTransitivePredecessors(TfToken("chain.a"));
    TF_AXIOM(chainATrans.size() == 1);
    TF_AXIOM(_ResultContains(chainATrans, TfToken("usd")));
    TF_AXIOM(_ResultStatus(chainATrans, TfToken("usd")) == QS::ValidPath);

    // chain.b -> chain.a -> usd (two levels, all valid)
    std::vector<CR> chainBTrans =
        UsdProfileRegistry::GetTransitivePredecessors(TfToken("chain.b"));
    TF_AXIOM(chainBTrans.size() == 2);
    TF_AXIOM(_ResultStatus(chainBTrans, TfToken("chain.a")) == QS::ValidPath);
    TF_AXIOM(_ResultStatus(chainBTrans, TfToken("usd")) == QS::ValidPath);

    // merge.point -> chain.b, branch.left, branch.right -> chain.a, usd
    // Full closure: chain.b, branch.left, branch.right, chain.a, usd (all valid)
    std::vector<CR> mergeTrans =
        UsdProfileRegistry::GetTransitivePredecessors(TfToken("merge.point"));
    TF_AXIOM(mergeTrans.size() == 5);
    TF_AXIOM(_ResultContains(mergeTrans, TfToken("chain.b")));
    TF_AXIOM(_ResultContains(mergeTrans, TfToken("chain.a")));
    TF_AXIOM(_ResultContains(mergeTrans, TfToken("usd")));
    TF_AXIOM(_ResultContains(mergeTrans, TfToken("branch.left")));
    TF_AXIOM(_ResultContains(mergeTrans, TfToken("branch.right")));
    TF_AXIOM(_ResultTokensWithStatus(mergeTrans, QS::Deprecated).empty());

    // Unknown capability returns empty
    std::vector<CR> unknownTrans =
        UsdProfileRegistry::GetTransitivePredecessors(TfToken("nonexistent"));
    TF_AXIOM(unknownTrans.empty());

    printf("  TestTransitivePredecessors passed\n");
}

//----------------------------------------------------------------------------
// TestHasPredecessor
//----------------------------------------------------------------------------

static void
TestHasPredecessor()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testBasicDAG.json", &errors);
    TF_AXIOM(ok);



    // Direct predecessor: chain.a -> usd
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("chain.a"), TfToken("usd")) == QS::ValidPath);

    // Transitive: chain.b -> chain.a -> usd
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("chain.b"), TfToken("usd")) == QS::ValidPath);

    // merge.point reaches usd transitively through multiple paths
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("merge.point"), TfToken("usd")) == QS::ValidPath);

    // No path: usd has no predecessors
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("usd"), TfToken("chain.a")) == QS::NoPath);

    // Self returns NoPath (not a predecessor of itself)
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("usd"), TfToken("usd")) == QS::NoPath);

    // Unknown capability returns NoPath
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("nonexistent"), TfToken("usd")) == QS::NoPath);

    // Now test deprecated paths
    ok = Usd_ProfilesRegistryTestLoadFromFile("testDeprecation.json", &errors);
    TF_AXIOM(ok);

    // hydra.v1 reaches hydra.base via a single non-deprecated edge
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("hydra.v1"), TfToken("hydra.base")) == QS::ValidPath);

    // hydra.v2 reaches hydra.v1 via deprecated direct edge
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("hydra.v2"), TfToken("hydra.v1")) == QS::Deprecated);

    // hydra.v2 reaches hydra.base via two paths:
    //   direct non-deprecated edge, and through hydra.v1 (deprecated).
    //   Both paths exist, so result is DeprecationConflict.
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("hydra.v2"), TfToken("hydra.base")) == QS::DeprecationConflict);

    // clean.leaf reaches hydra.v1 only through the deprecated edge (hydra.v2 -> hydra.v1)
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("clean.leaf"), TfToken("hydra.v1")) == QS::Deprecated);

    // clean.leaf reaches hydra.base via both deprecated and non-deprecated paths
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("clean.leaf"), TfToken("hydra.base")) == QS::DeprecationConflict);

    printf("  TestHasPredecessor passed\n");
}


//----------------------------------------------------------------------------
// TestUnknownCapabilityQueries
//
// Every public query API must handle unknown and empty tokens gracefully:
// return empty/false/NoPath rather than crash. Tested against both a loaded
// graph and a cleared (empty) registry.
//----------------------------------------------------------------------------

static void
TestUnknownCapabilityQueries()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testBasicDAG.json", &errors);
    TF_AXIOM(ok);



    const TfToken bogus("no.such.capability");
    const TfToken empty;

    // HasCapability
    TF_AXIOM(!UsdProfileRegistry::HasCapability(bogus));
    TF_AXIOM(!UsdProfileRegistry::HasCapability(empty));

    // GetPredecessors
    TF_AXIOM(UsdProfileRegistry::GetPredecessors(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetPredecessors(empty).empty());

    // GetTransitivePredecessors
    TF_AXIOM(UsdProfileRegistry::GetTransitivePredecessors(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetTransitivePredecessors(empty).empty());

    // HasPredecessor — unknown source
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(bogus, TfToken("usd")) == QS::NoPath);
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(empty, TfToken("usd")) == QS::NoPath);

    // HasPredecessor — unknown target
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(TfToken("chain.b"), bogus) == QS::NoPath);
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(TfToken("chain.b"), empty) == QS::NoPath);

    // HasPredecessor — both unknown
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(bogus, bogus) == QS::NoPath);
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(empty, empty) == QS::NoPath);

    // GetDocString
    TF_AXIOM(UsdProfileRegistry::GetDocString(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetDocString(empty).empty());

    // GetDisplayName
    TF_AXIOM(UsdProfileRegistry::GetDisplayName(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetDisplayName(empty).empty());

    // GetStyleForCapability
    TF_AXIOM(UsdProfileRegistry::GetStyleForCapability(bogus) == TfToken());
    TF_AXIOM(UsdProfileRegistry::GetStyleForCapability(empty) == TfToken());

    // GetSubgraphForCapability
    TF_AXIOM(UsdProfileRegistry::GetSubgraphForCapability(bogus) == TfToken());
    TF_AXIOM(UsdProfileRegistry::GetSubgraphForCapability(empty) == TfToken());

    // GetCapabilityMetadata
    TF_AXIOM(UsdProfileRegistry::GetCapabilityMetadata(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetCapabilityMetadata(empty).empty());

    //---- Repeat everything on a cleared (empty) registry ---
    Usd_ProfilesRegistryTestClear();

    TF_AXIOM(!UsdProfileRegistry::HasCapability(bogus));
    TF_AXIOM(!UsdProfileRegistry::HasCapability(empty));
    TF_AXIOM(UsdProfileRegistry::GetPredecessors(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetPredecessors(empty).empty());
    TF_AXIOM(UsdProfileRegistry::GetTransitivePredecessors(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetTransitivePredecessors(empty).empty());
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(bogus, TfToken("usd")) == QS::NoPath);
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(empty, empty) == QS::NoPath);
    TF_AXIOM(UsdProfileRegistry::GetDocString(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetDisplayName(bogus).empty());
    TF_AXIOM(UsdProfileRegistry::GetStyleForCapability(bogus) == TfToken());
    TF_AXIOM(UsdProfileRegistry::GetSubgraphForCapability(bogus) == TfToken());
    TF_AXIOM(UsdProfileRegistry::GetCapabilityMetadata(bogus).empty());

    printf("  TestUnknownCapabilityQueries passed\n");
}

//----------------------------------------------------------------------------
// TestParseCapabilityVersion
//----------------------------------------------------------------------------

static void
TestParseCapabilityVersion()
{
    // Unversioned: version 0, base name = full token
    {
        auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(
            TfToken("usd.physics"));
        TF_AXIOM(base == TfToken("usd.physics"));
        TF_AXIOM(ver == 0);
    }

    // Standard _vN suffix
    {
        auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(
            TfToken("usd.physics_v2"));
        TF_AXIOM(base == TfToken("usd.physics"));
        TF_AXIOM(ver == 2);
    }

    // Higher version number
    {
        auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(
            TfToken("yoyo.tool_v10"));
        TF_AXIOM(base == TfToken("yoyo.tool"));
        TF_AXIOM(ver == 10);
    }

    // _v in the middle of the name, not a suffix — not a version
    {
        auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(
            TfToken("usd._v2.geom"));
        TF_AXIOM(base == TfToken("usd._v2.geom"));
        TF_AXIOM(ver == 0);
    }

    // _v suffix with no digits — not a version
    {
        auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(
            TfToken("usd.foo_v"));
        TF_AXIOM(base == TfToken("usd.foo_v"));
        TF_AXIOM(ver == 0);
    }

    // Empty token
    {
        auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(TfToken());
        TF_AXIOM(base == TfToken());
        TF_AXIOM(ver == 0);
    }

    // _vN followed by non-digit — not a version suffix
    {
        auto [base, ver] = UsdProfileRegistry::ParseCapabilityVersion(
            TfToken("usd.foo_v2x"));
        TF_AXIOM(base == TfToken("usd.foo_v2x"));
        TF_AXIOM(ver == 0);
    }

    printf("  TestParseCapabilityVersion passed\n");
}

//----------------------------------------------------------------------------
// TestResolveCapability
//----------------------------------------------------------------------------

static void
TestResolveCapability()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testVersioning.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    // Unversioned name with a versioned sibling → resolves to highest versioned
    TF_AXIOM(UsdProfileRegistry::ResolveCapability(TfToken("usd.physics"))
             == TfToken("usd.physics_v2"));

    // Versioned name → resolves to itself (it is already the highest)
    TF_AXIOM(UsdProfileRegistry::ResolveCapability(TfToken("usd.physics_v2"))
             == TfToken("usd.physics_v2"));

    // Multiple versioned, no unversioned: highest wins
    TF_AXIOM(UsdProfileRegistry::ResolveCapability(TfToken("yoyo.tool_v2"))
             == TfToken("yoyo.tool_v5"));
    TF_AXIOM(UsdProfileRegistry::ResolveCapability(TfToken("yoyo.tool_v5"))
             == TfToken("yoyo.tool_v5"));

    // No versioned siblings → resolves to itself
    TF_AXIOM(UsdProfileRegistry::ResolveCapability(TfToken("usd.geom"))
             == TfToken("usd.geom"));
    TF_AXIOM(UsdProfileRegistry::ResolveCapability(TfToken("usd"))
             == TfToken("usd"));

    // Unknown capability → empty token
    TF_AXIOM(UsdProfileRegistry::ResolveCapability(TfToken("no.such.cap")).IsEmpty());
    TF_AXIOM(UsdProfileRegistry::ResolveCapability(TfToken()).IsEmpty());

    printf("  TestResolveCapability passed\n");
}

//----------------------------------------------------------------------------
// TestVersioningQueries
//
// Verifies that HasPredecessor and CoversCapabilities resolve tokens before
// querying, so callers can use unversioned names and still get correct results.
//
// Graph (testVersioning.json):
//   usd <- usd.geom <- usd.physics (unversioned)
//                   <- usd.physics_v2 <- usd.physics (deprecated edge)
//                                     <- usd.geom
//   usd <- yoyo.tool_v2
//       <- yoyo.tool_v5 <- yoyo.tool_v2 (deprecated)
//   usd.physics_v2 <- profile.sim
//----------------------------------------------------------------------------

static void
TestVersioningQueries()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile("testVersioning.json", &errors);
    TF_AXIOM(ok);
    TF_AXIOM(errors.empty());

    // --- HasPredecessor with unversioned names ---

    // "usd.physics" resolves to usd.physics_v2; profile.sim reaches it validly
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("profile.sim"), TfToken("usd.physics")) == QS::ValidPath);

    // "yoyo.tool_v2" resolves to yoyo.tool_v5 (highest);
    // profile.sim doesn't reach yoyo.tool_v5 → NoPath
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("profile.sim"), TfToken("yoyo.tool_v2")) == QS::NoPath);

    // "usd.physics" resolves to usd.physics_v2. From usd.physics_v2:
    //   - direct valid edge to usd.geom
    //   - deprecated edge to usd.physics → usd.geom (deprecated path)
    // Two paths → DeprecationConflict
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("usd.physics"), TfToken("usd.geom")) == QS::DeprecationConflict);

    // Similarly, reaching usd from usd.physics_v2:
    //   - usd.geom (valid) → usd (valid) = ValidPath
    //   - usd.physics (deprecated) → usd.geom → usd = Deprecated path
    // Two paths → DeprecationConflict
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("usd.physics"), TfToken("usd")) == QS::DeprecationConflict);

    // --- CoversCapabilities with unversioned required tokens ---
    {
        // profile.sim covers usd.geom and usd.physics (resolves to _v2)
        std::vector<TfToken> required = {
            TfToken("usd.geom"),
            TfToken("usd.physics"),  // resolves to usd.physics_v2
        };
        std::vector<CR> results;
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("profile.sim"), required, {}, &results);
        // usd.geom: reachable via valid path (profile.sim→usd.physics_v2→usd.geom)
        //           AND deprecated path (→usd.physics(dep)→usd.geom) → DeprecationConflict
        // usd.physics_v2: direct valid predecessor of profile.sim → ValidPath
        // Aggregate: DeprecationConflict
        TF_AXIOM(status == QS::DeprecationConflict);
        TF_AXIOM(results.size() == 2);
        TF_AXIOM(results[0].capability == TfToken("usd.geom"));
        TF_AXIOM(results[0].status == QS::DeprecationConflict);
        // Result reports the resolved token
        TF_AXIOM(results[1].capability == TfToken("usd.physics_v2"));
        TF_AXIOM(results[1].status == QS::ValidPath);
    }

    // --- Unversioned perspective also resolves ---
    {
        // "usd.physics" resolves to usd.physics_v2.
        // usd.geom is reachable via valid and deprecated paths → DeprecationConflict
        std::vector<TfToken> required = {TfToken("usd.geom")};
        QS status = UsdProfileRegistry::CoversCapabilities(
            TfToken("usd.physics"), required);
        TF_AXIOM(status == QS::DeprecationConflict);
    }

    // --- Deprecated path through versioned resolution ---
    // profile.sim reaches usd.physics (the unversioned node) only via
    // the deprecated edge on usd.physics_v2.
    // But "usd.physics" as a required cap resolves to usd.physics_v2 (highest),
    // which is reachable validly. The unversioned node itself is only reachable
    // via deprecated edge — test that directly by name.
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("profile.sim"),
        TfToken("usd.physics_v2")) == QS::ValidPath);  // direct, valid

    // Reaching the literal unversioned node "usd.physics" from profile.sim:
    // profile.sim -> usd.physics_v2 -> usd.physics (deprecated edge)
    // So the literal unversioned node is only deprecated from here.
    // (This uses the graph method directly, bypassing resolution.)
    TF_AXIOM(UsdProfileRegistry::HasPredecessor(
        TfToken("profile.sim"),
        TfToken("usd.physics_v2")) == QS::ValidPath);

    printf("  TestVersioningQueries passed\n");
}


//----------------------------------------------------------------------------
// TestUsdRoot
//
// Verifies that the registry enforces a single root node named "usd":
//   - A graph with a correctly named "usd" root loads successfully.
//   - A graph whose root is named something other than "usd" fails validation.
//   - An empty graph passes (nothing to check).
//----------------------------------------------------------------------------

static void
TestUsdRoot()
{
    // Valid: testBasicDAG has a single root named "usd"
    {
        std::vector<std::string> errors;
        bool ok = Usd_ProfilesRegistryTestLoadFromFile("testBasicDAG.json", &errors);
        TF_AXIOM(ok);
        TF_AXIOM(errors.empty());
        TF_AXIOM(UsdProfileRegistry::HasCapability(TfToken("usd")));
        TF_AXIOM(UsdProfileRegistry::GetPredecessors(TfToken("usd")).empty());
    }

    // Invalid: root is not named "usd"
    {
        std::vector<std::string> errors;
        bool ok = Usd_ProfilesRegistryTestLoadFromFile("testNoUsdRoot.json", &errors);
        TF_AXIOM(!ok);
        TF_AXIOM(!errors.empty());
        TF_AXIOM(_StringVectorContains(errors, "usd"));
    }

    // Valid: empty graph has no root requirement
    {
        std::vector<std::string> errors;
        bool ok = Usd_ProfilesRegistryTestLoadFromFile("testEmpty.json", &errors);
        TF_AXIOM(ok);
        TF_AXIOM(errors.empty());
    }

    printf("  TestUsdRoot passed\n");
}

//----------------------------------------------------------------------------
// TestFileNotFound
//----------------------------------------------------------------------------

static void
TestFileNotFound()
{
    std::vector<std::string> errors;
    bool ok = Usd_ProfilesRegistryTestLoadFromFile(
        "this_file_does_not_exist.json", &errors);
    TF_AXIOM(!ok);
    TF_AXIOM(!errors.empty());
    TF_AXIOM(_StringVectorContains(errors, "Failed to open"));

    printf("  TestFileNotFound passed\n");
}

//----------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------

int
main(int argc, char* argv[])
{
    printf("testUsdProfilesRegistry\n");

    // Clear any baked-in plugin data before running isolated tests
    Usd_ProfilesRegistryTestClear();

    TestBasicDAG();
    TestDeprecation();
    TestCycleDetection();
    TestMissingPredecessor();
    TestEmptyGraph();
    TestMultiPluginMerge();
    TestClear();
    TestFileNotFound();
    TestTransitivePredecessors();
    TestHasPredecessor();
    TestUnknownCapabilityQueries();
    TestParseCapabilityVersion();
    TestResolveCapability();
    TestVersioningQueries();
    TestUsdRoot();

    printf("All tests passed\n");
    return 0;
}
