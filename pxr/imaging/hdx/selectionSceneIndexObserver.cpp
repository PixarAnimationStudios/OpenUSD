//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/selectionSceneIndexObserver.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/instanceIndicesSchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/selectionSchema.h"
#include "pxr/imaging/hd/selectionsSchema.h"
#include "pxr/base/work/loops.h"

#include "pxr/base/trace/trace.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

static void
_AddToSelection(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    HdSelectionSharedPtr const &result,
    std::mutex &resultMutex);

HdxSelectionSceneIndexObserver::HdxSelectionSceneIndexObserver()
 : _version(0)
 , _selection(std::make_shared<HdSelection>())
{
}

static
void
_PopulateFromSceneIndex(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &path,
    HdSelectionSharedPtr const &result,
    std::mutex &resultMutex)
{
    TRACE_FUNCTION();

    _AddToSelection(sceneIndex, path, result, resultMutex);
    const SdfPathVector childPaths = sceneIndex->GetChildPrimPaths(path);
    if (!childPaths.empty()) {
        WorkParallelForEach(
            childPaths.begin(), childPaths.end(),
            [&](SdfPath const& childPath) {
                _PopulateFromSceneIndex(
                    sceneIndex, childPath, result, resultMutex);
            });
    }
}

void
HdxSelectionSceneIndexObserver::SetSceneIndex(
    HdSceneIndexBaseRefPtr const &sceneIndex)
{
    if (sceneIndex == _sceneIndex) {
        return;
    }

    HdSceneIndexObserverPtr self(this);

    if (_sceneIndex) {
        _sceneIndex->RemoveObserver(self);
    }

    _sceneIndex = sceneIndex;
    _selection = std::make_shared<HdSelection>();
    _dirtiedPrims.clear();

    if (_sceneIndex) {
        sceneIndex->AddObserver(self);
        std::mutex resultMutex;
        _PopulateFromSceneIndex(
            sceneIndex, SdfPath::AbsoluteRootPath(), _selection, resultMutex);
    }
    
    _version++;
}

int
HdxSelectionSceneIndexObserver::GetVersion() const
{
    return _version;
}

HdSelectionSharedPtr
HdxSelectionSceneIndexObserver::GetSelection()
{
    if (!_dirtiedPrims.empty()) {
        _selection = _ComputeSelection();
    }

    return _selection;
}

static
int
_GetNumInstancesForPrototypeIndex(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &instancerPath,
    const int prototypeIndex)
{
    TRACE_FUNCTION();

    const HdInstancerTopologySchema instancerTopologySchema =
        HdInstancerTopologySchema::GetFromParent(
            sceneIndex->GetPrim(instancerPath).dataSource);

    const HdIntArrayVectorSchema indicesSchema =
        instancerTopologySchema.GetInstanceIndices();
    
    HdIntArrayDataSourceHandle const indicesDs =
        indicesSchema.GetElement(prototypeIndex);
    if (!indicesDs) {
        return 1;
    }

    return indicesDs->GetTypedValue(0.0f).size();
}

// Finds the instancer and prototype from the HdInstanceIndicesSchema to
// query the instancer how often it instances the prototype.
static
int
_GetNumInstances(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const HdInstanceIndicesSchema &instanceIndices)
{
    TRACE_FUNCTION();

    HdPathDataSourceHandle const instancerPathDs =
        instanceIndices.GetInstancer();
    if (!instancerPathDs) {
        return 1;
    }
    const SdfPath instancerPath = instancerPathDs->GetTypedValue(0.0f);
    if (instancerPath.IsEmpty()) {
        return 1;
    }

    HdIntDataSourceHandle const prototypeIndexDs =
        instanceIndices.GetPrototypeIndex();
    if (!prototypeIndexDs) {
        return 1;
    }

    return _GetNumInstancesForPrototypeIndex(
        sceneIndex,
        instancerPath,
        prototypeIndexDs->GetTypedValue(0.0f));
}

static
VtIntArray
_GetInstanceIndices(const HdInstanceIndicesSchema &instanceIds)
{
    HdIntArrayDataSourceHandle const dataSource =
        instanceIds.GetInstanceIndices();
    if (!dataSource) {
        return {};
    }
    return dataSource->GetTypedValue(0.0f);
}

// The selection of the schema is something like:
//     instance 5 and 6 of 10 in the outer most instancer
//     instance 3 of 12 in the next instancer
//     instance 7 and 8 of 15 in the inner most instancer.
// So in total, we have 4 nested instances selected.
// HdSelection expects only one number for each selected nested instance
// which we will compute them as follows:
//  [ (5 * 12 + 3) * 15 + 7,
//    (5 * 12 + 3) * 15 + 8,
//    (6 * 12 + 3) * 15 + 7,
//    (6 * 12 + 3) * 15 + 8 ].
//
static
VtIntArray
_GetInstanceIndices(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const HdInstanceIndicesVectorSchema &instanceIndicesVector)
{
    TRACE_FUNCTION();

    const size_t n = instanceIndicesVector.GetNumElements();
    if (n == 0) {
        return {};
    }

    VtIntArray result({0});

    // Going from outer most to inner most instancer.

    // Continuing with the above example, the numbers after the
    // first iteration:
    // [5, 6],
    // second iteration:
    // [5 * 12 + 3, 6 * 12 + 3]
    // third iteration:
    // as above.

    for (size_t i = 0; i < n; i++) {
        const HdInstanceIndicesSchema instanceIndicesSchema =
            instanceIndicesVector.GetElement(i);

        // Number of instances of this prototype.
        const int numInstances = _GetNumInstances(
            sceneIndex, instanceIndicesSchema);

        const VtIntArray instanceIndices = _GetInstanceIndices(
            instanceIndicesSchema);
        const size_t l = instanceIndices.size();

        // Multiply number of nested instances by number of instances selected
        // at this nesting level.
        VtIntArray newResult(result.size() * l);
        for (size_t j = 0; j < result.size(); j++) {
            for (size_t k = 0; k < l; k++) {
                newResult[j * l + k] =
                    numInstances * result.AsConst()[j] +
                    instanceIndices[k];
            }
        }

        result = std::move(newResult);
    }

    return result;
}

// Given one of the data source under the selections locator
// for the prim at path in the scene index, process it and add
// to result.
static
void
_AddToSelection(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const HdSelectionSchema &selectionSchema,
    const SdfPath &primPath,
    HdSelectionSharedPtr const &result,
    std::mutex &resultMutex)
{
    // Only support fully selected for now.
    HdBoolDataSourceHandle const ds = selectionSchema.GetFullySelected();
    if (!ds) {
        return;
    }

    if (!ds->GetTypedValue(0.0f)) {
        return;
    }

    // Retrieve instancing information.
    const HdInstanceIndicesVectorSchema instanceIndicesVectorSchema =
        selectionSchema.GetNestedInstanceIndices();

    {
        if (instanceIndicesVectorSchema.GetNumElements() > 0) {
            const VtIntArray instanceIndices = 
                _GetInstanceIndices(sceneIndex, instanceIndicesVectorSchema);

            {
                std::unique_lock lock(resultMutex);

                result->AddInstance(
                    HdSelection::HighlightModeSelect,
                    primPath,
                    // The information in the schema is nested, that is it
                    // the instance id for each nesting level.
                    // HdSelection only expects one number for each selected
                    // instance encoding the selection of all levels.
                    instanceIndices);
            }
        } else {
            std::unique_lock lock(resultMutex);

            result->AddRprim(
                HdSelection::HighlightModeSelect,
                primPath);
        }
    }
}

// Query prim at path in scene index for selections data source
// and add to result.
static
void
_AddToSelection(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    HdSelectionSharedPtr const &result, 
    std::mutex &resultMutex)
{
    TRACE_FUNCTION();
    
    const HdSelectionsSchema selectionsSchema =
        HdSelectionsSchema::GetFromParent(
            sceneIndex->GetPrim(primPath).dataSource);
    if (!selectionsSchema) {
        return;
    }

    {
        TRACE_FUNCTION_SCOPE("Resolving selection schema");

        const size_t n = selectionsSchema.GetNumElements();
        for (size_t i = 0; i < n; ++i) {
            const HdSelectionSchema selectionSchema =
                selectionsSchema.GetElement(i);
            _AddToSelection(
                sceneIndex,
                selectionSchema,
                primPath,
                result, 
                resultMutex);
        }
    }    
}    

HdSelectionSharedPtr
HdxSelectionSceneIndexObserver::_ComputeSelection()
{
    TRACE_FUNCTION();

    HdSelectionSharedPtr const result = std::make_shared<HdSelection>();

    if (!_sceneIndex) {
        return result;
    }

    const SdfPathVector prims = _selection->GetAllSelectedPrimPaths();

    _dirtiedPrims.insert(prims.begin(), prims.end());

    if (!_dirtiedPrims.empty()) {
        TRACE_FUNCTION_SCOPE("Query prims for selection");
        
        std::mutex resultMutex;

        // On comparison with using WorkParallelForN with both a path set and
        // path vector, WorkParallelForEach seems to perform best even though
        // each worker thread processes just one element at a time, rather than
        // a range.
        //
        WorkParallelForEach(
            _dirtiedPrims.begin(), _dirtiedPrims.end(),
            [&](SdfPath const& path) {
                _AddToSelection(_sceneIndex, path, result, resultMutex);
            });

        _dirtiedPrims.clear();
    }

    return result;
}

void
HdxSelectionSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (entries.empty()) {
        return;
    }

    ++_version;

    for (const AddedPrimEntry &entry : entries) {
        _dirtiedPrims.insert(entry.primPath);
    }
}

void
HdxSelectionSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const DirtiedPrimEntry &entry : entries) {
        if (entry.dirtyLocators.Contains(
                HdSelectionsSchema::GetDefaultLocator())) {
            ++_version;
            _dirtiedPrims.insert(entry.primPath);
        }
    }
}

void
HdxSelectionSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    if (entries.empty()) {
        return;
    }

    ++_version;
}

void
HdxSelectionSceneIndexObserver::PrimsRenamed(
        const HdSceneIndexBase &sender,
        const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}

PXR_NAMESPACE_CLOSE_SCOPE
