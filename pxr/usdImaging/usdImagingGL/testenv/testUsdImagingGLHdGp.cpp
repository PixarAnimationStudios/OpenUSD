//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/usdImaging/usdImaging/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"

#include <iostream>

///////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_USING_DIRECTIVE

using UsdImagingGLEngineSharedPtr = std::shared_ptr<class UsdImagingGLEngine>;

namespace 
{

// used for capturing scene change notices
class _RecordingSceneIndexObserver : public HdSceneIndexObserver
{
public:

    enum EventType {
        EventType_PrimAdded = 0,
        EventType_PrimRemoved,
        EventType_PrimDirtied,
    };

    struct Event
    {
        EventType eventType;
        SdfPath primPath;
        TfToken primType;
        HdDataSourceLocator locator;
    };

    using EventVector = std::vector<Event>;

    void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override
    {
        for (const AddedPrimEntry &entry : entries) {
            _events.emplace_back(Event{
                EventType_PrimAdded, entry.primPath, entry.primType});
        }
    }

    void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override
    {
        for (const RemovedPrimEntry &entry : entries) {
            _events.emplace_back(Event{EventType_PrimRemoved, entry.primPath});
        }
    }

    void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override
    {
        for (const DirtiedPrimEntry &entry : entries) {
            for (const HdDataSourceLocator &locator : entry.dirtyLocators) {
                _events.emplace_back(Event{EventType_PrimDirtied,
                    entry.primPath, TfToken(), locator});
            }
        }
    }

    void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override
    {
        ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
    }

    EventVector GetEvents()
    {
        return _events;
    }

    void Clear()
    {
        _events.clear();
    }

private:
    EventVector _events;
};

}

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    std::string stageFilePath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stage" && i + 1 < argc) {
            stageFilePath = argv[++i];
        }
    } 

    // prepare GL context
    GarchGLDebugWindow window("UsdImagingGL Test", 512, 512);
    window.Init();

    // open stage
    UsdStageRefPtr stage = UsdStage::Open(stageFilePath);

    UsdImagingGLEngineSharedPtr engine;
    SdfPathVector excludedPaths;

    UsdImagingGLRenderParams params;
    params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    params.enableLighting = false;
    params.showGuides = false;
    params.showProxy = false;
    params.showRender = false;
    params.frame = 1;

    engine.reset(
        new UsdImagingGLEngine(stage->GetPseudoRoot().GetPath(), 
                excludedPaths));

    engine->SetRendererAov(HdAovTokens->color);

    //-------------------------------------------------------------------------
    engine->Render(stage->GetPseudoRoot(), params);

    // NOTE: this makes assumptions based on scene index emulation and will
    //       need to be updated when UsdImagingGLEngine no longer uses the
    //       emulated legacy APIs.
    auto registeredSceneIndexNames =
        HdSceneIndexNameRegistry::GetInstance().GetRegisteredNames();

    if (registeredSceneIndexNames.size() != 1) {
        std::cerr <<
            "expecting 1 registered scene index (via emulation) and found "
                << registeredSceneIndexNames.size() << " instead." << std::endl;

        return -1;
    }

    HdSceneIndexBaseRefPtr sceneIndex =
        HdSceneIndexNameRegistry::GetInstance().GetNamedSceneIndex(
            registeredSceneIndexNames[0]);

    if (!sceneIndex) {
        std::cerr << "registered scene index is null." << std::endl;
        return -1;
    }

    _RecordingSceneIndexObserver observer;

    sceneIndex->AddObserver(HdSceneIndexObserverPtr(&observer));

    // we are testing to confirm that an existing input prim is allowed to
    // pass through
    const size_t inputChildCount = 1;

    SdfPath cubePerMeshProcPrimPath("/World/cubePerMeshProc");

    std::cout << "Checking initial child count of: "
        << cubePerMeshProcPrimPath << std::endl;
    TF_AXIOM(
        sceneIndex->GetChildPrimPaths(cubePerMeshProcPrimPath).size() ==
            (4 + inputChildCount));
    std::cout << "...OK" << std::endl;


    UsdRelationship srcMeshRel;

    UsdPrim cubePerMeshProcPrim = stage->GetPrimAtPath(cubePerMeshProcPrimPath);
    if (cubePerMeshProcPrim) {
        srcMeshRel = cubePerMeshProcPrim.GetRelationship(
            TfToken("primvars:sourceMeshPath"));
    }

    //-------------------------------------------------------------------------

    std::cout << "retargeting 'primvars:sourceMeshPath' of "
        << cubePerMeshProcPrimPath << std::endl;

    if (srcMeshRel) {
        srcMeshRel.SetTargets({SdfPath("/World/myMesh")});
    }

    engine->Render(stage->GetPseudoRoot(), params);

    std::cout << "Checking adjusted child count of: "
        << cubePerMeshProcPrimPath << "..." << std::endl;
    TF_AXIOM(sceneIndex->GetChildPrimPaths(cubePerMeshProcPrimPath).size() ==
            (8 + inputChildCount));
    std::cout << "...OK" << std::endl;

    //-------------------------------------------------------------------------
    // confirm dirtied xforms of child prims with myMesh frame change

    auto _CountChildPrimsWithDirtiedXforms = [&observer](
            const SdfPath &parentPath) {
        std::unordered_set<SdfPath, TfHash> childPrimsWithDirtiedXforms;
        for (const auto &e : observer.GetEvents()) {
            if (e.eventType ==
                        _RecordingSceneIndexObserver::EventType_PrimDirtied
                    && e.primPath.HasPrefix(parentPath)
                    && e.primPath != parentPath) {
                if (e.locator.Intersects(HdXformSchema::GetDefaultLocator())) {
                    childPrimsWithDirtiedXforms.insert(e.primPath);
                }
            }
        }

        return childPrimsWithDirtiedXforms.size();
    };

    {
        observer.Clear();
        params.frame = 2;
        engine->Render(stage->GetPseudoRoot(), params);

        std::cout << "changing frame to 2" << std::endl;

        std::cout << "confirming count of child prims with dirtied xforms..."
            << std::endl;

        TF_AXIOM(
            _CountChildPrimsWithDirtiedXforms(cubePerMeshProcPrimPath) == 8);
        std::cout << "...OK" << std::endl;
    }
    //-------------------------------------------------------------------------
    {
        std::cout << "restoring 'primvars:sourceMeshPath' of "
            << cubePerMeshProcPrimPath << std::endl;

        if (srcMeshRel) {
            srcMeshRel.SetTargets({SdfPath("/World/myMesh2")});
        }

        engine->Render(stage->GetPseudoRoot(), params);

        std::cout << "Checking restored child count of: "
            << cubePerMeshProcPrimPath << "..." << std::endl;
        TF_AXIOM(
            sceneIndex->GetChildPrimPaths(cubePerMeshProcPrimPath).size() == 
                (4 + inputChildCount));
        std::cout << "...OK" << std::endl;
    }

    //-------------------------------------------------------------------------
    // Confirm no child prim transforms when changing back to frame 1
    // as myMesh2 does not animate
    {
        observer.Clear();
        params.frame = 1;
        engine->Render(stage->GetPseudoRoot(), params);

        std::cout << "changing frame to 1" << std::endl;\
        std::cout << "confirming no child prims with dirtied xforms..."
            << std::endl;

        TF_AXIOM(
            _CountChildPrimsWithDirtiedXforms(cubePerMeshProcPrimPath) == 0);
        std::cout << "...OK" << std::endl;
    }

    //-------------------------------------------------------------------------
    // Changes to "primvars:scale" should dirty the xform of all of the child
    // cube prims.
    {
        observer.Clear();
        cubePerMeshProcPrim.GetAttribute(
            TfToken("primvars:scale")).Set(VtValue(1.25f));
        engine->Render(stage->GetPseudoRoot(), params);

        std::cout << "setting 'primvars:scale' of "
            << cubePerMeshProcPrimPath << std::endl;

        std::cout << "confirming child prims xform dirtied..." << std::endl;

        TF_AXIOM(
            _CountChildPrimsWithDirtiedXforms(cubePerMeshProcPrimPath) == 4);
        std::cout << "...OK" << std::endl;
    }

    SdfPath makeSomeStuffProcPrimPath("/World/myGenerativeProc");
    
    //-------------------------------------------------------------------------
    // confirm initial state of myGenerativeProc
    {
        auto childPaths =
            sceneIndex->GetChildPrimPaths(makeSomeStuffProcPrimPath);

        std::cout << "confirming initial child count of "
            << makeSomeStuffProcPrimPath << "..." << std::endl;
        TF_AXIOM(childPaths.size() == 3);
        std::cout << "...OK" << std::endl;

        std::cout << "confirming child prim types of "
            << makeSomeStuffProcPrimPath << "..." << std::endl;
        for (const SdfPath &childPath : childPaths) {
            TF_AXIOM(
                sceneIndex->GetPrim(childPath).primType == TfToken("stuff"));
        }
        std::cout << "...OK" << std::endl;


        std::cout << "changing 'primvars:myDepth' of  "
            << makeSomeStuffProcPrimPath << "..." << std::endl;

        UsdPrim procPrim = stage->GetPrimAtPath(makeSomeStuffProcPrimPath);
        procPrim.GetAttribute(TfToken("primvars:myDepth")).Set(VtValue(2));

        engine->Render(stage->GetPseudoRoot(), params);

        std::cout << "confirming child and grandchild types "
            << makeSomeStuffProcPrimPath << "..." << std::endl;
        for (const SdfPath &childPath :
                sceneIndex->GetChildPrimPaths(makeSomeStuffProcPrimPath)) {
            TF_AXIOM(
                sceneIndex->GetPrim(childPath).primType == TfToken());

            for (const SdfPath &grandChildPath :
                    sceneIndex->GetChildPrimPaths(childPath)) {
                TF_AXIOM(
                    sceneIndex->GetPrim(grandChildPath).primType ==
                        TfToken("stuff"));
            }
        }
        std::cout << "...OK" << std::endl;

    }


    return EXIT_SUCCESS;
}
