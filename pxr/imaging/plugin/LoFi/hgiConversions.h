//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_HGI_CONVERSIONS_H
#define PXR_IMAGING_LOFI_HGI_CONVERSIONS_H

#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/imaging/hgi/enums.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdStHgiConversions
///
/// Converts from Hd types to Hgi types
///
class LoFiHgiConversions
{
public:

    LOFI_API
    static HgiFormat GetHgiFormat(HdFormat hdFormat);

    LOFI_API
    static HgiSamplerAddressMode GetHgiSamplerAddressMode(HdWrap hdWrap);

    LOFI_API
    static HgiSamplerFilter GetHgiMagFilter(HdMagFilter hdMagFilter);

    /// The HdMinFilter translates into two Hgi enums for
    /// HgiSamplerDesc::minFilter and HgiSamplerDesc::mipFilter.
    ///
    LOFI_API
    static void GetHgiMinAndMipFilter(
        HdMinFilter hdMinFilter,
        HgiSamplerFilter *hgiSamplerFilter, HgiMipFilter *hgiMipFilter);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
