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
#include "pxr/imaging/hdSt/materialParam.h"

#include "pxr/base/tf/staticTokens.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

HdSt_MaterialParam::HdSt_MaterialParam()
    : paramType(ParamTypeFallback)
    , name()
    , fallbackValue()
    , samplerCoords()
    , textureType(HdTextureType::Uv)
    , swizzle()
    , isPremultiplied(false)
{
}

HdSt_MaterialParam::HdSt_MaterialParam(ParamType paramType,
                                 TfToken const& name, 
                                 VtValue const& fallbackValue,
                                 TfTokenVector const& samplerCoords,
                                 HdTextureType textureType,
                                 std::string const& swizzle,
                                 bool const isPremultiplied)
    : paramType(paramType)
    , name(name)
    , fallbackValue(fallbackValue)
    , samplerCoords(samplerCoords)
    , textureType(textureType)
    , swizzle(swizzle)
    , isPremultiplied(isPremultiplied)
{
}

size_t
HdSt_MaterialParam::ComputeHash(HdSt_MaterialParamVector const &params)
{
    size_t hash = 0;
    for (HdSt_MaterialParam const& param : params) {
        boost::hash_combine(hash, param.paramType);
        boost::hash_combine(hash, param.name.Hash());
        for (TfToken const& coord : param.samplerCoords) {
            boost::hash_combine(hash, coord.Hash());
        }
        boost::hash_combine(hash, param.textureType);
        boost::hash_combine(hash, param.swizzle);
        boost::hash_combine(hash, param.isPremultiplied);
    }
    return hash;
}

HdTupleType
HdSt_MaterialParam::GetTupleType() const
{
    return HdGetValueTupleType(fallbackValue);
}

PXR_NAMESPACE_CLOSE_SCOPE

