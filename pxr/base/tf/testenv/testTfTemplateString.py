#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

from pxr import Tf
import unittest

def Replace(tmpl, safe=False, **kw):
    ts = Tf.TemplateString(tmpl)
    result = ''
    if safe:
        result = ts.SafeSubstitute(kw)
    else:
        result = ts.Substitute(kw)
    print(repr(tmpl), ' -> ', repr(result))
    return result

class TestTfTemplateString(unittest.TestCase):
    
    def test_TemplateString(self):
        self.assertTrue(Tf.TemplateString().valid)
        self.assertTrue(Tf.TemplateString('').valid)
        self.assertFalse(Tf.TemplateString('${}').valid)
        self.assertTrue(Tf.TemplateString('$').valid)
        self.assertTrue(Tf.TemplateString('$$').valid)

        self.assertFalse(Tf.TemplateString('${} ${} ${} ').valid)
        self.assertEqual(3, len(Tf.TemplateString('${} ${} ${} ').GetParseErrors()))

        self.assertFalse(Tf.TemplateString('${').valid)
        self.assertTrue(Tf.TemplateString('${').GetParseErrors())

        t = Tf.TemplateString('${foo}')
        self.assertEqual('${foo}', t.template)
        self.assertTrue(t.valid)
        self.assertFalse(t.GetParseErrors())

    def test_Substitute(self):
        self.assertEqual('value', Replace('$var', var='value'))
        self.assertEqual('valued', Replace('${var}d', var='value'))
        self.assertEqual('value-value', Replace('$var-value', var='value'))
        self.assertEqual('0000', Replace('$var$var$var$var', var='0'))
        self.assertEqual('0.0.0.0', Replace('${var}.${var}.${var}.${var}', var='0'))

        self.assertEqual("//brave/b952/shot/b952_17/b952_17.menva", 
                Replace("//$unit/$prod/shot/$shot/$shot.menva",
                        unit='brave', prod='b952', shot='b952_17'))

        self.assertEqual("Please remit the $sum of $19.95", 
                Replace("Please remit the $$sum of $$$sum", sum='19.95'))

        Replace("Unreplaced placeholders ${are} awesome", safe=True)

        with self.assertRaises(Tf.ErrorException):
            Replace("Unreplaced placeholders ${are} awesome")

        with self.assertRaises(Tf.ErrorException):
            Replace("Invalid characters in placeholders ${are not awesome")

        with self.assertRaises(Tf.ErrorException):
            Replace("Never stop ${quoting")

        with self.assertRaises(Tf.ErrorException):
            Replace("${}")

        with self.assertRaises(Tf.ErrorException):
            Replace("${  }")


    def test_EmptyMapping(self):
        t = Tf.TemplateString("//$unit/$prod/shot/$shot/$shot.menva")
        m = t.GetEmptyMapping()
        self.assertTrue('unit' in m)
        self.assertTrue('prod' in m)
        self.assertTrue('shot' in m)


        t = Tf.TemplateString("${ }")
        m = t.GetEmptyMapping()
        self.assertEqual(0, len(m))
        self.assertFalse(t.valid)
        self.assertNotEqual(0, len(t.GetParseErrors()))

if __name__ == '__main__':
    unittest.main()

