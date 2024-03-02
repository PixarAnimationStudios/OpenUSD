#!/pxrpythonsubst
#
# Copyright 2024 Pixar
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

from pxr import Sdf
import unittest

class TestSdfRelocates(unittest.TestCase):

    # Create a new layer with a single root prim /Root
    def _CreateTestLayer(self) :
        layer = Sdf.Layer.CreateAnonymous("test")
        prim = Sdf.PrimSpec(layer, 'Root', Sdf.SpecifierDef, 'Scope')
        return layer    

    # Runs the test set for a specific set of inputs from the test cases.
    def _RunRelocatesTest(self,
            layer, primOrLayer, inputRelocates, expectedWriteLayerContents):
        from collections import OrderedDict

        # Prim starts with no relocates
        self.assertEqual(len(primOrLayer.relocates), 0)
        self.assertEqual(dict(primOrLayer.relocates), {})

        # Set relocates
        primOrLayer.relocates = inputRelocates
        self.assertEqual(len(primOrLayer.relocates), 3)
        self.assertEqual(dict(primOrLayer.relocates), 
            {Sdf.Path('/Root/source1'): Sdf.Path('/Root/target1'), 
            Sdf.Path('/Root/source2'): Sdf.Path('/Root/target2'), 
            Sdf.Path('/Root/source3'): Sdf.Path('/Root/target3')})

        # Delete relocate
        del primOrLayer.relocates["/Root/source2"]
        self.assertEqual(len(primOrLayer.relocates), 2)
        self.assertFalse(("/Root/source2", "/Root/target2") in primOrLayer.relocates.items())
        self.assertEqual(dict(primOrLayer.relocates), 
            {Sdf.Path('/Root/source1'): Sdf.Path('/Root/target1'), 
            Sdf.Path('/Root/source3'): Sdf.Path('/Root/target3')})

        # Set a single new relocate
        primOrLayer.relocates["/Root/source4"] = '/Root/target4'
        self.assertEqual(len(primOrLayer.relocates), 3)
        self.assertTrue(("/Root/source4", "/Root/target4") in primOrLayer.relocates.items())
        self.assertEqual(dict(primOrLayer.relocates), 
            {Sdf.Path('/Root/source1'): Sdf.Path('/Root/target1'), 
            Sdf.Path('/Root/source3'): Sdf.Path('/Root/target3'),
            Sdf.Path('/Root/source4'): Sdf.Path('/Root/target4')})

        # Overwrite an existing relocate
        primOrLayer.relocates["/Root/source1"] = '/Root/targetFoo'
        self.assertEqual(len(primOrLayer.relocates), 3)
        self.assertFalse(("/Root/source1", "/Root/target1") in primOrLayer.relocates.items())
        self.assertTrue(("/Root/source1", "/Root/targetFoo") in primOrLayer.relocates.items())
        self.assertEqual(dict(primOrLayer.relocates), 
            {Sdf.Path('/Root/source1'): Sdf.Path('/Root/targetFoo'), 
            Sdf.Path('/Root/source3'): Sdf.Path('/Root/target3'),
            Sdf.Path('/Root/source4'): Sdf.Path('/Root/target4')})

        # Clear all relocates
        primOrLayer.relocates.clear()
        self.assertEqual(len(primOrLayer.relocates), 0)
        self.assertEqual(dict(primOrLayer.relocates), {})

        # Set relocates using OrderedDict
        primOrLayer.relocates = OrderedDict(inputRelocates)
        self.assertEqual(len(primOrLayer.relocates), 3)
        self.assertEqual(dict(primOrLayer.relocates), 
            {Sdf.Path('/Root/source1'): Sdf.Path('/Root/target1'), 
            Sdf.Path('/Root/source2'): Sdf.Path('/Root/target2'), 
            Sdf.Path('/Root/source3'): Sdf.Path('/Root/target3')})

        # Write layer
        self.assertEqual(layer.ExportToString(), expectedWriteLayerContents)

    # Test editing relocates on a real prim
    def test_PrimRelocates(self):
        # Use relative paths to the /Root prim.
        inputRelocates = { 
             "source1" : "target1", 
             "source2" : "target2", 
             "source3" : "target3"}

        # Relocates are written as prim metadata when the layer is written.
        expectedWriteLayerContents = '''#sdf 1.4.32

def Scope "Root" (
    relocates = {
        <source1>: <target1>, 
        <source2>: <target2>, 
        <source3>: <target3>
    }
)
{
}

'''

        # Test editing relocates on the /Root prim
        layer = self._CreateTestLayer()
        self._RunRelocatesTest(
            layer,
            layer.GetPrimAtPath("/Root"), 
            inputRelocates,
            expectedWriteLayerContents)

    # Test editing relocates in layer metadata
    def test_LayerRelocates(self):
        # Use absolute paths since they'll be authored in layer metadata.
        inputRelocates = { 
            "/Root/source1" : "/Root/target1", 
            "/Root/source2" : "/Root/target2", 
            "/Root/source3" : "/Root/target3"}

        # Relocates are written as layer metadata when the layer is written.
        expectedWriteLayerContents = '''#sdf 1.4.32
(
    relocates = {
        </Root/source1>: </Root/target1>, 
        </Root/source2>: </Root/target2>, 
        </Root/source3>: </Root/target3>
    }
)

def Scope "Root"
{
}

'''

        # Test editing relocates directly on the layer
        layer = self._CreateTestLayer()
        self._RunRelocatesTest(
            layer,
            layer,
            inputRelocates,
            expectedWriteLayerContents)

        # Test editing relocates on the layer's pseudo-root which should behave
        # exactly the same as editing using the layer interface.
        layer = self._CreateTestLayer()
        self._RunRelocatesTest(
            layer,
            layer.pseudoRoot,
            inputRelocates,
            expectedWriteLayerContents)

    # Test that layer relocates can be set and retrieved via Get/Set/HasInfo 
    # API on the pseudoroot
    def test_RelocatesMetadata(self):
        layer = self._CreateTestLayer()
        pseudoRoot = layer.pseudoRoot

        # New layer has no relocates metadata to start
        self.assertFalse(pseudoRoot.HasInfo(Sdf.Layer.RelocatesKey))
        self.assertEqual(pseudoRoot.GetInfo(Sdf.Layer.RelocatesKey), {})
        self.assertEqual(dict(layer.relocates), {})

        # Set the relocates field on the pseudo-root.
        pseudoRoot.SetInfo(Sdf.Layer.RelocatesKey, 
            {"/Root/source1" : "/Root/target1", 
             "/Root/source2" : "/Root/target2"})
        
        # Relocates field exists and is set to the path values.
        self.assertTrue(pseudoRoot.HasInfo(Sdf.Layer.RelocatesKey))
        self.assertEqual(pseudoRoot.GetInfo(Sdf.Layer.RelocatesKey),
            {Sdf.Path("/Root/source1") : Sdf.Path("/Root/target1"), 
             Sdf.Path("/Root/source2") : Sdf.Path("/Root/target2")})
        
        # Relocates proxy on the layer evaluates to the metadata value.
        self.assertEqual(dict(layer.relocates),
            {Sdf.Path("/Root/source1") : Sdf.Path("/Root/target1"), 
             Sdf.Path("/Root/source2") : Sdf.Path("/Root/target2")})

    # Test that an explicitly authored empty relocates gets written out as empty
    def test_EmptyRelocatesRoundtrip(self):
        # Create and new layer with an explicit empty relocates in the layer 
        # metadata.
        layerContents = '''#sdf 1.4.32
(
    relocates = {
    }
)

def Scope "Root"
{
}

'''
        layer = Sdf.Layer.CreateAnonymous("test")
        layer.ImportFromString(layerContents)

        # The pseudoroot will have a relocates field and the returned value
        # for that field will be an empty dictionary
        self.assertTrue(layer.pseudoRoot.HasInfo('relocates'))
        self.assertEqual(layer.pseudoRoot.GetInfo('relocates'), {})

        # Writing the layer will produce the same contents with the explicit
        # empty relocates field.
        self.assertEqual(layer.ExportToString(), layerContents)

        # Set the relocates on the layer to an empty dictionary via the proxy
        # API.
        layer.relocates = {}

        # This clears the relocates field on the pseudo-root (it no longer has
        # that field). The returned value for the field will still be the 
        # fallback empty dictionary though.
        self.assertFalse(layer.pseudoRoot.HasInfo('relocates'))
        self.assertEqual(layer.pseudoRoot.GetInfo('relocates'), {})

        # Writing the layer will no longer write the relocates as the field is
        # no longer present.
        self.assertEqual(layer.ExportToString(), 
'''#sdf 1.4.32

def Scope "Root"
{
}

''')

        # Now explicitly set the relocates field on the pseudo-root to an empty
        # dictionary using the SetInfo API
        layer.pseudoRoot.SetInfo(Sdf.PrimSpec.RelocatesKey , {})

        # The relocates field will exist again and be the empty dictionary.
        self.assertTrue(layer.pseudoRoot.HasInfo('relocates'))
        self.assertEqual(layer.pseudoRoot.GetInfo('relocates'), {})

        # Writing the layer again will produce the original contents of the 
        # layer with the explicitly empty relocates.
        self.assertEqual(layer.ExportToString(), layerContents)

if __name__ == '__main__':
    unittest.main()
