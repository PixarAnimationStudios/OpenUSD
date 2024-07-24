//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_TYPE_NOTICE_H
#define PXR_BASE_TF_TYPE_NOTICE_H

#include "pxr/pxr.h"

#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfTypeWasDeclaredNotice
///
/// TfNotice sent after a TfType is declared.
class TfTypeWasDeclaredNotice : public TfNotice
{
public:
    TfTypeWasDeclaredNotice( TfType t );
    virtual ~TfTypeWasDeclaredNotice();

    /// Get the newly declared TfType.
    TfType GetType() const { return _type; }

private:
    TfType _type;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_TYPE_NOTICE_H
