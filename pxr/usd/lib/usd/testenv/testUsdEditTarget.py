#!/pxrpythonsubst

import os, sys
from Mentor.Runtime import (SetAssertMode, MTR_EXIT_TEST, FindDataFile,
                            Assert, AssertTrue, AssertFalse, AssertEqual,
                            AssertNotEqual, AssertException,
                            RequiredException,
                            ExpectedErrorBegin, ExpectedErrorEnd, ExitTest)

from pxr import Gf, Tf, Sdf, Pcp, Usd

def TestPathTranslationAndValueResolution():

    layerFile = FindDataFile('testUsdEditTarget.testenv/test.usda')
    assert layerFile, 'failed to find "testUsdEditTarget.testenv/test.usda'

    layer = Sdf.Layer.FindOrOpen(layerFile)
    stage = Usd.Stage.Open(layerFile)
    assert stage, 'failed to create stage for %s' % layerFile

    prim = stage.GetPrimAtPath('/Sarah')
    assert prim, 'failed to find prim /Sarah'

    primIndex = prim.GetPrimIndex()

    Sarah_node = primIndex.rootNode
    class_Sarah_node = Sarah_node.children[0]
    Sarah_displayColor_red_node = Sarah_node.children[1]
    Sarah_Defaults_node = Sarah_node.children[2]
    Sarah_Base_node = Sarah_Defaults_node.children[0]
    Sarah_Base_displayColor_red_node = Sarah_Base_node.children[0]

    def MakeEditTarget(node):
        return Usd.EditTarget(stage.GetRootLayer(), node)

    Sarah = MakeEditTarget(Sarah_node)
    class_Sarah = MakeEditTarget(class_Sarah_node)
    Sarah_displayColor_red = MakeEditTarget(Sarah_displayColor_red_node)
    Sarah_Defaults = MakeEditTarget(Sarah_Defaults_node)
    Sarah_Base = MakeEditTarget(Sarah_Base_node)
    Sarah_Base_displayColor_red = MakeEditTarget(
        Sarah_Base_displayColor_red_node)

    # Test path translation across reference, inherit, and variant.
    def CheckPath(target, scenePath, specPath):
        result = target.MapToSpecPath(scenePath)
        if result != specPath:
            raise AssertionError, '%s -> %s, expected %s -> %s' % (
                scenePath, result, scenePath, specPath)

    CheckPath(Sarah, '/Sarah', '/Sarah')
    CheckPath(Sarah, '/Sarah/child', '/Sarah/child')
    CheckPath(Sarah, '/Sarah.x[/Sarah.y]', '/Sarah.x[/Sarah.y]')

    CheckPath(class_Sarah, '/Sarah', '/_class_Sarah')
    CheckPath(class_Sarah, '/Sarah/child', '/_class_Sarah/child')
    CheckPath(class_Sarah,
              '/Sarah.x[/Sarah.y]', '/_class_Sarah.x[/_class_Sarah.y]')

    CheckPath(Sarah_displayColor_red,
              '/Sarah', '/Sarah{displayColor=red}')
    CheckPath(Sarah_displayColor_red,
              '/Sarah/child', '/Sarah{displayColor=red}child')
    CheckPath(Sarah_displayColor_red,
              '/Sarah.x[/Sarah.y]', '/Sarah{displayColor=red}.x[/Sarah.y]')

    CheckPath(Sarah_Defaults, '/Sarah', '/Sarah_Defaults')
    CheckPath(Sarah_Defaults, '/Sarah/child', '/Sarah_Defaults/child')
    CheckPath(Sarah_Defaults,
              '/Sarah.x[/Sarah.y]', '/Sarah_Defaults.x[/Sarah_Defaults.y]')

    CheckPath(Sarah_Base, '/Sarah', '/Sarah_Base')
    CheckPath(Sarah_Base, '/Sarah/child', '/Sarah_Base/child')
    CheckPath(Sarah_Base, '/Sarah.x[/Sarah.y]', '/Sarah_Base.x[/Sarah_Base.y]')

    CheckPath(Sarah_Base_displayColor_red,
              '/Sarah', '/Sarah_Base{displayColor=red}')
    CheckPath(Sarah_Base_displayColor_red,
              '/Sarah/child', '/Sarah_Base{displayColor=red}child')
    CheckPath(Sarah_Base_displayColor_red,
              '/Sarah.x[/Sarah.y]',
              '/Sarah_Base{displayColor=red}.x[/Sarah_Base.y]')

    ########################################################################
    return
    ########################################################################
    # XXX !  The following portion of this test is disabled since Usd has no API
    # for composing up to some point.  This should be reenabled when that's
    # available.

    # Test value resolution across reference, inherit, and variant.
    def CheckValue(obj, key, target, expected):
        result = obj.ComposeInfo(
            key, defVal=None, editTarget=target,
            excerptType=Csd.ExcerptTypeAll, composeInfo=None)
        if not Gf.IsClose(result, expected, 1e-4):
            raise AssertionError, ("Got '%s' resolving '%s' on '%s', expected "
                                   "'%s'" % (result, key, obj.path, expected))

    displayColor = scene.GetObjectAtPath('/Sarah.displayColor')

    CheckValue(displayColor, 'default', Sarah,
               Gf.Vec3d(0.1, 0.2, 0.3))
    CheckValue(displayColor, 'default', class_Sarah,
               Gf.Vec3d(1, 1, 1))
    CheckValue(displayColor, 'default', Sarah_displayColor_red,
               Gf.Vec3d(1, 0, 0))
    CheckValue(displayColor, 'default', Sarah_Defaults,
               Gf.Vec3d(0, 0, 1))
    CheckValue(displayColor, 'default', Sarah_Base,
               Gf.Vec3d(0.8, 0, 0))
    CheckValue(displayColor, 'default', Sarah_Base_displayColor_red,
               Gf.Vec3d(0.8, 0, 0))
    ########################################################################


def TestStageEditTargetAPI():

    def OpenLayer(name):
        fullName = 'testUsdEditTarget.testenv/%s.usda' % name
        layerFile = FindDataFile(fullName)
        assert layerFile, 'failed to find @%s@' % fullName
        layer = Sdf.Layer.FindOrOpen(layerFile)
        assert layer, 'failed to open layer @%s@' % fullName
        return layer

    # Open stage.
    layer = OpenLayer('testAPI_root')
    stage = Usd.Stage.Open(layer.identifier)
    assert stage, 'failed to create stage for @%s@' % layer.identifier

    # Check GetLayerStack behavior.
    assert stage.GetLayerStack()[0] == stage.GetSessionLayer()

    # Get LayerStack without session layer.
    rootLayer, subLayer1, subLayer2 = \
        stage.GetLayerStack(includeSessionLayers=False)
    assert subLayer1 and subLayer2, ('expected @%s@ to have 2 sublayers' %
                                     layer.identifier)
    assert rootLayer == stage.GetRootLayer()

    # Get Sarah prim.
    prim = stage.GetPrimAtPath('/Sarah')
    assert prim, 'failed to find prim /Sarah'

    # Sanity check simple composition.
    assert prim.GetAttribute('color').Get() == Gf.Vec3d(1,1,1)
    assert prim.GetAttribute('sub1Color').Get() == Gf.Vec3d(1,1,1)
    assert prim.GetAttribute('sub2Color').Get() == Gf.Vec3d(1,1,1)

    # Should start out with EditTarget being local & root layer.
    assert (stage.GetEditTarget().IsLocalLayer() and
            stage.GetEditTarget().GetLayer() == stage.GetRootLayer())

    # Set EditTarget to sublayers.
    stage.SetEditTarget(subLayer1)
    assert stage.GetEditTarget() == subLayer1, 'failed to set EditTarget'

    stage.SetEditTarget(subLayer2)
    assert stage.GetEditTarget() == subLayer2, 'failed to set EditTarget'

    stage.SetEditTarget(stage.GetRootLayer())
    assert stage.GetEditTarget() == stage.GetRootLayer(), \
        'failed to set EditTarget'

    # Try authoring to sublayers using context object.
    with Usd.EditContext(stage, subLayer2):
        prim.GetAttribute('sub2Color').Set(Gf.Vec3d(3,4,5));
        assert prim.GetAttribute('sub2Color').Get() == Gf.Vec3d(3,4,5)
        assert not rootLayer.GetAttributeAtPath('/Sarah.sub2Color')
        assert (subLayer2.GetAttributeAtPath('/Sarah.sub2Color').default ==
                Gf.Vec3d(3,4,5))

    # Target should be back to root layer.
    assert stage.GetEditTarget() == stage.GetRootLayer(), \
        'EditContext failed to restore EditTarget'

    # Set to subLayer1.
    stage.SetEditTarget(subLayer1)

    # Try authoring to session layer using context object.
    sessionLayer = stage.GetSessionLayer()
    with Usd.EditContext(stage, sessionLayer):
        assert stage.GetEditTarget() == sessionLayer
        assert not sessionLayer.GetAttributeAtPath('/Sarah.color')
        prim.GetAttribute('color').Set(Gf.Vec3d(9,9,9));
        assert prim.GetAttribute('color').Get() == Gf.Vec3d(9,9,9)
        assert (sessionLayer.GetAttributeAtPath('/Sarah.color').default ==
                Gf.Vec3d(9,9,9))
        assert (rootLayer.GetAttributeAtPath('/Sarah.color').default ==
                Gf.Vec3d(1,1,1))

    # Target should be back to subLayer1.
    assert stage.GetEditTarget() == subLayer1, \
        'EditContext failed to restore EditTarget'

    # Verify an error is reported for setting EditTarget as a local layer
    # that's not in the local LayerStack.
    anon = Sdf.Layer.CreateAnonymous()
    assert anon
    with RequiredException(RuntimeError):
        stage.SetEditTarget(anon)

if __name__ == "__main__":
    TestPathTranslationAndValueResolution()
    TestStageEditTargetAPI()
    print 'OK'

