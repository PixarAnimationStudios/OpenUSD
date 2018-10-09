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
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

/// A bundle of state describing an AOV ("Alternate Output Value") display
/// channel. Note that in hydra API, this data is split between
/// HdRenderPassAttachment and HdRenderBufferDescriptor. This class is
/// provided for use in higher level application-facing API.
struct HdAovDescriptor
{
    HdAovDescriptor()
        : clearValue(), format(HdFormatInvalid), multiSampled(false) {}
    HdAovDescriptor(VtValue const& c, HdFormat f, bool ms)
        : clearValue(c), format(f), multiSampled(ms) {}

    /// The clear value to apply to the render buffer before rendering.
    /// The type of "clearValue" should match the provided format.
    /// If clearValue is empty, no clear will be performed.
    VtValue clearValue;

    /// The AOV output format.
    HdFormat format;

    /// Whether the render buffer should be multisampled.
    bool multiSampled;
};
typedef std::vector<HdAovDescriptor> HdAovDescriptorList;

/// \class HdAovIdentifier
///
/// Represents a parsed AOV identifier.
struct HdAovIdentifier {
    HD_API
    HdAovIdentifier(TfToken const& aovName);

    HdAovIdentifier()
        : name(), isPrimvar(false), isLpe(false), isShader(false) {}

    TfToken name;
    bool isPrimvar : 1;
    bool isLpe : 1;
    bool isShader : 1;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_AOV_H
