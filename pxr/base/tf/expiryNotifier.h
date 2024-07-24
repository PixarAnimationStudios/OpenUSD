//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_EXPIRY_NOTIFIER_H
#define PXR_BASE_TF_EXPIRY_NOTIFIER_H

/// \file tf/expiryNotifier.h

#include "pxr/pxr.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/api.h"

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
    TF_API static void Invoke(void const *p);
    TF_API static void SetNotifier(void (*func)(void const *));

    TF_API static void Invoke2(void const *p);
    TF_API static void SetNotifier2(void (*func)(void const *));
private:

    static void (*_func)(void const *);
    static void (*_func2)(void const *);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
