//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include <iostream>

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/prefixingSceneIndex.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"


PXR_NAMESPACE_USING_DIRECTIVE

// ----------------------------------------------------------------------------

namespace {

std::ostream &operator<<(std::ostream &os, const SdfPathVector &paths) {
    for (const auto &p : paths) {
        os << p << " ";
    }
    return os;
}

template<typename T>
bool
_CompareValue(const char *msg, const T &v1, const T &v2)
{
    if (v1 == v2) {
        std::cout << msg << " matches." << std::endl;
    } else {
        std::cerr << msg << " doesn't match. Expecting " << v2
                  << " got " << v1 << std::endl;
        return false;
    }
    return true;
}

template <typename T>
T
_GetTypedValue(
    const HdSceneIndexBaseRefPtr &sceneIndex,
    const SdfPath &primPath,
    const HdDataSourceLocator &locator)
{
    const HdSceneIndexPrim prim = sceneIndex->GetPrim(primPath);
    const HdDataSourceBaseHandle ds = 
        HdContainerDataSource::Get(prim.dataSource, locator);

    auto typedDs = HdTypedSampledDataSource<T>::Cast(ds);
    if (!typedDs) {
        return T();
    }
    return typedDs->GetTypedValue(0.0f);
}

HdContainerDataSourceHandle
_MakeEmptyContainer()
{
    static auto empty = HdRetainedContainerDataSource::New();
    return empty;
}

}

// ----------------------------------------------------------------------------

bool
TestPrimPaths()
{
    HdRetainedSceneIndexRefPtr inputSi = HdRetainedSceneIndex::New();
    const auto &emptyDs = _MakeEmptyContainer();

    inputSi->AddPrims({{SdfPath("/World"), TfToken("scope"), emptyDs}});
    inputSi->AddPrims({{SdfPath("/World/Geo"), TfToken("scope"), emptyDs}});
    inputSi->AddPrims({{SdfPath("/World/Geo/Cube"), TfToken("mesh"), emptyDs}});

    HdPrefixingSceneIndexRefPtr prefixingSi =
        HdPrefixingSceneIndex::New(inputSi, SdfPath("/Prefix"));

    // Prims should be visible under the prefixed paths.
    if (!_CompareValue("GetPrim /Prefix/World type",
            prefixingSi->GetPrim(SdfPath("/Prefix/World")).primType,
            TfToken("scope"))) {
        return false;
    }

    if (!_CompareValue("GetPrim /Prefix/World/Geo/Cube type",
            prefixingSi->GetPrim(SdfPath("/Prefix/World/Geo/Cube")).primType,
            TfToken("mesh"))) {
        return false;
    }

    // The un-prefixed paths should return empty prims.
    if (!TF_VERIFY(!prefixingSi->GetPrim(SdfPath("/World")).IsDefined())) {
        return false;
    }

    // GetChildPrimPaths from root should yield the prefix root.
    if (!_CompareValue("GetChildPrimPaths('/')",
            prefixingSi->GetChildPrimPaths(SdfPath("/")),
            SdfPathVector({SdfPath("/Prefix")}))) {
        return false;
    }

    if (!_CompareValue("GetChildPrimPaths('/Prefix')",
            prefixingSi->GetChildPrimPaths(SdfPath("/Prefix")),
            SdfPathVector({SdfPath("/Prefix/World")}))) {
        return false;
    }

    if (!_CompareValue("GetChildPrimPaths('/Prefix/World')",
            prefixingSi->GetChildPrimPaths(SdfPath("/Prefix/World")),
            SdfPathVector({SdfPath("/Prefix/World/Geo")}))) {
        return false;
    }

    if (!_CompareValue("GetChildPrimPaths('/Prefix/World/Geo')",
            prefixingSi->GetChildPrimPaths(SdfPath("/Prefix/World/Geo")),
            SdfPathVector({SdfPath("/Prefix/World/Geo/Cube")}))) {
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------

bool
TestPathDataSources()
{
    HdRetainedSceneIndexRefPtr inputSi = HdRetainedSceneIndex::New();
    const auto &emptyDs = _MakeEmptyContainer();

    inputSi->AddPrims({{SdfPath("/A"), TfToken("scope"),
        HdRetainedContainerDataSource::New(
            TfToken("target"),
            HdRetainedTypedSampledDataSource<SdfPath>::New(SdfPath("/A/B")),
            TfToken("targets"),
            HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                {SdfPath("/A/B"), SdfPath("/A/B/C")})
        )
    }});
    inputSi->AddPrims({{SdfPath("/A/B"), TfToken("mesh"), emptyDs}});
    inputSi->AddPrims({{SdfPath("/A/B/C"), TfToken("mesh"), emptyDs}});

    HdPrefixingSceneIndexRefPtr prefixingSi =
        HdPrefixingSceneIndex::New(inputSi, SdfPath("/My/Prefix"));

    // Absolute path should be prefixed.
    if (!_CompareValue("absolute path target",
            _GetTypedValue<SdfPath>(
                prefixingSi,
                SdfPath("/My/Prefix/A"),
                HdDataSourceLocator(TfToken("target"))),
            SdfPath("/My/Prefix/A/B"))) {
        return false;
    }

    // All paths in a path array should be prefixed.
    if (!_CompareValue("path array targets",
            _GetTypedValue<VtArray<SdfPath>>(
                prefixingSi,
                SdfPath("/My/Prefix/A"),
                HdDataSourceLocator(TfToken("targets"))),
            VtArray<SdfPath>{
                SdfPath("/My/Prefix/A/B"),
                SdfPath("/My/Prefix/A/B/C")})) {
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------

bool
TestPathExpressionDataSources()
{
    HdRetainedSceneIndexRefPtr inputSi = HdRetainedSceneIndex::New();
    const auto &emptyDs = _MakeEmptyContainer();

    inputSi->AddPrims({{SdfPath("/Chars"), TfToken("scope"),
        HdRetainedContainerDataSource::New(
            // Trivial expressions: should be left as-is
            TfToken("trivialExpr1"),
            HdRetainedTypedSampledDataSource<SdfPathExpression>::New(
                SdfPathExpression::Nothing()),
            TfToken("trivialExpr2"),
            HdRetainedTypedSampledDataSource<SdfPathExpression>::New(
                SdfPathExpression::Everything()),
            TfToken("trivialExpr3"),
            HdRetainedTypedSampledDataSource<SdfPathExpression>::New(
                SdfPathExpression("~//*.*")), // everything but properties

            // Expression that is simply a path.
            TfToken("simpleExpr"),
            HdRetainedTypedSampledDataSource<SdfPathExpression>::New(
                SdfPathExpression("/Chars/Hero")),

            // Expression with `//` indicating arbitrary levels of hierarchy.
            TfToken("complexExpr1"),
            HdRetainedTypedSampledDataSource<SdfPathExpression>::New(
                SdfPathExpression("/Chars//")),
            TfToken("complexExpr2"),
            HdRetainedTypedSampledDataSource<SdfPathExpression>::New(
                SdfPathExpression("//Foo/Bar")))
    }});

    HdPrefixingSceneIndexRefPtr prefixingSi =
        HdPrefixingSceneIndex::New(inputSi, SdfPath("/My/Prefix"));

    // Trivial path expression should be left as-is.
    for (const auto &token : {TfToken("trivialExpr1"), TfToken("trivialExpr2"),
            TfToken("trivialExpr3")}) {
        if (!_CompareValue("trivial path expression",
                _GetTypedValue<SdfPathExpression>(
                    prefixingSi,
                    SdfPath("/My/Prefix/Chars"),
                    HdDataSourceLocator(token)),
                _GetTypedValue<SdfPathExpression>(
                    inputSi,
                    SdfPath("/Chars"),
                    HdDataSourceLocator(token)))) {
            return false;
        }
    }

    // A path expression housing a simple path should be prefixed.
    if (!_CompareValue("simple path expression",
            _GetTypedValue<SdfPathExpression>(
                prefixingSi,
                SdfPath("/My/Prefix/Chars"),
                HdDataSourceLocator(TfToken("simpleExpr"))),
            SdfPathExpression("/My/Prefix/Chars/Hero"))) {
        return false;
    }

    // A complex path expression with trailing `//` should also be prefixed.
    if (!_CompareValue("complex path expression",
            _GetTypedValue<SdfPathExpression>(
                prefixingSi,
                SdfPath("/My/Prefix/Chars"),
                HdDataSourceLocator(TfToken("complexExpr1"))),
            SdfPathExpression("/My/Prefix/Chars//"))) {
        return false;
    }

    if (!_CompareValue("complex path expression",
            _GetTypedValue<SdfPathExpression>(
                prefixingSi,
                SdfPath("/My/Prefix/Chars"),
                HdDataSourceLocator(TfToken("complexExpr2"))),
            SdfPathExpression("/My/Prefix//Foo/Bar"))) {
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------

#define TEST(X) \
    std::cout << (++i) << ") " << #X << "..." << std::endl; \
    if (!X()) { std::cout << "FAILED" << std::endl; return -1; } \
    else std::cout << "...SUCCEEDED" << std::endl;

int
main(int argc, char **argv)
{
    std::cout << "STARTING testHdPrefixingSceneIndex" << std::endl;

    int i = 0;
    TEST(TestPrimPaths);
    TEST(TestPathDataSources);
    TEST(TestPathExpressionDataSources);

    std::cout << std::endl << "OK" << std::endl;
    return 0;
}
