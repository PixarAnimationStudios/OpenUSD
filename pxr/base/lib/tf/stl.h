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
#ifndef TF_STL_H
#define TF_STL_H

/// \file tf/stl.h
/// \ingroup group_tf_Stl

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/iterator.h"

#include <boost/call_traits.hpp>
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>

// Helper for TfMapLookup().  Uses std::map API to get a value by key.
template <class T>
struct Tf_MapLookupHelper {
    typedef T Container;

    template <class Key, class Result>
    static bool Lookup(Container const& map, Key const &key, Result* valuePtr)
    {
        typename Container::const_iterator i = map.find(key);
        if (i == map.end()) {
            return false;
        }
        else {
            *valuePtr = i->second;
            return true;
        }
    }
};

/// Checks if an item exists in a \c map or a \c TfHashMap.
///
/// If \p key exists in \p map, then this function returns \c true and the
/// value indexed by \p key is copied into \p *valuePtr.  Otherwise,
/// \p *valuePtr is not modified, and \c false is returned.
///
/// Example:
/// \code
///    TfHashMap<string, int, TfHash> m = ...;
///    int value;
///
///
///    if (TfMapLookup(m, "someKey", &value))
///        printf("Value found: %d\n", value);
///    else
///        printf("Value not found\n");
///        ...
/// \endcode
///
/// \ingroup group_tf_Stl
template <class Container, class Key, class Result>
bool TfMapLookup(Container const &map, Key const &key, Result* valuePtr)
{
    return Tf_MapLookupHelper<Container>::Lookup(map, key, valuePtr);
}

/// Checks if an item exists in a \c map or a \c TfHashMap.
///
/// If \p key exists in \p map, then this function returns the value index by
/// \p key.  Otherwise, \p defaultValue is returned.  Note that the result is
/// returned by value, so this is best used for types that are quick to copy.
///
/// Example:
/// \code
///    TfHashMap<string, int, TfHash> m;
///    m["foo"] = 1;
///
///    int value = TfMapLookup(m, "someKey", -1);
///    TF_AXIOM(value == -1);
///
///    int value = TfMapLookup(m, "foo", -1);
///    TF_AXIOM(value == 1);
///
/// \endcode
///
/// \ingroup group_tf_Stl
template <class Container, class Key, class Result>
const Result TfMapLookupByValue(Container const &map,
                 Key const &key, const Result &defaultValue)
{
    typename Container::const_iterator i = map.find(key);
    if (i == map.end()) {
        return defaultValue;
    } else {
        return i->second;
    }
}

/// Checks if an item exists in a \c map or \c TfHashMap, without copying it.
///
/// If \p key exists in the \p map, then this function returns a pointer to
/// the value indexed by \p key.  Otherwise, NULL is returned.
///
/// Example:
/// \code
///    TfHashMap<string, BigData, TfHash> m = ...;
///
///    if (BigData* bigPtr = TfMapLookupPtr(m, "someKey"))
///        bigPtr->ModifyStuff(); 
///    else
///        printf("Value not found\n");
/// \endcode
///
/// \ingroup group_tf_Stl
template <class Container, class Key>
typename Container::mapped_type *
TfMapLookupPtr(Container &map, Key const &key)
{
    typename Container::iterator i = map.find(key);
    return (i != map.end()) ? &(i->second) : NULL;
}

template <class Container, class Key>
typename Container::mapped_type const *
TfMapLookupPtr(Container const &map, Key const &key)
{
    typename Container::const_iterator i = map.find(key);
    return (i != map.end()) ? &(i->second) : NULL;
}

/// Return an \c std::pair in sorted order.
///
/// This call is a useful helper for maps whose key is an unordered pair of
/// elements.  One can either define a new data type such that (a,b) is deemed
/// equivalent to (b,a), or more simply, adopt the convention that a key is
/// always written (a,b) with a < b.
///
/// \ingroup group_tf_Stl
template <typename T>
inline std::pair<T,T>
TfOrderedPair(T a, T b) {
    return (a < b) ? std::pair<T,T>(a,b) : std::pair<T,T>(b,a);
}

/// Reset \a obj to be an empty, space-optimized object.
/// 
/// This can be used to clear c++ containers and reclaim their memory.  For
/// instance, std::vector::clear() will not reclaim any memory, even if the
/// vector previously had a large number of elements.  Often, this is what you
/// want because the vector is later filled again.  But sometimes you want to
/// reclaim the memory, and this function will do that.
///
/// As another example, gcc's hash_map and hash_set do not clear their bucket
/// lists when they themselves are cleared.  This can lead to poor performance
/// due to ever-growing bucket lists for hashes that are repeatedly filled,
/// cleared, and filled again.  TfReset will avoid this by effectively
/// clearing the bucket list.
///
/// This function requires that the expression T().swap(obj) where obj is of
/// type T& be valid.  This is true for many classes, including the standard
/// containers.
template <class T>
inline void TfReset(T &obj) {
    T().swap(obj);
}

TF_API size_t Tf_GetEmptyHashMapBucketCount();

/// Specialize for TfHashMap to make minimally sized hashes.
template <class Key, class Value, class Hash, class Equal, class Alloc>
inline void TfReset(TfHashMap<Key, Value, Hash, Equal, Alloc> &hash){
    // If the implementation of the hash map allocates buckets when
    // constructed asking for zero then only swap a constructed object
    // if \p hash has more than that many buckets already, otherwise
    // we just clear().  Note that this assumes that the number of
    // buckets does not depend on the template parameter types which
    // is reasonable.
    static size_t emptyCount = Tf_GetEmptyHashMapBucketCount();

    if (hash.bucket_count() > emptyCount)
        TfHashMap<Key, Value, Hash, Equal, Alloc>(0).swap(hash);
    else if (not hash.empty())
        hash.clear();
}

/// Specialize for TfHashSet to make minimally sized hashes.
template <class Value, class Hash, class Equal, class Alloc>
inline void TfReset(TfHashSet<Value, Hash, Equal, Alloc> &hash) {
    static size_t emptyCount = Tf_GetEmptyHashSetBucketCount();

    // See comment above about issues with TfHashSet(0).
    if (hash.bucket_count() > emptyCount)
        TfHashSet<Value, Hash, Equal, Alloc>(0).swap(hash);
    else if (not hash.empty())
        hash.clear();
}


/// An unary function that represents the identity function; it takes a single
/// argument \a arg, and returns \a arg.
///
/// This is similar to the sgi extension std::identity<T>.
template <class T>
inline typename boost::call_traits<T>::param_type
TfIdentity(typename boost::call_traits<T>::param_type arg) {
    return arg;
}

/// Produce a sequence consisting of the set difference of [\a first1, \a
/// last1) and [\a first2, \a last2), while maintaining the relative order of
/// the first sequence. No particular order is required for either range, but
/// elements must have a strict weak order provided by operator<.
///
/// If an element appears multiple times in either the first or second
/// sequence, the number of occurrences in the output is the difference
/// between the first sequence and the second (or zero if there are more
/// occurrences in the second sequence).  For example, if the first sequence
/// is (1, 3, 3, 1) and the second sequence is (2, 3, 2), the result will be
/// (1, 3, 1).
template <class InputIterator1, class InputIterator2, class OutputIterator>
void
TfOrderedSetDifference(InputIterator1 first1, InputIterator1 last1,
                       InputIterator2 first2, InputIterator2 last2,
                       OutputIterator result)
{
    typedef std::multiset<typename InputIterator2::value_type> SetType;
    SetType set2(first2, last2);

    // Walk [first1, last1).  If the element is in set2, skip it, and remove one
    // of those elements from set2, otherwise output it.
    for (InputIterator1 i = first1; i != last1; ++i) {
        typename SetType::iterator j = set2.find(*i);
        if (j != set2.end())
            set2.erase(j);
        else
            *result++ = *i;
    }
}

/// Produce a sequence consisting of the set difference of [\a first1, \a
/// last1) and [\a first2, \a last2), while maintaining the relative order of
/// the first sequence.  No particular order is required for either range, but
/// elements must have a strict weak order provided by operator<.
///
/// If an element appears multiple times in either the first or second
/// sequence, the number of occurrences in the output is the difference
/// between the first sequence and the second (or zero if there are more
/// occurrences in the second sequence).  For example, if the first sequence
/// is (1, 3, 3, 1) and the second sequence is (2, 3, 2), the result will be
/// (1, 3, 1).
template <class BackInsertionSequence,
          class InputIterator1, class InputIterator2>
BackInsertionSequence
TfOrderedSetDifferenceToContainer(InputIterator1 first1, InputIterator1 last1,
                                  InputIterator2 first2, InputIterator2 last2)
{
    BackInsertionSequence result;
    TfOrderedSetDifference(first1, last1, first2, last2,
                           std::back_inserter(result));
    return result;
}

/// Produce a sequence consisting of the set difference of the unique elements
/// in [\a first1, \a last1) and [\a first2, \a last2), while maintaining the
/// relative order of the first sequence.  No particular order is required for
/// either range, but elements must have a strict weak order provided by
/// operator<.
///
/// If an element appears multiple times in the first sequence, it appears
/// either zero or one times in the output.  It appears zero times if it
/// appears in the second sequence, and one time if it does not.  For example,
/// if the first sequence is (1, 3, 3, 1) and the second sequence is (2, 3,
/// 2), the result will be (1).
template <class InputIterator1, class InputIterator2, class OutputIterator>
void
TfOrderedUniquingSetDifference(InputIterator1 first1, InputIterator1 last1,
                               InputIterator2 first2, InputIterator2 last2,
                               OutputIterator result)
{
    typedef std::set<typename InputIterator1::value_type> Set1Type;
    typedef std::set<typename InputIterator2::value_type> Set2Type;

    Set1Type set1;
    Set2Type set2(first2, last2);

    // Walk [first1, last1).  If the element is in set1, skip it.  Else insert
    // it into set1, and if the element is not in set2, output it.
    for (InputIterator1 i = first1; i != last1; ++i)
        if (set1.insert(*i).second and not set2.count(*i))
            *result++ = *i;
}

/// Produce a sequence consisting of the set difference of the unique elements
/// in [\a first1, \a last1) and [\a first2, \a last2), while maintaining the
/// relative order of the first sequence.  No particular order is required for
/// either range, but elements must have a strict weak order provided by
/// operator<.
///
/// If an element appears multiple times in the first sequence, it appears
/// either zero or one times in the output.  It appears zero times if it
/// appears in the second sequence, and one time if it does not.  For example,
/// if the first sequence is (1, 3, 3, 1) and the second sequence is (2, 3,
/// 2), the result will be (1).
template <class BackInsertionSequence,
          class InputIterator1, class InputIterator2>
BackInsertionSequence
TfOrderedUniquingSetDifferenceToContainer(InputIterator1 first1,
                                          InputIterator1 last1,
                                          InputIterator2 first2,
                                          InputIterator2 last2)
{
    BackInsertionSequence result;
    TfOrderedUniquingSetDifference(first1, last1, first2, last2,
                                   std::back_inserter(result));
    return result;
}

/// A version of binary search that finds the boundary in a partitioned
/// sequence.  The Predicate pred should return true for objects on the
/// 'first' side (or left-hand) side of the boundary.
///
/// More precisely, given a range [first, last) and a Predicate pred for which
/// there is exactly one iterator called mid in that range such that pred(x)
/// is true for every x in [first, mid) and false for every x in [mid, last),
/// return mid.
template <class ForwardIterator, class Predicate>
static inline ForwardIterator
TfFindBoundary(ForwardIterator first, ForwardIterator last,
               Predicate const &pred)
{
    size_t len = std::distance(first, last);
    size_t half;
    ForwardIterator middle;
    
    while (len > 0) {
        half = len >> 1;
        middle = first;
        std::advance(middle, half);
        if (pred(*middle)) {
            first = middle;
            ++first;
            len = len - half - 1;
        }
        else
            len = half;
    }
    return first;
}

#endif
