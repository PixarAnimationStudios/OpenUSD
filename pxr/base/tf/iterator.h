//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_ITERATOR_H
#define PXR_BASE_TF_ITERATOR_H

/// \file tf/iterator.h
/// \ingroup group_tf_Containers
/// A simple iterator adapter for \c STL containers.

#include "pxr/pxr.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <iterator>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// May be specialized by container proxies and container "views" to indicate
// they should be copied for TfIterator iteration.
template <class T>
struct Tf_ShouldIterateOverCopy : std::false_type {};

// IteratorInterface abstracts the differences between forward/backward and
// const/non-const iteration so that TfIterator doesn't have to think about
// them.  It simply provides the IteratorType (which is either iterator,
// const_iterator, reverse_iterator, or reverse_const_iterator) and Begin and
// End which call the correct functions in the container (begin, rbegin, end,
// rend).
template <class T, bool Reverse>
struct Tf_IteratorInterface {
    typedef typename T::iterator IteratorType;
    static IteratorType Begin(T &c) { return c.begin(); }
    static IteratorType End(T &c) { return c.end(); }
};

template <class T, bool Reverse>
struct Tf_IteratorInterface<const T, Reverse> {
    typedef typename T::const_iterator IteratorType;
    static IteratorType Begin(T const &c) { return c.begin(); }
    static IteratorType End(T const &c) { return c.end(); }
};

template <class T>
struct Tf_IteratorInterface<T, true> {
    typedef typename T::reverse_iterator IteratorType;
    static IteratorType Begin(T &c) { return c.rbegin(); }
    static IteratorType End(T &c) { return c.rend(); }
};

template <class T>
struct Tf_IteratorInterface<const T, true> {
    typedef typename T::const_reverse_iterator IteratorType;
    static IteratorType Begin(T const &c) { return c.rbegin(); }
    static IteratorType End(T const &c) { return c.rend(); }
};

/// \class TfIterator
/// \ingroup group_tf_Containers group_tf_Stl
///
/// A simple iterator adapter for \c STL containers.
///
/// \c TfIterator iterates over the elements in an \c STL container, according
/// to the semantics of the \ref iterator_pattern "simple iterator pattern".
/// The following examples compare the \c TfIterator to \c STL, highlighting
/// the brevity of the \c TfIterator interface.
/// \code
///     std::vector<int> vector;
///     std::set<int> set;
///
///     // TfIterator 'while' loop
///     TfIterator< std::vector<int> > i(vector);
///     while (i) {
///         int x = *i++;
///     }
///
///     // STL 'while' loop
///     std::vector<int>::iterator i = vector.begin();
///     while (i != vector.end()) {
///         int x = *i++;
///     }
///
///     // TfIterator 'for' loop
///     std::set<int> set;
///     for (TfIterator< const std::set<int> > j = set; j; ++j) {
///         int x = *j;
///     }
///
///     // STL 'for' loop
///     std::set<int> set;
///     for (std::set<int>::iterator j = set.begin(); j != set.end(); ++j) {
///         int x = *j;
///     }
/// \endcode
///
/// Note that using the \c TF_FOR_ALL() macro, even more brevity is possible.
/// For example, to print out all items of a \c set<int> \c s, we could write
/// \code
///     TF_FOR_ALL(i, s)
///         printf("%d\n", *i);
/// \endcode
///
/// Typically, a \c TfIterator is used to traverse all of the elements in an
/// \c STL container.  For ordered sets, other uses include iterating over a
/// subset of the elements in the container, and using a \c TfIterator as a
/// sentinel.
/// \code
///     // Iterate over subset
///     TfIterator< std::vector<int> > start, finish;
///     TfIterator< std::vector<int> > iterator(start, finish);
///
///     // TfIterator sentinel
///     TfIterator< std::vector<int> > sentinel(finish, finish);
///     while (iterator != sentinel) {
///         int x = *iterator++;
///     }
/// \endcode
/// 
/// \anchor iterator_pattern
/// <b>The Simple Iterator Pattern</b>
///
/// The \e simple \e iterator pattern generalizes pointer semantics to
/// traverse a set of elements, much like \c STL iterators.  However, the
/// simple iterator pattern subscribes to a simpler subset of pointer
/// operations: pointer assignment (\c operator=), auto-increment (\c
/// operator++), dereferencing (\c operator*), redirection (\c operator->),
/// and null pointer comparison (\c operator! and \c operator \c bool).  The
/// simpler interface improves code legibility for the typical set traversals
/// for which iterators are most commonly used.  It is particularly useful for
/// specifying iterators over sets of elements that are maintained by a user
/// object, since the interface calls for only one \c GetIterator() entry
/// point rather than dual \c begin() and \c end() calls.  This is especially
/// desirable when the object owns many different sets.
/// \code
///     // The simple iterator pattern.
///     class Iterator {
///         Iterator();                                     // default c'tor
///         Iterator(const Iterator&);                      // copy c'tor
///         Iterator& operator=(const Iterator &);          // assignment
///         Iterator& operator++();                         // pre-increment
///         Iterator operator++(int);                       // post-increment
///         reference operator *();                         // dereference
///         pointer operator->();                           // redirection
///         bool operator==(const Iterator &) const;        // equality
///         bool operator!=(const Iterator &) const;        // inequality
///         bool operator!() const                          // is exhausted
///         operator bool() const;                          // is not exhausted
///     };
/// \endcode
///
/// \param T  container type
///
template <class T, bool Reverse=false>
class TfIterator {

    // Forward declare implementation structs.
    struct _IteratorPairAndCopy;
    struct _IteratorPair;

    // Select the correct data storage depending on whether we should iterate
    // over a copy of the container.
    typedef typename std::conditional<
        Tf_ShouldIterateOverCopy<T>::value, 
        _IteratorPairAndCopy, _IteratorPair
        >::type _Data;

public:
    // Choose either iterator or const_iterator for Iterator depending on
    // whether T is const.
    typedef Tf_IteratorInterface<T, Reverse> IterInterface;
    typedef typename IterInterface::IteratorType Iterator;

    typedef typename std::iterator_traits<Iterator>::reference Reference;

    /// Default constructor.  This iterator is uninitialized.
    TfIterator() { }

    /// Constructs an iterator to traverse each element of the specified
    /// \c STL container object.
    /// \param container  container object
    TfIterator(T &container) : _data(container) {}

    /// Allow rvalues only if the container type T should be copied by TfIterator.
    TfIterator(T &&container)
        : _data(container)
    {
        static_assert(
            Tf_ShouldIterateOverCopy<typename std::decay<T>::type>::value,
            "TfIterator only allows rvalues that it has been told to copy "
            "via Tf_ShouldIterateOverCopy");
    }

    /// Constructs an iterator to traverse a subset of the elements in a
    /// container.  This iterator is exhausted when it reaches the end
    /// iterator.
    /// \param begin  iterator at the beginning of the sequence
    /// \param end  iterator at the end of the sequence
    TfIterator(Iterator const &begin, Iterator const &end)
        : _data(begin, end)
    {
    }

    /// Returns true if this iterator is exhausted.
    /// \return true if this iterator is exhausted
    bool operator!() const {
        return _data.current == _data.end;
    }

    /// Returns true if this Iterator.has the same position in the sequence as
    /// the specified iterator.  The end of the sequence need not be the same.
    /// \param iterator  iterator to compare
    /// \return true if this Iterator.has the same position as \e iterator
    bool operator==(const TfIterator& iterator) const {
        return _data.current == iterator._data.current;
    }

    /// Returns false if (*this == \a iterator) returns true, returns true
    /// otherwise.
    bool operator!=(const TfIterator& iterator) const {
        return !(*this == iterator);
    }

    /// Pre-increment operator.  Advances this iterator to the next element in
    /// the sequence.
    /// \return this iterator
    TfIterator& operator++() {
        if (!*this) {
            TF_CODING_ERROR("iterator exhausted");
            return *this;
        }

        ++_data.current;
        return *this;
    }

    /// Post-increment operator.  Advances this iterator to the next element in
    /// the sequence, and returns a copy of this iterator prior to the increment.
    /// \return copy of this iterator prior to increment
    TfIterator operator++(int) {
        TfIterator iterator = *this;
        ++(*this);
        return iterator;
    }

    /// Returns the element referenced by this iterator.
    /// \return element
    Reference operator*() {
        if (ARCH_UNLIKELY(!*this))
            TF_FATAL_ERROR("iterator exhausted");
        return *_data.current;
    }

    /// Returns the element referenced by this iterator.
    /// \return element
    Reference operator*() const {
        if (ARCH_UNLIKELY(!*this))
            TF_FATAL_ERROR("iterator exhausted");
        return *_data.current;
    }

    /// Returns a pointer to the element referenced by this iterator.
    /// \return pointer to element
    Iterator& operator->() {
        if (ARCH_UNLIKELY(!*this))
            TF_FATAL_ERROR("iterator exhausted");
        return _data.current;
    }   

    /// Explicit bool conversion operator.
    /// The Iterator object converts to true if it has not been exhausted.
    explicit operator bool() const {
        return !(_data.current == _data.end);
    }

    /// Returns an \c STL iterator that has the same position as this
    /// iterator.
    /// \return \c STL iterator at the same position as this iterator
    operator Iterator() const {
        return _data.current;
    }

    /// Returns an \c STL iterator that has the same position as this
    /// iterator.
    /// \return \c STL iterator at the same position as this iterator
    const Iterator& base() const {
        return _data.current;
    }

    /// Returns an iterator that is positioned at the next element in the
    /// sequence.
    /// \return iterator at next element in the sequence
    TfIterator GetNext() const {
        TfIterator next = *this;
        ++next;
        return next;
    }

  private:  // state

    // Normal iteration just holds onto the begin/end pair of iterators.
    struct _IteratorPair {
        _IteratorPair() {}
        explicit _IteratorPair(T &c) {
            // Use assignment rather than initializer-list here to work around
            // a GCC 4.1.2 bug when using TfIterator with TfHashMap.
            current = IterInterface::Begin(c);
            end = IterInterface::End(c);
        }
        _IteratorPair(Iterator const &b, Iterator const &e) :
            current(b), end(e) {}
        Iterator current;
        Iterator end;
    };

    // Iterating over copies which is appropriate for proxies retains a copy of
    // 'container' and iterators into the copy.
    struct _IteratorPairAndCopy : public _IteratorPair {
        _IteratorPairAndCopy() {}
        explicit _IteratorPairAndCopy(T const &c) : _IteratorPair(), _copy(c) {
            current = IterInterface::Begin(_copy);
            end = IterInterface::End(_copy);
        }
        using _IteratorPair::current;
        using _IteratorPair::end;
    private:
        T _copy;
    };

    _Data _data;

};

/// Helper functions for creating TfIterator objects.
/// \ingroup group_tf_Containers
template <class T>
TfIterator<typename std::remove_reference<T>::type>
TfMakeIterator(T&& container)
{
    return TfIterator<typename std::remove_reference<T>::type>(
        std::forward<T>(container));
}

template <class T>
TfIterator<typename std::remove_reference<T>::type, /* Reverse = */ true>
TfMakeReverseIterator(T&& container)
{
    return TfIterator<typename std::remove_reference<T>::type, true>(
        std::forward<T>(container));
}

/// Macro for iterating over a container.
///
/// For any container \c c of type \c T, the following loop
/// \code
///     for (TfIterator<T> i = c.begin(); i; ++i) {
///         ...
///     }
/// \endcode
/// is equivalent to
/// \code
///     TF_FOR_ALL(i, c) {
///         ...
///     }
/// \endcode
///
/// \ingroup group_tf_Containers
/// \hideinitializer
#define TF_FOR_ALL(iter, c) \
    for (auto iter = TfMakeIterator(c); iter; ++iter)

/// Macro for iterating over a container in reverse.
///
/// Operates like \a TF_FOR_ALL, but iterates the container in reverse order.
/// 
/// \ingroup group_tf_Containers
/// \hideinitializer
#define TF_REVERSE_FOR_ALL(iter, c) \
    for (auto iter = TfMakeReverseIterator(c); iter; ++iter)

/// Returns the number of elements in a statically sized array.
///
/// This function is an implementation of the array version of C++17's
/// std::size()
template <class T, size_t N>
constexpr size_t TfArraySize(const T (&array)[N]) noexcept
{
    return N;
}

/// A reverse iterator adapter for `std::reverse_iterator` that provides
/// an `operator->` compatible with proxy reference types.
/// This should only be used when the underlying iterator's reference
/// is a value type and should become unnecessary in newer compilers and C++20.
/// This implementation was written for use with random access iterators but
/// could be extended to bidirectional iterators if necessary.
template <typename UnderlyingIterator>
class Tf_ProxyReferenceReverseIterator :
    private std::reverse_iterator<UnderlyingIterator> {
    // private API for interacting with an STL reverse_iterator of the
    // UnderlyingIterator
    using ReverseIterator = std::reverse_iterator<UnderlyingIterator>;
    const ReverseIterator& _reverse_iterator() const {  return *this; }
    ReverseIterator& _reverse_iterator() { return *this; }
    explicit Tf_ProxyReferenceReverseIterator(const ReverseIterator& it)
        : ReverseIterator(it) {}
    explicit Tf_ProxyReferenceReverseIterator(ReverseIterator&& it)
        : ReverseIterator(it) {}
public:
    using iterator_type = typename ReverseIterator::iterator_type;
    using iterator_category = typename ReverseIterator::iterator_category;
    using value_type = typename ReverseIterator::value_type;
    using reference = typename ReverseIterator::reference;
    using pointer = typename ReverseIterator::pointer;
    using difference_type = typename ReverseIterator::difference_type;

    static_assert(!std::is_reference<reference>::value,
                 "Tf_ProxyReferenceReverseIterator should only be used "
                 "when the underlying iterator's reference type is a "
                 "proxy (MyTypeRef) and not a true reference (MyType&)."
                 "Use std::reverse_iterator instead.");
    static_assert(std::is_same<iterator_category,
                               std::random_access_iterator_tag>::value,
                 "Tf_ProxyReferenceReverseIterator must wrap a random "
                 "access iterator.");

    Tf_ProxyReferenceReverseIterator() = default;
    explicit Tf_ProxyReferenceReverseIterator(UnderlyingIterator it) :
        ReverseIterator(it) {
    }

    // Operators and functions which can just use the underlying STL
    // implementation
    using ReverseIterator::base;
    using ReverseIterator::operator*;
    using ReverseIterator::operator[];

    /// Customize operator-> to support proxied reference types
    /// Compatible with the C++20 specification.
    pointer operator->() const { return std::prev(base()).operator->(); }

    // Many  methods can use the underlying STL implementation but need to
    // avoid returning a `std::reverse_iterator`
    Tf_ProxyReferenceReverseIterator& operator++() {
        ++_reverse_iterator();
        return *this;
    }

    Tf_ProxyReferenceReverseIterator operator++(int) {
        Tf_ProxyReferenceReverseIterator result{_reverse_iterator()};
        ++_reverse_iterator();
        return result;
    }

    Tf_ProxyReferenceReverseIterator& operator--() {
        --_reverse_iterator();
        return *this;
    }

    Tf_ProxyReferenceReverseIterator operator--(int) {
        Tf_ProxyReferenceReverseIterator result{_reverse_iterator()};
        --_reverse_iterator();
        return result;
    }

    Tf_ProxyReferenceReverseIterator operator+(difference_type increment) const {
        return Tf_ProxyReferenceReverseIterator(_reverse_iterator() + increment);
    }

    Tf_ProxyReferenceReverseIterator operator-(difference_type decrement) const {
        return Tf_ProxyReferenceReverseIterator(_reverse_iterator() - decrement);
    }

    template <typename OtherIt>
    difference_type operator-(
        const Tf_ProxyReferenceReverseIterator<OtherIt>& other) const {
        return _reverse_iterator() - other._reverse_iterator();
    }

    Tf_ProxyReferenceReverseIterator& operator+=(difference_type increment) {
        _reverse_iterator() += increment;
        return *this;
    }

    Tf_ProxyReferenceReverseIterator& operator-=(difference_type decrement) {
        _reverse_iterator() -= decrement;
        return *this;
    }

    inline friend Tf_ProxyReferenceReverseIterator
    operator+(const difference_type increment,
              const Tf_ProxyReferenceReverseIterator& iterator) {
        return Tf_ProxyReferenceReverseIterator(
            increment + iterator._reverse_iterator());
    }

    // Comparison operators defer to the STL implementation
    template <typename OtherIt>
    inline friend bool operator==(const Tf_ProxyReferenceReverseIterator& lhs,
                                  const Tf_ProxyReferenceReverseIterator<OtherIt>& rhs) {
        return lhs._reverse_iterator() == rhs._reverse_iterator();
    }

    template <typename OtherIt>
    inline friend bool operator!=(const Tf_ProxyReferenceReverseIterator& lhs,
                                  const Tf_ProxyReferenceReverseIterator<OtherIt>& rhs) {
        return lhs._reverse_iterator() != rhs._reverse_iterator();
    }

    template <typename OtherIt>
    inline friend bool operator<(const Tf_ProxyReferenceReverseIterator& lhs,
                                 const Tf_ProxyReferenceReverseIterator<OtherIt>& rhs) {
        return lhs._reverse_iterator() < rhs._reverse_iterator();
    }

    template <typename OtherIt>
    inline friend bool operator>(const Tf_ProxyReferenceReverseIterator& lhs,
                                 const Tf_ProxyReferenceReverseIterator<OtherIt>& rhs) {
        return lhs._reverse_iterator() > rhs._reverse_iterator();
    }

    template <typename OtherIt>
    inline friend bool operator<=(const Tf_ProxyReferenceReverseIterator& lhs,
                                  const Tf_ProxyReferenceReverseIterator<OtherIt>& rhs) {
        return lhs._reverse_iterator() <= rhs._reverse_iterator();
    }

    template <typename OtherIt>
    inline friend bool operator>=(const Tf_ProxyReferenceReverseIterator& lhs,
                                  const Tf_ProxyReferenceReverseIterator<OtherIt>& rhs) {
        return lhs._reverse_iterator() >= rhs._reverse_iterator();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_BASE_TF_ITERATOR_H
