//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/shaderKey.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/hash.h"
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
    TfToken const *PTCS = GetPTCS();
    TfToken const *PTVS = GetPTVS();
    TfToken const *GS = GetGS();
    TfToken const *FS = GetFS();
    TfToken const *CS = GetCS();

    while (VS && (!VS->IsEmpty())) {
        hash = TfHash::Combine(hash, VS->Hash());
        ++VS;
    }
    while (TCS && (!TCS->IsEmpty())) {
        hash = TfHash::Combine(hash, TCS->Hash());
        ++TCS;
    }
    while (TES && (!TES->IsEmpty())) {
        hash = TfHash::Combine(hash, TES->Hash());
        ++TES;
    }
    while (PTCS && (!PTCS->IsEmpty())) {
        hash = TfHash::Combine(hash, PTCS->Hash());
        ++PTCS;
    }
    while (PTVS && (!PTVS->IsEmpty())) {
        hash = TfHash::Combine(hash, PTVS->Hash());
        ++PTVS;
    }
    while (GS && (!GS->IsEmpty())) {
        hash = TfHash::Combine(hash, GS->Hash());
        ++GS;
    }
    while (FS && (!FS->IsEmpty())) {
        hash = TfHash::Combine(hash, FS->Hash());
        ++FS;
    }
    while (CS && (!CS->IsEmpty())) {
        hash = TfHash::Combine(hash, CS->Hash());
        ++CS;
    }
    
    // During batching, we rely on geometric shader equality, and thus the
    // shaderKey hash factors the following state opinions besides the mixins
    // themselves. 
    // Note that the GLSL programs still can be shared across GeometricShader
    // instances, when they are identical except the GL states, as long as
    // Hd_GeometricShader::ComputeHash() provides consistent hash values.
    hash = TfHash::Combine(
        hash,
        GetPrimitiveType(),
        GetCullStyle(),
        UseHardwareFaceCulling()
    );
    if (UseHardwareFaceCulling()) {
        hash = TfHash::Combine(
            hash,
            HasMirroredTransform(),
            IsDoubleSided()
        );
    }
    hash = TfHash::Combine(
        hash,
        GetPolygonMode(),
        IsFrustumCullingPass(),
        GetLineWidth(),
        GetFvarPatchType()
    );

    return hash;
}

/*static*/
std::string
HdSt_ShaderKey::_JoinTokens(
    const char *stage, TfToken const *tokens, bool *firstStage)
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
    ss << _JoinTokens("computeShader",     GetCS(),  &firstStage);
    ss << _JoinTokens("vertexShader",      GetVS(),  &firstStage);
    ss << _JoinTokens("tessControlShader", GetTCS(), &firstStage);
    ss << _JoinTokens("tessEvalShader",    GetTES(), &firstStage);
    ss << _JoinTokens("postTessControlShader",  GetPTCS(), &firstStage);
    ss << _JoinTokens("postTessVertexShader",   GetPTVS(), &firstStage);
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
HdSt_ShaderKey::GetPTCS() const
{
    return nullptr;
}

/*virtual*/
TfToken const*
HdSt_ShaderKey::GetPTVS() const
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
TfToken const*
HdSt_ShaderKey::GetCS() const
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
bool
HdSt_ShaderKey::UseMetalTessellation() const
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

/*virtual*/
HdSt_GeometricShader::FvarPatchType 
HdSt_ShaderKey::GetFvarPatchType() const {
    return HdSt_GeometricShader::FvarPatchType::PATCH_NONE;
}

PXR_NAMESPACE_CLOSE_SCOPE

