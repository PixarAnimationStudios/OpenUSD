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
    stage.GetPrimAtPath('/Set').GetInherits().AppendInherit('/Class')

    # Prior to the bug fix, the above would cause a new master prim
    # to be registered internally. However, since the parent of
    # the instance was deactivated, we would never actually instantiate
    # that master prim on the stage. This would lead to a crash later
    # on, as the stage would expect to find this master prim.

if __name__ == "__main__":
    TestBug141491()
    print 'OK'
