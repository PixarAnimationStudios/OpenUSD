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

PXR_NAMESPACE_OPEN_SCOPE


// hdxShaderInputs implementation
bool
operator==(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs)
{
    return  lhs.parameters == rhs.parameters && 
            lhs.textures == rhs.textures     && 
            lhs.attributes == rhs.attributes;
}

bool
operator!=(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs)
{
    return !(lhs == rhs);
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
    return  lhs.wrapS == rhs.wrapS                  && 
            lhs.wrapT == rhs.wrapT                  && 
            lhs.minFilter == rhs.minFilter          && 
            lhs.magFilter == rhs.magFilter          && 
            lhs.cropTop == rhs.cropTop              && 
            lhs.cropBottom == rhs.cropBottom        && 
            lhs.cropLeft == rhs.cropLeft            && 
            lhs.cropRight == rhs.cropRight          && 
            lhs.textureMemory == rhs.textureMemory  && 
            lhs.isPtex == rhs.isPtex;
}

bool
operator!=(const HdxTextureParameters& lhs, const HdxTextureParameters& rhs)
{
    return !(lhs == rhs);
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

PXR_NAMESPACE_CLOSE_SCOPE

