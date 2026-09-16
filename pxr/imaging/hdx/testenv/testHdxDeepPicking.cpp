//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/imaging/garch/glDebugWindow.h"

#include "pxr/imaging/hdSt/unitTestGLDrawing.h"
#include "pxr/imaging/hdSt/unitTestHelper.h"

#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hdx/unitTestUtils.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/errorMark.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens, (pickables));

// ---------------------------------------------------------------------------

class Hdx_TestDriver : public HdSt_TestDriverBase<Hdx_UnitTestDelegate>
{
public:
    Hdx_TestDriver();

    void DrawScene(GfVec4d const& viewport);

    HdxPickHitVector PickDeep(GfVec2i const& startPos, GfVec2i const& endPos,
        int screenW, int screenH, GfFrustum const& frustum,
        GfMatrix4d const& viewMatrix, TfToken const& pickTarget,
        int maxDeepEntries, GfVec2f const& depthRange = {0, 1});

protected:
    void _Init(HdReprSelector const& reprSelector) override;

private:
    HdRprimCollection _pickablesCol;
};

Hdx_TestDriver::Hdx_TestDriver()
{
    _Init(HdReprSelector(HdReprTokens->wireOnSurf));
}

void
Hdx_TestDriver::_Init(HdReprSelector const& reprSelector)
{
    _SetupSceneDelegate();
    Hdx_UnitTestDelegate& delegate = GetDelegate();

    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");
    delegate.AddRenderSetupTask(renderSetupTask);
    delegate.AddRenderTask(renderTask);

    VtValue vParam = delegate.GetTaskParam(renderSetupTask, HdTokens->params);
    HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
    param.enableLighting = true;
    delegate.SetTaskParam(renderSetupTask, HdTokens->params, VtValue(param));

    HdRprimCollection col(HdTokens->geometry, reprSelector);
    delegate.SetTaskParam(renderTask, HdTokens->collection, VtValue(col));

    delegate.AddPickTask(SdfPath("/pickTask"));

    _pickablesCol = HdRprimCollection(
        _tokens->pickables, HdReprSelector(HdReprTokens->hull));
    delegate.GetRenderIndex().GetChangeTracker().AddCollection(
        _tokens->pickables);
}

void
Hdx_TestDriver::DrawScene(GfVec4d const& viewport)
{
    SdfPath renderSetupTask("/renderSetupTask");
    SdfPath renderTask("/renderTask");

    HdxRenderTaskParams param =
        GetDelegate()
            .GetTaskParam(renderSetupTask, HdTokens->params)
            .Get<HdxRenderTaskParams>();
    param.viewport = viewport;
    param.aovBindings = _aovBindings;
    GetDelegate().SetTaskParam(
        renderSetupTask, HdTokens->params, VtValue(param));

    HdTaskSharedPtrVector tasks;
    tasks.push_back(GetDelegate().GetRenderIndex().GetTask(renderSetupTask));
    tasks.push_back(GetDelegate().GetRenderIndex().GetTask(renderTask));
    _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);
}

HdxPickHitVector
Hdx_TestDriver::PickDeep(GfVec2i const& startPos, GfVec2i const& endPos,
    int screenW, int screenH, GfFrustum const& frustum,
    GfMatrix4d const& viewMatrix, TfToken const& pickTarget, int maxDeepEntries,
    GfVec2f const& depthRange)
{
    HdxPickHitVector allHits;

    HdxPickTaskContextParams p;
    p.resolution = HdxUnitTestUtils::CalculatePickResolution(
        startPos, endPos, GfVec2i(4, 4));
    p.pickTarget = pickTarget;
    p.resolveMode = HdxPickTokens->resolveDeep;
    p.viewMatrix = viewMatrix;
    p.projectionMatrix = HdxUnitTestUtils::ComputePickingProjectionMatrix(
        startPos, endPos, GfVec2i(screenW, screenH), frustum);
    p.collection = _pickablesCol;
    p.maxNumDeepEntries = maxDeepEntries;
    p.deepPickDepthRange = depthRange;
    p.outHits = &allHits;

    HdTaskSharedPtrVector tasks;
    tasks.push_back(
        GetDelegate().GetRenderIndex().GetTask(SdfPath("/pickTask")));
    _GetEngine()->SetTaskContextData(HdxPickTokens->pickParams, VtValue(p));
    _GetEngine()->Execute(&GetDelegate().GetRenderIndex(), &tasks);

    return allHits;
}

// ---------------------------------------------------------------------------

class My_TestGLDrawing : public HdSt_UnitTestGLDrawing
{
public:
    explicit My_TestGLDrawing(int boxCount, bool instanced)
        : _requestedCount(boxCount)
        , _instanced(instanced)
    {
        SetCameraRotate(90, 0);
        SetCameraTranslate(GfVec3f(0, 0, -20));
    }

    void InitTest() override;
    void DrawTest() override;
    void OffscreenTest() override;
    void Present(uint32_t framebuffer) override;

    void MousePress(int button, int x, int y, int modKeys) override;
    void MouseRelease(int button, int x, int y, int modKeys) override;
    void MouseMove(int x, int y, int modKeys) override;

private:
    void _InitScene();
    void _DrawScene();

    // More than enough to capture every box face
    int _DefaultCapacity() const { return _boxCount * 8; }

    std::unique_ptr<Hdx_TestDriver> _driver;

    GfVec2i _startPos{}, _endPos{};
    int _requestedCount{}, _boxCount{};
    bool _instanced{};
    int _layerCount{};
    float _boxHalfExtent{};
};

// ---------------------------------------------------------------------------

static GfMatrix4d
_MakeScaleTranslate(float scale, float tx, float ty, float tz)
{
    GfMatrix4d m;
    m.SetScale(scale);
    m.SetTranslateOnly(GfVec3d(tx, ty, tz));
    return m;
}

void
My_TestGLDrawing::InitTest()
{
    _driver = std::make_unique<Hdx_TestDriver>();

    _InitScene();
    _driver->SetClearColor(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
    _driver->SetClearDepth(1.0f);
    _driver->SetupAovs(GetWidth(), GetHeight());
}

void
My_TestGLDrawing::_InitScene()
{
    // Pick the smallest base edge whose complete pyramid holds at least the
    // requested count: this rounds the count up to fill whole layers. Clamp
    // to >= 2 layers so there is always depth to stack and occlude.
    constexpr auto pyramidTotal = [](int edge) {
        return edge * (edge + 1) * (2 * edge + 1) / 6;
    };
    const int count = std::max(1, _requestedCount);
    int baseEdge = 2;
    while (pyramidTotal(baseEdge) < count) {
        baseEdge++;
    }
    const int layers = baseEdge;
    _boxCount = pyramidTotal(baseEdge);

    static constexpr float boxSize = 2;

    std::vector<float> layerZ(layers);
    for (int l = 0; l < layers; ++l) {
        layerZ[l] = l * boxSize;
    }
    const float zCenter = layerZ[layers - 1] / 2;
    for (float& z : layerZ) {
        z -= zCenter;
    }

    _layerCount = layers;
    _boxHalfExtent = boxSize / 2;

    std::cout << "Generated a " << layers << "-layer pyramid (base " << baseEdge
              << "x" << baseEdge << ") of " << _boxCount
              << " boxes for requested count " << count
              << (_instanced ? " (instanced)." : " (individual prims).")
              << "\n";

    const auto forEachCell = [&](auto&& visit) {
        int i = 0;
        for (int l = 0; l < layers; ++l) {
            const int edge = baseEdge - l;
            const float z = layerZ[l];
            for (int row = 0; row < edge; ++row) {
                for (int col = 0; col < edge; ++col, ++i) {
                    const float x = (col - (edge - 1) / 2.f) * boxSize;
                    const float y = (row - (edge - 1) / 2.f) * boxSize;
                    visit(i, x, y, z);
                }
            }
        }
    };

    Hdx_UnitTestDelegate& delegate = _driver->GetDelegate();

    static constexpr GfVec3f _boxColor{0.2f, 0.5f, 0.8f};

    if (_instanced) {
        const SdfPath instancerId{"/pyramidInstancer"};
        const SdfPath protoId{"/pyramidProto"};
        delegate.AddInstancer(instancerId);
        delegate.AddCube(protoId, GfMatrix4d{1}, false, instancerId,
            PxOsdOpenSubdivTokens->none, VtValue(_boxColor));

        VtVec3fArray scales(_boxCount);
        VtVec4fArray rotations(_boxCount);
        VtVec3fArray translations(_boxCount);
        VtIntArray prototypeIndices(_boxCount);

        forEachCell([&](int i, float x, float y, float z) {
            scales[i] = GfVec3f{1};
            rotations[i] = GfVec4f{};
            translations[i] = GfVec3f{x, y, z};
            prototypeIndices[i] = 0;
        });
        delegate.SetInstancerProperties(
            instancerId, prototypeIndices, scales, rotations, translations);
    } else {
        forEachCell([&](int i, float x, float y, float z) {
            delegate.AddCube(SdfPath{"/grid_box_" + std::to_string(i)},
                _MakeScaleTranslate(1, x, y, z), false, SdfPath(),
                PxOsdOpenSubdivTokens->none, VtValue(_boxColor));
        });
    }

    // Camera is looking toward -Z, so the pyramid is viewed from top down.
    // It's positioned far back enough so the base is fully visible, with a
    // margin.
    static constexpr float margin = 1.3f;
    const GfFrustum frustum = GetFrustum();
    const float nearClip = static_cast<float>(frustum.GetNearFar().GetMin());
    const float halfTanFovX =
        static_cast<float>(frustum.GetWindow().GetMax()[0]);
    const float halfTanFovY =
        static_cast<float>(frustum.GetWindow().GetMax()[1]);
    const float halfSpan = static_cast<float>(baseEdge);
    const float distance =
        std::max({halfSpan * margin / halfTanFovX + layerZ[0] + 1,
            halfSpan * margin / halfTanFovY + layerZ[0] + 1,
            layerZ[layers - 1] + 1 + nearClip + 0.1f});
    SetCameraTranslate(GfVec3f(0.f, 0.f, -distance));
}

void
My_TestGLDrawing::_DrawScene()
{
    const int width = GetWidth(), height = GetHeight();
    GfMatrix4d viewMatrix = GetViewMatrix();
    GfFrustum frustum = GetFrustum();

    _driver->GetDelegate().SetCamera(
        viewMatrix, frustum.ComputeProjectionMatrix());
    _driver->UpdateAovDimensions(width, height);
    _driver->DrawScene(GfVec4d(0, 0, width, height));
}

void
My_TestGLDrawing::DrawTest()
{
    _DrawScene();
}

void
My_TestGLDrawing::Present(uint32_t framebuffer)
{
    _driver->Present(GetWidth(), GetHeight(), framebuffer);
}

void
My_TestGLDrawing::MousePress(int button, int x, int y, int modKeys)
{
    HdSt_UnitTestGLDrawing::MousePress(button, x, y, modKeys);
    _startPos = _endPos = GetMousePos();
}

void
My_TestGLDrawing::MouseRelease(int button, int x, int y, int modKeys)
{
    HdSt_UnitTestGLDrawing::MouseRelease(button, x, y, modKeys);

    const int width = GetWidth(), height = GetHeight();
    const HdxPickHitVector hits = _driver->PickDeep(_startPos, _endPos, width,
        height, GetFrustum(), GetViewMatrix(),
        HdxPickTokens->pickPrimsAndInstances, _DefaultCapacity());

    std::cout << "\n=== Deep pick [" << _startPos[0] << "," << _startPos[1]
              << "] - [" << _endPos[0] << "," << _endPos[1]
              << "]: " << hits.size() << " unique hit(s) ===\n";
    constexpr size_t printMax = 10;
    const size_t printCount = std::min(hits.size(), printMax);
    for (size_t i = 0; i < printCount; ++i) {
        const HdxPickHit& hit = hits[i];
        std::cout << "  prim=" << hit.objectId;
        if (hit.instanceIndex >= 0) {
            std::cout << "  instance=" << hit.instanceIndex;
        }
        std::cout << "  depth=" << hit.normalizedDepth << "\n";
    }
    if (hits.size() > printCount) {
        std::cout << "  ... and " << (hits.size() - printCount) << " more\n";
    }

    _startPos = _endPos = {};
}

void
My_TestGLDrawing::MouseMove(int x, int y, int modKeys)
{
    HdSt_UnitTestGLDrawing::MouseMove(x, y, modKeys);
    _endPos = GetMousePos();
}
void
My_TestGLDrawing::OffscreenTest()
{
    const int width = GetWidth(), height = GetHeight();
    GfFrustum frustum = GetFrustum();
    const GfMatrix4d viewMatrix = GetViewMatrix();
    const TfToken pickTarget = HdxPickTokens->pickPrimsAndInstances;

    // Full-screen pick: covers all boxes
    const GfVec2i pickStart{0, 0}, pickEnd{width - 1, height - 1};

    // -----------------------------------------------------------------------
    // Test 1: Baseline: all boxes returned.
    std::cout << "\n=== Test 1: Baseline deep pick ===\n";
    const HdxPickHitVector baseline = _driver->PickDeep(pickStart, pickEnd,
        width, height, frustum, viewMatrix, pickTarget, _DefaultCapacity());
    std::cout << "  hits: " << baseline.size() << "  expected: " << _boxCount
              << "\n";
    TF_VERIFY(static_cast<int>(baseline.size()) == _boxCount,
        "Expected %d hits from %d grid boxes; got %zu", _boxCount, _boxCount,
        baseline.size());

    // -----------------------------------------------------------------------
    // Test 2: Full depth range [0, 1] is identical to the default.
    std::cout << "\n=== Test 2: Full depth range [0, 1] == baseline ===\n";
    {
        const HdxPickHitVector hits =
            _driver->PickDeep(pickStart, pickEnd, width, height, frustum,
                viewMatrix, pickTarget, _DefaultCapacity(), GfVec2f{0, 1});
        std::cout << "  hits: " << hits.size()
                  << "  expected: " << baseline.size() << "\n";
        TF_VERIFY(hits.size() == baseline.size(),
            "Full depth range [0,1] should match baseline (%zu); got %zu",
            baseline.size(), hits.size());
    }

    // -----------------------------------------------------------------------
    // Test 3: Outside of the [0, 1] depth range: all fragments discarded.
    // Boxes are always in [0, 1]; no fragments lie in [1.5, 2.0].
    std::cout << "\n=== Test 3: Out-of-range depth filter [1.5, 2.0] ===\n";
    {
        const HdxPickHitVector hits =
            _driver->PickDeep(pickStart, pickEnd, width, height, frustum,
                viewMatrix, pickTarget, _DefaultCapacity(), GfVec2f{1.5f, 2});
        std::cout << "  hits: " << hits.size() << "  expected: 0\n";
        TF_VERIFY(hits.empty(),
            "Depth range [1.5, 2.0] should produce 0 hits; got %zu",
            hits.size());
    }

    // -----------------------------------------------------------------------
    // Test 4: Zero-width depth range [0, 0]: no geometry at the near plane.
    std::cout << "\n=== Test 4: Zero-width depth range [0, 0] ===\n";
    {
        const HdxPickHitVector hits =
            _driver->PickDeep(pickStart, pickEnd, width, height, frustum,
                viewMatrix, pickTarget, _DefaultCapacity(), GfVec2f{0, 0});
        std::cout << "  hits: " << hits.size() << "  expected: 0\n";
        TF_VERIFY(hits.empty(),
            "Depth range [0,0] should produce 0 hits; got %zu", hits.size());
    }

    // -----------------------------------------------------------------------
    // Test 5: Small table, capacity-limited. A table far below the hit count
    // records at most its capacity (<= maxEntries) and drops the rest
    // (< _boxCount). The exact count depends on the hash distribution.
    const int lowCapacity = std::max(2, _boxCount / 6);
    std::cout << "\n=== Test 5: Small table (maxEntries=" << lowCapacity
              << " < " << _boxCount << ") ===\n";
    {
        const HdxPickHitVector hits = _driver->PickDeep(pickStart, pickEnd,
            width, height, frustum, viewMatrix, pickTarget, lowCapacity);
        std::cout << "  hits: " << hits.size()
                  << "  expected: <= " << lowCapacity << " and < " << _boxCount
                  << "\n";
        TF_VERIFY(static_cast<int>(hits.size()) <= lowCapacity,
            "Capacity-limited table must not exceed maxEntries=%d; got %zu",
            lowCapacity, hits.size());
        TF_VERIFY(static_cast<int>(hits.size()) < _boxCount,
            "Small table (maxEntries=%d < %d) must drop some hits; got %zu",
            lowCapacity, _boxCount, hits.size());
    }

    // -----------------------------------------------------------------------
    // Test 6: Medium table, still below the hit count, so still truncated.
    const int mediumCapacity = std::max(lowCapacity + 1, _boxCount * 2 / 3);
    std::cout << "\n=== Test 6: Medium table (maxEntries=" << mediumCapacity
              << " < " << _boxCount << ") ===\n";
    {
        const HdxPickHitVector hits = _driver->PickDeep(pickStart, pickEnd,
            width, height, frustum, viewMatrix, pickTarget, mediumCapacity);
        std::cout << "  hits: " << hits.size()
                  << "  expected: <= " << mediumCapacity << " and < "
                  << _boxCount << "\n";
        TF_VERIFY(static_cast<int>(hits.size()) <= mediumCapacity,
            "Capacity-limited table must not exceed maxEntries=%d; got %zu",
            mediumCapacity, hits.size());
        TF_VERIFY(static_cast<int>(hits.size()) < _boxCount,
            "Medium table (maxEntries=%d < %d) must drop some hits; got %zu",
            mediumCapacity, _boxCount, hits.size());
    }

    // -----------------------------------------------------------------------
    // Test 7: Sub-buffer overflow at nominal capacity (maxEntries == N).
    // Sub-buffers are a fixed 32 entries, so sizing the table to exactly the
    // hit count puts the mean bucket load at ~100% (N / (N/32) = 32 per
    // bucket). Over-mean buckets overflow, so the bounded neighbour probe
    // relocates most of that overflow into below-mean buckets. The table
    // still captures the large majority, but a few hits are dropped.
    const int fullCapacity = _boxCount;
    std::cout << "\n=== Test 7: Sub-buffer overflow at nominal capacity "
              << "(maxEntries=" << fullCapacity << " == " << _boxCount
              << ") ===\n";
    if (fullCapacity < 64) {
        // Skipped for tiny scenes whose cap spans fewer than two 32-entry
        // sub-buffers, where the table is too coarse for this to be meaningful.
        std::cout << "  skipped (needs >= 64 boxes)\n";
    } else {
        const HdxPickHitVector hits = _driver->PickDeep(pickStart, pickEnd,
            width, height, frustum, viewMatrix, pickTarget, fullCapacity);
        std::cout << "  hits: " << hits.size()
                  << "  expected: " << (_boxCount * 3 / 4)
                  << " <= hits <= " << _boxCount << "\n";
        TF_VERIFY(static_cast<int>(hits.size()) <= _boxCount,
            "Recorded count cannot exceed the scene (%d); got %zu", _boxCount,
            hits.size());
        TF_VERIFY(static_cast<int>(hits.size()) >= _boxCount * 3 / 4,
            "At nominal capacity the table should still capture the large "
            "majority (>= 75%% of %d); got %zu",
            _boxCount, hits.size());
    }

    // -----------------------------------------------------------------------
    // Test 8: pick faces. Front and back faces should always be visible, side
    // faces depend on the resolution, position and FoV. The result should be in
    // the range [2 * N, 6 * N].
    std::cout << "\n=== Test 8: Face deep pick ===\n";
    {
        const HdxPickHitVector hits =
            _driver->PickDeep(pickStart, pickEnd, width, height, frustum,
                viewMatrix, HdxPickTokens->pickFaces, _DefaultCapacity());
        std::cout << "  hits: " << hits.size()
                  << "  expected: " << 2 * _boxCount
                  << " <= hits <= " << 6 * _boxCount << "\n";
        TF_VERIFY(static_cast<int>(hits.size()) >= 2 * _boxCount,
            "Face pick should see front and back of every box "
            "(>= 2 * %d); got %zu",
            _boxCount, hits.size());
        TF_VERIFY(static_cast<int>(hits.size()) <= 6 * _boxCount,
            "Face pick cannot exceed 6 faces per box (6 * %d); got %zu",
            _boxCount, hits.size());
    }

    // -----------------------------------------------------------------------
    // Test 9: Depth range covering two consecutive inner layers.
    std::cout << "\n=== Test 9: Two inner layers by depth range ===\n";
    if (_layerCount < 4) {
        std::cout << "  skipped (needs >= 4 layers)\n";
    } else {
        const GfMatrix4d projMatrix = frustum.ComputeProjectionMatrix();
        const auto windowDepth = [&](float worldZ) {
            const GfVec4d clip =
                GfVec4d{0, 0, worldZ, 1} * viewMatrix * projMatrix;
            return static_cast<float>(clip[2] / clip[3] / 2 + 0.5);
        };
        // Complete pyramid: a layer has (_layerCount - layer) boxes per edge,
        // centered at z = (2 * layer - (_layerCount-1)) * _boxHalf.
        const auto layerCenterZ = [&](int layer) {
            return (2 * layer - (_layerCount - 1)) * _boxHalfExtent;
        };
        const auto layerBoxes = [&](int layer) {
            const int edge = _layerCount - layer;
            return edge * edge;
        };

        const int innerFront = _layerCount / 2;
        const int innerBack = innerFront - 1;

        // Sample a thin depth slice centred on the shared faces between the two
        // layers.
        const float interfaceZ = layerCenterZ(innerFront) - _boxHalfExtent;
        const float sliceHalfWidth = _boxHalfExtent / 2;
        const GfVec2f range{windowDepth(interfaceZ + sliceHalfWidth),
            windowDepth(interfaceZ - sliceHalfWidth)};
        const int expected = layerBoxes(innerFront) + layerBoxes(innerBack);

        const HdxPickHitVector hits =
            _driver->PickDeep(pickStart, pickEnd, width, height, frustum,
                viewMatrix, pickTarget, _DefaultCapacity(), range);
        std::cout << "  layers " << innerBack << " & " << innerFront
                  << ", depth range [" << range[0] << ", " << range[1]
                  << "]  hits: " << hits.size() << "  expected: " << expected
                  << "\n";

        const auto cellIndex = [&](const HdxPickHit& h) {
            if (_instanced)
                return h.instanceIndex;
            const std::string name = h.objectId.GetName();
            return std::atoi(name.c_str() + name.rfind('_') + 1);
        };

        std::vector<int> perLayer(_layerCount, 0);
        for (const HdxPickHit& hit : hits) {
            int index = cellIndex(hit);
            int layer = 0;
            for (; layer < _layerCount; ++layer) {
                const int boxCount = layerBoxes(layer);
                if (index < boxCount) {
                    break;
                }
                index -= boxCount;
            }
            if (layer < _layerCount)
                ++perLayer[layer];
        }

        bool layersOk = perLayer[innerFront] == layerBoxes(innerFront) &&
            perLayer[innerBack] == layerBoxes(innerBack);
        for (int layer = 0; layer < _layerCount; ++layer) {
            if (layer != innerFront && layer != innerBack) {
                layersOk &= perLayer[layer] == 0;
            }
        }

        TF_VERIFY(layersOk,
            "Depth slice must capture exactly layers %d and %d (%d boxes) and "
            "nothing else", innerBack, innerFront, expected);
    }
}

// ---------------------------------------------------------------------------

int
main(int argc, char* argv[])
{
    int boxCount = 24;
    bool instanced = false;
    std::vector<char*> filteredArgs;
    filteredArgs.push_back(argv[0]);
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--count") == 0 && i + 1 < argc) {
            boxCount = std::max(1, std::atoi(argv[++i]));
        } else if (std::strcmp(argv[i], "--instanced") == 0) {
            instanced = true;
        } else {
            filteredArgs.push_back(argv[i]);
        }
    }

    TfErrorMark mark;

    My_TestGLDrawing driver{boxCount, instanced};
    driver.RunTest(static_cast<int>(filteredArgs.size()), filteredArgs.data());

    if (mark.IsClean()) {
        std::cout << "OK" << "\n";
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << "\n";
        return EXIT_FAILURE;
    }
}
