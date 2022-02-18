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

#include "hdPrman/velocityMotionBlurSceneIndexPlugin.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (fps)
    (velocities)
    ((sceneIndexPluginName, "HdPrman_VelocityMotionBlurSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "Prman";

// XXX: We need to encode the fps in the scene index (in a standard
// place). Note that fps is called timeCodesPerSecond in USD.
static const float _fps = 24.0f;

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_VelocityMotionBlurSceneIndexPlugin,
        HdSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdContainerDataSourceHandle const inputArgs =
        HdRetainedContainerDataSource::New(
            _tokens->fps,
            HdRetainedSampledDataSource::New(VtValue(_fps)));

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        inputArgs,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

namespace
{

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PrimvarValueDataSource
///
/// Serves as data source for locator primvars>points>primvarValue.
/// Adds the velocities at locator primvars>velocities>primvarValue
///
class _PrimvarValueDataSource final : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarValueDataSource);

    /// samplesSource: original source for locator primvars>points>primvarValue
    /// primvarsSource: original source for locator primvars
    /// inputArgs: source from scene index plugin
    _PrimvarValueDataSource(
        const HdSampledDataSourceHandle &samplesSource,
        const HdContainerDataSourceHandle &primvarsSource,
        const HdContainerDataSourceHandle &inputArgs)
      : _samplesSource(samplesSource)
      , _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
    {
    }

    VtValue GetValue(Time shutterOffset) override;
    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> * outSampleTimes) override
    {
        // Unfortunately, the scene emulation always calls this
        // with startTime and endTime being the smallest and largest
        // finite floating point number.
        //
        // We rely on the UsdImaging knowing the relevant camera and
        // its shutter interval and UsdImaging returning a sample time
        // for the beginning and end of the shutter interval.
        //
        // We probably need to revisit this function.
        //
        if (!_samplesSource) {
            return false;
        }
        
        return _samplesSource->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private: 
    VtValue _GetSourceValue(const Time shutterOffset)
    {
        return _samplesSource->GetValue(shutterOffset);
    }

    HdSampledDataSourceHandle _samplesSource;
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarValueDataSource);

// Get fps from input arguments data source
float _GetFps(const HdContainerDataSourceHandle &inputArgs)
{
    if (!inputArgs) {
        return _fps;
    }

    HdSampledDataSourceHandle const fpsSource =
        HdSampledDataSource::Cast(inputArgs->Get(_tokens->fps));
    if (!fpsSource) {
        return _fps;
    }
    
    const VtValue &fpsValue = fpsSource->GetValue(0.0f);
    if (!fpsValue.IsHolding<float>()) {
        return _fps;
    }

    return fpsValue.UncheckedGet<float>();
}

VtValue
_PrimvarValueDataSource::GetValue(const Time shutterOffset)
{
    if (!_samplesSource) {
        return VtValue();
    }

    if (shutterOffset == 0.0f) {
        return _GetSourceValue(0.0f);
    }

    // Find velocities located on prim at primvars>velocities>primvarValue
    static const HdDataSourceLocator velocityValueLocator(
        _tokens->velocities,
        HdPrimvarSchemaTokens->primvarValue);

    HdSampledDataSourceHandle const velocitiesSource =
        HdSampledDataSource::Cast(
            HdContainerDataSource::Get(
                _primvarsSource, velocityValueLocator));

    if (!velocitiesSource) {
        return _GetSourceValue(shutterOffset);
    }

    const VtValue pointsValues = _GetSourceValue(0.0f);
    if (!pointsValues.IsHolding<VtVec3fArray>()) {
        return _GetSourceValue(shutterOffset);
    }

    const VtValue velocityValues = velocitiesSource->GetValue(0.0f);
    if (!velocityValues.IsHolding<VtVec3fArray>()) {
        return _GetSourceValue(shutterOffset);
    }

    const VtVec3fArray &pointsArray = pointsValues.UncheckedGet<VtVec3fArray>();
    const VtVec3fArray &velocityArray = velocityValues.UncheckedGet<VtVec3fArray>();
    
    const size_t num = pointsArray.size();

    if (velocityArray.size() != num) {
        TF_WARN("Number of velocity vectors does not match number of points");

        return _GetSourceValue(shutterOffset);
    }
    
    const float fps = _GetFps(_inputArgs);
    
    // Add velocities to points
    const float time = shutterOffset / fps;
    
    VtVec3fArray result(num);
    
    for (size_t i = 0; i < num; i++) {
        result[i] += pointsArray[i] + time * velocityArray[i];
    }
    
    return VtValue(result);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PointsDataSource
///
/// Serves as data source for locator primvars>points
///
class _PointsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PointsDataSource);

    /// pointsSource: original source for locator primvars>points
    /// primvarsSource: original source for locator primvars
    /// inputArgs: source from scene index plugin
    _PointsDataSource(
        const HdContainerDataSourceHandle &pointsSource,
        const HdContainerDataSourceHandle &primvarsSource,
        const HdContainerDataSourceHandle &inputArgs)
      : _pointsSource(pointsSource)
      , _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
    {
    }

    bool Has(const TfToken &name) override
    {
        if (!_pointsSource) {
            return false;
        }
        
        return _pointsSource->Has(name);
    }

    TfTokenVector GetNames() override
    {
        if (!_pointsSource) {
            return {};
        }
        
        return _pointsSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;
    
private:
    HdContainerDataSourceHandle _pointsSource;
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PointsDataSource);

HdDataSourceBaseHandle
_PointsDataSource::Get(const TfToken &name)
{
    if (!_pointsSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle const result = _pointsSource->Get(name);

    if (name == HdPrimvarSchemaTokens->primvarValue) {
        // Use our own data source for primvars>points>primvarValue
        if (HdSampledDataSourceHandle const primvarValueSource =
                HdSampledDataSource::Cast(result)) {
            return _PrimvarValueDataSource::New(
                primvarValueSource, _primvarsSource, _inputArgs);
        }
    }

    return _pointsSource->Get(name);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PrimvarsDataSource
///
/// Serves as data source for locator primvars
///
class _PrimvarsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    /// primvarsSource: original source for locator primvars
    /// inputArgs: source from scene index plugin
    _PrimvarsDataSource(
        const HdContainerDataSourceHandle &primvarsSource,
        const HdContainerDataSourceHandle &inputArgs)
      : _primvarsSource(primvarsSource)
      , _inputArgs(inputArgs)
    {
    }

    bool Has(const TfToken &name) override
    {
        if (!_primvarsSource) {
            return false;
        }
        
        return _primvarsSource->Has(name);
    }

    TfTokenVector GetNames() override
    {
        if (!_primvarsSource) {
            return {};
        }

        return _primvarsSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    HdContainerDataSourceHandle _primvarsSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarsDataSource);

HdDataSourceBaseHandle
_PrimvarsDataSource::Get(const TfToken &name)
{
    if (!_primvarsSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle const result = _primvarsSource->Get(name);

    if (name == HdPrimvarsSchemaTokens->points) {
        // Use our own data source for primvars>points
        if (HdContainerDataSourceHandle const pointsSource =
                HdContainerDataSource::Cast(result)) {
            return _PointsDataSource::New(
                pointsSource, _primvarsSource, _inputArgs);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _PrimDataSource
///
/// Serves as data source for a prim.
///
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    // primSource: original source for prim
    // inputArgs: source from scene index plugin
    _PrimDataSource(
        const HdContainerDataSourceHandle &primSource,
        const HdContainerDataSourceHandle &inputArgs)
      : _primSource(primSource)
      , _inputArgs(inputArgs)
    {
    }

    bool Has(const TfToken &name) override
    {
        if (!_primSource) {
            return false;
        }
        
        return _primSource->Has(name);
    }

    TfTokenVector GetNames() override
    {
        if (!_primSource) {
            return {};
        }
        
        return _primSource->GetNames();
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    HdContainerDataSourceHandle _primSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimDataSource);

HdDataSourceBaseHandle
_PrimDataSource::Get(const TfToken &name)
{
    if (!_primSource) {
        return nullptr;
    }

    HdDataSourceBaseHandle const result = _primSource->Get(name);
    
    // Use our own data source for primvars
    if (name == HdPrimvarsSchemaTokens->primvars) {
        if (HdContainerDataSourceHandle const primvarsSource =
                HdContainerDataSource::Cast(result)) {
            return _PrimvarsDataSource::New(primvarsSource, _inputArgs);
        }
    }

    return result;
}

TF_DECLARE_REF_PTRS(_SceneIndex);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/// \class _SceneIndex
///
/// The scene index provided by the HdPrman_VelocityMotionBlurSceneIndexPlugin
///
class _SceneIndex final : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _SceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs)
    {
        return TfCreateRefPtr(
            new _SceneIndex(
                inputSceneIndex, inputArgs));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        return { prim.primType,
                 _PrimDataSource::New(
                     prim.dataSource, _inputArgs) };
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _SceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
      , _inputArgs(inputArgs)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }
        
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }
        
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    HdContainerDataSourceHandle _inputArgs;
};

void
_SceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    static const HdDataSourceLocator pointsValueLocator =
        HdPrimvarsSchema::GetPointsLocator().Append(
            HdPrimvarSchemaTokens->primvarValue);

    static const HdDataSourceLocator velocitiesValueLocator(
        HdPrimvarsSchemaTokens->primvars,
        _tokens->velocities,
        HdPrimvarSchemaTokens->primvarValue);
    
    std::vector<size_t> indices;

    for (size_t i = 0; i < entries.size(); i++) {
        if (entries[i].dirtyLocators.Intersects(velocitiesValueLocator)) {
            if (!entries[i].dirtyLocators.Intersects(pointsValueLocator)) {
                indices.push_back(i);
            }
        }
    }
    
    if (indices.empty()) {
         _SendPrimsDirtied(entries);
        return;
    }
    
    HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
    for (size_t i : indices) {
        newEntries[i].dirtyLocators.insert(pointsValueLocator);
    }

    _SendPrimsDirtied(newEntries);
}

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Implementation of HdPrman_VelocityBlurSceneIndexPlugin.

HdPrman_VelocityMotionBlurSceneIndexPlugin::
HdPrman_VelocityMotionBlurSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_VelocityMotionBlurSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _SceneIndex::New(inputScene, inputArgs);
}

PXR_NAMESPACE_CLOSE_SCOPE
