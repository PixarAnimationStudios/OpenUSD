//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/materialParam.h"

#include "pxr/base/tf/staticTokens.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSt_MaterialParam::HdSt_MaterialParam()
    : paramType(ParamTypeFallback)
    , name()
    , fallbackValue()
    , samplerCoords()
    , textureType(HdStTextureType::Uv)
    , swizzle()
    , isPremultiplied(false)
    , arrayOfTexturesSize(0)
{
}

HdSt_MaterialParam::HdSt_MaterialParam(ParamType paramType,
                                 TfToken const& name, 
                                 VtValue const& fallbackValue,
                                 TfTokenVector const& samplerCoords,
                                 HdStTextureType textureType,
                                 std::string const& swizzle,
                                 bool const isPremultiplied,
                                 size_t const arrayOfTexturesSize)
    : paramType(paramType)
    , name(name)
    , fallbackValue(fallbackValue)
    , samplerCoords(samplerCoords)
    , textureType(textureType)
    , swizzle(swizzle)
    , isPremultiplied(isPremultiplied)
    , arrayOfTexturesSize(arrayOfTexturesSize)
{
}

size_t
HdSt_MaterialParam::ComputeHash(HdSt_MaterialParamVector const &params)
{
    size_t hash = 0;
    for (HdSt_MaterialParam const& param : params) {
        hash = TfHash::Combine(
            hash,
            param.paramType,
            param.name,
            param.samplerCoords,
            param.textureType,
            param.swizzle,
            param.isPremultiplied,
            param.arrayOfTexturesSize
        );
    }
    return hash;
}

HdTupleType
HdSt_MaterialParam::GetTupleType() const
{
    return HdGetValueTupleType(fallbackValue);
}

PXR_NAMESPACE_CLOSE_SCOPE

