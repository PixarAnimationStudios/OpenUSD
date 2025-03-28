//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
        : _parent(std::move(parentMatrix))
        , _local(std::move(localMatrix))
        , _cachedResultAt0(_local->GetTypedValue(0) * _parent->GetTypedValue(0))
    {
    }

    bool GetContributingSampleTimesForInterval(
        const HdSampledDataSource::Time startTime,
        const HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> * const outSampleTimes) override
    {
        HdSampledDataSourceHandle const ds[] = { _parent, _local };
        return HdGetMergedContributingSampleTimesForInterval(
            std::size(ds), ds,
            startTime, endTime, outSampleTimes);
    }

    VtValue GetValue(const HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    GfMatrix4d GetTypedValue(
        const HdSampledDataSource::Time shutterOffset) override
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
    const GfMatrix4d _cachedResultAt0;
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

    const HdXformSchema inputXform(ctx.GetInputDataSource());

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

    const HdXformSchema parentXform(ctx.GetFlattenedDataSourceFromParentPrim());
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
