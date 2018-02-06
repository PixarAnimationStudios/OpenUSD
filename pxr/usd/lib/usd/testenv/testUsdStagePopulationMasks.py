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

import unittest
from pxr import Usd, Sdf, Tf

class TestUsdStagePopulationMask(unittest.TestCase):
    def test_Basic(self):
        pm = Usd.StagePopulationMask.All()
        assert not pm.IsEmpty()
        assert pm.Includes('/any/path')
        assert pm.GetIncludedChildNames('/') == (True, [])

        pm = Usd.StagePopulationMask()
        assert pm.IsEmpty()
        assert not pm.Includes('/any/path')
        assert pm.GetIncludedChildNames('/') == (False, [])

        pm2 = Usd.StagePopulationMask().Add('/foo').Add('/bar')
        assert not pm.Includes(pm2)
        assert pm2.Includes(pm)
        assert pm.GetUnion(pm2) == pm2
        assert Usd.StagePopulationMask.Union(pm, pm2) == pm2
        
        assert pm2.GetIncludedChildNames('/') == (True, ['bar', 'foo'])
        assert pm2.GetIncludedChildNames('/foo') == (True, [])
        assert pm2.GetIncludedChildNames('/bar') == (True, [])
        assert pm2.GetIncludedChildNames('/baz') == (False, [])

        pm.Add('/World/anim/chars/CharGroup')
        assert pm.GetPaths() == ['/World/anim/chars/CharGroup']
        assert not pm.IsEmpty()
        pm.Add('/World/anim/chars/CharGroup/child')
        assert pm.GetPaths() == ['/World/anim/chars/CharGroup']
        pm.Add('/World/anim/chars/OtherCharGroup')
        assert pm.GetPaths() == ['/World/anim/chars/CharGroup',
                                 '/World/anim/chars/OtherCharGroup']
        pm.Add('/World/sets/arch/Building')
        assert pm.GetPaths() == ['/World/anim/chars/CharGroup',
                                 '/World/anim/chars/OtherCharGroup',
                                 '/World/sets/arch/Building']

        pm2 = Usd.StagePopulationMask()
        assert pm2 != pm
        pm2.Add('/World/anim/chars/CharGroup')
        assert pm2 != pm
        pm2.Add('/World/sets/arch/Building')
        pm2.Add('/World/anim/chars/OtherCharGroup')
        pm2.Add('/World/anim/chars/CharGroup/child')
        assert pm2 == pm

        assert pm2.GetUnion(pm) == pm
        assert pm2.GetUnion(pm) == pm2

        pm2 = Usd.StagePopulationMask()
        assert Usd.StagePopulationMask.Union(pm, pm2) == pm
        assert Usd.StagePopulationMask.Union(pm, pm2) != pm2

        assert pm.Includes('/World')
        assert not pm.IncludesSubtree('/World')
        assert pm.Includes('/World/anim')
        assert not pm.IncludesSubtree('/World/anim')
        assert pm.Includes('/World/anim/chars/CharGroup')
        assert pm.IncludesSubtree('/World/anim/chars/CharGroup')
        assert pm.Includes('/World/anim/chars/CharGroup/child')
        assert pm.IncludesSubtree('/World/anim/chars/CharGroup/child')

        pm = Usd.StagePopulationMask().Add('/world/anim')
        pm2 = pm.GetUnion('/world')
        assert pm2.GetPaths() == ['/world']

        pm = Usd.StagePopulationMask(['/A', '/AA', '/B/C', '/U'])
        pm2 = Usd.StagePopulationMask(['/A/X', '/B', '/Q'])
        assert (Usd.StagePopulationMask.Union(pm, pm2) ==
                Usd.StagePopulationMask(['/A', '/AA', '/B', '/Q', '/U']))
        assert (Usd.StagePopulationMask.Intersection(pm, pm2) ==
                Usd.StagePopulationMask(['/A/X', '/B/C']))

        pm = Usd.StagePopulationMask(['/A/B', '/A/C', '/A/D/E', '/A/D/F', '/B'])
        assert pm.GetIncludedChildNames('/') == (True, ['A', 'B'])
        assert pm.GetIncludedChildNames('/A') == (True, ['B', 'C', 'D'])
        assert pm.GetIncludedChildNames('/A/B') == (True, [])
        assert pm.GetIncludedChildNames('/A/C') == (True, [])
        assert pm.GetIncludedChildNames('/A/D') == (True, ['E', 'F'])
        assert pm.GetIncludedChildNames('/A/D/E') == (True, [])
        assert pm.GetIncludedChildNames('/A/D/F') == (True, [])
        assert pm.GetIncludedChildNames('/B') == (True, [])
        assert pm.GetIncludedChildNames('/C') == (False, [])

        # Errors.
        with self.assertRaises(Tf.ErrorException):
            Usd.StagePopulationMask(['relativePath/is/no/good'])
        with self.assertRaises(Tf.ErrorException):
            Usd.StagePopulationMask().Add('relativePath/is/no/good')
        with self.assertRaises(Tf.ErrorException):
            Usd.StagePopulationMask(['/property/path/is/no.good'])
        with self.assertRaises(Tf.ErrorException):
            Usd.StagePopulationMask().Add('/property/path/is/no.good')
        with self.assertRaises(Tf.ErrorException):
            Usd.StagePopulationMask(['/variant/selection/path/is{no=good}'])
        with self.assertRaises(Tf.ErrorException):
            Usd.StagePopulationMask().Add('/variant/selection/path/is{no=good}')

    def test_Stages(self):
        unmasked = Usd.Stage.CreateInMemory()
        unmasked.DefinePrim('/World/anim/chars/DoryGroup/Dory')
        unmasked.DefinePrim('/World/anim/chars/NemoGroup/Nemo')
        unmasked.DefinePrim('/World/sets/Reef/Coral/CoralGroup1')
        unmasked.DefinePrim('/World/sets/Reef/Rocks/RockGroup1')

        doryMask = Usd.StagePopulationMask().Add('/World/anim/chars/DoryGroup')
        doryStage = Usd.Stage.OpenMasked(unmasked.GetRootLayer(), doryMask)
        assert doryStage.GetPopulationMask() == doryMask

        assert doryStage.GetPrimAtPath('/World')
        assert doryStage.GetPrimAtPath('/World/anim')
        assert doryStage.GetPrimAtPath('/World/anim/chars')
        assert doryStage.GetPrimAtPath('/World/anim/chars/DoryGroup')
        assert doryStage.GetPrimAtPath('/World/anim/chars/DoryGroup/Dory')

        assert not doryStage.GetPrimAtPath('/World/sets')
        assert not doryStage.GetPrimAtPath('/World/anim/chars/NemoGroup')

        doryAndNemoMask = (Usd.StagePopulationMask()
                           .Add('/World/anim/chars/DoryGroup')
                           .Add('/World/anim/chars/NemoGroup'))

        # Test modifying an existing mask.
        doryStage.SetPopulationMask(doryAndNemoMask)
        
        assert doryStage.GetPrimAtPath('/World')
        assert doryStage.GetPrimAtPath('/World/anim')
        assert doryStage.GetPrimAtPath('/World/anim/chars')
        assert doryStage.GetPrimAtPath('/World/anim/chars/DoryGroup')
        assert doryStage.GetPrimAtPath('/World/anim/chars/DoryGroup/Dory')
        assert doryStage.GetPrimAtPath('/World/anim/chars/NemoGroup')
        assert doryStage.GetPrimAtPath('/World/anim/chars/NemoGroup/Nemo')
        
        doryAndNemoStage = Usd.Stage.OpenMasked(
            unmasked.GetRootLayer(), doryAndNemoMask)
        assert doryAndNemoStage.GetPopulationMask() == doryAndNemoMask

        assert doryAndNemoStage.GetPrimAtPath('/World')
        assert doryAndNemoStage.GetPrimAtPath('/World/anim')
        assert doryAndNemoStage.GetPrimAtPath('/World/anim/chars')
        assert doryAndNemoStage.GetPrimAtPath('/World/anim/chars/DoryGroup')
        assert doryAndNemoStage.GetPrimAtPath('/World/anim/chars/DoryGroup/Dory')
        assert doryAndNemoStage.GetPrimAtPath('/World/anim/chars/NemoGroup')
        assert doryAndNemoStage.GetPrimAtPath('/World/anim/chars/NemoGroup/Nemo')

        assert not doryAndNemoStage.GetPrimAtPath('/World/sets')

    def test_Expansion(self):
        stage = Usd.Stage.CreateInMemory()
        a = stage.DefinePrim('/World/A')
        b = stage.DefinePrim('/World/B')
        c = stage.DefinePrim('/World/C')
        d = stage.DefinePrim('/World/D')
        e = stage.DefinePrim('/World/E')

        cAttr = c.CreateAttribute('attr', Sdf.ValueTypeNames.Float)

        a.CreateRelationship('r').AddTarget(b.GetPath())
        b.CreateRelationship('r').AddTarget(cAttr.GetPath())
        c.CreateRelationship('r').AddTarget(d.GetPath())

        a.CreateRelationship('pred').AddTarget(e.GetPath())
        
        mask = Usd.StagePopulationMask().Add(a.GetPath())
        masked = Usd.Stage.OpenMasked(stage.GetRootLayer(), mask)
        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert not masked.GetPrimAtPath(e.GetPath())

        # Now expand the mask for all relationships.
        masked.ExpandPopulationMask()

        assert masked.GetPrimAtPath(a.GetPath())
        assert masked.GetPrimAtPath(b.GetPath())
        assert masked.GetPrimAtPath(c.GetPath())
        assert masked.GetPrimAtPath(d.GetPath())
        assert masked.GetPrimAtPath(e.GetPath())

        masked.SetPopulationMask(Usd.StagePopulationMask().Add(a.GetPath()))
     
        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert not masked.GetPrimAtPath(e.GetPath())

        # Expand with a predicate that only consults relationships named 'pred'
        masked.ExpandPopulationMask(lambda r: r.GetName() == 'pred')

        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert masked.GetPrimAtPath(e.GetPath())

    def test_Bug143308(self):
        # We didn't correctly mask calls to parallel prim indexing, leading to
        # errors with instancing.
        stage = Usd.Stage.CreateInMemory()
        foo, bar, i1, i2 = [
            stage.DefinePrim(p) for p in ('/foo', '/bar', '/i1', '/i2')]
        foo.SetInstanceable(True)
        [p.GetReferences().AddInternalReference(foo.GetPath()) for p in (i1, i2)]
        assert len(stage.GetMasters())
        stage2 = Usd.Stage.OpenMasked(
            stage.GetRootLayer(), Usd.StagePopulationMask(['/i1']))
        assert len(stage2.GetMasters())

    def test_Bug145873(self):
        # The payload inclusion predicate wasn't being invoked on ancestors of
        # requested index paths in pcp.
        payload = Usd.Stage.CreateInMemory()
        for n in ('One', 'Two', 'Three'):
            payload.DefinePrim('/CubesModel/Geom/Cube' + n)

        root = Usd.Stage.CreateInMemory()
        cubes = root.DefinePrim('/Cubes')
        cubes.SetPayload(payload.GetRootLayer().identifier, '/CubesModel')

        testStage = Usd.Stage.OpenMasked(
            root.GetRootLayer(),
            Usd.StagePopulationMask(['/Cubes/Geom/CubeTwo']))

        # Only /Cubes/Geom/CubeTwo (and ancestors) should be present.
        assert testStage.GetPrimAtPath('/Cubes')
        assert testStage.GetPrimAtPath('/Cubes/Geom')
        assert not testStage.GetPrimAtPath('/Cubes/Geom/CubeOne')
        assert testStage.GetPrimAtPath('/Cubes/Geom/CubeTwo')
        assert not testStage.GetPrimAtPath('/Cubes/Geom/CubeThree')

    def test_Bug152904(self):
        # Master prims weren't being generated on stages where the population
        # mask included paths of prims beneath instances.
        stage = Usd.Stage.CreateInMemory()
        stage.DefinePrim('/Ref/geom')
        stage.DefinePrim('/Ref/shading')

        for path in ['/Instance_1', '/Instance_2']:
            prim = stage.DefinePrim(path)
            prim.GetReferences().AddInternalReference('/Ref')
            prim.SetInstanceable(True)

        # Open the stage with a mask that includes the 'geom' prim beneath
        # the instances.   
        maskedStage = Usd.Stage.OpenMasked(
            stage.GetRootLayer(), 
            Usd.StagePopulationMask(['/Instance_1/geom', '/Instance_2/geom']))

        # Both instances should share the same master prim.
        instance_1 = maskedStage.GetPrimAtPath('/Instance_1')
        assert instance_1.IsInstance()
        assert instance_1.GetMaster()

        instance_2 = maskedStage.GetPrimAtPath('/Instance_2')
        assert instance_2.IsInstance()
        assert instance_2.GetMaster()

        # For now, all prims in masters will be composed, even if they are
        # not included in the population mask.
        assert instance_1.GetMaster() == instance_2.GetMaster()
        master = instance_1.GetMaster()

        assert master.GetChild('geom')
        assert master.GetChild('shading')

if __name__ == '__main__':
    unittest.main()
