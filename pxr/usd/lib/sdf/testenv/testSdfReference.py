#!/pxrpythonsubst

from pxr import Sdf, Tf
from Mentor.Runtime import *

# Configure mentor so assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)



print "\nTesting Sdf.Reference Repr:"

# Test all combinations of the following keyword arguments.
args = [
    ['assetPath', '//menv30/layer.menva'],
    ['primPath', '/rootPrim'],
    ['layerOffset', Sdf.LayerOffset(48, -2)],
    ['customData', {'key': 42, 'other': 'yes'}],
]

for n in range(1 << len(args)):
    kw = {}
    for i in range(len(args)):
        if (1 << i) & n:
            kw[args[i][0]] = args[i][1]

    ref = Sdf.Reference(**kw)
    print '  Testing Repr for: ' + repr(ref)

    AssertEqual(ref, eval(repr(ref)))
    for arg, value in args:
        if kw.has_key(arg):
            AssertEqual(eval('ref.' + arg), value)
        else:
            AssertEqual(eval('ref.' + arg), eval('Sdf.Reference().' + arg))



print "\nTesting Sdf.Reference immutability."

# There is no proxy for the Reference yet (we don't have a good
# way to support nested proxies).  Make sure the user can't modify
# temporary Reference objects.
AssertException(
    "Sdf.Reference().assetPath = '//menv30/blah.menva'",
    AttributeError)
AssertException(
    "Sdf.Reference().primPath = '/root'",
    AttributeError)
AssertException(
    "Sdf.Reference().layerOffset = Sdf.LayerOffset()",
    AttributeError)
AssertException(
    "Sdf.Reference().layerOffset.offset = 24",
    AttributeError)
AssertException(
    "Sdf.Reference().layerOffset.scale = 2",
    AttributeError)
AssertException(
    "Sdf.Reference().customData = {'refCustomData': {'refFloat': 1.0}}",
    AttributeError)



# Code coverage.
ref0 = Sdf.Reference(customData={'a': 0})
ref1 = Sdf.Reference(customData={'a': 0, 'b': 1})
AssertTrue(ref0 == ref0)
AssertTrue(ref0 != ref1)
AssertTrue(ref0 < ref1)
AssertTrue(ref1 > ref0)
AssertTrue(ref0 <= ref1)
AssertTrue(ref1 >= ref0)



print
ExitTest()
