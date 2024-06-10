//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hd/sortedIds.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/arch/timing.h"
#include "pxr/base/tf/stringUtils.h"
#include <random>
#include <algorithm>
#include <iterator>

PXR_NAMESPACE_USING_DIRECTIVE;

static char firstLevelChar[] = "ABYZ";
static const size_t numFirstLevel = sizeof(firstLevelChar) /
                                    sizeof(firstLevelChar[0]) - 1;

ARCH_PRAGMA_UNUSED_FUNCTION;

static
SdfPathVector const &
_GetInitPaths()
{
    static SdfPathVector theInitPaths = []() {
        SdfPathVector paths;
        
        char primName[] = "/_/_/_/_";
        
        for (size_t firstLevel = 0; firstLevel < numFirstLevel; ++firstLevel) {
            primName[1] = firstLevelChar[firstLevel];
            for (char secondLevel = 'A'; secondLevel <= 'Z'; ++secondLevel) {
                primName[3] = secondLevel;
                for (char thirdLevel = 'A'; thirdLevel <= 'Z'; ++thirdLevel) {
                    primName[5] = thirdLevel;
                    for (char fourthLevel = 'A'; fourthLevel <= 'Z'; ++fourthLevel) {
                        primName[7] = fourthLevel;
                        paths.push_back(SdfPath(primName));
                    }
                }
            }
        }
        // Shuffle paths randomly.
        const size_t seed = 5109223000;
        std::mt19937 randomGen(seed);
        std::shuffle(paths.begin(), paths.end(), randomGen);

        printf("Using %zu initial paths\n", paths.size());
        
        return paths;
    }();

    return theInitPaths;
}

// Metric name to time in nanoseconds.
using Metrics = std::vector<std::pair<std::string, int64_t>>;

// Replace interior '/' with '_', e.g. '/foo/bar' -> 'foo_bar'.
static std::string
_PathToLabel(SdfPath const &p)
{
    std::string str = p.GetAsString();
    if (TfStringStartsWith(str, "/")) {
        str = str.substr(1);
    }
    return TfStringReplace(str, "/", "_");
}

static
void
PopulateTest(Metrics &metrics)
{
    int64_t ticks = ArchMeasureExecutionTime([]() {
        Hd_SortedIds result;
        for (SdfPath const &p: _GetInitPaths()) {
            result.Insert(p);
        }
        result.GetIds(); // Ensure it's sorted.
    });
    metrics.emplace_back("populate", ArchTicksToNanoseconds(ticks));
}

static
Hd_SortedIds const &
_GetPopulatedIds() {
    static const Hd_SortedIds theIds = []() {
        Hd_SortedIds ids;
        for (SdfPath const &p: _GetInitPaths()) {
            ids.Insert(p);
        }
        ids.GetIds(); // Ensure it's sorted.
        return ids;
    }();
    return theIds;
}

static
void
SingleRemoveInsertTest(Metrics &metrics)
{
    SdfPathVector testPaths = {
        SdfPath("/A/A/A/A"),
        SdfPath("/B/Y/O/B"),
        SdfPath("/Y/M/M/V"),
        SdfPath("/Z/Z/Z/Z")
    };

    Hd_SortedIds ids = _GetPopulatedIds();

    for (SdfPath const &path: testPaths) {
        int64_t ticks = ArchMeasureExecutionTime([&ids, &path]() {
            ids.Remove(path);
            ids.GetIds(); // force sort.
            ids.Insert(path);
            ids.GetIds(); // force sort.
        });
        metrics.emplace_back("add_del_" + _PathToLabel(path),
                             ArchTicksToNanoseconds(ticks));
    }
}

static
void
MultiRemoveInsertTest(Metrics &metrics)
{
    SdfPathVector testPaths = {
        SdfPath("/A/A/A/A"),
        SdfPath("/B/Y/O/B"),
        SdfPath("/Y/M/M/V"),
        SdfPath("/Z/Z/Z/Z")
    };

    Hd_SortedIds ids = _GetPopulatedIds();

    int64_t ticks = ArchMeasureExecutionTime([&ids, &testPaths]() {
        for (SdfPath const &path: testPaths) {
            ids.Remove(path);
        }
        ids.GetIds(); // force sort.
        for (SdfPath const &path: testPaths) {
            ids.Insert(path);
        }
        ids.GetIds(); // force sort.
    });
    
    metrics.emplace_back("add_del_multiple", ArchTicksToNanoseconds(ticks));
}

static
void
SubtreeRemoveInsertTest(Metrics &metrics)
{
    const SdfPathVector prefixes = {
        SdfPath("/A/A/A"),
        SdfPath("/B/Y/O"),
        SdfPath("/Y/M/M"),
        SdfPath("/Z/Z/Z")
    };

    const std::vector<SdfPathVector> subtreePathVecs = [&prefixes]() {
        std::vector<SdfPathVector> ret;
        for (SdfPath const &prefix: prefixes) {
            ret.emplace_back();
            for (SdfPath const &path: _GetInitPaths()) {
                if (path.HasPrefix(prefix)) {
                    ret.back().push_back(path);
                }
            }
        }
        return ret;
    }();

    // These must correspond.
    TF_AXIOM(prefixes.size() == subtreePathVecs.size());

    Hd_SortedIds ids = _GetPopulatedIds();

    std::vector<int64_t> timings;
    
    for (SdfPathVector const &subtreePaths: subtreePathVecs) {
        timings.push_back(
            ArchMeasureExecutionTime([&ids, &subtreePaths]() {
                for (SdfPath const &path: subtreePaths) {
                    ids.Remove(path);
                }
                ids.GetIds(); // force sort.
                for (SdfPath const &path: subtreePaths) {
                    ids.Insert(path);
                }
                ids.GetIds(); // force sort.
            }));
    }

    for (size_t i = 0; i != timings.size(); ++i) {
        metrics.emplace_back("add_del_subtree_" + _PathToLabel(prefixes[i]),
                             ArchTicksToNanoseconds(timings[i]));
    }
}

static
void
PartialSubtreeRemoveInsertTest(Metrics &metrics)
{
    const SdfPathVector prefixes = {
        SdfPath("/A/A/A"),
        SdfPath("/B/Y/O"),
        SdfPath("/Y/M/M"),
        SdfPath("/Z/Z/Z")
    };

    const std::vector<SdfPathVector> subtreePathVecs = [&prefixes]() {
        size_t counter = 0;
        std::vector<SdfPathVector> ret;
        for (SdfPath const &prefix: prefixes) {
            ret.emplace_back();
            for (SdfPath const &path: _GetInitPaths()) {
                if (path.HasPrefix(prefix) && (++counter % 3 == 0)) {
                    ret.back().push_back(path);
                }
            }
        }
        return ret;
    }();

    // These must correspond.
    TF_AXIOM(prefixes.size() == subtreePathVecs.size());

    Hd_SortedIds ids = _GetPopulatedIds();

    std::vector<int64_t> timings;
    
    for (SdfPathVector const &subtreePaths: subtreePathVecs) {
        timings.push_back(
            ArchMeasureExecutionTime([&ids, &subtreePaths]() {
                for (SdfPath const &path: subtreePaths) {
                    ids.Remove(path);
                }
                ids.GetIds(); // force sort.
                for (SdfPath const &path: subtreePaths) {
                    ids.Insert(path);
                }
                ids.GetIds(); // force sort.
            }));
    }

    for (size_t i = 0; i != timings.size(); ++i) {
        metrics.emplace_back(
            "add_del_partial_subtree_" + _PathToLabel(prefixes[i]),
            ArchTicksToNanoseconds(timings[i]));
    }
}

static
void
ScatteredRemoveInsertTest(Metrics &metrics, unsigned divisor,
                          std::string const &lbl)
{
    const SdfPathVector paths = [&]() {
        SdfPathVector ret;
        const SdfPathVector &initPaths = _GetInitPaths();
        // Take random indexes that cover 10% of the total.
        std::mt19937 gen(5109223000);
        std::uniform_int_distribution<> distrib(0, initPaths.size()-1);
        for (size_t i = 0; i != initPaths.size() / divisor; ++i) {
            ret.push_back(initPaths[distrib(gen)]);
        }
        return ret;
    }();

    Hd_SortedIds ids = _GetPopulatedIds();
    int64_t ticks = ArchMeasureExecutionTime([&ids, &paths]() {
        for (SdfPath const &path: paths) {
            ids.Remove(path);
        }
        ids.GetIds(); // force sort.
        for (SdfPath const &path: paths) {
            ids.Insert(path);
        }
        ids.GetIds(); // force sort.
    });

    metrics.emplace_back("add_del_" + lbl + "_scattered",
                         ArchTicksToNanoseconds(ticks));
}

static
void
SpreadRemoveInsertTest(Metrics &metrics, size_t numElts)
{
    Hd_SortedIds ids = _GetPopulatedIds();

    // Determine which we'll remove/reinsert -- select evenly spread elements
    // from ids.
    SdfPathVector paths;
    for (size_t x = 0; x != numElts; ++x) {
        size_t idx = (ids.GetIds().size() * (x+1)) / (numElts+1);
        paths.push_back(ids.GetIds()[idx]);
    }
        
    int64_t ticks = ArchMeasureExecutionTime([&ids, &paths]() {
        for (SdfPath const &path: paths) {
            ids.Remove(path);
        }
        ids.GetIds(); // force sort.
        for (SdfPath const &path: paths) {
            ids.Insert(path);
        }
        ids.GetIds(); // force sort.
    });

    metrics.emplace_back(TfStringPrintf("add_del_%zu_spread", numElts),
                         ArchTicksToNanoseconds(ticks));
}

static
void
SubtreeRenameTest(Metrics &metrics,
                  SdfPath const &oldPrefix,
                  SdfPath const &newPrefix)
{
    const std::vector<std::pair<SdfPath, SdfPath>> renames = [&]() {
        std::vector<std::pair<SdfPath, SdfPath>> ret;
        for (SdfPath const &path: _GetInitPaths()) {
            if (path.HasPrefix(oldPrefix)) {
                ret.emplace_back(
                    path, path.ReplacePrefix(oldPrefix, newPrefix));
            }
        }
        return ret;
    }();

    Hd_SortedIds ids = _GetPopulatedIds();

    int64_t ticks =
        ArchMeasureExecutionTime([&ids, &renames]() {
            for (auto &names: renames) {
                ids.Remove(names.first);
                ids.Insert(names.second);
            }
            ids.GetIds(); // force sort.
            for (auto &names: renames) {
                ids.Remove(names.second);
                ids.Insert(names.first);
            }
            ids.GetIds(); // force sort.
        });

    metrics.emplace_back(
        "rename_" + _PathToLabel(oldPrefix) + "_to_" + _PathToLabel(newPrefix),
        ArchTicksToNanoseconds(ticks));
}

int main()
{
    Metrics metrics;
    
    PopulateTest(metrics);
    SingleRemoveInsertTest(metrics);
    MultiRemoveInsertTest(metrics);
    SubtreeRemoveInsertTest(metrics);
    PartialSubtreeRemoveInsertTest(metrics);
    ScatteredRemoveInsertTest(metrics, 10000, "0_01pct");
    ScatteredRemoveInsertTest(metrics, 1000, "0_1pct");
    ScatteredRemoveInsertTest(metrics, 100, "1pct");
    ScatteredRemoveInsertTest(metrics, 20, "5pct");
    ScatteredRemoveInsertTest(metrics, 10, "10pct");
    ScatteredRemoveInsertTest(metrics, 5, "20pct");
    ScatteredRemoveInsertTest(metrics, 2, "50pct");
    SpreadRemoveInsertTest(metrics, 1);
    SpreadRemoveInsertTest(metrics, 2);
    SpreadRemoveInsertTest(metrics, 5);
    SpreadRemoveInsertTest(metrics, 10);
    SpreadRemoveInsertTest(metrics, 20);
    SpreadRemoveInsertTest(metrics, 50);
    SpreadRemoveInsertTest(metrics, 100);
    SubtreeRenameTest(metrics, SdfPath("/A/B/C"), SdfPath("/A/B/_C"));
    SubtreeRenameTest(metrics, SdfPath("/A/B"), SdfPath("/A/_B"));
    SubtreeRenameTest(metrics, SdfPath("/Z/Z"), SdfPath("/A/B/_Z"));
    
    FILE *statsFile = fopen("perfstats.raw", "w");
    for (const auto &[metricName, ns]: metrics) {
        fprintf(statsFile,
                "{'profile':'%s','metric':'time','value':%zd,'samples':1}\n",
                metricName.c_str(), ns);
        printf("%s : %zd ns\n", metricName.c_str(), ns);
    }
    fclose(statsFile);
    
    printf("OK\n");
}
