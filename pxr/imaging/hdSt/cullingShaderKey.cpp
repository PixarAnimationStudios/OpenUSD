//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/cullingShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,       "frustumCull.glslfx"))
    ((instancing,       "Instancing.Transform"))
    ((counting,         "ViewFrustumCull.Counting"))
    ((noCounting,       "ViewFrustumCull.NoCounting"))
    ((tinyCull,         "ViewFrustumCull.TinyCull"))
    ((noTinyCull,       "ViewFrustumCull.NoTinyCull"))
    ((isVisible,        "ViewFrustumCull.IsVisible"))
    ((mainInstancingVS, "ViewFrustumCull.VertexInstancing"))
    ((mainVS,           "ViewFrustumCull.Vertex"))
    ((mainInstancingCS, "ViewFrustumCull.ComputeInstancing"))
    ((mainCS,           "ViewFrustumCull.Compute"))
);

HdSt_CullingShaderKey::HdSt_CullingShaderKey(
    bool instancing, bool tinyCull, bool counting)
    : glslfx(_tokens->baseGLSLFX)
{

    VS[0] = _tokens->instancing;
    VS[1] = counting ? _tokens->counting : _tokens->noCounting;
    VS[2] = tinyCull ? _tokens->tinyCull : _tokens->noTinyCull;
    VS[3] = _tokens->isVisible;
    VS[4] = instancing ? _tokens->mainInstancingVS : _tokens->mainVS;
    VS[5] = TfToken();
}

HdSt_CullingShaderKey::~HdSt_CullingShaderKey()
{
}

HdSt_CullingComputeShaderKey::HdSt_CullingComputeShaderKey(
    bool instancing, bool tinyCull, bool counting)
    : glslfx(_tokens->baseGLSLFX)
{

    CS[0] = _tokens->instancing;
    CS[1] = counting ? _tokens->counting : _tokens->noCounting;
    CS[2] = tinyCull ? _tokens->tinyCull : _tokens->noTinyCull;
    CS[3] = _tokens->isVisible;
    CS[4] = instancing ? _tokens->mainInstancingCS : _tokens->mainCS;
    CS[5] = TfToken();
}

HdSt_CullingComputeShaderKey::~HdSt_CullingComputeShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

