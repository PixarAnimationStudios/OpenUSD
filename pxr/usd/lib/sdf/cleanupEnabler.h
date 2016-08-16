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
#ifndef SDF_CLEANUP_ENABLER_H
#define SDF_CLEANUP_ENABLER_H

/// \file sdf/cleanupEnabler.h

#include "pxr/base/tf/stacked.h"

/// \class SdfCleanupEnabler
///
/// An RAII class which, when an instance is alive, enables scheduling of
/// automatic cleanup of SdfLayers.
/// 
/// Any affected specs which no longer contribute to the scene will be removed 
/// when the last SdfCleanupEnabler instance goes out of scope. Note that for 
/// this purpose, SdfPropertySpecs are removed if they have only required fields 
/// (see SdfPropertySpecs::HasOnlyRequiredFields), but only if the property spec 
/// itself was affected by an edit that left it with only required fields. This 
/// will have the effect of uninstantiating on-demand attributes. For example, 
/// if its parent prim was affected by an edit that left it otherwise inert, it 
/// will not be removed if it contains an SdfPropertySpec with only required 
/// fields, but if the property spec itself is edited leaving it with only 
/// required fields, it will be removed, potentially uninstantiating it if it's 
/// an on-demand property.
/// 
/// SdfCleanupEnablers are accessible in both C++ and Python.
/// 
/// /// SdfCleanupEnabler can be used in the following manner:
/// \code
/// {
///     SdfCleanupEnabler enabler;
///     
///     // Perform any action that might otherwise leave inert specs around, 
///     // such as removing info from properties or prims, or removing name 
///     // children. i.e:
///     primSpec->ClearInfo(SdfFieldKeys->Default);
/// 
///     // When enabler goes out of scope on the next line, primSpec will 
///     // be removed if it has been left as an empty over.
/// }
/// \endcode
///
class SdfCleanupEnabler : 
    public TfStacked<SdfCleanupEnabler, /* thread safe */ false>
{
public:

    SdfCleanupEnabler();

    ~SdfCleanupEnabler();

    /// Returns whether cleanup is currently being scheduled.
    static bool IsCleanupEnabled();
};

#endif  // #ifndef SDF_CLEANUP_ENABLER_H
