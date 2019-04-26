#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

import unittest
from pxr import Sdf, Usd

class TestUsdCrateForPayloadLists(unittest.TestCase):
    # Verify that a payload list has a specific set of explicit items
    def _VerifyPayloadList(self, payloadList, explicitItems=None):
        if explicitItems is None:
            # None means that we expect the payload list to be completely 
            # empty and not explicit which is different than the list being 
            # explicitly set to being an empty list.
            self.assertFalse(payloadList.isExplicit)
            self.assertEqual(list(payloadList.explicitItems), [])
        else:
            # Otherwise the payload list should be explicit and its 
            # explicitItems must match the list passed in.
            self.assertTrue(payloadList.isExplicit)
            self.assertEqual(list(payloadList.explicitItems), explicitItems)

        # All the other list op properties are expected to be empty.
        for attrName in ['addedItems', 'appendedItems', 'deletedItems', 
                         'orderedItems', 'prependedItems']:
            self.assertEqual(list(getattr(payloadList, attrName)), [])

    # Verify the opened layer has the prims we expect and these prim specs
    # have the payloads we expect.
    def _VerifyLayerPrims(self, layer):
        payloadRef1 = layer.GetPrimAtPath('/PayloadRef1')
        payloadRef2 = layer.GetPrimAtPath('/PayloadRef2')
        payloadRefNone = layer.GetPrimAtPath('/PayloadRefNone')
        payloadNoOpinion = layer.GetPrimAtPath('/PayloadNoOpinion')
        self.assertTrue(payloadRef1)
        self.assertTrue(payloadRef2)
        self.assertTrue(payloadRefNone)
        self.assertTrue(payloadNoOpinion)

        # Single explicit payload for these two prims            
        self._VerifyPayloadList(
            payloadRef1.payloadList, 
            explicitItems=[Sdf.Payload('Payload.usda', Sdf.Path('/Parent'))])
        self._VerifyPayloadList(
            payloadRef2.payloadList, 
            explicitItems=[Sdf.Payload('Payload2.usda', Sdf.Path('/Parent'))])

        # Payload is explicitly set to be None which is equates to an 
        # explicitly empty payload list
        self._VerifyPayloadList(
            payloadRefNone.payloadList, explicitItems=[])

        # No payload opinion on this prim which is equates to a non-explicit
        # empty payload list.
        self._VerifyPayloadList(
            payloadNoOpinion.payloadList, explicitItems=None)

    # Verifies a crate file is the prior 0.7.0 crate version
    def _VerifyPriorCrateVersion(self, filename):
        info = Usd.CrateInfo.Open(filename)
        self.assertEqual(info.GetFileVersion(), '0.7.0')
        self.assertNotEqual(info.GetFileVersion(), info.GetSoftwareVersion())

    # Verifies a crate file is the current software crate version
    def _VerifyCurrentCrateVersion(self, filename):
        info = Usd.CrateInfo.Open(filename)
        self.assertNotEqual(info.GetFileVersion(), '0.7.0')
        self.assertEqual(info.GetFileVersion(), info.GetSoftwareVersion())

    def test_ExportPayloadCrate(self):
        """Test exporting crate file from a layer with payloads"""
        usdaFilename = 'singlePayload.usda'
        singlePayloadCrateFilename = 'exportSinglePayload.usdc'
        listPayloadCrateFilename = 'exportListPayload.usdc'

        # Load the usda layer file containing single payload variations and 
        # verify its contents.
        usdaLayer = Sdf.Layer.FindOrOpen(usdaFilename)
        self._VerifyLayerPrims(usdaLayer)

        # Export the this layer to a usdc file and verify that it is exported
        # using the prior 0.7.0 crate file version.
        self.assertTrue(usdaLayer.Export(singlePayloadCrateFilename))
        self._VerifyPriorCrateVersion(singlePayloadCrateFilename)

        # Open the crate layer and verify that it has the same prims and 
        # payloads.
        crateLayer = Sdf.Layer.FindOrOpen(singlePayloadCrateFilename)
        self._VerifyLayerPrims(crateLayer)

        # Update the "none payloads" prim to have an explicit list of two 
        # payloads
        usdaPayloadRefNone = usdaLayer.GetPrimAtPath('/PayloadRefNone')
        usdaPayloadRefNone.payloadList.explicitItems = [
            Sdf.Payload('PayloadNew1.usda', Sdf.Path('/Parent')),
            Sdf.Payload('PayloadNew2.usda', Sdf.Path('/Parent'))]                        

        # Export layer to a new crate file and verify that it uses the current
        # crate version as this can not be represented in old versions.
        self.assertTrue(usdaLayer.Export(listPayloadCrateFilename))
        self._VerifyCurrentCrateVersion(listPayloadCrateFilename)
                                                
        # Similar to the generic _VerifyLayerPrims but instead verifies that
        # '/PayloadRefNone' has two payloads instead.
        def _VerifyExportedLayerPrims(layer):
            payloadRef1 = layer.GetPrimAtPath('/PayloadRef1')
            payloadRef2 = layer.GetPrimAtPath('/PayloadRef2')
            payloadRefNone = layer.GetPrimAtPath('/PayloadRefNone')
            payloadNoOpinion = layer.GetPrimAtPath('/PayloadNoOpinion')
            self.assertTrue(payloadRef1)
            self.assertTrue(payloadRef2)
            self.assertTrue(payloadRefNone)
            self.assertTrue(payloadNoOpinion)

            # Single explicit payload for these two prims            
            self._VerifyPayloadList(
                payloadRef1.payloadList, 
                explicitItems=[Sdf.Payload('Payload.usda', Sdf.Path('/Parent'))])
            self._VerifyPayloadList(
                payloadRef2.payloadList, 
                explicitItems=[Sdf.Payload('Payload2.usda', Sdf.Path('/Parent'))])

            # Payload was updated to be a two payload list
            self._VerifyPayloadList(
                payloadRefNone.payloadList, explicitItems=[
                    Sdf.Payload('PayloadNew1.usda', Sdf.Path('/Parent')),
                    Sdf.Payload('PayloadNew2.usda', Sdf.Path('/Parent'))])

            # No payload opinion on this prim which is equates to a non-explicit
            # empty payload list.
            self._VerifyPayloadList(
                payloadNoOpinion.payloadList, explicitItems=None)

        # Open the exported crate file and verify it matches the expected prims
        # and payloads
        listCrateLayer = Sdf.Layer.FindOrOpen(listPayloadCrateFilename)
        _VerifyExportedLayerPrims(listCrateLayer)
                
    def test_ExportPayloadCrateWithInternalPayload(self):
        """Test exporting crate file from a layer with payloads after adding
           an internal payload"""
        usdaFilename = 'singlePayload.usda'
        internalPayloadCrateFilename = "exportInternalPayload.usdc"

        # Load the usda layer file containing single payload variations and 
        # verify its contents.
        usdaLayer = Sdf.Layer.FindOrOpen(usdaFilename)
        self._VerifyLayerPrims(usdaLayer)

        # Update the no opinion payload to have a single internal payload 
        # (empty assetpath).
        usdaPayloadNoOpinion = usdaLayer.GetPrimAtPath('/PayloadNoOpinion')
        usdaPayloadNoOpinion.payloadList.explicitItems = [Sdf.Payload("","/PayloadRef1")]

        # Export layer to a new crate file and verify that it uses the current
        # crate version as this can not be represented in old versions.
        self.assertTrue(usdaLayer.Export(internalPayloadCrateFilename))
        self._VerifyCurrentCrateVersion(internalPayloadCrateFilename)

        # Similar to the generic _VerifyLayerPrims but instead verifies that
        # '/PayloadRefNoOpinion' has a single internal payload instead.
        def _VerifyExportedLayerPrims(layer):
            payloadRef1 = layer.GetPrimAtPath('/PayloadRef1')
            payloadRef2 = layer.GetPrimAtPath('/PayloadRef2')
            payloadRefNone = layer.GetPrimAtPath('/PayloadRefNone')
            payloadNoOpinion = layer.GetPrimAtPath('/PayloadNoOpinion')
            self.assertTrue(payloadRef1)
            self.assertTrue(payloadRef2)
            self.assertTrue(payloadRefNone)
            self.assertTrue(payloadNoOpinion)

            # Single explicit payload for these two prims            
            self._VerifyPayloadList(
                payloadRef1.payloadList, 
                explicitItems=[Sdf.Payload('Payload.usda', Sdf.Path('/Parent'))])
            self._VerifyPayloadList(
                payloadRef2.payloadList, 
                explicitItems=[Sdf.Payload('Payload2.usda', Sdf.Path('/Parent'))])

            # No payload
            self._VerifyPayloadList(
                payloadRefNone.payloadList, explicitItems=[])

            # Payload was updated to have an internal payload
            self._VerifyPayloadList(
                payloadNoOpinion.payloadList, 
                explicitItems=[Sdf.Payload('', Sdf.Path('/PayloadRef1'))])

        # Open the exported crate file and verify it matches the expected prims
        # and payloads
        crateLayer = Sdf.Layer.FindOrOpen(internalPayloadCrateFilename)
        _VerifyExportedLayerPrims(crateLayer)
                                                                        
    def test_ExportPayloadCrateWithLayerOffset(self):
        """Test exporting crate file from a layer with payloads after adding
           an layer offset to a payload"""
        usdaFilename = 'singlePayload.usda'
        exportCrateFilename = "exportLayerOffsetPayload.usdc"

        # Load the usda layer file containing single payload variations and 
        # verify its contents.
        usdaLayer = Sdf.Layer.FindOrOpen(usdaFilename)
        self._VerifyLayerPrims(usdaLayer)

        # Update the ref1 payload to have a non-empty layer offset
        usdaPayloadRef1 = usdaLayer.GetPrimAtPath('/PayloadRef1')
        usdaPayloadRef1.payloadList.explicitItems = [
            Sdf.Payload('Payload.usda', Sdf.Path('/Parent'), Sdf.LayerOffset(12.0, 1.0))]

        # Export layer to a new crate file and verify that it uses the current
        # crate version as this can not be represented in old versions.
        self.assertTrue(usdaLayer.Export(exportCrateFilename))
        self._VerifyCurrentCrateVersion(exportCrateFilename)

        # Similar to the generic _VerifyLayerPrims but instead verifies that
        # '/PayloadRef1' has a layer offset.
        def _VerifyExportedLayerPrims(layer):
            payloadRef1 = layer.GetPrimAtPath('/PayloadRef1')
            payloadRef2 = layer.GetPrimAtPath('/PayloadRef2')
            payloadRefNone = layer.GetPrimAtPath('/PayloadRefNone')
            payloadNoOpinion = layer.GetPrimAtPath('/PayloadNoOpinion')
            self.assertTrue(payloadRef1)
            self.assertTrue(payloadRef2)
            self.assertTrue(payloadRefNone)
            self.assertTrue(payloadNoOpinion)

            # Single explicit payload for these two prims            
            self._VerifyPayloadList(
                payloadRef1.payloadList, 
                explicitItems=[Sdf.Payload('Payload.usda', Sdf.Path('/Parent'), 
                                           Sdf.LayerOffset(12.0, 1.0))])
            self._VerifyPayloadList(
                payloadRef2.payloadList, 
                explicitItems=[Sdf.Payload('Payload2.usda', Sdf.Path('/Parent'))])

            # No payload
            self._VerifyPayloadList(
                payloadRefNone.payloadList, explicitItems=[])

            # Payload was updated to have an internal payload
            self._VerifyPayloadList(
                payloadNoOpinion.payloadList, explicitItems=None)

        # Open the exported crate file and verify it matches the expected prims
        # and payloads
        crateLayer = Sdf.Layer.FindOrOpen(exportCrateFilename)
        _VerifyExportedLayerPrims(crateLayer)

    def test_ReadAndSaveCrateFileVersion07(self):
        """Test reading and saving crate files from the prior crate version"""
        filename = 'crate07SinglePayload.usdc'

        # Assert the crate file we're going to open is an older version '0.7.0',
        # before payload list op support was added.
        self._VerifyPriorCrateVersion(filename)

        # Open the crate file layer and verify it matches the single payload
        # prims.
        layer = Sdf.Layer.FindOrOpen(filename)
        self._VerifyLayerPrims(layer)

        # Change the prim spec with the empty payload to have an explicit single
        # payload list and save the layer.
        payloadRefNone = layer.GetPrimAtPath('/PayloadRefNone')
        payloadRefNone.payloadList.explicitItems = [
            Sdf.Payload('PayloadNew.usda', Sdf.Path('/Parent'))]
        self.assertTrue(layer.Save())

        # Assert that the crate file is still using the same file version after
        # save as a single explicit payload can be expressed in the old version.
        self._VerifyPriorCrateVersion(filename)

        # Change the prim spec with no payload opinion to do a list op add of
        # a single payload and save the layer.
        payloadNoOpinion = layer.GetPrimAtPath('/PayloadNoOpinion')
        payloadNoOpinion.payloadList.addedItems = [
            Sdf.Payload('PayloadNew.usda', Sdf.Path('/Parent'))]
        self.assertTrue(layer.Save())

        # Assert that the crate file has now been upgraded to the software 
        # version as we can't represent adds without list ops.
        self._VerifyCurrentCrateVersion(filename)

if __name__ == "__main__":
    unittest.main()
