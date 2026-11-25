//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Tests that value clip layers opened lazily by many threads at once are
// resolved correctly and tracked for change notification.  Each iteration opens
// a fresh stage while no clip layers are open, so every clip opens its layer
// during the parallel value resolution phase.  It then edits each clip layer
// separately and checks that the stage reports exactly the attributes that use
// that layer.  Clip layers are shared among many clip sets so that threads also
// race to record the same layer in the clip cache.

#include "pxr/pxr.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <set>
#include <string>
#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_CLI;

// Each prim uses numClipsPerPrim consecutive layers from a pool of
// numClipLayers, so each layer is used by
// numPrims * numClipsPerPrim / numClipLayers clip sets.  All clip sets share
// one manifest.
static const size_t numPrims        = 64;
static const size_t numClipLayers   = 16;
static const size_t numClipsPerPrim = 4;

static std::atomic<size_t> numFailures(0);

#define CHECK(cond, ...)                                        \
    do {                                                        \
        if (!(cond)) {                                          \
            ++numFailures;                                      \
            fprintf(stderr, "FAILED %s:%d: %s: %s\n",           \
                    __FILE__, __LINE__, #cond,                  \
                    TfStringPrintf(__VA_ARGS__).c_str());       \
        }                                                       \
    } while (0)

static std::string
_ClipLayerPath(size_t layerIndex)
{
    return TfAbsPath(TfStringPrintf("clip_%zu.usda", layerIndex));
}

static std::string
_ManifestPath()
{
    return TfAbsPath("manifest.usda");
}

static SdfPath
_PrimPath(size_t primIndex)
{
    return SdfPath(TfStringPrintf("/Prim_%zu", primIndex));
}

static size_t
_LayerIndexForClip(size_t primIndex, size_t clipIndex)
{
    return (primIndex + clipIndex) % numClipLayers;
}

// Value authored in clip layer layerIndex at time t.  Clip k of each prim is
// active from time k, and clip times are the identity, so the prim at primIndex
// takes its value at integral time k from layer _LayerIndexForClip(primIndex,
// k) at time k.
static double
_ClipValue(size_t layerIndex, size_t t, double offset)
{
    return offset + 10.0 * layerIndex + t;
}

static double
_ExpectedValue(size_t primIndex, size_t t, double offset)
{
    return _ClipValue(_LayerIndexForClip(primIndex, t), t, offset);
}

static void
_WriteTestFiles(const std::string &rootPath)
{
    const TfToken x("x");
    const SdfPath clipPrimPath("/Clip");
    const SdfPath clipAttrPath = clipPrimPath.AppendProperty(x);

    {
        SdfLayerRefPtr manifest = SdfLayer::CreateNew(_ManifestPath());
        SdfPrimSpecHandle prim = SdfPrimSpec::New(
            manifest, "Clip", SdfSpecifierOver);
        SdfAttributeSpec::New(prim, "x", SdfValueTypeNames->Double);
        TF_AXIOM(manifest->Save());
    }

    for (size_t j = 0; j != numClipLayers; ++j) {
        SdfLayerRefPtr clip = SdfLayer::CreateNew(_ClipLayerPath(j));
        SdfPrimSpecHandle prim = SdfPrimSpec::New(
            clip, "Clip", SdfSpecifierOver);
        SdfAttributeSpec::New(prim, "x", SdfValueTypeNames->Double);
        for (size_t t = 0; t != numClipsPerPrim; ++t) {
            clip->SetTimeSample(clipAttrPath, t, _ClipValue(j, t, 0.0));
        }
        TF_AXIOM(clip->Save());
    }

    UsdStageRefPtr stage = UsdStage::CreateNew(rootPath);
    for (size_t i = 0; i != numPrims; ++i) {
        UsdPrim prim = stage->DefinePrim(_PrimPath(i));
        prim.CreateAttribute(x, SdfValueTypeNames->Double);

        VtArray<SdfAssetPath> assetPaths;
        VtVec2dArray active;
        for (size_t k = 0; k != numClipsPerPrim; ++k) {
            assetPaths.push_back(SdfAssetPath(TfStringPrintf(
                "clip_%zu.usda", _LayerIndexForClip(i, k))));
            active.push_back(GfVec2d(k, k));
        }
        UsdClipsAPI clips(prim);
        clips.SetClipAssetPaths(assetPaths);
        clips.SetClipPrimPath(clipPrimPath.GetString());
        clips.SetClipActive(active);
        clips.SetClipTimes(
            VtVec2dArray{GfVec2d(0, 0), GfVec2d(numClipsPerPrim - 1,
                                                numClipsPerPrim - 1)});
        clips.SetClipManifestAssetPath(SdfAssetPath("manifest.usda"));
    }
    TF_AXIOM(stage->GetRootLayer()->Save());
}

// Return the number of clip and manifest layers that are open.
static size_t
_NumOpenClipLayers()
{
    size_t n = SdfLayer::Find(_ManifestPath()) ? 1 : 0;
    for (size_t j = 0; j != numClipLayers; ++j) {
        if (SdfLayer::Find(_ClipLayerPath(j))) {
            ++n;
        }
    }
    return n;
}

// Records the paths reported by the ObjectsChanged notices for one stage.
class _Listener : public TfWeakBase
{
public:
    explicit _Listener(const UsdStageRefPtr &stage) {
        _key = TfNotice::Register(
            TfCreateWeakPtr(this), &_Listener::_HandleNotice,
            UsdStagePtr(stage));
    }
    ~_Listener() {
        TfNotice::Revoke(_key);
    }

    void Clear() {
        numNotices = 0;
        infoOnlyPaths.clear();
        resyncedPaths.clear();
    }

    size_t numNotices = 0;
    std::set<SdfPath> infoOnlyPaths;
    std::set<SdfPath> resyncedPaths;

private:
    void _HandleNotice(const UsdNotice::ObjectsChanged &n) {
        ++numNotices;
        for (const SdfPath &p : n.GetChangedInfoOnlyPaths()) {
            infoOnlyPaths.insert(p);
        }
        for (const SdfPath &p : n.GetResyncedPaths()) {
            resyncedPaths.insert(p);
        }
    }

    TfNotice::Key _key;
};

static void
_CheckValues(const UsdStageRefPtr &stage, double offset, size_t start = 0)
{
    const TfToken x("x");
    for (size_t n = 0; n != numPrims; ++n) {
        const size_t i = (start + n) % numPrims;
        UsdAttribute attr = stage->GetAttributeAtPath(
            _PrimPath(i).AppendProperty(x));
        for (size_t t = 0; t != numClipsPerPrim; ++t) {
            double value = 0.0;
            const bool ok = attr.Get(&value, UsdTimeCode(t));
            CHECK(ok && value == _ExpectedValue(i, t, offset),
                  "%s at time %zu: got %g, expected %g",
                  attr.GetPath().GetText(), t, value,
                  _ExpectedValue(i, t, offset));
        }
    }
}

// Resolve every attribute at every time on numThreads threads at once.  The
// threads are divided into groups of threadsPerGroup.  Threads in a group visit
// the prims in the same order, so that they race to open each clip's layer.
// Each group starts at a different prim, so that different clips that share a
// layer open it at the same time and race to record it in the clip cache.
static void
_ResolveInParallel(
    const UsdStageRefPtr &stage, size_t numThreads, size_t threadsPerGroup)
{
    const size_t numGroups =
        std::max<size_t>(1, numThreads / threadsPerGroup);
    WorkWithScopedDispatcher([&](WorkDispatcher &wd) {
        for (size_t n = 0; n != numThreads; ++n) {
            const size_t start = (n % numGroups) * numPrims / numGroups;
            wd.Run([&stage, start]() { _CheckValues(stage, 0.0, start); });
        }
    });
}

static void
_EditAndCheckNotices(const UsdStageRefPtr &stage)
{
    const TfToken x("x");
    const SdfPath clipAttrPath("/Clip.x");
    const double offset = 1000.0;

    _Listener listener(stage);

    // Each clip layer must report exactly the attributes that use it.
    for (size_t j = 0; j != numClipLayers; ++j) {
        SdfLayerHandle layer = SdfLayer::Find(_ClipLayerPath(j));
        CHECK(layer, "clip layer %zu is not open", j);
        if (!layer) {
            continue;
        }

        std::set<SdfPath> expected;
        for (size_t i = 0; i != numPrims; ++i) {
            for (size_t k = 0; k != numClipsPerPrim; ++k) {
                if (_LayerIndexForClip(i, k) == j) {
                    expected.insert(_PrimPath(i).AppendProperty(x));
                }
            }
        }

        listener.Clear();
        {
            SdfChangeBlock block;
            for (size_t t = 0; t != numClipsPerPrim; ++t) {
                layer->SetTimeSample(
                    clipAttrPath, t, _ClipValue(j, t, offset));
            }
        }
        CHECK(listener.numNotices == 1,
              "clip layer %zu: got %zu notices", j, listener.numNotices);
        CHECK(listener.infoOnlyPaths == expected,
              "clip layer %zu: got %zu info-only paths, expected %zu",
              j, listener.infoOnlyPaths.size(), expected.size());
        CHECK(listener.resyncedPaths.empty(),
              "clip layer %zu: got %zu resynced paths",
              j, listener.resyncedPaths.size());
    }

    _CheckValues(stage, offset);

    // The manifest is used by every clip set, so an edit to its attribute must
    // report that attribute on every prim.
    SdfLayerHandle manifest = SdfLayer::Find(_ManifestPath());
    CHECK(manifest, "manifest is not open");
    if (manifest) {
        std::set<SdfPath> expected;
        for (size_t i = 0; i != numPrims; ++i) {
            expected.insert(_PrimPath(i).AppendProperty(x));
        }

        listener.Clear();
        manifest->GetAttributeAtPath(clipAttrPath)->SetDocumentation("edit");
        CHECK(listener.numNotices == 1,
              "manifest: got %zu notices", listener.numNotices);
        CHECK(listener.infoOnlyPaths == expected,
              "manifest: got %zu info-only paths, expected %zu",
              listener.infoOnlyPaths.size(), expected.size());
    }
}

static void
_RunIteration(
    const SdfLayerRefPtr &rootLayer, size_t numThreads, size_t threadsPerGroup)
{
    // No clip layers may be open, or clips would find them already open when
    // constructed and nothing would race to open them.
    CHECK(_NumOpenClipLayers() == 0,
          "%zu clip layers open before opening stage", _NumOpenClipLayers());

    UsdStageRefPtr stage = UsdStage::Open(rootLayer);
    CHECK(_NumOpenClipLayers() == 0,
          "%zu clip layers open after opening stage", _NumOpenClipLayers());

    _ResolveInParallel(stage, numThreads, threadsPerGroup);
    CHECK(_NumOpenClipLayers() == numClipLayers + 1,
          "%zu clip layers open after resolving, expected %zu",
          _NumOpenClipLayers(), numClipLayers + 1);

    _EditAndCheckNotices(stage);

    // Dropping the stage closes the clip layers, discarding the edits, so the
    // next iteration starts again from the files.
    stage.Reset();
}

int
main(int argc, char const **argv)
{
    size_t numThreads = 0;
    size_t msecsToRun = 0;
    size_t threadsPerGroup = 0;
    bool runForever = false;

    CLI::App app(
        "Tests concurrent opening of value clip layers",
        "testUsdValueClipsThreaded");
    app.add_flag("--forever", runForever, "Run until a failure");
    app.add_option("--numThreads", numThreads, "Number of threads to use")
        ->default_val(std::thread::hardware_concurrency());
    app.add_option("--msec", msecsToRun, "Milliseconds to run")
        ->default_val(5 * 1000);
    app.add_option("--threadsPerGroup", threadsPerGroup,
                   "Number of threads that visit prims in the same order")
        ->default_val(4)->check(CLI::PositiveNumber);
    CLI11_PARSE(app, argc, argv);

    const std::string rootPath = TfAbsPath("root.usda");
    _WriteTestFiles(rootPath);

    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(rootPath);
    TF_AXIOM(rootLayer);

    printf("Using %zu threads, %zu prims, %zu clip layers\n",
           numThreads, numPrims, numClipLayers);

    TfStopwatch sw;
    size_t numIterations = 0;
    do {
        sw.Start();
        _RunIteration(rootLayer, numThreads, threadsPerGroup);
        sw.Stop();
        ++numIterations;
    } while (numFailures == 0 &&
             (runForever ||
              static_cast<size_t>(sw.GetMilliseconds()) < msecsToRun));

    printf("Ran %zu iterations in %.3f sec (%.3f ms/iteration), "
           "%zu failures\n",
           numIterations, sw.GetSeconds(),
           sw.GetMilliseconds() / double(numIterations),
           size_t(numFailures));

    return numFailures == 0 ? 0 : 1;
}
