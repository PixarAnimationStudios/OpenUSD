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
#ifndef PXR_USD_USD_CLIP_SET_H
#define PXR_USD_USD_CLIP_SET_H

#include "pxr/pxr.h"

#include "pxr/usd/usd/clip.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/declarePtrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(PcpLayerStack);

class Usd_ClipSet;
class Usd_ClipSetDefinition;

using Usd_ClipSetRefPtr = std::shared_ptr<Usd_ClipSet>;

/// \class Usd_ClipSet
///
/// Represents a clip set for value resolution. A clip set primarily
/// consists of a list of Usd_Clip objects from which attribute values
/// are retrieved during value resolution.
///
class Usd_ClipSet
{
public:
    /// Create a new clip set based on the given definition. If clip 
    /// set creation fails, returns a null pointer and populates
    /// \p status with an error message. Otherwise \p status may be
    /// populated with other information or debugging output.
    static Usd_ClipSetRefPtr New(
        const Usd_ClipSetDefinition& definition,
        std::string* status);

    Usd_ClipSet(const Usd_ClipSet&) = delete;
    Usd_ClipSet& operator=(const Usd_ClipSet&) = delete;

    PcpLayerStackPtr sourceLayerStack;
    SdfPath sourcePrimPath;
    size_t sourceLayerIndex;
    Usd_ClipRefPtr manifestClip;
    Usd_ClipRefPtrVector valueClips;

private:
    Usd_ClipSet(const Usd_ClipSetDefinition& definition);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
