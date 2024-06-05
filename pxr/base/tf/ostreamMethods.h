//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_OSTREAM_METHODS_H
#define PXR_BASE_TF_OSTREAM_METHODS_H

/// \file tf/ostreamMethods.h
/// \ingroup group_tf_DebuggingOutput
///
/// Handy ostream output for various lib/tf and STL containers.
///
/// These functions are useful when you need to quickly output various STL
/// containers.  The stream operators are only available if the contained
/// types have stream operators.
///
/// This facility should \e not be used to output data for later input: this
/// is essentially a "write-only" facility meant for diagnostics or
/// human-readable display; the formats described herein are subject to change
/// without notice.

#include "pxr/pxr.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/smallVector.h"

#include <ostream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

template <class T>
constexpr auto Tf_IsOstreamable_Impl(int) ->
    decltype(std::declval<std::ostream &>() << std::declval<T>(), bool())
{
    return true;
}

template <class T>
constexpr bool Tf_IsOstreamable_Impl(...) {
    return false;
}

template <class T>
constexpr bool Tf_IsOstreamable() {
    return Tf_IsOstreamable_Impl<T>(0);
}

/// Output a TfSmallVector using [ ] as delimiters.
/// \ingroup group_tf_DebuggingOutput
template <class T, uint32_t N>
typename std::enable_if<PXR_NS::Tf_IsOstreamable<T>(), std::ostream &>::type
operator<<(std::ostream &out, const TfSmallVector<T, N> &v)
{
    out << "[ ";
    for (auto const &obj: v)
        out << obj << " ";
    out << "]";

    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

// These operator<< overloads need to go in the std namespace for
// Koenig lookup to work.
namespace std {

/// Output an STL vector using [ ] as delimiters.
/// \ingroup group_tf_DebuggingOutput
template <class T>
typename std::enable_if<PXR_NS::Tf_IsOstreamable<T>(), std::ostream &>::type
operator<<(std::ostream &out, const std::vector<T> &v)
{
    out << "[ ";
    for (auto const &obj: v)
        out << obj << " ";
    out << "]";

    return out;
}

/// Output an STL set using ( ) as delimiters.
/// \ingroup group_tf_DebuggingOutput
template <class T>
typename std::enable_if<PXR_NS::Tf_IsOstreamable<T>(), std::ostream &>::type
operator<<(std::ostream &out, const std::set<T> &v)
{
    out << "( ";
    for (auto const &obj: v)
        out << obj << " ";
    out << ")";

    return out;
}

/// Output an STL list using { } as delimiters.
/// \ingroup group_tf_DebuggingOutput
template <class T>
typename std::enable_if<PXR_NS::Tf_IsOstreamable<T>(), std::ostream &>::type
operator<<(std::ostream &out, const std::list<T> &l)
{
    out << "{ ";
    for (auto const &obj: l)
        out << obj << " ";
    out << "}";

    return out;
}

/// Output an TfHashMap using < > as delimiters.
/// \ingroup group_tf_DebuggingOutput
template <class K, class M, class H, class C, class A>
typename std::enable_if<
    PXR_NS::Tf_IsOstreamable<K>() && PXR_NS::Tf_IsOstreamable<M>(), std::ostream &>::type
operator<<(std::ostream &out, const PXR_NS::TfHashMap<K, M, H, C, A> &h)
{
    out << "< ";
    for (auto const &p: h)
        out << "<" << p.first << ": " << p.second << "> ";
    out << ">";
    return out;
}

/// Output an STL map using < > as delimiters.
/// \ingroup group_tf_DebuggingOutput
template <class K, class M>
typename std::enable_if<
    PXR_NS::Tf_IsOstreamable<K>() && PXR_NS::Tf_IsOstreamable<M>(), std::ostream &>::type
operator<<(std::ostream &out, const std::map<K, M> &h)
{
    out << "< ";
    for (auto const &p: h)
        out << "<" << p.first << ": " << p.second << "> ";
    out << ">";
    return out;
}

} // namespace std

#endif // PXR_BASE_TF_OSTREAM_METHODS_H 
