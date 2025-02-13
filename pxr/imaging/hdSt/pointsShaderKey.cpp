//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/pointsShaderKey.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((baseGLSLFX,         "points.glslfx"))

    // point id mixins (for point picking & selection)
    ((pointIdVS,                "PointId.Vertex.PointParam"))
    ((pointIdSelDecodeUtilsVS,  "Selection.DecodeUtils"))
    ((pointIdSelPointSelVS,     "Selection.Vertex.PointSel"))
    ((pointIdFS,                "PointId.Fragment.PointParam"))

    // main for all the shader stages
    ((mainVS,                   "Point.Vertex"))
    ((mainFS,                   "Point.Fragment"))

    // terminals        
    ((commonFS,                 "Fragment.CommonTerminals"))
    ((surfaceFS,                "Fragment.Surface"))
    ((noScalarOverrideFS,       "Fragment.NoScalarOverride"))

    // instancing       
    ((instancing,               "Instancing.Transform"))

    // rounded points
    ((pointSizeBiasVS,          "PointDisk.Vertex.PointSizeBias"))
    ((noPointSizeBiasVS,        "PointDisk.Vertex.None"))
    ((diskSampleMaskFS,         "PointDisk.Fragment.SampleMask"))
    ((noDiskSampleMaskFS,       "PointDisk.Fragment.None"))
);

HdSt_PointsShaderKey::HdSt_PointsShaderKey(
    bool nativeRoundPoints)
    : glslfx(_tokens->baseGLSLFX)
{
    VS[0] = _tokens->instancing;
    VS[1] = nativeRoundPoints ? _tokens->noPointSizeBiasVS :
        _tokens->pointSizeBiasVS;
    VS[2] = _tokens->mainVS;
    VS[3] = _tokens->pointIdVS;
    VS[4] = _tokens->pointIdSelDecodeUtilsVS;
    VS[5] = _tokens->pointIdSelPointSelVS;
    VS[6] = TfToken();

    // Common must be first as it defines terminal interfaces
    FS[0] = _tokens->commonFS;
    FS[1] = _tokens->surfaceFS;
    FS[2] = _tokens->noScalarOverrideFS;
    FS[3] = nativeRoundPoints ? _tokens->noDiskSampleMaskFS :
        _tokens->diskSampleMaskFS;
    FS[4] = _tokens->mainFS;
    FS[5] = _tokens->pointIdFS;
    FS[6] = TfToken();
}

HdSt_PointsShaderKey::~HdSt_PointsShaderKey()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

