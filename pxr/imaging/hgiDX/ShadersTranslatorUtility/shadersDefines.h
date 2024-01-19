
//
// Copyright 2023 Pixar
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


#include <map>
#include <string>

#pragma once

namespace shadersAnalysisDefines
{

   const std::map<std::string, std::string> shadersLibsFolders{
      {"glf", "\\lib\\usd\\glf\\resources\\shaders"},
      {"hdx", "\\lib\\usd\\hdx\\resources\\shaders"},
      {"hdSt", "\\lib\\usd\\hdSt\\resources\\shaders"}
   };

   const std::string strGlLibExt = ".glslfx";
   const std::string strHlLibExt = ".hlslfx";

#define MD5LEN 16

   const std::map<std::string, std::string> knownHashes = {
      {"glf/pcfShader.glslfx","{3147599F-4BD7-097D-C16D-B1DBB5300572}"},
      {"glf/simpleLighting.glslfx","{A551DD3E-4A42-15B1-03AF-EDD7EF4D33D6}"},

      {"hdSt/basisCurves.glslfx","{20B5CF81-1D32-0A02-9BAC-897A0A2B200E}"},
      {"hdSt/compute.glslfx","{FA7D3F25-F791-786B-9F22-D9820D18E110}"},
      {"hdSt/domeLight.glslfx","{59FC285B-1D99-3B8F-C235-3CDCE61ED04A}"},
      {"hdSt/edgeId.glslfx","{E734F70B-062A-D036-A345-C9DC3399AC10}"},
      {"hdSt/fallbackLighting.glslfx","{7CFE4D6C-DACC-27D9-6D7F-F379C6F2C5B7}"},
      {"hdSt/fallbackLightingShader.glslfx","{1EC92347-A15B-4E13-55B4-0D095D34D073}"},
      {"hdSt/fallbackMaterialNetwork.glslfx","{25D3565B-B290-4279-E3D4-B04E1AE4169F}"},
      {"hdSt/fallbackVolume.glslfx","{25585F4C-A69F-39DE-35C8-71E420982D1A}"},
      {"hdSt/frustumCull.glslfx","{D3429380-22E9-8FFA-F556-0EE264BFAB61}"},
      {"hdSt/imageShader.glslfx","{0BD5A5ED-460D-AF88-0F5B-55000E8F64E3}"},
      {"hdSt/instancing.glslfx","{55BD2BE5-06DC-3BE4-AC0E-2DDEDBE72D51}"},
      {"hdSt/invalidMaterialNetwork.glslfx","{C51D5679-7702-13CB-CBFC-A7A9088FDD32}"},
      {"hdSt/mesh.glslfx","{89B508F8-9488-90ED-B42D-16F48FDCC506}"},
      {"hdSt/meshFaceCull.glslfx","{FD95CEFA-9B28-2520-71DB-7F37B2A5FF87}"},
      {"hdSt/meshNormal.glslfx","{04275C5B-1FE4-03B2-DD47-0C1F81A0D150}"},
      {"hdSt/meshWire.glslfx","{F1D71CA6-B38A-CA3E-3FDA-4B1CA6DA28B3}"},
      {"hdSt/pointId.glslfx","{6FFA4F95-ABA6-471E-37A1-1DE4811CC38E}"},
      {"hdSt/points.glslfx","{6E8B5176-79C7-9F39-266A-6184BBA8A1DA}"},
      {"hdSt/ptexTexture.glslfx","{E3B6EA7A-143C-840D-A0D4-C1D7BDA456D6}"},
      {"hdSt/renderPass.glslfx","{B3626278-9CAF-4CF8-AB0F-5DB55C793556}"},
      {"hdSt/renderPassShader.glslfx","{F78B6FB4-011F-F0A1-3A39-CB6C2976030F}"},
      {"hdSt/secondaryGraphics.glslfx","{A9BFE8C3-3514-828E-67E2-26972E345F2A}"},
      {"hdSt/simpleLightingShader.glslfx","{A7394C3F-9A3B-83DC-8C2A-199AEF06257A}"},
      {"hdSt/surfaceHelpers.glslfx","{360B4857-C01B-D2C2-389C-A5E998A543DE}"},
      {"hdSt/terminals.glslfx","{D5B5FCA7-D1D6-DF0A-BBCA-1B1FDADE345F}"},
      {"hdSt/text.glslfx","{529DF29C-568B-2FBA-96F4-538A369B5E87}"},
      {"hdSt/visibility.glslfx","{9F7F6F7B-D389-A26F-13E2-9A86F88F986C}"},
      {"hdSt/volume.glslfx","{697CA111-91EF-BA94-4106-A9376CA6B75B}"},

      {"hdx/boundingBox.glslfx","{5B77738A-6205-618E-A0BC-845697A299FA}"},
      {"hdx/colorChannel.glslfx","{A1924071-0656-63F0-9B7F-40DF2D5FEF62}"},
      {"hdx/colorCorrection.glslfx","{4A9AE01A-1991-800A-A581-E7E3447C2140}"},
      {"hdx/fullscreen.glslfx","{FDE82A5D-46F1-AB13-69DD-07977564E633}"},
      {"hdx/oitResolveImageShader.glslfx","{2D6BBDA1-B873-2BAB-F2C2-2C3256650E06}"},
      {"hdx/outline.glslfx","{A49A4051-A288-28B9-1C77-9646FD21C4E4}"},
      {"hdx/renderPass.glslfx","{BBD9E565-6A50-9975-7E48-DE6F798AB604}"},
      {"hdx/renderPassColorAndSelectionShader.glslfx","{32A1106D-FC79-883C-E2BB-814EB4D4EFE8}"},
      {"hdx/renderPassColorShader.glslfx","{977D9EAD-DCA7-E7B3-DC85-629496D092AC}"},
      {"hdx/renderPassColorWithOccludedSelectionShader.glslfx","{892F24E0-3106-4185-D75B-0C5CE59E3465}"},
      {"hdx/renderPassIdShader.glslfx","{885B0A88-C246-EC80-9DFA-EC98D1F0DD6A}"},
      {"hdx/renderPassOitOpaqueShader.glslfx","{29938C56-FE3F-B165-A601-696DD93B6520}"},
      {"hdx/renderPassOitShader.glslfx","{A7F36826-43FE-880E-CF81-DE7486FC3A7B}"},
      {"hdx/renderPassOitVolumeShader.glslfx","{8B4102A3-5CB3-2F83-62F3-A88F709BAAD8}"},
      {"hdx/renderPassPickingShader.glslfx","{728AFA79-1B11-09B6-60FF-5D7C5DC54A36}"},
      {"hdx/renderPassShadowShader.glslfx","{C65B16D4-1C74-12F8-4CD1-DCCDC6605266}"},
      {"hdx/selection.glslfx","{9AA4CE13-A19D-F9DB-98E3-F63772448EDA}"},
      {"hdx/skydome.glslfx","{C42B9D73-FC01-8052-D6E1-7DA880F6D31D}"},
      {"hdx/visualize.glslfx","{022E3F87-A9AA-07BC-627F-773661E3E3EC}"},
   };
};