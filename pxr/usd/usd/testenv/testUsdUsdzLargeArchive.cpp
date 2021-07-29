//
// Copyright 2021 Pixar
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

#include "pxr/pxr.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <ctime>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE;

static bool
AttributesEqual(UsdAttribute a, UsdAttribute b)
{
    if (a.IsValid() != b.IsValid()) {
        return false;
    }
    if (!a.IsValid()) {
        return true;
    }
    VtValue va, vb;
    if (!a.Get(&va) || !b.Get(&vb)) {
        return false;
    }
    return va == vb;
}

// Test that calling ArResolver::OpenAsset on a file within a .usdz
// file produces the expected result.
static void
TestOpenLargeArchive()
{
    std::cout << "TestOpenLargeArchive...";
    const std::string usdzFile("test.usdz");
    const auto startTime = std::clock();
    auto stage = UsdStage::Open(usdzFile);
    const auto endTime = std::clock();

    // Testing the time to open is possibly not the greatest, but the file in question
    // took over 15 seconds without caching, and less than 2.5 seconds with;
    // So conservatively, we should be able to open it in 5 seconds in processor time ?
    //
    const auto elapsedTime = float(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "stage creation took " << elapsedTime << std::endl;

    if (!stage) {
        TF_FATAL_ERROR("Failed to load stage at '%s'", usdzFile.c_str());
    }
    if (elapsedTime > 5.f) {
        TF_FATAL_ERROR("Open of '%s' took %f seconds in proc time", usdzFile.c_str(), elapsedTime);
    }

    UsdPrim scene = stage->GetPrimAtPath(SdfPath("/scene"));
    TF_AXIOM(scene);

    UsdGeomMesh baseMesh;
    size_t numChildren = 0;
    for (UsdPrim child : scene.GetAllChildren()) {
        TF_AXIOM(child.IsA<UsdGeomMesh>());

        UsdGeomMesh mesh(child);
        if (baseMesh) {
            TF_AXIOM(AttributesEqual(baseMesh.GetFaceVertexCountsAttr(), mesh.GetFaceVertexCountsAttr()));
            TF_AXIOM(AttributesEqual(baseMesh.GetFaceVertexIndicesAttr(), mesh.GetFaceVertexIndicesAttr()));
            TF_AXIOM(AttributesEqual(baseMesh.GetPointsAttr(), mesh.GetPointsAttr()));
            TF_AXIOM(AttributesEqual(baseMesh.GetSubdivisionSchemeAttr(), mesh.GetSubdivisionSchemeAttr()));
        }
        else {
            baseMesh = std::move(mesh);
        }
        ++numChildren;
    }
    TF_AXIOM(numChildren == 25000);
}

int main(int argc, char** argv)
{
    TestOpenLargeArchive();

    std::cout << "Passed!" << std::endl;

    return EXIT_SUCCESS;
}
