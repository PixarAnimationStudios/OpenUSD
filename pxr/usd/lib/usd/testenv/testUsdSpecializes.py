#!/pxrpythonsubst

from pxr import Usd, Sdf, Tf

from Mentor.Runtime import (AssertEqual)

allFormats = ['usd' + x for x in 'abc']

def TestBasicApi():
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory("x."+fmt)
        specA = stage.DefinePrim("/SpecA")
        concrete = stage.OverridePrim("/Concrete")
        items = None

        assert not concrete.HasAuthoredSpecializes()
        assert concrete.GetSpecializes().Add(specA.GetPath())
        assert concrete.HasAuthoredSpecializes()
        AssertEqual(len(concrete.GetMetadata("specializes").addedItems), 1)
        AssertEqual(concrete.GetMetadata("specializes").addedItems[0],
                    specA.GetPath())
        AssertEqual(len(concrete.GetMetadata("specializes").explicitItems), 0)
        # This will be used later in the test.
        items = concrete.GetMetadata("specializes").addedItems

        assert concrete.GetSpecializes().Remove(specA.GetPath())
        assert concrete.HasAuthoredSpecializes()
        AssertEqual(len(concrete.GetMetadata("specializes").addedItems), 0)
        AssertEqual(len(concrete.GetMetadata("specializes").deletedItems), 1)
        AssertEqual(len(concrete.GetMetadata("specializes").explicitItems), 0)

        assert concrete.GetSpecializes().Clear()
        assert not concrete.HasAuthoredSpecializes()
        assert not concrete.GetMetadata("specializes")

        # Set the list of added items explicitly.
        assert concrete.GetSpecializes().SetItems(items)
        assert concrete.HasAuthoredSpecializes()
        AssertEqual(len(concrete.GetMetadata("specializes").addedItems), 0)
        AssertEqual(len(concrete.GetMetadata("specializes").deletedItems), 0)
        AssertEqual(len(concrete.GetMetadata("specializes").explicitItems), 1)


def TestSpecializedPrim():
    for fmt in allFormats:
        stage = Usd.Stage.CreateInMemory("x."+fmt)
        specA = stage.CreateClassPrim("/SpecA")
        stage.DefinePrim("/SpecA/Child")

        concrete = stage.DefinePrim("/Concrete")

        assert not concrete.GetChildren() 
        assert concrete.GetSpecializes().Add(specA.GetPath())

        AssertEqual(concrete.GetChildren()[0].GetPath(),
                    concrete.GetPath().AppendChild("Child"))

        assert concrete.GetSpecializes().Remove(specA.GetPath())
        assert len(concrete.GetChildren()) == 0


if __name__ == '__main__':
    TestBasicApi()
    TestSpecializedPrim()
    print "OK"

