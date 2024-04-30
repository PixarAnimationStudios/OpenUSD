//
// Copyright 2024 Pixar
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

#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/cube.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/materialSchema.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <queue>
#include <sstream>


PXR_NAMESPACE_OPEN_SCOPE

static std::ostream &
operator<<(std::ostream &out, const SdfPathSet &s) {
    out << "{\n";
    for (auto const& p : s) {
        out << p << "\n";
    }
    out << "}\n";

    return out;
}

static std::ostream &
operator<<(std::ostream &out,
           const HdSceneIndexObserver::DirtiedPrimEntries &entries) {
    out << "{\n";
    for (auto const& e : entries) {
        out << "<" << e.primPath << ">: { ";
        bool comma = false;
        for (auto const& l : e.dirtyLocators) {
           if (comma) {
              out << ", ";
           } else {
              comma = true;
           }
           out << l.GetString();
        }
       out << " }\n";
    }
    out << "}\n";

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

class PrimListener : public HdSceneIndexObserver
{
public:
    void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override {
        for (const AddedPrimEntry &entry : entries) {
            _prims.insert(entry.primPath);
        }
        _added.insert(_added.end(), entries.cbegin(), entries.cend());
    }

    void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override {
        for (const RemovedPrimEntry &entry : entries) {
            for (SdfPathSet::iterator it = _prims.begin();
                 it != _prims.end(); ) {
                if (it->HasPrefix(entry.primPath)) {
                    it = _prims.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override {
        _dirtied.insert(_dirtied.end(), entries.cbegin(), entries.cend());
    }

    void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override {
        ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
    }

    SdfPathSet const& GetPrimPaths() { return _prims; }
    AddedPrimEntries const& GetAdded() { return _added; }
    DirtiedPrimEntries const& GetDirtied() { return _dirtied; }

private:
    SdfPathSet _prims;
    AddedPrimEntries _added;
    DirtiedPrimEntries _dirtied;
};


void TraversalTest()
{
    UsdStageRefPtr stage = UsdStage::Open("traversal.usda");
    if (!TF_VERIFY(stage)) {
        return;
    }

    UsdImagingStageSceneIndexRefPtr inputSceneIndex =
        UsdImagingStageSceneIndex::New();
    if (!TF_VERIFY(inputSceneIndex)) {
        return;
    }

    PrimListener primListener;
    inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(&primListener));
    inputSceneIndex->SetStage(stage);

    SdfPathSet fromGetChild;
    std::queue<SdfPath> roots;
    roots.push(SdfPath::AbsoluteRootPath());
    while (!roots.empty()) {
        SdfPath root = roots.front();
        roots.pop();
        SdfPathVector children = inputSceneIndex->GetChildPrimPaths(root);
        fromGetChild.insert(children.begin(), children.end());
        for (auto const &p : children) {
            roots.push(p);
        }
    }

    SdfPathSet fromPrimsAdded = primListener.GetPrimPaths();

    // Verify that "/" was added, and we remove it for the compare test below
    // this one.
    size_t numErased = fromPrimsAdded.erase(SdfPath::AbsoluteRootPath());
    TF_VERIFY(numErased == 1);

    // We expect traversal by GetChildPrimPaths to return the same topology
    // as the add notices.
    TF_VERIFY(fromPrimsAdded == fromGetChild, "%s\n...vs...\n\n%s",
            TfStringify(fromPrimsAdded).c_str(),
            TfStringify(fromGetChild).c_str());

    inputSceneIndex->SetStage(nullptr);

    // After we reset the stage, we expect a PrimsRemoved{"/"}.
    TF_VERIFY(primListener.GetPrimPaths().empty());
}

static bool
_InvalidationsEqual(
        const HdSceneIndexObserver::DirtiedPrimEntries &aEntries,
        const HdSceneIndexObserver::DirtiedPrimEntries &bEntries)
{
    // Note that we're turning the entries into maps so that equality doesn't
    // depend on prim order, and we collapse redundant bits.
    using _DirtyMap = std::map<SdfPath, HdDataSourceLocatorSet>;
    _DirtyMap aMap;
    for (auto const& e : aEntries) {
        aMap[e.primPath].insert(e.dirtyLocators);
    }

    _DirtyMap bMap;
    for (auto const& e : bEntries) {
        bMap[e.primPath].insert(e.dirtyLocators);
    }

    return aMap == bMap;
}

static HdDataSourceLocator
_ParseLoc(const std::string & inputStr)
{
    std::vector<TfToken> tokens;

    for (const std::string & s : TfStringSplit(inputStr, "/")) {
        if (!s.empty()) {
            tokens.push_back(TfToken(s));
        }
    }

    return HdDataSourceLocator(tokens.size(), tokens.data());
}

void SetTimeTest()
{
    UsdStageRefPtr stage = UsdStage::Open("varying.usda");
    if (!TF_VERIFY(stage)) {
        return;
    }

    UsdImagingStageSceneIndexRefPtr inputSceneIndex =
        UsdImagingStageSceneIndex::New();
    if (!TF_VERIFY(inputSceneIndex)) {
        return;
    }

    PrimListener primListener;
    inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(&primListener));

    // SetStage should only send a PrimsRemoved and PrimsAdded.
    inputSceneIndex->SetStage(stage);
    TF_VERIFY(primListener.GetDirtied().size() == 0);

    // If we haven't pulled on any data yet, nothing should be variable.
    inputSceneIndex->SetTime(UsdTimeCode(1));
    TF_VERIFY(primListener.GetDirtied().size() == 0);

    // Pull all of the data.
    for (SdfPath const& primPath : primListener.GetPrimPaths()) {
        HdSceneIndexPrim prim = inputSceneIndex->GetPrim(primPath);
        std::stringstream output;
        HdDebugPrintDataSource(output, prim.dataSource);
    }

    // Grab the translate at T == 1 and check it's the right value.
    GfVec3d expected1(0.83975313416116, -0.580522750321479, 7.63364433995336);
    HdMatrixDataSourceHandle xform1 =
        HdMatrixDataSource::Cast(
            HdContainerDataSource::Get(
                inputSceneIndex->GetPrim(SdfPath("/pCube1")).dataSource,
                _ParseLoc("xform/matrix")));
    if (!TF_VERIFY(xform1)) {
        return;
    }
    GfVec3d translate1 = xform1->GetTypedValue(0).ExtractTranslation();
    TF_VERIFY(expected1 == translate1, "%s\n\n...vs...\n\n%s\n",
            TfStringify(expected1).c_str(),
            TfStringify(translate1).c_str());



    inputSceneIndex->SetTime(UsdTimeCode(2));
    HdSceneIndexObserver::DirtiedPrimEntries expectedDirtied = {
        { SdfPath("/pCube1"),
            { _ParseLoc("extent"),
              _ParseLoc("primvars/points/primvarValue"),
              _ParseLoc("visibility"),
              _ParseLoc("xform")
            }
        },
        { SdfPath("/testMaterial"),
            { HdDataSourceLocator(
                TfToken("material"),
                TfToken(),
                TfToken("nodes"),
                TfToken("/testMaterial/Surface"),
                TfToken("parameters"),
                TfToken("emitColor")).Append(TfToken("value"))
            }
        }
    };
    TF_VERIFY(_InvalidationsEqual(primListener.GetDirtied(), expectedDirtied),
            "%s\n...vs...\n\n%s",
            TfStringify(primListener.GetDirtied()).c_str(),
            TfStringify(expectedDirtied).c_str());

    // Grab the translate at T == 2 and check it's the right value.
    GfVec3d expected2(0.83975313416116, -0.580522750321479, 2.76924600182721);
    HdMatrixDataSourceHandle xform2 =
        HdMatrixDataSource::Cast(
            HdContainerDataSource::Get(
                inputSceneIndex->GetPrim(SdfPath("/pCube1")).dataSource,
                _ParseLoc("xform/matrix")));
    if (!TF_VERIFY(xform2)) {
        return;
    }
    GfVec3d translate2 = xform2->GetTypedValue(0).ExtractTranslation();
    TF_VERIFY(expected2 == translate2, "%s\n\n...vs...\n\n%s\n",
            TfStringify(expected2).c_str(),
            TfStringify(translate2).c_str());
}

void PropertyChangeTest()
{
    UsdStageRefPtr stage = UsdStage::Open("varying.usda");
    if (!TF_VERIFY(stage)) {
        return;
    }

    UsdImagingStageSceneIndexRefPtr inputSceneIndex =
        UsdImagingStageSceneIndex::New();
    if (!TF_VERIFY(inputSceneIndex)) {
        return;
    }

    inputSceneIndex->SetStage(stage);

    PrimListener primListener;
    inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(&primListener));


    SdfPath meshPath("/pCube1");
    SdfPath materialPath("/testMaterial");



    UsdPrim surfacePrim = stage->GetPrimAtPath(
        materialPath.AppendChild(TfToken("Surface")));
    if (!TF_VERIFY(surfacePrim)) {
        return;
    }

    UsdAttribute roughnessAttr = surfacePrim.GetAttribute(
        TfToken("inputs:roughness"));
    if (!TF_VERIFY(roughnessAttr)) {
        return;
    }

    roughnessAttr.Set(VtValue(0.25f));

    stage->GetPrimAtPath(meshPath)
        .GetAttribute(TfToken("faceVertexCounts"))
        .Set(VtValue(VtIntArray()));

    stage->GetPrimAtPath(meshPath)
        .GetAttribute(TfToken("points"))
        .Set(VtVec3fArray());

    inputSceneIndex->ApplyPendingUpdates();

    bool materialDirtied = false;
    bool meshTopologyDirtied = false;
    bool meshPointsDirtied = false;

    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry :
            primListener.GetDirtied()) {

        if (entry.primPath == materialPath) {
            if (entry.dirtyLocators.Intersects(
                    HdDataSourceLocator(TfToken("material")))) {
                materialDirtied = true;
            }
        } else if (entry.primPath == meshPath) {
            if (entry.dirtyLocators.Intersects(
                    HdMeshTopologySchema::GetDefaultLocator())) {
                meshTopologyDirtied = true;
            }

            if (entry.dirtyLocators.Intersects(
                    HdPrimvarsSchema::GetPointsLocator())) {
                meshPointsDirtied = true;
            }
        }
    }

    if (!TF_VERIFY(materialDirtied)) {
        return;
    }

    if (!TF_VERIFY(meshTopologyDirtied)) {
        return;
    }

    if (!TF_VERIFY(meshPointsDirtied)) {
        return;
    }
}

void NodeGraphInputChangeTest()
{
    UsdStageRefPtr stage = UsdStage::Open("nodegraph.usda");
    if (!TF_VERIFY(stage)) {
        return;
    }

    UsdImagingStageSceneIndexRefPtr inputSceneIndex =
        UsdImagingStageSceneIndex::New();
    if (!TF_VERIFY(inputSceneIndex)) {
        return;
    }

    inputSceneIndex->SetStage(stage);

    PrimListener primListener;
    inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(&primListener));

    SdfPath materialPath("/World/Material");
    UsdPrim ngPrim = stage->GetPrimAtPath(SdfPath("/World/Material/NodeGraph"));
    if (!TF_VERIFY(ngPrim)) {
        return;
    }

    UsdAttribute diffuseColorAttr =
        ngPrim.GetAttribute(TfToken("inputs:diffuseColor"));
    if (!TF_VERIFY(diffuseColorAttr)) {
        return;
    }

    // Change the NodeGraph's diffuseColor
    diffuseColorAttr.Set(VtValue(GfVec3f(0.0f, 1.0f, 0.0f)));

    inputSceneIndex->ApplyPendingUpdates();

    bool materialDirtied = false;
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry :
            primListener.GetDirtied()) {

        if (entry.primPath == materialPath) {
            if (entry.dirtyLocators.Intersects(
                    HdMaterialSchema::GetDefaultLocator())) {
                materialDirtied = true;
            }
        }
    }

    if (!TF_VERIFY(materialDirtied)) {
        return;
    }
}

void AddNonEmptyLayerTest()
{
    // Create a new stage with a cube at "/cube"
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous(".usda");
    UsdStageRefPtr stage = UsdStage::Open(rootLayer);
    if (!TF_VERIFY(stage)) {
        return;
    }

    UsdGeomCube cube = UsdGeomCube::Define(stage, SdfPath("/cube"));

    // Populate the stage scene index
    UsdImagingStageSceneIndexRefPtr inputSceneIndex =
        UsdImagingStageSceneIndex::New();
    if (!TF_VERIFY(inputSceneIndex)) {
        return;
    }

    inputSceneIndex->SetStage(stage);

    PrimListener primListener;
    inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(&primListener));

    // Create a layer with just an over on "/cube".
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    SdfPrimSpecHandle prim = SdfCreatePrimInLayer(layer, SdfPath("/cube"));
    stage->GetRootLayer()->InsertSubLayerPath(layer->GetIdentifier());

    inputSceneIndex->ApplyPendingUpdates();

    // We expect for "/cube" to be resynced.
    bool cubeResynced = false;
    for (const HdSceneIndexObserver::AddedPrimEntry &entry :
            primListener.GetAdded()) {
        if (entry.primPath == SdfPath("/cube")) {
            cubeResynced = true;
        }
    }

    if (!TF_VERIFY(cubeResynced)) {
        return;
    }
}

int main()
{
    TfErrorMark mark;

    // Ensure that the prim view we get from PrimsAdded matches the view from
    // GetChildPrimPaths/GetPrims.
    TraversalTest();

    // Ensure that calling SetTime() triggers appropriate invalidations;
    // ensure that data values are returned for the correct time.
    SetTimeTest();

    // Ensure that changing a shader parameter results in its enclosing
    // material being dirtied.
    PropertyChangeTest();

    // Ensure that edits made to the nodegraphs result in the enclosing material
    // being dirtied.
    NodeGraphInputChangeTest();

    // Ensure that adding a non-empty layer to the layer stack will trigger the
    // appropriate resyncs.
    AddNonEmptyLayerTest();

    if (TF_VERIFY(mark.IsClean())) {
        std::cout << "OK" << std::endl;
    } else {
        std::cout << "FAILED" << std::endl;
    }
}
