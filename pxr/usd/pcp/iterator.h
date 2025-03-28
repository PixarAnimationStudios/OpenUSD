//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_ITERATOR_H
#define PXR_USD_PCP_ITERATOR_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/node.h"

#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/site.h"

#include "pxr/base/tf/iterator.h"

#include <iterator>

PXR_NAMESPACE_OPEN_SCOPE

class PcpPrimIndex;
class PcpPrimIndex_Graph;
class PcpPropertyIndex;

/// \class PcpNodeIterator
///
/// Object used to iterate over nodes in the prim index graph in strong-to-weak
/// order.
///
class PcpNodeIterator
{
    class _PtrProxy {
    public:
        PcpNodeRef* operator->() { return &_nodeRef; }
    private:
        friend class PcpNodeIterator;
        explicit _PtrProxy(const PcpNodeRef& nodeRef) : _nodeRef(nodeRef) {}
        PcpNodeRef _nodeRef;
    };
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = PcpNodeRef;
    using reference = PcpNodeRef;
    using pointer = _PtrProxy;
    using difference_type = std::ptrdiff_t;

    /// Constructs an invalid iterator.
    PcpNodeIterator() = default;

    // Returns a compressed Sd site.  For internal use only.
    Pcp_CompressedSdSite GetCompressedSdSite(size_t layerIndex) const
    {
        return Pcp_CompressedSdSite(_nodeIdx, layerIndex);
    }

    reference operator*() const { return dereference(); }
    pointer operator->() const { return pointer(dereference()); }
    reference operator[](const difference_type index) const {
        PcpNodeIterator advanced(*this);
        advanced.advance(index);
        return advanced.dereference();
    }

    difference_type operator-(const PcpNodeIterator& other) const {
        return -distance_to(other);
    }

    PcpNodeIterator& operator++() {
        increment();
        return *this;
    }

    PcpNodeIterator& operator--() {
        decrement();
        return *this;
    }

    PcpNodeIterator operator++(int) {
        PcpNodeIterator result(*this);
        increment();
        return result;
    }

    PcpNodeIterator operator--(int) {
        PcpNodeIterator result(*this);
        decrement();
        return result;
    }

    PcpNodeIterator operator+(const difference_type increment) const {
        PcpNodeIterator result(*this);
        result.advance(increment);
        return result;
    }

    PcpNodeIterator operator-(const difference_type decrement) const {
        PcpNodeIterator result(*this);
        result.advance(-decrement);
        return result;
    }

    PcpNodeIterator& operator+=(const difference_type increment) {
        advance(increment);
        return *this;
    }

    PcpNodeIterator& operator-=(const difference_type decrement) {
        advance(-decrement);
        return *this;
    }

    bool operator==(const PcpNodeIterator& other) const {
        return equal(other);
    }

    bool operator!=(const PcpNodeIterator& other) const {
        return !equal(other);
    }

    bool operator<(const PcpNodeIterator& other) const {
        TF_DEV_AXIOM(_graph == other._graph);
        return _nodeIdx < other._nodeIdx;
    }

    bool operator<=(const PcpNodeIterator& other) const {
        TF_DEV_AXIOM(_graph == other._graph);
        return _nodeIdx <= other._nodeIdx;
    }

    bool operator>(const PcpNodeIterator& other) const {
        TF_DEV_AXIOM(_graph == other._graph);
        return _nodeIdx > other._nodeIdx;
    }

    bool operator>=(const PcpNodeIterator& other) const {
        TF_DEV_AXIOM(_graph == other._graph);
        return _nodeIdx >= other._nodeIdx;
    }

private:
    friend class PcpPrimIndex;
    PcpNodeIterator(PcpPrimIndex_Graph* graph, size_t nodeIdx) :
        _graph(graph), _nodeIdx(nodeIdx) {}

    void increment() { ++_nodeIdx; }
    void decrement() { --_nodeIdx; }
    void advance(difference_type n) { _nodeIdx += n; }
    difference_type distance_to(const PcpNodeIterator& other) const {
        return (difference_type)(other._nodeIdx) - _nodeIdx;
    }
    bool equal(const PcpNodeIterator& other) const {
        return (_graph == other._graph) & (_nodeIdx == other._nodeIdx);
    }
    reference dereference() const {
        return PcpNodeRef(_graph, _nodeIdx);
    }

private:
    PcpPrimIndex_Graph* _graph = nullptr;
    size_t _nodeIdx = PCP_INVALID_INDEX;
};

/// \class PcpNodeReverseIterator
///
/// Object used to iterate over nodes in the prim index graph in weak-to-strong
/// order.
///
class PcpNodeReverseIterator
    : public Tf_ProxyReferenceReverseIterator<PcpNodeIterator>
{
public:
    PcpNodeReverseIterator() { }
    explicit PcpNodeReverseIterator(const PcpNodeIterator& iter)
        : Tf_ProxyReferenceReverseIterator<PcpNodeIterator>(iter) {}
};

/// \class PcpPrimIterator
///
/// Object used to iterate over prim specs in the prim index graph in 
/// strong-to-weak order.
///
class PcpPrimIterator 
{
    class _PtrProxy {
    public:
        SdfSite* operator->() { return &_site; }
    private:
        friend class PcpPrimIterator;
        explicit _PtrProxy(const SdfSite& site) : _site(site) {}
        SdfSite _site;
    };
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = SdfSite;
    using reference = SdfSite;
    using pointer = _PtrProxy;
    using difference_type = std::ptrdiff_t;

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

    reference operator*() const { return dereference(); }
    pointer operator->() const { return pointer(dereference()); }
    reference operator[](const difference_type index) const {
        PcpPrimIterator advanced(*this);
        advanced.advance(index);
        return advanced.dereference();
    }

    difference_type operator-(const PcpPrimIterator& other) const {
        return -distance_to(other);
    }

    PcpPrimIterator& operator++() {
        increment();
        return *this;
    }

    PcpPrimIterator& operator--() {
        decrement();
        return *this;
    }

    PcpPrimIterator operator++(int) {
        PcpPrimIterator result(*this);
        increment();
        return result;
    }

    PcpPrimIterator operator--(int) {
        PcpPrimIterator result(*this);
        decrement();
        return result;
    }

    PcpPrimIterator operator+(const difference_type increment) const {
        PcpPrimIterator result(*this);
        result.advance(increment);
        return result;
    }

    PcpPrimIterator operator-(const difference_type decrement) const {
        PcpPrimIterator result(*this);
        result.advance(-decrement);
        return result;
    }

    PcpPrimIterator& operator+=(const difference_type increment) {
        advance(increment);
        return *this;
    }

    PcpPrimIterator& operator-=(const difference_type decrement) {
        advance(-decrement);
        return *this;
    }

    bool operator==(const PcpPrimIterator& other) const {
        return equal(other);
    }

    bool operator!=(const PcpPrimIterator& other) const {
        return !equal(other);
    }

    bool operator<(const PcpPrimIterator& other) const {
        TF_DEV_AXIOM(_primIndex == other._primIndex);
        return _pos < other._pos;
    }

    bool operator<=(const PcpPrimIterator& other) const {
        TF_DEV_AXIOM(_primIndex == other._primIndex);
        return _pos <= other._pos;
    }

    bool operator>(const PcpPrimIterator& other) const {
        TF_DEV_AXIOM(_primIndex == other._primIndex);
        return _pos > other._pos;
    }

    bool operator>=(const PcpPrimIterator& other) const {
        TF_DEV_AXIOM(_primIndex == other._primIndex);
        return _pos >= other._pos;
    }

private:
    PCP_API
    void increment();
    PCP_API
    void decrement();
    PCP_API
    void advance(difference_type n);
    PCP_API
    difference_type distance_to(const PcpPrimIterator& other) const;
    PCP_API
    bool equal(const PcpPrimIterator& other) const;
    PCP_API
    reference dereference() const;

private:
    const PcpPrimIndex* _primIndex = nullptr;
    size_t _pos = PCP_INVALID_INDEX;
};

/// \class PcpPrimReverseIterator
///
/// Object used to iterate over prim specs in the prim index graph in 
/// weak-to-strong order.
///
class PcpPrimReverseIterator
    : public Tf_ProxyReferenceReverseIterator<PcpPrimIterator>
{
public:
    PcpPrimReverseIterator() { }
    explicit PcpPrimReverseIterator(const PcpPrimIterator& iter)
        : Tf_ProxyReferenceReverseIterator<PcpPrimIterator>(iter) { }
        
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
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = const SdfPropertySpecHandle;
    using reference = const SdfPropertySpecHandle&;
    using pointer = const SdfPropertySpecHandle*;
    using difference_type = std::ptrdiff_t;

    /// Constructs an invalid iterator.
    PCP_API
    PcpPropertyIterator();

    /// Constructs a property iterator for \p index beginning at position
    /// \p pos in the property stack.
    PCP_API
    PcpPropertyIterator(const PcpPropertyIndex& index, size_t pos = 0);

    /// Returns the PcpNode from which the current property originated.
    PCP_API
    PcpNodeRef GetNode() const;

    /// Returns true if the current property is local to the owning
    /// property index's layer stack, false otherwise.
    PCP_API
    bool IsLocal() const;

    reference operator*() const { return dereference(); }
    pointer operator->() const { return &(dereference()); }
    reference operator[](const difference_type index) const {
        PcpPropertyIterator advanced(*this);
        advanced.advance(index);
        return advanced.dereference();
    }

    difference_type operator-(const PcpPropertyIterator& other) const {
        return -distance_to(other);
    }

    PcpPropertyIterator& operator++() {
        increment();
        return *this;
    }

    PcpPropertyIterator& operator--() {
        decrement();
        return *this;
    }

    PcpPropertyIterator operator++(int) {
        PcpPropertyIterator result(*this);
        increment();
        return result;
    }

    PcpPropertyIterator operator--(int) {
        PcpPropertyIterator result(*this);
        decrement();
        return result;
    }

    PcpPropertyIterator operator+(const difference_type increment) const {
        PcpPropertyIterator result(*this);
        result.advance(increment);
        return result;
    }

    PcpPropertyIterator operator-(const difference_type decrement) const {
        PcpPropertyIterator result(*this);
        result.advance(-decrement);
        return result;
    }

    PcpPropertyIterator& operator+=(const difference_type increment) {
        advance(increment);
        return *this;
    }

    PcpPropertyIterator& operator-=(const difference_type decrement) {
        advance(-decrement);
        return *this;
    }

    bool operator==(const PcpPropertyIterator& other) const {
        return equal(other);
    }

    bool operator!=(const PcpPropertyIterator& other) const {
        return !equal(other);
    }

    bool operator<(const PcpPropertyIterator& other) const {
        TF_DEV_AXIOM(_propertyIndex == other._propertyIndex);
        return _pos < other._pos;
    }

    bool operator<=(const PcpPropertyIterator& other) const {
        TF_DEV_AXIOM(_propertyIndex == other._propertyIndex);
        return _pos <= other._pos;
    }

    bool operator>(const PcpPropertyIterator& other) const {
        TF_DEV_AXIOM(_propertyIndex == other._propertyIndex);
        return _pos > other._pos;
    }

    bool operator>=(const PcpPropertyIterator& other) const {
        TF_DEV_AXIOM(_propertyIndex == other._propertyIndex);
        return _pos >= other._pos;
    }

private:
    PCP_API
    void increment();
    PCP_API
    void decrement();
    PCP_API
    void advance(difference_type n);
    PCP_API
    difference_type distance_to(const PcpPropertyIterator& other) const;
    PCP_API
    bool equal(const PcpPropertyIterator& other) const;
    PCP_API
    reference dereference() const;

private:
    const PcpPropertyIndex* _propertyIndex = nullptr;
    size_t _pos = 0;
};

/// \class PcpPropertyReverseIterator
///
/// Object used to iterate over property specs in a property index in
/// weak-to-strong order.
///
class PcpPropertyReverseIterator
    : public std::reverse_iterator<PcpPropertyIterator>
{
public:
    PcpPropertyReverseIterator() { }
    explicit PcpPropertyReverseIterator(const PcpPropertyIterator& iter)
        : std::reverse_iterator<PcpPropertyIterator>(iter) { }
        
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
    struct Tf_ShouldIterateOverCopy<Range> : std::true_type {};         \
                                                                        \
    template <>                                                         \
    struct Tf_ShouldIterateOverCopy<const Range> : std::true_type {}

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_ITERATOR_H
