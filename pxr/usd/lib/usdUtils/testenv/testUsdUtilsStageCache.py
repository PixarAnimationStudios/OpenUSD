#!/pxrpythonsubst

# Verify that the UsdUtils.StageCache singleton does not crash upon system
# teardown.
def main():
    from pxr import UsdUtils, Sdf, Usd
    lyr = Sdf.Layer.CreateAnonymous()
    with Usd.StageCacheContext(UsdUtils.StageCache.Get()):
        stage = Usd.Stage.Open(lyr)

if __name__=="__main__":
    main()

