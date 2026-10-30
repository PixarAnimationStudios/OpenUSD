//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Image test for HdsiExpressionEvaluatingSceneIndexBase.
//
// Usage:
//   testUsdImagingGLExpressionEvaluatingSceneIndex
//       -offscreen -frameAll -lighting -sceneLights -shading smooth
//       -stage chemistry.usda
//       -write out.png
//       -exprs "/World/Row_NoInstancing//" "/World/Row_NI_1Level//"
//
// The -exprs flag accepts one or more SdfPathExpression strings. For each
// expression the test sets collection:invis:membershipExpression on
// /World/InvisControl, renders the scene, and writes out an output image.
//
// Prims targeted by the "invis" collection should not be visible in the
// rendered output.
//

#include "pxr/usdImaging/usdImaging/collectionPredicateLibrary.h"
#include "pxr/usdImaging/usdImagingGL/unitTestGLDrawing.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/token.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/collectionSchema.h"
#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/instancingAwareCollectionExpressionEvaluator.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/sceneIndexPlugin.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hdsi/expressionEvaluatingSceneIndexBase.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE


// ----------------------------------------------------------------------------
// _InvisCollectionSceneIndex
//
// Tracks all prims that carry a "invis" collection and evaluates the union of
// their membership expressions on every change. Provides:
//   - _ModifyPrim: visibility=false override for matched non-instance prims;
//     instancerTopology.mask override for instancers corresponding to matched
//     instance prims.
//   - _PrimsAdded/_PrimsRemoved/_PrimsDirtied: rebuild the unioned expression
//     and emit the appropriate PrimsDirtied notices downstream.
//
// XXX This scene index does not currently de-instance partially matched
// instances. This limitation will be addressed once
// HdsiExpressionEvaluatingSceneIndexBase supports de-instancing, 
//
class _InvisCollectionSceneIndex
    : public HdsiExpressionEvaluatingSceneIndexBase
{
public:
    TF_DECLARE_REF_PTRS(_InvisCollectionSceneIndex);

    static _InvisCollectionSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _InvisCollectionSceneIndex(inputSceneIndex));
    }

protected:
    _InvisCollectionSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
      : HdsiExpressionEvaluatingSceneIndexBase(
            inputSceneIndex,
            UsdImagingGetCollectionPredicateLibrary())
    {
    }

    // -----------------------------------------------------------------------
    // Query hooks

    void _ModifyPrim(
        const SdfPath &primPath, HdSceneIndexPrim *prim) const override
    {
        // Use the cached evaluation result from notice processing to override
        // visibility. For our simplistic use case, this strategy lets us avoid
        // re-evaluating the expression for each prim.
        //
        if (prim->primType == HdPrimTypeTokens->instancer) {
            const auto it = _instancerToMatchedInstances.find(primPath);
            if (it == _instancerToMatchedInstances.end()) {
                return;
            }
            const HdInstancerTopologySchema topologySchema =
                HdInstancerTopologySchema::GetFromParent(prim->dataSource);
            const HdPathArrayDataSourceHandle instanceLocationsDs =
                topologySchema.GetInstanceLocations();
            if (instanceLocationsDs) {
                const VtArray<SdfPath> locs =
                    instanceLocationsDs->GetTypedValue(0.f);
                VtArray<bool> mask(locs.size(), true);
                for (size_t i = 0; i < locs.size(); ++i) {
                    if (it->second.count(locs[i])) {
                        mask[i] = false;
                    }
                }
                prim->dataSource = HdOverlayContainerDataSource::New(
                    HdRetainedContainerDataSource::New(
                        HdInstancerTopologySchema::GetSchemaToken(),
                        HdInstancerTopologySchema::Builder()
                            .SetMask(HdRetainedTypedSampledDataSource<
                                VtArray<bool>>::New(mask))
                            .Build()),
                    prim->dataSource);
            }

        } else {
            static const auto invisibleVisibilityDataSource =
                HdRetainedContainerDataSource::New(
                    HdVisibilitySchema::GetSchemaToken(),
                    HdVisibilitySchema::Builder()
                        .SetVisibility(
                            HdRetainedTypedSampledDataSource<bool>::New(false))
                        .Build());

            if (_cachedMatchResult.nonInstanceMatches.count(primPath)) {
                prim->dataSource = HdOverlayContainerDataSource::New(
                    invisibleVisibilityDataSource,
                    prim->dataSource);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Notice hooks

    NoticeProcessingResult _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &inputEntries) override
    {
        bool expressionChanged = false;
        for (const auto &entry : inputEntries) {
            expressionChanged |= _UpdateTracking(entry.primPath);
        }
        if (expressionChanged) {
            _RebuildAndEvaluateUnionedExpression();
            const auto dirtied = _BuildDirtiedEntries();
            if (!dirtied.empty()) {
                return NoticeProcessingResult::ForwardInputNotices()
                    .SetDirtiedEntries(dirtied);
            }
        }
        return NoticeProcessingResult::ForwardInputNotices();
    }

    NoticeProcessingResult _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &inputEntries) override
    {
        bool expressionChanged = false;
        for (const auto &entry : inputEntries) {
            if (_trackedPrims.erase(entry.primPath)) {
                expressionChanged = true;
            }
        }
        if (expressionChanged) {
            _RebuildAndEvaluateUnionedExpression();
            const auto dirtied = _BuildDirtiedEntries();
            if (!dirtied.empty()) {
                return NoticeProcessingResult::ForwardInputNotices()
                    .SetDirtiedEntries(dirtied);
            }
        }
        return NoticeProcessingResult::ForwardInputNotices();
    }

    NoticeProcessingResult _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &inputEntries) override
    {
        bool expressionChanged = false;
        for (const auto &entry : inputEntries) {
            if (entry.dirtyLocators.Intersects(
                    HdCollectionsSchema::GetDefaultLocator())) {
                expressionChanged |= _UpdateTracking(entry.primPath);
            }
        }
        if (expressionChanged) {
            _RebuildAndEvaluateUnionedExpression();
            const auto dirtied = _BuildDirtiedEntries();
            if (!dirtied.empty()) {
                return NoticeProcessingResult::ForwardInputNotices()
                    .SetDirtiedEntries(dirtied);
            }
        }
        return NoticeProcessingResult::ForwardInputNotices();
    }

private:
    bool _UpdateTracking(const SdfPath &primPath)
    {
        const HdSceneIndexPrim prim =
            _GetInputSceneIndex()->GetPrim(primPath);
        const HdCollectionSchema collSchema =
            HdCollectionsSchema::GetFromParent(prim.dataSource)
                .GetCollection(TfToken("invis"));

        if (!collSchema.IsDefined()) {
            return _trackedPrims.erase(primPath) > 0;
        }
        _trackedPrims.insert(primPath);
        return true;
    }

    void _RebuildAndEvaluateUnionedExpression()
    {
        _prevMatchResult = std::move(_cachedMatchResult);
        _prevInstancerToInstances = std::move(_instancerToMatchedInstances);
        _cachedMatchResult = {};
        _instancerToMatchedInstances.clear();

        if (_trackedPrims.empty()) {
            _unionedExpression = SdfPathExpression();
            return;
        }

        std::optional<SdfPathExpression> unionExpr;
        for (const SdfPath &primPath : _trackedPrims) {
            const HdSceneIndexPrim prim =
                _GetInputSceneIndex()->GetPrim(primPath);
            const HdCollectionSchema collSchema =
                HdCollectionsSchema::GetFromParent(prim.dataSource)
                    .GetCollection(TfToken("invis"));
            const HdPathExpressionDataSourceHandle exprDs =
                collSchema.GetMembershipExpression();
            if (!exprDs) continue;
            const SdfPathExpression expr = exprDs->GetTypedValue(0.f);
            if (expr.IsEmpty()) continue;

            if (!unionExpr) {
                unionExpr = expr;
            } else {
                unionExpr = SdfPathExpression::MakeOp(
                    SdfPathExpression::ImpliedUnion,
                    std::move(*unionExpr),
                    expr);
            }
        }

        if (!unionExpr) {
            _unionedExpression = SdfPathExpression();
            return;
        }

        _unionedExpression = std::move(*unionExpr);

        _cachedMatchResult = _MakeEvaluator(_unionedExpression).GetAllMatches(
            SdfPath::AbsoluteRootPath(),
            HdCollectionExpressionEvaluator::MatchAll,
            /*includeInstanceProxyMatches=*/false);

        for (const SdfPath &partialPath :
                _cachedMatchResult.partiallyMatchedInstances) {
            TF_WARN("Instance <%s> is only partially matched by the 'invis' "
                    "collection expression. De-instancing is required to "
                    "correctly apply per-instance visibility.",
                    partialPath.GetText());
        }

        // Query the instancer paths for the fully matched instances and cache
        // the mapping.
        const SdfPathVector instancerPaths =
            GetInstancerPrimPaths(_cachedMatchResult.fullyMatchedInstances);
        size_t idx = 0;
        for (const SdfPath &instancePath :
                _cachedMatchResult.fullyMatchedInstances) {
            if (idx < instancerPaths.size() &&
                    !instancerPaths[idx].IsEmpty()) {
                _instancerToMatchedInstances[instancerPaths[idx]]
                    .insert(instancePath);
            }
            ++idx;
        }
    }

    HdSceneIndexObserver::DirtiedPrimEntries _BuildDirtiedEntries() const
    {
        HdSceneIndexObserver::DirtiedPrimEntries entries;

        static const HdDataSourceLocatorSet visLocSet{
            HdVisibilitySchema::GetDefaultLocator()};

        SdfPathSet visibilityDirtyPaths = _cachedMatchResult.nonInstanceMatches;
        visibilityDirtyPaths.insert(
            _prevMatchResult.nonInstanceMatches.begin(),
            _prevMatchResult.nonInstanceMatches.end());
        for (const SdfPath &primPath : visibilityDirtyPaths) {
            entries.push_back({primPath, visLocSet});
        }

        SdfPathSet instancerDirtyPaths;
        for (const auto &pair : _instancerToMatchedInstances) {
            instancerDirtyPaths.insert(pair.first);
        }
        for (const auto &pair : _prevInstancerToInstances) {
            instancerDirtyPaths.insert(pair.first);
        }
        for (const SdfPath &instancerPath : instancerDirtyPaths) {
            entries.push_back( {instancerPath,
                {HdInstancerTopologySchema::GetDefaultLocator()}});
        }

        return entries;
    }

private:
    SdfPathSet _trackedPrims;
    SdfPathExpression _unionedExpression;

    HdInstancingAwareCollectionExpressionEvaluator::MatchResult
        _cachedMatchResult;
    std::unordered_map<SdfPath, SdfPathSet, SdfPath::Hash>
        _instancerToMatchedInstances;

    HdInstancingAwareCollectionExpressionEvaluator::MatchResult
        _prevMatchResult;
    std::unordered_map<SdfPath, SdfPathSet, SdfPath::Hash>
        _prevInstancerToInstances;
};

// ----------------------------------------------------------------------------
// Test harness similar to testUsdImagingGLBasicDrawing that additionally
// accepts expressions for collection membership evaluation via the -exprs
// command-line argument.
// 

using UsdImagingGLEngineSharedPtr = std::shared_ptr<UsdImagingGLEngine>;

class My_TestGLDrawing : public UsdImagingGL_UnitTestGLDrawing
{
public:
    My_TestGLDrawing() {
        _rotate[0] = _rotate[1] = 0.f;
        _translate[0] = _translate[1] = _translate[2] = 0.f;
    }

    void SetExprs(std::vector<std::string> exprs) {
        _exprs = std::move(exprs);
    }

    void InitTest() override;
    void DrawTest(bool offscreen) override;

private:
    void _Draw(std::optional<size_t> index = std::nullopt);

    UsdStageRefPtr _stage;
    UsdImagingGLEngineSharedPtr _engine;

    // collection:invis:membershipExpression on /World/InvisControl, if present.
    UsdAttribute _exprAttr;

    // Expressions provided via -exprs.
    std::vector<std::string> _exprs;

    GfFrustum _frustum;
    GfMatrix4d _viewMatrix;

    float _rotate[2];
    float _translate[3];
};

void
My_TestGLDrawing::InitTest()
{
    std::cout << "My_TestGLDrawing::InitTest()\n";
    _stage = UsdStage::Open(GetStageFilePath());

    _engine.reset(new UsdImagingGLEngine(
        _stage->GetPseudoRoot().GetPath(),
        SdfPathVector{}));

    // Locate the membership-expression attribute on /World/InvisControl so
    // DrawTest can update it for each expression in _exprs.
    const UsdPrim control =
        _stage->GetPrimAtPath(SdfPath("/World/InvisControl"));
    if (control) {
        _exprAttr = control.GetAttribute(
            TfToken("collection:invis:membershipExpression"));
    }

    if (_ShouldFrameAll()) {
        TfTokenVector purposes = {
            UsdGeomTokens->default_,
            UsdGeomTokens->proxy
        };
        bool useExtentHints = true;
        UsdGeomBBoxCache bboxCache(
            UsdTimeCode::Default(), purposes, useExtentHints);

        GfBBox3d bbox = bboxCache.ComputeWorldBound(_stage->GetPseudoRoot());
        GfRange3d world = bbox.ComputeAlignedRange();
        GfVec3d worldCenter = (world.GetMin() + world.GetMax()) / 2.0;
        double worldSize = world.GetSize().GetLength();

        if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
            _translate[0] = -worldCenter[0];
            _translate[1] = -worldCenter[2];
            _translate[2] = -worldCenter[1] - worldSize;
        } else {
            _translate[0] = -worldCenter[0];
            _translate[1] = -worldCenter[1];
            _translate[2] = -worldCenter[2] - worldSize;
        }
    }
}

void
My_TestGLDrawing::DrawTest(bool /*offscreen*/)
{
    std::cout << "My_TestGLDrawing::DrawTest()\n";

    if (_exprs.empty()) {
        // No expressions provided: render once with whatever is on the stage.
        _Draw();
        return;
    }

    const bool multiExpr = _exprs.size() > 1;
    for (size_t i = 0; i < _exprs.size(); ++i) {
        std::cout << "Setting expression: " << _exprs[i] << "\n";
        if (_exprAttr) {
            _exprAttr.Set(SdfPathExpression(_exprs[i]));
        }
        // Pass the expr index for use as suffix when multiple expressions are
        // provided.
        _Draw(multiExpr ? std::optional<size_t>(i) : std::nullopt);
    }
}

void
My_TestGLDrawing::_Draw(std::optional<size_t> index)
{
    const int width = GetWidth();
    const int height = GetHeight();

    const double aspectRatio = double(width) / height;
    _frustum.SetPerspective(60.0, aspectRatio, 1, 100000.0);

    _viewMatrix.SetIdentity();
    _viewMatrix *=
        GfMatrix4d().SetRotate(GfRotation(GfVec3d(0, 1, 0), _rotate[0]));
    _viewMatrix *=
        GfMatrix4d().SetRotate(GfRotation(GfVec3d(1, 0, 0), _rotate[1]));
    _viewMatrix *=
        GfMatrix4d().SetTranslate(
            GfVec3d(_translate[0], _translate[1], _translate[2]));

    GfMatrix4d projMatrix = _frustum.ComputeProjectionMatrix();

    if (UsdGeomGetStageUpAxis(_stage) == UsdGeomTokens->z) {
        _viewMatrix =
            GfMatrix4d().SetRotate(GfRotation(GfVec3d(1, 0, 0), -90.0)) *
            _viewMatrix;
    }

    _engine->SetCameraState(_viewMatrix, projMatrix);
    _engine->SetRenderViewport(GfVec4d(0, 0, width, height));
    _engine->SetRendererAov(GetRendererAov());

    UsdImagingGLRenderParams params;
    params.drawMode = GetDrawMode();
    params.enableLighting = IsEnabledTestLighting();
    params.complexity = _GetComplexity();
    params.cullStyle = GetCullStyle();
    params.clearColor = GetClearColor();
    params.clipPlanes = GetClipPlanes();

    _engine->Render(_stage->GetPseudoRoot(), params);

    std::string imageFilePath = GetOutputFilePath();
    if (!imageFilePath.empty()) {
        // Add a suffix like _000.png, _001.png, etc. if multiple expressions
        // were provided.
        if (index.has_value()) {
            std::stringstream suffix;
            suffix << "_" << std::setw(3) << std::setfill('0')
                   << index.value() << ".png";
            imageFilePath = TfStringReplace(
                imageFilePath, ".png", suffix.str());
        }
        std::cout << imageFilePath << "\n";
        WriteToFile(_engine.get(), HdAovTokens->color, imageFilePath);
    }
}

// ----------------------------------------------------------------------------

static void
_RegisterInvisCollectionSceneIndex() {
    // Use a large number so it is added towards/at the end.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 10000;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        HdSceneIndexPluginRegistryTokens->allRenderers,
        []( const std::string &renderInstanceId,
            HdSceneIndexBaseRefPtr const &inputSceneIndex,
            const HdContainerDataSourceHandle &inputArgs) {
            return _InvisCollectionSceneIndex::New(inputSceneIndex);
        },
        nullptr, // inputArgs
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd
    );
}


int
main(int argc, char *argv[])
{
    _RegisterInvisCollectionSceneIndex();

    My_TestGLDrawing driver;

    // Pre-scan argv for -exprs, extracting expression strings before
    // passing the remaining args to RunTest. Expressions are collected
    // until the next flag (arg starting with '-') or end of args.
    std::vector<std::string> exprs;
    std::vector<char*> filteredArgv;
    for (int i = 0; i < argc; ++i) {
        if (std::string(argv[i]) == "-exprs") {
            for (++i; i < argc && argv[i][0] != '-'; ++i) {
                exprs.push_back(argv[i]);
            }
            --i; // outer ++i will advance past the last consumed arg
        } else {
            filteredArgv.push_back(argv[i]);
        }
    }

    driver.SetExprs(std::move(exprs));
    driver.RunTest(static_cast<int>(filteredArgv.size()), filteredArgv.data());
    return 0;
}
