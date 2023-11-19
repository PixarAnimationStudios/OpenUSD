//
// Copyright 2023 Pixar
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
#include "pxr/imaging/hd/flattenedXformDataSourceProvider.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

class _MatrixCombinerDataSource :
    public HdTypedSampledDataSource<GfMatrix4d>
{
public:
    HD_DECLARE_DATASOURCE(_MatrixCombinerDataSource);

    _MatrixCombinerDataSource(
        HdMatrixDataSourceHandle parentMatrix,
        HdMatrixDataSourceHandle localMatrix)
        : _parent(parentMatrix)
        , _local(localMatrix)
    {
        _cachedResultAt0 = _local->GetTypedValue(0) * _parent->GetTypedValue(0);
    }

    bool GetContributingSampleTimesForInterval(
            HdSampledDataSource::Time startTime,
            HdSampledDataSource::Time endTime,
            std::vector<HdSampledDataSource::Time> *outSampleTimes) override
    {
        std::vector<HdSampledDataSource::Time> parentTimes;
        const bool parentVarying =
            _parent->GetContributingSampleTimesForInterval(
                    startTime, endTime, &parentTimes);

        std::vector<HdSampledDataSource::Time> localTimes;
        const bool localVarying =
            _local->GetContributingSampleTimesForInterval(
                    startTime, endTime, &localTimes);

        if (outSampleTimes) {
            if (parentVarying && localVarying) {
                std::set_union(
                    parentTimes.begin(), parentTimes.end(),
                    localTimes.begin(), localTimes.end(),
                    std::back_inserter(*outSampleTimes));
            } else if (parentVarying) {
                *outSampleTimes = std::move(parentTimes);
            } else if (localVarying) {
                *outSampleTimes = std::move(localTimes);
            }
        }
        return parentVarying || localVarying;
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        if (shutterOffset == 0) {
            return _cachedResultAt0;
        }

        // XXX: Note that this preserves legacy behavior of only caching at
        // time 0, but it's probably worth caching on demand. We'd need to
        // evaluate the extra memory used, and also figure out a lightweight
        // storage mechanism (since GetTypedValue can be called concurrently,
        // but a whole concurrent_map<Time,Matrix> might be too heavy).
        return _local->GetTypedValue(shutterOffset) *
            _parent->GetTypedValue(shutterOffset);
    }

protected:
    const HdMatrixDataSourceHandle _parent;
    const HdMatrixDataSourceHandle _local;
    GfMatrix4d _cachedResultAt0;
};

} // namespace

HdContainerDataSourceHandle
HdFlattenedXformDataSourceProvider::GetFlattenedDataSource(
    const Context &ctx) const
{
    static const HdContainerDataSourceHandle identityXform =
        HdXformSchema::Builder()
            .SetMatrix(
                HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                    GfMatrix4d().SetIdentity()))
            .SetResetXformStack(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .Build();

    HdXformSchema inputXform(ctx.GetInputDataSource());

    // If the local xform is fully composed, early out.
    if (HdBoolDataSourceHandle const resetXformStack =
                    inputXform.GetResetXformStack()) {
        if (resetXformStack->GetTypedValue(0.0f)) {
            // Only use the local transform, or identity if no matrix was
            // provided...
            if (inputXform.GetMatrix()) {
                return inputXform.GetContainer();
            } else {
                return identityXform;
            }
        }
    }

    HdMatrixDataSourceHandle const inputMatrixDataSource =
        inputXform.GetMatrix();

    HdXformSchema parentXform(ctx.GetFlattenedDataSourceFromParentPrim());
    HdMatrixDataSourceHandle const parentMatrixDataSource =
        parentXform.GetMatrix();

    if (!inputMatrixDataSource && !parentMatrixDataSource) {
        // If there's no local or parent matrix, return the identity. In
        // practice, this means we're resolving the root prim and it doesn't
        // have an authored transform.
        return identityXform;
    } else if (!inputMatrixDataSource) {
        // If there's a parent matrix, but not a local matrix, just return
        // the parent matrix. Note that parentXform (if it exists) is flattened,
        // so it will have the composed bit set.
        return parentXform.GetContainer();
    } else if (!parentMatrixDataSource) {
        // If there's no parent (e.g. because we're at the root), use the local
        // transform.
        return HdXformSchema::Builder()
            .SetMatrix(inputMatrixDataSource)
            .SetResetXformStack(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .Build();
    } else {
        // Otherwise, concatenate the matrices. The return value is marked as
        // fully composed, so that it doesn't get double-flattened by accident.
        return HdXformSchema::Builder()
            .SetMatrix(_MatrixCombinerDataSource::New(
                parentMatrixDataSource, inputMatrixDataSource))
            .SetResetXformStack(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .Build();
    }
}

void
HdFlattenedXformDataSourceProvider::ComputeDirtyLocatorsForDescendants(
    HdDataSourceLocatorSet * const locators) const
{
    *locators = HdDataSourceLocatorSet::UniversalSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
