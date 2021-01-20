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

import sys, unittest
from pxr import Sdf, Tf, Usd

allFormats = ['usd' + x for x in 'ac']

def _CreateStage(fmt):
    s = Usd.Stage.CreateInMemory('_CreateStage.'+fmt)
    s.GetRootLayer().ImportFromString('''#usda 1.0
        def Scope "Foo"
        {
            custom int someAttr
            add rel testRel = [
                </Qux>,
                </Bar>,
                </Baz>,
                </Foo.someAttr>,
            ]
            add rel testRelBug138452 = </Bug138452>
        }

        def Scope "Bar"
        {
            add rel cycle = </Bar.fwd>
            add rel fwd = [
                </Baz>,
                </Foo.testRel>,
                </Qux>,
                </Bar.cycle>,
            ]
            add rel fwd2 = [
                </Bar.fwd2a>,
                </Bar.fwd2b>,
                </Bar.fwd2c>,
            ]
            add rel fwd2a = </Qux>
            add rel fwd2b = </Baz>
            add rel fwd2c = </Bar>
        }

        def Scope "Baz"
        {
            add rel bogus = </MissingTargetPath>
        }

        def Scope "Qux"
        {
        }

        def Scope "Bug138452"
        {
            custom rel Bug138452
            add rel Bug138452 = </Qux>
        }

        def "Recursive" {
            def "A" { custom rel AtoB = <../B>
            }
            def "B" { custom rel BtoC = <../C>
            }
            def "C" { custom rel CtoD = <../D>
            }
            def "D" { custom rel DtoA = <../A>
                def "A" { custom rel AtoB = <../B>
                }
                def "B" { custom rel BtoC = <../C>
                }
                def "C" { custom rel CtoD = <../D>
                }
                def "D" { custom rel DtoA = <../A>
                }
            }
        }
        ''')

    return s

class TestUsdRelationships(unittest.TestCase):
    def test_Targets(self):
        for fmt in allFormats:
            stage = _CreateStage(fmt)

            # Simple target list with correct order
            r = stage.GetPrimAtPath("/Foo").GetRelationship("testRel")
            sol = [Sdf.Path(path) for path in ['/Qux', '/Bar', '/Baz', '/Foo.someAttr']]
            self.assertEqual(r.GetTargets(), sol) 

            # Forwarded targets
            r = stage.GetPrimAtPath("/Bar").GetRelationship("fwd")
            sol = [Sdf.Path(path) for path in ['/Baz', '/Qux', '/Bar', '/Foo.someAttr']]
            self.assertEqual(r.GetForwardedTargets(), sol)

            # Forwarded targets
            r = stage.GetPrimAtPath("/Bar").GetRelationship("fwd2")
            sol = [Sdf.Path(path) for path in ['/Qux', '/Baz', '/Bar']]
            self.assertEqual(r.GetForwardedTargets(), sol)

            # Forwarded targets, bug 138452.  With that bug, the forwarded targets
            # would be ['/Qux']
            r = stage.GetPrimAtPath('/Foo').GetRelationship('testRelBug138452')
            self.assertEqual(r.GetForwardedTargets(), ['/Bug138452'])

            # Cycle detection
            r = stage.GetPrimAtPath("/Bar").GetRelationship("cycle")
            sol = [Sdf.Path(path) for path in ['/Baz', '/Qux', '/Bar', '/Foo.someAttr']]
            self.assertEqual(r.GetForwardedTargets(), sol)

            # Bogus target path
            r = stage.GetPrimAtPath("/Baz").GetRelationship("bogus")
            sol = [Sdf.Path('/MissingTargetPath')]
            self.assertEqual(r.GetForwardedTargets(), sol)

            # Recursive finding
            recursive = stage.GetPrimAtPath("/Recursive")
            self.assertEqual(
                set(recursive.FindAllRelationshipTargetPaths()),
                set([Sdf.Path('/Recursive/A'), Sdf.Path('/Recursive/B'),
                     Sdf.Path('/Recursive/C'), Sdf.Path('/Recursive/D'),
                     Sdf.Path('/Recursive/D/A'), Sdf.Path('/Recursive/D/B'),
                     Sdf.Path('/Recursive/D/C'), Sdf.Path('/Recursive/D/D')]))

            self.assertEqual(
                set(recursive.FindAllRelationshipTargetPaths(
                    predicate = lambda rel: rel.GetPrim().GetName() in ('B', 'D'))),
                set([Sdf.Path('/Recursive/A'), Sdf.Path('/Recursive/C'),
                     Sdf.Path('/Recursive/D/A'), Sdf.Path('/Recursive/D/C')]))

            self.assertEqual(
                set(recursive.FindAllRelationshipTargetPaths(
                    predicate = lambda rel: rel.GetPrim().GetName() in ('A', 'C'))),
                set([Sdf.Path('/Recursive/B'), Sdf.Path('/Recursive/D'),
                     Sdf.Path('/Recursive/D/B'), Sdf.Path('/Recursive/D/D')]))
                
            recursiveA = stage.GetPrimAtPath("/Recursive/A")
            self.assertEqual(set(recursiveA.FindAllRelationshipTargetPaths()),
                        set([Sdf.Path('/Recursive/B')]))
            
            self.assertEqual(set(
                recursiveA.FindAllRelationshipTargetPaths(recurseOnTargets=True)),
                set([Sdf.Path('/Recursive/A'), Sdf.Path('/Recursive/B'),
                     Sdf.Path('/Recursive/C'), Sdf.Path('/Recursive/D'),
                     Sdf.Path('/Recursive/D/A'), Sdf.Path('/Recursive/D/B'),
                     Sdf.Path('/Recursive/D/C'), Sdf.Path('/Recursive/D/D')]))

            self.assertEqual(set(
                recursiveA.FindAllRelationshipTargetPaths(
                    recurseOnTargets=True,
                    predicate=lambda rel: rel.GetPrim().GetParent().GetName() ==
                    'Recursive' or rel.GetPrim().GetName() in ('A', 'C'))),
                set([Sdf.Path('/Recursive/A'), Sdf.Path('/Recursive/B'),
                     Sdf.Path('/Recursive/C'), Sdf.Path('/Recursive/D'),
                     Sdf.Path('/Recursive/D/B'), Sdf.Path('/Recursive/D/D')]))

    def test_TargetsInInstances(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestTargetsInInstances.'+fmt)
            s.GetRootLayer().ImportFromString('''#usda 1.0
            def Scope "Ref"
            {
                def Scope "Foo"
                {
                    custom int someAttr
                    add rel testRel = [
                        </Ref/Qux>,
                        </Ref/Bar>,
                        </Ref/Baz>,
                        </Ref/Foo.someAttr>,
                    ]
                }

                def Scope "Bar"
                {
                    add rel cycle = </Ref/Bar.fwd>
                    add rel fwd = [
                        </Ref/Baz>,
                        </Ref/Foo.testRel>,
                        </Ref/Qux>,
                        </Ref/Bar.cycle>,
                    ]
                    add rel fwd2 = [
                        </Ref/Bar.fwd2a>,
                        </Ref/Bar.fwd2b>,
                        </Ref/Bar.fwd2c>,
                    ]
                    add rel fwd2a = </Ref/Qux>
                    add rel fwd2b = </Ref/Baz>
                    add rel fwd2c = </Ref/Bar>
                }

                def Scope "Baz"
                {
                    add rel bogus = </Ref/MissingTargetPath>
                    add rel root = </Ref>
                }

                def Scope "Qux"
                {
                }
            }

            def Scope "Root" (
                instanceable = true
                references = </Ref>
            )
            {
            }
            ''')

            prototype = s.GetPrimAtPath('/Root').GetPrototype()
            self.assertTrue(prototype)

            # Simple target list with correct order
            r = prototype.GetChild("Foo").GetRelationship("testRel")
            sol = [prototype.GetPath().AppendChild('Qux'), 
                   prototype.GetPath().AppendChild('Bar'), 
                   prototype.GetPath().AppendChild('Baz'), 
                   prototype.GetPath().AppendPath(Sdf.Path('Foo.someAttr'))]
            self.assertEqual(r.GetTargets(), sol) 

            # Forwarded targets
            r = prototype.GetChild("Bar").GetRelationship("fwd")
            sol = [prototype.GetPath().AppendChild('Baz'), 
                   prototype.GetPath().AppendChild('Qux'), 
                   prototype.GetPath().AppendChild('Bar'), 
                   prototype.GetPath().AppendPath(Sdf.Path('Foo.someAttr'))]
            self.assertEqual(r.GetForwardedTargets(), sol)

            # Forwarded targets
            r = prototype.GetChild("Bar").GetRelationship("fwd2")
            sol = [prototype.GetPath().AppendChild('Qux'), 
                   prototype.GetPath().AppendChild('Baz'), 
                   prototype.GetPath().AppendChild('Bar')]
            self.assertEqual(r.GetForwardedTargets(), sol)

            # Cycle detection
            r = prototype.GetChild("Bar").GetRelationship("cycle")
            sol = [prototype.GetPath().AppendChild('Baz'), 
                   prototype.GetPath().AppendChild('Qux'), 
                   prototype.GetPath().AppendChild('Bar'), 
                   prototype.GetPath().AppendPath(Sdf.Path('Foo.someAttr'))]
            self.assertEqual(r.GetForwardedTargets(), sol)

            # Bogus target path
            r = prototype.GetChild("Baz").GetRelationship("bogus")
            sol = [prototype.GetPath().AppendChild("MissingTargetPath")]
            self.assertEqual(r.GetTargets(), sol)
            self.assertEqual(r.GetForwardedTargets(), sol)

            # Path inside an instance that points to the instance root
            r = prototype.GetChild("Baz").GetRelationship("root")
            sol = [prototype.GetPath()]
            self.assertEqual(r.GetTargets(), sol)
            self.assertEqual(r.GetForwardedTargets(), sol)

    def test_TargetsToObjectsInInstances(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory(
                'TestTargetsToObjectsInInstances.'+fmt)
            stage.GetRootLayer().ImportFromString('''#usda 1.0
            def "Instance"
            {
                double attr = 1.0

                def "A"
                {
                    double attr = 1.0
                    rel rel = [ 
                        </Instance>,
                        </Instance.attr>,
                        </Instance/A>,
                        </Instance/A.attr>,
                        </Instance/NestedInstance_1>,
                        </Instance/NestedInstance_1.attr>,
                        </Instance/NestedInstance_1/B>,
                        </Instance/NestedInstance_1/B.attr>,
                        </Instance/NestedInstance_2>,
                        </Instance/NestedInstance_2.attr>,
                        </Instance/NestedInstance_2/B>,
                        </Instance/NestedInstance_2/B.attr>
                    ]
                }

                def "NestedInstance_1" (
                    instanceable = true
                    references = </NestedInstance>
                )
                {
                }

                def "NestedInstance_2" (
                    instanceable = true
                    references = </NestedInstance>
                )
                {
                }
            }

            def "NestedInstance"
            {
                double attr = 1.0
                def "B"
                {
                    double attr = 1.0
                }
            }

            def "Root"
            {
                rel fwdRel = [
                    </Root/Instance_1/A.rel>,
                    </Root/Instance_2/A.rel>
                ]

                rel rel = [ 
                    </Root/Instance_1>,
                    </Root/Instance_1.attr>,
                    </Root/Instance_1/A>,
                    </Root/Instance_1/A.attr>,
                    </Root/Instance_1/NestedInstance_1>,
                    </Root/Instance_1/NestedInstance_1.attr>,
                    </Root/Instance_1/NestedInstance_1/B>,
                    </Root/Instance_1/NestedInstance_1/B.attr>,
                    </Root/Instance_1/NestedInstance_2>,
                    </Root/Instance_1/NestedInstance_2.attr>,
                    </Root/Instance_1/NestedInstance_2/B>,
                    </Root/Instance_1/NestedInstance_2/B.attr>,
                    </Root/Instance_2>,
                    </Root/Instance_2.attr>,
                    </Root/Instance_2/A>,
                    </Root/Instance_2/A.attr>,
                    </Root/Instance_2/NestedInstance_1>,
                    </Root/Instance_2/NestedInstance_1.attr>,
                    </Root/Instance_2/NestedInstance_1/B>,
                    </Root/Instance_2/NestedInstance_1/B.attr>,
                    </Root/Instance_2/NestedInstance_2>,
                    </Root/Instance_2/NestedInstance_2.attr>,
                    </Root/Instance_2/NestedInstance_2/B>,
                    </Root/Instance_2/NestedInstance_2/B.attr>
                ]

                def "Instance_1" (
                    instanceable = true
                    references = </Instance>
                )
                {
                    rel fwdRel = [
                        </Root/Instance_1/A.rel>,
                        </Root/Instance_2/A.rel>
                    ]

                    rel rel = [ 
                        </Root/Instance_1>,
                        </Root/Instance_1.attr>,
                        </Root/Instance_1/A>,
                        </Root/Instance_1/A.attr>,
                        </Root/Instance_1/NestedInstance_1>,
                        </Root/Instance_1/NestedInstance_1.attr>,
                        </Root/Instance_1/NestedInstance_1/B>,
                        </Root/Instance_1/NestedInstance_1/B.attr>,
                        </Root/Instance_1/NestedInstance_2>,
                        </Root/Instance_1/NestedInstance_2.attr>,
                        </Root/Instance_1/NestedInstance_2/B>,
                        </Root/Instance_1/NestedInstance_2/B.attr>,
                        </Root/Instance_2>,
                        </Root/Instance_2.attr>,
                        </Root/Instance_2/A>,
                        </Root/Instance_2/A.attr>,
                        </Root/Instance_2/NestedInstance_1>,
                        </Root/Instance_2/NestedInstance_1.attr>,
                        </Root/Instance_2/NestedInstance_1/B>,
                        </Root/Instance_2/NestedInstance_1/B.attr>,
                        </Root/Instance_2/NestedInstance_2>,
                        </Root/Instance_2/NestedInstance_2.attr>,
                        </Root/Instance_2/NestedInstance_2/B>,
                        </Root/Instance_2/NestedInstance_2/B.attr>
                    ]
                }

                def "Instance_2" (
                    instanceable = true
                    references = </Instance>
                )
                {
                }
            }
            ''')

            prototype = stage.GetPrimAtPath("/Root/Instance_1").GetPrototype()
            nestedPrototype = \
                prototype.GetChild("NestedInstance_1").GetPrototype()

            # Test retrieving relationship targets that point to instances and
            # prims within instances.
            def _TestRelationship(rel):
                self.assertTrue(rel)

                # Targets to objects in prototypes cannot be authored.
                primInPrototypePath = prototype.GetPath().AppendChild("A")
                with self.assertRaises(Tf.ErrorException):
                    self.assertFalse(rel.AddTarget(primInPrototypePath))
                with self.assertRaises(Tf.ErrorException):
                    self.assertFalse(rel.RemoveTarget(primInPrototypePath))
                with self.assertRaises(Tf.ErrorException):
                    self.assertFalse(rel.SetTargets(
                        ["/Root/Instance_1", primInPrototypePath]))

                targets = rel.GetTargets()
                expected = [
                    Sdf.Path("/Root/Instance_1"),
                    Sdf.Path("/Root/Instance_1.attr"),
                    Sdf.Path("/Root/Instance_1/A"),
                    Sdf.Path("/Root/Instance_1/A.attr"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_1"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_1.attr"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_1/B"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_1/B.attr"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_2"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_2.attr"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_2/B"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_2/B.attr"),
                    Sdf.Path("/Root/Instance_2"),
                    Sdf.Path("/Root/Instance_2.attr"),
                    Sdf.Path("/Root/Instance_2/A"),
                    Sdf.Path("/Root/Instance_2/A.attr"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_1"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_1.attr"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_1/B"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_1/B.attr"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_2"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_2.attr"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_2/B"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_2/B.attr")]
                self.assertEqual(targets, expected)

            rel = stage.GetPrimAtPath("/Root").GetRelationship("rel")
            _TestRelationship(rel)

            rel = stage.GetPrimAtPath("/Root/Instance_1").GetRelationship("rel")
            _TestRelationship(rel)

            def _TestRelationshipInPrototype(rel):
                self.assertTrue(rel)
                self.assertTrue(rel.GetPrim().IsInPrototype())

                targets = rel.GetTargets()
                prototypePath = prototype.GetPath()
                expected = [
                    prototypePath,
                    prototypePath.AppendPath(".attr"),
                    prototypePath.AppendPath("A"),
                    prototypePath.AppendPath("A.attr"),
                    prototypePath.AppendPath("NestedInstance_1"),
                    prototypePath.AppendPath("NestedInstance_1.attr"),
                    prototypePath.AppendPath("NestedInstance_1/B"),
                    prototypePath.AppendPath("NestedInstance_1/B.attr"),
                    prototypePath.AppendPath("NestedInstance_2"),
                    prototypePath.AppendPath("NestedInstance_2.attr"),
                    prototypePath.AppendPath("NestedInstance_2/B"),
                    prototypePath.AppendPath("NestedInstance_2/B.attr")]
                self.assertEqual(targets, expected)

            rel = prototype.GetChild("A").GetRelationship("rel")
            _TestRelationshipInPrototype(rel)

            def _TestRelationshipForwarding(rel):
                self.assertTrue(rel)

                # Expect warning here due to targets authored inside an instance
                # that point to the instance root.
                targets = rel.GetForwardedTargets()
                expected = [
                    Sdf.Path("/Root/Instance_1"),
                    Sdf.Path("/Root/Instance_1.attr"),
                    Sdf.Path("/Root/Instance_1/A"),
                    Sdf.Path("/Root/Instance_1/A.attr"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_1"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_1.attr"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_1/B"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_1/B.attr"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_2"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_2.attr"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_2/B"),
                    Sdf.Path("/Root/Instance_1/NestedInstance_2/B.attr"),
                    Sdf.Path("/Root/Instance_2"),
                    Sdf.Path("/Root/Instance_2.attr"),
                    Sdf.Path("/Root/Instance_2/A"),
                    Sdf.Path("/Root/Instance_2/A.attr"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_1"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_1.attr"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_1/B"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_1/B.attr"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_2"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_2.attr"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_2/B"),
                    Sdf.Path("/Root/Instance_2/NestedInstance_2/B.attr")
                ]
                self.assertEqual(targets, expected)

            rel = stage.GetPrimAtPath("/Root").GetRelationship("fwdRel")
            _TestRelationshipForwarding(rel)

            rel = stage.GetPrimAtPath("/Root/Instance_1").GetRelationship("fwdRel")
            _TestRelationshipForwarding(rel)

    def test_AuthoringTargets(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory("testAuthoringTargets." + fmt)

            prim = stage.DefinePrim("/Test")
            rel = prim.CreateRelationship("rel")
            relSpec = stage.GetEditTarget().GetPropertySpecForScenePath(
                rel.GetPath())

            rel.SetTargets(["/Test/A", "/Test/B"])
            self.assertEqual(rel.GetTargets(), ["/Test/A", "/Test/B"])

            expectedListOp = Sdf.PathListOp()
            expectedListOp.explicitItems = ["/Test/A", "/Test/B"]
            self.assertEqual(relSpec.GetInfo("targetPaths"), expectedListOp)

            rel.AddTarget("/Test/C")
            self.assertEqual(rel.GetTargets(), 
                             ["/Test/A", "/Test/B", "/Test/C"])

            expectedListOp = Sdf.PathListOp()
            expectedListOp.explicitItems = ["/Test/A", "/Test/B", "/Test/C"]
            self.assertEqual(relSpec.GetInfo("targetPaths"), expectedListOp)

            rel.ClearTargets(removeSpec=False)
            self.assertEqual(rel.GetTargets(), [])

            expectedListOp = Sdf.PathListOp()
            self.assertEqual(relSpec.GetInfo("targetPaths"), expectedListOp)

            rel.AddTarget("/Test/A", Usd.ListPositionFrontOfPrependList)
            self.assertEqual(rel.GetTargets(), ["/Test/A"])

            expectedListOp = Sdf.PathListOp()
            expectedListOp.prependedItems = ["/Test/A"]
            self.assertEqual(relSpec.GetInfo("targetPaths"), expectedListOp)

            rel.AddTarget("/Test/B", Usd.ListPositionBackOfPrependList)
            self.assertEqual(rel.GetTargets(), ["/Test/A", "/Test/B"])

            expectedListOp = Sdf.PathListOp()
            expectedListOp.prependedItems = ["/Test/A", "/Test/B"]
            self.assertEqual(relSpec.GetInfo("targetPaths"), expectedListOp)

            rel.AddTarget("/Test/C", Usd.ListPositionFrontOfAppendList)
            self.assertEqual(rel.GetTargets(), 
                             ["/Test/A", "/Test/B", "/Test/C"])

            expectedListOp = Sdf.PathListOp()
            expectedListOp.prependedItems = ["/Test/A", "/Test/B"]
            expectedListOp.appendedItems = ["/Test/C"]
            self.assertEqual(relSpec.GetInfo("targetPaths"), expectedListOp)

            rel.AddTarget("/Test/D", Usd.ListPositionBackOfAppendList)
            self.assertEqual(rel.GetTargets(), 
                             ["/Test/A", "/Test/B", "/Test/C", "/Test/D"])

            expectedListOp = Sdf.PathListOp()
            expectedListOp.prependedItems = ["/Test/A", "/Test/B"]
            expectedListOp.appendedItems = ["/Test/C", "/Test/D"]
            self.assertEqual(relSpec.GetInfo("targetPaths"), expectedListOp)

if __name__ == '__main__':
    unittest.main()
