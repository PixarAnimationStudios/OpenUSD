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

from pxr import Tf, Usd, UsdMtlx
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
        with open('NodeGraphs.usda', 'w') as f:
            print >>f, stage.GetRootLayer().ExportToString()

    def test_Looks(self):
        """
        Test general MaterialX look conversions.
        """

        stage = UsdMtlx._TestFile('Looks.mtlx')
        with open('Looks.usda', 'w') as f:
            print >>f, stage.GetRootLayer().ExportToString()

if __name__ == '__main__':
    unittest.main()
