#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.


# Test to ensure loading and unloading of large nested instance paths, do not
# cause a crash, resulting from a race between 2 threads computing subtrees.


import unittest

class TestUsdLoadUnloadDeepNestedInstancing(unittest.TestCase):
    # 1. Tests a scenario where normal prim and prototypes for an instance under
    # this normal prim are present in subtreesToLoad in parallel 
    def test_PrototypeIsDescendant(self):
        from pxr import Sdf
        from pxr import Usd

        stage = Usd.Stage.Open("Mountain.usd")
        self.assertEqual(len(stage.GetPrototypes()), 60)

        parentToInstances = Sdf.Path('/Mountain')
        pathsToLoad = [Sdf.Path('/Mountain/Rocks1')]
        pathsToUnload = [Sdf.Path('/Mountain/Rocks'+str(n))
                         for n in [1, 2, 3, 4, 5]]

        # unload everything
        stage.Unload()
        self.assertEqual(len(stage.GetPrototypes()), 0)

        stage.LoadAndUnload(tuple([parentToInstances]), tuple())
        self.assertEqual(len(stage.GetPrototypes()), 60)

        # Following should not cause a crash
        stage.LoadAndUnload(tuple(pathsToLoad), tuple(pathsToUnload))
        self.assertEqual(len(stage.GetPrototypes()), 60)

    # Test the scenario where an instance under a prototype have the same
    # sourcePath as another prototype's sourcePath. 
    # Tests the scenario where 2 prototypes' sourcePath have an
    # ancestor/descendant relationship. 
    def test_PrototypeAndNestedPrototypePath(self):
        from pxr import Sdf
        from pxr import Usd

        stage = Usd.Stage.Open("Earth.usd")
        self.assertEqual(len(stage.GetPrototypes()), 61)

        pathToUnload = None
        # find a nested instance (representing a source for a prototype) to unload
        for m in stage.GetPrototypes():
            if 'Pebble' in m._GetSourcePrimIndex().rootNode.path.pathString:
                pathToUnload = m._GetSourcePrimIndex().rootNode.path
                break

        stage.LoadAndUnload(tuple(), tuple([pathToUnload]))
        self.assertEqual(len(stage.GetPrototypes()), 62)

if __name__ == "__main__":
    unittest.main()

