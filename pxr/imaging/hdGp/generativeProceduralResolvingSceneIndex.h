//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_RESOLVING_SCENE_INDEX_H
#define PXR_IMAGING_HD_GP_GENERATIVE_PROCEDURAL_RESOLVING_SCENE_INDEX_H

#include "pxr/imaging/hdGp/generativeProcedural.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/base/tf/denseHashSet.h"

#include <tbb/concurrent_unordered_map.h>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdGpGenerativeProceduralResolvingSceneIndex
/// 
/// HdGpGenerativeProceduralResolvingSceneIndex is a scene index which
/// evaluates prims representing generative procedurals within its incoming
/// scene and outputs their resulting prims its own observers.
///
/// The hydra prim type used to identify generative procedurals can be
/// configured per instance of this scene index to allow for a pipeline to
/// stage when certain procedural prims are resolved within the chain of scene
/// indicies. By default that type is "generativeProcedural".
/// 
/// This scene index also re-types (to its observers) any procedural prim it
/// acts upon to be of type "resolvedGenerativeProcedural" to avoid potentially
/// evaluating a single procedural multiple times.
/// 
/// In its current form, it does NOT recursively resolve any procedural prims
/// which are the result of the procedural prims for which it is itself
/// evaluting. Additionally, all procedural prims evaluated here see the same
/// input scene -- and not the results of other procedurals resolved by the
/// same scene index instance.
///
class HdGpGenerativeProceduralResolvingSceneIndex;
TF_DECLARE_REF_PTRS(HdGpGenerativeProceduralResolvingSceneIndex);

class HdGpGenerativeProceduralResolvingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:

    static HdGpGenerativeProceduralResolvingSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene) {
        return TfCreateRefPtr(
            new HdGpGenerativeProceduralResolvingSceneIndex(inputScene));
    }

    static HdGpGenerativeProceduralResolvingSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene,
            const TfToken &targetPrimTypeName) {
        return TfCreateRefPtr(
            new HdGpGenerativeProceduralResolvingSceneIndex(
                inputScene, targetPrimTypeName));
    }

    /// SATISFYING HdSceneIndexBase ///////////////////////////////////////////

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HdGpGenerativeProceduralResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene);

    HdGpGenerativeProceduralResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene,
        const TfToken &targetPrimTypeName);

    /// SATISFYING HdSingleInputFilteringSceneIndexBase ///////////////////////

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    ///////////////////////////////////////////////////////////////////////////

private:

    static HdGpGenerativeProcedural *_ConstructProcedural(
        const TfToken &typeName, const SdfPath &proceduralPrimPath);


    // MEMBER TYPES ///////////////////////////////////////////////////////////

    using _DensePathSet = TfDenseHashSet<SdfPath, TfHash>;

    static void _CombinePathArrays(const _DensePathSet &s, SdfPathVector *v);

    struct _ProcEntry
    {
        enum State : unsigned char {
            StateUncooked = 0,
            StateDependenciesCooking,
            StateDependenciesCooked,
            StateCooking,
            StateCooked,
        };

        using _PathSetMap =
            TfDenseHashMap<SdfPath, _DensePathSet, TfHash>;

        std::atomic<State> state;
        TfToken typeName;
        std::shared_ptr<HdGpGenerativeProcedural> proc;
        HdGpGenerativeProcedural::ChildPrimTypeMap childTypes;
        HdGpGenerativeProcedural::DependencyMap dependencies;
        _PathSetMap childHierarchy;
        std::mutex cookMutex;


        _ProcEntry()
        : state(StateUncooked)
        {}

        _ProcEntry(const _ProcEntry &rhs)
        {
            state.store(rhs.state.load());
            proc = rhs.proc;
            typeName = rhs.typeName;
            childTypes = rhs.childTypes;
            dependencies = rhs.dependencies;
            childHierarchy = rhs.childHierarchy;
        }
    };

    struct _GeneratedPrimEntry
    {
        _GeneratedPrimEntry()
        : responsibleProc(nullptr)
        {}

        _GeneratedPrimEntry(_ProcEntry * p)
        : responsibleProc(p)
        {}

        _GeneratedPrimEntry(const _GeneratedPrimEntry &rhs)
        {
            responsibleProc.store(rhs.responsibleProc.load());
        }
        std::atomic<_ProcEntry *> responsibleProc;
    };

    using _GeneratedPrimsMap = tbb::concurrent_unordered_map<
        SdfPath, _GeneratedPrimEntry, SdfPath::Hash>;

    using _ProcEntryMap =
        std::unordered_map<SdfPath, _ProcEntry, TfHash>;

    using _PathSet = std::unordered_set<SdfPath, TfHash>;

    using _DependencyMap =
        std::unordered_map<SdfPath, _PathSet, SdfPath::Hash>;

    struct _Notices
    {
        HdSceneIndexObserver::AddedPrimEntries added;
        HdSceneIndexObserver::RemovedPrimEntries removed;
        HdSceneIndexObserver::DirtiedPrimEntries dirtied;
    };

    // MEMBER FUNCTIONS ///////////////////////////////////////////////////////

    _ProcEntry * _UpdateProceduralDependencies(
        const SdfPath &proceduralPrimPath) const;

    _ProcEntry * _UpdateProcedural(
        const SdfPath &proceduralPrimPath,
        bool forceUpdate,
        _Notices *outputNotices,
        const HdGpGenerativeProcedural::DependencyMap
            *dirtiedDependencies = nullptr
    ) const;

    void _RemoveProcedural(
        const SdfPath &proceduralPrimPath,
        _Notices *outputNotices=nullptr) const;

    // XXX Does thread-unsafe deletion.
    // Removes deleted entries from _generatedPrims.
    // This is private for now but intended for future use by a discussed formal
    // method on HdSceneIndexBase itself.
    void _GarbageCollect();

    // MEMBER VARIABLES ///////////////////////////////////////////////////////
    // procedural prim path -> entry
    mutable _ProcEntryMap _procedurals;

    // reverse mapping of dependency -> dependent roots
    mutable _DependencyMap _dependencies;

    mutable _GeneratedPrimsMap _generatedPrims;

    // no shared mutex, shared/unique lock is the same
    using _MapMutex = std::mutex;
    using _MapLock = std::lock_guard<_MapMutex>;
    mutable _MapMutex _dependenciesMutex;
    mutable _MapMutex _proceduralsMutex;

    TfToken _targetPrimTypeName;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
