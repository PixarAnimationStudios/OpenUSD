#!/pxrpythonsubst

from pxr import Sdf, Tf
from Mentor.Runtime import *

# Configure mentor so assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)


emptyPayload = Sdf.Payload()

# Generate a bunch of unique payloads
args = [
    ['assetPath', '//menv30/layer.menva'],
    ['primPath', '/rootPrim'],
]
payloads = []
for n in range(1 << len(args)):
    kw = {}
    for i in range(len(args)):
        if (1 << i) & n:
            kw[args[i][0]] = args[i][1]

    payload = Sdf.Payload(**kw)

    payloads.append( payload )

    # Test property access
    for arg, value in args:
        if kw.has_key(arg):
            AssertEqual( getattr(payload, arg), value )
        else:
            AssertEqual( getattr(payload, arg), getattr(emptyPayload, arg) )

# Sort using <
payloads.sort()

# Test comparison operators
for i in range(len(payloads)):
    a = payloads[i]
    for j in range(i, len(payloads)):
        b = payloads[j]

        assert (a == b) == (i == j)
        assert (a != b) == (i != j)
        assert (a <= b) == (i <= j)
        assert (a >= b) == (i >= j)
        assert (a  < b) == (i  < j)
        assert (a  > b) == (i  > j)

# Test repr
for payload in payloads:
    AssertEqual(payload, eval(repr(payload)))

ExitTest()
