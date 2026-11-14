//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_INSTANCE_KEY_H
#define PXR_USD_PCP_INSTANCE_KEY_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/pcp/types.h"

#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/base/tf/hash.h"

#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PcpPrimIndex;

/// \class PcpInstanceKey
///
/// A PcpInstanceKey identifies instanceable prim indexes that share the
/// same set of opinions. Instanceable prim indexes with equal instance
/// keys are guaranteed to have the same opinions for name children and
/// properties beneath those name children. They are NOT guaranteed to have
/// the same opinions for direct properties of the prim indexes themselves.
///
/// Target and connection paths beneath key-equal instances correspond by
/// replacing one instance's path with the other's for paths within the
/// instances, and by identity for paths outside them, provided that each
/// composed path outside the instances is mapped to the root alike by every
/// node that contributes it.  Key-equal instances may map such paths
/// differently, for example the same asset instanced in two sets with a
/// target into the set, so their targets outside the instances need not
/// correspond this way.
///
class PcpInstanceKey
{
public:
    PCP_API
    PcpInstanceKey();

    /// Create an instance key for the given prim index.
    PCP_API
    explicit PcpInstanceKey(const PcpPrimIndex& primIndex);

    /// Comparison operators
    PCP_API
    bool operator==(const PcpInstanceKey& rhs) const;
    PCP_API
    bool operator!=(const PcpInstanceKey& rhs) const;

    /// Appends hash value for this instance key.
    template <typename HashState>
    friend void TfHashAppend(HashState& h, const PcpInstanceKey& key)
    {
        h.Append(key._hash);
    }
    /// Returns hash value for this instance key.
    friend size_t hash_value(const PcpInstanceKey& key) 
    {
        return key._hash;
    }

    /// \struct Hash
    ///
    /// Hash functor.
    ///
    struct Hash {
        inline size_t operator()(const PcpInstanceKey& key) const
        {
            return key._hash;
        }
    };

    /// Returns string representation of this instance key
    /// for debugging purposes.
    PCP_API
    std::string GetString() const;

private:
    struct _Arc
    {
        explicit _Arc(const PcpNodeRef& node);

        bool operator==(const _Arc& rhs) const
        {
            return _arcType == rhs._arcType    &&
                _sourceSite == rhs._sourceSite &&
                _timeOffset == rhs._timeOffset &&
                _domain == rhs._domain;
        }

        template <typename HashState>
        friend void TfHashAppend(HashState &h, const _Arc& arc) {
            h.Append(arc._arcType);
            h.Append(arc._sourceSite);
            h.Append(arc._timeOffset);
            h.Append(arc._domain);
        }

        PcpArcType _arcType;
        PcpSite _sourceSite;
        SdfLayerOffset _timeOffset;
        // The node's map to the root, up to translation between instances:
        // each source path whose image differs from the one its nearest
        // ancestor entry implies, with that image classified as blocked
        // (empty), within the instance (relative to the instance's path), or
        // outside it (the absolute root path).  Images outside the instance
        // are not distinguished, so key-equal instances may compose
        // different targets outside the instance, which Usd's prototype
        // translation does not account for; see PcpInstanceKey.
        std::vector<std::pair<SdfPath, SdfPath>> _domain;
    };
    std::vector<_Arc> _arcs;

    typedef std::pair<std::string, std::string> _VariantSelection;
    std::vector<_VariantSelection> _variantSelection;

    size_t _hash;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_INSTANCE_KEY_H
