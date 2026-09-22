#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf
import unittest

class TestSdfRelocates(unittest.TestCase):

    # Create a new layer with a single root prim /Root
    def _CreateTestLayer(self) :
        layer = Sdf.Layer.CreateAnonymous(".usda")
        prim = Sdf.PrimSpec(layer, 'Root', Sdf.SpecifierDef, 'Scope')
        return layer    

    # Test editing relocates in layer metadata. The relocates property on layer
    # is NOT a proxy object and is just a simple read/write property. Any edits
    # to layers relocates must be explicitly set from a full list value. Also
    # layer relocates are a list of relocation path pairs which is different 
    # from prim relocates which are path to path map type.
    def test_LayerRelocates(self):

        # Test editing relocates directly on the layer
        layer = self._CreateTestLayer()

        # Layer starts with no relocates
        self.assertEqual(layer.relocates, [])
        self.assertFalse(layer.HasRelocates())

        # Set relocates; use absolute paths since they'll be authored in layer
        # metadata.
        layer.relocates = [
             ("/Root/source2", "/Root/target2"),
             ("/Root/source1", "/Root/target1"),
             ("/Root/source3", "/Root/target3"),
             ("/Root/sourceToDelete", Sdf.Path())]
        self.assertTrue(layer.HasRelocates())
        self.assertEqual(layer.relocates, 
            [("/Root/source2", "/Root/target2"),
             ("/Root/source1", "/Root/target1"),
             ("/Root/source3", "/Root/target3"),
             ("/Root/sourceToDelete", Sdf.Path())])

        # Write layer; relocates are written as layer metadata when the layer 
        # is written.
        self.assertEqual(layer.ExportToString(), '''#usda 1.0
(
    relocates = {
        </Root/source2>: </Root/target2>, 
        </Root/source1>: </Root/target1>, 
        </Root/source3>: </Root/target3>, 
        </Root/sourceToDelete>: <>
    }
)

def Scope "Root"
{
}

''')

        # Delete relocate
        newRelocates = layer.relocates
        del newRelocates[0]
        layer.relocates = newRelocates
        self.assertTrue(layer.HasRelocates())
        self.assertFalse(("/Root/source2", "/Root/target2") in layer.relocates)
        self.assertEqual(layer.relocates, 
            [(Sdf.Path('/Root/source1'), Sdf.Path('/Root/target1')), 
             (Sdf.Path('/Root/source3'), Sdf.Path('/Root/target3')),
             (Sdf.Path('/Root/sourceToDelete'), Sdf.Path())])

        # Add a single new relocate
        newRelocates = layer.relocates
        newRelocates.append(("/Root/source4", "/Root/target4"))
        layer.relocates = newRelocates
        self.assertTrue(layer.HasRelocates())
        self.assertTrue(("/Root/source4", "/Root/target4") in layer.relocates)
        self.assertEqual(layer.relocates, 
            [(Sdf.Path('/Root/source1'), Sdf.Path('/Root/target1')), 
             (Sdf.Path('/Root/source3'), Sdf.Path('/Root/target3')),
             (Sdf.Path('/Root/sourceToDelete'), Sdf.Path()),
             (Sdf.Path('/Root/source4'), Sdf.Path('/Root/target4'))])

        # Overwrite an existing relocate
        newRelocates = layer.relocates
        newRelocates[0] = ("/Root/source1", "/Root/targetFoo")
        layer.relocates = newRelocates
        self.assertTrue(layer.HasRelocates())
        self.assertFalse(("/Root/source1", "/Root/target1") in layer.relocates)
        self.assertTrue(("/Root/source1", "/Root/targetFoo") in layer.relocates)
        self.assertEqual(layer.relocates, 
            [(Sdf.Path('/Root/source1'), Sdf.Path('/Root/targetFoo')), 
             (Sdf.Path('/Root/source3'), Sdf.Path('/Root/target3')),
             (Sdf.Path('/Root/sourceToDelete'), Sdf.Path()),
             (Sdf.Path('/Root/source4'), Sdf.Path('/Root/target4'))])

        # Clear all relocates
        layer.ClearRelocates()
        self.assertFalse(layer.HasRelocates())
        self.assertEqual(layer.relocates, [])

    # Test that layer relocates can be set and retrieved via Get/Set/HasInfo 
    # API on the pseudoroot
    def test_LayerRelocatesMetadata(self):
        layer = self._CreateTestLayer()
        pseudoRoot = layer.pseudoRoot

        # New layer has no relocates metadata to start
        self.assertFalse(pseudoRoot.HasInfo(Sdf.Layer.LayerRelocatesKey))
        self.assertEqual(pseudoRoot.GetInfo(Sdf.Layer.LayerRelocatesKey), [])
        self.assertEqual(layer.relocates, [])

        # Set the relocates field on the pseudo-root.
        pseudoRoot.SetInfo(Sdf.Layer.LayerRelocatesKey, 
            [("/Root/source1", "/Root/target1"), 
             ("/Root/source2", "/Root/target2")])
        
        # Relocates field exists and is set to the path values.
        self.assertTrue(pseudoRoot.HasInfo(Sdf.Layer.LayerRelocatesKey))
        self.assertEqual(pseudoRoot.GetInfo(Sdf.Layer.LayerRelocatesKey),
            [(Sdf.Path("/Root/source1"), Sdf.Path("/Root/target1")), 
             (Sdf.Path("/Root/source2"), Sdf.Path("/Root/target2"))])
        
        # Relocates on the layer evaluates to the metadata value.
        self.assertEqual(layer.relocates,
            [(Sdf.Path("/Root/source1"), Sdf.Path("/Root/target1")), 
             (Sdf.Path("/Root/source2"), Sdf.Path("/Root/target2"))])

    # Test that an explicitly authored empty layer relocates gets written out as
    # empty data in the layer
    def test_EmptyRelocatesRoundtrip(self):
        # Create and new layer with an explicit empty relocates in the layer 
        # metadata.
        layerContents = '''#usda 1.0
(
    relocates = {
    }
)

def Scope "Root"
{
}

'''
        layer = Sdf.Layer.CreateAnonymous(".usda")
        layer.ImportFromString(layerContents)

        # The pseudoroot will have a layer relocates field and the returned
        # value for that field will be an empty list
        self.assertTrue(layer.pseudoRoot.HasInfo('layerRelocates'))
        self.assertEqual(layer.pseudoRoot.GetInfo('layerRelocates'), [])

        # Writing the layer will produce the same contents with the explicit
        # empty layer relocates field.
        self.assertEqual(layer.ExportToString(), layerContents)

        # Clear the relocates on the layer via the layer API.
        layer.ClearRelocates()

        # This clears the relocates field on the pseudo-root (it no longer has
        # that field). The returned value for the field will still be the 
        # fallback empty list though.
        self.assertFalse(layer.pseudoRoot.HasInfo('layerRelocates'))
        self.assertEqual(layer.pseudoRoot.GetInfo('layerRelocates'), [])

        # Writing the layer will no longer write the relocates as the field is
        # no longer present.
        self.assertEqual(layer.ExportToString(), 
'''#usda 1.0

def Scope "Root"
{
}

''')

        # Now explicitly set the relocates field on the pseudo-root to an empty
        # list using the SetInfo API
        layer.pseudoRoot.SetInfo(Sdf.Layer.LayerRelocatesKey, [])

        # The relocates field will exist again and be the empty dictionary.
        self.assertTrue(layer.pseudoRoot.HasInfo('layerRelocates'))
        self.assertEqual(layer.pseudoRoot.GetInfo('layerRelocates'), [])

        # Writing the layer again will produce the original contents of the 
        # layer with the explicitly empty relocates.
        self.assertEqual(layer.ExportToString(), layerContents)

if __name__ == '__main__':
    unittest.main()
