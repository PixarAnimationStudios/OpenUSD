//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_HIO_CONVERSIONS_H
#define PXR_IMAGING_HD_ST_HIO_CONVERSIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hio/types.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdStHioConversions 
{
public:
    HDST_API
    static HioFormat GetHioFormat(HdFormat hdFormat);

    HDST_API
    static HdFormat GetHdFormat(HioFormat hioFormat);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_HIO_CONVERSIONS_H
