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
#ifndef PCP_INSTANCE_KEY_H
#define PCP_INSTANCE_KEY_H

#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/site.h"
#include "pxr/usd/pcp/types.h"

#include "pxr/usd/sdf/layerOffset.h"

#include <boost/functional/hash.hpp>

#include <string>
#include <utility>
#include <vector>

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
    PcpInstanceKey();

    /// Create an instance key for the given prim index.
    explicit PcpInstanceKey(const PcpPrimIndex& primIndex);

    /// Comparison operators
    bool operator==(const PcpInstanceKey& rhs) const;
    bool operator!=(const PcpInstanceKey& rhs) const;

    /// Returns hash value for this instance key.
    friend size_t hash_value(const PcpInstanceKey& key) 
    {
        return key._hash;
    }

    /// Hash functor
    struct Hash {
        inline size_t operator()(const PcpInstanceKey& key) const
        {
            return key._hash;
        }
    };

    /// Returns string representation of this instance key
    /// for debugging purposes.
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
            return _arcType == rhs._arcType and
            _sourceSite == rhs._sourceSite and
            _timeOffset == rhs._timeOffset;
        }

        size_t GetHash() const
        {
            size_t hash = _arcType;
            boost::hash_combine(hash, _sourceSite);
            boost::hash_combine(hash, _timeOffset.GetHash());
            return hash;
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

#endif // PCP_INSTANCE_KEY_H
