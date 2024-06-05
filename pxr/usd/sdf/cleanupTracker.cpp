//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/cleanupTracker.h"
#include "pxr/usd/sdf/cleanupEnabler.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/spec.h"

#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/weakPtr.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef Sdf_CleanupTracker This;

TF_INSTANTIATE_SINGLETON(Sdf_CleanupTracker);

This &This::GetInstance() {
    return TfSingleton<This>::GetInstance();
}

This::Sdf_CleanupTracker()
{
    // make it possible to call GetInstance()
    TfSingleton<Sdf_CleanupTracker>::SetInstanceConstructed(*this);
}

// CODE_COVERAGE_OFF -- singleton instance
This::~Sdf_CleanupTracker()
{
}
// CODE_COVERAGE_ON


void This::AddSpecIfTracking(SdfSpecHandle const &spec)
{
    if (SdfCleanupEnabler::IsCleanupEnabled()) {

        // We don't want to store duplicate specs, but using a vector is cheaper
        // than using a set. Still, we make a quick check for the common case of
        // adding the same spec multiple times in a row to avoid obvious 
        // duplicates without having to search through the vector.
        if (_specs.empty() || !_specs.back() || !(_specs.back() == spec)) {
            _specs.push_back(spec);
        }
    }
}

void This::CleanupSpecs()
{
    // Instead of iterating through the vector then clearing it, we pop the back
    // element off until the vector is empty. This way if any more specs are 
    // added to the vector we don't end up with invalid iterators.
    while (!_specs.empty()) {

        SdfSpecHandle spec = _specs.back();

        // This pop_back must come before the ScneduleRemoveIfInert call below 
        // because that call might push more specs onto the vector.
        _specs.pop_back();

        if (spec) {
            spec->GetLayer()->ScheduleRemoveIfInert(spec.GetSpec());
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
