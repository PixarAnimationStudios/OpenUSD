#!/pxrpythonsubst

from Mentor.Runtime import FindDataFile

from pxr import Sdf, Tf, Usd


def TestBug119633():

    layerFile = FindDataFile('testUsdBug119633.testenv/root.usda')
    assert layerFile, 'failed to find "testUsdBug119633.testenv/root.usda'

    Sdf.Layer.FindOrOpen(layerFile)
    try:
        stage = Usd.Stage.Open(layerFile)
        assert stage, 'failed to create stage for %s' % layerFile
        prim = stage.GetPrimAtPath('/SardineGroup_OceanA')
        assert prim, 'failed to find prim /World'
    except Tf.ErrorException:
        pass


if __name__ == "__main__":
    TestBug119633()
    print 'OK'
