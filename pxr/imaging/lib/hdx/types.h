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
#ifndef HDX_TYPES_H
#define HDX_TYPES_H

#include "pxr/imaging/hdx/version.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/imaging/hd/enums.h"

// Struct used to send shader inputs from Presto and send them to Hydra
struct HdxShaderInputs
{
    VtDictionary         parameters;
    VtDictionary         textures;
    std::vector<TfToken> attributes;
};

bool operator==(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs);
bool operator!=(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs);
std::ostream& operator<<(std::ostream& out, const HdxShaderInputs& pv);

// Struct used to send texture parameters from Presto and send them to Hydra
struct HdxTextureParameters
{
    HdWrap wrapS;
    HdWrap wrapT;
    HdMinFilter minFilter;
    HdMagFilter magFilter;
    int cropTop;
    int cropBottom;
    int cropLeft;
    int cropRight;
    float textureMemory;
    bool isPtex;
};

bool operator==(const HdxTextureParameters& lhs, const HdxTextureParameters& rhs);
bool operator!=(const HdxTextureParameters& lhs, const HdxTextureParameters& rhs);
std::ostream& operator<<(std::ostream& out, const HdxTextureParameters& pv);

#endif //HDX_TYPES_H
