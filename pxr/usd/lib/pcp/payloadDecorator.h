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
#ifndef PCP_PAYLOAD_DECORATOR_H
#define PCP_PAYLOAD_DECORATOR_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

class PcpPayloadContext;
class SdfPayload;
TF_DECLARE_REF_PTRS(PcpPayloadDecorator);

/// \class PcpPayloadDecorator
///
/// PcpPayloadDecorator provides a way to specify additional information
/// to the prim indexing algorithm when it loads payload layers.
///
/// If a decorator has been specified as an prim indexing input,
/// it will be invoked whenever a payload arc is encountered. 
/// The decorator can then fill in an SdfLayer::FileFormatArguments object
/// with any information it wants. This information will be passed to 
/// SdfLayer::FindOrOpen when the layer is ultimately opened.
/// 
/// When processing a payload, the decorator can examine scene description
/// values from stronger nodes in the index via the supplied
/// PcpPayloadContext object. For instance, a decorator might use the
/// PcpPayloadContext to find the strongest available metadata value
/// authored on a prim, and use that to control its behavior.
///
/// Since decoration happens before the payload is actually loaded
/// it cannot examine locations introduced inside the payload. 
/// For example, if a payload introduces a class inherit,
/// the context will not be able to see values from class overrides that
/// are stronger than the payload.
///
class PcpPayloadDecorator :
    public TfRefBase, public TfWeakBase
{
public:
    // PcpPayloadDecorator needs to be noncopyable
    PCP_API
    PcpPayloadDecorator(const PcpPayloadDecorator&) = delete;

    PCP_API
    PcpPayloadDecorator& operator=(const PcpPayloadDecorator&) = delete;

    /// Decorate the SdfLayer arguments \p args with additional arguments
    /// that will be used when opening the layer specified in the payload 
    /// \p payload when composing the index at \p primIndexPath.
    PCP_API
    void DecoratePayload(
        const SdfPath& primIndexPath,
        const SdfPayload& payload, 
        const PcpPayloadContext& context,
        SdfLayer::FileFormatArguments* args);

    /// Return true if changes to the scene description field \p field
    /// may affect the decoration of payloads, false otherwise. 
    /// 
    /// If a change is made to a field for which this function returns
    /// true, IsFieldChangeRelevantForDecoration will be called during
    /// change processing to allow the decorator to determine if the
    /// change is relevant and requires affected prims to be recomposed.
    PCP_API
    bool IsFieldRelevantForDecoration(const TfToken& field);

    /// Return true if the change to scene description field \p field on
    /// the prim spec at \p sitePath in the layer \p siteLayer may affect the 
    /// decoration of payloads when composing the index at \p primIndexPath, 
    /// false otherwise. \p oldAndNewValues contain the old and new values 
    /// of the field.
    ///
    /// This is used during change processing to determine whether a scene
    /// description change affects a prim's payload arcs and requires the
    /// prim to be recomposed.
    PCP_API
    bool IsFieldChangeRelevantForDecoration(
        const SdfPath& primIndexPath,
        const SdfLayerHandle& siteLayer,
        const SdfPath& sitePath,
        const TfToken& field,
        const std::pair<VtValue, VtValue>& oldAndNewValues);

protected:
    PCP_API
    PcpPayloadDecorator();
    PCP_API
    virtual ~PcpPayloadDecorator();

    /// Virtual implementation functions. See corresponding public
    /// API for documentation.
    virtual void _DecoratePayload(
        const SdfPath& primIndexPath,
        const SdfPayload& payload, 
        const PcpPayloadContext& context,
        SdfLayer::FileFormatArguments* args) = 0;

    virtual bool _IsFieldRelevantForDecoration(
        const TfToken& field) = 0;

    virtual bool _IsFieldChangeRelevantForDecoration(
        const SdfPath& primIndexPath, 
        const SdfLayerHandle& siteLayer,
        const SdfPath& sitePath,
        const TfToken& field,
        const std::pair<VtValue, VtValue>& oldAndNewValues) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_PAYLOAD_DECORATOR_H
