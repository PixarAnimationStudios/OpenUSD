#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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
#
from pxr import Ar

import unittest

# XXX
# For the Ar 2.0 rollout, skip this test if the Ar.ResolverContext object
# does not exist. This object is only available if Ar 2.0 is enabled.
def HasResolverContextClass():
    try:
        ctx = Ar.ResolverContext()
        return True
    except:
        pass
    return False

@unittest.skipIf(not HasResolverContextClass(), "No Ar.ResolverContext object")
class TestArResolverContext(unittest.TestCase):
    def test_Repr(self):
        ctxObj = Ar.DefaultResolverContext(["/test"])
        ctx = Ar.ResolverContext(ctxObj)
        
        expectedRepr = "Ar.ResolverContext({})".format(repr(ctxObj))
        self.assertEqual(str(ctx), expectedRepr)
        self.assertEqual(repr(ctx), expectedRepr)

    def test_Get(self):
        ctxObj = Ar.DefaultResolverContext(["/test"])
        ctx = Ar.ResolverContext(ctxObj)
        self.assertEqual(ctx.Get(), [ctxObj])

    def test_ExplicitConstruction(self):
        # Passing in None or an empty tuple or list to Ar.ResolverContext
        # should all result in creating an empty Ar.ResolverContext.
        self.assertEqual(Ar.ResolverContext(None), Ar.ResolverContext())
        self.assertEqual(Ar.ResolverContext(()), Ar.ResolverContext())
        self.assertEqual(Ar.ResolverContext([]), Ar.ResolverContext())

        # Passing in a Python-wrapped context object or tuple or list
        # containing such objects should create an Ar.ResolverContext holding
        # these objects.
        ctxObj = Ar.DefaultResolverContext(["/test"])
        ctx = Ar.ResolverContext(ctxObj)

        self.assertEqual(Ar.ResolverContext(ctxObj), ctx)
        self.assertEqual(Ar.ResolverContext((ctxObj, )), ctx)
        self.assertEqual(Ar.ResolverContext([ctxObj, ]), ctx)

        # Passing in an object that hasn't been registered as a context object
        # should result in an error.
        with self.assertRaises(TypeError):
            ctx = Ar.ResolverContext(1)
        with self.assertRaises(TypeError):
            ctx = Ar.ResolverContext((1,))
        with self.assertRaises(TypeError):
            ctx = Ar.ResolverContext([1,])
        with self.assertRaises(TypeError):
            ctx = Ar.ResolverContext((1, ctxObj))
        with self.assertRaises(TypeError):
            ctx = Ar.ResolverContext([1, ctxObj])

    def test_ImplicitConversion(self):
        """Test implicit conversion of a Python-wrapped context object
        when passed to a C++ function that takes an ArResolverContext."""

        # Passing in None or an empty tuple or list should implicitly
        # convert to an empty Ar.ResolverContext()
        self.assertEqual(Ar._TestImplicitConversion(None), Ar.ResolverContext())
        self.assertEqual(Ar._TestImplicitConversion(()), Ar.ResolverContext())
        self.assertEqual(Ar._TestImplicitConversion([]), Ar.ResolverContext())

        # Passing in a Python-wrapped context object or tuple or list
        # containing such objects should implicitly convert to an
        # Ar.ResolverContext holding these objects.
        ctxObj = Ar.DefaultResolverContext(["/test"])
        ctx = Ar.ResolverContext(ctxObj)

        self.assertEqual(Ar._TestImplicitConversion(ctxObj), ctx)
        self.assertEqual(Ar._TestImplicitConversion((ctxObj, )), ctx)
        self.assertEqual(Ar._TestImplicitConversion([ctxObj, ]), ctx)

        # Passing in an object that hasn't been registered as a context object
        # should result in an error.
        with self.assertRaises(TypeError):
            ctx = Ar._TestImplicitConversion(1)
        with self.assertRaises(TypeError):
            ctx = Ar._TestImplicitConversion((1,))
        with self.assertRaises(TypeError):
            ctx = Ar._TestImplicitConversion([1,])
        with self.assertRaises(TypeError):
            ctx = Ar._TestImplicitConversion((1, ctxObj))
        with self.assertRaises(TypeError):
            ctx = Ar._TestImplicitConversion([1, ctxObj])

if __name__ == '__main__':
    unittest.main()
