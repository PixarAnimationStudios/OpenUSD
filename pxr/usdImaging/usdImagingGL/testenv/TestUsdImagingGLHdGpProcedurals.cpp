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
#include "pxr/imaging/hdGp/generativeProceduralPlugin.h"
#include "pxr/imaging/hdGp/generativeProceduralPluginRegistry.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/rotation.h"

#include <iostream>
#include <chrono>

#include <math.h>

PXR_NAMESPACE_USING_DIRECTIVE

/// _MakeSomeStuffGenerativeProcedural ////////////////////////////////////////

namespace
{

TF_DEFINE_PRIVATE_TOKENS(
    _makeSomeStuffTokens,
    (stuff)
    (myCount)
    (myDepth)
    (addHierarchySparsely)
);


TF_DEFINE_PRIVATE_TOKENS(
    _childNameTokens,
    (A)
    (B)
    (C)
    (D)
    (E)
    (F)
);

// Procedural which makes up to 6 children (controlled via MyCount) at an
// arbitrary depth (controlled by myDepth). addHierarchySparsely controls
// whether only the leaf prims are typed.
class _MakeSomeStuffGenerativeProcedural : public HdGpGenerativeProcedural
{
public:

    _MakeSomeStuffGenerativeProcedural(const SdfPath &proceduralPrimPath)
    : HdGpGenerativeProcedural(proceduralPrimPath)
    {
    }

    static HdGpGenerativeProcedural *New(
        const SdfPath &proceduralPrimPath)
    {
        return new _MakeSomeStuffGenerativeProcedural(proceduralPrimPath);
    }

    DependencyMap UpdateDependencies(
        const HdSceneIndexBaseRefPtr &inputScene) override
    {
        DependencyMap result;
        return result;
    }

    ChildPrimTypeMap Update(
        const HdSceneIndexBaseRefPtr &inputScene,
        const ChildPrimTypeMap &previousResult,
        const DependencyMap &dirtiedDependencies,
        HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims) override
    {
        ChildPrimTypeMap result;

        int count = 0;
        HdSceneIndexPrim myPrim = inputScene->GetPrim(_GetProceduralPrimPath());

        if (HdSampledDataSourceHandle countDs =
                HdPrimvarsSchema::GetFromParent(myPrim.dataSource)
                .GetPrimvar(_makeSomeStuffTokens->myCount)
                .GetPrimvarValue()) {

            VtValue v = countDs->GetValue(0.0f);

            if (v.IsHolding<int>()) {
                count = v.UncheckedGet<int>();
            }
        }

        bool argsChanged = false;

        if (count != _count) {
            _count = count;
            argsChanged = true;
        }


        int depth = 1;
        if (HdSampledDataSourceHandle ds =
                HdPrimvarsSchema::GetFromParent(myPrim.dataSource)
                .GetPrimvar(_makeSomeStuffTokens->myDepth)
                .GetPrimvarValue()) {

            VtValue v = ds->GetValue(0.0f);

            if (v.IsHolding<int>()) {
                depth = v.UncheckedGet<int>();
            }
        }

        if (depth < 1) {
            depth = 1;
        }

        if (depth != _depth) {
            argsChanged = true;
            _depth = depth;
        }

        bool addHierarchySparsely = true;
        if (HdSampledDataSourceHandle ds =
                HdPrimvarsSchema::GetFromParent(myPrim.dataSource)
                .GetPrimvar(_makeSomeStuffTokens->addHierarchySparsely)
                .GetPrimvarValue()) {

            VtValue v = ds->GetValue(0.0f);

            if (v.IsHolding<bool>()) {
                addHierarchySparsely = v.UncheckedGet<bool>();
            }
        }

        if (addHierarchySparsely != _addHierarchySparsely) {
            _addHierarchySparsely = addHierarchySparsely;
            argsChanged = true;
        }

        if (!argsChanged) {
           return previousResult;
        }

        for (const TfToken &t : _childNameTokens->allTokens) {
            if (count < 1) {
                break;
            }

            SdfPath path = _GetProceduralPrimPath();
            for (int j = 0; j < depth; ++j) {
                path = path.AppendChild(t);
                if (!_addHierarchySparsely) {
                    result[path] = _makeSomeStuffTokens->stuff;
                }
            }

            if (_addHierarchySparsely && depth > 0) {
                result[path] = _makeSomeStuffTokens->stuff;
            }

            --count;
        }

        return result;
    }

    HdSceneIndexPrim GetChildPrim(
        const HdSceneIndexBaseRefPtr &inputScene,
        const SdfPath &childPrimPath) override
    {

        SdfPath relPath = childPrimPath.ReplacePrefix(_GetProceduralPrimPath(),
            SdfPath::AbsoluteRootPath());

        if (_addHierarchySparsely && static_cast<int>(
                relPath.GetPathElementCount()) < _depth) {
            return HdSceneIndexPrim();
        }


        return {_makeSomeStuffTokens->stuff, 
            HdRetainedContainerDataSource::New(
                _makeSomeStuffTokens->stuff,
                HdRetainedTypedSampledDataSource<int>::New(5)
            )
        };
    }

private:
    int _count = 0;
    int _depth = 1;
    bool _addHierarchySparsely = true;
};

}

class MakeSomeStuffGenerativeProceduralPlugin
    : public HdGpGenerativeProceduralPlugin
{
public:
    MakeSomeStuffGenerativeProceduralPlugin() = default;

    HdGpGenerativeProcedural *Construct(
        const SdfPath &proceduralPrimPath) override
    {
        return _MakeSomeStuffGenerativeProcedural::New(proceduralPrimPath);
    }
};

/// _CubePerMeshPointProcedural ///////////////////////////////////////////////

namespace
{

TF_DEFINE_PRIVATE_TOKENS(
    _cubePerMeshTokens,
    (sourceMeshPath)
    (scale)
);

// Procedural which makes a scaled cube (scale controlled via "primvars:scale")
// at each point of the mesh referenced by a "primvars:sourceMeshPath"
// relationship.
class _CubePerMeshPointProcedural : public HdGpGenerativeProcedural
{
public:
    _CubePerMeshPointProcedural(const SdfPath &proceduralPrimPath)
    : HdGpGenerativeProcedural(proceduralPrimPath)
    {
    }

    // Looks at arguments declares current state of dependencies
    DependencyMap UpdateDependencies(
        const HdSceneIndexBaseRefPtr &inputScene) override
    {
        DependencyMap result;
        _Args args = _GetArgs(inputScene);
        if (!args.sourceMeshPath.IsEmpty()) {
            result[args.sourceMeshPath] = {
                HdPrimvarsSchema::GetPointsLocator(),
                HdXformSchema::GetDefaultLocator(),
                };
        }

        return result;
    }

    // Cooks/Recooks and returns the current state of child paths and their
    // types
    ChildPrimTypeMap Update(
        const HdSceneIndexBaseRefPtr &inputScene,
        const ChildPrimTypeMap &previousResult,
        const DependencyMap &dirtiedDependencies,
        HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims) override
    {
        ChildPrimTypeMap result;

        _Args args = _GetArgs(inputScene);
        // BEGIN potential comparsions between current and previous args

        bool meshPointsStillValid = false;
        bool meshXformStillValid = false;

        // if we already have mesh points...
        if (_meshPointsDs) {
            // ...and our source path unchanged...
            if (args.sourceMeshPath == _args.sourceMeshPath) {
                //...and the mesh path isn't present in dirtiedDependencies...
                if (dirtiedDependencies.find(args.sourceMeshPath) ==
                        dirtiedDependencies.end()) {
                    meshPointsStillValid = true;
                    meshXformStillValid = true;
                }
            }
        }

        if (!meshPointsStillValid) {
            _meshPointsDs = nullptr;
        }

        if (!meshXformStillValid) {
            _primMatrixDs = nullptr;
        }

        // END potential comparsions between current and previous args
        _args = args; // store args, could compare

        if (args.sourceMeshPath.IsEmpty()) {
            _childIndices.clear();
            return result;
        }

        VtValue pointsValue;

        if (!_meshPointsDs) {
            HdSceneIndexPrim sourceMeshPrim =
                inputScene->GetPrim(args.sourceMeshPath);

            if (sourceMeshPrim.primType == HdPrimTypeTokens->mesh) {
                // retrieve points from source mesh
                if (HdSampledDataSourceHandle pointsDs = 
                        HdPrimvarsSchema::GetFromParent(
                            sourceMeshPrim.dataSource)
                                .GetPrimvar(HdPrimvarsSchemaTokens->points)
                                    .GetPrimvarValue()) {

                    VtValue v = pointsDs->GetValue(0.0f);
                    if (v.IsHolding<VtArray<GfVec3f>>()) {
                        _meshPointsDs = pointsDs;
                        pointsValue = v;
                    }
                }

                _primMatrixDs =
                    HdXformSchema::GetFromParent(
                        sourceMeshPrim.dataSource).GetMatrix();
            }
        } else {
            // For now, let's dirty everything from the previous result and
            // return it. We could be more specific in comparisons of our args.
            return _DirtyAll(previousResult, outputDirtiedPrims);
        }

        if (!_meshPointsDs) {
            _childIndices.clear();
            return result;
        }

        const VtArray<GfVec3f> points =
            pointsValue.UncheckedGet<VtArray<GfVec3f>>();

        // Even if the point positions have changed, if the point count hasn't
        // changed, we can return our previous result, dirtying the xform of
        // our child prims
        if (points.size() == _childIndices.size()) {
            return _DirtyAll(previousResult, outputDirtiedPrims);
        }

        char buffer[64];
        SdfPath myPath = _GetProceduralPrimPath();
        for (int i = 0; i < static_cast<int>(points.size()); ++i) {
            ::sprintf(buffer, "c%d", i);
            SdfPath childPath = myPath.AppendChild(TfToken(buffer));
            result[childPath] = HdPrimTypeTokens->mesh;

            // if the child already exist, indicate that its transform is dirty
            if (!_childIndices.insert({childPath, i}).second) {
                if (outputDirtiedPrims) {
                    outputDirtiedPrims->emplace_back(childPath,
                        HdXformSchema::GetDefaultLocator());
                }
            }
            // otherwise it is unnecessary as that child is new and need not
            // be dirtied
        }

        // Remove _childIndices entries not present in the current result
        // only if the new result has fewer points than the previous
        if (result.size() < _childIndices.size()) {
            _ChildIndexMap::iterator it = _childIndices.begin();
            while (it != _childIndices.end()) {

                _ChildIndexMap::iterator cit = it;
                ++it;

                if (result.find(cit->first) == result.end()) {
                    _childIndices.erase(cit);
                }
            }
        }

        return result;
    }

    // Returns dataSource of a child prim -- in this case deferring the
    // calculation of the transform matrix to a _XformFromMeshPointDataSource
    HdSceneIndexPrim GetChildPrim(
        const HdSceneIndexBaseRefPtr &inputScene,
        const SdfPath &childPrimPath) override
    {
        HdSceneIndexPrim result;

        if (_meshPointsDs) {
            auto it = _childIndices.find(childPrimPath);
            if (it != _childIndices.end()) {
                result.primType = HdPrimTypeTokens->mesh;
                result.dataSource = HdRetainedContainerDataSource::New(
                    HdXformSchemaTokens->xform,
                    HdXformSchema::Builder()
                        .SetMatrix(_XformFromMeshPointDataSource::New(
                            _args.scale,
                            it->second,
                            _meshPointsDs,
                            _primMatrixDs))
                        .Build(),
                    HdMeshSchemaTokens->mesh,
                    _GetChildMeshDs(),
                    HdPrimvarsSchemaTokens->primvars,
                    _GetChildPrimvarsDs(),
                    TfToken("taco"),
                    HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                        HdDataSourceLocator(TfToken("taco"), TfToken("salsa")))
                
                );
            }
        }

        return result;
    }

private:

    /// private types /////////////////////////////////////////////////////////

    using _ChildIndexMap = std::unordered_map<SdfPath, int, TfHash>;

    struct _Args
    {
        _Args()
        : scale(1.0f)
        {}

        SdfPath sourceMeshPath;
        float scale;
    };

    // Stores the sourceMesh's points datasource, an index and a scale value
    // computes the resulting matrix on demand (inclusive of source mesh
    // motion samples if requested)
    class _XformFromMeshPointDataSource : public HdMatrixDataSource
    {
    public:

        HD_DECLARE_DATASOURCE(_XformFromMeshPointDataSource);


        bool GetContributingSampleTimesForInterval(
            Time startTime, 
            Time endTime,
            std::vector<Time> * outSampleTimes) override
        {
            return _pointsDs->GetContributingSampleTimesForInterval(
                startTime, endTime, outSampleTimes);
        }

        VtValue GetValue(Time shutterOffset)
        {
            return VtValue(GetTypedValue(shutterOffset));
        }

        GfMatrix4d GetTypedValue(Time shutterOffset)
        {
            VtArray<GfVec3f> p =
                _pointsDs->GetValue(shutterOffset)
                    .UncheckedGet<VtArray<GfVec3f>>();

            if (_index < 0 || _index >= static_cast<int>(p.size())) {
                return GfMatrix4d(1);
            }

            GfMatrix4d m = GfMatrix4d(1).SetTranslateOnly(p[_index]);
            m = GfMatrix4d(1).SetScale(_scale) * m; 
            
            if (_primMatrixDs) {
                m = m * _primMatrixDs->GetTypedValue(shutterOffset);
            }

            return m;
        }

    private:

        _XformFromMeshPointDataSource(
            float scale,
            int index,
            HdSampledDataSourceHandle pointsDs,
            HdMatrixDataSourceHandle primMatrixDs
            )
        : _scale(scale)
        , _index(index)
        , _pointsDs(pointsDs)
        , _primMatrixDs(primMatrixDs)
        {}

        float _scale;
        int _index;
        HdSampledDataSourceHandle _pointsDs;
        HdMatrixDataSourceHandle _primMatrixDs;
    };


    /// private member variables //////////////////////////////////////////////

    _Args _args;
    _ChildIndexMap _childIndices;
    HdSampledDataSourceHandle _meshPointsDs;
    HdMatrixDataSourceHandle _primMatrixDs;

    /// private member functions //////////////////////////////////////////////

    _Args _GetArgs(const HdSceneIndexBaseRefPtr &inputScene)
    {
        _Args result;

        HdSceneIndexPrim myPrim = inputScene->GetPrim(_GetProceduralPrimPath());

        HdPrimvarsSchema primvars =
            HdPrimvarsSchema::GetFromParent(myPrim.dataSource);

        if (HdSampledDataSourceHandle sourceMeshDs = primvars
                .GetPrimvar(_cubePerMeshTokens->sourceMeshPath)
                .GetPrimvarValue()) {
            VtValue v = sourceMeshDs->GetValue(0.0f);

            if (v.IsHolding<VtArray<SdfPath>>()) {
                VtArray<SdfPath> a = v.UncheckedGet<VtArray<SdfPath>>();
                if (a.size() == 1) {
                    result.sourceMeshPath = a[0];
                }
            } else if (v.IsHolding<std::string>()) {
                result.sourceMeshPath = SdfPath(v.UncheckedGet<std::string>());
            }
        }

        if (HdSampledDataSourceHandle ds = primvars
                .GetPrimvar(_cubePerMeshTokens->scale)
                .GetPrimvarValue()) {
            VtValue v = ds->GetValue(0.0f);
            if (v.IsHolding<float>()) {
                result.scale = v.UncheckedGet<float>();
            }
        }

        return result;
    }


    HdGpGenerativeProcedural::ChildPrimTypeMap _DirtyAll(
        const HdGpGenerativeProcedural::ChildPrimTypeMap &childTypes,
        HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims)
    {
        if (outputDirtiedPrims) {
            for (const auto &pathTypePair : childTypes) {
                outputDirtiedPrims->emplace_back(pathTypePair.first,
                    HdXformSchema::GetDefaultLocator());
            }
        }
        return childTypes;
    }


    HdContainerDataSourceHandle _GetChildMeshDs()
    {
        static const VtIntArray faceVertexCounts =
            {4, 4, 4, 4, 4, 4};

        static const VtIntArray faceVertexIndices =
            {0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1,
                7, 5, 3, 6, 0, 2, 4};

        using _IntArrayDs =
            HdRetainedTypedSampledDataSource<VtIntArray>;

        static const _IntArrayDs::Handle fvcDs =
            _IntArrayDs::New(faceVertexCounts);

        static const _IntArrayDs::Handle fviDs =
            _IntArrayDs::New(faceVertexIndices);

        static const HdContainerDataSourceHandle meshDs =
            HdMeshSchema::Builder()
                .SetTopology(HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(fvcDs)
                    .SetFaceVertexIndices(fviDs)
                    .Build())
                .Build();

        return meshDs;

    }

    HdContainerDataSourceHandle _GetChildPrimvarsDs()
    {
        static const VtArray<GfVec3f> points = {
            {-0.1f, -0.1f, 0.1f},
            {0.1f, -0.1f, 0.1f},
            {-0.1f, 0.1f, 0.1f},
            {0.1f, 0.1f, 0.1f},
            {-0.1f, 0.1f, -0.1f},
            {0.1f, 0.1f, -0.1f},
            {-0.1f, -0.1f, -0.1f},
            {0.1f, -0.1f, -0.1f}};

        using _PointArrayDs =
            HdRetainedTypedSampledDataSource<VtArray<GfVec3f>>;

        static const HdContainerDataSourceHandle primvarsDs =
            HdRetainedContainerDataSource::New(
                HdPrimvarsSchemaTokens->points,
                HdPrimvarSchema::Builder()
                    .SetPrimvarValue(_PointArrayDs::New(points))
                    .SetInterpolation(HdPrimvarSchema::
                        BuildInterpolationDataSource(
                            HdPrimvarSchemaTokens->vertex))
                    .SetRole(HdPrimvarSchema::
                        BuildRoleDataSource(
                            HdPrimvarSchemaTokens->point))
                    .Build()
            );

        return primvarsDs;
    }

};
}

class CubePerMeshPointProceduralPlugin
    : public HdGpGenerativeProceduralPlugin
{
public:
    CubePerMeshPointProceduralPlugin() = default;

    HdGpGenerativeProcedural *Construct(
        const SdfPath &proceduralPrimPath) override
    {
        return new _CubePerMeshPointProcedural(proceduralPrimPath);
    }
};


/// _DependsOnChildNamesProcedural ////////////////////////////////////////////

// This procedural which makes data sources on a single prim based of the
// immediate children of another prim on the inpout scene..
// This is here to test that a "__childNames" dependency works correctly for
// when children are added or removed from a specified prim path.

TF_DEFINE_PRIVATE_TOKENS(
    _dependsOnChildTokens,
    (targetPrim)
    (childNames)
    (myResult)
    (myType)
);


namespace
{
class _DependsOnChildNamesProcedural : public HdGpGenerativeProcedural
{
public:

    _DependsOnChildNamesProcedural(const SdfPath &proceduralPrimPath)
    : HdGpGenerativeProcedural(proceduralPrimPath)
    {
    }

    static HdGpGenerativeProcedural *New(const SdfPath &proceduralPrimPath)
    {
        return new _DependsOnChildNamesProcedural(proceduralPrimPath);
    }

    DependencyMap UpdateDependencies(const HdSceneIndexBaseRefPtr &inputScene)
        override
    {
        DependencyMap result;

        HdSceneIndexPrim myPrim = inputScene->GetPrim(_GetProceduralPrimPath());

        _targetPrimPath = SdfPath::EmptyPath();

        HdPrimvarsSchema primvars =
            HdPrimvarsSchema::GetFromParent(myPrim.dataSource);

        if (HdSampledDataSourceHandle targetPrimDs = primvars
                .GetPrimvar(_dependsOnChildTokens->targetPrim)
                .GetPrimvarValue()) {
            VtValue v = targetPrimDs->GetValue(0.0f);
            if (v.IsHolding<VtArray<SdfPath>>()) {
                VtArray<SdfPath> a = v.UncheckedGet<VtArray<SdfPath>>();
                if (a.size() == 1) {
                    _targetPrimPath = a[0];
                }
            } else if (v.IsHolding<std::string>()) {
                _targetPrimPath = SdfPath(v.UncheckedGet<std::string>());
            }
        }
        
        if (!_targetPrimPath.IsEmpty()) {
            result[_targetPrimPath].insert(GetChildNamesDependencyKey());
        }

        return result;
    }

    ChildPrimTypeMap Update(
        const HdSceneIndexBaseRefPtr &inputScene,
        const ChildPrimTypeMap &previousResult,
        const DependencyMap &dirtiedDependencies,
        HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims) override
    {
        // if this isn't the first cook, dirty our previously returned prim
        if (!previousResult.empty()) {
            if (outputDirtiedPrims) {
                static const HdDataSourceLocator loc(
                    _dependsOnChildTokens->childNames);
                for (const auto &pathTypePair : previousResult) {
                    outputDirtiedPrims->emplace_back(pathTypePair.first,
                        HdDataSourceLocatorSet(loc));
                }
            }
            return previousResult;
        }

        return {{
            _GetProceduralPrimPath().AppendChild(
                _dependsOnChildTokens->myResult),_dependsOnChildTokens->myType
        }};
    }

    HdSceneIndexPrim GetChildPrim(
        const HdSceneIndexBaseRefPtr &inputScene,
        const SdfPath &childPrimPath) override
    {
        return {_dependsOnChildTokens->myType,
            HdRetainedContainerDataSource::New(
                _dependsOnChildTokens->childNames,
                _ChildNamesContainer::New(_targetPrimPath, inputScene))};
    }

    // lazily query at target path
    class _ChildNamesContainer : public HdContainerDataSource
    {
    public:
        HD_DECLARE_DATASOURCE(_ChildNamesContainer);

        _ChildNamesContainer(
            const SdfPath &targetPrimPath,
            const HdSceneIndexBaseRefPtr &inputScene)
        : _targetPrimPath(targetPrimPath)
        , _inputScene(inputScene)
        {}


        TfTokenVector GetNames() override
        {
            TfTokenVector result;
            if (!_targetPrimPath.IsEmpty()) {
                for (const SdfPath &childPath :
                        _inputScene->GetChildPrimPaths(_targetPrimPath)) {
                    result.push_back(childPath.GetNameToken());
                }
            }

            return result;
        }

        HdDataSourceBaseHandle Get(const TfToken &name) override
        {
            // Check that it exists and is meaningful (as the hydra scene
            // browser updates in place based on whether a data source is
            // returned and it's confusing if we always return something)
            if (_inputScene->GetPrim(
                    _targetPrimPath.AppendChild(name)).primType.IsEmpty()) {
                return nullptr;
            }
            return HdRetainedTypedSampledDataSource<bool>::New(true);
        }

    private:
        SdfPath _targetPrimPath;
        HdSceneIndexBaseRefPtr _inputScene;
    };


private:
    SdfPath _targetPrimPath;

};
} //


class DependsOnChildNamesProceduralPlugin
    : public HdGpGenerativeProceduralPlugin
{
public:
    DependsOnChildNamesProceduralPlugin() = default;

    HdGpGenerativeProcedural *Construct(
        const SdfPath &proceduralPrimPath) override
    {
        return _DependsOnChildNamesProcedural::New(proceduralPrimPath);
    }
};

///////////////////////////////////////////////////////////////////////////////


TF_DEFINE_PRIVATE_TOKENS(
    _asyncTestTokens,
    (asyncChild)
);


namespace
{
class _AsyncTestProcedural : public HdGpGenerativeProcedural
{
public:
    _AsyncTestProcedural(const SdfPath &proceduralPrimPath)
    : HdGpGenerativeProcedural(proceduralPrimPath)
    , _asyncEnabled(false)
    , _phase(0.0f)
    {}

    static HdGpGenerativeProcedural *New(const SdfPath &proceduralPrimPath)
    {
        return new _AsyncTestProcedural(proceduralPrimPath);
    }

    DependencyMap UpdateDependencies(const HdSceneIndexBaseRefPtr &inputScene)
        override
    {
        DependencyMap result;
        return result;
    }

    ChildPrimTypeMap Update(
        const HdSceneIndexBaseRefPtr &inputScene,
        const ChildPrimTypeMap &previousResult,
        const DependencyMap &dirtiedDependencies,
        HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims) override
    {
        if (_asyncEnabled) {
            return previousResult;
        }

        return {
            {_GetProceduralPrimPath().AppendChild(_asyncTestTokens->asyncChild),
                HdPrimTypeTokens->mesh},
        };
    }


    HdSceneIndexPrim GetChildPrim(
        const HdSceneIndexBaseRefPtr &inputScene,
        const SdfPath &childPrimPath) override
    {

        const SdfPath childPath =
            _GetProceduralPrimPath().AppendChild(_asyncTestTokens->asyncChild);

        if (childPrimPath != childPath) {
            return {TfToken(), nullptr};
        }

        HdSceneIndexPrim result;

        result.primType = HdPrimTypeTokens->mesh;

        result.dataSource = HdRetainedContainerDataSource::New(
                    HdXformSchemaTokens->xform,
                    _GetXformDataSourceHandle(),
                    HdMeshSchemaTokens->mesh,
                    _GetChildMeshDs(),
                    HdPrimvarsSchemaTokens->primvars,
                    _GetChildPrimvarsDs()
                );


        return result;
    }


    bool AsyncBegin(bool asyncEnabled) override
    {
        _asyncEnabled = asyncEnabled;

        if (asyncEnabled) {
            return true;
        }
    
        return false;
    }

    AsyncState AsyncUpdate(
        const ChildPrimTypeMap &previousResult,
        ChildPrimTypeMap *outputPrimTypes,
        HdSceneIndexObserver::DirtiedPrimEntries *outputDirtiedPrims)
    {
        const std::chrono::time_point<std::chrono::system_clock> now =
            std::chrono::system_clock::now();

        auto int_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();

        _phase = static_cast<float>((int_ms % 3000)) / 3000.0f;

        const SdfPath childPath =
            _GetProceduralPrimPath().AppendChild(_asyncTestTokens->asyncChild);

        (*outputPrimTypes)[childPath] = HdPrimTypeTokens->mesh;

        outputDirtiedPrims->emplace_back(
            childPath, HdXformSchema::GetDefaultLocator());

        return ContinuingWithNewChanges;
    }

private:

    class _XformMatrixDataSource : public HdMatrixDataSource
    {
    public:

        HD_DECLARE_DATASOURCE(_XformMatrixDataSource);

        bool GetContributingSampleTimesForInterval(
            Time startTime, 
            Time endTime,
            std::vector<Time> * outSampleTimes) override
        {
            return false;
        }

        VtValue GetValue(Time shutterOffset) override
        {
            return VtValue(GetTypedValue(shutterOffset));
        }

        GfMatrix4d GetTypedValue(Time shutterOffset) override
        {
            GfMatrix4d m = GfMatrix4d(1)
                .SetTranslateOnly(GfVec3d(
                    _scale * ::sin(_phase * 6.283185307179586f), 0,
                    2.0 +  ::cos(_phase * 6.283185307179586f * 4) * 0.25))
                .SetRotateOnly(GfRotation(GfVec3d(1, 0, 0), _phase * 360.0));
            return m;
        }

    private:

        _XformMatrixDataSource(
            float phase,
            float scale)
        : _phase(phase)
        , _scale(scale)
        {}

        float _phase;
        float _scale;
    };

    HdContainerDataSourceHandle _GetXformDataSourceHandle()
    {
        
        return  HdXformSchema::Builder()
            .SetMatrix(_XformMatrixDataSource::New(_phase, 3.0f))
            .Build();
    }

    HdContainerDataSourceHandle _GetChildPrimvarsDs()
    {
        static const VtArray<GfVec3f> points = {
            {-0.1f, -0.1f, 0.1f},
            {0.1f, -0.1f, 0.1f},
            {-0.1f, 0.1f, 0.1f},
            {0.1f, 0.1f, 0.1f},
            {-0.1f, 0.1f, -0.1f},
            {0.1f, 0.1f, -0.1f},
            {-0.1f, -0.1f, -0.1f},
            {0.1f, -0.1f, -0.1f}};

        using _PointArrayDs =
            HdRetainedTypedSampledDataSource<VtArray<GfVec3f>>;

        static const HdContainerDataSourceHandle primvarsDs =
            HdRetainedContainerDataSource::New(
                HdPrimvarsSchemaTokens->points,
                HdPrimvarSchema::Builder()
                    .SetPrimvarValue(_PointArrayDs::New(points))
                    .SetInterpolation(HdPrimvarSchema::
                        BuildInterpolationDataSource(
                            HdPrimvarSchemaTokens->vertex))
                    .SetRole(HdPrimvarSchema::
                        BuildRoleDataSource(
                            HdPrimvarSchemaTokens->point))
                    .Build()
            );

        return primvarsDs;
    }

    HdContainerDataSourceHandle _GetChildMeshDs()
    {
        static const VtIntArray faceVertexCounts =
            {4, 4, 4, 4, 4, 4};

        static const VtIntArray faceVertexIndices =
            {0, 1, 3, 2, 2, 3, 5, 4, 4, 5, 7, 6, 6, 7, 1, 0, 1,
                7, 5, 3, 6, 0, 2, 4};

        using _IntArrayDs =
            HdRetainedTypedSampledDataSource<VtIntArray>;

        static const _IntArrayDs::Handle fvcDs =
            _IntArrayDs::New(faceVertexCounts);

        static const _IntArrayDs::Handle fviDs =
            _IntArrayDs::New(faceVertexIndices);

        static const HdContainerDataSourceHandle meshDs =
            HdMeshSchema::Builder()
                .SetTopology(HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(fvcDs)
                    .SetFaceVertexIndices(fviDs)
                    .Build())
                .Build();

        return meshDs;

    }

    bool _asyncEnabled;
    float _phase;

};

} // anonymous namespace


class AsyncTestProceduralPlugin
    : public HdGpGenerativeProceduralPlugin
{
public:
    AsyncTestProceduralPlugin() = default;

    HdGpGenerativeProcedural *Construct(
        const SdfPath &proceduralPrimPath) override
    {
        return _AsyncTestProcedural::New(proceduralPrimPath);
    }
};


///////////////////////////////////////////////////////////////////////////////




TF_REGISTRY_FUNCTION(TfType)
{
    HdGpGenerativeProceduralPluginRegistry::Define<
        MakeSomeStuffGenerativeProceduralPlugin,
        HdGpGenerativeProceduralPlugin>();

    HdGpGenerativeProceduralPluginRegistry::Define<
        CubePerMeshPointProceduralPlugin,
        HdGpGenerativeProceduralPlugin>();

    HdGpGenerativeProceduralPluginRegistry::Define<
        DependsOnChildNamesProceduralPlugin,
        HdGpGenerativeProceduralPlugin>();

    HdGpGenerativeProceduralPluginRegistry::Define<
        AsyncTestProceduralPlugin,
        HdGpGenerativeProceduralPlugin>();

}
