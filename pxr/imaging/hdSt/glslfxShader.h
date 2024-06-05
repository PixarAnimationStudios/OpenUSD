//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_GLSLFX_SHADER_H
#define PXR_IMAGING_HD_ST_GLSLFX_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/materialNetworkShader.h"

PXR_NAMESPACE_OPEN_SCOPE

using HioGlslfxSharedPtr = std::shared_ptr<class HioGlslfx>;

/// \class HdStGLSLFXShader
///
/// A simple specialization of HdSt_MaterialNetworkShader used to
/// load the built-in fallback material network.
///
class HdStGLSLFXShader final : public HdSt_MaterialNetworkShader
{
public:
    HDST_API
    HdStGLSLFXShader(HioGlslfxSharedPtr const& glslfx);
    HDST_API
    ~HdStGLSLFXShader() override;

    /// If the prim is based on asset, reload that asset.
    HDST_API
    void Reload() override;

private:
    HioGlslfxSharedPtr _glslfx;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_GLSLFX_SHADER_H
