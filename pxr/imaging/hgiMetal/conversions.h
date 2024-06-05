//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_CONVERSIONS_H
#define PXR_IMAGING_HGI_METAL_CONVERSIONS_H

#include <Metal/Metal.h>
#include "pxr/pxr.h"
#include "pxr/imaging/hgiMetal/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiMetalConversions
///
/// Converts from Hgi types to Metal types.
///
class HgiMetalConversions final
{
public:
    //
    // Hgi to Metal conversions
    //

    HGIMETAL_API
    static MTLPixelFormat GetPixelFormat(HgiFormat inFormat,
                                         HgiTextureUsage inUsage);

    HGIMETAL_API
    static MTLVertexFormat GetVertexFormat(HgiFormat inFormat);
    
    HGIMETAL_API
    static MTLCullMode GetCullMode(HgiCullMode cm);

    HGIMETAL_API
    static MTLTriangleFillMode GetPolygonMode(HgiPolygonMode pm);
    
    HGIMETAL_API
    static MTLBlendFactor GetBlendFactor(HgiBlendFactor bf);

    HGIMETAL_API
    static MTLBlendOperation GetBlendEquation(HgiBlendOp bo);
    
    HGIMETAL_API
    static MTLWinding GetWinding(HgiWinding winding);
    
    HGIMETAL_API
    static MTLLoadAction GetAttachmentLoadOp(HgiAttachmentLoadOp loadOp);

    HGIMETAL_API
    static MTLStoreAction GetAttachmentStoreOp(HgiAttachmentStoreOp storeOp);
    
    HGIMETAL_API
    static MTLCompareFunction GetCompareFunction(HgiCompareFunction cf);

    HGIMETAL_API
    static MTLStencilOperation GetStencilOp(HgiStencilOp op);
    
    HGIMETAL_API
    static MTLTextureType GetTextureType(HgiTextureType tt);

    HGIMETAL_API
    static MTLSamplerAddressMode GetSamplerAddressMode(HgiSamplerAddressMode a);

    HGIMETAL_API
    static MTLSamplerMinMagFilter GetMinMagFilter(HgiSamplerFilter mf);

    HGIMETAL_API
    static MTLSamplerMipFilter GetMipFilter(HgiMipFilter mf);

    HGIMETAL_API
    static MTLSamplerBorderColor GetBorderColor(HgiBorderColor bc);

#if (defined(__MAC_10_15) && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_15) \
    || __IPHONE_OS_VERSION_MAX_ALLOWED >= 130000
    HGIMETAL_API
    static MTLTextureSwizzle GetComponentSwizzle(HgiComponentSwizzle);
#endif

    HGIMETAL_API
    static MTLPrimitiveTopologyClass GetPrimitiveClass(HgiPrimitiveType pt);

    HGIMETAL_API
    static MTLPrimitiveType GetPrimitiveType(HgiPrimitiveType pt);

    HGIMETAL_API
    static MTLColorWriteMask GetColorWriteMask(HgiColorMask mask);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

