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
#include "pxr/imaging/hdSt/shaderKey.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/token.h"
#include <boost/functional/hash.hpp>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

/*virtual*/
HdSt_ShaderKey::~HdSt_ShaderKey()
{
    /*NOTHING*/
}

HdSt_ShaderKey::ID
HdSt_ShaderKey::ComputeHash() const
{
    ID hash = GetGlslfxFilename().Hash();

    TfToken const *VS = GetVS();
    TfToken const *TCS = GetTCS();
    TfToken const *TES = GetTES();
    TfToken const *GS = GetGS();
    TfToken const *FS = GetFS();

    while (VS && (!VS->IsEmpty())) {
        boost::hash_combine(hash, VS->Hash());
        ++VS;
    }
    while (TCS && (!TCS->IsEmpty())) {
        boost::hash_combine(hash, TCS->Hash());
        ++TCS;
    }
    while (TES && (!TES->IsEmpty())) {
        boost::hash_combine(hash, TES->Hash());
        ++TES;
    }
    while (GS && (!GS->IsEmpty())) {
        boost::hash_combine(hash, GS->Hash());
        ++GS;
    }
    while (FS && (!FS->IsEmpty())) {
        boost::hash_combine(hash, FS->Hash());
        ++FS;
    }
    
    // During batching, we rely on geometric shader equality, and thus the
    // shaderKey hash factors the following state opinions besides the mixins
    // themselves. 
    // Note that the GLSL programs still can be shared across GeometricShader
    // instances, when they are identical except the GL states, as long as
    // Hd_GeometricShader::ComputeHash() provides consistent hash values.
    boost::hash_combine(hash, GetPrimitiveType());    
    boost::hash_combine(hash, GetCullStyle());
    boost::hash_combine(hash, UseHardwareFaceCulling());
    if (UseHardwareFaceCulling()) {
        boost::hash_combine(hash, HasMirroredTransform());
        boost::hash_combine(hash, IsDoubleSided());
    }
    boost::hash_combine(hash, GetPolygonMode());
    boost::hash_combine(hash, IsFrustumCullingPass());
    boost::hash_combine(hash, GetLineWidth());

    return hash;
}

static
std::string
_JoinTokens(const char *stage, TfToken const *tokens, bool *firstStage)
{
    if ((!tokens) || tokens->IsEmpty()) return std::string();

    std::stringstream ss;

    if (*firstStage == false) ss << ", ";
    *firstStage = false;

    ss << "\"" << stage << "\" : { "
       << "\"source\" : [";

    bool first = true;
    while (!tokens->IsEmpty()) {
        if (first == false) ss << ", ";
        ss << "\"" << tokens->GetText() << "\"";
        ++tokens;
        first = false;
    }

    ss << "] }\n";
    return ss.str();
}

std::string
HdSt_ShaderKey::GetGlslfxString() const
{
    std::stringstream ss;

    ss << "-- glslfx version 0.1\n";

    if (!GetGlslfxFilename().IsEmpty())
        ss << "#import $TOOLS/hdSt/shaders/" 
           << GetGlslfxFilename().GetText() << "\n";

    ss << "-- configuration\n"
       << "{\"techniques\": {\"default\": {\n";

    bool firstStage = true;
    ss << _JoinTokens("vertexShader",      GetVS(),  &firstStage);
    ss << _JoinTokens("tessControlShader", GetTCS(), &firstStage);
    ss << _JoinTokens("tessEvalShader",    GetTES(), &firstStage);
    ss << _JoinTokens("geometryShader",    GetGS(),  &firstStage);
    ss << _JoinTokens("fragmentShader",    GetFS(),  &firstStage);
    ss << "}}}\n";

    return ss.str();
}

/*virtual*/
TfToken const*
HdSt_ShaderKey::GetVS() const
{
    return nullptr;
}

/*virtual*/
TfToken const*
HdSt_ShaderKey::GetTCS() const
{
    return nullptr;
}

/*virtual*/
TfToken const*
HdSt_ShaderKey::GetTES() const
{
    return nullptr;
}

/*virtual*/
TfToken const*
HdSt_ShaderKey::GetGS() const
{
    return nullptr;
}

/*virtual*/
TfToken const*
HdSt_ShaderKey::GetFS() const
{
    return nullptr;
}

/*virtual*/
bool
HdSt_ShaderKey::IsFrustumCullingPass() const
{
    return false;
}

/*virtual*/
HdCullStyle
HdSt_ShaderKey::GetCullStyle() const
{
    return HdCullStyleDontCare;
}

/*virtual*/
bool
HdSt_ShaderKey::UseHardwareFaceCulling() const
{
    return false;
}

/*virtual*/
bool
HdSt_ShaderKey::HasMirroredTransform() const
{
    return false;
}

/*virtual*/
bool
HdSt_ShaderKey::IsDoubleSided() const
{
    return false;
}

/*virtual*/
HdPolygonMode 
HdSt_ShaderKey::GetPolygonMode() const
{
    return HdPolygonModeFill;
}

/*virtual*/
float
HdSt_ShaderKey::GetLineWidth() const
{
    return 0.0f;
}

PXR_NAMESPACE_CLOSE_SCOPE

