#!/pxrpythonsubst
from Mentor.Runtime import FindDataFile
from pxr import Sdf

# Test the customLayerData API via Sdf's Layer API
def TestCustomLayerDataBasicUsage():
    filePath = FindDataFile('testSdfCustomLayerData.testenv/layerAccess.sdf')
    layer = Sdf.Layer.FindOrOpen(filePath)
    assert layer

    expectedValue = { 'layerAccessId' : 'id',
                      'layerAccessAssetPath' : Sdf.AssetPath('/layer/access.sdf'),
                      'layerAccessRandomNumber' : 5 }
    assert layer.customLayerData == expectedValue

    assert layer.HasCustomLayerData()
    layer.ClearCustomLayerData()
    assert not layer.HasCustomLayerData()

    newValue = { 'newLayerAccessId' : 'newId',
                 'newLayerAccessAssetPath' : Sdf.AssetPath('/new/layer/access.sdf'),
                 'newLayerAccessRandomNumber' : 1 }
    layer.customLayerData = newValue
    assert layer.customLayerData == newValue

    assert layer.HasCustomLayerData()
    layer.ClearCustomLayerData()
    assert not layer.HasCustomLayerData()

if __name__ == '__main__':
    TestCustomLayerDataBasicUsage()
    print 'OK'
