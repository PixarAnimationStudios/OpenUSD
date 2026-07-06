#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Gf, Tf

# Test GfIsFloatingPointType with TfType -- floating point types return True.
assert Gf.IsFloatingPointType(Tf.Type.FindByName('double'))
assert Gf.IsFloatingPointType(Tf.Type.FindByName('float'))
assert Gf.IsFloatingPointType(Tf.Type.FindByName('pxr_half::half'))

# Test GfIsFloatingPointType with TfType -- non-FP types return False.
assert not Gf.IsFloatingPointType(Tf.Type.FindByName('int'))
assert not Gf.IsFloatingPointType(Tf.Type.FindByName('string'))
assert not Gf.IsFloatingPointType(Tf.Type.FindByName('bool'))
assert not Gf.IsFloatingPointType(Tf.Type())

print('OK')
