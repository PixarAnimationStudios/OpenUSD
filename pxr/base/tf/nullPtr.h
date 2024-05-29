//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_NULL_PTR_H
#define PXR_BASE_TF_NULL_PTR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

PXR_NAMESPACE_OPEN_SCOPE

// A type used to create the \a TfNullPtr token.
struct TfNullPtrType
{
};

// A token to represent null for smart pointers like \a TfWeakPtr and \a
// TfRefPtr.
TF_API extern const TfNullPtrType TfNullPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_NULL_PTR_H
