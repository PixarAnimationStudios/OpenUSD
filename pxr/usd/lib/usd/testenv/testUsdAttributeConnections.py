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
            add int testAttr.connect = [
                </Qux>,
                </Bar>,
                </Baz>,
                </Foo.someAttr>,
            ]
        }

        def Scope "Baz"
        {
            add int bogus.connect = </MissingConnectionPath>
        }

        def "Recursive" {
            def "A" { add int AtoB.connect = <../B>
            }
            def "B" { add int BtoC.connect = <../C>
            }
            def "C" { add int CtoD.connect = <../D>
            }
            def "D" { add int DtoA.connect = <../A>
                def "A" { add int AtoB.connect = <../B>
                }
                def "B" { add int BtoC.connect = <../C>
                }
                def "C" { add int CtoD.connect = <../D>
                }
                def "D" { add int DtoA.connect = <../A>
                }
            }
        }
        ''')

    return s

class TestUsdAttributeConnections(unittest.TestCase):
    def test_Connections(self):
        for fmt in allFormats:
            stage = _CreateStage(fmt)

        # Simple connect list with correct order
        r = stage.GetPrimAtPath("/Foo").GetAttribute("testAttr")
        sol = map(Sdf.Path, ['/Qux', '/Bar', '/Baz', '/Foo.someAttr'])
        self.assertEqual(r.GetConnections(forwardToObjectsInMasters=True), sol) 
        self.assertEqual(r.GetConnections(forwardToObjectsInMasters=False), sol) 

        # Recursive finding
        recursive = stage.GetPrimAtPath("/Recursive")
        # self.assertEqual(
        #     set(recursive.FindAllAttributeConnectionPaths()),
        #     set([Sdf.Path('/Recursive/A'), Sdf.Path('/Recursive/B'),
        #          Sdf.Path('/Recursive/C'), Sdf.Path('/Recursive/D'),
        #          Sdf.Path('/Recursive/D/A'), Sdf.Path('/Recursive/D/B'),
        #          Sdf.Path('/Recursive/D/C'), Sdf.Path('/Recursive/D/D')]))

        # self.assertEqual(
        #     set(recursive.FindAllAttributeConnectionPaths(
        #         predicate =
        #         lambda attr: attr.GetPrim().GetName() in ('B', 'D'))),
        #     set([Sdf.Path('/Recursive/A'), Sdf.Path('/Recursive/C'),
        #          Sdf.Path('/Recursive/D/A'), Sdf.Path('/Recursive/D/C')]))

        # self.assertEqual(
        #     set(recursive.FindAllAttributeConnectionPaths(
        #         predicate =
        #         lambda attr: attr.GetPrim().GetName() in ('A', 'C'))),
        #     set([Sdf.Path('/Recursive/B'), Sdf.Path('/Recursive/D'),
        #          Sdf.Path('/Recursive/D/B'), Sdf.Path('/Recursive/D/D')]))
                
        # recursiveA = stage.GetPrimAtPath("/Recursive/A")
        # self.assertEqual(set(recursiveA.FindAllAttributeConnectionPaths()),
        #             set([Sdf.Path('/Recursive/B')]))
            
        # self.assertEqual(set(
        #     recursiveA.FindAllAttributeConnectionPaths(recurseOnSources=True)),
        #     set([Sdf.Path('/Recursive/A'), Sdf.Path('/Recursive/B'),
        #          Sdf.Path('/Recursive/C'), Sdf.Path('/Recursive/D'),
        #          Sdf.Path('/Recursive/D/A'), Sdf.Path('/Recursive/D/B'),
        #          Sdf.Path('/Recursive/D/C'), Sdf.Path('/Recursive/D/D')]))

        # self.assertEqual(set(
        #     recursiveA.FindAllAttributeConnectionPaths(
        #         recurseOnSources=True,
        #         predicate=lambda attr: attr.GetPrim().GetParent().GetName() ==
        #         'Recursive' or attr.GetPrim().GetName() in ('A', 'C'))),
        #     set([Sdf.Path('/Recursive/A'), Sdf.Path('/Recursive/B'),
        #          Sdf.Path('/Recursive/C'), Sdf.Path('/Recursive/D'),
        #          Sdf.Path('/Recursive/D/B'), Sdf.Path('/Recursive/D/D')]))

    def test_ConnectionsInInstances(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestConnectionsInInstances.'+fmt)
            s.GetRootLayer().ImportFromString('''#usda 1.0
            def Scope "Ref"
            {
                def Scope "Foo"
                {
                    custom int someAttr
                add int testAttr.connect = [
                        </Ref/Qux>,
                        </Ref/Bar>,
                        </Ref/Baz>,
                        </Ref/Foo.someAttr>,
                    ]
                }

                def Scope "Baz"
                {
                add int bogus.connect = </Ref/MissingConnectionPath>
                add int bogus2.connect = </Ref>
                }

            }

            def Scope "Root" (
                instanceable = true
                references = </Ref>
            )
            {
            }
            ''')

            master = s.GetPrimAtPath('/Root').GetMaster()
            self.assertTrue(master)

            # Simple source list with correct order
            a = master.GetChild("Foo").GetAttribute("testAttr")
            sol = [master.GetPath().AppendChild('Qux'), 
                   master.GetPath().AppendChild('Bar'), 
                   master.GetPath().AppendChild('Baz'), 
                   master.GetPath().AppendPath(Sdf.Path('Foo.someAttr'))]
            self.assertEqual(a.GetConnections(), sol) 

            # Bogus source path
            a = master.GetChild("Baz").GetAttribute("bogus")
            sol = [master.GetPath().AppendChild("MissingConnectionPath")]
            self.assertEqual(a.GetConnections(), sol)

            # Another bogus path -- paths inside instances cannot point to the
            # instance root.
            #with ExpectedWarnings(2):
            a = master.GetChild("Baz").GetAttribute("bogus2")
            sol = []
            self.assertEqual(a.GetConnections(), sol)

    def test_ConnectionsToObjectsInInstances(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory(
                'TestConnectionsToObjectsInInstances.'+fmt)
            stage.GetRootLayer().ImportFromString('''#usda 1.0
                def "Instance"
                {
                    double attr = 1.0

                    def "A"
                    {
                        double attr = 1.0
                    int cattr.connect = [
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
                int cattr.connect = [ 
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
                    int cattr.connect = [ 
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

            master = stage.GetPrimAtPath("/Root/Instance_1").GetMaster()
            nestedMaster = master.GetChild("NestedInstance_1").GetMaster()

            master = master.GetPath()
            nestedMaster = nestedMaster.GetPath()

        # Test retrieving connections that point to instances and prims within
        # instances.
        def _TestConnection(attr):
            self.assertTrue(attr)

            # Connections to objects in masters cannot be authored.
            with self.assertRaises(Tf.ErrorException):
                self.assertTrue(not attr.AppendConnection(master.AppendChild("A")))
            with self.assertRaises(Tf.ErrorException):
                self.assertTrue(not attr.RemoveConnection(master.AppendChild("A")))
            with self.assertRaises(Tf.ErrorException):
                self.assertTrue(not attr.SetConnections(
                    ["/Root/Instance_1", master.AppendChild("A")]))

            # Connections pointing to prims within instances will be forwarded
                # to the corresponding master by default.
            connections = attr.GetConnections()
            expected = [
                Sdf.Path("/Root/Instance_1"),
                Sdf.Path("/Root/Instance_1.attr"),
                master.AppendChild("A"),
                master.AppendChild("A").AppendProperty("attr"),
                master.AppendChild("NestedInstance_1"),
                master.AppendChild("NestedInstance_1").AppendProperty("attr"),
                nestedMaster.AppendChild("B"),
                nestedMaster.AppendChild("B").AppendProperty("attr"),
                master.AppendChild("NestedInstance_2"),
                master.AppendChild("NestedInstance_2").AppendProperty("attr"),
                nestedMaster.AppendChild("B"),
                nestedMaster.AppendChild("B").AppendProperty("attr"),
                Sdf.Path("/Root/Instance_2"),
                Sdf.Path("/Root/Instance_2.attr"),
                master.AppendChild("A"),
                master.AppendChild("A").AppendProperty("attr"),
                master.AppendChild("NestedInstance_1"),
                master.AppendChild("NestedInstance_1").AppendProperty("attr"),
                nestedMaster.AppendChild("B"),
                nestedMaster.AppendChild("B").AppendProperty("attr"),
                master.AppendChild("NestedInstance_2"),
                master.AppendChild("NestedInstance_2").AppendProperty("attr"),
                nestedMaster.AppendChild("B"),
                nestedMaster.AppendChild("B").AppendProperty("attr")]
            self.assertEqual(connections, expected)

            # GetConnections will not forward paths to objects in masters if
                # requested.
            connections = attr.GetConnections(forwardToObjectsInMasters=False)
            expected2 = [
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
            self.assertEqual(connections, expected2)

        attr = stage.GetPrimAtPath("/Root").GetAttribute("cattr")
        _TestConnection(attr)

        attr = stage.GetPrimAtPath("/Root/Instance_1").GetAttribute("cattr")
        _TestConnection(attr)

if __name__ == '__main__':
    unittest.main()
