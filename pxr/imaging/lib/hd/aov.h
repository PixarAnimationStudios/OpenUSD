//
// Copyright 2018 Pixar
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
#ifndef HD_AOV_H
#define HD_AOV_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> HdAovSettingsMap;

/// \class HdAovDescriptor
///
/// A bundle of state describing an AOV ("Alternate Output Value") display
/// channel. Note that in hydra API, this data is split between
/// HdRenderPassAovBinding and HdRenderBufferDescriptor. This class is
/// provided for use in higher level application-facing API.
struct HdAovDescriptor
{
    HdAovDescriptor()
        : format(HdFormatInvalid), multiSampled(false)
        , clearValue(), aovSettings() {}
    HdAovDescriptor(HdFormat f, bool ms, VtValue const& c)
        : format(f), multiSampled(ms), clearValue(c), aovSettings() {}

    // ----------------------------------------------------------------
    // Render buffer parameters
    // ----------------------------------------------------------------

    /// The AOV output format. See also HdRenderBufferDescriptor#format.
    HdFormat format;

    /// Whether the render buffer should be multisampled.
    /// See also HdRenderBufferDescriptor#multiSampled.
    bool multiSampled;

    // ----------------------------------------------------------------
    // Renderpass binding parameters.
    // ----------------------------------------------------------------

    /// The clear value to apply to the render buffer before rendering.
    /// The type of "clearValue" should match the provided format.
    /// If clearValue is empty, no clear will be performed.
    /// See also HdRenderPassAovBinding#clearValue.
    VtValue clearValue;

    /// Extra settings for AOV rendering, such as pixel filtering options.
    /// See also HdRenderPassAovBinding#aovSettings.
    HdAovSettingsMap aovSettings;
};
typedef std::vector<HdAovDescriptor> HdAovDescriptorList;

/// \struct HdRenderBufferDescriptor
///
/// Describes the allocation structure of a render buffer bprim.
struct HdRenderBufferDescriptor {
    /// The width, height, and depth of the allocated render buffer.
    GfVec3i dimensions;

    /// The data format of the render buffer. See also HdAovDescriptor#format.
    HdFormat format;

    /// Whether the render buffer should be multisampled. See also
    /// HdAovDescriptor#multiSampled.
    bool multiSampled;

    bool operator==(HdRenderBufferDescriptor const& rhs) const {
        return dimensions == rhs.dimensions &&
               format == rhs.format && multiSampled == rhs.multiSampled;
    }
    bool operator!=(HdRenderBufferDescriptor const& rhs) const {
        return !(*this == rhs);
    }
};

/// \class HdRenderPassAovBinding
///
/// A renderpass AOV represents a binding of some output of the
/// rendering process to an output buffer.

class HdRenderBuffer;

struct HdRenderPassAovBinding {

    HdRenderPassAovBinding()
        : renderBuffer(nullptr) {}

    /// The identifier of the renderer output to be consumed. This should take
    /// a value from HdAovTokens.
    TfToken aovName;

    /// The render buffer to be bound to the above terminal output.
    ///
    /// From the app or scene, this can be specified as either a pointer or
    /// a path to a renderbuffer in the render index. If both are specified,
    /// the pointer is used preferentially.
    ///
    /// Note: hydra never takes ownership of the renderBuffer, but assumes it
    /// will be alive until the end of the renderpass, or whenever the buffer
    /// is marked converged, whichever is later.
    HdRenderBuffer* renderBuffer;

    /// The render buffer to be bound to the above terminal output.
    SdfPath renderBufferId;

    /// The clear value to apply to the bound render buffer, before rendering.
    /// The type of "clearValue" should match the type of the bound buffer.
    /// If clearValue is empty, it indicates no clear should be performed.
    /// See also HdAovDescriptor#clearValue.
    VtValue clearValue;

    /// Extra settings for AOV rendering, such as pixel filtering options.
    /// See also HdAovDescriptor#aovSettings.
    HdAovSettingsMap aovSettings;
};

typedef std::vector<HdRenderPassAovBinding> HdRenderPassAovBindingVector;

// VtValue requirements for HdRenderPassAovBinding
HD_API
std::ostream& operator<<(std::ostream& out,
    const HdRenderPassAovBinding& desc);
HD_API
bool operator==(const HdRenderPassAovBinding& lhs,
    const HdRenderPassAovBinding& rhs);
HD_API
bool operator!=(const HdRenderPassAovBinding& lhs,
    const HdRenderPassAovBinding& rhs);

/// \class HdParsedAovToken
///
/// Represents an AOV token which has been parsed to extract the prefix
/// (in the case of "primvars:"/"lpe:"/etc.).
struct HdParsedAovToken {
    HD_API
    HdParsedAovToken();
    HD_API
    HdParsedAovToken(TfToken const& aovName);

    TfToken name;
    bool isPrimvar : 1;
    bool isLpe : 1;
    bool isShader : 1;
};
typedef std::vector<HdParsedAovToken> HdParsedAovTokenVector;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_AOV_H
