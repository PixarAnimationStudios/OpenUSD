// Test that a simple USD program can open a stage without initializing
// Python.

#include <Python.h>

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/variantSets.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/stringUtils.h"

#include <algorithm>
#include <cstdio>

// Unfortunately, this test wants ensure that Python is not required for
// simple Usd clients.  However, FindDataFile is only provided in Python.
// Other C++ tests that use it, do so by invoking Python.  This function
// implements a subset of the real FindDataFile's behavior.
static std::string
_FindDataFile(const std::string &testRelPath)
{
    // Mentor provides a colon-separated list in the MENTOR_SEARCH_PATH
    // environment variable where we can look for test assets.
    const std::string searchPath = TfGetenv("MENTOR_SEARCH_PATH");
    // The mentor "full" test name is its path relative to the build
    // component's installation root.
    const std::string fullTestName = TfGetenv("MENTOR_TEST_FULLNAME");
    if (searchPath.empty()) {
        TF_FATAL_ERROR("Could not determine test search path");
    }
    if (fullTestName.empty()) {
        TF_FATAL_ERROR("Could not determine test path");
    }

    std::string::const_iterator i = searchPath.begin();
    std::string::const_iterator j = i;
    for (; j != searchPath.end(); i=j+1) {
        j = std::find(i, searchPath.end(), ':');

        std::string possiblePath(i, j);
        possiblePath = TfStringCatPaths(possiblePath, fullTestName);
        possiblePath = TfStringCatPaths(possiblePath, testRelPath);

        if (TfPathExists(possiblePath)) {
            return possiblePath;
        }
    }

    TF_FATAL_ERROR("Could not locate test asset: %s", testRelPath.c_str());
    return std::string();
}

static void
_OpenAndExport(const std::string &assetPath)
{
    printf("Opening stage: %s\n", assetPath.c_str());

    UsdStageRefPtr stage = UsdStage::Open(assetPath);
    TF_AXIOM(stage);

    printf("============= Begin Stage Dump =============\n");
    std::string flattened;
    bool exportSucceeded = stage->ExportToString(&flattened);
    TF_AXIOM(exportSucceeded);
    printf("%s", flattened.c_str());
    printf("============= End Stage Dump  =============\n");

    TF_AXIOM(not Py_IsInitialized());
}

int
main(int argc, const char *argv[])
{
    // Sanity check that no libraries have a static initializer that is
    // initializing Python.
    TF_AXIOM(not Py_IsInitialized());

    UsdStageRefPtr emptyStage = UsdStage::CreateInMemory();
    TF_AXIOM(not Py_IsInitialized());

    std::string asciiFilePath = _FindDataFile(
        "testUsdStageNoPython.testenv/ascii.usd");
    _OpenAndExport(asciiFilePath);

    std::string binaryFilePath = _FindDataFile(
        "testUsdStageNoPython.testenv/binary.usd");
    _OpenAndExport(binaryFilePath);

    return 0;
}
