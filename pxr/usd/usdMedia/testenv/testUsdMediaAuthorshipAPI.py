#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Tf, Usd, UsdGeom, UsdMedia
import unittest


def _RecordIds(records):
    """Reduces records to (primPath, instanceName) pairs for comparison."""
    return [(str(r.GetPrim().GetPath()), r.GetName()) for r in records]


class TestUsdMediaAuthorshipAPI(unittest.TestCase):

    def test_ApplyAndName(self):
        stage = Usd.Stage.CreateInMemory()
        prim = UsdGeom.Mesh.Define(stage, '/World/Bunny').GetPrim()

        # Not applied yet.
        self.assertFalse(UsdMedia.AuthorshipAPI(prim, 'hunyuan3d'))
        self.assertFalse(prim.HasAPI(UsdMedia.AuthorshipAPI, 'hunyuan3d'))

        api = UsdMedia.AuthorshipAPI.Apply(prim, 'hunyuan3d')
        self.assertTrue(api)
        self.assertEqual(api.GetName(), 'hunyuan3d')
        self.assertTrue(prim.HasAPI(UsdMedia.AuthorshipAPI, 'hunyuan3d'))
        self.assertEqual(prim.GetAppliedSchemas(), ['AuthorshipAPI:hunyuan3d'])

    def test_PropertyNamespacing(self):
        """Property names are the interop contract."""
        stage = Usd.Stage.CreateInMemory()
        prim = UsdGeom.Mesh.Define(stage, '/World/Bunny').GetPrim()
        api = UsdMedia.AuthorshipAPI.Apply(prim, 'hunyuan3d')

        api.CreateSoftwarePackageAttr('net.trellis3d.hunyuan3d')
        self.assertEqual(api.GetSoftwarePackageAttr().GetName(),
                         'authorship:hunyuan3d:softwarePackage')

        expected = set([
            'authorship:hunyuan3d:softwarePackage',
            'authorship:hunyuan3d:softwareVersion',
            'authorship:hunyuan3d:digitalSourceType',
            'authorship:hunyuan3d:creator',
            'authorship:hunyuan3d:description',
            'authorship:hunyuan3d:prompt:inputNames',
            'authorship:hunyuan3d:prompt:inputValues',
            'authorship:hunyuan3d:created',
            'authorship:hunyuan3d:instanceID',
            'authorship:hunyuan3d:usageTerms',
            'authorship:hunyuan3d:copyrightOwner',
            'authorship:hunyuan3d:contact',
        ])
        self.assertEqual(
            set(UsdMedia.AuthorshipAPI.GetSchemaAttributeNames(
                False, 'hunyuan3d')),
            expected)

    def test_RoundTripAllFields(self):
        stage = Usd.Stage.CreateInMemory()
        prim = UsdGeom.Mesh.Define(stage, '/World/Bunny').GetPrim()
        api = UsdMedia.AuthorshipAPI.Apply(prim, 'hunyuan3d')

        api.CreateSoftwarePackageAttr('net.trellis3d.hunyuan3d')
        api.CreateSoftwareVersionAttr('2.1')
        api.CreateDigitalSourceTypeAttr(
            'http://cv.iptc.org/newscodes/digitalsourcetype/'
            'trainedAlgorithmicMedia')
        api.CreateCreatorAttr(['Trellis Hunyuan 3D', 'John Doe'])
        api.CreateDescriptionAttr('Generated, then decimated.')
        api.CreatePromptInputNamesAttr(['prompt', 'image', 'seed'])
        api.CreatePromptInputValuesAttr(
            ['A fluffy bunny', './refs/bunny_front.png', '1234567'])
        api.CreateCreatedAttr('2025-02-16T12:03:17+01:00')
        api.CreateInstanceIDAttr('6530a534-ca8f-487c-8968-0fecd8e717a6')
        api.CreateUsageTermsAttr('CC-BY-SA-4.0')
        api.CreateCopyrightOwnerAttr(['Acme Studios LLC'])
        api.CreateContactAttr(['johndoe@sample.com'])

        self.assertEqual(api.GetSoftwarePackageAttr().Get(),
                         'net.trellis3d.hunyuan3d')
        self.assertEqual(api.GetSoftwareVersionAttr().Get(), '2.1')
        self.assertEqual(
            api.GetDigitalSourceTypeAttr().Get(),
            'http://cv.iptc.org/newscodes/digitalsourcetype/'
            'trainedAlgorithmicMedia')
        self.assertEqual(list(api.GetCreatorAttr().Get()),
                         ['Trellis Hunyuan 3D', 'John Doe'])
        self.assertEqual(api.GetDescriptionAttr().Get(),
                         'Generated, then decimated.')
        self.assertEqual(list(api.GetPromptInputNamesAttr().Get()),
                         ['prompt', 'image', 'seed'])
        self.assertEqual(list(api.GetPromptInputValuesAttr().Get()),
                         ['A fluffy bunny', './refs/bunny_front.png',
                          '1234567'])
        self.assertEqual(api.GetCreatedAttr().Get(),
                         '2025-02-16T12:03:17+01:00')
        self.assertEqual(api.GetInstanceIDAttr().Get(),
                         '6530a534-ca8f-487c-8968-0fecd8e717a6')
        self.assertEqual(api.GetUsageTermsAttr().Get(), 'CC-BY-SA-4.0')
        self.assertEqual(list(api.GetCopyrightOwnerAttr().Get()),
                         ['Acme Studios LLC'])
        self.assertEqual(list(api.GetContactAttr().Get()),
                         ['johndoe@sample.com'])

        # No fallbacks, so an unauthored field is distinguishable from an
        # authored empty value.
        other = UsdMedia.AuthorshipAPI.Apply(prim, 'blender')
        self.assertFalse(other.GetSoftwarePackageAttr().HasAuthoredValue())
        self.assertIsNone(other.GetSoftwarePackageAttr().Get())
        self.assertFalse(other.GetUsageTermsAttr().HasAuthoredValue())
        self.assertIsNone(other.GetCopyrightOwnerAttr().Get())

    def test_MultipleRecordsDoNotCollide(self):
        stage = Usd.Stage.CreateInMemory()
        prim = UsdGeom.Mesh.Define(stage, '/World/Bunny').GetPrim()

        ai = UsdMedia.AuthorshipAPI.Apply(prim, 'hunyuan3d')
        ai.CreateSoftwarePackageAttr('net.trellis3d.hunyuan3d')
        human = UsdMedia.AuthorshipAPI.Apply(prim, 'blender')
        human.CreateSoftwarePackageAttr('org.blender')

        self.assertEqual(ai.GetSoftwarePackageAttr().Get(), 'net.trellis3d.hunyuan3d')
        self.assertEqual(human.GetSoftwarePackageAttr().Get(), 'org.blender')
        self.assertEqual(
            sorted(r.GetName() for r in UsdMedia.AuthorshipAPI.GetAll(prim)),
            ['blender', 'hunyuan3d'])

    def test_RecordsComposeFromSeparateLayers(self):
        strong = Sdf.Layer.CreateAnonymous('strong.usda')
        weak = Sdf.Layer.CreateAnonymous('weak.usda')

        weakStage = Usd.Stage.Open(weak)
        weakPrim = weakStage.DefinePrim('/Bunny', 'Mesh')
        UsdMedia.AuthorshipAPI.Apply(weakPrim, 'hunyuan3d') \
            .CreateSoftwarePackageAttr('net.trellis3d.hunyuan3d')

        strong.subLayerPaths.append(weak.identifier)
        stage = Usd.Stage.Open(strong)
        prim = stage.GetPrimAtPath('/Bunny')
        UsdMedia.AuthorshipAPI.Apply(prim, 'blender') \
            .CreateSoftwarePackageAttr('org.blender')

        self.assertEqual(
            sorted(r.GetName() for r in UsdMedia.AuthorshipAPI.GetAll(prim)),
            ['blender', 'hunyuan3d'])
        self.assertEqual(
            UsdMedia.AuthorshipAPI(prim, 'hunyuan3d').GetSoftwarePackageAttr().Get(),
            'net.trellis3d.hunyuan3d')

    def test_GetAllOnStageSorted(self):
        stage = Usd.Stage.CreateInMemory()
        for path, names in [('/B', ['zebra', 'apple']), ('/A', ['mid'])]:
            prim = stage.DefinePrim(path, 'Xform')
            for name in names:
                UsdMedia.AuthorshipAPI.Apply(prim, name)

        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.GetAllOnStage(stage)),
            [('/A', 'mid'), ('/B', 'apple'), ('/B', 'zebra')])

    def test_GetAllOnStageEmpty(self):
        stage = Usd.Stage.CreateInMemory()
        stage.DefinePrim('/NoRecords', 'Xform')
        self.assertEqual(UsdMedia.AuthorshipAPI.GetAllOnStage(stage), [])
        self.assertEqual(
            UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True), [])

    def test_GetAllOnStageIncludesInactivePrims(self):
        """A record is worth reporting whether or not its prim is currently
        composed into the scene."""
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim('/Hidden', 'Xform')
        UsdMedia.AuthorshipAPI.Apply(prim, 'worldgen')
        prim.SetActive(False)

        self.assertEqual(_RecordIds(UsdMedia.AuthorshipAPI.GetAllOnStage(stage)),
                         [('/Hidden', 'worldgen')])

    def test_DeepSearchFindsUnappliedProperties(self):
        """Some tools author the properties without ever applying the schema.
        The composed prim cannot report those, but the deeper search can."""
        layer = Sdf.Layer.CreateAnonymous('.usda')
        spec = Sdf.CreatePrimInLayer(layer, '/Orphan')
        spec.specifier = Sdf.SpecifierDef
        attr = Sdf.AttributeSpec(spec, 'authorship:ghost:softwarePackage',
                                 Sdf.ValueTypeNames.String,
                                 Sdf.VariabilityUniform)
        attr.default = 'com.example.ghost'

        stage = Usd.Stage.Open(layer)
        prim = stage.GetPrimAtPath('/Orphan')
        self.assertFalse(prim.HasAPI(UsdMedia.AuthorshipAPI, 'ghost'))

        self.assertEqual(UsdMedia.AuthorshipAPI.GetAllOnStage(stage), [])

        deep = UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True)
        self.assertEqual(_RecordIds(deep), [('/Orphan', 'ghost')])
        # The record reads composed values normally even though the schema was
        # never applied.
        self.assertEqual(deep[0].GetSoftwarePackageAttr().Get(), 'com.example.ghost')

    def test_DeepSearchIgnoresUnrelatedProperties(self):
        layer = Sdf.Layer.CreateAnonymous('.usda')
        spec = Sdf.CreatePrimInLayer(layer, '/Decoy')
        spec.specifier = Sdf.SpecifierDef
        for name in ['authorship', 'authorship:nofield',
                     'authorship:inst:notAField', 'notauthorship:inst:softwarePackage']:
            attr = Sdf.AttributeSpec(spec, name, Sdf.ValueTypeNames.String,
                                     Sdf.VariabilityUniform)
            attr.default = 'x'

        stage = Usd.Stage.Open(layer)
        self.assertEqual(
            UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True), [])

    def test_DeepSearchRecoversShadowedApplication(self):
        """An explicit apiSchemas list in a stronger layer can shadow an
        application coming from a reference. The properties still compose, so
        the record is still readable, but the composed prim no longer reports
        the schema as applied."""
        asset = Sdf.Layer.CreateAnonymous('asset.usda')
        assetStage = Usd.Stage.Open(asset)
        rock = assetStage.DefinePrim('/RockLarge', 'Xform')
        UsdMedia.AuthorshipAPI.Apply(rock, 'rockmaker') \
            .CreateSoftwarePackageAttr('com.example.rocktool')
        assetStage.SetDefaultPrim(rock)

        root = Sdf.Layer.CreateAnonymous('scene.usda')
        stage = Usd.Stage.Open(root)
        referencing = stage.DefinePrim('/World/Rock_1', 'Xform')
        referencing.GetReferences().AddReference(asset.identifier)
        self.assertEqual(referencing.GetAppliedSchemas(),
                         ['AuthorshipAPI:rockmaker'])

        primSpec = root.GetPrimAtPath('/World/Rock_1')
        primSpec.SetInfo('apiSchemas',
                         Sdf.TokenListOp.CreateExplicit(
                             ['AuthorshipAPI:layout']))

        self.assertEqual(referencing.GetAppliedSchemas(),
                         ['AuthorshipAPI:layout'])
        # The shadowed record's value is still readable.
        self.assertEqual(
            referencing.GetAttribute('authorship:rockmaker:softwarePackage').Get(),
            'com.example.rocktool')

        self.assertEqual(
            sorted(r.GetName()
                   for r in UsdMedia.AuthorshipAPI.GetAllOnStage(stage)),
            ['layout'])
        self.assertEqual(
            sorted(r.GetName()
                   for r in UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True)),
            ['layout', 'rockmaker'])

    def test_DeepSearchOneSpecReportedOnce(self):
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim('/World/Bunny', 'Mesh')
        UsdMedia.AuthorshipAPI.Apply(prim, 'hunyuan3d') \
            .CreateSoftwarePackageAttr('net.trellis3d.hunyuan3d')

        # A record declared in one spec both by apiSchemas and by its
        # properties is one contribution, not two.
        self.assertEqual(_RecordIds(UsdMedia.AuthorshipAPI.GetAllOnStage(stage)),
                         [('/World/Bunny', 'hunyuan3d')])
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True)),
            [('/World/Bunny', 'hunyuan3d')])

    def test_DeepSearchReportsClobberedRecordPerSpec(self):
        """The same instance name in two layers is one composed record, so
        composition silently drops the weaker values. The deeper search reports
        one entry per contributing spec, which is how that becomes visible."""
        weak = Sdf.Layer.CreateAnonymous('weak.usda')
        weakStage = Usd.Stage.Open(weak)
        weakPrim = weakStage.DefinePrim('/Bunny', 'Mesh')
        weakApi = UsdMedia.AuthorshipAPI.Apply(weakPrim, 'blender')
        weakApi.CreateSoftwarePackageAttr('org.blender')
        weakApi.CreateSoftwareVersionAttr('4.1')

        strong = Sdf.Layer.CreateAnonymous('strong.usda')
        strong.subLayerPaths.append(weak.identifier)
        stage = Usd.Stage.Open(strong)
        prim = stage.GetPrimAtPath('/Bunny')
        strongApi = UsdMedia.AuthorshipAPI.Apply(prim, 'blender')
        strongApi.CreateSoftwarePackageAttr('com.example.other')
        strongApi.CreateSoftwareVersionAttr('9.9')

        # Composition keeps one record, and only the strongest values.
        self.assertEqual(prim.GetAppliedSchemas(), ['AuthorshipAPI:blender'])
        self.assertEqual(_RecordIds(UsdMedia.AuthorshipAPI.GetAllOnStage(stage)),
                         [('/Bunny', 'blender')])
        self.assertEqual(
            prim.GetAttribute('authorship:blender:softwarePackage').Get(),
            'com.example.other')

        # Two specs contribute, so the deeper search returns two entries.
        deep = UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True)
        self.assertEqual(_RecordIds(deep),
                         [('/Bunny', 'blender'), ('/Bunny', 'blender')])
        # Both read composed values; they record that two specs contribute, not
        # what each one said. The clobbered value is still in the prim stack.
        self.assertEqual([r.GetSoftwarePackageAttr().Get() for r in deep],
                         ['com.example.other', 'com.example.other'])
        self.assertEqual(
            [spec.attributes['authorship:blender:softwarePackage'].default
             for spec in prim.GetPrimStack()],
            ['com.example.other', 'org.blender'])

    def test_DeepSearchFindsNamespacedInstanceNames(self):
        """Instance names may themselves be namespaced."""
        layer = Sdf.Layer.CreateAnonymous('.usda')
        spec = Sdf.CreatePrimInLayer(layer, '/Prim')
        spec.specifier = Sdf.SpecifierDef
        attr = Sdf.AttributeSpec(spec, 'authorship:studio:tool:softwarePackage',
                                 Sdf.ValueTypeNames.String,
                                 Sdf.VariabilityUniform)
        attr.default = 'com.example.tool'

        stage = Usd.Stage.Open(layer)
        deep = UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True)
        self.assertEqual(_RecordIds(deep), [('/Prim', 'studio:tool')])
        self.assertEqual(deep[0].GetSoftwarePackageAttr().Get(), 'com.example.tool')

    def test_DeepSearchFindsNamespacedBaseName(self):
        """A property base name may itself be namespaced (prompt:inputNames),
        so the deeper search must not mistake its last segment for the whole
        base name when locating the instance name ahead of it."""
        layer = Sdf.Layer.CreateAnonymous('.usda')
        spec = Sdf.CreatePrimInLayer(layer, '/Prim')
        spec.specifier = Sdf.SpecifierDef
        attr = Sdf.AttributeSpec(spec, 'authorship:hunyuan3d:prompt:inputNames',
                                 Sdf.ValueTypeNames.StringArray,
                                 Sdf.VariabilityUniform)
        attr.default = ['prompt']

        stage = Usd.Stage.Open(layer)
        deep = UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True)
        self.assertEqual(_RecordIds(deep), [('/Prim', 'hunyuan3d')])
        self.assertEqual(list(deep[0].GetPromptInputNamesAttr().Get()),
                         ['prompt'])

        # Combined with a namespaced instance name too.
        attr2 = Sdf.AttributeSpec(
            spec, 'authorship:studio:tool:prompt:inputValues',
            Sdf.ValueTypeNames.StringArray, Sdf.VariabilityUniform)
        attr2.default = ['A fluffy bunny']

        deep = UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True)
        self.assertEqual(
            sorted(_RecordIds(deep)),
            [('/Prim', 'hunyuan3d'), ('/Prim', 'studio:tool')])

    def test_DeepSearchScansEveryListOpType(self):
        """A record is found however its application was list-edited."""
        def MakeListOp(field, name):
            listOp = Sdf.TokenListOp()
            setattr(listOp, field, ['AuthorshipAPI:' + name])
            return listOp

        cases = [('explicitItems', 'a'), ('prependedItems', 'b'),
                 ('appendedItems', 'c'), ('orderedItems', 'd'),
                 ('addedItems', 'e')]
        for field, name in cases:
            layer = Sdf.Layer.CreateAnonymous('.usda')
            spec = Sdf.CreatePrimInLayer(layer, '/Prim')
            spec.specifier = Sdf.SpecifierDef
            spec.SetInfo('apiSchemas', MakeListOp(field, name))

            stage = Usd.Stage.Open(layer)
            deep = UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True)
            self.assertEqual(_RecordIds(deep), [('/Prim', name)],
                             'failed for %s' % field)

    def test_RecordsInsidePrototypes(self):
        """Authorship inside a native instance is shared through the prototype,
        so it is reported once against the prototype rather than once per
        instance."""
        asset = Sdf.Layer.CreateAnonymous('proto.usda')
        assetStage = Usd.Stage.Open(asset)
        group = assetStage.DefinePrim('/Group', 'Xform')
        inner = assetStage.DefinePrim('/Group/Inner', 'Mesh')
        UsdMedia.AuthorshipAPI.Apply(inner, 'worldgen') \
            .CreateSoftwarePackageAttr('com.example.worldgen')
        assetStage.SetDefaultPrim(group)

        root = Sdf.Layer.CreateAnonymous('scene.usda')
        stage = Usd.Stage.Open(root)
        for path in ['/World/A', '/World/B']:
            prim = stage.DefinePrim(path, 'Xform')
            prim.GetReferences().AddReference(asset.identifier)
            prim.SetInstanceable(True)

        self.assertEqual(len(stage.GetPrototypes()), 1)

        records = UsdMedia.AuthorshipAPI.GetAllOnStage(stage)
        # Reported once, against the prototype's child rather than either
        # instance.
        self.assertEqual([r.GetName() for r in records], ['worldgen'])
        self.assertTrue(records[0].GetPrim().IsInPrototype())
        self.assertEqual(records[0].GetSoftwarePackageAttr().Get(),
                         'com.example.worldgen')

    def test_ComputeAccumulatedRecords(self):
        stage = Usd.Stage.CreateInMemory()
        group = stage.DefinePrim('/Set', 'Xform')
        UsdMedia.AuthorshipAPI.Apply(group, 'worldgen') \
            .CreateSoftwarePackageAttr('com.example.worldgen')
        hero = stage.DefinePrim('/Set/HeroProp', 'Mesh')
        UsdMedia.AuthorshipAPI.Apply(hero, 'jane') \
            .CreateSoftwarePackageAttr('org.blender')
        detail = stage.DefinePrim('/Set/HeroProp/Detail', 'Mesh')
        UsdMedia.AuthorshipAPI.Apply(detail, 'detail')

        # Ancestors first, so the list reads as how this prim came to be.
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.ComputeAccumulatedRecords(hero)),
            [('/Set', 'worldgen'), ('/Set/HeroProp', 'jane')])
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.ComputeAccumulatedRecords(detail)),
            [('/Set', 'worldgen'), ('/Set/HeroProp', 'jane'),
             ('/Set/HeroProp/Detail', 'detail')])
        # A record does not propagate upward.
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.ComputeAccumulatedRecords(group)),
            [('/Set', 'worldgen')])

        # A prim's own record is distinguishable from an inherited one.
        records = UsdMedia.AuthorshipAPI.ComputeAccumulatedRecords(hero)
        self.assertEqual([r.GetPrim() == hero for r in records], [False, True])

    def test_ComputeAccumulatedRecordsNoRecords(self):
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim('/A/B/C', 'Xform')
        self.assertEqual(
            UsdMedia.AuthorshipAPI.ComputeAccumulatedRecords(prim), [])

        with self.assertRaises(Tf.ErrorException):
            UsdMedia.AuthorshipAPI.ComputeAccumulatedRecords(Usd.Prim())

    def test_ComputeAccumulatedRecordsDeep(self):
        """The deeper search applies to every prim in the chain, so a record an
        ancestor hid is still accumulated."""
        asset = Sdf.Layer.CreateAnonymous('asset.usda')
        assetStage = Usd.Stage.Open(asset)
        group = assetStage.DefinePrim('/Group', 'Xform')
        UsdMedia.AuthorshipAPI.Apply(group, 'hidden') \
            .CreateSoftwarePackageAttr('com.example.hidden')
        assetStage.SetDefaultPrim(group)

        root = Sdf.Layer.CreateAnonymous('scene.usda')
        stage = Usd.Stage.Open(root)
        referencing = stage.DefinePrim('/World', 'Xform')
        referencing.GetReferences().AddReference(asset.identifier)
        child = stage.DefinePrim('/World/Child', 'Mesh')
        UsdMedia.AuthorshipAPI.Apply(child, 'own')

        # Shadow the referenced application on the ancestor.
        root.GetPrimAtPath('/World').SetInfo(
            'apiSchemas', Sdf.TokenListOp.CreateExplicit([]))

        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.ComputeAccumulatedRecords(child)),
            [('/World/Child', 'own')])
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.ComputeAccumulatedRecords(
                child, True)),
            [('/World', 'hidden'), ('/World/Child', 'own')])

    def test_GetAllUnder(self):
        stage = Usd.Stage.CreateInMemory()
        group = stage.DefinePrim('/Set', 'Xform')
        UsdMedia.AuthorshipAPI.Apply(group, 'worldgen')
        hero = stage.DefinePrim('/Set/HeroProp', 'Mesh')
        UsdMedia.AuthorshipAPI.Apply(hero, 'jane')
        stage.DefinePrim('/Set/HeroProp/Detail', 'Mesh')
        UsdMedia.AuthorshipAPI.Apply(
            stage.GetPrimAtPath('/Set/HeroProp/Detail'), 'detail')
        elsewhere = stage.DefinePrim('/Elsewhere', 'Xform')
        UsdMedia.AuthorshipAPI.Apply(elsewhere, 'other')

        # Includes the prim itself, and stops at the subtree.
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.GetAllUnder(group)),
            [('/Set', 'worldgen'), ('/Set/HeroProp', 'jane'),
             ('/Set/HeroProp/Detail', 'detail')])
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.GetAllUnder(hero)),
            [('/Set/HeroProp', 'jane'), ('/Set/HeroProp/Detail', 'detail')])
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.GetAllUnder(elsewhere)),
            [('/Elsewhere', 'other')])

        with self.assertRaises(Tf.ErrorException):
            UsdMedia.AuthorshipAPI.GetAllUnder(Usd.Prim())

    def test_GetAllUnderDeep(self):
        layer = Sdf.Layer.CreateAnonymous('.usda')
        spec = Sdf.CreatePrimInLayer(layer, '/Root/Child')
        spec.specifier = Sdf.SpecifierDef
        attr = Sdf.AttributeSpec(spec, 'authorship:ghost:softwarePackage',
                                 Sdf.ValueTypeNames.String,
                                 Sdf.VariabilityUniform)
        attr.default = 'com.example.ghost'

        stage = Usd.Stage.Open(layer)
        root = stage.GetPrimAtPath('/Root')
        self.assertEqual(UsdMedia.AuthorshipAPI.GetAllUnder(root), [])
        self.assertEqual(
            _RecordIds(UsdMedia.AuthorshipAPI.GetAllUnder(root, True)),
            [('/Root/Child', 'ghost')])

    def test_GetAllInLayerScopedToThatLayer(self):
        """A layer query reports what that layer authors, composing nothing, so
        a sublayer's records belong to the sublayer."""
        weak = Sdf.Layer.CreateAnonymous('weak.usda')
        weakStage = Usd.Stage.Open(weak)
        UsdMedia.AuthorshipAPI.Apply(
            weakStage.DefinePrim('/A', 'Xform'), 'inweak')

        root = Sdf.Layer.CreateAnonymous('root.usda')
        root.subLayerPaths.append(weak.identifier)
        stage = Usd.Stage.Open(root)
        UsdMedia.AuthorshipAPI.Apply(
            stage.DefinePrim('/B', 'Xform'), 'inroot')

        self.assertEqual([str(p) for p in
                          UsdMedia.AuthorshipAPI.GetAllInLayer(root)],
                         ['/B.authorship:inroot'])
        self.assertEqual([str(p) for p in
                          UsdMedia.AuthorshipAPI.GetAllInLayer(weak)],
                         ['/A.authorship:inweak'])

        # The composed stage sees both, which is the distinction being drawn.
        self.assertEqual(
            sorted(r.GetName()
                   for r in UsdMedia.AuthorshipAPI.GetAllOnStage(stage)),
            ['inroot', 'inweak'])

    def test_GetAllInLayerFindsUnselectedVariants(self):
        """Specs inside an unselected variant are not part of any prim's spec
        stack, so no stage can report them. A layer query can."""
        layer = Sdf.Layer.CreateAnonymous('variants.usda')
        stage = Usd.Stage.Open(layer)
        prim = stage.DefinePrim('/Prim', 'Xform')
        variantSet = prim.GetVariantSets().AddVariantSet('v')
        variantSet.AddVariant('on')
        variantSet.AddVariant('off')
        variantSet.SetVariantSelection('on')
        with variantSet.GetVariantEditContext():
            UsdMedia.AuthorshipAPI.Apply(prim, 'invariant') \
                .CreateSoftwarePackageAttr('com.example.variant')
        variantSet.SetVariantSelection('off')

        self.assertEqual(UsdMedia.AuthorshipAPI.GetAllOnStage(stage), [])
        self.assertEqual(
            UsdMedia.AuthorshipAPI.GetAllOnStage(stage, True), [])
        self.assertEqual([str(p) for p in
                          UsdMedia.AuthorshipAPI.GetAllInLayer(layer)],
                         ['/Prim{v=on}.authorship:invariant'])

    def test_GetAllInLayerPathsRoundTrip(self):
        """Returned paths are in the form Get() and IsAuthorshipAPIPath()
        accept."""
        layer = Sdf.Layer.CreateAnonymous('round.usda')
        stage = Usd.Stage.Open(layer)
        UsdMedia.AuthorshipAPI.Apply(
            stage.DefinePrim('/Prim', 'Xform'), 'gen') \
            .CreateSoftwarePackageAttr('com.example.gen')

        paths = UsdMedia.AuthorshipAPI.GetAllInLayer(layer)
        self.assertEqual(len(paths), 1)
        self.assertEqual(
            UsdMedia.AuthorshipAPI.Get(stage, paths[0]).GetSoftwarePackageAttr().Get(),
            'com.example.gen')

    def test_GetAllInLayerEmptyAndInvalid(self):
        layer = Sdf.Layer.CreateAnonymous('bare.usda')
        Usd.Stage.Open(layer).DefinePrim('/NoRecords', 'Xform')
        self.assertEqual(UsdMedia.AuthorshipAPI.GetAllInLayer(layer), [])

        with self.assertRaises(Tf.ErrorException):
            UsdMedia.AuthorshipAPI.GetAllInLayer(None)

    def test_InvalidStage(self):
        with self.assertRaises(Tf.ErrorException):
            UsdMedia.AuthorshipAPI.GetAllOnStage(None)


if __name__ == '__main__':
    unittest.main()
