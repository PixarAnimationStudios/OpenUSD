//
// Copyright 2024 Pixar
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
#include "pxr/imaging/hdsi/velocityMotionResolvingSceneIndex.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/rotation.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/visitValue.h"

#include "pxr/pxr.h"

#include <algorithm>
#include <cstddef>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (fps));
    
namespace {
    
// XXX: We need to encode the fps in the scene index (in a standard
// place). Note that fps is called timeCodesPerSecond in USD.
const float _fps = 24.0f;
    
float _GetFps(const HdContainerDataSourceHandle& inputArgs)
{
    if (!inputArgs) {
        return _fps;
    }
    const auto source = HdSampledDataSource::Cast(inputArgs->Get(_tokens->fps));
    if (!source) {
        return _fps;
    }
    const VtValue &value = source->GetValue(0.0f);
    if (!value.IsHolding<float>()) {
        return _fps;
    }
    return value.UncheckedGet<float>();
}

bool
_PrimTypeUsesVelocity(const TfToken& primType)
{
    static const TfToken::Set types {
        HdPrimTypeTokens->points,
        HdPrimTypeTokens->basisCurves,
        HdPrimTypeTokens->nurbsCurves,
        HdPrimTypeTokens->nurbsPatch,
        HdPrimTypeTokens->tetMesh,
        HdPrimTypeTokens->mesh,
        HdPrimTypeTokens->instancer };
    return types.count(primType) > 0;
}

bool
_PrimvarAffectedByVelocity(const TfToken& primvar)
{
    static const TfToken::Set primvars {
        HdPrimvarsSchemaTokens->points,
        HdInstancerTokens->instanceTranslations,
        HdInstancerTokens->instanceRotations };
    return primvars.count(primvar) > 0;
}

// -----------------------------------------------------------------------------

// Since we can have rotations as VtQuathArray or VtQuatfArray, these helpers
// make applying angular velocities to them less ugly.

template <typename T>
VtValue
_ApplyAngularVelocities(
    const VtArray<T>& rotations,
    const VtVec3fArray& velocities,
    const HdSampledDataSource::Time scaledTime)
{
    VtArray<T> result(rotations.size());
    for (size_t i = 0; i < rotations.size(); ++i) {
        GfRotation rotation = GfRotation(rotations[i]);
        rotation *= GfRotation(velocities[i],
            scaledTime * velocities[i].GetLength());
        result[i] = T(rotation.GetQuat());
    }
    return VtValue(result);
}

VtValue
_ApplyAngularVelocities(
    const VtValue& rotations,
    const VtVec3fArray& velocities,
    const HdSampledDataSource::Time scaledTime)
{
    if (rotations.IsHolding<VtQuathArray>()) {
        return _ApplyAngularVelocities(
            rotations.UncheckedGet<VtQuathArray>(), velocities, scaledTime);
    }
    if (rotations.IsHolding<VtQuatfArray>()) {
        return _ApplyAngularVelocities(
            rotations.UncheckedGet<VtQuatfArray>(), velocities, scaledTime);
    }
    TF_CODING_ERROR("Unexpected rotations type");
    return VtValue();
}

// -----------------------------------------------------------------------------

class _VelocityHelper
{
public:
    using Time = HdSampledDataSource::Time;
    
    _VelocityHelper(
        const TfToken& name,
        const HdSampledDataSourceHandle& source,
        const SdfPath& primPath,
        const HdContainerDataSourceHandle& primSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _name(name)
      , _source(source)
      , _primPath(primPath)
      , _primSource(primSource)
      , _inputArgs(inputArgs)
    { }
    
protected:
    bool
    _GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time>* const outSampleTimes)
    {
        // If the source reports no motion over the interval defer to source.
        const bool sourceRes = _source->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
        VtValue sourceValue;
        Time sampleTime;
        if (!_VelocityMotionValidForCurrentFrame(&sourceValue, nullptr, &sampleTime)) {
            return sourceRes;
        }
        
        // Velocity motion is valid; check for non-linear motion and
        // insert any additional required sample times according to
        // nonlinearSampleCount.
        *outSampleTimes = { startTime, endTime };
        if (_name == HdInstancerTokens->instanceRotations ||
            _GetAccelerations(sampleTime).size() >= sourceValue.GetArraySize()) {
            const int n = std::max(3, _GetNonlinearSampleCount()) - 1;
            for (int k = 1; k < n; ++k) {
                outSampleTimes->insert(outSampleTimes->end() - 1, 
                    startTime + float(k) / float(n) * (endTime - startTime));
            }
        }
        return true;
    }
    
    VtValue
    _GetValue(Time shutterOffset)
    {
        VtVec3fArray velocities;
        VtValue sourceVal;
        Time sampleTime;
        if (!_VelocityMotionValidForCurrentFrame(
            &sourceVal, &velocities, &sampleTime)) {
            return _source->GetValue(shutterOffset);
        }
        const float fps = _GetFps(_inputArgs);
        const Time scaledTime = (shutterOffset - sampleTime) / fps;
        
        // rotations
        if (_name == HdInstancerTokens->instanceRotations) {
            return _ApplyAngularVelocities(sourceVal, velocities, scaledTime);
        }
        
        // positions
        const auto positions = sourceVal.UncheckedGet<VtVec3fArray>();
        
        // check for accelerations
        const auto accelerations = _GetAccelerations(sampleTime);
        const bool useAccelerations = accelerations.size() >= positions.size();
        
        // perform velocity motion on positions
        VtVec3fArray result(positions.size());
        if (useAccelerations) {
            const float timeSqrHalf = 0.5f * scaledTime * scaledTime;
            for (size_t i = 0; i < positions.size(); ++i) {
                result[i] = positions[i]
                  + scaledTime * velocities[i]
                  + timeSqrHalf * accelerations[i];
            }
        } else {
            for (size_t i = 0; i < positions.size(); ++i) {
                result[i] = positions[i]
                  + scaledTime * velocities[i];
            }
        }
        return VtValue(result);
    }

private:    
    // Retrieves the value of the accelerations primvar for the current frame,
    // if present. If not present, or incorrect type, or the contributing sample
    // time doesn't match the given one, returns an empty VtVec3fArray.
    // Caller still needs to check that there are enough accelerations
    // values to cover all the positions needing transformation.
    VtVec3fArray
    _GetAccelerations(
        const Time sampleTime) const
    {
        static const VtVec3fArray empty { };
        static const HdDataSourceLocator accelerationsLocator {
            HdPrimvarsSchema::GetSchemaToken(),
            HdTokens->accelerations,
            HdPrimvarSchemaTokens->primvarValue };
        const auto accelerationsDs = HdSampledDataSource::Cast(
            HdContainerDataSource::Get(_primSource, accelerationsLocator));
        if (!accelerationsDs) {
            // accelerations not present
            return empty;
        }
        std::vector<Time> times;
        if (!accelerationsDs->GetContributingSampleTimesForInterval(
            0.0, 0.0, &times)) {
            // accelerations has constant value across all time; sample timing
            // does not matter
            times.resize(1);
            times.front() = sampleTime;
        }
        if (times.front() != sampleTime) {
            // accelerations not authored at same starting time as source
            return empty;
        }
        const VtValue accelerationsVal = accelerationsDs->GetValue(times.front());
        if (!accelerationsVal.IsHolding<VtVec3fArray>()) {
            // accelerations are wrong type
            return empty;
        }
        return accelerationsVal.UncheckedGet<VtVec3fArray>();
    }
    
    /// \brief Determines whether the conditions are met for performing velocity-
    /// based motion on the underlying source primvar at the current frame.
    /// Populates \p srcVal, \p velocities, and \p outSampleTime, if provided,
    /// with their respective values so they do not need to be fetched again by
    /// the caller.
    /// \param srcValue (optional) will be filled with the sampled value of the
    /// underlying source for the current frame. This will be 0-order points,
    /// positions, or orientations at the first authored sample time at or
    /// before shutterOffset 0
    /// \param velocities (optional) will be filled with the sampled velocities
    /// value at for the current frame. This will be the 1-order velocities or
    /// angular velocities t the first authored sample time at or before
    /// shutterOffset 0
    /// \param outSampleTime (optional) will be filled with the shutterOffset
    /// value at which \p srcValue and \p velocities were actually sampled. This
    /// is the offset to the most recent authored time sample, and will always
    /// be <= 0
    /// \returns true if conditions are met for velocity motion, false
    /// otherwise. The optional parameters will not be populated if the return
    /// value is false.
    bool
    _VelocityMotionValidForCurrentFrame(
        VtValue* srcValue = nullptr,
        VtVec3fArray* velocities = nullptr,
        Time* outSampleTime = nullptr) const
    {
        const HdDataSourceLocator velocitiesLocator {
            HdPrimvarsSchema::GetSchemaToken(),
            _name == HdInstancerTokens->instanceRotations
              ? HdTokens->angularVelocities
              : HdTokens->velocities,
            HdPrimvarSchemaTokens->primvarValue };
        const HdSampledDataSourceHandle velocitiesDs =
            HdSampledDataSource::Cast(HdContainerDataSource::Get(
                _primSource, velocitiesLocator));
        if (!velocitiesDs) {
            // velocities not present
            return false;
        }
        std::vector<std::vector<Time>> times(2);
        if (!_source->GetContributingSampleTimesForInterval(
            0.0, 0.0, &times[0])) {
            // source has constant value across all time; no velocity motion
            return false;
        }
        if (!velocitiesDs->GetContributingSampleTimesForInterval(
            0.0, 0.0, &times[1])) {
            // velocities has constant value across all time; sample timing
            // does not matter
            times[1].resize(1);
            times[1].front() = times[0].front();
        };
        if (times[1].front() != times[0].front()) {
            // velocities not authored at same starting time as source
            return false;
        }
        const Time sampleTime = times[0].front();
        const VtValue velocitiesVal = velocitiesDs->GetValue(sampleTime);
        if (!velocitiesVal.IsHolding<VtVec3fArray>()) {
            // velocities are wrong type
            return false;
        }
        const VtValue sourceVal = _source->GetValue(sampleTime);
        if (sourceVal.GetArraySize() > velocitiesVal.GetArraySize()) {
            // not enough velocities
            return false;
        }
        if (_name == HdInstancerTokens->instanceRotations) {
            if (!(sourceVal.IsHolding<VtQuathArray>() ||
                sourceVal.IsHolding<VtQuatfArray>())) {
                // source rotations are wrong type
                return false;
            }
        } else if (!sourceVal.IsHolding<VtVec3fArray>()) {
            // source points/positions are wrong type
            return false;
        }
        if (srcValue != nullptr) {
            *srcValue = sourceVal;
        }
        if (velocities != nullptr) {
            *velocities = velocitiesVal.UncheckedGet<VtVec3fArray>();
        }
        if (outSampleTime != nullptr) {
            *outSampleTime = sampleTime;
        }
        return true;
    }
    
    int
    _GetNonlinearSampleCount()
    {
        static const int defaultValue = 3; // From UsdGeomMotionAPI
        static const HdDataSourceLocator locator = {
            HdPrimvarsSchema::GetSchemaToken(),
            HdTokens->nonlinearSampleCount,
            HdPrimvarSchemaTokens->primvarValue };
        const auto ds = HdSampledDataSource::Cast(HdContainerDataSource::Get(
            _primSource, locator));
        if (!ds) {
            return defaultValue;
        }
        return ds->GetValue(0.0).GetWithDefault(defaultValue);
    }
    
    // name of the 0-order parameter this data source wraps
    // (positions, points, or orientations)
    TfToken _name;
    // the incoming data source for the 0-order parameter to be wrapped
    HdSampledDataSourceHandle _source;
    SdfPath _primPath;
    HdContainerDataSourceHandle _primSource;
    HdContainerDataSourceHandle _inputArgs;
};

// -----------------------------------------------------------------------------

class _UntypedValueDataSource final
  : public HdSampledDataSource
  , private _VelocityHelper
{
public:
    using Time = HdSampledDataSource::Time;
    
    HD_DECLARE_DATASOURCE(_UntypedValueDataSource);
    
    _UntypedValueDataSource(
        const TfToken& name,
        const HdSampledDataSourceHandle& source,
        const SdfPath& primPath,
        const HdContainerDataSourceHandle& primSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _VelocityHelper(name, source, primPath, primSource, inputArgs)
    { }
    
    VtValue
    GetValue(Time shutterOffset) override
    {
        return _GetValue(shutterOffset);
    }
    
    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        return _GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }
};

HD_DECLARE_DATASOURCE_HANDLES(_UntypedValueDataSource);

// -----------------------------------------------------------------------------

template <typename T>
class _TypedValueDataSource final
  : public HdTypedSampledDataSource<T>
  , private _VelocityHelper
{
public:
    using Time = HdSampledDataSource::Time;
    
    HD_DECLARE_DATASOURCE_ABSTRACT(_TypedValueDataSource<T>);
    
    _TypedValueDataSource(
        const TfToken& name,
        const HdSampledDataSourceHandle& source,
        const SdfPath& primPath,
        const HdContainerDataSourceHandle& primSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _VelocityHelper(name, source, primPath, primSource, inputArgs)
    { }
    
    VtValue
    GetValue(Time shutterOffset) override
    {
        return _GetValue(shutterOffset);
    }
    
    bool
    GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        return _GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }
    
    T
    GetTypedValue(Time shutterOffset) override
    {
        const VtValue& v = GetValue(shutterOffset);
        if (v.IsHolding<T>()) {
            return v.UncheckedGet<T>();
        }
        return T();
    }
    
    static typename
    _TypedValueDataSource<T>::Handle New(
        const TfToken& name,
        const HdSampledDataSourceHandle& source,
        const SdfPath& primPath,
        const HdContainerDataSourceHandle& primSource,
        const HdContainerDataSourceHandle& inputArgs)
    {
        return _TypedValueDataSource<T>::Handle(
            new _TypedValueDataSource<T>(
                name, source, primPath, primSource, inputArgs));
    }
};

// -----------------------------------------------------------------------------

struct _PrimvarSourceTypeVisitor
{
    const TfToken name;
    const HdSampledDataSourceHandle source;
    const SdfPath& primPath;
    const HdContainerDataSourceHandle primSource;
    const HdContainerDataSourceHandle inputArgs;
    
    template <typename T>
    HdDataSourceBaseHandle
    operator()(const T&)
    {
        return _TypedValueDataSource<T>::New(
            name, source, primPath, primSource, inputArgs);
    }
    
    HdDataSourceBaseHandle
    operator()(const VtValue&)
    {
        return _UntypedValueDataSource::New(
            name, source, primPath, primSource, inputArgs);
    }
};

// -----------------------------------------------------------------------------

class _PrimvarDataSource final
  : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarDataSource);
    
    _PrimvarDataSource(
        const TfToken& name,
        const HdContainerDataSourceHandle& source,
        const SdfPath& primPath,
        const HdContainerDataSourceHandle& primSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _name(name)
      , _source(source)
      , _primPath(primPath)
      , _primSource(primSource)
      , _inputArgs(inputArgs)
    { }
    
    TfTokenVector
    GetNames() override
    {
        if (!_source) {
            return { };
        }

        return _source->GetNames();
    }
    
    HdDataSourceBaseHandle
    Get(const TfToken& name) override
    {
        if (!_source) {
            return nullptr;
        }
        HdDataSourceBaseHandle ds = _source->Get(name);
        if (ds && name == HdPrimvarSchemaTokens->primvarValue) {
            if (const auto source = HdSampledDataSource::Cast(ds)) {
                // XXX: source is sampled at time 0 only to determine its type
                return VtVisitValue(
                    source->GetValue(0.0f),
                    _PrimvarSourceTypeVisitor {
                        _name, source, _primPath, _primSource, _inputArgs });
            }
        }
        return ds;
    }
    
private:
    TfToken _name;
    HdContainerDataSourceHandle _source;
    SdfPath _primPath;
    HdContainerDataSourceHandle _primSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarDataSource);

// -----------------------------------------------------------------------------

class _PrimvarsDataSource final
  : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);
    
    _PrimvarsDataSource(
        const HdContainerDataSourceHandle& source,
        const SdfPath& primPath,
        const HdContainerDataSourceHandle& primSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _source(source)
      , _primPath(primPath)
      , _primSource(primSource)
      , _inputArgs(inputArgs)
    { }
    
    TfTokenVector
    GetNames() override
    {
        if (!_source) {
            return { };
        }
        return _source->GetNames();
    }
    
    HdDataSourceBaseHandle
    Get(const TfToken& name) override
    {
        if (!_source) {
            return nullptr;
        }
        HdDataSourceBaseHandle ds = _source->Get(name);
        if (ds && _PrimvarAffectedByVelocity(name)) {
            return _PrimvarDataSource::New(
                name, HdContainerDataSource::Cast(ds),
                _primPath, _primSource, _inputArgs);
        }
        return ds;
    }
private:
    HdContainerDataSourceHandle _source;
    SdfPath _primPath;
    HdContainerDataSourceHandle _primSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimvarsDataSource);

// -----------------------------------------------------------------------------

class _PrimDataSource final
  : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);
    
    _PrimDataSource(
        const SdfPath& primPath,
        const HdContainerDataSourceHandle& primSource,
        const HdContainerDataSourceHandle& inputArgs)
      : _primPath(primPath)
      , _primSource(primSource)
      , _inputArgs(inputArgs)
    { }
    
    TfTokenVector
    GetNames() override
    {
        if (!_primSource) {
            return { };
        }
        return _primSource->GetNames();
    }
    
    HdDataSourceBaseHandle
    Get(const TfToken& name) override
    {
        if (!_primSource) {
            return nullptr;
        }
        HdDataSourceBaseHandle ds = _primSource->Get(name);
        if (ds && name == HdPrimvarsSchema::GetSchemaToken()) {
            return _PrimvarsDataSource::New(
                HdContainerDataSource::Cast(ds),
                _primPath, _primSource, _inputArgs);
        }
        return ds;
    }
private:
    SdfPath _primPath;
    HdContainerDataSourceHandle _primSource;
    HdContainerDataSourceHandle _inputArgs;
};

HD_DECLARE_DATASOURCE_HANDLES(_PrimDataSource);

} // anonymous namespace

HdsiVelocityMotionResolvingSceneIndexRefPtr
HdsiVelocityMotionResolvingSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdContainerDataSourceHandle& inputArgs)
{
    return TfCreateRefPtr(
        new HdsiVelocityMotionResolvingSceneIndex(inputSceneIndex, inputArgs));
}

HdsiVelocityMotionResolvingSceneIndex::HdsiVelocityMotionResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const HdContainerDataSourceHandle& inputArgs)
 : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
 , _inputArgs(inputArgs)
{ }

HdSceneIndexPrim
HdsiVelocityMotionResolvingSceneIndex::GetPrim(
    const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (_PrimTypeUsesVelocity(prim.primType)) {
        prim.dataSource = _PrimDataSource::New(
            primPath, prim.dataSource, _inputArgs);
    }
    return prim;
}

SdfPathVector
HdsiVelocityMotionResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiVelocityMotionResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&  /*sender*/,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    return _SendPrimsAdded(entries);
}

void
HdsiVelocityMotionResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&  /*sender*/,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    return _SendPrimsRemoved(entries);
}

void
HdsiVelocityMotionResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase&  /*sender*/,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    static const HdDataSourceLocatorSet positionsLocators {
        HdPrimvarsSchema::GetPointsLocator(),
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdInstancerTokens->instanceTranslations) };
    static const HdDataSourceLocatorSet positionsAffectingLocators {
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdTokens->velocities),
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdTokens->accelerations),
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdTokens->nonlinearSampleCount) };
    static const HdDataSourceLocatorSet rotationsLocators {
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdInstancerTokens->instanceRotations) };
    static const HdDataSourceLocatorSet rotationsAffectingLocators {
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdTokens->angularVelocities),
        HdPrimvarsSchema::GetDefaultLocator()
            .Append(HdTokens->nonlinearSampleCount) };
    HdSceneIndexObserver::DirtiedPrimEntries newEntries;
    for (const auto& entry : entries) {
        HdSceneIndexObserver::DirtiedPrimEntry newEntry(entry);
        if (entry.dirtyLocators.Intersects(positionsAffectingLocators) &&
            !entry.dirtyLocators.Intersects(positionsLocators)) {
            newEntry.dirtyLocators.insert(positionsLocators);
        }
        if (entry.dirtyLocators.Intersects(rotationsAffectingLocators) &&
            !entry.dirtyLocators.Intersects(rotationsLocators)) {
            newEntry.dirtyLocators.insert(rotationsLocators);
        }
        newEntries.push_back(newEntry);
    }
    return _SendPrimsDirtied(newEntries);
}

PXR_NAMESPACE_CLOSE_SCOPE
