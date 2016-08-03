#!/pxrpythonsubst

from pxr import Sdf, Usd, Tf

import Mentor.Runtime
from Mentor.Runtime import AssertEqual, AssertTrue, AssertFalse

Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)

allFormats = ['usd' + x for x in 'abc']

def BasicClassTest():
    for fmt in allFormats:
        l = Sdf.Layer.CreateAnonymous('BasicClassTest.'+fmt)

        stage = Usd.Stage.Open(l.identifier)

        # Create a new class "foo" and set some attributes.
        f = stage.CreateClassPrim("/foo")
        a =  f.CreateAttribute("bar", Sdf.ValueTypeNames.Int)
        a.Set(42)
        AssertEqual(a.Get(), 42)

        a = f.CreateAttribute("baz", Sdf.ValueTypeNames.Int)
        a.Set(24)
        AssertEqual(a.Get(), 24)

        # Create a new prim that will ultimately become an instance of
        # "foo".
        fd = stage.DefinePrim("/fooDerived", "Scope")
        AssertFalse(fd.GetAttribute("bar").IsDefined())
        AssertFalse(fd.GetAttribute("baz").IsDefined())

        a = fd.CreateAttribute("baz", Sdf.ValueTypeNames.Int)
        a.Set(42)
        AssertEqual(a.Get(), 42)

        # Author the inherits statement to make "fooDerived" an instance of
        # "foo"
        AssertTrue(fd.GetInherits().Add("/foo"))

        # Verify that opinions from the class come through, but are overridden
        # by any opinions on the instance.
        AssertTrue(fd.GetAttribute("bar").IsDefined())
        AssertEqual(fd.GetAttribute("bar").Get(), 42)

        AssertTrue(fd.GetAttribute("baz").IsDefined())
        AssertEqual(fd.GetAttribute("baz").Get(), 42)

if __name__ == "__main__":
    BasicClassTest()
    Mentor.Runtime.ExitTest()
