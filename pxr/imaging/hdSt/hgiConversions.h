//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_HGI_CONVERSIONS_H
#define PXR_IMAGING_HD_ST_HGI_CONVERSIONS_H

#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/imaging/hgi/enums.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdStHgiConversions
///
/// Converts from Hd types to Hgi types
///
class HdStHgiConversions
{
public:

    HDST_API
    static HgiFormat GetHgiFormat(HdFormat hdFormat);

    HDST_API
    static HgiFormat GetHgiVertexFormat(HdType hdType);

    HDST_API
    static HgiSamplerAddressMode GetHgiSamplerAddressMode(HdWrap hdWrap);

    HDST_API
    static HgiSamplerFilter GetHgiMagFilter(HdMagFilter hdMagFilter);

    /// The HdMinFilter translates into two Hgi enums for
    /// HgiSamplerDesc::minFilter and HgiSamplerDesc::mipFilter.
    ///
    HDST_API
    static void GetHgiMinAndMipFilter(
        HdMinFilter hdMinFilter,
        HgiSamplerFilter *hgiSamplerFilter, HgiMipFilter *hgiMipFilter);

    HDST_API
    static HgiBorderColor GetHgiBorderColor(HdBorderColor hdBorderColor);

    HDST_API
    static HgiCompareFunction GetHgiCompareFunction(
        HdCompareFunction hdCompareFunc);

    HDST_API
    static HgiStencilOp GetHgiStencilOp(HdStencilOp hdStencilOp);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
