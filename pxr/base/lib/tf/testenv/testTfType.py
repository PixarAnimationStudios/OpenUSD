#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

from pxr import Tf

import unittest

class Base(object):
    pass
class Derived(Base):
    pass
class AbstractInterface(object):
    pass
class ConcreteChild(Derived, AbstractInterface):
    pass

class TestTfType(unittest.TestCase):

    def _TestCppType(self, t):
        # Test API of a type we expect to be a pure C++ type.
        self.assertEqual(t, eval(repr(t)))
        self.assertEqual(t, Tf.Type.FindByName(t.typeName))
        self.assertFalse(t.pythonClass)
        if not t.isUnknown:
            self.assertTrue(t.IsA(t))

    @classmethod
    def setUpClass(cls):
        Tf.Type.Define(Base)
        Tf.Type.Define(Derived)
        Tf.Type.Define(AbstractInterface)
        Tf.Type.Define(ConcreteChild)

    def setUp(self):
        self.tUnknown = Tf.Type()
        self.tRoot = Tf.Type.GetRoot()
        self.tBase = Tf.Type.Find(Base)
        self.tDerived = Tf.Type.Find(Derived)
        self.tConcreteChild = Tf.Type.Find(ConcreteChild)
        self.tAbstractInterface = Tf.Type.Find(AbstractInterface)

    def test_UnknownType(self):
        self._TestCppType(self.tUnknown)
        self.assertEqual(self.tUnknown, Tf.Type.FindByName('bogus'))
        self.assertFalse(self.tUnknown.baseTypes)
        self.assertFalse(self.tUnknown.derivedTypes)
        self.assertFalse(self.tUnknown.IsA(Tf.Type.GetRoot()))
        self.assertEqual(hash(self.tUnknown), hash(Tf.Type.FindByName('bogus')))

    def test_RootType(self):
        self._TestCppType(self.tRoot)
        self.assertFalse(self.tRoot.baseTypes)

        # We should already have some builtInTypes defined:
        self.assertGreater(len(self.tRoot.derivedTypes), 1)
        self.assertEqual(hash(self.tRoot), hash(Tf.Type.GetRoot()))
        self.assertNotEqual(hash(self.tRoot), hash(Tf.Type()))

    def test_OtherBuiltinTypes(self):
        builtInTypeNames = 'int bool long float double size_t'.split()
        for typeName in builtInTypeNames:
            t = Tf.Type.FindByName(typeName)
            # Check that typeName is as we expect, except for size_t, which
            # is an alias.
            self.assertTrue(t.typeName == typeName or typeName == 'size_t')
            self._TestCppType(t)
            self.assertEqual((self.tRoot,), t.baseTypes)
            self.assertEqual((), t.derivedTypes)
            self.assertTrue(t.IsA(Tf.Type.GetRoot()))
            self.assertTrue(t == Tf.Type.FindByName('void') or t.sizeof > 0)
            self.assertEqual(hash(t),hash(Tf.Type.FindByName(typeName)))
            self.assertNotEqual(hash(t), hash(Tf.Type.GetRoot()))

    def test_PurePythonHierarchy(self):
        self.assertFalse(self.tBase.isUnknown)
        self.assertIn(Base.__name__, self.tBase.typeName)
        self.assertEqual(Base, self.tBase.pythonClass)
        self.assertEqual(self.tBase, Tf.Type.FindByName(self.tBase.typeName))
        self.assertEqual(self.tBase, Tf.Type.Find(Base))
        self.assertEqual((Tf.Type.Find(object),), self.tBase.baseTypes)

        self.assertFalse(self.tDerived.isUnknown)
        self.assertIn(Derived.__name__, self.tDerived.typeName)
        self.assertEqual(Derived, self.tDerived.pythonClass)
        self.assertEqual(self.tDerived, Tf.Type.FindByName(self.tDerived.typeName))
        self.assertEqual(self.tDerived, Tf.Type.Find(Derived))
        self.assertNotEqual(self.tBase, self.tDerived)
        self.assertEqual((self.tBase,), self.tDerived.baseTypes)
        #self.assertEqual((), self.tDerived.derivedTypes)
        self.assertEqual((Tf.Type.Find(object),), self.tBase.baseTypes)
        self.assertEqual((self.tDerived,), self.tBase.derivedTypes)

        self.assertFalse(self.tAbstractInterface.isUnknown)
        self.assertIn(AbstractInterface.__name__, self.tAbstractInterface.typeName)
        self.assertEqual(AbstractInterface, self.tAbstractInterface.pythonClass)
        self.assertEqual(self.tAbstractInterface, Tf.Type.FindByName(self.tAbstractInterface.typeName))
        self.assertEqual(self.tAbstractInterface, Tf.Type.Find(AbstractInterface))
        self.assertEqual((Tf.Type.Find(object), ), self.tAbstractInterface.baseTypes)
        #self.assertEqual((), self.tAbstractInterface.derivedTypes)

        self.assertFalse(self.tConcreteChild.isUnknown)
        self.assertIn(ConcreteChild.__name__, self.tConcreteChild.typeName)
        self.assertEqual(ConcreteChild, self.tConcreteChild.pythonClass)
        self.assertEqual(self.tConcreteChild, Tf.Type.FindByName(self.tConcreteChild.typeName))
        self.assertEqual(self.tConcreteChild, Tf.Type.Find(ConcreteChild))
        self.assertEqual((self.tDerived, self.tAbstractInterface), self.tConcreteChild.baseTypes)
        self.assertEqual((), self.tConcreteChild.derivedTypes)
        self.assertEqual((self.tConcreteChild,), self.tDerived.derivedTypes)
        self.assertEqual((self.tConcreteChild, ), self.tAbstractInterface.derivedTypes)

        # Base
        with self.assertRaises(Tf.ErrorException):
            self.assertFalse(self.tBase.IsA(self.tUnknown))
        self.assertTrue(self.tBase.IsA(self.tBase))
        self.assertFalse(self.tBase.IsA(self.tDerived))
        self.assertFalse(self.tBase.IsA(self.tAbstractInterface))
        self.assertFalse(self.tBase.IsA(self.tConcreteChild))

        # Derived
        with self.assertRaises(Tf.ErrorException):
            self.assertFalse(self.tDerived.IsA(self.tUnknown))
        self.assertTrue(self.tDerived.IsA(self.tBase))
        self.assertTrue(self.tDerived.IsA(self.tDerived))
        self.assertFalse(self.tDerived.IsA(self.tAbstractInterface))
        self.assertFalse(self.tDerived.IsA(self.tConcreteChild))

        # AbstractInterface
        with self.assertRaises(Tf.ErrorException):
            self.assertFalse(self.tAbstractInterface.IsA(self.tUnknown))
        self.assertFalse(self.tAbstractInterface.IsA(self.tBase))
        self.assertFalse(self.tAbstractInterface.IsA(self.tDerived))
        self.assertTrue(self.tAbstractInterface.IsA(self.tAbstractInterface))
        self.assertFalse(self.tAbstractInterface.IsA(self.tConcreteChild))

        # Concrete
        with self.assertRaises(Tf.ErrorException):
            self.assertFalse(self.tConcreteChild.IsA(self.tUnknown))
        self.assertTrue(self.tConcreteChild.IsA(self.tBase))
        self.assertTrue(self.tConcreteChild.IsA(self.tDerived))
        self.assertTrue(self.tConcreteChild.IsA(self.tAbstractInterface))
        self.assertTrue(self.tConcreteChild.IsA(self.tConcreteChild))

    def test_FindDerivedByName(self):
        # No types should derive from Unknown.
        self.assertEqual(self.tUnknown, self.tUnknown.FindDerivedByName(self.tUnknown.typeName))
        self.assertEqual(self.tUnknown, self.tUnknown.FindDerivedByName(self.tDerived.typeName))
        self.assertEqual(self.tUnknown, self.tUnknown.FindDerivedByName(self.tAbstractInterface.typeName))
        self.assertEqual(self.tUnknown, self.tUnknown.FindDerivedByName(self.tConcreteChild.typeName))

        # Check types deriving from Base.
        self.assertEqual(self.tBase, self.tBase.FindDerivedByName( self.tBase.typeName ))
        self.assertEqual(self.tDerived, self.tBase.FindDerivedByName( self.tDerived.typeName ))
        self.assertEqual(self.tUnknown, self.tBase.FindDerivedByName( self.tAbstractInterface.typeName ))
        self.assertEqual(self.tConcreteChild, self.tBase.FindDerivedByName( self.tConcreteChild.typeName ))

        # Check types deriving from Derived.
        self.assertEqual(self.tUnknown, self.tDerived.FindDerivedByName( self.tBase.typeName ))
        self.assertEqual(self.tDerived, self.tDerived.FindDerivedByName( self.tDerived.typeName ))
        self.assertEqual(self.tUnknown, self.tDerived.FindDerivedByName( self.tAbstractInterface.typeName ))
        self.assertEqual(self.tConcreteChild, self.tDerived.FindDerivedByName( self.tConcreteChild.typeName ))

        # Check types deriving from AbstractInterface.
        self.assertEqual(self.tUnknown, self.tAbstractInterface.FindDerivedByName( self.tBase.typeName )) 
        self.assertEqual(self.tUnknown, self.tAbstractInterface.FindDerivedByName( self.tDerived.typeName ))
        self.assertEqual(self.tAbstractInterface, self.tAbstractInterface.FindDerivedByName( self.tAbstractInterface.typeName ))
        self.assertEqual(self.tConcreteChild, self.tAbstractInterface.FindDerivedByName( self.tConcreteChild.typeName ))

        # Check types deriving from ConcreteChild.
        self.assertEqual(self.tUnknown, self.tConcreteChild.FindDerivedByName( self.tBase.typeName ))
        self.assertEqual(self.tUnknown, self.tConcreteChild.FindDerivedByName( self.tDerived.typeName ))
        self.assertEqual(self.tUnknown, self.tConcreteChild.FindDerivedByName( self.tAbstractInterface.typeName))
        self.assertEqual(self.tConcreteChild, self.tConcreteChild.FindDerivedByName( self.tConcreteChild.typeName ))


    @unittest.skip("XXX")
    def test_PythonSubclassOfCppClass(self):
        class TestPyDerived( Tf.TestCppBase ):
            def TestVirtual(self):
                return 123

        tTestCppBase = Tf.Type.Find( Tf.TestCppBase )
        tTestPyDerived = Tf.Type.Define( TestPyDerived )

        self.assertFalse(tTestCppBase.isUnknown)
        self.assertFalse(tTestPyDerived.isUnknown)

        self.assertEqual(Tf.TestCppBase, tTestCppBase.pythonClass)
        self.assertEqual(tTestCppBase, Tf.Type.Find( Tf.TestCppBase ))

        self.assertIn(TestPyDerived.__name__, tTestPyDerived.typeName)
        self.assertEqual(TestPyDerived, tTestPyDerived.pythonClass)
        self.assertEqual(tTestPyDerived, Tf.Type.Find( TestPyDerived ))
        self.assertEqual(tTestPyDerived, Tf.Type.FindByName( tTestPyDerived.typeName ))
        self.assertEqual((tTestCppBase,), tTestPyDerived.baseTypes)
        self.assertEqual((), tTestPyDerived.derivedTypes)

        self.assertFalse(tTestCppBase.IsA( tTestPyDerived ))
        self.assertTrue(tTestPyDerived.IsA( tTestCppBase ))

        # Test C++ TfType::Find() on a polymorphic base C++ type with a python
        # subclass.
        self.assertNotEqual(tTestPyDerived, tTestCppBase)
        self.assertEqual(tTestCppBase, Tf._TestFindType(Tf.TestCppBase()))
        self.assertEqual(tTestPyDerived, Tf._TestFindType(TestPyDerived()))

    def test_TypeAliases(self):
        class JamesBond(object):
            pass
        tBond = Tf.Type.Define(JamesBond)
        self.assertTrue(Tf.Type.FindByName('007').isUnknown)
        self.assertNotIn('007', Tf.Type.GetRoot().GetAliases(tBond))
        tBond.AddAlias( Tf.Type.GetRoot(), '007' )
        self.assertEqual(tBond, Tf.Type.FindByName('007'))
        self.assertIn('007', Tf.Type.GetRoot().GetAliases(tBond))

        #Test alias collision -- error expected
        with self.assertRaises(Tf.ErrorException):
            tBond.AddAlias(Tf.Type.GetRoot(), 'int' )
        self.assertNotEqual(tBond, Tf.Type.FindByName('int'))

    def test_InheritanceResolution(self):
        def CheckTfTypeOrderVsPython(classObj):
            '''Assert that the order of GetAllAncestorTypes() matches Python's
            native 'mro' method resolution order -- while accounting for the extra
            root TfType.'''
            t = Tf.Type.Find( classObj )
            # We should have a valid TfType.
            self.assertFalse(t.isUnknown)
            # Get Python's mro and convert the results to TfTypes.
            pythonOrder = map( Tf.Type.Find, classObj.mro() )
            self.assertEqual(tuple(pythonOrder + [self.tRoot,]), t.GetAllAncestorTypes())

        # Unknown type
        with self.assertRaises(Tf.ErrorException):
            Tf.Type().GetAllAncestorTypes()

        # Root type
        self.tRoot = Tf.Type.GetRoot()
        self.assertEqual((self.tRoot,), self.tRoot.GetAllAncestorTypes())

        # Python 'object' class
        tObject = Tf.Type.Find(object)
        self.assertFalse(tObject.isUnknown)
        self.assertEqual((tObject, self.tRoot,), tObject.GetAllAncestorTypes())
        CheckTfTypeOrderVsPython( object )

        # Single custom class
        class T1_Z(object): pass
        tT1_Z = Tf.Type.Define(T1_Z)
        self.assertEqual((tT1_Z, tObject, self.tRoot), tT1_Z.GetAllAncestorTypes())
        CheckTfTypeOrderVsPython( T1_Z )

        # Single inheritance test
        class T2_A(object): pass
        class T2_B(T2_A): pass
        class T2_Z(T2_B): pass
        tT2_A = Tf.Type.Define(T2_A)
        tT2_B = Tf.Type.Define(T2_B)
        tT2_Z = Tf.Type.Define(T2_Z)
        self.assertEqual((tT2_Z, tT2_B, tT2_A, tObject, self.tRoot), tT2_Z.GetAllAncestorTypes())
        CheckTfTypeOrderVsPython( T2_Z )

        # Simple multiple inheritance test
        class T3_A(object): pass
        class T3_B(object): pass
        class T3_Z(T3_A, T3_B): pass
        tT3_A = Tf.Type.Define(T3_A)
        tT3_B = Tf.Type.Define(T3_B)
        tT3_Z = Tf.Type.Define(T3_Z)
        self.assertEqual((tT3_Z, tT3_A, tT3_B, tObject, self.tRoot), tT3_Z.GetAllAncestorTypes())
        CheckTfTypeOrderVsPython( T3_Z )

        # Complex multiple inheritance test.
        #
        # This is an example where a simple depth-first traversal of base types
        # gives non-intuitive results.  This was taken from the Python 2.3 release
        # notes, here:
        #
        #   http://www.python.org/download/releases/2.3/mro/
        #
        class T4_A(object): pass
        class T4_B(object): pass
        class T4_C(object): pass
        class T4_D(object): pass
        class T4_E(object): pass
        class T4_K1(T4_A,T4_B,T4_C): pass
        class T4_K2(T4_D,T4_B,T4_E): pass
        class T4_K3(T4_D,T4_A): pass
        class T4_Z(T4_K1,T4_K2,T4_K3): pass
        tT4_A = Tf.Type.Define(T4_A)
        tT4_B = Tf.Type.Define(T4_B)
        tT4_C = Tf.Type.Define(T4_C)
        tT4_D = Tf.Type.Define(T4_D)
        tT4_E = Tf.Type.Define(T4_E)
        tT4_K1 = Tf.Type.Define(T4_K1)
        tT4_K2 = Tf.Type.Define(T4_K2)
        tT4_K3 = Tf.Type.Define(T4_K3)
        tT4_Z = Tf.Type.Define(T4_Z)
        self.assertEqual(tT4_Z.GetAllAncestorTypes(),
            (tT4_Z, tT4_K1, tT4_K2, tT4_K3, tT4_D, tT4_A, tT4_B, tT4_C, tT4_E,
            tObject, self.tRoot))
        CheckTfTypeOrderVsPython( T4_Z )

        # Inconsistent inheritance.
        #
        # Python prevents us from declaring a type hierarchy with inconsistent
        # inheritance:
        #
        #    >>> class A(object): pass
        #    ...
        #    >>> class B(object): pass
        #    ...
        #    >>> class X(A,B): pass
        #    ...
        #    >>> class Y(B,A): pass
        #    ...
        #    >>> class Z(X,Y): pass
        #    ...
        #    TypeError: Cannot create a consistent method resolution order (MRO)
        #    for bases A, B
        #
        # Since we're testing for exactly this error, as detected by the TfType
        # system, we test this from C++ instead, in test/typeMultipleInheritance.cpp.

if __name__ == '__main__':
    unittest.main()
