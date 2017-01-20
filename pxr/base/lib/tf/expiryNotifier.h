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
#ifndef TF_EXPIRYNOTIFIER_H
#define TF_EXPIRYNOTIFIER_H

/// \file tf/expiryNotifier.h

#include "pxr/pxr.h"
#include "pxr/base/arch/hints.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class Tf_ExpiryNotifier
///
/// Set the extra notifier function.
///
/// Objects derived from \c TfWeakBase can request that extra notification be
/// performed when they expire.  Objects that do not request extra
/// notification do not suffer any performance penalty when they expire, while
/// objects that request the notification will have the extra notifier
/// function run on them, with the address returned by \c
/// TfWeakPtr::GetUniqueIdentifier() passed to the notifier function.
///
/// An object requests extra notification be performed by calling \c
/// wp.RequestExtraNotification() where \c wp is a \c TfWeakPtr pointing to
/// the object in question.
///
/// This functions sets the extra notifier function to \p func.  At present,
/// this notifier is intended to be used only by the scripting system to
/// inform the system that a particular object has expired and scripting
/// resources devoted to that object can be discarded.  Accordingly, if
/// \p func is non-NULL and a previous non-NULL notifier function has been
/// set, a fatal error is issued.
class Tf_ExpiryNotifier {
public:
    static void Invoke(void const *p);
    static void SetNotifier(void (*func)(void const *));

    static void Invoke2(void const *p);
    static void SetNotifier2(void (*func)(void const *));
private:

    static void (*_func)(void const *);
    static void (*_func2)(void const *);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
