//
// Copyright 2016 Pixar
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
