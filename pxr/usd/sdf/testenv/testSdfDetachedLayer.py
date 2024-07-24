#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os, unittest

from pxr import Plug, Sdf

def DetachedLayerRulesEnvVarIsSet():
    return ('SDF_LAYER_INCLUDE_DETACHED' in os.environ or
            'SDF_LAYER_EXCLUDE_DETACHED' in os.environ)

class TestSdfLayer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register dso plugins.
        testRoot = os.path.join(os.path.dirname(__file__), 'SdfPlugins')
        testPluginsDso = testRoot + '/lib'
        testPluginsDsoSearch = testPluginsDso + '/*/Resources/'
        Plug.Registry().RegisterPlugins(testPluginsDsoSearch)

    def setUp(self):
        if not DetachedLayerRulesEnvVarIsSet():
            Sdf.Layer.SetDetachedLayerRules(Sdf.Layer.DetachedLayerRules())

    @unittest.skipIf(DetachedLayerRulesEnvVarIsSet(), 'rules env var is set')
    def test_DetachedLayerRules(self):
        rules = Sdf.Layer.DetachedLayerRules()
        self.assertFalse(rules.IsIncluded('some_layer.sdf'))
        self.assertFalse(rules.IsIncluded('some_layer_2.sdf'))
        self.assertFalse(rules.IsIncluded('other_layer.sdf'))

        rules.Include(['some_layer'])
        self.assertFalse(rules.IncludedAll())
        self.assertEqual(rules.GetIncluded(), ['some_layer'])
        self.assertEqual(rules.GetExcluded(), [])
        self.assertTrue(rules.IsIncluded('some_layer.sdf'))
        self.assertTrue(rules.IsIncluded('some_layer_2.sdf'))
        self.assertFalse(rules.IsIncluded('other_layer.sdf'))

        rules.Exclude(['some_layer_2'])
        self.assertFalse(rules.IncludedAll())
        self.assertEqual(rules.GetIncluded(), ['some_layer'])
        self.assertEqual(rules.GetExcluded(), ['some_layer_2'])
        self.assertTrue(rules.IsIncluded('some_layer.sdf'))
        self.assertFalse(rules.IsIncluded('some_layer_2.sdf'))
        self.assertFalse(rules.IsIncluded('other_layer.sdf'))

        rules.IncludeAll()
        self.assertTrue(rules.IncludedAll())
        self.assertEqual(rules.GetIncluded(), [])
        self.assertEqual(rules.GetExcluded(), ['some_layer_2'])
        self.assertTrue(rules.IsIncluded('some_layer.sdf'))
        self.assertFalse(rules.IsIncluded('some_layer_2.sdf'))
        self.assertTrue(rules.IsIncluded('other_layer.sdf'))

    @unittest.skipIf(DetachedLayerRulesEnvVarIsSet(), 'rules env var is set')
    def test_SettingRulesAffectsNewLayers(self):
        # Create a test layer to open. We don't care what's inside.
        Sdf.Layer.CreateAnonymous().Export('open_layer.test_streaming_format')

        # Creating a new layer or opening an existing layer that uses our test
        # streaming file format should produce a non-detached layer, since an
        # empty rules is currently set.
        l = Sdf.Layer.CreateNew('new_layer.test_streaming_format')
        self.assertTrue(l)
        self.assertFalse(l.IsDetached())
        del l

        l = Sdf.Layer.FindOrOpen('open_layer.test_streaming_format')
        self.assertTrue(l)
        self.assertFalse(l.IsDetached())
        del l

        # If a new layer that uses our test streaming file format and whose
        # identifier is in the rules is created or opened, we should get a
        # detached layer instead.
        Sdf.Layer.SetDetachedLayerRules(
            Sdf.Layer.DetachedLayerRules().Include(
                ['new_layer.test_streaming_format',
                 'open_layer.test_streaming_format']))

        l = Sdf.Layer.CreateNew('new_layer.test_streaming_format')
        self.assertTrue(l)
        self.assertTrue(l.IsDetached())
        del l

        l = Sdf.Layer.FindOrOpen('open_layer.test_streaming_format')
        self.assertTrue(l)
        self.assertTrue(l.IsDetached())
        del l

    @unittest.skipIf(DetachedLayerRulesEnvVarIsSet(), 'rules env var is set')
    def test_SettingRulesAffectsOpenLayers(self):
        # Create a test layer to open. We don't care what's inside.
        Sdf.Layer.CreateAnonymous().Export('open_layer.test_streaming_format')

        # With an empty rules set, this layer should open as not detached.
        l = Sdf.Layer.FindOrOpen('open_layer.test_streaming_format')
        self.assertTrue(l)
        self.assertFalse(l.IsDetached())

        # Setting rules that includes the layer should reload it so that it
        # is now detached.
        Sdf.Layer.SetDetachedLayerRules(
            Sdf.Layer.DetachedLayerRules().Include(
                ['open_layer.test_streaming_format']))
        self.assertTrue(l.IsDetached())

        # Excluding the layer from the rules should reload it again so that it
        # goes back to non-detached.
        Sdf.Layer.SetDetachedLayerRules(
            Sdf.Layer.DetachedLayerRules()
            .IncludeAll()
            .Exclude(['open_layer.test_streaming_format']))
        self.assertFalse(l.IsDetached())

    @unittest.skipIf(not DetachedLayerRulesEnvVarIsSet(),
                     'rules env var is not set')
    def test_InitializingRulesWithEnvVar(self):
        included = os.environ.get('SDF_LAYER_INCLUDE_DETACHED')
        if included:
            included = included.split(',')
        else:
            included = []

        excluded = os.environ.get('SDF_LAYER_EXCLUDE_DETACHED')
        if excluded:
            excluded = excluded.split(',')
        else:
            excluded = []

        rules = Sdf.Layer.GetDetachedLayerRules()
        
        if '*' in included:
            self.assertTrue(rules.IncludedAll())
            self.assertEqual(rules.GetIncluded(), [])
        else:
            self.assertEqual(set(included), set(rules.GetIncluded()))
        
        self.assertEqual(set(excluded), set(rules.GetExcluded()))

if __name__ == "__main__":
    unittest.main()


