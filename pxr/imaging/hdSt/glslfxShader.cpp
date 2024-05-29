//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/glslfxShader.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hio/glslfx.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStGLSLFXShader::HdStGLSLFXShader(HioGlslfxSharedPtr const& glslfx)
 : HdSt_MaterialNetworkShader()
    , _glslfx(glslfx)
{
    _SetSource(HdShaderTokens->fragmentShader, _glslfx->GetSurfaceSource());
    _SetSource(HdShaderTokens->displacementShader,
               _glslfx->GetDisplacementSource());
}

HdStGLSLFXShader::~HdStGLSLFXShader()
{
}

void
HdStGLSLFXShader::Reload()
{
    HioGlslfxSharedPtr newGlslFx =
        std::make_shared<HioGlslfx>(_glslfx->GetFilePath());

    if (newGlslFx->IsValid())
    {
        _glslfx = newGlslFx;

        _SetSource(HdShaderTokens->fragmentShader, _glslfx->GetSurfaceSource());
        _SetSource(HdShaderTokens->displacementShader,
                   _glslfx->GetDisplacementSource());
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
