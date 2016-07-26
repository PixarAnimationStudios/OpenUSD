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

#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/declarePtrs.h"
#include <boost/noncopyable.hpp>

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
    public TfRefBase, public TfWeakBase, boost::noncopyable
{
public:
    /// Decorate the SdfLayer arguments \p args with additional arguments
    /// that will be used when opening the layer specified in the payload 
    /// \p payload.
    void DecoratePayload(
        const SdfPayload& payload, 
        const PcpPayloadContext& context,
        SdfLayer::FileFormatArguments* args);

    /// Return true if the scene description field \p field in layer 
    /// \p layer may affect the decoration of payloads, false otherwise.
    ///
    /// This is used during change processing to determine whether a scene
    /// description change affects a prim's payload arcs.
    bool IsFieldRelevantForDecoration(
        const SdfLayerHandle& layer,
        const SdfPath& path,
        const TfToken& field);

protected:
    PcpPayloadDecorator();
    virtual ~PcpPayloadDecorator();

    /// Virtual implementation functions. See corresponding public
    /// API for documentation.
    virtual void _DecoratePayload(
        const SdfPayload& payload, 
        const PcpPayloadContext& context,
        SdfLayer::FileFormatArguments* args) = 0;

    virtual bool _IsFieldRelevantForDecoration(
        const SdfLayerHandle& layer,
        const SdfPath& path,
        const TfToken& field) = 0;
};

#endif // PCP_PAYLOAD_DECORATOR_H
