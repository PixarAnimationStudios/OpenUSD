#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Ar

import unittest

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
