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

from __future__ import print_function
from pxr import Ar, Tf, Sdf, Usd, UsdMtlx, UsdShade
import unittest

def _EmptyLayer():
    stage = Usd.Stage.CreateInMemory()
    return stage.GetRootLayer().ExportToString()

class TestFileFormat(unittest.TestCase):
    def test_EmptyFile(self):
        """
        Verify that an empty MaterialX document fails.
        """
        with self.assertRaises(Tf.ErrorException) as e:
            UsdMtlx._TestString('')

    def test_MissingFile(self):
        """
        Verify that a missing MaterialX file fails.
        """
        with self.assertRaises(Tf.ErrorException) as e:
            UsdMtlx._TestFile('non-existent-file.xml')

    def test_BadMagic(self):
        """
        Verify that a MaterialX file with a bad XML header fails.
        """
        with self.assertRaises(Tf.ErrorException) as e:
            UsdMtlx._TestString('''<?not_xml version="1.0" ?>''')

    def test_EmptyXMLDocument(self):
        """
        Verify that a MaterialX file with only an XML header fails.
        """
        with self.assertRaises(Tf.ErrorException) as e:
            UsdMtlx._TestString('''<?xml version="1.0" ?>''')

    def test_MissingMaterialXDocument(self):
        """
        Verify that a MaterialX file without a materialx element is okay.
        """
        stage = UsdMtlx._TestString(
            '''<?xml version="1.0" ?>
               <not_materialx version="1.35">
               </not_materialx>
            ''')
        self.assertEqual(stage.GetRootLayer().ExportToString(),
                         _EmptyLayer())

    def test_EmptyMaterialXDocument(self):
        """
        Verify that a file with an empty a materialx element is okay.
        """
        stage = UsdMtlx._TestString(
            '''<?xml version="1.0" ?>
               <materialx version="1.35">
               </materialx>
            ''')
        self.assertEqual(stage.GetRootLayer().ExportToString(),
                         _EmptyLayer())

    def test_DuplicateName(self):
        """
        Verify that a MaterialX file with duplicate element names fails.
        """
        with self.assertRaises(Tf.ErrorException) as e:
            UsdMtlx._TestString(
                '''<?xml version="1.0" ?>
                   <materialx version="1.35">
                       <typedef name="type1">
                       <typedef name="type1">
                   </materialx>
                ''')

    def test_Cycle(self):
        """
        Verify that a MaterialX file with an inherits cycle fails.
        """
        with self.assertRaises(Tf.ErrorException) as e:
            UsdMtlx._TestString(
                '''<?xml version="1.0" ?>
                   <materialx version="1.35">
                       <nodedef name="n1" type="float" node="test" inherit="n2">
                       <nodedef name="n2" type="float" node="test" inherit="n1">
                   </materialx>
                ''')

    def test_NodeGraphs(self):
        """
        Test general MaterialX node graph conversions.
        """

        stage = UsdMtlx._TestFile('NodeGraphs.mtlx', nodeGraphs=True)
        stage.GetRootLayer().Export('NodeGraphs.usda')

    def test_MultiBindInputs(self):
        """
        Test MaterialX conversion with mutliple bind inputs.
        """

        stage = UsdMtlx._TestFile('MultiBindInputs.mtlx')
        
        # Get the node graph and make sure there are exactly 3 inputs
        nodeGraph = UsdShade.NodeGraph.Get(stage,
                        Sdf.Path('/MaterialX/Materials/layered/ND_layerShader'))
        inputs = nodeGraph.GetInputs()
        self.assertEqual(len(inputs), 3)

        # Make sure each input is connected as expected
        inputToSource = {
            'weight_1':
            '/MaterialX/Materials/layered/NodeGraphs/layered_layer1_gradient',
            'weight_2':
            '/MaterialX/Materials/layered/NodeGraphs/layered_layer2_gradient',
            'weight_3':
            '/MaterialX/Materials/layered/NodeGraphs/layered_layer3_gradient'
        }
        for inputName, source in inputToSource.items():
            input = nodeGraph.GetInput(inputName)
            self.assertEqual(input.HasConnectedSource(), True)
            self.assertEqual(
                input.GetConnectedSources()[0][0].source.GetPath(), source)

    def test_MultiOutputNodes(self):
        """
        Test MaterialX nodes with multiple outputs
        """

        stage = UsdMtlx._TestFile('MultiOutputNode.mtlx')
        testInfo = [
            ('/MaterialX/Materials/test_m/test_ng/specular',
            'artistic_ior', 'extinction'),
            ('/MaterialX/Materials/test_m/test_ng/ior',
            'artistic_ior', 'ior')
        ]

        for path, connNodeName, connectionName in testInfo:
            node = UsdShade.Shader.Get(stage, path)
            conn = node.GetInput('in').GetConnectedSources()[0][0]
            self.assertEqual(conn.source.GetPath().name, connNodeName)
            self.assertEqual(conn.sourceName, connectionName)
        
    def test_nodesWithoutNodegraphs(self):
        """
        Test MaterialX material with nodes not contained in a nodegraph and no
        explicit outputs
        """
    
        stage = UsdMtlx._TestFile('GraphlessNodes.mtlx')
        stage.GetRootLayer().Export('GraphlessNodes.usda')

    def test_NodegraphsWithInputs(self):
        """
        Test that inputs on nodegraphs are found and connected when used 
        inside that nodegraph
        """
    
        stage = UsdMtlx._TestFile('NodeGraphInputs.mtlx')

        path = '/MaterialX/Materials/test_material/test_nodegraph/mult1'
        node = UsdShade.Shader.Get(stage, path)
        conn = node.GetInput('in2').GetConnectedSources()[0][0]
        self.assertEqual(conn.source.GetPath().name, 'test_nodegraph')
        self.assertEqual(conn.sourceName, 'scale')
        

    def test_Looks(self):
        """
        Test general MaterialX look conversions.
        """

        stage = UsdMtlx._TestFile('Looks.mtlx')
        stage.GetRootLayer().Export('Looks.usda')

    def test_StdlibShaderRefs(self):
        """
        Test that we can use a shader nodedef from the MaterialX stdlib.
        """

        stage = UsdMtlx._TestFile('usd_preview_surface_gold.mtlx')
        # check stage contents
        mprim = stage.GetPrimAtPath("/MaterialX/Materials/USD_Gold")
        self.assertTrue(mprim)
        material = UsdShade.Material(mprim)
        self.assertTrue(material)
        input = material.GetInput("specularColor")
        self.assertTrue(input)
        self.assertEqual(input.GetFullName(),"inputs:specularColor")
    
    def test_customNodeDefs(self):
        """
        Test that custom nodedefs are flattend out and replaced with 
        their associated nodegraph
        """
        stage = UsdMtlx._TestFile('CustomNodeDef.mtlx')
        stage.GetRootLayer().Export('CustomNodeDef.usda')

    @unittest.skipIf(not hasattr(Ar.Resolver, "CreateIdentifier"),
                     "Requires Ar 2.0")
    def test_XInclude(self):
        """
        Verify documents referenced via XInclude statements are read
        properly.
        """
        stage = UsdMtlx._TestFile('include/Include.mtlx')
        stage.GetRootLayer().Export('Include.usda')

        stage = UsdMtlx._TestFile('include/Include.usdz[Include.mtlx]')
        stage.GetRootLayer().Export('Include_From_Usdz.usda')

    @unittest.skipIf(not hasattr(Ar.Resolver, "CreateIdentifier"),
                     "Requires Ar 2.0")
    def test_EmbedInUSDZ(self):
        """
        Verify that a MaterialX file can be read from within a .usdz file.
        """

        stage = UsdMtlx._TestFile(
            'usd_preview_surface_gold.usdz[usd_preview_surface_gold.mtlx]')
        stage.GetRootLayer().Export('usd_preview_surface_gold.usda')

    def test_Capabilities(self):
        self.assertTrue(Sdf.FileFormat.FormatSupportsReading('.mtlx'))
        self.assertFalse(Sdf.FileFormat.FormatSupportsWriting('.mtlx'))
        self.assertFalse(Sdf.FileFormat.FormatSupportsEditing('.mtlx'))

    def test_ExpandFilePrefix(self):
        """
        Test active file prefix defined by the fileprefix attribute
        in a parent tag.
        """
        stage = UsdMtlx._TestFile('ExpandFilePrefix.mtlx')

        for nodeName, expectedResult in [
            ('image_base', 'outer_scope/textures/base.tif'),
            ('image_spec', 'inner_scope/textures/spec.tif')
        ]:
            primPath = f'/MaterialX/Materials/test_material/test_nodegraph/{nodeName}'
            shader = UsdShade.Shader.Get(stage, primPath)
            self.assertTrue(shader)

            fileInput = shader.GetInput('file')
            self.assertTrue(fileInput)

            actualResult = fileInput.Get().path
            self.assertEqual(actualResult, expectedResult)

if __name__ == '__main__':
    unittest.main()
