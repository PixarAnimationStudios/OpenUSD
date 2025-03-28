//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_MAP_FUNCTION_H
#define PXR_USD_PCP_MAP_FUNCTION_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/layerOffset.h"
#include "pxr/usd/sdf/pathExpression.h"

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
    /// The identity function has an identity path mapping and time offset.
    PCP_API
    bool IsIdentity() const;
    
    /// Return true if the map function uses the identity path mapping.
    /// If true, MapSourceToTarget() always returns the path unchanged.
    /// However, this map function may have a non-identity time offset.
    PCP_API
    bool IsIdentityPathMapping() const;

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

    /// Map all path pattern prefix paths and expression reference paths in the
    /// source namespace to the target.  For any references or patterns with
    /// prefix paths that are not in the domain, replace with an
    /// SdfPathPattern::Nothing() subexpression, to be simplified.
    ///
    /// For example, if the mapping specifies /Foo -> /World/Foo_1, and the
    /// expression is '/Foo/Bar//Baz + /Something/Else//Entirely', the resulting
    /// expression will be '/World/Foo_1/Bar//Baz', since the
    /// /Something/Else prefix is outside the domain.
    ///
    /// If \p excludedPatterns and/or \p excludedReferences are supplied, they
    /// are populated with those patterns & references that could not be
    /// translated and were replaced with SdfPathPattern::Nothing().
    PCP_API
    SdfPathExpression
    MapSourceToTarget(
        const SdfPathExpression &pathExpr,
        std::vector<SdfPathExpression::PathPattern>
            *unmappedPatterns = nullptr,
        std::vector<SdfPathExpression::ExpressionReference>
            *unmappedRefs = nullptr
        ) const;

    /// Map all path pattern prefix paths and expression reference paths in the
    /// target namespace to the source.  For any references or patterns with
    /// prefix paths that are not in the co-domain, replace with an
    /// SdfPathPattern::Nothing() subexpression, to be simplified.
    ///
    /// For example, if the mapping specifies /World/Foo_1 -> /Foo, and the
    /// expression is '/World/Foo_1/Bar//Baz + /World/Bar//', the resulting
    /// expression will be '/Foo/Bar//Baz', since the /World/Bar prefix is
    /// outside the co-domain.
    ///
    /// If \p excludedPatterns and/or \p excludedReferences are supplied, they
    /// are populated with those patterns & references that could not be
    /// translated and were replaced with SdfPathPattern::Nothing().
    PCP_API
    SdfPathExpression
    MapTargetToSource(
        const SdfPathExpression &pathExpr,
        std::vector<SdfPathExpression::PathPattern>
            *unmappedPatterns = nullptr,
        std::vector<SdfPathExpression::ExpressionReference>
            *unmappedRefs = nullptr
        ) const;
    
    /// Compose this map over the given map function.
    /// The result will represent the application of f followed by
    /// the application of this function.
    PCP_API
    PcpMapFunction Compose(const PcpMapFunction &f) const;

    /// Compose this map function over a hypothetical map function that has an
    /// identity path mapping and \p offset.  This is equivalent to building
    /// such a map function and invoking Compose(), but is faster.
    PCP_API
    PcpMapFunction ComposeOffset(const SdfLayerOffset &newOffset) const;

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

    PCP_API
    SdfPathExpression
    _MapPathExpressionImpl(
        bool invert,
        const SdfPathExpression &pathExpr,
        std::vector<SdfPathExpression::PathPattern> *unmappedPatterns,
        std::vector<SdfPathExpression::ExpressionReference> *unmappedRefs
        ) const;

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

        template <class HashState>
        friend void TfHashAppend(HashState &h, _Data const &data){
            h.Append(data.hasRootIdentity);
            h.Append(data.numPairs);
            h.AppendRange(std::begin(data), std::end(data));
        }

        union {
            PathPair localPairs[_MaxLocalPairs > 0 ? _MaxLocalPairs : 1];
            std::shared_ptr<PathPair> remotePairs;
        };
        typedef int PairCount;
        PairCount numPairs = 0;
        bool hasRootIdentity = false;
    };

    // Specialize TfHashAppend for PcpMapFunction.
    template <typename HashState>
    friend inline
    void TfHashAppend(HashState& h, const PcpMapFunction& x){
        h.Append(x._data);
        h.Append(x._offset);
    }

    _Data _data;
    SdfLayerOffset _offset;
};

// Specialize hash_value for PcpMapFunction.
inline
size_t hash_value(const PcpMapFunction& x)
{
    return TfHash{}(x);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_MAP_FUNCTION_H
