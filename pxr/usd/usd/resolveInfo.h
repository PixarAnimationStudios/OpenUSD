//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_RESOLVE_INFO_H
#define PXR_USD_USD_RESOLVE_INFO_H

/// \file usd/resolveInfo.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/ts/spline.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/pcp/node.h"

#include "pxr/base/tf/declarePtrs.h"

#include <limits>
#include <optional>

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
    UsdResolveInfoSourceSpline,          ///< Spline value
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
            _source == UsdResolveInfoSourceSpline ||
            _valueIsBlocked;
    }

    /// Return true if this UsdResolveInfo represents an attribute that has an
    /// authored value that is not \ref Usd_AttributeBlocking "blocked"
    bool HasAuthoredValue() const {
        return
            _source == UsdResolveInfoSourceDefault ||
            _source == UsdResolveInfoSourceTimeSamples ||
            _source == UsdResolveInfoSourceValueClips ||
            _source == UsdResolveInfoSourceSpline;
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

    /// Returns true if the resolve info source might be time-varying; false
    /// otherwise.
    ///
    /// Note that this is different from UsdAttribute::ValueMightBeTimeVarying()
    /// which provides more granular answer since it has additional context from
    /// the attribute itself.
    bool ValueSourceMightBeTimeVarying() const {
        return _source == UsdResolveInfoSourceTimeSamples ||
            _source == UsdResolveInfoSourceSpline ||
            _source == UsdResolveInfoSourceValueClips;
    }

private:
    /// The LayerStack that provides the strongest value opinion. 
    /// 
    /// If \p source is either \p UsdResolveInfoSourceDefault
    /// or \p UsdResolveInfoSourceTimeSamples or \p UsdResolveInfoSourceSpline, 
    /// the source will be a layer in this LayerStack (\sa _layer). 
    ///
    /// If \p source is UsdResolveInfoSourceValueClips, the source clips 
    /// will have been introduced in this LayerStack.
    ///
    /// Otherwise, this LayerStack will be invalid.
    PcpLayerStackPtr _layerStack;

    /// The layer in \p layerStack that provides the strongest time sample or
    /// default opinion.
    ///
    /// This is valid only if \p source is either 
    /// \p UsdResolveInfoSourceDefault or \p UsdResolveInfoSourceTimeSamples or
    /// \p UsdResolveInfoSourceSpline.
    SdfLayerHandle _layer;

    /// The node within the containing PcpPrimIndex that provided
    /// the strongest value opinion.
    PcpNodeRef _node;

    /// If \p source is \p UsdResolveInfoTimeSamples, the time 
    /// offset that maps time in the strongest resolved layer
    /// to the stage.
    /// If no offset applies, this will be the identity offset.
    SdfLayerOffset _layerToStageOffset;

    /// The path to the prim that owns the attribute to query in
    /// \p layerStack to retrieve the strongest value opinion.
    ///
    /// If \p source is either \p UsdResolveInfoSourceDefault or
    /// \p UsdResolveInfoTimeSamples or \p UsdResolveInfoSourceSpline, this is 
    /// the path to the prim specs in \p layerStack that own the attribute spec 
    /// containing strongest value opinion.
    ///
    /// If \p source is UsdResolveInfoSourceValueClips, this is the
    /// path to the prim that should be used to query clips for attribute
    /// values.
    SdfPath _primPathInLayerStack;

    /// The source of the associated attribute's value.
    UsdResolveInfoSource _source;

    /// If the source is UsdResolveInfoSourceSpline, then _spline represents the
    /// underlying spline data. If not, this will be nullopt.
    std::optional<TsSpline> _spline;

    /// If \p source is \p UsdResolveInfoSourceNone or 
    /// \p UsdResolveInfoSourceFallback, this indicates whether or not
    /// this due to the value being blocked.
    bool _valueIsBlocked;

    friend class UsdAttribute;
    friend class UsdStage;
    friend class UsdStage_ResolveInfoAccess;
    friend class UsdAttributeQuery;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_RESOLVE_INFO_H
