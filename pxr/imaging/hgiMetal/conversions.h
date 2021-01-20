//
// Copyright 2020 Pixar
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
    static MTLPixelFormat GetPixelFormat(HgiFormat inFormat);

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
    static MTLCompareFunction GetDepthCompareFunction(HgiCompareFunction cf);
    
    HGIMETAL_API
    static MTLTextureType GetTextureType(HgiTextureType tt);

    HGIMETAL_API
    static MTLSamplerAddressMode GetSamplerAddressMode(HgiSamplerAddressMode a);

    HGIMETAL_API
    static MTLSamplerMinMagFilter GetMinMagFilter(HgiSamplerFilter mf);

    HGIMETAL_API
    static MTLSamplerMipFilter GetMipFilter(HgiMipFilter mf);

#if (defined(__MAC_10_15) && __MAC_OS_X_VERSION_MAX_ALLOWED >= __MAC_10_15) \
    || __IPHONE_OS_VERSION_MAX_ALLOWED >= 130000
    HGIMETAL_API
    static MTLTextureSwizzle GetComponentSwizzle(HgiComponentSwizzle);
#endif

    HGIMETAL_API
    static MTLPrimitiveTopologyClass GetPrimitiveClass(HgiPrimitiveType pt);

    HGIMETAL_API
    static MTLPrimitiveType GetPrimitiveType(HgiPrimitiveType pt);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

