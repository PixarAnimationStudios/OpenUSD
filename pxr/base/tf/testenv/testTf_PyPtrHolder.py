#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function

import sys
from pxr.Tf import *

status = 0

def err(msg):
    global status
    status += 1
    return 'ERROR: ' + msg + ' failed'


try:

    #
    # Test basic RefPointer operation
    #
    p = TestRefObject()
    assert p.getObjectName() == "RefObject", err("ref construction")
    assert p.GetNumInstances() == 1, err("ref construction")
    q = p
    assert TestRefObject.GetNumInstances() == 1, err("ref sharing")
    assert p == q, err("ref equality testing")
    assert not (p != q), err("ref non-equality testing")
    p = None
    assert TestRefObject.GetNumInstances() == 1, err("ref sharing")
    q = None
    assert TestRefObject.GetNumInstances() == 0, err("ref deletion")
    
    #
    # Try getting a null refptr.
    #
    assert TestRefObject.NullPtr() is None, err("null ref ptr")

    #
    # Try weakptr stuff.
    #
    p = TestWeakObject()
    assert p.getObjectName() == 'WeakObject', err('weak construction')
    assert p.GetNumInstances() == 1, err("weak construction")
    assert p.getWeakObjectName() == 'WeakObject', err('injected method')
    try:
        p.getWeakObjectDerivedNameBAD()
    except:
        pass
    else:
        assert False, err('injected method')

    q = p
    assert TestWeakObject.GetNumInstances() == 1, err("weak sharing")
    p = None
    assert TestWeakObject.GetNumInstances() == 1, err("weak sharing")
    p = q.GetInstance()
    assert p == q, err("weak equality testing")
    assert not (p != q), err("weak non-equality testing")
    q.KillInstance()
    assert TestWeakObject.GetNumInstances() == 0, err("weak sharing")
    assert q.expired and p.expired, err("weak ptr expiry")
    try:
        print(q.getName())
    except:
        pass
    else:
        err("accessing expired weak ptr")

    # Other weakptr stuff (inheritance, injected methods)

    o = TestWeakObject()
    od = TestWeakObjectDerived()
    o2 = TestWeakObject2()

    assert o != od and od != o2 and o2 != o, err('weak equality')

    assert od.getObjectName() == 'WeakObjectDerived' and \
           od.getWeakObjectName() == 'WeakObjectDerived' and \
           od.getWeakObjectDerivedName() == 'WeakObjectDerived', \
           err('good injected methods on WeakObjectDerived')

    try:
        od.getWeakObject2NameBAD()
    except:
        pass
    else:
        assert False, err('bad injected method on WeakObjectDerived')


    assert o2.getObjectName() == 'WeakObject 2' and \
           o2.getWeakObject2Name() == 'WeakObject 2', \
           err('good injected methods on WeakObject2')

    try:
        o2.getWeakObjectNameBAD()
    except:
        pass
    else:
        assert False, err('bad injected method on WeakObject2')

    try:
        o2.getWeakObjectDerivedNameBAD()
    except:
        pass
    else:
        assert False, err('bad injected method on WeakObject2')
        

except Exception as e:
    status += 1
    print(e)

if status != 0:
    print("Test FAILED")
else:
    print("Test SUCCEEDED")

sys.exit(status)
