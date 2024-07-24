#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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

        filepath = parsed.LogReport()

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

            # Spot test to make sure we read in the report correctly.
            root = parsed.GetRoot()
            self.assertEqual(len(root.GetChildren()), 2)
            self.assertEqual(root.nBytes, 2952)

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

    def test_MultipleTrees(self):
        """
        Test reading a report that contains multiple malloc tag report trees.
        """

        # Load a report with more than one tree.
        #
        # Note that the trees have to be sorted by descending inclusive memory
        # in order for the test to work because that's how we sort when
        # reporting.
        trace = MallocTagTextFile(textwrap.dedent("""
            Tree view  ==============
                  inclusive       exclusive
              297,871,934 B   283,163,709 B 1063871 samples    mem_after_activate
                    1,803 B         1,803 B      27 samples    | AfPalette
               13,218,646 B             0 B       0 samples    | Aui
               13,218,646 B        29,480 B     283 samples    |   AuiIconManager::_GetIcon
               13,189,166 B    13,189,166 B    1195 samples    |   | AuiIconManager::_LoadPixmaps
                    2,080 B             0 B       0 samples    | Browserq
                    2,080 B         2,080 B      39 samples    |   BrowserqPrimDataSource::_GetChildren
                    2,973 B         2,973 B      38 samples    | BrowserqPalette
                1,482,723 B             0 B       0 samples    | Cmd
                1,482,723 B     1,482,723 B   21068 samples    |   CmdRegistry::CreateCommand

            --------------------------------------------------------------------------------

            Malloc Tag Report


            Total bytes = 564,840,530


            Runner Info: > TestOpenShot

            Tree view  ==============
                  inclusive       exclusive
              108,248,789 B    94,582,913 B  517043 samples    mem_after_instantiate
                    1,803 B         1,803 B      27 samples    | AfPalette
               12,181,350 B             0 B       0 samples    | Aui
               12,181,350 B        27,824 B     253 samples    |   AuiIconManager::_GetIcon
               12,153,526 B    12,153,526 B    1189 samples    |   | AuiIconManager::_LoadPixmaps
                1,482,723 B             0 B       0 samples    | Cmd
                1,482,723 B     1,482,723 B   21068 samples    |   CmdRegistry::CreateCommand

            --------------------------------------------------------------------------------

            Malloc Tag Report


            Total bytes = 3,410,460,789
            """))
        with trace as filepath:
            parsed = Tf.MallocTag.GetCallTree()
            self.assertTrue(parsed.LoadReport(filepath))
            self.AssertRoundTripValid(parsed)

            # Spot test to make sure we read in the trees correctly.
            rootChildren = parsed.GetRoot().GetChildren()
            self.assertEqual(len(rootChildren), 2)
            self.assertEqual(rootChildren[0].nBytes, 297871934)
            self.assertEqual(rootChildren[1].nBytes, 108248789)

    def test_Errors(self):
        """
        Test error conditions.
        """

        # Attempt to load a report with a path to a file that doesn't exist.
        with self.assertRaises(Tf.ErrorException):
            parsed = Tf.MallocTag.GetCallTree()
            self.assertFalse(parsed.LoadReport("bad_file_path"))

if __name__ == '__main__':
    unittest.main()

