#!/pxrpythonsubst

from pxr import UsdUtils

from Mentor.Framework.RunTime import AssertEqual, FindDataFile

def testUsdUtilsComputeStageStats():
    expectedStageStats = {
        'masters': {
            'primCountsByType': {'untyped': 4, 'Mesh': 7, 'Xform': 2}, 
            'primCounts': {'activePrimCount': 13, 'inactivePrimCount': 0,
                            'instanceCount': 2, 'pureOverCount': 4, 
                            'totalPrimCount': 13}},
            'usedLayerCount': 1, 
        'primary': {
            'primCountsByType': {'untyped': 3, 'Scope': 2, 'Mesh': 16, 'Xform': 13},
            'primCounts': {'activePrimCount': 32, 'inactivePrimCount': 2,
                        'instanceCount': 6, 'pureOverCount': 1, 
                        'totalPrimCount': 34}}, 
        'modelCount': 8, 
        'instancedModelCount': 1, 
        'totalPrimCount': 47, 
        'totalInstanceCount': 8, 
        'masterCount': 4, 
        'assetCount': 1}

    AssertEqual(UsdUtils.ComputeUsdStageStats(FindDataFile('stageStats.usda')),
                expectedStageStats)

def main():
    testUsdUtilsComputeStageStats()
    
if __name__=="__main__":
    main()
