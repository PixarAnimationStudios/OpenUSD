//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hd/sortedIds.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/errorMark.h"
#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <mutex>

PXR_NAMESPACE_USING_DIRECTIVE;

static char firstLevelChar[] =
{
    'A',
    'B',
    'Y',
    'Z'
};
static const size_t numFirstLevel = sizeof(firstLevelChar) /
                                    sizeof(firstLevelChar[0]);

static SdfPathVector populatePaths;

static
void
_InitPaths()
{
    char primName[] = "/_/_";

    populatePaths.reserve(numFirstLevel * ('Z' - 'A' + 1));
    for (size_t firstLevel = 0; firstLevel < numFirstLevel; ++firstLevel) {
        primName[1] = firstLevelChar[firstLevel];
        for (char secondLevel = 'A'; secondLevel <= 'Z'; ++secondLevel) {
            primName[3] = secondLevel;

            populatePaths.push_back(SdfPath(primName));
        }
    }

    // Shuffle paths randomly.
    const size_t seed =
        std::chrono::system_clock::now().time_since_epoch().count();
    std::cout << "Random seed: " << seed << "\n";
    std::mt19937 randomGen(seed);

    std::shuffle(populatePaths.begin(), populatePaths.end(), randomGen);

    std::cout << "Inital Path Set:\n";
    std::copy(populatePaths.begin(),
              populatePaths.end(),
              std::ostream_iterator<SdfPath>(std::cout, "\n"));
}

static
void
_Populate(Hd_SortedIds *sortedIds)
{
    static std::once_flag initPaths;
    std::call_once(initPaths, _InitPaths);

    size_t numPaths = populatePaths.size();
    for (size_t pathNum = 0; pathNum < numPaths; ++pathNum) {
        sortedIds->Insert(populatePaths[pathNum]);
    }
    sortedIds->GetIds(); // Make sure list get sorted
}

static
void
_Dump(Hd_SortedIds *sortedIds, const char *filename)
{
    std::ofstream outputStream(filename);

    const SdfPathVector &ids = sortedIds->GetIds();

    std::copy(ids.begin(),
              ids.end(),
              std::ostream_iterator<SdfPath>(outputStream, "\n"));
}


static
bool
PopulateTest()
{
    std::cout << "\n\nPopulateTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);
    _Dump(&sortedIds, "testHdSortedId_populateTest.txt");

    return true;
}

static
bool
SingleInsertTest()
{
    std::cout << "\n\nSingleInsertTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);
    sortedIds.GetIds(); // Trigger Inital Sort

    sortedIds.Insert(SdfPath("/I/J"));

    _Dump(&sortedIds, "testHdSortedId_singleInsertTest.txt");

    return true;
}

static
bool
MultiInsertTest()
{
    std::cout << "\n\nMultiInsertTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);
    sortedIds.GetIds(); // Trigger Inital Sort

    char primName[] = "/I/_";

    SdfPathVector insertPaths;
    insertPaths.reserve( ('Z' - 'A' + 1));
    for (char pathChar = 'A'; pathChar <= 'Z'; ++pathChar) {
        primName[3] = pathChar;

        insertPaths.push_back(SdfPath(primName));
    }

    std::mt19937 randomGen(0);

    std::shuffle(insertPaths.begin(), insertPaths.end(), randomGen);

    std::cout << "Insert Set:\n";
    std::copy(insertPaths.begin(),
            insertPaths.end(),
              std::ostream_iterator<SdfPath>(std::cout, "\n"));


    size_t numPaths = insertPaths.size();
    for (size_t pathNum = 0; pathNum < numPaths; ++pathNum) {
        sortedIds.Insert(insertPaths[pathNum]);
    }

    _Dump(&sortedIds, "testHdSortedId_multiInsertTest.txt");

    return true;
}

static
bool
RemoveTest()
{
    std::cout << "\n\nRemoveTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);

    SdfPathVector removedIds;

    std::cout << "Remove Set:\n";
    for (size_t pathNum = 10; pathNum < 20; ++pathNum) {
        const SdfPath& removedId = populatePaths[pathNum];
        std::cout << removedId << "\n";
        sortedIds.Remove(removedId);
        removedIds.push_back(removedId);
    }

    const SdfPathVector& sortedIdsVector = sortedIds.GetIds();

    // Veryify sortedIds are still sorted.
    TF_VERIFY(std::is_sorted(sortedIdsVector.cbegin(), sortedIdsVector.cend()));
    // Verify size of sortedIds.
    TF_VERIFY(sortedIdsVector.size() == populatePaths.size() - 10);
    // Verify correct ids were removed.
    for (const SdfPath& removedId : removedIds) {
        TF_VERIFY(std::find(sortedIdsVector.begin(), sortedIdsVector.end(),
            removedId) == sortedIdsVector.end());
    }

    return true;
}

static
bool
RemoveOnlyElementTest()
{
    std::cout << "\n\nRemoveOneElementTest():\n";

    Hd_SortedIds sortedIds;

    sortedIds.Insert(populatePaths[0]);
    sortedIds.GetIds();
    sortedIds.Remove(populatePaths[0]);

    _Dump(&sortedIds, "testHdSortedId_removeOnlyElementTest.txt");

    return true;
}

static
bool
RemoveRangeTest()
{
    std::cout << "\n\nRemoveRangeTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);

    // Delete the B subtree
    const SdfPathVector &ids = sortedIds.GetIds();
    size_t rangeStart = std::lower_bound(ids.begin(),
                                         ids.end(),
                                         SdfPath("/B")) - ids.begin();

    size_t rangeEnd  = std::lower_bound(ids.begin(),
                                        ids.end(),
                                        SdfPath("/C")) - ids.begin();
    // SortedId's ranges are inclusive [begin, end] , but the above code
    // will return the first element past the end [begin, end)
    --rangeEnd;

    std::cout << "Removing Range "
              << rangeStart << "(" << ids[rangeStart] << ") - "
              << rangeEnd   << "(" << ids[rangeEnd]   << ")\n";

    sortedIds.RemoveRange(rangeStart, rangeEnd);

    _Dump(&sortedIds, "testHdSortedId_removeRangeTest.txt");

    return true;
}

static
bool
RemoveBatchTest()
{
    std::cout << "\n\nRemoveBatchTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);

    // Try to hit the batching operation by removing the N subtree
    // As this is correctness and not performance, it doesn't verify the
    // Optimization is actually hit, but more try to target the
    // external behavior that should trigger the optimization.
    char primName[] = "/Y/_";

    for (char pathChar = 'A'; pathChar <= 'Z'; ++pathChar) {
        primName[3] = pathChar;

        sortedIds.Remove(SdfPath(primName));
    }

    _Dump(&sortedIds, "testHdSortedId_removeBatchTest.txt");

    return true;
}

static
bool
RemoveSortedTest()
{
    std::cout << "\n\nRemoveSortedTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);

    // Continuously remove prims that should be in the sorted bucket
    char primName[] = "/_/_";

    for (size_t charNum = numFirstLevel; charNum > 0; --charNum) {
        char pathChar = firstLevelChar[charNum - 1];

        primName[1] = pathChar;
        primName[3] = pathChar;

        sortedIds.Remove(SdfPath(primName));
    }


    _Dump(&sortedIds, "testHdSortedId_removeSortedTest.txt");

    return true;
}

static
bool
RemoveUnsortedTest()
{
    std::cout << "\n\nRemoveUnsortedTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);

    // Continuously remove prims that should be in the unsorted bucket
    char primName[] = "/_/_";

    for (size_t charNum = 0; charNum < numFirstLevel; ++charNum) {
        char pathChar = firstLevelChar[charNum];

        primName[1] = pathChar;
        primName[3] = pathChar;

        sortedIds.Remove(SdfPath(primName));
    }


    _Dump(&sortedIds, "testHdSortedId_removeUnsortedTest.txt");

    return true;
}

static
bool
RemoveAfterInsertNoSync()
{
    std::cout << "\n\nRemoveAfterInsertNoSync():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);

    sortedIds.Remove(SdfPath("/Z/A"));
    sortedIds.Insert(SdfPath("/I/I"));
    sortedIds.Remove(SdfPath("/I/I"));

    _Dump(&sortedIds, "testHdSortedId_removeAfterInsertNoSyncTest.txt");

    return true;
}

static
bool
RemoveLastItemTest()
{
    std::cout << "\n\nRemoveLastItemTest():\n";

    Hd_SortedIds sortedIds;

    _Populate(&sortedIds);
    SdfPathVector paths =  sortedIds.GetIds(); // Trigger Inital Sort

    for (size_t pathNum = paths.size(); pathNum > 0; --pathNum) {
        sortedIds.Remove(paths[pathNum - 1]);
    }

    return true;
}

int main()
{
    TfErrorMark mark;

    bool success  = PopulateTest()
                 && SingleInsertTest()
                 && MultiInsertTest()
                 && RemoveTest()
                 && RemoveOnlyElementTest()
                 && RemoveRangeTest()
                 && RemoveBatchTest()
                 && RemoveSortedTest()
                 && RemoveUnsortedTest()
                 && RemoveAfterInsertNoSync()
                 && RemoveLastItemTest();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
