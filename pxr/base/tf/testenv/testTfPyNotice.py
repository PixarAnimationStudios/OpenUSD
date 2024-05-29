#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf
import unittest


class PyNoticeBase(Tf.Notice):
    def func(self):
        return 'func from PyNoticeBase'

class PyNoticeDerived(PyNoticeBase):
    def func(self):
        return 'func from PyNoticeDerived'

def testListening(regNoticeType, sendNotice, sender = None):
    class cbClass(object):
        def __init__(self, regType, sendNotice, sender):
            self.received = 0
            self.regType = regType
            self.sendNotice = sendNotice
            self.sender = sender
        def callback(self, n, s):
            assert n is self.sendNotice
            assert isinstance(self.regType, str) or \
                   isinstance(n, self.regType)
            assert s is self.sender
            self.received += 1
    cb = cbClass(regNoticeType, sendNotice, sender)
    l = Tf.Notice.RegisterGlobally(regNoticeType, cb.callback)
    if (sender):
        l = Tf.Notice.Register(regNoticeType, cb.callback, sender)
    if (sender):
        sendNotice.Send(sender)
    else:
        sendNotice.SendGlobally()
    return cb.received


class testPySender(object):
    pass

args = [
    # Global notices
    (1, "TfNotice", Tf.Notice()),
    (1, Tf.Notice, Tf.Notice()),
    (1, Tf.Notice, PyNoticeBase()),
    (1, PyNoticeBase, PyNoticeBase()),
    (1, PyNoticeBase, PyNoticeDerived()),
    (0, PyNoticeDerived, PyNoticeBase()),
    
    # Notices from C++ sender.
    (1, "TfNotice", Tf.Notice(), Tf._TestBase()),
    (1, Tf.Notice, Tf.Notice(), Tf._TestBase()),
    (1, Tf.Notice, PyNoticeBase(), Tf._TestBase()),
    (1, PyNoticeBase, PyNoticeBase(), Tf._TestBase()),
    (1, PyNoticeBase, PyNoticeDerived(), Tf._TestBase()),
    (0, PyNoticeDerived, PyNoticeBase(), Tf._TestBase()),

    # Notices from Python sender.
    (1, "TfNotice", Tf.Notice(), testPySender()),
    (1, Tf.Notice, Tf.Notice(), testPySender()),
    (1, Tf.Notice, PyNoticeBase(), testPySender()),
    (1, PyNoticeBase, PyNoticeBase(), testPySender()),
    (1, PyNoticeBase, PyNoticeDerived(), testPySender()),
    (0, PyNoticeDerived, PyNoticeBase(), testPySender())
    ]

class listenerClass(object):
    def __init__(self):
        self.received = 0
    def cb(self, n, s):
        self.received += 1

class TestTfPyNotice(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        Tf.Type.Define(PyNoticeBase)
        Tf.Type.Define(PyNoticeDerived)

    def test_Listening(self):
        for pkg in args:
            self.assertEqual(pkg[0], testListening(*pkg[1:]))


        lc = listenerClass()
        testSender = Tf._TestDerived()
        listener = Tf.Notice.Register(Tf.Notice, lc.cb, testSender)
        Tf._sendTfNoticeWithSender(testSender)
        self.assertEqual(1, lc.received)

    def test_UnsupportedSender(self):
        def cb(*args):
            pass

        with self.assertRaises(TypeError):
            Tf.Notice.Register("TfNotice", cb, 3)
        with self.assertRaises(TypeError):
            Tf.Notice().Send(3)
    
if __name__ == '__main__':
    unittest.main()
 
