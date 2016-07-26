#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
from pxr import Vt, Sdf, Usd, UsdGeom, UsdShade, UsdHydra

def Main():
    s = Usd.Stage.CreateInMemory()

    # Setup a quad mesh 
    gprim = s.DefinePrim("/Model/Geom/Mesh", "Mesh")
    gprim.GetAttribute("faceVertexCounts").Set([4])
    gprim.GetAttribute("faceVertexIndices").Set([0,1,3,2])
    gprim.GetAttribute("points").Set([(-2, -2, -2), (2, -2, -2), (-2, -2, 2), (2, -2, 2)])
    UsdGeom.Gprim(gprim).CreatePrimvar("somePrimvar",
            Sdf.ValueTypeNames.Color3fArray, UsdGeom.Tokens.constant).Set([1.0, 1.0, 1.0])
    UsdGeom.Gprim(gprim).CreatePrimvar("map1", Sdf.ValueTypeNames.Float2Array,
            UsdGeom.Tokens.faceVarying).Set([1.0, 1.0])

    lookPrim = UsdShade.Look.Define(s, "/Model/Looks/Pbs")
    lookPrim.Bind(gprim)

    # Add a Hydra shader and surface relationship from the look to the shader
    hydraShader = UsdHydra.Shader(UsdShade.Shader.Define(s, "/Model/Looks/Pbs/PolPbs"))
    hydraShader.CreateIdAttr("PolPbs_1")
    hydraLook = UsdHydra.LookAPI(lookPrim)
    hydraLook.CreateBxdfRel().SetTargets([hydraShader.GetPath()])

    uv = UsdHydra.Primvar(UsdShade.Shader.Define(s, "/Model/Looks/Pbs/UvPrimvar"))
    attr = uv.CreateVarnameAttr("map1")
    faceIndex = UsdHydra.Primvar(UsdShade.Shader.Define(s, "/Model/Looks/Pbs/FaceIndexPrimvar"))
    attr = uv.CreateVarnameAttr("ptexFaceIndex")
    faceOffset = UsdHydra.Primvar(UsdShade.Shader.Define(s, "/Model/Looks/Pbs/FaceOffsetPrimvar"))
    attr = uv.CreateVarnameAttr("ptexFaceOffset")

    tex = UsdHydra.UvTexture(UsdShade.Shader.Define(s, "/Model/Looks/Pbs/UvTex"))
    tex.CreateIdAttr("HwUvTexture_1")
    tex.CreateWrapSAttr(UsdHydra.Tokens.clamp)
    tex.CreateWrapTAttr(UsdHydra.Tokens.repeat)
    tex.CreateMinFilterAttr(UsdHydra.Tokens.linearMipmapLinear)
    tex.CreateMagFilterAttr(UsdHydra.Tokens.linear)
    tex.CreateTextureMemoryAttr(100.0)
    tex.CreateFilenameAttr("/foo/bar.jpg")
    tex.CreateParameter("outputs:color", Sdf.ValueTypeNames.Color3f)
    param = UsdShade.Parameter(tex.CreateUvAttr())
    param.ConnectToSource(uv, "result", False)

    texturePath = Sdf.AssetPath("/foo.tex")

    ptex = UsdHydra.PtexTexture(UsdShade.Shader.Define(s, "/Model/Looks/Pbs/Ptex"))
    ptex.CreateIdAttr("HwPtexTexture_1")
    ptex.CreateTextureMemoryAttr(100.0)
    ptex.CreateFilenameAttr("/foo/bar.ptex")
    ptex.CreateParameter("outputs:color", Sdf.ValueTypeNames.Color3f)
    param = UsdShade.Parameter(ptex.CreateFaceIndexAttr())
    param.ConnectToSource(faceIndex, "result", False)
    param = UsdShade.Parameter(ptex.CreateFaceOffsetAttr())
    param.ConnectToSource(faceOffset, "result", False)


    #
    hydraShader.CreateFilenameAttr(Sdf.AssetPath("PbsSurface.glslfx"))
    #
    param = hydraShader.CreateParameter("colorFromUv", Sdf.ValueTypeNames.Color3f)
    param.ConnectToSource(tex, "color", False)
    #
    param = hydraShader.CreateParameter("colorFromPtex1", Sdf.ValueTypeNames.Color3f)
    param.ConnectToSource(ptex, "color", False)
    #
    color = UsdHydra.Primvar(UsdShade.Shader.Define(s, "/Model/Looks/Pbs/ColorPrimvar"))
    attr = uv.CreateVarnameAttr("displayColor")
    param = hydraShader.CreateParameter("colorFromPrimvar", Sdf.ValueTypeNames.Color3f)
    param.ConnectToSource(color, "result", False)

    print hydraShader.GetConnectedPrimvar(param.GetAttr())
    print hydraShader.IsConnectedToPrimvar(param.GetAttr())


    print s.GetRootLayer().ExportToString()

if __name__ == '__main__':
    Main()

