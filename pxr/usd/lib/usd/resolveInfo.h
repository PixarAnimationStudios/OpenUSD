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
#ifndef USD_RESOLVE_INFO_H
#define USD_RESOLVE_INFO_H

#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/declarePtrs.h"

#include <limits>

TF_DECLARE_WEAK_PTRS(PcpLayerStack);

/// \enum Usd_ResolveInfoSource
/// Describes the various sources of attribute values.
enum Usd_ResolveInfoSource
{
    Usd_ResolveInfoSourceNone,        //< No value

    Usd_ResolveInfoSourceFallback,    //< Built-in fallback value
    Usd_ResolveInfoSourceDefault,     //< Attribute default value
    Usd_ResolveInfoSourceTimeSamples, //< Attribute time samples
    Usd_ResolveInfoSourceValueClips,  //< Value clips
};

/// \class Usd_ResolveInfo
/// Container for information about the source of an attribute's
/// value, i.e. the 'resolved' location of the attribute.
struct Usd_ResolveInfo
{
public:
    Usd_ResolveInfo()
        : source(Usd_ResolveInfoSourceNone)
        , layerIndex(std::numeric_limits<size_t>::max())
        , valueIsBlocked(false)
    {
    }
    
    /// The source of the associated attribute's value.
    Usd_ResolveInfoSource source;

    /// The LayerStack that provides the strongest value opinion. 
    /// 
    /// If \p source is either \p Usd_ResolveInfoSourceDefault
    /// or \p Usd_ResolveInfoTimeSamples, the source will be a layer
    /// in this LayerStack (\sa layerIndex). 
    ///
    /// If \p source is Usd_ResolveInfoSourceValueClips, the source clips 
    /// will have been introduced in this LayerStack.
    ///
    /// Otherwise, this LayerStack will be invalid.
    PcpLayerStackPtr layerStack;

    /// The path to the prim that owns the attribute to query in
    /// \p layerStack to retrieve the strongest value opinion.
    ///
    /// If \p source is either \p Usd_ResolveInfoSourceDefault or
    /// \p Usd_ResolveInfoTimeSamples, this is the path to the prim
    /// specs in \p layerStack that own the attribute spec containing
    /// strongest value opinion.
    ///
    /// If \p source is Usd_ResolveInfoSourceValueClips, this is the
    /// path to the prim that should be used to query clips for attribute
    /// values.
    SdfPath primPathInLayerStack;

    /// The index of the layer in \p layerStack that provides the
    /// strongest time sample or default opinion. 
    ///
    /// This is valid only if \p source is either 
    /// \p Usd_ResolveInfoSourceDefault or \p Usd_ResolveInfoTimeSamples.
    size_t layerIndex;

    /// If \p source is \p Usd_ResolveInfoTimeSamples, the time 
    /// offset that maps a given time to the times in the layer
    /// containing the strongest time sample values.  Otherwise,
    /// this will be the identity offset.
    SdfLayerOffset offset;

    /// If \p source is \p Usd_ResolveInfoSourceNone or 
    /// \p Usd_ResolveInfoSourceFallback, this indicates whether or not
    /// this due to the value being blocked.
    bool valueIsBlocked;
};

#endif // USD_RESOLVE_INFO_H
