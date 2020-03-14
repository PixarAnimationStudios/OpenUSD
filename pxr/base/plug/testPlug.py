#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
#

import sys, os
import unittest

from pxr import Plug, Tf, Gf

# Test plugins are installed relative to this script
testRoot = os.path.join(os.path.dirname(__file__), 'PlugPlugins')
testPluginsDso = testRoot + '/lib'
testPluginsPython = testRoot + '/lib/python'
testPluginsDsoSearch = testPluginsDso + '/*/Resources/'
testPluginsPythonSearch = testPluginsPython + '/**/'

# Append the sys.path so that we will be able to load python plugins
sys.path.append(testPluginsPython)

# ------------------------------------------------------------
# Helper class for testing notification
# ------------------------------------------------------------

class NoticeListener:
    def __init__(self):
        self.numReceived = 0
        self.newPlugins = []
        self.blocked = False

        self._listener = Tf.Notice.RegisterGlobally(
            "PlugNotice::DidRegisterPlugins", self._OnNotice)

    # Ignore further notices.
    def Block(self):
        self.blocked = True

    def _OnNotice(self, notice, sender):
        if not self.blocked:
            self.numReceived += 1
            self.newPlugins += notice.GetNewPlugins()

class TestPlug(unittest.TestCase):

    # We can't undefine Tf.Types or unregister plugins so we do the
    # registration in class setup and then prevent the notice listeners
    # from responding to further notices so test_Registration() can do
    # its tests.
    @classmethod
    def setUpClass(cls):
        # pre-load some plugins
        import TestPlugModuleLoaded
        import TestPlugModuleLoadedBadBase

        cls.listener1 = NoticeListener()
        cls.listener2 = NoticeListener()

        # Register dso plugins.  Discard possible exception due to TestPlugDsoEmpty.
        # The exception only shows up here if it happens in the main thread so we
        # can't rely on it.
        try:
            Plug.Registry().RegisterPlugins(testPluginsDsoSearch)
        except RuntimeError:
            pass
        cls.listener1.Block()

        # Register python module plugins
        try:
            Plug.Registry().RegisterPlugins(testPluginsPythonSearch)
        except RuntimeError:
            pass
        cls.listener2.Block()

    def test_Registration(self):
        # Verify we received the appropriate notification
        self.assertEqual(self.listener1.numReceived, 1)
        self.assertEqual(set([p.name for p in self.listener1.newPlugins]),
                    set(['TestPlugDso1', 'TestPlugDso2', 'TestPlugDso3', 
                        'TestPlugDsoUnloadable']))

        # Verify we received the appropriate notification
        self.assertEqual(self.listener2.numReceived, 2)
        self.assertEqual(set([p.name for p in self.listener2.newPlugins]),
                    set(['TestPlugDso1', 'TestPlugDso2', 'TestPlugDso3', 
                        'TestPlugDsoUnloadable',
                        'TestPlugModule1', 'TestPlugModule2', 'TestPlugModule3',
                        'TestPlugModuleDepBadBase', 'TestPlugModuleDepBadDep',
                        'TestPlugModuleDepBadDep2', 'TestPlugModuleDepBadLoad',
                        'TestPlugModuleDepCycle', 
                        'TestPlugModuleLoaded', 'TestPlugModuleLoadedBadBase',
                        'TestPlugModuleUnloadable']))

        # Check available subclasses of TestPlugBase<1>
        base1Subclasses = Tf.Type.FindByName('_TestPlugBase<1>').GetAllDerivedTypes()
        base1SubclassesExpected = \
            ('_TestPlugDerived0', 'TestPlugDerived1',
            'TestPlugModule1.TestPlugPythonDerived1',
            'TestPlugModuleLoaded.TestPlugPythonLoaded',
            'TestPlugModuleLoadedBadBase.TestPlugPythonLoadedBadBase',
            'TestPlugUnloadable', 'TestPlugPythonUnloadable')
        for sc in base1SubclassesExpected:
            self.assertIn(sc, base1Subclasses)
        self.assertEqual(len(base1Subclasses), len(base1SubclassesExpected))

        # Check available subclasses of _TestPlugBase<2>
        base2Subclasses = Tf.Type.FindByName('_TestPlugBase<2>').GetAllDerivedTypes()
        base2SubclassesExpected = ('TestPlugDerived2',
                                   'TestPlugModule2.TestPlugPythonDerived2')
        for sc in base2SubclassesExpected:
            self.assertIn(sc, base2Subclasses)
        self.assertEqual(len(base2Subclasses), len(base2SubclassesExpected))

        allPlugins = Plug.Registry().GetAllPlugins()
        self.assertTrue(len(allPlugins) >= 8)

    def test_ManufacturingCppDerivedClasses(self):
        # Construct and verify an instance of _TestPlugBase<1>
        tb1 = Plug._TestPlugBase1()
        self.assertEqual(tb1.GetTypeName(), '_TestPlugBase<1>')

        # Get the plugin for TestPlugDerived1
        pd1 = Plug.Registry().GetPluginForType('TestPlugDerived1')
        self.assertIsNotNone(pd1)
        self.assertFalse(pd1.isLoaded)

        # Manufacture and verify an instance of TestPlugDerived1
        td1 = Plug._TestPlugBase1('TestPlugDerived1')
        self.assertTrue(td1 and not td1.expired)
        self.assertEqual(td1.GetTypeName(), 'TestPlugDerived1')
        self.assertTrue(pd1.isLoaded)

        # Construct and verify an instance of _TestPlugBase<2>
        tb2 = Plug._TestPlugBase2()
        self.assertEqual(tb2.GetTypeName(), '_TestPlugBase<2>')

        # Get the plugin for TestPlugDerived2
        pd2 = Plug.Registry().GetPluginForType('TestPlugDerived2')
        self.assertIsNotNone(pd2)
        self.assertFalse(pd2.isLoaded)

        # Manufacture and verify an instance of TestPlugDerived2
        td2 = Plug._TestPlugBase2('TestPlugDerived2')
        self.assertTrue(td2 and not td2.expired)
        self.assertEqual(td2.GetTypeName(), 'TestPlugDerived2')
        self.assertTrue(pd2.isLoaded)

        # Check that plugin correctly reports its declared types.
        self.assertFalse(pd1.DeclaresType(Tf.Type('_TestPlugBase<1>'), includeSubclasses=False))
        self.assertTrue(pd1.DeclaresType(Tf.Type('_TestPlugBase<1>'), includeSubclasses=True))
        self.assertTrue(pd1.DeclaresType(Tf.Type('TestPlugDerived1'), includeSubclasses=False))
        self.assertTrue(pd1.DeclaresType(Tf.Type('TestPlugDerived1'), includeSubclasses=True))

    def test_ManufacturingPythonDerivedClasses(self):
        ppd1 = Plug.Registry().GetPluginForType('TestPlugModule1.TestPlugPythonDerived1')
        self.assertIsNotNone(ppd1)
        self.assertFalse(ppd1.isLoaded)
        self.assertTrue(ppd1.isPythonModule)

        ppd2 = Plug.Registry().GetPluginForType('TestPlugModule2.TestPlugPythonDerived2')
        self.assertIsNotNone(ppd2)
        self.assertFalse(ppd2.isLoaded)
        self.assertTrue(ppd2.isPythonModule)

        # Load and construct an instance of TestPlugPythonDerived1
        # (Loading should add TestPlugModule1 to the namespace)
        ppd1.Load()
        tpd1 = TestPlugModule1.TestPlugPythonDerived1()
        self.assertEqual(tpd1.GetTypeName(), 'TestPlugModule1.TestPlugPythonDerived1')
        self.assertTrue(ppd1.isLoaded)

        # Load and construct an instance of TestPlugPythonDerived2
        # (Loading should add TestPlugModule2 to the namespace)
        ppd2.Load()
        tpd2 = TestPlugModule2.TestPlugPythonDerived2()
        self.assertEqual(tpd2.GetTypeName(), 'TestPlugModule2.TestPlugPythonDerived2')
        self.assertTrue(ppd2.isLoaded)

        # Check that loading an already loaded plugin is a noop.
        ppd2.Load()

        # Check that plugin correctly reports its declared types.
        self.assertFalse(ppd1.DeclaresType(Tf.Type('_TestPlugBase<1>'), 
                                    includeSubclasses=False))
        self.assertTrue(ppd1.DeclaresType(Tf.Type('_TestPlugBase<1>'), 
                                includeSubclasses=True))
        self.assertTrue(ppd1.DeclaresType(Tf.Type('TestPlugModule1.TestPlugPythonDerived1'), 
                                includeSubclasses=False))
        self.assertTrue(ppd1.DeclaresType(Tf.Type('TestPlugModule1.TestPlugPythonDerived1'), 
                                includeSubclasses=True))

    def test_LoadingPluginDependencies(self):
        # Get the plugin for a subclass
        pd3 = Plug.Registry().GetPluginForType('TestPlugDerived3_3')
        self.assertIsNotNone(pd3)
        self.assertFalse(pd3.isLoaded)

        # Get the plugin for one of its subclass dependencies
        ppd3 = Plug.Registry().GetPluginForType('TestPlugModule3.TestPlugPythonDerived3_3')
        self.assertIsNotNone(ppd3)
        self.assertFalse(ppd3.isLoaded)

        # Load the plugin using the PlugRegistry
        Plug.Registry().GetPluginForType('TestPlugDerived3_3').Load()

        # Check that both the plugin and its dependency were loaded
        self.assertTrue(pd3.isLoaded)
        self.assertTrue(ppd3.isLoaded)

    def test_MetadataAccess(self):
        base1Subclasses = Tf.Type.FindByName('_TestPlugBase<1>').derivedTypes
        self.assertIn('TestPlugUnloadable', base1Subclasses)
        plugin = Plug.Registry().GetPluginForType('TestPlugUnloadable')
        self.assertIsNotNone(plugin)
        metadata = plugin.metadata
        self.assertIsNotNone(metadata)
        self.assertTrue('Types' in metadata)
        self.assertTrue('TestPlugUnloadable' in metadata['Types'])

        md = metadata['Types']['TestPlugUnloadable']

        self.assertTrue(md == 
            plugin.GetMetadataForType(Tf.Type.FindByName('TestPlugUnloadable')))

        self.assertTrue('bases' in md)
        self.assertTrue(md['bases'] == ['_TestPlugBase<1>'])
        self.assertTrue('description' in md)
        self.assertTrue(md['description'] == 'unloadable plugin')
        self.assertTrue('notLoadable' in md)
        self.assertTrue(md['notLoadable'] == True)
        self.assertTrue('vectorInt' in md)
        self.assertTrue(md['vectorInt'] == [ 1, 2, 3 ])
        self.assertTrue('vectorString' in md)
        self.assertTrue(md['vectorString'] == [ "f", "l", "o" ])
        self.assertTrue('vectorDouble' in md)
        self.assertTrue(md['vectorDouble'] == [ 1.1, 2.2, 3.3 ])
        self.assertTrue('Int' in md)
        self.assertTrue(md['Int'] == 4711)
        self.assertTrue('Double' in md)
        self.assertTrue(Gf.IsClose(md['Double'], 0.815, 1e-6))


    def test_ErrorCases(self):
        allplugins = Plug.Registry().GetAllPlugins()

        listener = NoticeListener()

        # try to find a plugin for an unknown type
        with self.assertRaises(TypeError):
            badPlugin = Plug.Registry().GetPluginForType('BadSubclass')

        # try to register a non-existent plugin path
        self.assertFalse(os.path.exists('/nowhere'))
        Plug.Registry().RegisterPlugins('/nowhere')
        self.assertEqual(listener.numReceived, 0)

        # try to register an incomplete plugin
        badPluginPath = testPluginsDso + '/TestPlugDsoIncomplete.framework/Resources/'
        Plug.Registry().RegisterPlugins(badPluginPath)
        self.assertEqual(listener.numReceived, 0)

        # try to register an incomplete python plugin path
        badPluginPath = testPluginsPython + '/TestPlugModuleIncomplete'
        Plug.Registry().RegisterPlugins(badPluginPath)
        self.assertEqual(listener.numReceived, 0)

        # try to find a plugin for a Tf type which doesn't exist
        unknownPlug = None
        with self.assertRaises(RuntimeError):
            unknownPlug = Plug.Registry().GetPluginForType(Tf.Type.FindByName('Bad'))
        self.assertFalse(unknownPlug)
        self.assertEqual(listener.numReceived, 0)

        # try to load an unloadable plugin
        badPlugin = Plug.Registry().GetPluginForType('TestPlugUnloadable')
        self.assertIsNotNone(badPlugin)
        with self.assertRaises(RuntimeError):
            badPlugin.Load()

        # try to load an unloadable plugin python module
        badPlugin = Plug.Registry().GetPluginForType('TestPlugPythonUnloadable')
        self.assertIsNotNone(badPlugin)
        with self.assertRaises(RuntimeError):
            badPlugin.Load()

        # try to load a plugin with an unknown base dependency
        badPlugin = Plug.Registry().GetPluginForType('TestPlugPythonDepBadBase')
        self.assertTrue(badPlugin)
        with self.assertRaises(RuntimeError):
            badPlugin.Load()

        # try to load a plugin with an unknown dependent dependency
        badPlugin = Plug.Registry().GetPluginForType('TestPlugPythonDepBadDep')
        self.assertTrue(badPlugin)
        with self.assertRaises(RuntimeError):
            badPlugin.Load()

        # try to load a plugin with a bad dependent dependency
        badPlugin = Plug.Registry().GetPluginForType('TestPlugPythonDepBadDep2')
        self.assertTrue(badPlugin)
        with self.assertRaises(RuntimeError):
            badPlugin.Load()

        # try to load a plugin with an unloadable dependent dependency
        badPlugin = Plug.Registry().GetPluginForType('TestPlugPythonDepBadLoad')
        self.assertTrue(badPlugin)
        with self.assertRaises(RuntimeError):
            badPlugin.Load()

        # try to load a plugin with a cyclic dependency
        badPlugin = Plug.Registry().GetPluginForType('TestPlugPythonDepCycle')
        self.assertTrue(badPlugin)
        with self.assertRaises(RuntimeError):
            badPlugin.Load()

        # Try registering plugins again, this should be a no-op.
        Plug.Registry().RegisterPlugins(testPluginsDsoSearch)
        Plug.Registry().RegisterPlugins(testPluginsPythonSearch)
        self.assertEqual(allplugins, Plug.Registry().GetAllPlugins())
        self.assertEqual(listener.numReceived, 0)

    def test_DebugCodeExistence(self):
        debugCodes = Tf.Debug.GetDebugSymbolNames()
        self.assertTrue('PLUG_INFO_SEARCH' in debugCodes)
        self.assertTrue('PLUG_REGISTRATION' in debugCodes)

if __name__ == '__main__':
    unittest.main()
