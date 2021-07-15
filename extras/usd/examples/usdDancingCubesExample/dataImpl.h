//
// Copyright 2019 Pixar
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
#ifndef PXR_EXTRAS_USD_EXAMPLES_USD_DANCING_CUBES_EXAMPLE_DATA_IMPL_H
#define PXR_EXTRAS_USD_EXAMPLES_USD_DANCING_CUBES_EXAMPLE_DATA_IMPL_H

#include "pxr/pxr.h"
#include "data.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/abstractData.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdDancingCubesExample_DataImpl
///
/// This is the implementation of the read only functionality provided by 
/// UsdDancingCubesExample_Data. All queries on specs and time samples are based
/// on the procedural generated from the input paramters object. Some of the 
/// generated data is cached, like the names of prim specs and sharable frame
/// of animation data, while the rest is computed on the fly when queried.
/// See data.h for the overall explanation of what this data represents.
/// 
class UsdDancingCubesExample_DataImpl
{
public:
    UsdDancingCubesExample_DataImpl();

    UsdDancingCubesExample_DataImpl(
        const UsdDancingCubesExample_DataParams &params);

    /// Generates the spec type for the path.
    SdfSpecType GetSpecType(const SdfPath &path) const;

    /// Returns whether a value should exist for the given \a path and 
    /// \a fieldName. Optionally returns the value if it exists.
    bool Has(const SdfPath &path, const TfToken &field,
             VtValue *value = NULL) const;

    /// Visits every spec generated from our params with the given 
    /// \p visitor. 
    void VisitSpecs(const SdfAbstractData &data,
                    SdfAbstractDataSpecVisitor *visitor) const;

    /// Returns the list of all fields generated for spec path.
    const std::vector<TfToken> &List(const SdfPath &path) const;

    /// Returns a set that enumerates all integer frame values from 0 to the 
    /// total number of animation frames specified in the params object.
    const std::set<double> &ListAllTimeSamples() const;

    /// Returns the same set as ListAllTimeSamples if the spec path is for one
    /// of the animated properties. Returns an empty set for all other spec
    /// paths.
    const std::set<double> &ListTimeSamplesForPath(
        const SdfPath &path) const;

    /// Returns the total number of animation frames iif the spec path is for
    /// one of the animated properties. Returns 0 for all other spec paths.
    size_t GetNumTimeSamplesForPath(const SdfPath &path) const;

    /// Sets the upper and lower bound time samples of the value time and 
    /// returns true as long as there are any animated frames for this data.
    bool GetBracketingTimeSamples(
        double time, double *tLower, double *tUpper) const;

    /// Sets the upper and lower bound time samples of the value time and 
    /// returns true if the spec path is for one of the animated properties.
    /// Returns false for all other spec paths.
    bool GetBracketingTimeSamplesForPath(
        const SdfPath &path, double time, 
        double *tLower, double *tUpper) const;

    /// Computes the value for the time sample if the spec path is one of the 
    /// animated properties.
    bool QueryTimeSample(const SdfPath &path, double time,
                         VtValue *value) const;
private:
    // Initializes the cached data from the params object.
    void _InitFromParams();

    // Helper functions for queries about property specs.
    bool _IsAnimatedProperty(const SdfPath &path) const;
    bool _HasPropertyDefaultValue(const SdfPath &path, VtValue *value) const;
    bool _HasPropertyTypeNameValue(const SdfPath &path, VtValue *value) const;

    // Helper for computing the animated property values at an arbitrary time.
    double _GetTranslateOffset(double time) const;
    double _GetRotateAmount(double time) const;
    GfVec3f _GetColor(double time) const;

    // The parameters use to generate specs and time samples, obtained from the
    // layer's file format arguments.
    UsdDancingCubesExample_DataParams _params;

    // Cached set of generated time sample times. All of the animated property
    // time sample fields have the same time sample times.
    std::set<double> _animTimeSampleTimes;

    // Cached set of all paths with a generated prim spec.
    TfHashSet<SdfPath, SdfPath::Hash> _primSpecPaths;

    // Cached list of the names of all child prims for each generated prim spec
    // that is not a leaf. The child prim names are the same for all prims that
    // make up the cube layout hierarchy. 
    std::vector<TfToken> _primChildNames;

    // Cached default position and animation frame offset for each leaf prim
    struct _LeafPrimData
    {
        GfVec3d pos;
        double frameOffset;
    };
    TfHashMap<SdfPath, _LeafPrimData, SdfPath::Hash> _leafPrimDataMap;

    // Cached animation data for each frame in a single animation cycle.
    struct _AnimData
    {
        double transOffset;
        GfVec3f color;
    };
    std::vector<_AnimData> _animCycleSampleData;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_EXTRAS_USD_EXAMPLES_USD_DANCING_CUBES_EXAMPLE_DATA_IMPL_H
