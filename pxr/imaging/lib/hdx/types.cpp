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
#include "pxr/imaging/hdx/types.h"

#include <iostream>

// hdxShaderInputs implementation
bool
operator==(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs)
{
    return  lhs.parameters == rhs.parameters and
            lhs.textures == rhs.textures and
            lhs.attributes == rhs.attributes;
}

bool
operator!=(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs)
{
    return not (lhs == rhs);
}

std::ostream&
operator<<(std::ostream& out, const HdxShaderInputs& pv)
{
    out <<  pv.parameters << " " << pv.textures << " ";

    TF_FOR_ALL(it, pv.attributes) {
        out << *it;
    }
    return out;
}

// HdxTextureParameters implementation
bool
operator==(const HdxTextureParameters& lhs, const HdxTextureParameters& rhs)
{
    return  lhs.wrapS == rhs.wrapS and
            lhs.wrapT == rhs.wrapT and
            lhs.minFilter == rhs.minFilter and
            lhs.magFilter == rhs.magFilter and
            lhs.cropTop == rhs.cropTop and
            lhs.cropBottom == rhs.cropBottom and
            lhs.cropLeft == rhs.cropLeft and
            lhs.cropRight == rhs.cropRight and
            lhs.textureMemory == rhs.textureMemory and
            lhs.isPtex == rhs.isPtex;
}

bool
operator!=(const HdxTextureParameters& lhs, const HdxTextureParameters& rhs)
{
    return not (lhs == rhs);
}

std::ostream&
operator<<(std::ostream& out, const HdxTextureParameters& pv)
{
    out << pv.wrapS << " " 
        << pv.wrapT << " "
        << pv.minFilter << " "
        << pv.magFilter << " "
        << pv.cropTop << " "
        << pv.cropBottom << " "
        << pv.cropLeft << " "
        << pv.cropRight << " "
        << pv.textureMemory << " "
        << pv.isPtex;
    return out;
}
