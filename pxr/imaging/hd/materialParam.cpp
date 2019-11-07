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
#include "pxr/imaging/hd/materialParam.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/envSetting.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

HdMaterialParam::HdMaterialParam()
{
}

HdMaterialParam::HdMaterialParam(ParamType paramType,
                                 TfToken const& name, 
                                 VtValue const& fallbackValue,
                                 SdfPath const& connection,
                                 TfTokenVector const& samplerCoords,
                                 HdTextureType textureType)
    : paramType(paramType)
    , name(name)
    , fallbackValue(fallbackValue)
    , connection(connection)
    , samplerCoords(samplerCoords)
    , textureType(textureType)
{
}

HdMaterialParam::~HdMaterialParam()
{
}

size_t
HdMaterialParam::ComputeHash(HdMaterialParamVector const &params)
{
    size_t hash = 0;
    for (HdMaterialParam const& param : params) {
        boost::hash_combine(hash, param.paramType);
        boost::hash_combine(hash, param.name.Hash());
        boost::hash_combine(hash, param.connection.GetHash());
        for (TfToken const& coord : param.samplerCoords) {
            boost::hash_combine(hash, coord.Hash());
        }
        boost::hash_combine(hash, param.textureType);
    }
    return hash;
}

HdTupleType
HdMaterialParam::GetTupleType() const
{
    return HdGetValueTupleType(fallbackValue);
}

PXR_NAMESPACE_CLOSE_SCOPE

