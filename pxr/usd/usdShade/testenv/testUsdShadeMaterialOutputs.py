#!/pxrpythonsubst                                                                   
#                                                                                   
# Copyright 2018 Pixar                                                              
#                                                                                   
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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

        # Create a few more outputs to exercise the multi-renderContext
        # terminal output getters.

        # A deeply namespaced "surface" output.
        mat.CreateSurfaceOutput(
            renderContext=Sdf.Path.namespaceDelimiter.join(
                ['deep', 'context']))

        # A deeply namespaced, float3-typed "displacement" output.
        mat.CreateOutput(
            Sdf.Path.namespaceDelimiter.join(
                ['deep', 'context', 'float', UsdShade.Tokens.displacement]),
            Sdf.ValueTypeNames.Float3)

        # A color-typed "volume" output.
        mat.CreateOutput(
            Sdf.Path.namespaceDelimiter.join(
                ['colorRenderContext', UsdShade.Tokens.volume]),
            Sdf.ValueTypeNames.Color3d)

        # Arbitrary outputs that the getters should *not* return.
        mat.CreateOutput('bogusOutput', Sdf.ValueTypeNames.Token)
        mat.CreateOutput(
            Sdf.Path.namespaceDelimiter.join(['bogusContext', 'randomOutput']),
            Sdf.ValueTypeNames.Token)

        outputBaseNames = [o.GetBaseName() for o in mat.GetSurfaceOutputs()]
        self.assertEqual(outputBaseNames,
            ['surface', 'deep:context:surface', 'ri:surface'])

        outputBaseNames = [o.GetBaseName() for o in mat.GetDisplacementOutputs()]
        self.assertEqual(outputBaseNames,
            ['displacement', 'deep:context:float:displacement', 'ri:displacement'])

        outputBaseNames = [o.GetBaseName() for o in mat.GetVolumeOutputs()]
        self.assertEqual(outputBaseNames,
            ['volume', 'colorRenderContext:volume', 'ri:volume'])


if __name__ == "__main__":
    unittest.main()
