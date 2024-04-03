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

        assert not doryStage._GetPcpCache().FindPrimIndex('/World/sets')
        assert not doryStage._GetPcpCache().FindPrimIndex(
            '/World/anim/chars/NemoGroup')

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

        assert doryStage._GetPcpCache().FindPrimIndex(
            '/World/anim/chars/NemoGroup')

        doryStage.SetPopulationMask(doryMask)

        assert doryStage.GetPrimAtPath('/World')
        assert doryStage.GetPrimAtPath('/World/anim')
        assert doryStage.GetPrimAtPath('/World/anim/chars')
        assert doryStage.GetPrimAtPath('/World/anim/chars/DoryGroup')
        assert doryStage.GetPrimAtPath('/World/anim/chars/DoryGroup/Dory')
        assert not doryStage.GetPrimAtPath('/World/anim/chars/NemoGroup')
        assert not doryStage.GetPrimAtPath('/World/anim/chars/NemoGroup/Nemo')

        assert not doryStage._GetPcpCache().FindPrimIndex(
            '/World/anim/chars/NemoGroup')
        
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

    def test_ExpansionRelationships(self):
        stage = Usd.Stage.CreateInMemory()
        a = stage.DefinePrim('/World/A')
        b = stage.DefinePrim('/World/B')
        c = stage.DefinePrim('/World/C')
        d = stage.DefinePrim('/World/D')
        e = stage.DefinePrim('/World/E')
        f = stage.OverridePrim('/World/F')
        g = stage.OverridePrim('/World/F/G')
        h = stage.OverridePrim('/World/H')

        cAttr = c.CreateAttribute('attr', Sdf.ValueTypeNames.Float)

        a.CreateRelationship('r').AddTarget(b.GetPath())
        b.CreateRelationship('r').AddTarget(cAttr.GetPath())
        c.CreateRelationship('r').AddTarget(d.GetPath())
        d.CreateRelationship('r').AddTarget(f.GetPath())
        g.CreateRelationship('r').AddTarget(h.GetPath())

        a.CreateRelationship('pred').AddTarget(e.GetPath())
        
        mask = Usd.StagePopulationMask().Add(a.GetPath())
        masked = Usd.Stage.OpenMasked(stage.GetRootLayer(), mask)
        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert not masked.GetPrimAtPath(e.GetPath())
        assert not masked.GetPrimAtPath(f.GetPath())
        assert not masked.GetPrimAtPath(g.GetPath())
        assert not masked.GetPrimAtPath(h.GetPath())

        # Now expand the mask for all relationships, with default traversal.
        masked.ExpandPopulationMask()

        assert masked.GetPrimAtPath(a.GetPath())
        assert masked.GetPrimAtPath(b.GetPath())
        assert masked.GetPrimAtPath(c.GetPath())
        assert masked.GetPrimAtPath(d.GetPath())
        assert masked.GetPrimAtPath(e.GetPath())
        assert masked.GetPrimAtPath(f.GetPath())     # f is hit by d,
        assert masked.GetPrimAtPath(g.GetPath())     # g is included by being
                                                     # descendant to f, but it
                                                     # is not traversed so...
        assert not masked.GetPrimAtPath(h.GetPath()) # h is not included.

        # Reset to just a.
        masked.SetPopulationMask(Usd.StagePopulationMask().Add(a.GetPath()))
     
        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert not masked.GetPrimAtPath(e.GetPath())
        assert not masked.GetPrimAtPath(f.GetPath())
        assert not masked.GetPrimAtPath(g.GetPath())
        assert not masked.GetPrimAtPath(h.GetPath())

        # Expand with a predicate that only consults relationships named 'pred'
        masked.ExpandPopulationMask(
            relationshipPredicate=lambda r: r.GetName() == 'pred')

        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert masked.GetPrimAtPath(e.GetPath())
        assert not masked.GetPrimAtPath(f.GetPath())
        assert not masked.GetPrimAtPath(g.GetPath())
        assert not masked.GetPrimAtPath(h.GetPath())

        # Reset to just a again, then expand with the all prims predicate.
        masked.SetPopulationMask(Usd.StagePopulationMask().Add(a.GetPath()))
        masked.ExpandPopulationMask(Usd.PrimAllPrimsPredicate)
        assert masked.GetPrimAtPath(a.GetPath())
        assert masked.GetPrimAtPath(b.GetPath())
        assert masked.GetPrimAtPath(c.GetPath())
        assert masked.GetPrimAtPath(d.GetPath())
        assert masked.GetPrimAtPath(e.GetPath())
        assert masked.GetPrimAtPath(f.GetPath())
        assert masked.GetPrimAtPath(g.GetPath()) # traversed now, hitting h
        assert masked.GetPrimAtPath(h.GetPath()) # now hit by g.

    def test_ExpansionConnections(self):
        stage = Usd.Stage.CreateInMemory()
        a = stage.DefinePrim('/World/A')
        b = stage.DefinePrim('/World/B')
        c = stage.DefinePrim('/World/C')
        d = stage.DefinePrim('/World/D')
        e = stage.DefinePrim('/World/E')
        f = stage.OverridePrim('/World/F')
        g = stage.OverridePrim('/World/F/G')
        h = stage.OverridePrim('/World/H')

        bAttr = b.CreateAttribute('attr', Sdf.ValueTypeNames.Float)
        cAttr = c.CreateAttribute('attr', Sdf.ValueTypeNames.Float)
        dAttr = d.CreateAttribute('attr', Sdf.ValueTypeNames.Float)
        eAttr = e.CreateAttribute('attr', Sdf.ValueTypeNames.Float)
        fAttr = f.CreateAttribute('attr', Sdf.ValueTypeNames.Float)
        gAttr = g.CreateAttribute('attr', Sdf.ValueTypeNames.Float)
        hAttr = h.CreateAttribute('attr', Sdf.ValueTypeNames.Float)

        floatType = Sdf.ValueTypeNames.Float
        a.CreateAttribute('a', floatType).AddConnection(bAttr.GetPath())
        b.CreateAttribute('a', floatType).AddConnection(cAttr.GetPath())
        c.CreateAttribute('a', floatType).AddConnection(dAttr.GetPath())
        d.CreateAttribute('a', floatType).AddConnection(fAttr.GetPath())
        g.CreateAttribute('a', floatType).AddConnection(hAttr.GetPath())

        a.CreateAttribute('pred', floatType).AddConnection(eAttr.GetPath())
        
        mask = Usd.StagePopulationMask().Add(a.GetPath())
        masked = Usd.Stage.OpenMasked(stage.GetRootLayer(), mask)
        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert not masked.GetPrimAtPath(e.GetPath())
        assert not masked.GetPrimAtPath(f.GetPath())
        assert not masked.GetPrimAtPath(g.GetPath())
        assert not masked.GetPrimAtPath(h.GetPath())

        # Now expand the mask for all connections, with default traversal.
        masked.ExpandPopulationMask()

        assert masked.GetPrimAtPath(a.GetPath())
        assert masked.GetPrimAtPath(b.GetPath())
        assert masked.GetPrimAtPath(c.GetPath())
        assert masked.GetPrimAtPath(d.GetPath())
        assert masked.GetPrimAtPath(e.GetPath())
        assert masked.GetPrimAtPath(f.GetPath())     # f is hit by d,
        assert masked.GetPrimAtPath(g.GetPath())     # g is included by being
                                                     # descendant to f, but it
                                                     # is not traversed so...
        assert not masked.GetPrimAtPath(h.GetPath()) # h is not included.

        # Reset to just a.
        masked.SetPopulationMask(Usd.StagePopulationMask().Add(a.GetPath()))
     
        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert not masked.GetPrimAtPath(e.GetPath())
        assert not masked.GetPrimAtPath(f.GetPath())
        assert not masked.GetPrimAtPath(g.GetPath())
        assert not masked.GetPrimAtPath(h.GetPath())

        # Expand with a predicate that only consults attributes named 'pred'
        masked.ExpandPopulationMask(
            attributePredicate=lambda r: r.GetName() == 'pred')

        assert masked.GetPrimAtPath(a.GetPath())
        assert not masked.GetPrimAtPath(b.GetPath())
        assert not masked.GetPrimAtPath(c.GetPath())
        assert not masked.GetPrimAtPath(d.GetPath())
        assert masked.GetPrimAtPath(e.GetPath())
        assert not masked.GetPrimAtPath(f.GetPath())
        assert not masked.GetPrimAtPath(g.GetPath())
        assert not masked.GetPrimAtPath(h.GetPath())

        # Reset to just a again, then expand with the all prims predicate.
        masked.SetPopulationMask(Usd.StagePopulationMask().Add(a.GetPath()))
        masked.ExpandPopulationMask(Usd.PrimAllPrimsPredicate)
        assert masked.GetPrimAtPath(a.GetPath())
        assert masked.GetPrimAtPath(b.GetPath())
        assert masked.GetPrimAtPath(c.GetPath())
        assert masked.GetPrimAtPath(d.GetPath())
        assert masked.GetPrimAtPath(e.GetPath())
        assert masked.GetPrimAtPath(f.GetPath())
        assert masked.GetPrimAtPath(g.GetPath()) # traversed now, hitting h
        assert masked.GetPrimAtPath(h.GetPath()) # now hit by g.

    def test_Bug143308(self):
        # We didn't correctly mask calls to parallel prim indexing, leading to
        # errors with instancing.
        stage = Usd.Stage.CreateInMemory()
        foo, bar, i1, i2 = [
            stage.DefinePrim(p) for p in ('/foo', '/bar', '/i1', '/i2')]
        foo.SetInstanceable(True)
        [p.GetReferences().AddInternalReference(foo.GetPath()) for p in (i1, i2)]
        assert len(stage.GetPrototypes())
        stage2 = Usd.Stage.OpenMasked(
            stage.GetRootLayer(), Usd.StagePopulationMask(['/i1']))
        assert len(stage2.GetPrototypes())

    def test_Bug145873(self):
        # The payload inclusion predicate wasn't being invoked on ancestors of
        # requested index paths in pcp.
        payload = Usd.Stage.CreateInMemory()
        for n in ('One', 'Two', 'Three'):
            payload.DefinePrim('/CubesModel/Geom/Cube' + n)

        root = Usd.Stage.CreateInMemory()
        cubes = root.DefinePrim('/Cubes')
        cubes.GetPayloads().AddPayload(payload.GetRootLayer().identifier, 
                                       '/CubesModel')

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
        # Prototype prims weren't being generated on stages where the population
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

        # Both instances should share the same prototype prim.
        instance_1 = maskedStage.GetPrimAtPath('/Instance_1')
        assert instance_1.IsInstance()
        assert instance_1.GetPrototype()

        instance_2 = maskedStage.GetPrimAtPath('/Instance_2')
        assert instance_2.IsInstance()
        assert instance_2.GetPrototype()

        # Only the 'geom' prim in the prototype will be composed, since
        # it's the only one in the population mask.
        assert instance_1.GetPrototype() == instance_2.GetPrototype()
        prototype = instance_1.GetPrototype()

        assert prototype.GetChild('geom')
        assert not prototype.GetChild('shading')

        # Open the stage with a mask that includes the 'geom' prim beneath
        # /Instance_1 and all children beneath /Instance_2.
        maskedStage = Usd.Stage.OpenMasked(
            stage.GetRootLayer(), 
            Usd.StagePopulationMask(['/Instance_1/geom', '/Instance_2']))

        # Both instances should *not* share the same prototype, since they
        # are affected by different population masks.
        instance_1 = maskedStage.GetPrimAtPath('/Instance_1')
        assert instance_1.IsInstance()
        assert instance_1.GetPrototype()

        instance_2 = maskedStage.GetPrimAtPath('/Instance_2')
        assert instance_2.IsInstance()
        assert instance_2.GetPrototype()

        # Only the 'geom' prim will be composed in the prototype for the
        # /Instance_1, but both 'geom' and 'shading' will be composed for
        # /Instance_2.
        assert instance_1.GetPrototype() != instance_2.GetPrototype()
        prototype = instance_1.GetPrototype()

        assert prototype.GetChild('geom')
        assert not prototype.GetChild('shading')

        prototype = instance_2.GetPrototype()
        
        assert prototype.GetChild('geom')
        assert prototype.GetChild('shading')

if __name__ == '__main__':
    unittest.main()
