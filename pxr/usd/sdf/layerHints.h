//
// Copyright 2020 Pixar
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
#ifndef PXR_USD_SDF_LAYER_HINTS_H
#define PXR_USD_SDF_LAYER_HINTS_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Contains hints about layer contents that may be used to accelerate certain
/// composition operations.
class SdfLayerHints
{
public:
    /// Default constructed hints provide the most conservative set of values
    /// such that consumers of the hints will act correctly if not optimally.
    SdfLayerHints() = default;

    /// Construct hints with specific values.  Using this constructor requires
    /// that all hint fields be specified.
    explicit SdfLayerHints(bool mightHaveRelocates)
        : mightHaveRelocates(mightHaveRelocates)
    {}

    /// If this field is false, the layer does not contain relocates.  If
    /// true, relocates may be present but are not guaranteed to exist.
    bool mightHaveRelocates = true;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
