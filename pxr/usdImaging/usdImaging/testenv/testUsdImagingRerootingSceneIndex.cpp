//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/directMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/materialBindingsSchema.h"

#include "pxr/usd/usd/stage.h"

#include "pxr/imaging/hd/collectionSchema.h"
#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/pathExpression.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static std::ostream&
operator<<(std::ostream& out, const SdfPathVector& v)
{
    out << "{" << std::endl;
    for (const SdfPath& path : v) {
        out << path << std::endl;
    }
    out << "}" << std::endl;

    return out;
}

template <typename T>
bool
_CompareValue(const char* msg, const T& v1, const T& v2)
{
    if (v1 == v2) {
        std::cout << msg << " matches." << std::endl;
    }
    else {
        std::cerr << msg << " doesn't match. Expecting '" << v2 << "' got '" << v1 << "'"
                  << std::endl;
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Stage-backed tests, analogous to the ones in testHdPrefixingSceneIndex.

static bool
TestPrimPaths()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->GetRootLayer()->ImportFromString(R"usda(
#usda 1.0

def Scope "World" {
    def Scope "Geo" {
        def Mesh "Cube" {}
    }
}
)usda");

    UsdImagingStageSceneIndexRefPtr stageSi = UsdImagingStageSceneIndex::New();
    stageSi->SetStage(stage);

    UsdImagingRerootingSceneIndexRefPtr rerootingSi =
        UsdImagingRerootingSceneIndex::New(stageSi, SdfPath("/"), SdfPath("/Prefix"));

    // Prims should be visible under the prefixed paths.
    if (!TF_VERIFY(rerootingSi->GetPrim(SdfPath("/Prefix/World")).IsDefined(),
            "Expected /Prefix/World to be defined")) {
        return false;
    }

    if (!_CompareValue("GetPrim /Prefix/World/Geo/Cube type",
            rerootingSi->GetPrim(SdfPath("/Prefix/World/Geo/Cube")).primType,
            HdPrimTypeTokens->mesh)) {
        return false;
    }

    // The un-prefixed paths should return empty prims.
    if (!TF_VERIFY(!rerootingSi->GetPrim(SdfPath("/World")).IsDefined(),
            "Expected /World to be undefined after rerooting")) {
        return false;
    }

    // GetChildPrimPaths from root should yield the prefix root.
    if (!_CompareValue("GetChildPrimPaths('/')",
            rerootingSi->GetChildPrimPaths(SdfPath("/")),
            SdfPathVector({SdfPath("/Prefix")}))) {
        return false;
    }

    if (!_CompareValue("GetChildPrimPaths('/Prefix')",
            rerootingSi->GetChildPrimPaths(SdfPath("/Prefix")),
            SdfPathVector({SdfPath("/Prefix/World")}))) {
        return false;
    }

    if (!_CompareValue("GetChildPrimPaths('/Prefix/World')",
            rerootingSi->GetChildPrimPaths(SdfPath("/Prefix/World")),
            SdfPathVector({SdfPath("/Prefix/World/Geo")}))) {
        return false;
    }

    if (!_CompareValue("GetChildPrimPaths('/Prefix/World/Geo')",
            rerootingSi->GetChildPrimPaths(SdfPath("/Prefix/World/Geo")),
            SdfPathVector({SdfPath("/Prefix/World/Geo/Cube")}))) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

static bool
TestPathDataSources()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->GetRootLayer()->ImportFromString(R"usda(
#usda 1.0

def Mesh "A" (
    prepend apiSchemas = ["MaterialBindingAPI"]
) {
    rel material:binding = </A/B>

    def Material "B" {}
}
)usda");

    UsdImagingStageSceneIndexRefPtr stageSi = UsdImagingStageSceneIndex::New();
    stageSi->SetStage(stage);

    UsdImagingRerootingSceneIndexRefPtr rerootingSi =
        UsdImagingRerootingSceneIndex::New(
            stageSi, SdfPath("/"), SdfPath("/My/Prefix"));

    // The material binding path should be remapped.
    HdSceneIndexPrim prim = rerootingSi->GetPrim(SdfPath("/My/Prefix/A"));
    UsdImagingMaterialBindingsSchema bindingsSchema =
        UsdImagingMaterialBindingsSchema::GetFromParent(prim.dataSource);
    if (!TF_VERIFY(bindingsSchema.IsDefined(),
            "Expected material bindings on /My/Prefix/A")) {
        return false;
    }

    UsdImagingMaterialBindingVectorSchema bindingVec =
        bindingsSchema.GetMaterialBindings();
    if (!TF_VERIFY(bindingVec.IsDefined() && bindingVec.GetNumElements() == 1,
            "Expected one material binding entry")) {
        return false;
    }

    HdPathDataSourceHandle matPathDs =
        bindingVec.GetElement(0).GetDirectMaterialBinding().GetMaterialPath();
    if (!TF_VERIFY(matPathDs, "Expected material path data source")) {
        return false;
    }

    // Absolute path should be remapped.
    if (!_CompareValue("material binding path",
            matPathDs->GetTypedValue(0.0f),
            SdfPath("/My/Prefix/A/B"))) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

static bool
TestPathExpressionDataSources()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->GetRootLayer()->ImportFromString(R"usda(
#usda 1.0

def Scope "Chars" (
    prepend apiSchemas = [
        "CollectionAPI:trivialExpr", "CollectionAPI:simpleExpr",
        "CollectionAPI:complexExpr1", "CollectionAPI:complexExpr2"]
) {
    pathExpression collection:trivialExpr:membershipExpression = "~//*.*"
    pathExpression collection:simpleExpr:membershipExpression = "/Chars/Hero"
    pathExpression collection:complexExpr1:membershipExpression = "/Chars//"
    pathExpression collection:complexExpr2:membershipExpression = "//Foo/Bar"
}
)usda");

    UsdImagingStageSceneIndexRefPtr stageSi = UsdImagingStageSceneIndex::New();
    stageSi->SetStage(stage);

    UsdImagingRerootingSceneIndexRefPtr rerootingSi =
        UsdImagingRerootingSceneIndex::New(
            stageSi, SdfPath("/"), SdfPath("/My/Prefix"));

    HdSceneIndexPrim prim =
        rerootingSi->GetPrim(SdfPath("/My/Prefix/Chars"));
    HdCollectionsSchema collectionsSchema =
        HdCollectionsSchema::GetFromParent(prim.dataSource);
    if (!TF_VERIFY(collectionsSchema.IsDefined(),
            "Expected collections on /My/Prefix/Chars")) {
        return false;
    }

    // Trivial expression should be left as-is.
    if (!_CompareValue("trivial expression ~//*.*",
            collectionsSchema.GetCollection(TfToken("trivialExpr"))
                .GetMembershipExpression()->GetTypedValue(0.0f),
            SdfPathExpression("~//*.*"))) {
        return false;
    }

    // A simple path expression should be prefixed.
    if (!_CompareValue("simple path expression /Chars/Hero",
            collectionsSchema.GetCollection(TfToken("simpleExpr"))
                .GetMembershipExpression()->GetTypedValue(0.0f),
            SdfPathExpression("/My/Prefix/Chars/Hero"))) {
        return false;
    }

    // Complex path expressions should also be prefixed.
    if (!_CompareValue("complex path expression /Chars//",
            collectionsSchema.GetCollection(TfToken("complexExpr1"))
                .GetMembershipExpression()->GetTypedValue(0.0f),
            SdfPathExpression("/My/Prefix/Chars//"))) {
        return false;
    }

    if (!_CompareValue("complex path expression //Foo/Bar",
            collectionsSchema.GetCollection(TfToken("complexExpr2"))
                .GetMembershipExpression()->GetTypedValue(0.0f),
            SdfPathExpression("/My/Prefix//Foo/Bar"))) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

#define str(s) #s
#define TEST(X)                                                                \
    std::cout << (++i) << ") " << str(X) << "..." << std::endl;                \
    if (!X()) {                                                                \
        std::cout << "FAILED" << std::endl;                                    \
        return -1;                                                             \
    }                                                                          \
    else                                                                       \
        std::cout << "...SUCCEEDED" << std::endl;

int
main(int argc, char** argv)
{
    //-------------------------------------------------------------------------
    std::cout << "STARTING testUsdImagingRerootingSceneIndex" << std::endl;

    int i = 0;
    // XXX Add a test case where the UsdImagingRerootingSceneIndex is not
    // used purely as a prefixing scene index.
    TEST(TestPrimPaths);
    TEST(TestPathDataSources);
    TEST(TestPathExpressionDataSources);

    //--------------------------------------------------------------------------
    std::cout << "DONE testUsdImagingRerootingSceneIndex" << std::endl;
    return 0;
}
