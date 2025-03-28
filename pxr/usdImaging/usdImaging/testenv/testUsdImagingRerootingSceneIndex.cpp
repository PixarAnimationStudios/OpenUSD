//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/base/vt/array.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

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
std::vector<T>
_Sorted(std::vector<T> v)
{
    std::sort(v.begin(), v.end());
    return v;
}

void
_PrintContainer(
    HdContainerDataSourceHandle container,
    const std::string& prefix = std::string())
{
    if (!container) {
        return;
    }

    for (const TfToken& name : container->GetNames()) {

        HdDataSourceBaseHandle childSource = container->Get(name);
        if (!childSource) {
            std::cout << prefix << "(@" << name << ")" << std::endl;
            continue;
        }

        std::cout << prefix << "@" << name << ": ";

        if (HdContainerDataSourceHandle childContainer
            = HdContainerDataSource::Cast(childSource)) {
            std::cout << std::endl;
            _PrintContainer(childContainer, prefix + "  ");
        }
        else if (
            HdSampledDataSourceHandle sampledChild
            = HdSampledDataSource::Cast(childSource)) {
            std::cout << sampledChild->GetValue(0.0f) << std::endl;
        }
        else {
            std::cout << "(unknown)" << std::endl;
        }
    }
}

void
_PrintSceneIndexPrim(
    HdSceneIndexBase& sceneIndex,
    const SdfPath& primPath,
    bool includeChildren,
    const std::string& prefix = std::string())
{
    HdSceneIndexPrim prim = sceneIndex.GetPrim(primPath);
    std::cout << prefix << primPath << " (" << prim.primType << ")"
              << std::endl;
    _PrintContainer(prim.dataSource, prefix + "  ");

    if (!includeChildren) {
        return;
    }

    for (const SdfPath& childPath : sceneIndex.GetChildPrimPaths(primPath)) {
        _PrintSceneIndexPrim(sceneIndex, childPath, true, prefix);
    }
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
HdDataSourceBaseHandle
_GetDataSourceFromScene(
    HdSceneIndexBase& sceneIndex,
    const SdfPath& primPath,
    const HdDataSourceLocator& locator)
{
    HdSceneIndexPrim prim = sceneIndex.GetPrim(primPath);
    return HdContainerDataSource::Get(prim.dataSource, locator);
}

template <typename T>
T
_GetTypedValueFromScene(
    HdSceneIndexBase& sceneIndex,
    const SdfPath& primPath,
    const HdDataSourceLocator& locator)
{
    auto dataSource = HdTypedSampledDataSource<T>::Cast(
        _GetDataSourceFromScene(sceneIndex, primPath, locator));

    if (!dataSource) {
        return T();
    }

    return dataSource->GetTypedValue(0.0f);
}

//-----------------------------------------------------------------------------
// UsdImagingRerootingSceneIndex

static bool
TestRerootingSceneIndex()
{
    // /A/foo
    // /path/B/bar
    // /path/B/X/baz
    HdRetainedSceneIndexRefPtr sceneIndex_ = HdRetainedSceneIndex::New();
    HdRetainedSceneIndex& sceneIndex = *sceneIndex_;
    sceneIndex.AddPrims({ { SdfPath("/A"), TfToken("huh"), nullptr } });
    sceneIndex.AddPrims({ { SdfPath("/path"), TfToken("huh"), nullptr } });
    sceneIndex.AddPrims({ { SdfPath("/path/B"), TfToken("huh"), nullptr } });

    sceneIndex.AddPrims({
        { SdfPath("/A/foo"), TfToken("huh"),
          HdRetainedContainerDataSource::New(
              TfToken("pathToA"),
              HdRetainedTypedSampledDataSource<SdfPath>::New(SdfPath("/A")),
              TfToken("pathToB"),
              HdRetainedTypedSampledDataSource<SdfPath>::New(
                  SdfPath("/path/B")),
              TfToken("pathArray"),
              HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                  { SdfPath("/A"), SdfPath("/path/B") }),
              TfToken("relPathToB"),
              HdRetainedTypedSampledDataSource<SdfPath>::New(
                  SdfPath("../../path/B"))) },
    });

    std::cout << "\n-- SCENE -----------------------" << std::endl;
    _PrintSceneIndexPrim(sceneIndex, SdfPath("/"), true);

    std::cout << "\n-- REROOTED /A -> /path/B SCENE --------------"
              << std::endl;
    UsdImagingRerootingSceneIndexRefPtr rerootAToB_
        = UsdImagingRerootingSceneIndex::New(
            sceneIndex_, SdfPath("/A"), SdfPath("/path/B"));
    UsdImagingRerootingSceneIndex& rerootAToB = *rerootAToB_;
    _PrintSceneIndexPrim(rerootAToB, SdfPath("/"), true);

    std::cout << "\n-- REROOTED /A -> / SCENE --------------" << std::endl;
    UsdImagingRerootingSceneIndexRefPtr rerootAToRoot_
        = UsdImagingRerootingSceneIndex::New(
            sceneIndex_, SdfPath("/A"), SdfPath("/"));
    UsdImagingRerootingSceneIndex& rerootAToRoot = *rerootAToRoot_;
    _PrintSceneIndexPrim(rerootAToRoot, SdfPath("/"), true);

    std::cout << "\n-- REROOTED / -> /Some/Prefix SCENE --------------"
              << std::endl;
    UsdImagingRerootingSceneIndexRefPtr prefixViaReroot_
        = UsdImagingRerootingSceneIndex::New(
            sceneIndex_, SdfPath("/"), SdfPath("/Some/Prefix"));
    UsdImagingRerootingSceneIndex& prefixViaReroot = *prefixViaReroot_;
    _PrintSceneIndexPrim(prefixViaReroot, SdfPath("/"), true);

    // Make sure that we replace everything at toPath.  This means /B/bar will
    // be gone.  Also, the fromPath (/A) is removed.
    if (!_CompareValue(
            "TESTING TO_PATH REPLACED",
            _Sorted(rerootAToB.GetChildPrimPaths(SdfPath("/path/B"))),
            _Sorted(SdfPathVector({
                SdfPath("/path/B/foo"),
            })))) {
        return false;
    }
    if (!_CompareValue(
            "TESTING ALL OTHER PATHS REMOVED",
            _Sorted(rerootAToB.GetChildPrimPaths(SdfPath("/"))),
            _Sorted(SdfPathVector({
                SdfPath("/path"),
            })))) {
        return false;
    }

    // The instancing code is relying on this behavior for material
    // bindings (which target prims outside of the "src").
    if (!_CompareValue(
            "COMPARING PATH TO NON REROOTED",
            _GetTypedValueFromScene<SdfPath>(
                rerootAToB, 
                SdfPath("/path/B/foo"),
                HdDataSourceLocator(TfToken("pathToB"))),
            SdfPath("/path/B"))) {
        return false;
    }
    if (!_CompareValue(
            "COMPARING PATH ARRAY",
            _GetTypedValueFromScene<VtArray<SdfPath>>(
                rerootAToB, 
                SdfPath("/path/B/foo"),
                HdDataSourceLocator(TfToken("pathArray"))),
            VtArray<SdfPath> { SdfPath("/path/B"), SdfPath("/path/B") })) {
        return false;
    }

    // Make sure that if we move to /, it replaces everything
    if (!_CompareValue(
            "TESTING MOVING TO /",
            _Sorted(rerootAToRoot.GetChildPrimPaths(SdfPath("/"))),
            _Sorted(SdfPathVector({
                SdfPath("/foo"),
            })))) {
        return false;
    }

    // Make sure that this can work as a prefixer
    if (!_CompareValue(
            "TESTING USING AS PREFIX (children of /)",
            _Sorted(prefixViaReroot.GetChildPrimPaths(SdfPath("/"))),
            _Sorted(SdfPathVector({
                SdfPath("/Some"),
            })))) {
        return false;
    }
    if (!_CompareValue(
            "TESTING USING AS PREFIX (children of /Some)",
            _Sorted(prefixViaReroot.GetChildPrimPaths(SdfPath("/"))),
            _Sorted(SdfPathVector({
                SdfPath("/Some"),
            })))) {
        return false;
    }
    if (!_CompareValue(
            "TESTING USING AS PREFIX (children of /Some/Prefix)",
            _Sorted(prefixViaReroot.GetChildPrimPaths(SdfPath("/Some/Prefix"))),
            _Sorted(SdfPathVector({
                SdfPath("/Some/Prefix/A"),
                SdfPath("/Some/Prefix/path"),
            })))) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

#define xstr(s) str(s)
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
    std::cout << "STARTING testUsdUnderlayRerootingSceneIndex" << std::endl;

    int i = 0;
    TEST(TestRerootingSceneIndex);

    //--------------------------------------------------------------------------
    std::cout << "DONE testUsdUnderlayRerootingSceneIndex" << std::endl;
    return 0;
}
