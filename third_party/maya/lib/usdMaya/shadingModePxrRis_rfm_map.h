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
// This table comes from RenderMan for Maya version 20.1
// The software does under the hood transformation implementing some Maya native shader nodes
// as RIS patter objects. The conversion allows to write in USD a fully working shader graph
// We use this table both for export and for import when RIS mode is active.
// XXX This should probably live in a xml or json file that can be easily updated without recompile

#include "pxr/pxr.h"

#include "pxr/base/tf/token.h"

#include <utility>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

static const std::vector<std::pair<TfToken, TfToken> > _RFM_RISNODE_TABLE = {
    { TfToken("blendColors", TfToken::Immortal), TfToken("PxrMayaBlendColors", TfToken::Immortal) },
    { TfToken("bulge", TfToken::Immortal), TfToken("PxrMayaBulge", TfToken::Immortal) },
    { TfToken("bump2d", TfToken::Immortal), TfToken("PxrMayaBump2d", TfToken::Immortal) },
    { TfToken("bump3d", TfToken::Immortal), TfToken("PxrMayaBump3d", TfToken::Immortal) },
    { TfToken("brownian", TfToken::Immortal), TfToken("PxrMayaBrownian", TfToken::Immortal) },
    { TfToken("checker", TfToken::Immortal), TfToken("PxrMayaChecker", TfToken::Immortal) },
    { TfToken("clamp", TfToken::Immortal), TfToken("PxrMayaClamp", TfToken::Immortal) },
    { TfToken("cloth", TfToken::Immortal), TfToken("PxrMayaCloth", TfToken::Immortal) },
    { TfToken("cloud", TfToken::Immortal), TfToken("PxrMayaCloud", TfToken::Immortal) },
    { TfToken("condition", TfToken::Immortal), TfToken("PxrCondition", TfToken::Immortal) },
    { TfToken("contrast", TfToken::Immortal), TfToken("PxrMayaContrast", TfToken::Immortal) },
    { TfToken("crater", TfToken::Immortal), TfToken("PxrMayaCrater", TfToken::Immortal) },
    { TfToken("file", TfToken::Immortal), TfToken("PxrMayaFile", TfToken::Immortal) },
    { TfToken("fluidShape", TfToken::Immortal), TfToken("PxrMayaFluidShape", TfToken::Immortal) },
    { TfToken("fractal", TfToken::Immortal), TfToken("PxrMayaFractal", TfToken::Immortal) },
    { TfToken("gammaCorrect", TfToken::Immortal), TfToken("PxrMayaGammaCorrect", TfToken::Immortal) },
    { TfToken("granite", TfToken::Immortal), TfToken("PxrMayaGranite", TfToken::Immortal) },
    { TfToken("grid", TfToken::Immortal), TfToken("PxrMayaGrid", TfToken::Immortal) },
    { TfToken("hairSystem", TfToken::Immortal), TfToken("PxrMayaHair", TfToken::Immortal) },
    { TfToken("hsvToRgb", TfToken::Immortal), TfToken("PxrMayaHsvToRgb", TfToken::Immortal) },
    { TfToken("imagePlane", TfToken::Immortal), TfToken("PxrMayaImagePlane", TfToken::Immortal) },
    { TfToken("layeredTexture", TfToken::Immortal), TfToken("PxrMayaLayeredTexture", TfToken::Immortal) },
    { TfToken("leather", TfToken::Immortal), TfToken("PxrMayaLeather", TfToken::Immortal) },
    { TfToken("luminance", TfToken::Immortal), TfToken("PxrMayaLuminance", TfToken::Immortal) },
    { TfToken("marble", TfToken::Immortal), TfToken("PxrMayaMarble", TfToken::Immortal) },
    { TfToken("mountain", TfToken::Immortal), TfToken("PxrMayaMountain", TfToken::Immortal) },
    { TfToken("multiplyDivide", TfToken::Immortal), TfToken("PxrMultiplyDivide", TfToken::Immortal) },
    { TfToken("noise", TfToken::Immortal), TfToken("PxrMayaNoise", TfToken::Immortal) },
    { TfToken("place2dTexture", TfToken::Immortal), TfToken("PxrMayaPlacement2d", TfToken::Immortal) },
    { TfToken("place3dTexture", TfToken::Immortal), TfToken("PxrMayaPlacement3d", TfToken::Immortal) },
    { TfToken("plusMinusAverage", TfToken::Immortal), TfToken("PxrMayaPlusMinusAverage", TfToken::Immortal) },
    { TfToken("ramp", TfToken::Immortal), TfToken("PxrMayaRamp", TfToken::Immortal) },
    { TfToken("remapColor", TfToken::Immortal), TfToken("PxrMayaRemapColor", TfToken::Immortal) },
    { TfToken("remapHsv", TfToken::Immortal), TfToken("PxrMayaRemapHsv", TfToken::Immortal) },
    { TfToken("remapValue", TfToken::Immortal), TfToken("PxrMayaRemapValue", TfToken::Immortal) },
    { TfToken("reverse", TfToken::Immortal), TfToken("PxrMayaReverse", TfToken::Immortal) },
    { TfToken("rgbToHsv", TfToken::Immortal), TfToken("PxrMayaRgbToHsv", TfToken::Immortal) },
    { TfToken("rock", TfToken::Immortal), TfToken("PxrMayaRock", TfToken::Immortal) },
    { TfToken("setRange", TfToken::Immortal), TfToken("PxrMayaSetRange", TfToken::Immortal) },
    { TfToken("snow", TfToken::Immortal), TfToken("PxrMayaSnow", TfToken::Immortal) },
    { TfToken("solidFractal", TfToken::Immortal), TfToken("PxrMayaSolidFractal", TfToken::Immortal) },
    { TfToken("stucco", TfToken::Immortal), TfToken("PxrMayaStucco", TfToken::Immortal) },
    { TfToken("uvChooser", TfToken::Immortal), TfToken("PxrMayaUVChooser", TfToken::Immortal) },
    { TfToken("volumeFog", TfToken::Immortal), TfToken("PxrMayaVolumeFog", TfToken::Immortal) },
    { TfToken("volumeNoise", TfToken::Immortal), TfToken("PxrMayaVolumeNoise", TfToken::Immortal) },
    { TfToken("wood", TfToken::Immortal), TfToken("PxrMayaWood", TfToken::Immortal) }
};


PXR_NAMESPACE_CLOSE_SCOPE
