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
#ifndef PCP_MAPFUNCTION_H
#define PCP_MAPFUNCTION_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/layerOffset.h"

#include <atomic>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// \class PcpMapFunction
///
/// A function that maps values from one namespace (and time domain) to
/// another. It represents the transformation that an arc such as a reference
/// arc applies as it incorporates values across the arc.
///
/// Take the example of a reference arc, where a source path
/// \</Model\> is referenced as a target path, \</Model_1\>.
/// The source path \</Model\> is the source of the opinions;
/// the target path \</Model_1\> is where they are incorporated in the scene.
/// Values in the model that refer to paths relative to \</Model\> must be
/// transformed to be relative to \</Model_1\> instead.
/// The PcpMapFunction for the arc provides this service.
///
/// Map functions have a specific \em domain, or set of values they can
/// operate on.  Any values outside the domain cannot be mapped.
/// The domain precisely tracks what areas of namespace can be
/// referred to across various forms of arcs.
///
/// Map functions can be chained to represent a series of map
/// operations applied in sequence.  The map function represent the
/// cumulative effect as efficiently as possible.  For example, in
/// the case of a chained reference from \</Model\> to \</Model\>
/// to \</Model\> to \</Model_1\>, this is effectively the same as
/// a mapping directly from \</Model\> to \</Model_1\>.  Representing
/// the cumulative effect of arcs in this way is important for
/// handling larger scenes efficiently.
///
/// Map functions can be \em inverted. Formally, map functions are
/// bijections (one-to-one and onto), which ensures that they can
/// be inverted.  Put differently, no information is lost by applying
/// a map function to set of values within its domain; they retain
/// their distinct identities and can always be mapped back.
///
/// One analogy that may or may not be helpful:
/// In the same way a geometric transform maps a model's points in its
/// rest space into the world coordinates for a particular instance,
/// a PcpMapFunction maps values about a referenced model into the
/// composed scene for a particular instance of that model. But rather
/// than translating and rotating points, the map function shifts the
/// values in namespace (and time).
///
///
class PcpMapFunction
{
public:
    /// A mapping from path to path.
    typedef std::map<SdfPath, SdfPath, SdfPath::FastLessThan> PathMap;
    typedef std::pair<SdfPath, SdfPath> PathPair;
    typedef std::vector<PathPair> PathPairVector;

    /// Construct a null function.
    PcpMapFunction() = default;

    /// Constructs a map function with the given arguments.
    /// Returns a null map function on error (see IsNull()).
    ///
    /// \param sourceToTargetMap The map from source paths to target paths.
    /// \param offset The time offset to apply from source to target.
    ///
    PCP_API
    static PcpMapFunction 
    Create(const PathMap &sourceToTargetMap,
           const SdfLayerOffset &offset);

    /// Construct an identity map function.
    PCP_API
    static const PcpMapFunction &Identity();

    /// Returns an identity path mapping.
    PCP_API
    static const PathMap &IdentityPathMap();
    
    /// Swap the contents of this map function with \p map.
    PCP_API
    void Swap(PcpMapFunction &map);
    void swap(PcpMapFunction &map) { Swap(map); }

    /// Equality.
    PCP_API
    bool operator==(const PcpMapFunction &map) const;

    /// Inequality.
    PCP_API
    bool operator!=(const PcpMapFunction &map) const;

    /// Return true if this map function is the null function.
    /// For a null function, MapSourceToTarget() always returns an empty path.
    PCP_API
    bool IsNull() const;

    /// Return true if the map function is the identity function.
    /// For identity, MapSourceToTarget() always returns the path unchanged.
    PCP_API
    bool IsIdentity() const;

    /// Return true if the map function maps the absolute root path to the
    /// absolute root path, false otherwise.
    bool HasRootIdentity() const { return _data.hasRootIdentity; }

    /// Map a path in the source namespace to the target.
    /// If the path is not in the domain, returns an empty path.
    PCP_API
    SdfPath MapSourceToTarget(const SdfPath &path) const;

    /// Map a path in the target namespace to the source.
    /// If the path is not in the co-domain, returns an empty path.
    PCP_API
    SdfPath MapTargetToSource(const SdfPath &path) const;

    /// Compose this map over the given map function.
    /// The result will represent the application of f followed by
    /// the application of this function.
    PCP_API
    PcpMapFunction Compose(const PcpMapFunction &f) const;

    /// Return the inverse of this map function.
    /// This returns a true inverse \p inv: for any path p in this function's
    /// domain that it maps to p', inv(p') -> p.
    PCP_API
    PcpMapFunction GetInverse() const;

    /// The set of path mappings, from source to target.
    PCP_API
    PathMap GetSourceToTargetMap() const;

    /// The time offset of the mapping.
    const SdfLayerOffset &GetTimeOffset() const { return _offset; }

    /// Returns a string representation of this mapping for debugging
    /// purposes.
    PCP_API
    std::string GetString() const;

    /// Return a size_t hash for this map function.
    PCP_API
    size_t Hash() const;

private:

    PCP_API
    PcpMapFunction(PathPair const *sourceToTargetBegin,
                   PathPair const *sourceToTargetEnd,
                   SdfLayerOffset offset,
                   bool hasRootIdentity);

private:
    friend PcpMapFunction *Pcp_MakeIdentity();
    
    static const int _MaxLocalPairs = 2;
    struct _Data final {
        _Data() {};

        _Data(PathPair const *begin, PathPair const *end, bool hasRootIdentity)
            : numPairs(end-begin)
            , hasRootIdentity(hasRootIdentity) {
            if (numPairs == 0)
                return;
            if (numPairs <= _MaxLocalPairs) {
                std::uninitialized_copy(begin, end, localPairs);
            }
            else {
                new (&remotePairs) std::shared_ptr<PathPair>(
                    new PathPair[numPairs], std::default_delete<PathPair[]>());
                std::copy(begin, end, remotePairs.get());
            }
        }
        
        _Data(_Data const &other)
            : numPairs(other.numPairs)
            , hasRootIdentity(other.hasRootIdentity) {
            if (numPairs <= _MaxLocalPairs) {
                std::uninitialized_copy(
                    other.localPairs,
                    other.localPairs + other.numPairs, localPairs);
            }
            else {
                new (&remotePairs) std::shared_ptr<PathPair>(other.remotePairs);
            }
        }
        _Data(_Data &&other)
            : numPairs(other.numPairs)
            , hasRootIdentity(other.hasRootIdentity) {
            if (numPairs <= _MaxLocalPairs) {
                PathPair *dst = localPairs;
                PathPair *src = other.localPairs;
                PathPair *srcEnd = other.localPairs + other.numPairs;
                for (; src != srcEnd; ++src, ++dst) {
                    ::new (static_cast<void*>(std::addressof(*dst)))
                        PathPair(std::move(*src));
                }
            }
            else {
                new (&remotePairs)
                    std::shared_ptr<PathPair>(std::move(other.remotePairs));
            }
        }
        _Data &operator=(_Data const &other) {
            if (this != &other) {
                this->~_Data();
                new (this) _Data(other);
            }
            return *this;
        }
        _Data &operator=(_Data &&other) {
            if (this != &other) {
                this->~_Data();
                new (this) _Data(std::move(other));
            }
            return *this;
        }
        ~_Data() {
            if (numPairs <= _MaxLocalPairs) {
                for (PathPair *p = localPairs; numPairs--; ++p) {
                    p->~PathPair();
                }
            }
            else {
                remotePairs.~shared_ptr<PathPair>();
            }
        }

        bool IsNull() const {
            return numPairs == 0 && !hasRootIdentity;
        }

        PathPair const *begin() const {
            return numPairs <= _MaxLocalPairs ? localPairs : remotePairs.get();
        }

        PathPair const *end() const {
            return begin() + numPairs;
        }

        bool operator==(_Data const &other) const {
            return numPairs == other.numPairs &&
                hasRootIdentity == other.hasRootIdentity &&
                std::equal(begin(), end(), other.begin());
        }

        bool operator!=(_Data const &other) const {
            return !(*this == other);
        }

        union {
            PathPair localPairs[_MaxLocalPairs > 0 ? _MaxLocalPairs : 1];
            std::shared_ptr<PathPair> remotePairs;
        };
        typedef int PairCount;
        PairCount numPairs = 0;
        bool hasRootIdentity = false;
    };

    _Data _data;
    SdfLayerOffset _offset;
};

// Specialize hash_value for PcpMapFunction.
inline
size_t hash_value(const PcpMapFunction& x)
{
    return x.Hash();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_MAPFUNCTION_H
