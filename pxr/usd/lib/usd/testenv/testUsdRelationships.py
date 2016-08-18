#!/pxrpythonsubst

import sys
from pxr import Sdf, Tf, Usd

from Mentor.Runtime.Utility import (Assert, AssertEqual, ExpectedErrors,
                                    ExpectedWarnings, RequiredException)

allFormats = ['usd' + x for x in 'abc']

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
        ''')

    return s

def TestTargets():
    for fmt in allFormats:
        stage = _CreateStage(fmt)

        # Simple target list with correct order
        r = stage.GetPrimAtPath("/Foo").GetRelationship("testRel")
        sol = map(Sdf.Path, ['/Qux', '/Bar', '/Baz', '/Foo.someAttr'])
        AssertEqual(r.GetTargets(forwardToObjectsInMasters=True), sol) 
        AssertEqual(r.GetTargets(forwardToObjectsInMasters=False), sol) 

        # Forwarded targets
        r = stage.GetPrimAtPath("/Bar").GetRelationship("fwd")
        sol = map(Sdf.Path, ['/Baz', '/Qux', '/Bar', '/Foo.someAttr'])
        AssertEqual(r.GetForwardedTargets(forwardToObjectsInMasters=True), sol)
        AssertEqual(r.GetForwardedTargets(forwardToObjectsInMasters=False), sol)

        # Forwarded targets
        r = stage.GetPrimAtPath("/Bar").GetRelationship("fwd2")
        sol = map(Sdf.Path, ['/Qux', '/Baz', '/Bar'])
        AssertEqual(r.GetForwardedTargets(forwardToObjectsInMasters=True), sol)
        AssertEqual(r.GetForwardedTargets(forwardToObjectsInMasters=False), sol)

        # Cycle detection
        r = stage.GetPrimAtPath("/Bar").GetRelationship("cycle")
        sol = map(Sdf.Path, ['/Baz', '/Qux', '/Bar', '/Foo.someAttr'])
        AssertEqual(r.GetForwardedTargets(forwardToObjectsInMasters=True), sol)
        AssertEqual(r.GetForwardedTargets(forwardToObjectsInMasters=False), sol)

        # Bogus target path
        r = stage.GetPrimAtPath("/Baz").GetRelationship("bogus")
        sol = [Sdf.Path('/MissingTargetPath')]
        AssertEqual(r.GetForwardedTargets(forwardToObjectsInMasters=True), sol)
        AssertEqual(r.GetForwardedTargets(forwardToObjectsInMasters=False), sol)

def TestTargetsInInstances():
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
                add rel bogus2 = </Ref>
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

        master = s.GetPrimAtPath('/Root').GetMaster()
        Assert(master)

        # Simple target list with correct order
        r = master.GetChild("Foo").GetRelationship("testRel")
        sol = [master.GetPath().AppendChild('Qux'), 
               master.GetPath().AppendChild('Bar'), 
               master.GetPath().AppendChild('Baz'), 
               master.GetPath().AppendPath(Sdf.Path('Foo.someAttr'))]
        AssertEqual(r.GetTargets(), sol) 

        # Forwarded targets
        r = master.GetChild("Bar").GetRelationship("fwd")
        sol = [master.GetPath().AppendChild('Baz'), 
               master.GetPath().AppendChild('Qux'), 
               master.GetPath().AppendChild('Bar'), 
               master.GetPath().AppendPath(Sdf.Path('Foo.someAttr'))]
        AssertEqual(r.GetForwardedTargets(), sol)

        # Forwarded targets
        r = master.GetChild("Bar").GetRelationship("fwd2")
        sol = [master.GetPath().AppendChild('Qux'), 
               master.GetPath().AppendChild('Baz'), 
               master.GetPath().AppendChild('Bar')]
        AssertEqual(r.GetForwardedTargets(), sol)

        # Cycle detection
        r = master.GetChild("Bar").GetRelationship("cycle")
        sol = [master.GetPath().AppendChild('Baz'), 
               master.GetPath().AppendChild('Qux'), 
               master.GetPath().AppendChild('Bar'), 
               master.GetPath().AppendPath(Sdf.Path('Foo.someAttr'))]
        AssertEqual(r.GetForwardedTargets(), sol)

        # Bogus target path
        r = master.GetChild("Baz").GetRelationship("bogus")
        sol = [master.GetPath().AppendChild("MissingTargetPath")]
        AssertEqual(r.GetTargets(), sol)
        AssertEqual(r.GetForwardedTargets(), sol)

        # Another bogus target path -- target paths authored inside instances
        # cannot target the instance root.
        with ExpectedWarnings(2):
            r = master.GetChild("Baz").GetRelationship("bogus2")
            sol = []
            AssertEqual(r.GetTargets(), sol)
            AssertEqual(r.GetForwardedTargets(), sol)

def TestTargetsToObjectsInInstances():
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory('TestTargetsToObjectsInInstances.'+fmt)
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

        master = stage.GetPrimAtPath("/Root/Instance_1").GetMaster()
        nestedMaster = master.GetChild("NestedInstance_1").GetMaster()

        master = master.GetPath()
        nestedMaster = nestedMaster.GetPath()

        # Test retrieving relationship targets that point to instances and
        # prims within instances.
        def _TestRelationship(rel):
            Assert(rel)

            # Targets to objects in masters cannot be authored.
            with RequiredException(Tf.ErrorException):
                Assert(not rel.AddTarget(master.AppendChild("A")))
            with RequiredException(Tf.ErrorException):
                Assert(not rel.RemoveTarget(master.AppendChild("A")))
            with RequiredException(Tf.ErrorException):
                Assert(not rel.SetTargets(
                        ["/Root/Instance_1", master.AppendChild("A")]))

            # Targets pointing to prims within instances will be forwarded
            # to the corresponding master by default.
            targets = rel.GetTargets()
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
            AssertEqual(targets, expected)

            # GetTargets will not forward target paths to objects in masters if
            # requested.
            targets = rel.GetTargets(forwardToObjectsInMasters=False)
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
            AssertEqual(targets, expected2)

        rel = stage.GetPrimAtPath("/Root").GetRelationship("rel")
        _TestRelationship(rel)

        rel = stage.GetPrimAtPath("/Root/Instance_1").GetRelationship("rel")
        _TestRelationship(rel)

        def _TestRelationshipForwarding(rel):
            Assert(rel)

            # Expect warning here due to targets authored inside an instance
            # that point to the instance root.
            with ExpectedWarnings(1):
                targets = rel.GetForwardedTargets()
            expected = [
                master.AppendChild("A"),
                master.AppendChild("A").AppendProperty("attr"),
                master.AppendChild("NestedInstance_1"),
                master.AppendChild("NestedInstance_1").AppendProperty("attr"),
                nestedMaster.AppendChild("B"),
                nestedMaster.AppendChild("B").AppendProperty("attr"),
                master.AppendChild("NestedInstance_2"),
                master.AppendChild("NestedInstance_2").AppendProperty("attr")]
            AssertEqual(targets, expected)

            targets = rel.GetForwardedTargets(forwardToObjectsInMasters=False)
            expected2 = [
                Sdf.Path("/Root/Instance_1/A.rel"),
                Sdf.Path("/Root/Instance_2/A.rel")]
            AssertEqual(targets, expected2)

        rel = stage.GetPrimAtPath("/Root").GetRelationship("fwdRel")
        _TestRelationshipForwarding(rel)

        rel = stage.GetPrimAtPath("/Root/Instance_1").GetRelationship("fwdRel")
        _TestRelationshipForwarding(rel)

if __name__ == '__main__':
    TestTargets()
    TestTargetsInInstances()
    TestTargetsToObjectsInInstances()
    print 'OK'

