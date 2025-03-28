#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys, unittest
from pxr import Sdf, Tf, Usd

class testUsdAttributeAssetPathVariableExpressions(unittest.TestCase):
    def test_Get(self):
        stage = Usd.Stage.CreateInMemory('GetAtDefaultTime.usda')
        stage.GetRootLayer().ImportFromString('''#usda 1.0
            (
                expressionVariables = {
                    string NAME = "assetPathsTest"
                    string NAME5 = "assetPathsTest5"
                    string NAME10 = "assetPathsTest10"
                }
            )

            def "Test" 
            {
                asset a = @`"./${NAME}.usda"`@
                asset a.timeSamples = {
                    5: @`"./${NAME5}.usda"`@,
                    10: @`"./${NAME10}.usda"`@
                }
            }
            ''')

        attr = stage.GetAttributeAtPath("/Test.a")
        self.assertIsNotNone(attr)

        assetPath = attr.Get()
        self.assertEqual(assetPath.authoredPath, '`"./${NAME}.usda"`')
        self.assertEqual(assetPath.evaluatedPath, './assetPathsTest.usda')
        self.assertEqual(assetPath.path, assetPath.evaluatedPath)

        assetPath = attr.Get(5)
        self.assertEqual(assetPath.authoredPath, '`"./${NAME5}.usda"`')
        self.assertEqual(assetPath.evaluatedPath, './assetPathsTest5.usda')
        self.assertEqual(assetPath.path, assetPath.evaluatedPath)

        assetPath = attr.Get(100)
        self.assertEqual(assetPath.authoredPath, '`"./${NAME10}.usda"`')
        self.assertEqual(assetPath.evaluatedPath, "./assetPathsTest10.usda")
        self.assertEqual(assetPath.path, assetPath.evaluatedPath)

    def test_GetArray(self):
        stage = Usd.Stage.CreateInMemory('GetAtDefaultTime.usda')
        stage.GetRootLayer().ImportFromString('''#usda 1.0
            (
                expressionVariables = {
                    string NAME0 = "assetPathsTest0"
                    string NAME1 = "assetPathsTest1"
                    string NAME2 = "assetPathsTest2"
                                              
                    string NAME_5_0 = "assetPathsTest5_0"
                    string NAME_5_1 = "assetPathsTest5_1"
                    string NAME_5_2 = "assetPathsTest5_2"
                                              
                    string NAME_10_0 = "assetPathsTest10_0"
                    string NAME_10_1 = "assetPathsTest10_1"
                    string NAME_10_2 = "assetPathsTest10_2"
                }
            )

            def "Test" 
            {
                asset[] a = [
                        @`"./${NAME0}.usda"`@, 
                        @`"./${NAME1}.usda"`@,
                        @`"./${NAME2}.usda"`@
                ]
                asset[] a.timeSamples = {
                    5: [
                            @`"./${NAME_5_0}.usda"`@,
                            @`"./${NAME_5_1}.usda"`@,
                            @`"./${NAME_5_2}.usda"`@,
                        ],
                    10: [
                            @`"./${NAME_10_0}.usda"`@,
                            @`"./${NAME_10_1}.usda"`@,
                            @`"./${NAME_10_2}.usda"`@,
                        ],
                }
            }
            ''')
        
        attr = stage.GetAttributeAtPath("/Test.a")
        self.assertIsNotNone(attr)

        assetPathArr = attr.Get()
        self.assertEqual(Sdf.AssetPathArray([
            Sdf.AssetPath(authoredPath  = '`"./${NAME0}.usda"`',
                          evaluatedPath = './assetPathsTest0.usda'),
            Sdf.AssetPath(authoredPath  = '`"./${NAME1}.usda"`',
                          evaluatedPath = './assetPathsTest1.usda'),
            Sdf.AssetPath(authoredPath  = '`"./${NAME2}.usda"`',
                          evaluatedPath = './assetPathsTest2.usda')
            ])
            , assetPathArr)
        
        assetPathArr = attr.Get(3)
        self.assertEqual(Sdf.AssetPathArray([
            Sdf.AssetPath(authoredPath  = '`"./${NAME_5_0}.usda"`',
                          evaluatedPath = './assetPathsTest5_0.usda'),
            Sdf.AssetPath(authoredPath  = '`"./${NAME_5_1}.usda"`',
                          evaluatedPath = './assetPathsTest5_1.usda'),
            Sdf.AssetPath(authoredPath  = '`"./${NAME_5_2}.usda"`',
                          evaluatedPath = './assetPathsTest5_2.usda')
            ])
            , assetPathArr)
        
        assetPathArr = attr.Get(10)
        self.assertEqual(Sdf.AssetPathArray([
            Sdf.AssetPath(authoredPath  = '`"./${NAME_10_0}.usda"`',
                          evaluatedPath = './assetPathsTest10_0.usda'),
            Sdf.AssetPath(authoredPath  = '`"./${NAME_10_1}.usda"`',
                          evaluatedPath = './assetPathsTest10_1.usda'),
            Sdf.AssetPath(authoredPath  = '`"./${NAME_10_2}.usda"`',
                          evaluatedPath = './assetPathsTest10_2.usda')
            ])
            , assetPathArr)

        
if __name__ == '__main__':
    unittest.main()
