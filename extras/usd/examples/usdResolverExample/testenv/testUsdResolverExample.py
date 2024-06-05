#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import json
import os
import shutil
import unittest

from pxr import Ar, Plug, Tf, Usd, UsdResolverExample

class TestUsdResolverExample(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # plugInfo.json file for the example resolver should be installed
        # in the test directory.
        Plug.Registry().RegisterPlugins(os.getcwd())

    @staticmethod
    def writeVersionsDict(filename, d):
        with open(filename, "w") as f:
            json.dump(d, f, indent=4)

    def test_Resolve(self):
        resolver = Ar.GetResolver()

        self.assertEqual(
            resolver.Resolve("asset:Buzz/Buzz.usd"), 
            Ar.ResolvedPath("asset:Buzz/Buzz.usd"))

        self.assertEqual(
            resolver.Resolve("asset:DoesntExist/DoesntExist.usd"),
            Ar.ResolvedPath())

    def test_ResolveWithContext(self):
        resolver = Ar.GetResolver()

        self.writeVersionsDict(
            "resolveWithContext.json",
            {
                "Buzz" : "1"
            })

        ctx = UsdResolverExample.ResolverContext("resolveWithContext.json")
        with Ar.ResolverContextBinder(ctx):
            self.assertEqual(
                resolver.Resolve("asset:Buzz/{$VERSION}/Buzz.usd"),
                Ar.ResolvedPath("asset:Buzz/1/Buzz.usd"))

            self.assertEqual(
                resolver.Resolve("asset:Woody/{$VERSION}/Woody.usd"),
                Ar.ResolvedPath("asset:Woody/latest/Woody.usd"))

    def test_CreateContextFromString(self):
        resolver = Ar.GetResolver()

        self.writeVersionsDict(
            "createContextFromString.json",
            {
                "Buzz" : "1"
            })

        ctx = resolver.CreateContextFromString(
            "asset", "createContextFromString.json")

        self.assertEqual(
            ctx, 
            UsdResolverExample.ResolverContext("createContextFromString.json"))

        with Ar.ResolverContextBinder(ctx):
            self.assertEqual(
                resolver.Resolve("asset:Buzz/{$VERSION}/Buzz.usd"),
                Ar.ResolvedPath("asset:Buzz/1/Buzz.usd"))

            self.assertEqual(
                resolver.Resolve("asset:Woody/{$VERSION}/Woody.usd"),
                Ar.ResolvedPath("asset:Woody/latest/Woody.usd"))

    def test_RefreshContext(self):
        resolver = Ar.GetResolver()

        self.writeVersionsDict(
            "refreshContext.json",
            {
                "Buzz" : "1"
            })

        ctx = UsdResolverExample.ResolverContext("refreshContext.json")
        with Ar.ResolverContextBinder(ctx):
            self.assertEqual(
                resolver.Resolve("asset:Buzz/{$VERSION}/Buzz.usd"),
                Ar.ResolvedPath("asset:Buzz/1/Buzz.usd"))

        self.writeVersionsDict(
            "refreshContext.json",
            {
                "Buzz" : "latest"
            })
            
        with Ar.ResolverContextBinder(ctx):
            self.assertEqual(
                resolver.Resolve("asset:Buzz/{$VERSION}/Buzz.usd"),
                Ar.ResolvedPath("asset:Buzz/1/Buzz.usd"))
        
        class _Listener(object):
            def __init__(self):
                self._key = Tf.Notice.RegisterGlobally(
                    Ar.Notice.ResolverChanged, self._HandleNotice)
                self.receivedNotice = False

            def _HandleNotice(self, notice, sender):
                self.receivedNotice = True

        l = _Listener()
        resolver.RefreshContext(ctx)
        self.assertTrue(l.receivedNotice)

        with Ar.ResolverContextBinder(ctx):
            self.assertEqual(
                resolver.Resolve("asset:Buzz/{$VERSION}/Buzz.usd"),
                Ar.ResolvedPath("asset:Buzz/latest/Buzz.usd"))

    def test_RefreshContextUsdStage(self):
        self.writeVersionsDict("refreshContextUsdStage.json", {})

        stage = Usd.Stage.Open(
            'shots/shot_1/shot_1.usda',
            UsdResolverExample.ResolverContext("refreshContextUsdStage.json"))

        self.assertEqual(
            stage.GetPrimAtPath('/World/chars/Buzz').GetAssetInfo(),
            { 'version' : '2'})

        self.assertEqual(
            stage.GetPrimAtPath('/World/chars/Woody').GetAssetInfo(),
            { 'version' : '2'})

        self.writeVersionsDict(
            "refreshContextUsdStage.json",
            {
                "Buzz" : "1"
            })

        stage.Reload()

        self.assertEqual(
            stage.GetPrimAtPath('/World/chars/Buzz').GetAssetInfo(),
            { 'version' : '1'})

        self.assertEqual(
            stage.GetPrimAtPath('/World/chars/Woody').GetAssetInfo(),
            { 'version' : '2'})

if __name__ == '__main__':
    unittest.main()

