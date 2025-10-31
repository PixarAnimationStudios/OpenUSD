//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/materialXSyncSceneIndex.h"
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/sceneIndexAdapterSceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/path.h"

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

HdSt_MaterialXSyncSceneIndexRefPtr
HdSt_MaterialXSyncSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdStRenderDelegate& renderDelegate)
{
    return TfCreateRefPtr(
        new HdSt_MaterialXSyncSceneIndex(inputSceneIndex, renderDelegate));
}

HdSt_MaterialXSyncSceneIndex::HdSt_MaterialXSyncSceneIndex(
    const HdSceneIndexBaseRefPtr&   inputSceneIndex,
    const HdStRenderDelegate&       renderDelegate)
    : HdSingleInputFilteringSceneIndexBase( inputSceneIndex)
    , _renderDelegate(                      renderDelegate)
{
}

HdSt_MaterialXSyncSceneIndex::~HdSt_MaterialXSyncSceneIndex()
{
}

HdSceneIndexPrim
HdSt_MaterialXSyncSceneIndex::GetPrim(const SdfPath &primPath) const
{
    // Just forward to input scene index - we don't modify prim data
    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdSt_MaterialXSyncSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    // Just forward to input scene index - we don't modify hierarchy
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdSt_MaterialXSyncSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    HD_TRACE_FUNCTION();

    for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
        if (entry.primType == HdPrimTypeTokens->material) {
            _AddMaterial(entry.primPath);
        }
    }
    
    // Forward the notification to observers
    _SendPrimsAdded(entries);
}

void
HdSt_MaterialXSyncSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    // Just forward the notification - we don't need to do anything special for
    // removed prims
    _SendPrimsRemoved(entries);
}

void
HdSt_MaterialXSyncSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    HD_TRACE_FUNCTION();
    
    for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
        // If the dirtied prim is a material for which we've cached a filter
        // task, remove the task since it's now stale. Any ongoing generator 
        // tasks will keep their own shared pointers to the respective filter 
        // tasks, so this is safe. If the element is not in the map, then the 
        // lookup won't involve locking.
        _FilterTaskMap::accessor accessor;
        if (_filterTaskMap.find(accessor, entry.primPath)) {
            _filterTaskMap.erase(accessor);
        }
    }
    
    // Forward the notification to observers
    _SendPrimsDirtied(entries);
}

void
HdSt_MaterialXSyncSceneIndex::_AddMaterial(const SdfPath& id)
{
    HD_TRACE_FUNCTION();

    HdSceneIndexPrim materialPrim =
        _GetInputSceneIndex()->GetPrim(id);

    VtValue vtMat = HdSceneIndexAdapterSceneDelegate::GetMaterialResourceFromSceneIndexPrim(
        materialPrim,
        _renderDelegate.GetMaterialRenderContexts());

    HdStResourceRegistrySharedPtr resourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(
            _renderDelegate.GetResourceRegistry());

    auto generatorTask = HdSt_CreateMaterialXGeneratorTask(
        id,
        vtMat,
        *resourceRegistry->GetHgi());

    if (generatorTask) {
        {
            _FilterTaskMap::accessor accessor;
            _filterTaskMap.insert(accessor, id);
            accessor->second = generatorTask->GetFilterTask();
        }
        _AddGeneratorTask(std::move(generatorTask), resourceRegistry);
    }
}

void
HdSt_MaterialXSyncSceneIndex::_AddGeneratorTask(
    std::unique_ptr<HdSt_MaterialXGeneratorTask> generatorTask,
    HdStResourceRegistrySharedPtr& resourceRegistry)
{
    HD_TRACE_FUNCTION();

    if (resourceRegistry->ContainsMaterialXShader(
        generatorTask->GetShaderHash())) {
        // We already have a shader for this topology.
        return;
    }

    // Use a separate concurrent container for tracking unique generator tasks.
    // The generated shader will be registered in the resource registry when
    // the task completes.
    auto insertResult =
        _generatorTaskSet.insert(generatorTask->GetShaderHash());

    if (insertResult.second) {
        // If no generator task with the given hash existed,
        // run the task
        _LaunchGeneratorTask(
            std::move(generatorTask), resourceRegistry);
    }
}

void
HdSt_MaterialXSyncSceneIndex::_LaunchGeneratorTask(
    std::unique_ptr<HdSt_MaterialXGeneratorTask> generatorTask,
    HdStResourceRegistrySharedPtr& resourceRegistry)
{
    HD_TRACE_FUNCTION();

    // Just in case, pass the registry to the parallel task by a weak pointer,
    // to avoid keeping it alive if something goes wrong with synchronization.
    std::weak_ptr<HdStResourceRegistry> resourceRegistryWeakPtr
        = resourceRegistry;

    _dispatcher.Run(
        [generatorTask = std::move(generatorTask), resourceRegistryWeakPtr]
        {
            HD_TRACE_FUNCTION();

            mx::ShaderPtr mxShader;

            try {
                mxShader = generatorTask->Generate();
            }
            catch (mx::Exception& exception) {
                TF_CODING_ERROR("Unable to generate the MaterialX shader.\n"
                    "MxException: %s", exception.what());
            }

            if (HdStResourceRegistrySharedPtr resourceRegistry =
                resourceRegistryWeakPtr.lock()) {

                HdInstance<mx::ShaderPtr> mxShaderInstance =
                    resourceRegistry->RegisterMaterialXShader(
                        generatorTask->GetShaderHash());

                // Store the mx::ShaderPtr
                mxShaderInstance.SetValue(mxShader);
            }
            else {
                TF_CODING_ERROR(
                    "HdStResourceRegistry destroyed before "
                    "the async task completes");
            }
        }
    );
}

void
HdSt_MaterialXSyncSceneIndex::_Wait()
{
    HD_TRACE_FUNCTION();

    _dispatcher.Wait();
    _generatorTaskSet.clear();
}

HdSt_MaterialFilterTaskSharedPtr
HdSt_MaterialXSyncSceneIndex::WaitAndExtractFilterTask(const SdfPath& materialPath)
{
    HD_TRACE_FUNCTION();

    // Make sure that all generator tasks have completed
    _Wait();

    // Find and extract the filter task from the map using thread-safe accessor
    _FilterTaskMap::accessor accessor;
    if (_filterTaskMap.find(accessor, materialPath)) {
        // Transfer ownership by copying the shared_ptr and removing from map
        HdSt_MaterialFilterTaskSharedPtr filterTask = accessor->second;
        _filterTaskMap.erase(accessor);  // Thread-safe erase using accessor
        return filterTask;
    }
    
    // No filter task found for this path
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
