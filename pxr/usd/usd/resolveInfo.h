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
#ifndef PXR_USD_USD_RESOLVE_INFO_H
#define PXR_USD_USD_RESOLVE_INFO_H

/// \file usd/resolveInfo.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/pcp/node.h"

#include "pxr/base/tf/declarePtrs.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_PTRS(PcpLayerStack);

/// \enum UsdResolveInfoSource
///
/// Describes the various sources of attribute values.
///
/// For more details, see \ref Usd_ValueResolution.
///
enum UsdResolveInfoSource
{
    UsdResolveInfoSourceNone,            ///< No value

    UsdResolveInfoSourceFallback,        ///< Built-in fallback value
    UsdResolveInfoSourceDefault,         ///< Attribute default value
    UsdResolveInfoSourceTimeSamples,     ///< Attribute time samples
    UsdResolveInfoSourceValueClips,      ///< Value clips
};

/// \class UsdResolveInfo
///
/// Container for information about the source of an attribute's value, i.e.
/// the 'resolved' location of the attribute.
///
/// For more details, see \ref Usd_ValueResolution.
///
class UsdResolveInfo
{
public:
    UsdResolveInfo()
        : _source(UsdResolveInfoSourceNone)
        , _layerIndex(std::numeric_limits<size_t>::max())
        , _valueIsBlocked(false)
    {
    }

    /// Return the source of the associated attribute's value.
    UsdResolveInfoSource GetSource() const {
        return _source;
    }
    
    /// Return true if this UsdResolveInfo represents an attribute that has an
    /// authored value opinion.  This will return `true` if there is *any*
    /// authored value opinion, including a \ref Usd_AttributeBlocking "block"
    ///
    /// This is equivalent to `HasAuthoredValue() || ValueIsBlocked()`
    bool HasAuthoredValueOpinion() const {
        return
            _source == UsdResolveInfoSourceDefault ||
            _source == UsdResolveInfoSourceTimeSamples ||
            _source == UsdResolveInfoSourceValueClips ||
            _valueIsBlocked;
    }

    /// Return true if this UsdResolveInfo represents an attribute that has an
    /// authored value that is not \ref Usd_AttributeBlocking "blocked"
    bool HasAuthoredValue() const {
        return
            _source == UsdResolveInfoSourceDefault ||
            _source == UsdResolveInfoSourceTimeSamples ||
            _source == UsdResolveInfoSourceValueClips;
    }

    /// Return the node within the containing PcpPrimIndex that provided
    /// the resolved value opinion.
    PcpNodeRef GetNode() const {
        return _node;
    }

    /// Return true if this UsdResolveInfo represents an attribute whose
    /// value is blocked.
    ///
    /// \see UsdAttribute::Block()
    bool ValueIsBlocked() const {
        return _valueIsBlocked;
    }
    
private:
    /// The source of the associated attribute's value.
    UsdResolveInfoSource _source;

    /// The LayerStack that provides the strongest value opinion. 
    /// 
    /// If \p source is either \p UsdResolveInfoSourceDefault
    /// or \p UsdResolveInfoTimeSamples, the source will be a layer
    /// in this LayerStack (\sa layerIndex). 
    ///
    /// If \p source is UsdResolveInfoSourceValueClips, the source clips 
    /// will have been introduced in this LayerStack.
    ///
    /// Otherwise, this LayerStack will be invalid.
    PcpLayerStackPtr _layerStack;

    /// The path to the prim that owns the attribute to query in
    /// \p layerStack to retrieve the strongest value opinion.
    ///
    /// If \p source is either \p UsdResolveInfoSourceDefault or
    /// \p UsdResolveInfoTimeSamples, this is the path to the prim
    /// specs in \p layerStack that own the attribute spec containing
    /// strongest value opinion.
    ///
    /// If \p source is UsdResolveInfoSourceValueClips, this is the
    /// path to the prim that should be used to query clips for attribute
    /// values.
    SdfPath _primPathInLayerStack;

    /// The index of the layer in \p layerStack that provides the
    /// strongest time sample or default opinion. 
    ///
    /// This is valid only if \p source is either 
    /// \p UsdResolveInfoSourceDefault or \p UsdResolveInfoTimeSamples.
    size_t _layerIndex;

    /// If \p source is \p UsdResolveInfoTimeSamples, the time 
    /// offset that maps time in the strongest resolved layer
    /// to the stage.
    /// If no offset applies, this will be the identity offset.
    SdfLayerOffset _layerToStageOffset;

    /// If \p source is \p UsdResolveInfoSourceNone or 
    /// \p UsdResolveInfoSourceFallback, this indicates whether or not
    /// this due to the value being blocked.
    bool _valueIsBlocked;

    /// The node within the containing PcpPrimIndex that provided
    /// the strongest value opinion.
    PcpNodeRef _node;

    friend class UsdAttribute;
    friend class UsdStage;
    friend class UsdStage_ResolveInfoAccess;
    friend class UsdAttributeQuery;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_RESOLVE_INFO_H
