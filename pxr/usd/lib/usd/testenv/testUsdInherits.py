#!/pxrpythonsubst

from pxr import Usd, Sdf, Tf

from Mentor.Runtime import (AssertEqual)

allFormats = ['usd' + x for x in 'abc']

def TestBasicApi():
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory("x."+fmt)
        classA = stage.CreateClassPrim("/ClassA")
        concrete = stage.OverridePrim("/Concrete")
        items = None

        assert not concrete.HasAuthoredInherits()
        assert concrete.GetInherits().Add(classA.GetPath())
        assert concrete.HasAuthoredInherits()
        AssertEqual(len(concrete.GetMetadata("inheritPaths").addedItems), 1)
        AssertEqual(concrete.GetMetadata("inheritPaths").addedItems[0],
                    classA.GetPath())
        AssertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 0)
        # This will be used later in the test.
        items = concrete.GetMetadata("inheritPaths").addedItems

        assert concrete.GetInherits().Remove(classA.GetPath())
        assert concrete.HasAuthoredInherits()
        AssertEqual(len(concrete.GetMetadata("inheritPaths").addedItems), 0)
        AssertEqual(len(concrete.GetMetadata("inheritPaths").deletedItems), 1)
        AssertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 0)

        assert concrete.GetInherits().Clear()
        assert not concrete.HasAuthoredInherits()
        assert not concrete.GetMetadata("inheritPaths")

        # Set the list of added items explicitly.
        assert concrete.GetInherits().SetItems(items)
        assert concrete.HasAuthoredInherits()
        AssertEqual(len(concrete.GetMetadata("inheritPaths").addedItems), 0)
        AssertEqual(len(concrete.GetMetadata("inheritPaths").deletedItems), 0)
        AssertEqual(len(concrete.GetMetadata("inheritPaths").explicitItems), 1)


def TestInheritedPrim():
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory("x."+fmt)
        classA = stage.CreateClassPrim("/ClassA")
        stage.DefinePrim("/ClassA/Child")

        concrete = stage.DefinePrim("/Concrete")

        assert not concrete.GetChildren() 
        assert concrete.GetInherits().Add(classA.GetPath())

        AssertEqual(concrete.GetChildren()[0].GetPath(),
                    concrete.GetPath().AppendChild("Child"))

        assert concrete.GetInherits().Remove(classA.GetPath())
        assert len(concrete.GetChildren()) == 0


if __name__ == '__main__':
    TestBasicApi()
    TestInheritedPrim()
    print "OK"

