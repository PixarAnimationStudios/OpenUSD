#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

from pxr import Sdf, Usd

def TestBug141491():
    """
    Regression test for bug where deactivating an ancestor of an
    instance prim and then making an edit that would have affected
    that instance prim would lead to a crash at stage teardown.
    """
    stage = Usd.Stage.Open('root.usda')
    instancePrim = stage.GetPrimAtPath('/World/sets/Set_1')
    assert instancePrim
    assert instancePrim.IsInstance()

    # Deactivate the ancestor group containing instancePrim.
    # This should cause the instancePrim to be removed from
    # the stage.
    ancestorPrim = stage.GetPrimAtPath('/World/sets')
    assert ancestorPrim.IsActive()
    ancestorPrim.SetActive(False)
    assert not instancePrim

    # Now make a scene description edit that would affect
    # the instance. Here, we just add an inherit arc to the
    # 'Set' prim that the instance was referencing.
    stage.GetPrimAtPath('/Set').GetInherits().AddInherit('/Class')

    # Prior to the bug fix, the above would cause a new prototype prim
    # to be registered internally. However, since the parent of
    # the instance was deactivated, we would never actually instantiate
    # that prototype prim on the stage. This would lead to a crash later
    # on, as the stage would expect to find this prototype prim.

if __name__ == "__main__":
    TestBug141491()
    print('OK')
