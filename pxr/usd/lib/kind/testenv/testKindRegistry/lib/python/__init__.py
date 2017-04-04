from pxr import Plug, Tf

class TestPlugPythonDerived1(Plug._TestPlugBase1):
    def GetTypeName(self):
        return 'TestPlugPythonDerived1'

Tf.Type.Define(TestPlugPythonDerived1)
