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
#ifndef SDF_CLEANUP_TRACKER_H
#define SDF_CLEANUP_TRACKER_H

/// \file sdf/cleanupTracker.h

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/spec.h"

SDF_DECLARE_HANDLES(SdfSpec);

#include <vector>

/// \class Sdf_CleanupTracker
///
/// A singleton that tracks specs edited within an Sdf_CleanupEnabler scope.
///
/// When the last Sdf_CleanupEnabler goes out of scope, the specs are removed
/// from the layer if they are inert.
///
class Sdf_CleanupTracker : public TfWeakBase
{
public:
    
    /// Retrieves singleton instance.
    static Sdf_CleanupTracker &GetInstance();

    /// Adds the spec to the vector of tracked specs if there is at least one
    /// Sdf_CleanupEnabler on the stack.
    void AddSpecIfTracking(SdfSpecHandle const &spec);

    /// Return the authoring monitor indentified by the index
    void CleanupSpecs();

private:
    
    Sdf_CleanupTracker();
    ~Sdf_CleanupTracker();

    std::vector<SdfSpecHandle> _specs;
    
    friend class TfSingleton<Sdf_CleanupTracker>;
};

#endif // SDF_CLEANUP_TRACKER_H
