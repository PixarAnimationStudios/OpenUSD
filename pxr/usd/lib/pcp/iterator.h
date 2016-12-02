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
#ifndef PCP_ITERATOR_H
#define PCP_ITERATOR_H

#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/api.h"

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/site.h"

#include "pxr/base/tf/iterator.h"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/reverse_iterator.hpp>

class PcpPrimIndex;
class PcpPrimIndex_Graph;
class PcpPropertyIndex;

/// \class PcpNodeIterator
///
/// Object used to iterate over nodes in the prim index graph in strong-to-weak
/// order.
///
class PcpNodeIterator
    : public boost::iterator_facade<
                 /* Derived =   */ PcpNodeIterator, 
                 /* ValueType = */ PcpNodeRef,
                 /* Category =  */ boost::random_access_traversal_tag,
                 /* RefType =   */ PcpNodeRef
             >
{
public:
    /// Constructs an invalid iterator.
    PCP_API
    PcpNodeIterator();

    // Returns a compressed Sd site.  For internal use only.
    Pcp_CompressedSdSite GetCompressedSdSite(size_t layerIndex) const
    {
        return Pcp_CompressedSdSite(_nodeIdx, layerIndex);
    }

private:
    friend class PcpPrimIndex;
    PCP_API
    PcpNodeIterator(PcpPrimIndex_Graph* graph, size_t nodeIdx);

    friend class boost::iterator_core_access;
    PCP_API
    void increment();
    PCP_API
    void decrement();
    PCP_API
    void advance(difference_type n);
    PCP_API
    difference_type distance_to(const PcpNodeIterator& other) const;
    PCP_API
    bool equal(const PcpNodeIterator& other) const;
    PCP_API
    reference dereference() const;

private:
    PcpPrimIndex_Graph* _graph;
    size_t _nodeIdx;
};

/// \class PcpNodeReverseIterator
///
/// Object used to iterate over nodes in the prim index graph in weak-to-strong
/// order.
///
class PcpNodeReverseIterator
    : public boost::reverse_iterator<PcpNodeIterator>
{
public:
    PcpNodeReverseIterator() { }
    explicit PcpNodeReverseIterator(const PcpNodeIterator& iter)
        : boost::reverse_iterator<PcpNodeIterator>(iter) { }
};

/// \class PcpPrimIterator
///
/// Object used to iterate over prim specs in the prim index graph in 
/// strong-to-weak order.
///
class PcpPrimIterator 
    : public boost::iterator_facade<
                 /* Derived  = */ PcpPrimIterator, 
                 /* Value    = */ SdfSite,
                 /* Category = */ boost::random_access_traversal_tag,
                 /* Ref      = */ SdfSite
             >
{
public:
    /// Constructs an invalid iterator.
    PCP_API
    PcpPrimIterator();

    /// Constructs a prim iterator beginning at position \p pos in the
    /// prim stack of prim index \p primIndex.
    PCP_API
    PcpPrimIterator(const PcpPrimIndex* primIndex, size_t pos);

    /// Returns the PcpNode from which the current prim originated.
    PCP_API
    PcpNodeRef GetNode() const;

    // Returns the \c Pcp_SdSiteRef from which the current prim originated.
    // For internal use only.
    PCP_API
    Pcp_SdSiteRef _GetSiteRef() const;

private:
    friend class boost::iterator_core_access;
    PCP_API void increment();
    PCP_API void decrement();
    PCP_API void advance(difference_type n);
    PCP_API difference_type distance_to(const PcpPrimIterator& other) const;
    PCP_API bool equal(const PcpPrimIterator& other) const;
    PCP_API reference dereference() const;

private:
    const PcpPrimIndex* _primIndex;
    size_t _pos;
};

/// \class PcpPrimReverseIterator
///
/// Object used to iterate over prim specs in the prim index graph in 
/// weak-to-strong order.
///
class PcpPrimReverseIterator
    : public boost::reverse_iterator<PcpPrimIterator>
{
public:
    PcpPrimReverseIterator() { }
    explicit PcpPrimReverseIterator(const PcpPrimIterator& iter)
        : boost::reverse_iterator<PcpPrimIterator>(iter) { }
        
    PcpNodeRef GetNode() const
    {
        PcpPrimIterator tmp = base();
        return (--tmp).GetNode();
    }

    Pcp_SdSiteRef _GetSiteRef() const
    {
        PcpPrimIterator tmp = base();
        return (--tmp)._GetSiteRef();
    }
};

/// \class PcpPropertyIterator
///
/// Object used to iterate over property specs in a property index in
/// strong-to-weak order.
///
class PcpPropertyIterator
    : public boost::iterator_facade<
                 /* Derived  = */ PcpPropertyIterator, 
                 /* Value    = */ const SdfPropertySpecHandle,
                 /* Category = */ boost::random_access_traversal_tag
             >
{
public:
    /// Constructs an invalid iterator.
    PCP_API
        PcpPropertyIterator();

    /// Constructs a property iterator for \p index beginning at position
    /// \p pos in the property stack.
    PCP_API
    PcpPropertyIterator(const PcpPropertyIndex& index, size_t pos = 0);

    /// Returns the PcpNode from which the current property originated.
    PCP_API PcpNodeRef GetNode() const;

    /// Returns true if the current property is local to the owning
    /// property index's layer stack, false otherwise.
    PCP_API bool IsLocal() const;

private:
    friend class boost::iterator_core_access;
    PCP_API void increment();
    PCP_API void decrement();
    PCP_API void advance(difference_type n);
    PCP_API difference_type distance_to(const PcpPropertyIterator& other) const;
    PCP_API bool equal(const PcpPropertyIterator& other) const;
    PCP_API reference dereference() const;

private:
    const PcpPropertyIndex* _propertyIndex;
    size_t _pos;
};

/// \class PcpPropertyReverseIterator
///
/// Object used to iterate over property specs in a property index in
/// weak-to-strong order.
///
class PcpPropertyReverseIterator
    : public boost::reverse_iterator<PcpPropertyIterator>
{
public:
    PcpPropertyReverseIterator() { }
    explicit PcpPropertyReverseIterator(const PcpPropertyIterator& iter)
        : boost::reverse_iterator<PcpPropertyIterator>(iter) { }
        
    PcpNodeRef GetNode() const
    {
        PcpPropertyIterator tmp = base();
        return (--tmp).GetNode();
    }

    bool IsLocal() const
    {
        PcpPropertyIterator tmp = base();
        return (--tmp).IsLocal();
    }
};

// Helper macro for defining iterator ranges, which are simply pairs of
// iterators denoting the [start, end) of a series of values. These ranges
// may be used with TF_FOR_ALL and TF_REVERSE_FOR_ALL.
#define PCP_DEFINE_RANGE(Range, Iterator, ReverseIterator)              \
    typedef std::pair<Iterator, Iterator> Range;                        \
                                                                        \
    inline Iterator begin(Range &range) { return range.first; }         \
    inline Iterator begin(const Range &range) { return range.first; }   \
    inline Iterator end(Range &range) { return range.second; }          \
    inline Iterator end(const Range &range) { return range.second; }    \
                                                                        \
    template <>                                                         \
    struct Tf_IteratorInterface<Range, false> {                         \
        typedef Iterator IteratorType;                                  \
        static IteratorType Begin(Range &c) { return c.first; }         \
        static IteratorType End(Range &c) { return c.second; }          \
    };                                                                  \
                                                                        \
    template <>                                                         \
    struct Tf_IteratorInterface<const Range, false> {                   \
        typedef Iterator IteratorType;                                  \
        static IteratorType Begin(Range const &c) { return c.first; }   \
        static IteratorType End(Range const &c) { return c.second; }    \
    };                                                                  \
                                                                        \
    template <>                                                         \
    struct Tf_IteratorInterface<Range, true> {                          \
        typedef ReverseIterator IteratorType;                           \
        static IteratorType Begin(Range &c)                             \
            { return IteratorType(c.second); }                          \
        static IteratorType End(Range &c)                               \
            { return IteratorType(c.first); }                           \
    };                                                                  \
                                                                        \
    template <>                                                         \
    struct Tf_IteratorInterface<const Range, true> {                    \
        typedef ReverseIterator IteratorType;                           \
        static IteratorType Begin(Range const &c)                       \
            { return IteratorType(c.second); }                          \
        static IteratorType End(Range const &c)                         \
            { return IteratorType(c.first); }                           \
    };                                                                  \
                                                                        \
    template <>                                                         \
    struct Tf_ShouldIterateOverCopy<Range> : boost::true_type {};       \
                                                                        \
    template <>                                                         \
    struct Tf_ShouldIterateOverCopy<const Range> : boost::true_type {}

PCP_DEFINE_RANGE(PcpNodeRange, PcpNodeIterator, PcpNodeReverseIterator);
PCP_DEFINE_RANGE(PcpPrimRange, PcpPrimIterator, PcpPrimReverseIterator);
PCP_DEFINE_RANGE(PcpPropertyRange, PcpPropertyIterator, 
                 PcpPropertyReverseIterator);

/// \class PcpIteratorTraits
///
/// Traits class for retrieving useful characteristics about one of the
/// Pcp iterator types above.
///
template <class Iterator> struct PcpIteratorTraits;

template <>
struct PcpIteratorTraits<PcpNodeIterator>
{
    typedef PcpNodeRange RangeType;
    typedef PcpNodeReverseIterator ReverseIteratorType;
};

template <>
struct PcpIteratorTraits<PcpPrimIterator>
{
    typedef PcpPrimRange RangeType;
    typedef PcpPrimReverseIterator ReverseIteratorType;
};

template <>
struct PcpIteratorTraits<PcpPropertyIterator>
{
    typedef PcpPropertyRange RangeType;
    typedef PcpPropertyReverseIterator ReverseIteratorType;
};

#endif // PCP_ITERATOR_H
