#!/pxrpythonsubst
#
# Copyright 2024 Pixar
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
#
from pxr import Tf

import contextlib
import dataclasses
import os
import tempfile
import textwrap
from typing import Generator, List, Tuple
import unittest

@contextlib.contextmanager
def MallocTagTextFile(contents: str) -> Generator[str, None, None]:
    """
    Context object which returns the filepath of a temporary file that contains
    the given contents.
    """

    # Note that Windows has a strict file locking policy which prevents us from
    # having multiple handles into the same file. To workaround this, close out
    # our handle to it before yielding the path, and manually clean it up when 
    # we're done.
    try:
        with tempfile.NamedTemporaryFile(
            suffix=".mallocTag", mode="w+", delete=False) as tmp:
            tmp.write(contents)
            tmp.flush()
        yield tmp.name
    finally:
        os.remove(tmp.name)

@dataclasses.dataclass
class NodeData:
    """
    A simple dataclass for verifying malloc tag trees.
    """
    key: str
    inclusive: int
    exclusive: int
    samples: int
    children: List["NodeData"] = dataclasses.field(default_factory=list)

def GetNodeDataTree(tree: Tf.MallocTag.CallTree):
    def _GetNodeDataTree(node: Tf.MallocTag.CallTree.PathNode):
        """
        Convert a MallocTag CallTree into a NodeData tree.
        """
        nodeData = NodeData(
            node.siteName, node.nBytes, node.nBytesDirect, node.nAllocations)
        for child in node.GetChildren():
            nodeData.children.append(_GetNodeDataTree(child))
        return nodeData
    return _GetNodeDataTree(tree.GetRoot())


def BuildNodeDataTree(data: List[Tuple[int, int, int, str]]):
    """
    Convenience class for converting a list of tuples into a NodeData tree.

    Each item in the list must contain, in order:
        inclusive time, exclusive time, samples, and indented tag

    The number of leading spaces / 2 in the indented tag indicates the depth of
    that node.

    The returned tree will be rooted under a node named "root."

    For example,

    BuildNodeDataTree([
        # incl    excl    samp  indent+tag
        ( 251109,      0,  0,   "__root"),
        (   1803,   1803,  0,   "  AfPallette"),
        ( 242760,      0,  0,   "  Aui"),
        (   9776,   9776,  0,   "    AuiIconManager::_GetIcon"),
        ( 232984,      0,  0,   "    Aui"),
        ( 232984, 232984,  0,   "      AuiIconManager::_LoadPixmaps"),
        (   3573,      0,  0,   "  Browserq"),
        (    600,    600,  0,   "    BrowserqPrimDataSource::_GetChildren"),
        (   2973,   2973,  0,   "  BrowserqPalette")])
    """

    root = None
    stack = []

    for (incl, excl, samples, depthAndKey) in data:
        key = depthAndKey.lstrip()
        depth = (len(depthAndKey) - len(key))/2
        nodeData = NodeData(key, incl, excl, samples)

        if not root:
            root = nodeData
            stack.append(nodeData)
            continue

        # Pop the stack to get the parent
        while len(stack) > depth:
            stack.pop()
        parent = stack[-1]
        parent.children.append(nodeData)
        stack.append(nodeData)

    return root

class TestMallocTagReporterLoadMallocTag(unittest.TestCase):
    def AssertNodeTreesEqual(self, 
        rootA: NodeData,
        rootB: NodeData):
        """
        Compare two NodeData trees.
        """
        self.assertEqual(rootA.key, rootB.key, 
            msg="keys are different")
        self.assertEqual(rootA.inclusive, rootB.inclusive,
            msg=f"{rootA.key}: inclusive memory is different")
        self.assertEqual(rootA.exclusive, rootB.exclusive,
            msg=f"{rootA.key}: exclusive memory is different")
        self.assertEqual(rootA.samples, rootB.samples,
            msg=f"{rootA.key}: sample counts are different")
        self.assertEqual(len(rootA.children), len(rootB.children), 
            msg=f"{rootA.key}: child counts are different")

        for childA, childB in zip(rootA.children, rootB.children):
            self.AssertNodeTreesEqual(childA, childB)

    def AssertRoundTripValid(self, parsed: Tf.MallocTag.CallTree):
        """
        Take an already parsed report, write it to a temporary file, and then 
        re-parse it.

        This will verify that the re-parsed report is identical to the first 
        parsed report, which will ensure that saving and loading are consistent.
        """

        with MallocTagTextFile("") as filepath:
            parsed.Report(filepath)

            with open(filepath, 'r') as file:
                print(f"Saved report for {self.id().split('.')[-1]}:")
                print(file.read())

            newParsed = Tf.MallocTag.GetCallTree()
            self.assertTrue(newParsed.LoadReport(filepath))

            self.AssertNodeTreesEqual(
                GetNodeDataTree(parsed), GetNodeDataTree(newParsed))

    def test_Basic(self):
        """
        Test that a basic malloc tag report text file can be parsed.
        """
        trace = MallocTagTextFile(textwrap.dedent("""
        Tree view  ==============
              inclusive       exclusive
            2,952 B         1,232 B      20 samples    __root
              432 B             0 B       0 samples    | Csd
              176 B            64 B       2 samples    |   CsdAttribute::_New
              112 B             0 B       0 samples    |   | Csd
               88 B            88 B       1 samples    |   |   CsdProperty
               24 B            24 B       1 samples    |   |   Csd_Identity::New
               16 B            16 B       1 samples    |   CsdBase::_GetRemnantForHandles
              112 B           112 B       1 samples    |   CsdScene::_GetComposedSymmetryData
               96 B            96 B       3 samples    |   CsdScene::_PopulateComposedPropertyCache
               32 B            32 B       1 samples    |   Csd_PrimCache
            1,288 B             0 B       0 samples    | Did
              120 B           120 B       1 samples    |   DidArena::_NewState(inverse, inverseTag)
            1,168 B           992 B       7 samples    |   DidManager::AddLogMessage
              176 B             0 B       0 samples    |   | Did
              176 B           176 B       2 samples    |   |   DidManager::_BeginInteraction
        """))
        with trace as filepath:
            parsed = Tf.MallocTag.GetCallTree()
            self.assertTrue(parsed.LoadReport(filepath))
            parsed.Report()

        expected = BuildNodeDataTree([
            # incl  excl samp indent+tag
            ( 2952, 1232, 20, "__root"),
            (  432,    0,  0, "  Csd"),
            (  176,   64,  2, "    CsdAttribute::_New"),
            (  112,    0,  0, "      Csd"),
            (   88,   88,  1, "        CsdProperty"),
            (   24,   24,  1, "        Csd_Identity::New"),
            (   16,   16,  1, "    CsdBase::_GetRemnantForHandles"),
            (  112,  112,  1, "    CsdScene::_GetComposedSymmetryData"),
            (   96,   96,  3, "    CsdScene::_PopulateComposedPropertyCache"),
            (   32,   32,  1, "    Csd_PrimCache"),
            ( 1288,    0,  0, "  Did"),
            (  120,  120,  1, "    DidArena::_NewState(inverse, inverseTag)"),
            ( 1168,  992,  7, "    DidManager::AddLogMessage"),
            (  176,    0,  0, "      Did"),
            (  176,  176,  2, "        DidManager::_BeginInteraction")])

        self.AssertNodeTreesEqual(expected, GetNodeDataTree(parsed))
        self.AssertRoundTripValid(parsed)

    def test_Errors(self):
        """
        Test error conditions.
        """

        # Attempt to load a report with a path to a file that doesn't exist.
        with self.assertRaises(Tf.ErrorException):
            parsed = Tf.MallocTag.GetCallTree()
            self.assertFalse(parsed.LoadReport("bad_file_path"))

        # Attempt to load a report with more than one root.
        trace = MallocTagTextFile(textwrap.dedent("""
        Tree view  ==============
              inclusive       exclusive
                    0 B             0 B       0 samples    root1
                    0 B             0 B       0 samples    | A
                    0 B             0 B       0 samples    root2
                    0 B             0 B       0 samples    | B
            """))
        with trace as filepath:
            parsed = Tf.MallocTag.GetCallTree()
            with self.assertRaises(Tf.ErrorException):
                self.assertFalse(parsed.LoadReport(filepath))

if __name__ == '__main__':
    unittest.main()

