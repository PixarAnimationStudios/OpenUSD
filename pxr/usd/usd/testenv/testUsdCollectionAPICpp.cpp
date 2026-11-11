//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/collectionMembershipQuery.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include <cstdio>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// Tests of UsdCollectionAPI behavior that the Python bindings cannot observe.
// See testUsdCollectionAPI.py for most collection test cases.

// Records the warnings issued while it is alive.
class _WarningRecorder : public TfDiagnosticMgr::Delegate
{
public:
    _WarningRecorder() {
        TfDiagnosticMgr::GetInstance().AddDelegate(this);
    }
    ~_WarningRecorder() override {
        TfDiagnosticMgr::GetInstance().RemoveDelegate(this);
    }

    void IssueError(TfError const &) override {}
    void IssueFatalError(TfCallContext const &, std::string const &) override {}
    void IssueStatus(TfStatus const &) override {}
    void IssueWarning(TfWarning const &warning) override {
        _warnings.push_back(warning.GetCommentary());
    }

    // Return the number of warnings about referencing a relationship-mode
    // collection.
    size_t CountRelationshipModeReferenceWarnings() const {
        size_t count = 0;
        for (std::string const &warning : _warnings) {
            if (TfStringContains(warning, "is in relationship-mode")) {
                ++count;
            }
        }
        return count;
    }

private:
    std::vector<std::string> _warnings;
};

static SdfPathVector
_ComputeIncludedPaths(UsdCollectionAPI const &collection)
{
    const SdfPathSet paths = UsdComputeIncludedPathsFromCollection(
        collection.ComputeMembershipQuery(), collection.GetPrim().GetStage());
    return SdfPathVector(paths.begin(), paths.end());
}

static void
TestReferenceToRelationshipModeCollectionWarns()
{
    printf("TestReferenceToRelationshipModeCollectionWarns...\n");

    const UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->DefinePrim(SdfPath("/World/A"));
    stage->DefinePrim(SdfPath("/World/B"));
    const UsdPrim sets = stage->DefinePrim(SdfPath("/Sets"));

    // Relationship-mode, inferred from the includes.
    const UsdCollectionAPI rel = UsdCollectionAPI::Apply(sets, TfToken("rel"));
    TF_AXIOM(rel.CreateIncludesRel().AddTarget(SdfPath("/World/A")));

    // Explicit relationship-mode, with no rules.
    const UsdCollectionAPI emptyRel =
        UsdCollectionAPI::Apply(sets, TfToken("emptyRel"));
    TF_AXIOM(emptyRel.CreateModeAttr(VtValue(UsdTokens->relationship)));

    // Explicit expression-mode, with relationships that are ignored.
    const UsdCollectionAPI expr =
        UsdCollectionAPI::Apply(sets, TfToken("expr"));
    TF_AXIOM(expr.CreateModeAttr(VtValue(UsdTokens->expression)));
    TF_AXIOM(expr.CreateIncludesRel().AddTarget(SdfPath("/World/A")));
    TF_AXIOM(expr.CreateMembershipExpressionAttr().Set(
                 SdfPathExpression("/World/B")));

    const UsdCollectionAPI probe =
        UsdCollectionAPI::Apply(sets, TfToken("probe"));

    // A reference to each relationship-mode collection warns and matches
    // nothing.
    for (const char *refName : { "rel", "emptyRel" }) {
        TF_AXIOM(probe.CreateMembershipExpressionAttr().Set(
                     SdfPathExpression(
                         TfStringPrintf("/World/B %%:%s", refName))));
        _WarningRecorder recorder;
        TF_AXIOM(_ComputeIncludedPaths(probe) ==
                 SdfPathVector { SdfPath("/World/B") });
        TF_AXIOM(recorder.CountRelationshipModeReferenceWarnings() == 1);
    }

    // A reference to an expression-mode collection resolves without warning.
    {
        TF_AXIOM(probe.CreateMembershipExpressionAttr().Set(
                     SdfPathExpression("%:expr")));
        _WarningRecorder recorder;
        TF_AXIOM(_ComputeIncludedPaths(probe) ==
                 SdfPathVector { SdfPath("/World/B") });
        TF_AXIOM(recorder.CountRelationshipModeReferenceWarnings() == 0);
    }
}

static void
TestIgnoredExpressionDoesNotWarn()
{
    printf("TestIgnoredExpressionDoesNotWarn...\n");

    const UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->DefinePrim(SdfPath("/World/A"));
    const UsdPrim sets = stage->DefinePrim(SdfPath("/Sets"));

    const UsdCollectionAPI rel = UsdCollectionAPI::Apply(sets, TfToken("rel"));
    TF_AXIOM(rel.CreateIncludesRel().AddTarget(SdfPath("/World/A")));

    // Relationship-mode collections whose membershipExpressions, which are
    // ignored, reference a relationship-mode collection.
    const UsdCollectionAPI autoRel =
        UsdCollectionAPI::Apply(sets, TfToken("autoRel"));
    TF_AXIOM(autoRel.CreateIncludesRel().AddTarget(SdfPath("/World/A")));
    TF_AXIOM(autoRel.CreateMembershipExpressionAttr().Set(
                 SdfPathExpression("%:rel")));

    const UsdCollectionAPI explicitRel =
        UsdCollectionAPI::Apply(sets, TfToken("explicitRel"));
    TF_AXIOM(explicitRel.CreateModeAttr(VtValue(UsdTokens->relationship)));
    TF_AXIOM(explicitRel.CreateMembershipExpressionAttr().Set(
                 SdfPathExpression("%:rel")));

    _WarningRecorder recorder;
    TF_AXIOM(_ComputeIncludedPaths(autoRel) ==
             SdfPathVector { SdfPath("/World/A") });
    TF_AXIOM(_ComputeIncludedPaths(explicitRel).empty());
    std::string reason;
    TF_AXIOM(autoRel.Validate(&reason));
    TF_AXIOM(explicitRel.Validate(&reason));
    TF_AXIOM(recorder.CountRelationshipModeReferenceWarnings() == 0);
}

int main()
{
    TestReferenceToRelationshipModeCollectionWarns();
    TestIgnoredExpressionDoesNotWarn();

    printf("\n\n>>> Test SUCCEEDED\n");
    return 0;
}
