#!/pxrpythonsubst                                                                   
#                                                                                   
# Copyright 2018 Pixar                                                              
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

from pxr import Sdf, Usd, UsdShade
import os, unittest

class TestUsdShadeMaterialOutputs(unittest.TestCase):
    def test_MaterialOutputs(self):
        stage = Usd.Stage.CreateInMemory()
        mat = UsdShade.Material.Define(stage, "/Material")
        
        univSurfaceOutput = mat.GetSurfaceOutput(
            renderContext=UsdShade.Tokens.universalRenderContext)
        univDisplacementOutput = mat.GetDisplacementOutput()
        univVolumeOutput = mat.GetVolumeOutput()

        self.assertTrue(univSurfaceOutput)
        self.assertTrue(univDisplacementOutput)
        self.assertTrue(univVolumeOutput)

        riSurfOutput = mat.GetSurfaceOutput(renderContext="ri")
        riDispOutput = mat.GetDisplacementOutput(renderContext="ri")
        riVolOutput = mat.GetVolumeOutput(renderContext="ri")

        self.assertFalse(riSurfOutput)
        self.assertFalse(riDispOutput)
        self.assertFalse(riVolOutput)

        riSurfOutput = mat.CreateSurfaceOutput(renderContext='ri')
        riDispOutput = mat.CreateDisplacementOutput(renderContext='ri')
        riVolOutput = mat.CreateVolumeOutput(renderContext='ri')

        self.assertTrue(riSurfOutput)
        self.assertTrue(riDispOutput)
        self.assertTrue(riVolOutput)

        surfShader = UsdShade.Shader.Define(stage, "/Material/Surf")
        surfOutput = surfShader.CreateOutput("out", Sdf.ValueTypeNames.Token)
        
        dispShader = UsdShade.Shader.Define(stage, "/Material/Disp")
        dispOutput = dispShader.CreateOutput("out", Sdf.ValueTypeNames.Token)
        
        volShader = UsdShade.Shader.Define(stage, "/Material/Vol")
        volOutput = volShader.CreateOutput("out", Sdf.ValueTypeNames.Token)

        ng = UsdShade.NodeGraph.Define(stage, "/Material/NodeGraph")
        ngSurfOutput= ng.CreateOutput("ngSurfOutput", 
                Sdf.ValueTypeNames.Token)
        ngDispOutput= ng.CreateOutput("ngDispOutput", 
                Sdf.ValueTypeNames.Token)
        ngVolOutput= ng.CreateOutput("ngVolOutput", 
                Sdf.ValueTypeNames.Token)

        ngSurfShader = UsdShade.Shader.Define(stage, "/Material/NodeGraph/NGSurf")
        ngSurfShaderOutput = ngSurfShader.CreateOutput("out", Sdf.ValueTypeNames.Token)
        ngDispShader = UsdShade.Shader.Define(stage, "/Material/NodeGraph/NGDisp")
        ngDispShaderOutput = ngDispShader.CreateOutput("out", Sdf.ValueTypeNames.Token)
        ngVolShader = UsdShade.Shader.Define(stage, "/Material/NodeGraph/NGVol")
        ngVolShaderOutput = ngVolShader.CreateOutput("out", Sdf.ValueTypeNames.Token)

        # Author universal output connections.
        univSurfaceOutput.ConnectToSource(surfOutput)
        univDisplacementOutput.ConnectToSource(dispOutput)
        univVolumeOutput.ConnectToSource(volOutput)

        univSurfSource = mat.ComputeSurfaceSource()
        univDispSource = mat.ComputeDisplacementSource()
        univVolSource = mat.ComputeVolumeSource()

        riSurfSource = mat.ComputeSurfaceSource(renderContext='ri')
        riDispSource = mat.ComputeDisplacementSource(renderContext='ri')
        riVolSource = mat.ComputeVolumeSource(renderContext='ri')

        self.assertEqual(univSurfSource[0].GetPath(), surfShader.GetPath())
        self.assertEqual(univDispSource[0].GetPath(), dispShader.GetPath())
        self.assertEqual(univVolSource[0].GetPath(), volShader.GetPath())
        
        self.assertEqual(riSurfSource[0].GetPath(), surfShader.GetPath())
        self.assertEqual(riDispSource[0].GetPath(), dispShader.GetPath())
        self.assertEqual(riVolSource[0].GetPath(), volShader.GetPath())        

        ngSurfOutput.ConnectToSource(ngSurfShaderOutput)
        ngDispOutput.ConnectToSource(ngDispShaderOutput)
        ngVolOutput.ConnectToSource(ngVolShaderOutput)

        # Author render-context specific output connections.
        riSurfOutput.ConnectToSource(ngSurfOutput)
        riDispOutput.ConnectToSource(ngDispOutput)
        # connect to shader inside the node-graph directly.
        riVolOutput.ConnectToSource(ngVolShaderOutput)

        univSurfSource = mat.ComputeSurfaceSource()
        univDispSource = mat.ComputeDisplacementSource()
        univVolSource = mat.ComputeVolumeSource()

        self.assertEqual(univSurfSource[0].GetPath(), surfShader.GetPath())
        self.assertEqual(univDispSource[0].GetPath(), dispShader.GetPath())
        self.assertEqual(univVolSource[0].GetPath(), volShader.GetPath())

        riSurfSource = mat.ComputeSurfaceSource(renderContext='ri')
        riDispSource = mat.ComputeDisplacementSource(renderContext='ri')
        riVolSource = mat.ComputeVolumeSource(renderContext='ri')

        self.assertEqual(riSurfSource[0].GetPath(), ngSurfShader.GetPath())
        self.assertEqual(riDispSource[0].GetPath(), ngDispShader.GetPath())
        self.assertEqual(riVolSource[0].GetPath(), ngVolShader.GetPath())

if __name__ == "__main__":
    unittest.main()
