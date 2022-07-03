#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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

