//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SAFE_TYPE_COMPARE_H
#define PXR_BASE_TF_SAFE_TYPE_COMPARE_H

/// \file tf/safeTypeCompare.h
/// \ingroup group_tf_RuntimeTyping
/// Safely compare C++ RTTI type structures.

#include "pxr/pxr.h"

#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

/// Safely compare \c std::type_info structures.
///
/// Returns \c true if \p t1 and \p t2 denote the same type.
inline bool TfSafeTypeCompare(const std::type_info& t1, const std::type_info& t2) {
#if defined(__APPLE__)
    // Workaround until issue 1475 aka USD-6608 is properly fixed. This way USD
    // containers, in particular VtArray objects, can be passed between DSOs
    // linked with USD (e.g. plugins) compiled with hidden visibility and have
    // their contents safely accessed. USD itself still has to be compiled with
    // default visibility.
    // The two types are compared by name explicitly, as opposed to the default
    // implementation which compares them as pointers and relies on typeinfos
    // from different DSOs being properly merged at load time. __builtin_strcmp
    // is used to avoid bringing in unneeded headers.
    return __builtin_strcmp(t1.name(), t2.name()) == 0;
#else
    return t1 == t2;
#endif
}

/// Safely perform a dynamic cast.
///
/// Usage should mirror regular \c dynamic_cast:
/// \code
///     Derived* d = TfSafeDynamic_cast<Derived*>(basePtr);
/// \endcode
///  
/// Note that this function also works with \c TfRefPtr and \c TfWeakPtr
/// managed objects.
template <typename TO, typename FROM>
TO
TfSafeDynamic_cast(FROM* ptr) {
        return dynamic_cast<TO>(ptr);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SAFE_TYPE_COMPARE_H
