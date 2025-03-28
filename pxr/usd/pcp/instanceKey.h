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
    struct _Collector;

    struct _Arc
    {
        explicit _Arc(const PcpNodeRef& node)
            : _arcType(node.GetArcType())
            , _sourceSite(node.GetSite())
            , _timeOffset(node.GetMapToRoot().GetTimeOffset())
        { 
        }

        bool operator==(const _Arc& rhs) const
        {
            return _arcType == rhs._arcType    &&
                _sourceSite == rhs._sourceSite &&
                _timeOffset == rhs._timeOffset;
        }

        template <typename HashState>
        friend void TfHashAppend(HashState &h, const _Arc& arc) {
            h.Append(arc._arcType);
            h.Append(arc._sourceSite);
            h.Append(arc._timeOffset);
        }

        PcpArcType _arcType;
        PcpSite _sourceSite;
        SdfLayerOffset _timeOffset;
    };
    std::vector<_Arc> _arcs;

    typedef std::pair<std::string, std::string> _VariantSelection;
    std::vector<_VariantSelection> _variantSelection;

    size_t _hash;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_INSTANCE_KEY_H
