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
from pxr import Usd, Sdf, Kind

allFormats = ['usd' + x for x in 'ac']

class TestUsdModel(unittest.TestCase):
    def test_ModelKind(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestModelKind.'+fmt)
            p = s.DefinePrim('/World', 'Xform')
            model = Usd.ModelAPI(p)
            self.assertEqual(model.GetKind(), '')
            self.assertFalse(model.IsModel())
            self.assertFalse(model.IsGroup())

            model.SetKind(Kind.Tokens.component)
            self.assertEqual(model.GetKind(), Kind.Tokens.component)
            self.assertTrue(model.IsModel())
            self.assertFalse(model.IsGroup())

            model.SetKind(Kind.Tokens.assembly)
            self.assertEqual(model.GetKind(), Kind.Tokens.assembly)
            self.assertTrue(model.IsModel())
            self.assertTrue(model.IsGroup())

            model.SetKind(Kind.Tokens.subcomponent)
            self.assertEqual(model.GetKind(), Kind.Tokens.subcomponent)
            self.assertFalse(model.IsModel())
            self.assertFalse(model.IsGroup())

    def test_ModelHierarchy(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestModelHierarchy.'+fmt)
            x = s.DefinePrim('/X', 'Scope')
            y = s.DefinePrim('/X/Y', 'Scope')
            z = s.DefinePrim('/X/Y/Z', 'Scope')

            self.assertFalse(x.IsModel())
            self.assertFalse(y.IsModel())
            self.assertFalse(z.IsModel())

            xm = Usd.ModelAPI(x)
            ym = Usd.ModelAPI(y)
            zm = Usd.ModelAPI(z)

            # X is not a model. Hence, Y can't be a model, even if it has kind set
            # to 'component'.
            ym.SetKind(Kind.Tokens.component)
            self.assertFalse(ym.IsModel())

            # Setting X's kind to component, causes it to be a model, but Y still 
            # remains a non-model as component below another component violates the 
            # model hierarchy.
            xm.SetKind(Kind.Tokens.component)
            self.assertTrue(xm.IsModel())
            self.assertFalse(ym.IsModel())

            # Setting X's kind to assembly fixes the model hierarchy and causes 
            # X to be a model group and Y to be a model.
            xm.SetKind(Kind.Tokens.assembly)
            self.assertTrue(xm.IsModel())
            self.assertTrue(xm.IsGroup())
            self.assertTrue(ym.IsModel())
            self.assertFalse(ym.IsGroup())

            # A component below a component violates model hierarchy.
            zm.SetKind(Kind.Tokens.component)
            self.assertFalse(zm.IsModel())

            # A subcomponent also isn't considered to be a model.
            zm.SetKind(Kind.Tokens.subcomponent)
            self.assertFalse(zm.IsModel())

    def test_AssetInfo(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestAssetInfo.'+fmt)
            p = s.DefinePrim('/World', 'Xform')
            model = Usd.ModelAPI(p)

            self.assertEqual(model.GetAssetInfo(), {})

            model.SetAssetName('PaperCup')
            self.assertEqual(model.GetAssetName(), 'PaperCup')

            model.SetAssetVersion('10a')
            self.assertEqual(model.GetAssetVersion(), '10a')

            model.SetAssetIdentifier('PaperCup/usd/PaperCup.usd')
            self.assertEqual(model.GetAssetIdentifier(), 'PaperCup/usd/PaperCup.usd')

            pad = Sdf.AssetPathArray([Sdf.AssetPath('Paper/usd/Paper.usd'), 
                    Sdf.AssetPath('Cup/usd/Cup.usd')])
            model.SetPayloadAssetDependencies(pad)
            self.assertEqual(model.GetPayloadAssetDependencies(), pad)

            expectedAssetInfo = {
                'identifier': Sdf.AssetPath('PaperCup/usd/PaperCup.usd'), 
                'name': 'PaperCup', 
                'version': '10a',
                'payloadAssetDependencies':
                Sdf.AssetPathArray([Sdf.AssetPath('Paper/usd/Paper.usd'),
                                    Sdf.AssetPath('Cup/usd/Cup.usd')])
            }
            self.assertEqual(model.GetAssetInfo(), expectedAssetInfo)

            stageContents = s.ExportToString()

            self.assertTrue('string name = "PaperCup"' in stageContents)
            self.assertTrue('asset identifier = @PaperCup/usd/PaperCup.usd@'
                       in stageContents)
            self.assertTrue('asset[] payloadAssetDependencies = '
                       '[@Paper/usd/Paper.usd@, @Cup/usd/Cup.usd@]'
                       in stageContents)
            self.assertTrue('string version = "10a"' in stageContents)

    # This test attempts to exercise some features of generated schemas that
    # we cannot test in any other way at this level.  Ideally we would be able
    # to build and test the files generated by testUsdSchemaGen, but that's not
    # currently possible.
    def test_ModelAPI(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestModelAPI.'+fmt)
            p = s.DefinePrim('/World', 'Xform')
            model = Usd.ModelAPI(p)
            self.assertEqual(model.GetKind(), '')
            self.assertFalse(model.IsModel())
            self.assertFalse(model.IsGroup())

            model.SetKind(Kind.Tokens.group)

            # Testing that initializing a schema from another schema works
            newSchema = Usd.ModelAPI(model)
            self.assertEqual(newSchema.GetKind(), model.GetKind())

if __name__ == '__main__':
    unittest.main()
