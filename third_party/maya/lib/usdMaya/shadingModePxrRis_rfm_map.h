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

PXR_NAMESPACE_OPEN_SCOPE

static const std::vector<std::pair<std::string, std::string> > _RFM_RISNODE_TABLE = boost::assign::list_of
    ( std::make_pair("blendColors", "PxrMayaBlendColors") )
    ( std::make_pair("bulge", "PxrMayaBulge") )
    ( std::make_pair("bump2d", "PxrMayaBump2d") )
    ( std::make_pair("bump3d", "PxrMayaBump3d") )
    ( std::make_pair("brownian", "PxrMayaBrownian") )
    ( std::make_pair("checker", "PxrMayaChecker") )
    ( std::make_pair("clamp", "PxrMayaClamp") )
    ( std::make_pair("cloth", "PxrMayaCloth") )
    ( std::make_pair("cloud", "PxrMayaCloud") )
    ( std::make_pair("condition", "PxrCondition") )
    ( std::make_pair("contrast", "PxrMayaContrast") )
    ( std::make_pair("crater", "PxrMayaCrater") )
    ( std::make_pair("file", "PxrMayaFile") )
    ( std::make_pair("fluidShape", "PxrMayaFluidShape") )
    ( std::make_pair("fractal", "PxrMayaFractal") )
    ( std::make_pair("gammaCorrect", "PxrMayaGammaCorrect") )
    ( std::make_pair("granite", "PxrMayaGranite") )
    ( std::make_pair("grid", "PxrMayaGrid") )
    ( std::make_pair("hairSystem", "PxrMayaHair") )
    ( std::make_pair("hsvToRgb", "PxrMayaHsvToRgb") )
    ( std::make_pair("imagePlane", "PxrMayaImagePlane") )
    ( std::make_pair("layeredTexture", "PxrMayaLayeredTexture") )
    ( std::make_pair("leather", "PxrMayaLeather") )
    ( std::make_pair("luminance", "PxrMayaLuminance") )
    ( std::make_pair("marble", "PxrMayaMarble") )
    ( std::make_pair("mountain", "PxrMayaMountain") )
    ( std::make_pair("multiplyDivide", "PxrMultiplyDivide") )
    ( std::make_pair("noise", "PxrMayaNoise") )
    ( std::make_pair("place2dTexture", "PxrMayaPlacement2d") )
    ( std::make_pair("place3dTexture", "PxrMayaPlacement3d") )
    ( std::make_pair("plusMinusAverage", "PxrMayaPlusMinusAverage") )
    ( std::make_pair("ramp", "PxrMayaRamp") )
    ( std::make_pair("remapColor", "PxrMayaRemapColor") )
    ( std::make_pair("remapHsv", "PxrMayaRemapHsv") )
    ( std::make_pair("remapValue", "PxrMayaRemapValue") )
    ( std::make_pair("reverse", "PxrMayaReverse") )
    ( std::make_pair("rgbToHsv", "PxrMayaRgbToHsv") )
    ( std::make_pair("rock", "PxrMayaRock") )
    ( std::make_pair("setRange", "PxrMayaSetRange") )
    ( std::make_pair("snow", "PxrMayaSnow") )
    ( std::make_pair("solidFractal", "PxrMayaSolidFractal") )
    ( std::make_pair("stucco", "PxrMayaStucco") )
    ( std::make_pair("uvChooser", "PxrMayaUVChooser") )
    ( std::make_pair("volumeFog", "PxrMayaVolumeFog") )
    ( std::make_pair("volumeNoise", "PxrMayaVolumeNoise") )
    ( std::make_pair("wood", "PxrMayaWood") );

PXR_NAMESPACE_CLOSE_SCOPE
