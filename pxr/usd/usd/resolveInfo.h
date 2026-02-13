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
#include <memory>
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
        , _defaultCanCompose(false)
        , _defaultCanComposeOverWeakerTimeVaryingSources(false)
    {
    }

    /// Return some information about the source of the associated attribute's
    /// value.  Note that if the attribute's value composes over other values,
    /// there may be more than a single source that varies over time.  For
    /// example, a VtArray-valued attribute may have a resolved value composed
    /// of a stronger VtArrayEdit over a weaker VtArray.
    ///
    /// A call to UsdAttribute::GetResolveInfo() with no arguments produces a
    /// UsdResolveInfo that only contains the proximal potential value source.
    /// In many cases this is the only source, but for more complex scenarios,
    /// call the overload of UsdAttribute::GetResolveInfo() that takes a time.
    /// Then if there is more than one source, the weaker sources can by
    /// accessed by calling HasWeakerInfo() and GetNextWeakerInfo().
    ///
    /// Spline value types (scalar floating point values) can never compose, so
    /// UsdResolveInfoSourceSpline instances never have weaker resolve info.
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

    /// Return true if the resolve info value source might be time-varying;
    /// false otherwise.  A return of true means that the value may or may not
    /// actually be time-varying.  A return of false means the the value is
    /// definitely not time-varying.  This is meant to enable optimizations for
    /// scene-data consumers like renderers, when they can handle non-varying
    /// values more efficiently.
    ///
    /// Note that this is different from UsdAttribute::ValueMightBeTimeVarying()
    /// which provides more granular answer since it has additional context from
    /// the attribute itself.
    bool ValueSourceMightBeTimeVarying() const {
        if (_source == UsdResolveInfoSourceTimeSamples ||
            _source == UsdResolveInfoSourceSpline ||
            _source == UsdResolveInfoSourceValueClips) {
            return true;
        }
        if (HasNextWeakerInfo()) {
            return GetNextWeakerInfo()->ValueSourceMightBeTimeVarying();
        }
        return _source == UsdResolveInfoSourceDefault &&
            _defaultCanComposeOverWeakerTimeVaryingSources;
    }

    /// If this object was returned by a call to the UsdAttribute::GetResolve()
    /// overload that takes a `time`, then this function returns true if the
    /// attribute's value comes from more than a single source.  For example, a
    /// VtIntArray-valued attribute that has a default authored in a weaker
    /// layer, and a VtIntArrayEdit default authored in a stronger layer will
    /// return true.  This object's value source will indicate the stronger
    /// array edit source, and calling GetNextWeakerInfo() will return a pointer
    /// to a resolve info indicating the weaker VtArray source.
    bool HasNextWeakerInfo() const {
        return static_cast<bool>(_nextWeaker);
    }

    /// If this object was returned by a call to the UsdAttribute::GetResolve()
    /// overload that takes a `time` and the attribute's value comes from more
    /// than a single source, return a pointer to a UsdResolveInfo indicating
    /// the next weaker source.  If many sources are involved, the chain will
    /// continue to indicate all the value sources.
    UsdResolveInfo const *GetNextWeakerInfo() const {
        return _nextWeaker.get();
    }

private:

    // Helper to chain an additional weaker info onto this one.
    USD_API
    UsdResolveInfo *_AddNextWeakerInfo();
    
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

    /// If the source is UsdResolveInfoSourceSpline, then _spline represents the
    /// underlying spline data. If not, this will be nullopt.
    std::optional<TsSpline> _spline;

    /// If the resolved value is possibly composed from several authored values,
    /// this points to the next weaker source of values.
    std::shared_ptr<UsdResolveInfo> _nextWeaker;
    
    /// The source of the associated attribute's value.
    UsdResolveInfoSource _source;

    /// If \p source is \p UsdResolveInfoSourceNone or 
    /// \p UsdResolveInfoSourceFallback, this indicates whether or not
    /// this due to the value being blocked.
    bool _valueIsBlocked;

    /// Set to true if \p source is \p UsdResolveInfoSourceDefault and the
    /// default value can compose.
    bool _defaultCanCompose;
    
    /// Set to true if \p source is \p UsdResolveInfoSourceDefault and the
    /// default value can compose, and there were weaker time varying sources
    /// (clips or samples).
    bool _defaultCanComposeOverWeakerTimeVaryingSources;

    friend class UsdAttribute;
    friend class UsdAttributeQuery;
    friend class UsdStage;
    friend class UsdStage_ResolveInfoAccess;
    friend class Usd_Resolver;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_RESOLVE_INFO_H
