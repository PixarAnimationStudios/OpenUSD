#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

from pxr import Sdf, Tf, Pcp
import unittest, sys

class TestPcpPathTranslation(unittest.TestCase):
    def _GetPcpCacheForLayer(self, rootLayerPath):
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)
        self.assertTrue(rootLayer, "Cannot open layer %s" % rootLayerPath)

        return Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

    ############################################################

    def test_PathTranslationWithVariants(self):
        """Tests path translation issues that were at the root of bug 77818"""
        print "TestPathTranslationWithVariants..."

        pcpCache = self._GetPcpCacheForLayer(
            "bug77818/unit_main_cam.sdf")

        # Verify that translating a path to a variant node keeps any relevant
        # variant selections.
        (primIndex, errors) = pcpCache.ComputePrimIndex("/main_cam/Lens")
        self.assertEqual(len(errors), 0)

        variantNode = primIndex.rootNode.children[0]
        self.assertEqual(variantNode.path, Sdf.Path("/main_cam{lenses=Cooke_S4}Lens"))
        self.assertEqual(Pcp.TranslatePathFromRootToNode(variantNode, "/main_cam/Lens"),
                    Sdf.Path("/main_cam{lenses=Cooke_S4}Lens"))
        self.assertEqual(Pcp.TranslatePathFromNodeToRoot(
                        variantNode, Sdf.Path("/main_cam{lenses=Cooke_S4}Lens")),
                    Sdf.Path("/main_cam/Lens"))

    ############################################################
    # The following test cases are pulled from testCsdLayering8.py
    ############################################################

    def test_LocalAbsoluteTargetPaths(self):
        print "TestLocalAbsoluteTargetPaths..."

        pcpCache = self._GetPcpCacheForLayer("Root.sdf")

        errors = []
        pathMap = {"/World/Chars/Ref1.localSelfAbs" : "/World/Chars/Ref1",
                   "/World/Chars/Ref1.localParentAbs" : "/World/Chars",
                   "/World/Chars/Ref1.localChildAbs" : "/World/Chars/Ref1/RefChild",
                   "/World/Chars/Ref1/RefChild.localSelfAbs" : "/World/Chars/Ref1/RefChild",
                   "/World/Chars/Ref1/RefChild.localParentAbs" : "/World/Chars/Ref1",

                   "/World/Chars/Ref2.localSelfAbs" : "/World/Chars/Ref2",
                   "/World/Chars/Ref2.localParentAbs" : "/World/Chars",
                   "/World/Chars/Ref2.localChildAbs" : "/World/Chars/Ref2/RefChild",
                   "/World/Chars/Ref2/RefChild.localSelfAbs" : "/World/Chars/Ref2/RefChild",
                   "/World/Chars/Ref2/RefChild.localParentAbs" : "/World/Chars/Ref2",

                   "/World/Ref3.localSelfAbs" : "/World/Ref3",
                   "/World/Ref3.localParentAbs" : "/World",
                   "/World/Ref3.localChildAbs" : "/World/Ref3/RefChild",
                   "/World/Ref3/RefChild.localSelfAbs" : "/World/Ref3/RefChild",
                   "/World/Ref3/RefChild.localParentAbs" : "/World/Ref3"}

        for (path, expectedTargetPath) in pathMap.items():
            (curTargetPaths, curErrors) = \
                pcpCache.ComputeRelationshipTargetPaths(path)

            for err in curErrors:
                print >> sys.stderr, err
            errors += curErrors

            self.assertEqual(curTargetPaths, [Sdf.Path(expectedTargetPath)])

        self.assertEqual(len(errors), 0)

    def test_LocalRelativeTargetPaths(self):
        print "TestLocalRelativeTargetPaths..."

        pcpCache = self._GetPcpCacheForLayer("Root.sdf")

        errors = []
        pathMap = {"/World/Chars/Ref1.localSelfRel" : ".",
                   "/World/Chars/Ref1.localParentRel" : "..",
                   "/World/Chars/Ref1.localChildRel" : "RefChild",
                   "/World/Chars/Ref1/RefChild.localSelfRel" : ".",
                   "/World/Chars/Ref1/RefChild.localParentRel" : "..",

                   "/World/Chars/Ref2.localSelfRel" : ".",
                   "/World/Chars/Ref2.localParentRel" : "..",
                   "/World/Chars/Ref2.localChildRel" : "RefChild",
                   "/World/Chars/Ref2/RefChild.localSelfRel" : ".",
                   "/World/Chars/Ref2/RefChild.localParentRel" : "..",
                   
                   "/World/Ref3.localSelfRel" : ".",
                   "/World/Ref3.localParentRel" : "..",
                   "/World/Ref3.localChildRel" : "RefChild",
                   "/World/Ref3/RefChild.localSelfRel" : ".",
                   "/World/Ref3/RefChild.localParentRel" : ".."}

        for (path, expectedTargetPath) in pathMap.items():
            relPath = Sdf.Path(path)
            (curTargetPaths, curErrors) = pcpCache.ComputeRelationshipTargetPaths(relPath)

            for err in curErrors:
                print >> sys.stderr, err
            errors += curErrors

            self.assertEqual(curTargetPaths, 
                        [Sdf.Path(expectedTargetPath).MakeAbsolutePath(relPath.GetPrimPath())])

        self.assertEqual(len(errors), 0)
        
    def test_ReferenceAbsoluteTargetPaths(self):
        print "TestReferenceAbsoluteTargetPaths..."

        pcpCache = self._GetPcpCacheForLayer("Root.sdf")

        errors = []
        pathMap = {"/World/Chars/Ref1.refSelfAbs" : "/World/Chars/Ref1",
                   "/World/Chars/Ref1.refChildAbs" : "/World/Chars/Ref1/RefChild",
                   "/World/Chars/Ref1/RefChild.refSelfAbs" : "/World/Chars/Ref1/RefChild",
                   "/World/Chars/Ref1/RefChild.refParentAbs" : "/World/Chars/Ref1",

                   "/World/Chars/Ref2.refSelfAbs" : "/World/Chars/Ref2",
                   "/World/Chars/Ref2.refChildAbs" : "/World/Chars/Ref2/RefChild",
                   "/World/Chars/Ref2/RefChild.refSelfAbs" : "/World/Chars/Ref2/RefChild",
                   "/World/Chars/Ref2/RefChild.refParentAbs" : "/World/Chars/Ref2",

                   "/World/Ref3.refSelfAbs" : "/World/Ref3",
                   "/World/Ref3.refChildAbs" : "/World/Ref3/RefChild",
                   "/World/Ref3/RefChild.refSelfAbs" : "/World/Ref3/RefChild",
                   "/World/Ref3/RefChild.refParentAbs" : "/World/Ref3"}

        for (path, expectedTargetPath) in pathMap.items():
            (curTargetPaths, curErrors) = \
                pcpCache.ComputeRelationshipTargetPaths(path)

            for err in curErrors:
                print >> sys.stderr, err
            errors += curErrors

            self.assertEqual(curTargetPaths, [Sdf.Path(expectedTargetPath)])

        self.assertEqual(len(errors), 0)

    def test_ReferenceRelativeTargetPaths(self):
        print "TestReferenceRelativeTargetPaths..."

        pcpCache = self._GetPcpCacheForLayer("Root.sdf")

        errors = []
        pathMap = {"/World/Chars/Ref1.refSelfRel" : ".",
                   "/World/Chars/Ref1.refChildRel" : "RefChild",
                   "/World/Chars/Ref1/RefChild.refSelfRel" : ".",
                   "/World/Chars/Ref1/RefChild.refParentRel" : "..",

                   "/World/Chars/Ref2.refSelfRel" : ".",
                   "/World/Chars/Ref2.refChildRel" : "RefChild",
                   "/World/Chars/Ref2/RefChild.refSelfRel" : ".",
                   "/World/Chars/Ref2/RefChild.refParentRel" : "..",

                   "/World/Ref3.refSelfRel" : ".",
                   "/World/Ref3.refChildRel" : "RefChild",
                   "/World/Ref3/RefChild.refSelfRel" : ".",
                   "/World/Ref3/RefChild.refParentRel" : ".."}

        for (path, expectedTargetPath) in pathMap.items():
            relPath = Sdf.Path(path)
            (curTargetPaths, curErrors) = pcpCache.ComputeRelationshipTargetPaths(relPath)

            for err in curErrors:
                print >> sys.stderr, err
            errors += curErrors

            self.assertEqual(curTargetPaths, 
                        [Sdf.Path(expectedTargetPath).MakeAbsolutePath(relPath.GetPrimPath())])
            
        self.assertEqual(len(errors), 0)

    def test_ReferenceErrorCases(self):
        print "TestReferenceErrorCases..."

        pcpCache = self._GetPcpCacheForLayer("Root.sdf")

        errors = []
        paths = ["/World/Chars/Ref1.refBadAbs",
                 "/World/Chars/Ref1.refBadRel",
                 "/World/Chars/Ref1/RefChild.refBadAbs",
                 "/World/Chars/Ref1/RefChild.refBadRel",

                 "/World/Chars/Ref2.refBadAbs",
                 "/World/Chars/Ref2.refBadRel",
                 "/World/Chars/Ref2/RefChild.refBadAbs",
                 "/World/Chars/Ref2/RefChild.refBadRel",

                 "/World/Ref3.refBadAbs",
                 "/World/Ref3.refBadRel",
                 "/World/Ref3/RefChild.refBadAbs",
                 "/World/Ref3/RefChild.refBadRel"]
        
        for path in paths:
            (targetPaths, curErrors) = \
                pcpCache.ComputeRelationshipTargetPaths(path)
            errors += curErrors

        for err in errors:
            print >> sys.stderr, err
            self.assertTrue(isinstance(err, Pcp.ErrorInvalidExternalTargetPath), 
                   "Unexpected Error: %s" % err)

        self.assertEqual(len(errors), len(paths))



if __name__ == "__main__":
    unittest.main()
