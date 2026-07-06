//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/instanceProxyViewSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/pathExpression.h"

#include <fstream>
#include <iostream>
#include <ostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {


std::string
_GetPrimTypeToLog(const TfToken &primType)
{
    if (primType.IsEmpty()) {
        return "<empty>";
    } else {
        return primType.GetString();
    }
}

}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: testUsdImagingInstanceProxyTraversal"
                     " <file.usd>\n";
        return -1;
    }

    UsdStageRefPtr stage = UsdStage::Open(argv[1]);
    if (!TF_VERIFY(stage, "Failed to open stage at path <%s>.\n", argv[1])) {
        return -1;
    }

    const std::string outputFilePrefix =
        TfStringReplace(TfGetBaseName(argv[1]), ".usda", "");

    // -------------------------------------------------------------------------
    // USD traversal
    //
    {
        std::ofstream output(outputFilePrefix + "_usdTraversal.txt");

        UsdPrimRange range = UsdPrimRange::Stage(
            stage, UsdTraverseInstanceProxies(UsdPrimDefaultPredicate));

        for (const UsdPrim &prim : range) {
            output
                << prim.GetPath()
                << " type=" << _GetPrimTypeToLog(prim.GetTypeName())
                << (prim.IsInstanceProxy() ? " (instance proxy)" : "");
            output << "\n";
        }
    }
    
    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);

    // ------------------------------------------------------------------------
    // Scene index traversal w/o instance proxy view
    //
    {
        std::ofstream output(outputFilePrefix + "_sceneIndexTraversal.txt");

        HdSceneIndexPrimView view(
            sceneIndices.finalSceneIndex, SdfPath::AbsoluteRootPath()); 

        for (const SdfPath &primPath : view) {
            const auto prim = sceneIndices.finalSceneIndex->GetPrim(primPath);
            output
                << primPath
                // Needed for Windows only bug where paths above 128 characters
                // get linesplit in fc.exe
                << (primPath.GetString().length() > 100 ? "\n\t" : " ")
                << "type=" << _GetPrimTypeToLog(prim.primType);
            output << "\n";
        }
    }

    const auto proxyViewSi =
        HdInstanceProxyViewSceneIndex::New(sceneIndices.finalSceneIndex);

    // ------------------------------------------------------------------------
    // Scene index traversal of instance proxy view
    //
    {
        std::ofstream output(
            outputFilePrefix + "_sceneIndexInstanceProxyTraversal.txt");

        HdSceneIndexPrimView view(
            proxyViewSi, SdfPath::AbsoluteRootPath());

        for (const SdfPath &primPath : view) {
            const auto prim = proxyViewSi->GetPrim(primPath);
            output
                << primPath
                // Needed for Windows only bug where paths above 128 characters
                // get linesplit in fc.exe
                << (primPath.GetString().length() > 100 ? "\n\t" : " ")
                << "type=" << _GetPrimTypeToLog(prim.primType)
                << (proxyViewSi->IsInstanceProxy(primPath)
                    ? " (instance proxy)" : "");
            output << "\n";
        }
    }

    return 0;
}
