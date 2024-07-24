#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest, shutil
from pxr import Sdf, Usd

class TestUsdCrateRelocates(unittest.TestCase):

   
    def test_ReadWriteRelocates(self):
        """Tests that reading and writing of relocates in layer metadata works
           as expected in crate files."""
        
        # Expected layer relocates field in the file with relocates.
        expectedRelocates = [
            (Sdf.Path('/World/Char/Rig/Body/Anim'), 
                Sdf.Path('/World/Char/Anim/Body')),
            (Sdf.Path('/World/Char/Rig/Head/Anim'), 
                Sdf.Path('/World/Char/Anim/Head'))
        ]

        # Open a crate file and verify the layer has no relocates.
        def _OpenCrateLayerAndVerifyNoRelocates(crateFileName):
            # A newly created crate file that has never had relocates added will
            # be verion 0.9
            self.assertEqual(
                Usd.CrateInfo.Open(crateFileName).GetFileVersion(), 
                '0.9.0')
            
            # The layer will have no relocates in its metadata
            layer = Sdf.Layer.FindOrOpen(crateFileName)
            self.assertTrue(layer)
            self.assertFalse(layer.HasRelocates())
            self.assertEqual(layer.relocates, [])
            return layer

        # Open a crate file and verify the layer does have the expected 
        # relocates.
        def _OpenCrateLayerAndVerifyHasRelocates(crateFileName):
            # A crate file that ever had relocates when it was created or saved
            # will have been upgraded to version 0.11
            self.assertEqual(
                Usd.CrateInfo.Open(crateFileName).GetFileVersion(), 
                '0.11.0')
            
            # The layer will have the expected relocates in its metadata.
            layer = Sdf.Layer.FindOrOpen(crateFileName)
            self.assertTrue(layer)
            self.assertTrue(layer.HasRelocates())
            self.assertEqual(layer.relocates, expectedRelocates)
            return layer

        # The test directory has two usdc files that are identical except one
        # has relocates and the other doesn't
        #
        # Open the no relocates file as a layer and verify it has no relocates.
        noRelocatesLayer = _OpenCrateLayerAndVerifyNoRelocates(
            'root_no_relocates.usdc')

        # Open the with relocates file as a layer and verify it has the expected
        # relocates.
        relocatesLayer = _OpenCrateLayerAndVerifyHasRelocates(
            'root_with_relocates.usdc')

        # Export both files to new file paths
        exportNoRelocatesCrateFilename = 'exported_root_no_relocates.usdc'
        self.assertTrue(noRelocatesLayer.Export(exportNoRelocatesCrateFilename))
        exportRelocatesCrateFilename = 'exported_root_with_relocates.usdc'
        self.assertTrue(relocatesLayer.Export(exportRelocatesCrateFilename))

        # The exported no relocates file still has no relocates and was not
        # updgraded. The exported "has relocates" file will have relocates.
        noRelocatesLayer = _OpenCrateLayerAndVerifyNoRelocates(
            exportNoRelocatesCrateFilename)
        _OpenCrateLayerAndVerifyHasRelocates(exportRelocatesCrateFilename)

        # Note that we use the exported no relocates layer for the following
        # editing tests so we don't alter the initial files for other future
        # test cases.

        # Add the expected relocates value to the layer with no relocates; it
        # will now have the expected relocates.
        noRelocatesLayer.relocates = expectedRelocates
        self.assertTrue(noRelocatesLayer.HasRelocates())
        self.assertEqual(noRelocatesLayer.relocates, expectedRelocates)

        # Export this edited layer to yet another new file. Open this new crate
        # will correctly have the added relocates.
        exportEditedNoRelocatesCrateFilename = 'exported_edited_root_no_relocates.usdc'
        self.assertTrue(noRelocatesLayer.Export(exportEditedNoRelocatesCrateFilename))
        _OpenCrateLayerAndVerifyHasRelocates(exportEditedNoRelocatesCrateFilename)

        # Now save the "no relocates" layer that we edited to add relocates and
        # close it. When we open it again, it will have the save relocates.
        noRelocatesLayer.Save()
        del noRelocatesLayer
        _OpenCrateLayerAndVerifyHasRelocates(exportNoRelocatesCrateFilename)

    def test_RelocatesOnStage(self):
        """Tests that relocates are composed correctly on a stage opened from a
           crate layer."""

        # Create a stage from the test file with no relocates
        noRelocatesLayer = Sdf.Layer.FindOrOpen('root_no_relocates.usdc')
        noRelocatesStage = Usd.Stage.Open(noRelocatesLayer)
        self.assertTrue(noRelocatesStage)

        # Create a stage from the test file that relocates
        relocatesLayer = Sdf.Layer.FindOrOpen('root_with_relocates.usdc')
        relocatesStage = Usd.Stage.Open(relocatesLayer)
        self.assertTrue(relocatesStage)

        # Verify the prims in the "no relocates" stage. Note that without 
        # relocates none of the "Anim" prims have been moved out of 
        # /World/Char/Rig
        nonRelocatesPrims = [p.GetPath() for p in noRelocatesStage.Traverse()]
        self.assertEqual(nonRelocatesPrims, 
            [Sdf.Path('/World'), 
             Sdf.Path('/World/Char'), 
             Sdf.Path('/World/Char/Rig'), 
             Sdf.Path('/World/Char/Rig/Body'), 
             Sdf.Path('/World/Char/Rig/Body/Geom'), 
             Sdf.Path('/World/Char/Rig/Body/Anim'), 
             Sdf.Path('/World/Char/Rig/Head'), 
             Sdf.Path('/World/Char/Rig/Head/Geom'), 
             Sdf.Path('/World/Char/Rig/Head/Anim'), 
             Sdf.Path('/World/Char/Anim')])

        # Verify the prims in the "has relocates" stage. Note that here the  
        # "Anim" prims have been moved out of /World/Char/Rig and renamed to 
        # "Head" and "Body" under /World/Char/Anim
        relocatedPrims = [p.GetPath() for p in relocatesStage.Traverse()]
        self.assertEqual(relocatedPrims, 
            [Sdf.Path('/World'), 
             Sdf.Path('/World/Char'), 
             Sdf.Path('/World/Char/Rig'), 
             Sdf.Path('/World/Char/Rig/Body'), 
             Sdf.Path('/World/Char/Rig/Body/Geom'), 
             Sdf.Path('/World/Char/Rig/Head'), 
             Sdf.Path('/World/Char/Rig/Head/Geom'), 
             Sdf.Path('/World/Char/Anim'), 
             Sdf.Path('/World/Char/Anim/Body'), 
             Sdf.Path('/World/Char/Anim/Head')])

        # Set the layer relocates in the "no relocates" to be the same as the
        # layer relocates in the "has relocates" layer.
        noRelocatesLayer.relocates = relocatesLayer.relocates 

        # Verify the prims in the "no relocates" stage have been relocated in
        # the same way as the relocates stage.
        prims = [p.GetPath() for p in noRelocatesStage.Traverse()]
        self.assertEqual(prims, relocatedPrims)

if __name__ == "__main__":
    unittest.main()
