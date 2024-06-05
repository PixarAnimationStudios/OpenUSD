#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def testUsdviewInputFunction(appController):
    from pxr import Sdf
    
    assert Sdf.Layer.Find('root.usd').IsDetached()
    assert not Sdf.Layer.Find('foo.usd').IsDetached()
    assert Sdf.Layer.Find('bar.usd').IsDetached()
    assert not Sdf.Layer.Find('with spaces.usd').IsDetached()
