
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

      {"hdx/boundingBox.glslfx","{5B77738A-6205-618E-A0BC-845697A299FA}"},
      {"hdx/colorChannel.glslfx","{A1924071-0656-63F0-9B7F-40DF2D5FEF62}"},
      {"hdx/colorCorrection.glslfx","{4A9AE01A-1991-800A-A581-E7E3447C2140}"},
      {"hdx/fullscreen.glslfx","{FE868AC1-1C1A-355D-3775-172CFC58DC36}"},
      {"hdx/oitResolveImageShader.glslfx","{2D6BBDA1-B873-2BAB-F2C2-2C3256650E06}"},
      {"hdx/outline.glslfx","{A49A4051-A288-28B9-1C77-9646FD21C4E4}"},
      {"hdx/renderPass.glslfx","{051161B2-FF14-3EC1-EA54-0ABFFFF8A380}"},
      {"hdx/renderPassColorAndSelectionShader.glslfx","{4958AEDE-6D30-AA7F-A454-78E99626826A}"},
      {"hdx/renderPassColorShader.glslfx","{9152475B-C891-3326-13C3-2E1667DCAE16}"},
      {"hdx/renderPassColorWithOccludedSelectionShader.glslfx","{D72C7ED9-D0BB-3223-F2B6-C16EDE1F68F3}"},
      {"hdx/renderPassIdShader.glslfx","{3105EEFD-EAF7-D8A1-CD1C-C0D5206E6B58}"},
      {"hdx/renderPassOitOpaqueShader.glslfx","{2C550999-788F-E7CD-C3C3-433138FE00A8}"},
      {"hdx/renderPassOitShader.glslfx","{5AEA40ED-7772-57DB-8C89-6027DC59E2A4}"},
      {"hdx/renderPassOitVolumeShader.glslfx","{21A344E5-687D-2132-4673-CB96207AABB2}"},
      {"hdx/renderPassPickingShader.glslfx","{86D3A8C1-4092-6201-9B48-D22817210F20}"},
      {"hdx/renderPassShadowShader.glslfx","{D71E61A2-8BE0-0E0F-736C-C091E88A32D6}"},
      {"hdx/selection.glslfx","{68670663-C03B-F5FD-97CF-CF13F185E52E}"},
      {"hdx/skydome.glslfx","{2DAA9164-2C83-FBC1-5118-4CB79E1D1F52}"},
      {"hdx/visualize.glslfx","{022E3F87-A9AA-07BC-627F-773661E3E3EC}"},

      {"hdSt/basisCurves.glslfx","{F9B1A7F7-AD07-D7CF-9B1B-72405C61624B}"},
      {"hdSt/compute.glslfx","{FA7D3F25-F791-786B-9F22-D9820D18E110}"},
      {"hdSt/domeLight.glslfx","{6170DE6E-31EF-ED6B-0383-576721736C0D}"},
      {"hdSt/edgeId.glslfx","{E734F70B-062A-D036-A345-C9DC3399AC10}"},
      {"hdSt/fallbackLighting.glslfx","{B4F82D78-FF71-731B-83BD-2E5853531BE9}"},
      {"hdSt/fallbackLightingShader.glslfx","{1EC92347-A15B-4E13-55B4-0D095D34D073}"},
      {"hdSt/fallbackMaterialNetwork.glslfx","{B9465B36-B46A-E36D-E128-8EE921482541}"},
      {"hdSt/fallbackVolume.glslfx","{25585F4C-A69F-39DE-35C8-71E420982D1A}"},
      {"hdSt/frustumCull.glslfx","{D3429380-22E9-8FFA-F556-0EE264BFAB61}"},
      {"hdSt/imageShader.glslfx","{0BD5A5ED-460D-AF88-0F5B-55000E8F64E3}"},
      {"hdSt/instancing.glslfx","{8962B6D7-741D-E5B8-F65B-49D8149A1AFD}"},
      {"hdSt/mesh.glslfx","{A9599FCC-65A9-4E43-49B2-07A35207D0DE}"},
      {"hdSt/meshFaceCull.glslfx","{FD95CEFA-9B28-2520-71DB-7F37B2A5FF87}"},
      {"hdSt/meshNormal.glslfx","{04275C5B-1FE4-03B2-DD47-0C1F81A0D150}"},
      {"hdSt/meshWire.glslfx","{EB61B921-0BAB-BDFC-DC53-1DAA5CD06093}"},
      {"hdSt/pointId.glslfx","{6FFA4F95-ABA6-471E-37A1-1DE4811CC38E}"},
      {"hdSt/points.glslfx","{061D1FAB-1E17-08C5-7C45-1FE1D9ABB5FF}"},
      {"hdSt/ptexTexture.glslfx","{E3B6EA7A-143C-840D-A0D4-C1D7BDA456D6}"},
      {"hdSt/renderPass.glslfx","{5590ABAE-7805-1817-EC82-455A81A599EB}"},
      {"hdSt/renderPassShader.glslfx","{D55F0A44-E3B4-3144-AD65-4BD56F3C6F85}"},
      {"hdSt/secondaryGraphics.glslfx","{A9BFE8C3-3514-828E-67E2-26972E345F2A}"},
      {"hdSt/simpleLightingShader.glslfx","{A7394C3F-9A3B-83DC-8C2A-199AEF06257A}"},
      {"hdSt/terminals.glslfx","{D5B5FCA7-D1D6-DF0A-BBCA-1B1FDADE345F}"},
      {"hdSt/text.glslfx","{529DF29C-568B-2FBA-96F4-538A369B5E87}"},
      {"hdSt/visibility.glslfx","{9F7F6F7B-D389-A26F-13E2-9A86F88F986C}"},
      {"hdSt/volume.glslfx","{697CA111-91EF-BA94-4106-A9376CA6B75B}"},

   };
};