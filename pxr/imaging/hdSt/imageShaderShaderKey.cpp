//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/imageShaderShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,       "imageShader.glslfx"))
    ((mainVS,           "ImageShader.Vertex"))
    ((mainFS,           "ImageShader.Fragment"))
);

HdSt_ImageShaderShaderKey::HdSt_ImageShaderShaderKey()
    : glslfx(_tokens->baseGLSLFX)
{
    VS[0] = _tokens->mainVS;
    VS[1] = TfToken();

    FS[0] = _tokens->mainFS;
    FS[1] = TfToken();
}

HdSt_ImageShaderShaderKey::~HdSt_ImageShaderShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

