//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/collectionPredicateLibrary.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/collectionPredicateLibrary.h"
#include "pxr/imaging/hd/instancingAwareCollectionExpressionEvaluator.h"
#include "pxr/imaging/hd/instanceProxyViewSceneIndex.h"

#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/usd/collectionMembershipQuery.h"
#include "pxr/usd/usd/primFlags.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <ostream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

void
_PrintPaths(
    std::ostream &output,
    const std::string &label,
    const SdfPathSet &paths,
    bool printCount = false)
{
    output << label
           << (printCount? " (count = " + std::to_string(paths.size()) + ")"
                         : "")
           << ":\n";
    for (const SdfPath &primPath : paths) {
        output << "\t" << primPath << "\n";
    }
    output << "\n";
}

SdfPathSet
_ComputeAllHydraMatches(
    const HdInstancingAwareCollectionExpressionEvaluator::MatchResult &result)
{
    SdfPathSet allMatches;
    // Remove any internal paths (e.g. /UsdNiPropagatedPrototypes) from
    // the non-instance matches so we can compare results from Hydra evaluation
    // with USD.
    //
    std::copy_if(
        result.nonInstanceMatches.begin(),
        result.nonInstanceMatches.end(),
        std::inserter(allMatches, allMatches.end()),
        [](const SdfPath &path) {
            return !path.HasPrefix(
                SdfPath("/UsdNiPropagatedPrototypes"));
        });

    // Add fully matched instance prim paths and instance proxy matches.
    // (Skip partially matched instance prim paths since they don't
    // fully match; they're in service of de-instancing.)
    allMatches.insert(result.fullyMatchedInstances.begin(),
                        result.fullyMatchedInstances.end());
    allMatches.insert(result.matchedInstanceProxyPaths.begin(),
                        result.matchedInstanceProxyPaths.end());
    return allMatches;
}

SdfPathSet
_ComputeAllUsdMatches(
    const UsdStageRefPtr &stage,
    const SdfPathExpression &expr)
{
    // Since the expressions aren't authored using UsdCollectionAPI, use the
    // lower-level API to do so.
    //
    TfErrorMark mark;
    UsdObjectCollectionExpressionEvaluator exprEval(stage, expr);
    const bool hasUnsupportedPredicates = exprEval.IsEmpty();
    mark.Clear();

    if (hasUnsupportedPredicates) {
        std::cerr << "Expression \"" << expr.GetText()
                  << "\" has unsupported predicates for USD evaluation.\n";
        return {};
    }

    // Construct a query object for use in the
    // UsdComputeIncludedPathsFromCollection() API below.
    // (default c'tor object doesn't seem to suffice...)
    UsdCollectionMembershipQuery query(
        UsdCollectionMembershipQuery::PathExpansionRuleMap{},
        SdfPathSet{}, /*includedCollections*/
        UsdTokens->expandPrims); /*expansionRule*/
    query.SetExpressionEvaluator(std::move(exprEval));

    // Note: The traversal predicate dictates what is actually matched...
    const SdfPathSet matchedPaths =
        UsdComputeIncludedPathsFromCollection(
            query, stage, UsdTraverseInstanceProxies());

    return matchedPaths;
}

}

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cerr << "Usage: testUsdImagingCollectionExpressionEvaluation"
                     " <file.usd> <expr1> [expr2 ...]\n";
        return -1;
    }

    UsdStageRefPtr stage = UsdStage::Open(argv[1]);
    if (!TF_VERIFY(stage, "Failed to open stage at path <%s>.\n", argv[1])) {
        return -1;
    }

    const std::string outputFilePrefix =
        TfStringReplace(TfGetBaseName(argv[1]), ".usda", "");

    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);

    const auto proxyViewSi =
        HdInstanceProxyViewSceneIndex::New(sceneIndices.finalSceneIndex);

    // Collection expression evaluation w/ instance proxy view.
    {
        std::ofstream output(
            outputFilePrefix + "_collectionExpressionEvaluation.txt");

        std::vector<SdfPathExpression> exprs;
        for (int i = 2; i < argc; ++i) {
            exprs.emplace_back(argv[i]);
        }

        size_t idx = 1;
        for (const auto &expr : exprs) {
            HdInstancingAwareCollectionExpressionEvaluator eval(
                proxyViewSi, expr, UsdImagingGetCollectionPredicateLibrary());

            output << (idx++) << ". Evaluating expression \""
                   << expr.GetText() << "\"\n\n";

            HdInstancingAwareCollectionExpressionEvaluator::MatchResult result =
                eval.GetAllMatches(
                    SdfPath::AbsoluteRootPath(),
                    HdCollectionExpressionEvaluator::MatchAll,
                    /*includeInstanceProxyMatches=*/true);

            _PrintPaths(output, "(Hydra) Matched non-instance prim paths",
                result.nonInstanceMatches);

            _PrintPaths(output, "(Hydra) Fully matched instance prim paths",
                result.fullyMatchedInstances);

            _PrintPaths(output, "(Hydra) Partially matched instance prim paths",
                result.partiallyMatchedInstances);

            _PrintPaths(output, "(Hydra) Matched instance proxy prim paths",
                result.matchedInstanceProxyPaths);


            _PrintPaths(output,
                "All (non-internal) matched prim paths w/ Hydra",
                _ComputeAllHydraMatches(result), /*printCount=*/true);

            _PrintPaths(output,
                "All matched prim paths w/ USD",
                _ComputeAllUsdMatches(stage, expr), /*printCount=*/true);

            // XXX Ideally, we can compare the two sets above. There are
            //     some open questions to address first.
            // 1. Should the pseudo-root path be matched?
            //    Hydra does, USD doesn't. Latter seems more correct, but Hydra
            //    treats the absolute root path as a valid prim for evaluation.
            //
            // 2. Since the traversal predicate dictates what is matched in USD,
            //    what's the expectation in practice w/ instancing in play?
            //
            // 3. What is the expected behavior when only the outer USD instance
            //    prim matches, say with light linking?
            //

            output << "-----------------------------------------------------\n";
        }
    }

    return 0;
}
