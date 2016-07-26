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

import sys
from pxr import Tf

import unittest

status = 0

f1called = False
def f1():
    global f1called
    print 'called to python!'
    f1called = True

def f2():
    return 'called to python, return string!'

def f3(arg1, arg2):
    print 'args', arg1, arg2

def f4(stringArg):
    return 'got string ' + stringArg

class MyBase(Tf._TestBase):
    def __init__(self):
        Tf._TestBase.__init__(self)
    def Virtual(self):
        return 'python base'
    def Virtual2(self):
        print 'python base v2'
    def Virtual3(self, arg):
        print 'python base v3', arg
    def UnwrappedVirtual(self):
        return 'unwrapped virtual'

class MyDerived(Tf._TestDerived):
    def Virtual(self):
        return 'python derived'
    def Virtual2(self):
        print 'python derived v2'
    def Virtual3(self, arg):
        print 'python derived v3', arg
    def Virtual4(self):
        return 'python derived v4'

class Raiser(Tf._TestBase):
    def __init__(self):
        Tf._TestBase.__init__(self)
    def Virtual(self):
        raise RuntimeError('testing exception stuff')
    def Virtual2(self):
        print 'raiser v2'
    def Virtual3(self, arg):
        print 'py virtual 3', arg

class TestPython(unittest.TestCase):

    def test_BaseDerived(self):
        mb = MyBase()
        d = Tf._TestDerived()
        md = MyDerived()

        self.assertEqual('unwrapped virtual', mb.TestCallVirtual())

        self.assertEqual('cpp base', mb.Virtual4())
        self.assertEqual('python derived v4', md.Virtual4())

        self.assertEqual((False, 'python base'), Tf._TakesBase(mb))
        self.assertEqual((True, 'cpp derived'), Tf._TakesBase(d))
        self.assertEqual((True, 'python derived'), Tf._TakesBase(md))

        self.assertEqual('python base', Tf._TakesConstBase(mb))
        self.assertEqual('cpp derived', Tf._TakesConstBase(d))
        self.assertEqual('python derived', Tf._TakesConstBase(md))

        self.assertEqual('cpp derived', Tf._TakesDerived(d))
        self.assertEqual('python derived', Tf._TakesDerived(md))

        self.assertIs(Tf._ReturnsConstBase(md), md)
        self.assertIs(Tf._ReturnsBase(md), md)
        self.assertIs(Tf._ReturnsBaseRefPtr(md), md)

        try:
            Tf._TestBase().Virtual()
            assert False, 'calling pure virtual'
        except:
            pass


    def test_Factory(self):
        df = Tf._DerivedFactory()
        self.assertIsInstance(df, Tf._TestDerived)
        self.assertTrue(hasattr(df, '__owner'))
        self.assertEqual((True, 'cpp derived'), Tf._TakesBase(df))
        Tf._TakesReference(df)
        self.assertFalse(hasattr(df, '__owner'))

        self.assertIs(Tf._DerivedNullFactory(), None)


    def test_Exception(self):
        with self.assertRaises(RuntimeError):
            Tf._TakesBase(Raiser())

        with self.assertRaises(Tf.ErrorException) as cm:
            Tf._mightRaise(True)
        for x in cm.exception:
            self.assertTrue(len(repr(x)))

        with self.assertRaises(Tf.ErrorException):
            Tf.RaiseCodingError("some error")

        with self.assertRaises(Tf.ErrorException):
            Tf.RaiseRuntimeError("some error")

        with self.assertRaises(Tf.ErrorException):
            Tf._doErrors()


    def test_StaticMethodPosting(self):
        with self.assertRaises(Tf.ErrorException):
            Tf._TestStaticMethodError.Error()

        Tf.Warn("expected warning, for coverage")

        Tf.Status("expected status message, for coverage")

    def test_NoticeListener(self):
        global noticesHandled
        noticesHandled = 0

        def HandleNotice(notice, sender):
            global noticesHandled
            noticesHandled += 1

        listener = Tf.Notice.RegisterGlobally('TfNotice', HandleNotice)
        self.assertEqual(0, noticesHandled)
        Tf.Notice().SendGlobally()
        self.assertEqual(1, noticesHandled)
        listener.Revoke()
        Tf.Notice().SendGlobally()
        self.assertEqual(1, noticesHandled)

        # Raise in notice listener
        def RaiseOnNotice(notice, sender):
            raise RuntimeError('got notice!')

        class CustomTestNotice(Tf.Notice):
            pass
        Tf.Type.Define(CustomTestNotice)

        listener = Tf.Notice.RegisterGlobally(CustomTestNotice, RaiseOnNotice)
        with self.assertRaises(RuntimeError):
            CustomTestNotice().SendGlobally()

        # Register a bad notice
        with self.assertRaises(TypeError):
            listener = Tf.Notice.RegisterGlobally('BogusNotice', HandleNotice)


    def test_Enums(self):
        Tf._takesTfEnum(Tf._Alpha)
        Tf._takesTfEnum(Tf._Delta)

        self.assertEqual(Tf._Delta, Tf._returnsTfEnum(Tf._Delta))
        self.assertIs(Tf._returnsTfEnum(Tf._Delta), Tf._Delta)

        Tf._takesTfEnum(Tf._Enum.One)

        Tf._takesTestEnum(Tf._Alpha)
        Tf._takesTestEnum2(Tf._Enum.One)


    def test_EnumComparisons(self):
        self.assertTrue(Tf._Alpha < Tf._Bravo < Tf._Charlie < Tf._Delta)
        self.assertTrue(Tf._Delta > Tf._Charlie > Tf._Bravo > Tf._Alpha)
        self.assertTrue(Tf._Alpha != Tf._Bravo != Tf._Charlie != Tf._Delta)
        self.assertTrue(Tf._Alpha != Tf._Enum.One)
        self.assertTrue(Tf._Alpha > Tf._Enum.One)
        self.assertTrue(Tf._Alpha >= Tf._Alpha)
        self.assertTrue(Tf._Alpha <= Tf._Delta)
        self.assertTrue(Tf._Charlie >= Tf._Bravo)


    def test_EnumValuesRemovedFromTypeScope(self):
        with self.assertRaises(AttributeError):
            Tf._takesTestEnum(Tf._TestEnum._Alpha)

        self.assertEqual((Tf._Alpha, Tf._Bravo, Tf._Charlie, Tf._Delta),
                Tf._TestEnum.allValues)

        with self.assertRaises(TypeError):
            Tf._takesTestEnum(Tf._Enum.One)
        with self.assertRaises(TypeError):
            Tf._takesTestEnum2(Tf._Alpha)

        self.assertEqual((Tf._Enum.One, Tf._Enum.Two, Tf._Enum.Three),
                Tf._Enum.TestEnum2.allValues)

        self.assertEqual(1, Tf._Enum.One.value)
        self.assertEqual(2, Tf._Enum.Two.value)
        self.assertEqual(3, Tf._Alpha.value)
        self.assertEqual('A', Tf._Alpha.displayName)
        self.assertEqual('B', Tf._Bravo.displayName)
        self.assertEqual(Tf._Alpha, Tf.Enum.GetValueFromFullName(Tf._Alpha.fullName))
        self.assertEqual(None, Tf.Enum.GetValueFromFullName("invalid_enum_name"))

        self.assertTrue(Tf._Enum.One == 1)
        self.assertTrue(Tf._Enum.Two == 2)
        self.assertTrue(Tf._Alpha == 3)

        self.assertTrue(1 == Tf._Enum.One)
        self.assertTrue(2 == Tf._Enum.Two)
        self.assertTrue(3 == Tf._Alpha)

        self.assertTrue(Tf._Alpha | Tf._Alpha is Tf._Alpha)
        self.assertTrue(Tf._Alpha & Tf._Alpha is Tf._Alpha)
        self.assertTrue(Tf._Alpha == 3)
        self.assertTrue(Tf._Alpha | 1 is Tf._Alpha)
        self.assertTrue(2 | Tf._Alpha is Tf._Alpha)
        self.assertTrue(4 | Tf._Alpha == 7)

        self.assertTrue(Tf._Alpha & 3 is Tf._Alpha)
        self.assertTrue(3 & Tf._Alpha is Tf._Alpha)
        self.assertTrue(2 & Tf._Alpha == 2)

        self.assertTrue(Tf._Enum.One ^ Tf._Enum.Two == 3)
        self.assertTrue(4 ^ Tf._Alpha == 7)
        self.assertTrue(Tf._Alpha ^ 4 == 7)


    def test_EnumRegistrationCollision(self):
        with self.assertRaises(Tf.ErrorException):
            Tf._registerInvalidEnum(Tf)


    def test_EnumInvalidBitwiseOperations(self):
        '''Bitwise operations are not permitted between enum values of different types.'''
        with self.assertRaises(TypeError):
            Tf._Alpha & Tf._Enum.Two
            assert False, "Should not permit bitwise AND between different enum types"

        with self.assertRaises(TypeError):
            Tf._Alpha | Tf._Enum.Two

        with self.assertRaises(TypeError):
            Tf._Alpha ^ Tf._Enum.Two

    def test_EnumOneObjectPerUniqueValue(self):
        '''Only one object should be created for each unique value per type.'''
        value1 = Tf._Alpha | Tf._Delta
        value2 = Tf._Alpha | Tf._Delta
        self.assertIs(value1, value2)

    def test_EnumConversion(self):
        value1 = Tf._Alpha | Tf._Delta
        # Conversions of TfEnum objects to python should retain the correct type.'''
        self.assertIs(Tf._returnsTfEnum(value1), value1)

        # The auto-generated python object should be convertible to the original type.
        Tf._takesTestEnum(value1)

    def test_Callbacks(self):
        global f1called
        Tf._callback(f1)
        self.assertTrue(f1called)

        self.assertEqual('called to python, return string!', Tf._stringCallback(f2))

        self.assertEqual('got string c++ is calling...', Tf._stringStringCallback(f4))

        with self.assertRaises(TypeError):
            Tf._callback(f3)
        with self.assertRaises(TypeError):
            Tf._stringCallback(f1)


    def test_WeakStrongRefCallbacks(self):
        class Foo(object):
            def method(self):
                return 'python method'
        f = Foo()
        m = f.method

        # the callback is an instancemethod, it should not keep the object 
        # alive.
        Tf._setTestCallback(m)
        self.assertEqual('python method', Tf._invokeTestCallback())
        del f
        del m
        print 'expected warning...'
        self.assertEqual('', Tf._invokeTestCallback())
        print 'end of expected warning'

        l = lambda : 'python lambda'

        # the callback is a lambda, it should stay alive (and will keep f alive)
        Tf._setTestCallback(l)
        self.assertEqual('python lambda', Tf._invokeTestCallback())
        del l
        self.assertEqual('python lambda', Tf._invokeTestCallback())


        # the callback is a function, it should not stay alive
        def func():
            return 'python func'

        Tf._setTestCallback(func)
        self.assertEqual('python func', Tf._invokeTestCallback())
        del func
        print 'expected warning...'
        self.assertEqual('', Tf._invokeTestCallback())
        print 'end of expected warning'

        del Foo


    def test_Singleton(self):
        class Foo(Tf.Singleton):
            def init(self):
                print 'Initializing singleton'
                self._value = 100
            def GetValue(self):
                return self._value
            def SetValue(self, value):
                self._value = value

        # Get the singleton instance (first time causes creation)
        f = Foo()

        # Subsequent times do not cause creation
        f = Foo()

        # Always get same instance (there is only one)
        f is Foo() and Foo() is Foo()

        self.assertEqual(100, Foo().GetValue())

        Foo().SetValue(123)

        self.assertEqual(123, Foo().GetValue())


    def test_TfPyClassMethod(self):
        c = Tf._ClassWithClassMethod()

        # Test classmethod invokation.
        def _TestCallable():
            return 123
        self.assertEqual((Tf._ClassWithClassMethod, 123), c.Test(_TestCallable))

        # Test classmethod error handling.
        class _TestException(Exception):
            '''A sample exception to raise.'''
            pass
        def _TestCallableWithException():
            raise _TestException()

        with self.assertRaises(_TestException):
            c.Test(_TestCallableWithException)


    def test_Debug(self):
        # should allow setting TfDebug's output file to either stdout or 
        # stderr, but not other files.
        Tf.Debug.SetOutputFile(sys.__stdout__)
        Tf.Debug.SetOutputFile(sys.__stderr__)

        # other files not allowed.
        import os, tempfile
        fname = tempfile.mkstemp()[1]
        f = open(fname, 'w')
        
        with self.assertRaises(Tf.ErrorException):
            Tf.Debug.SetOutputFile(f)

        f.close()
        os.remove(fname)

        # argument checking.
        with self.assertRaises(TypeError):
            Tf.Debug.SetOutputFile(1234)


    def test_ExceptionWithoutCurrentThreadState(self):
        with self.assertRaises(RuntimeError):
            Tf._ThrowCppException()


    def test_TakeVectorOfVectorOfStrings(self):
        self.assertEqual(4, Tf._TakesVecVecString([['1', '2', '3'], ['4', '5'], [], ['6']]))


    def test_TfPyObjWrapper(self):
        self.assertEqual('a', Tf._RoundTripWrapperTest('a'))
        self.assertEqual(1234, Tf._RoundTripWrapperTest(1234))
        self.assertEqual([], Tf._RoundTripWrapperTest([]))
        self.assertEqual(None, Tf._RoundTripWrapperTest(None))

        self.assertEqual('a', Tf._RoundTripWrapperCallTest(lambda: 'a'))
        self.assertEqual(1234, Tf._RoundTripWrapperCallTest(lambda: 1234))
        self.assertEqual([], Tf._RoundTripWrapperCallTest(lambda: []))
        self.assertEqual(None, Tf._RoundTripWrapperCallTest(lambda: None))

        self.assertEqual('a', Tf._RoundTripWrapperIndexTest(['a','b'], 0))
        self.assertEqual('b', Tf._RoundTripWrapperIndexTest(['a','b'], 1))
        self.assertEqual(4, Tf._RoundTripWrapperIndexTest([1,2,3,4], -1))

    def test_TfMakePyConstructorWithVarArgs(self):
        with self.assertRaises(TypeError):
            Tf._ClassWithVarArgInit()

        def CheckResults(c, allowExtraArgs, args, kwargs):
            self.assertEqual(c.allowExtraArgs, allowExtraArgs)
            self.assertEqual(c.args, args)
            self.assertEqual(c.kwargs, kwargs)

        def StandardTests(allowExtraArgs):
            CheckResults(Tf._ClassWithVarArgInit(allowExtraArgs),
                        allowExtraArgs, (), {})
            CheckResults(Tf._ClassWithVarArgInit(allowExtraArgs, 1),
                        allowExtraArgs, (), {'a':1})
            CheckResults(Tf._ClassWithVarArgInit(allowExtraArgs, 1, 2, 3),
                        allowExtraArgs, (), {'a':1, 'b':2, 'c':3})
            CheckResults(Tf._ClassWithVarArgInit(allowExtraArgs, 1, 2, c=3),
                        allowExtraArgs, (), {'a':1, 'b':2, 'c':3})

        # Tests with extra arguments disallowed.
        StandardTests(allowExtraArgs=False)

        # These cases should emit an error because there are unexpected 
        # arguments
        with self.assertRaises(TypeError):
            Tf._ClassWithVarArgInit(False, 1, 2, 3, 4)

        with self.assertRaises(TypeError):
            Tf._ClassWithVarArgInit(False, d=4)

        # This should emit an error because we have multiple values for a single 
        # arg.
        with self.assertRaises(TypeError):
            Tf._ClassWithVarArgInit(False, 1, 2, 3, b=4)

        # Tests with extra arguments allowed.
        StandardTests(allowExtraArgs=True)

        CheckResults(Tf._ClassWithVarArgInit(True, 1, 2, 3, 4, 5),
                    True, (4,5), {'a':1, 'b':2, 'c':3})
        CheckResults(Tf._ClassWithVarArgInit(True, 1, 2, c=3, d=6, f=8),
                    True, (), {'a':1, 'b':2, 'c':3, 'd':6, 'f':8})
        CheckResults(Tf._ClassWithVarArgInit(True, 1, 2, 3, 4, d=8),
                    True, (4,), {'a':1, 'b':2, 'c':3, 'd':8})

if __name__ == '__main__':
    unittest.main()
