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
#include "pxr/imaging/hd/shader.h"

PXR_NAMESPACE_OPEN_SCOPE

HdShader::HdShader(SdfPath const& id)
 : HdSprim(id)
{
    // NOTHING
}

HdShader::~HdShader()
{
    // NOTHING
}


// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdMaterialNodes& pv)
{
    out << "HdMaterialNodes Params: (...) " ;
    return out;
}

bool operator==(const HdMaterialRelationship& lhs, 
                const HdMaterialRelationship& rhs)
{
    return lhs.sourceId       == rhs.sourceId && 
           lhs.sourceTerminal == rhs.sourceTerminal &&
           lhs.remoteId       == rhs.remoteId &&
           lhs.remoteTerminal == rhs.remoteTerminal;
}

bool operator==(const HdMaterialNode& lhs, const HdMaterialNode& rhs)
{
    return lhs.path == rhs.path &&
           lhs.type == rhs.type &&
           lhs.parameters == rhs.parameters;
}

bool operator==(const HdMaterialNodes& lhs, const HdMaterialNodes& rhs) 
{
    return lhs.relationships           == rhs.relationships && 
           lhs.nodes                   == rhs.nodes;
}

bool operator!=(const HdMaterialNodes& lhs, const HdMaterialNodes& rhs) 
{
    return !(lhs == rhs);
}


PXR_NAMESPACE_CLOSE_SCOPE
