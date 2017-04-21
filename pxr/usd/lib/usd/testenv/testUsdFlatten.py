#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

import os, shutil, sys, unittest
from pxr import Usd, Sdf, Tf

def _CompareFlattened(layerFile, primPath, timeRange):
    composed = Usd.Stage.Open(layerFile)
    flattened = composed.Flatten()
    flatComposed = Usd.Stage.Open(flattened)

    # attributes from non flattened scene
    prim = composed.GetPrimAtPath(primPath)
    
    # attributes from flattened scene
    flatPrim = flatComposed.GetPrimAtPath(primPath)

    for attr, flatAttr in zip(prim.GetAttributes(), flatPrim.GetAttributes()):
        for i in timeRange:
            assert attr.Get(i) == flatAttr.Get(i)

    return True

def _CompareMetadata(composed, flat, indent):
    # These fields will not match, so we explicitly exclude them during
    # comparison.
    # 
    # XXX: This list is not exhaustive, additional fields may need to be added
    # if the test inputs change.
    exclude = ("active", "inheritPaths", "payload", "references", "subLayers", 
               "subLayerOffsets", "variantSelection", "variantSetNames",
               "properties", "variantSetChildren", "primChildren", 
               "targetPaths")

    cdata = composed.GetAllMetadata()
    fdata = flat.GetAllMetadata()
    for cKey in cdata.keys():
        if cKey in exclude:
            continue

        print (" " * indent) + ":",cKey
        assert cKey in fdata, str(composed.GetPath()) + " : " + cKey
        assert composed.GetMetadata(cKey) == flat.GetMetadata(cKey), "GetMetadata -- " + str(composed.GetPath()) + " : " + cKey
        assert cdata[cKey] == fdata[cKey], str(composed.GetPath()) + " : " + cKey

class TestUsdFlatten(unittest.TestCase):
    def test_Flatten(self):
        composed = Usd.Stage.Open("root.usd")
        flatLayer = composed.Flatten()
        flat = Usd.Stage.Open(flatLayer)

        assert composed.GetPrimAtPath("/Foo").GetAttribute("size").Get(3.0) == 1.0
        assert flat.GetPrimAtPath("/Foo").GetAttribute("size").Get(3.0) == 1.0

        for pc in composed.Traverse():
            print pc.GetPath()

            # We elide deactivated prims, so skip the check.
            if not pc.IsActive():
                continue

            pf = flat.GetPrimAtPath(pc.GetPath())
            assert pf

            _CompareMetadata(pc, pf, 1)

            for attr in pf.GetAttributes():
                print "    Attr:" , attr.GetName()
                assert attr.IsDefined()

                attrc = pc.GetAttribute(attr.GetName())

                # Compare metadata.
                _CompareMetadata(attrc, attr, 10)

                # Compare time samples.
                ts_c = attrc.GetTimeSamples()
                ts_f = attr.GetTimeSamples()
                self.assertEqual(ts_f, ts_c)
                if len(ts_c):
                    print ((" "*12) + "["),
                for t in ts_c:
                    self.assertEqual(attrc.Get(t), attr.Get(t))
                    print ("%.1f," % t),
                if len(ts_c):
                    print "]"

                # Compare defaults.
                self.assertEqual(attrc.Get(), attr.Get())
                print " "*12 + 'default =', attr.Get()

            for rel in pf.GetRelationships():
                print "    Rel:" , rel.GetName()
                assert rel and rel.IsDefined()
                _CompareMetadata(pc.GetRelationship(rel.GetName()), rel, 10)

    def test_NoFallbacks(self):
        strLayer = Usd.Stage.CreateInMemory().ExportToString()
        assert "endFrame" not in strLayer

    def test_Export(self):
        # Verify that exporting to a .usd file produces the default usd file format.
        composed = Usd.Stage.Open("root.usd")
        assert composed.Export('TestExport.usd')

        newFileName = 'TestExport.' + Tf.GetEnvSetting('USD_DEFAULT_FILE_FORMAT')
        shutil.copyfile('TestExport.usd', newFileName)
        assert Sdf.Layer.FindOrOpen(newFileName)

        # Verify that exporting to a .usd file but specifying ASCII
        # via file format arguments produces an ASCII file.
        assert composed.Export('TestExport_ascii.usd', args={'format':'usda'})
        
        shutil.copyfile('TestExport_ascii.usd', 'TestExport_ascii.usda')
        assert Sdf.Layer.FindOrOpen('TestExport_ascii.usda')

        # Verify that exporting to a .usd file but specifying crate
        # via file format arguments produces a usd crate file.
        assert composed.Export('TestExport_crate.usd', args={'format':'usdc'})
        
        shutil.copyfile('TestExport_crate.usd', 'TestExport_crate.usdc')
        assert Sdf.Layer.FindOrOpen('TestExport_crate.usdc')

    def test_FlattenClips(self): 
        # This test verifies the behavior of flattening clips in 
        # two cases.

        # 1. When the clip range is uniform in its properties and prims.
        #    Meaning that it has the same topology across the scene.

        # 2. When the clips have hole's, i.e. time samples where a property
        #    is not authored at all.

        # verify that flattening a valid clip range works
        assert _CompareFlattened("clips/root.usd", 
                                 "/World/fx/Particles_Splash/points",
                                 range(101, 105))

        assert _CompareFlattened("hole_clips/root.usd", 
                                "/World/fx/Particles_Splash/points",
                                range(101, 105))

        # verify that flattening with a sparse topology works
        assert _CompareFlattened("sparse_topology_clips/root.usda",
                                "/World/fx/Particles_Splash/points",
                                range(101, 105))

    def test_FlattenBadMetadata(self):
        # Shouldn't fail with unknown fields.
        s = Usd.Stage.Open("badMetadata.usd")
        assert s
        assert s.Flatten()

    def test_FlattenAttributeWithUnregisteredType(self):
        # We should still be able to open a valid stage
        stageFile = 'badPropertyTypeNames.usd'
        stage = Usd.Stage.Open(stageFile)
        assert stage
        
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('myAttr')
        # Ensure we are actually trying to flatten an unknown type
        assert attr.GetTypeName().type == Tf.Type.Unknown

        # Ensure flatten completes
        flattened = stage.Flatten()
        assert flattened

        # Ensure the property has been omitted in the flattened result
        assert not flattened.GetPropertyAtPath('/main.myAttr')
        # Ensure the valid properties made it through
        assert flattened.GetPropertyAtPath('/main.validAttr1')
        assert flattened.GetPropertyAtPath('/main.validAttr2')

    def test_FlattenRelationshipTargets(self):
        basePath = 'relationshipTargets/'
        stageFile = basePath+'source.usda'

        stage = Usd.Stage.Open(stageFile)
        assert stage
        prim = stage.GetPrimAtPath('/bar')
        assert prim
        rel  = prim.GetRelationship('foo')
        assert rel
        self.assertEqual(rel.GetTargets(), 
                         [prim.GetMaster().GetChild('baz').GetPath()])

        resultFile = basePath+'result.usda'
        stage.Export(resultFile)

        resultStage = Usd.Stage.Open(resultFile)
        assert resultStage

        prim = resultStage.GetPrimAtPath('/bar')
        assert prim
        rel  = prim.GetRelationship('foo')
        assert rel
        self.assertEqual(rel.GetTargets(), 
                         [prim.GetMaster().GetChild('baz').GetPath()])

if __name__ == "__main__":
    unittest.main()
