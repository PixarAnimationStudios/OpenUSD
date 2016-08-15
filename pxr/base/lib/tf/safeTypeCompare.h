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
#ifndef TF_SAFETYPECOMPARE_H
#define TF_SAFETYPECOMPARE_H

/// \file tf/safeTypeCompare.h
/// \ingroup group_tf_RuntimeTyping
/// Safely compare C++ RTTI type structures.

#include <typeinfo>

/// Safely compare \c std::type_info structures.
///
/// Returns \c true if \p t1 and \p t2 denote the same type.
inline bool TfSafeTypeCompare(const std::type_info& t1, const std::type_info& t2) {
    return t1 == t2;
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

#endif
