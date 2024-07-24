#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Trace

import contextlib
import dataclasses
import os
import tempfile
import textwrap
from typing import Generator, List, Tuple
import unittest

@contextlib.contextmanager
def TraceTextFile(contents: str) -> Generator[str, None, None]:
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
            suffix=".trace", mode="w+", delete=False) as tmp:
            tmp.write(contents)
            tmp.flush()
        yield tmp.name
    finally:
        os.remove(tmp.name)

@dataclasses.dataclass
class NodeData:
    """
    A simple dataclass for verifying trace trees.
    """
    key: str
    inclusive: float
    exclusive: float
    samples: int
    children: List["NodeData"] = dataclasses.field(default_factory=list)

def GetNodeDataTree(tree: Trace.AggregateTree):
    def _GetNodeDataTree(node: Trace.AggregateNode):
        """
        Convert a TraceAggregateTree into a NodeData tree.
        """
        nodeData = NodeData(
            node.key, node.inclusiveTime, node.exclusiveTime, node.count)
        for child in node.children:
            nodeData.children.append(_GetNodeDataTree(child))
        return nodeData
    return _GetNodeDataTree(tree.root)

def BuildNodeDataTree(data: List[Tuple[float, float, int, str]]):
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
        (484.707, 31.384,  2,   "Main Thread"),
        (  0.000,  0.001,  1,   "  CmdRegistry::_HandleNotice"),
        (  0.064,  0.058,  3,   "  AppIterationFinished"),
        (  0.006,  0.006,  3,   "    AppIterationFinished: Send AppNotice::IterationFinished"),
        (453.258,  0.044, 24,   "  UiqApplication::Exec (Event loop)")])
    """
    root = NodeData("root", 0.0, 0.0, 0)
    stack = [root]
    for (incl, excl, samples, depthAndKey) in data:
        key = depthAndKey.lstrip()
        depth = (len(depthAndKey) - len(key))/2

        # Pop the stack to get the parent
        while len(stack) > depth+1:
            stack.pop()
        parent = stack[-1]

        # Build the node data.
        nodeData = NodeData(key, incl, excl, samples)
        parent.children.append(nodeData)
        stack.append(nodeData)

    return root

# The reporter only reports up to 3 decimal places.
THRESHOLD = 0.001

class TestTraceReporterLoadTrace(unittest.TestCase):
    def AssertNodeTreesEqual(self, 
        rootA: NodeData, rootB: NodeData, delta=THRESHOLD):
        """
        Compare two NodeData trees.
        """
        self.assertEqual(rootA.key, rootB.key, 
            msg="keys are different")
        self.assertAlmostEqual(rootA.inclusive, rootB.inclusive, delta=delta,
            msg=f"{rootA.key}: inclusive times are different")
        self.assertAlmostEqual(rootA.exclusive, rootB.exclusive, delta=delta,
            msg=f"{rootA.key}: exclusive times are different")
        self.assertEqual(rootA.samples, rootB.samples,
            msg=f"{rootA.key}: sample counts are different")
        self.assertEqual(len(rootA.children), len(rootB.children), 
            msg=f"{rootA.key}: child counts are different")

        for childA, childB in zip(rootA.children, rootB.children):
            self.AssertNodeTreesEqual(childA, childB, delta)

    def AssertRoundTripValid(self, 
        parsed: Trace.Reporter.ParsedTree, delta=THRESHOLD):
        """
        Take an already parsed report, write it to a temporary file, and then 
        re-parse it.

        This will verify that the re-parsed report is identical to the first 
        parsed report, which will ensure that saving and loading are consistent.
        """
        with TraceTextFile("") as filepath:
            for index, parsedEntry in enumerate(parsed):
                tree, iterationCount = \
                    parsedEntry.tree, parsedEntry.iterationCount

                reporter = Trace.Reporter(f"{self.id()}_{index}")
                root = reporter.aggregateTreeRoot
                for child in tree.root.children:
                    root.Append(child)
                reporter.Report(filepath, iterationCount, append=True)

            with open(filepath, 'r') as file:
                print(f"Saved report for {self.id().split('.')[-1]}:")
                print(file.read())

            newParsed = Trace.Reporter.LoadReport(filepath)
            self.assertEqual(len(parsed), len(newParsed),
                msg="Number of trees saved != number of trees loaded")
            for old, new in zip(parsed, newParsed):
                self.assertEqual(old.iterationCount, new.iterationCount,
                    msg="iterationCount saved != iterationCount loaded")
                self.AssertNodeTreesEqual(
                    GetNodeDataTree(old.tree), GetNodeDataTree(new.tree), delta)

    def test_Basic(self):
        """
        Test that a basic trace text file can be parsed.
        """
        trace = TraceTextFile(textwrap.dedent("""
            Tree view  ==============
                inclusive    exclusive        
               484.707 ms    31.384 ms       2 samples    Main Thread
                 0.001 ms     0.001 ms       1 samples    | CmdRegistry::_HandleNotice
                 0.064 ms     0.058 ms       3 samples    | AppIterationFinished
                 0.006 ms     0.006 ms       3 samples    |   AppIterationFinished: Send AppNotice::IterationFinished
                                             1 samples    |   | DidManager::FlushPendingInteractions
               453.258 ms     0.044 ms      24 samples    | UiqApplication::Exec (Event loop)
               453.214 ms   440.033 ms      24 samples    |   UiqApplication::Exec (Process Qt events)
                13.181 ms    13.181 ms      30 samples    |   | UiqApplication::_notify
                                             1 samples    |   |   AppExecuteQueuedLaterFunctions
            """))
        with trace as filepath:
            parsed = Trace.Reporter.LoadReport(filepath)
        
        self.assertEqual(len(parsed), 1)
        self.assertEqual(parsed[0].iterationCount, 1)

        expected = BuildNodeDataTree([
            #   incl     excl samp  indent+tag
            (484.707,  31.384,   2, "Main Thread"),
            (  0.001,   0.001,   1, "  CmdRegistry::_HandleNotice"),
            (  0.064,   0.058,   3, "  AppIterationFinished"),
            (  0.006,   0.006,   3, "    AppIterationFinished: Send AppNotice::IterationFinished"),
            (  0.000,   0.000,   1, "      DidManager::FlushPendingInteractions"),
            (453.258,   0.044,  24, "  UiqApplication::Exec (Event loop)"),
            (453.214, 440.033,  24, "    UiqApplication::Exec (Process Qt events)"),
            ( 13.181,  13.181,  30, "      UiqApplication::_notify"),
            (  0.000,   0.000,   1, "        AppExecuteQueuedLaterFunctions")
        ])
        self.AssertNodeTreesEqual(expected, GetNodeDataTree(parsed[0].tree))

        self.AssertRoundTripValid(parsed)

    def test_BlankEntryOnMainThreadExclusiveTime(self):
        """
        Regression test: main thread's times gets incorrectly parsed if its 
        exclusive time entry is blank.
        """
        trace = TraceTextFile(textwrap.dedent("""
            Tree view  ==============
               inclusive    exclusive        
                9.064 ms                    1 samples    Main Thread
                9.064 ms     0.072 ms       1 samples    | Tormentor::menva_read_time
                8.992 ms     5.792 ms       1 samples    |   SdfLayer::FindOrOpen
                3.200 ms     3.200 ms       1 samples    |   | SdfLayer::_ComputeInfoToFindOrOpenLayer
            """))
        with trace as filepath:
            parsed = Trace.Reporter.LoadReport(filepath)
        
        self.assertEqual(len(parsed), 1)
        self.assertEqual(parsed[0].iterationCount, 1)

        expected = BuildNodeDataTree([
            # incl   excl samp  indent+tag
            (9.064, 0.000,   1, "Main Thread"),
            (9.064, 0.072,   1, "  Tormentor::menva_read_time"),
            (8.992, 5.792,   1, "    SdfLayer::FindOrOpen"),
            (3.200, 3.200,   1, "      SdfLayer::_ComputeInfoToFindOrOpenLayer")
        ])
        self.AssertNodeTreesEqual(expected, GetNodeDataTree(parsed[0].tree))

        self.AssertRoundTripValid(parsed)

    def test_Iters(self):
        """
        Test reading a trace tree with iterations.
        """
        trace = TraceTextFile(textwrap.dedent("""
            Number of iterations: 36
        
            Tree view  ==============
            inclusive    exclusive        
                0.040 ms                    0.028 samples    Main Thread
                0.040 ms     0.004 ms       0.028 samples    | AppIterationFinished
                0.036 ms                    0.028 samples    |   AppExecuteQueuedLaterFunctions
                0.036 ms     0.012 ms       0.028 samples    |   | UpPlayer::_SendPendingPlayModeNotification
                0.024 ms     0.002 ms       0.028 samples    |   |   UpPlayer::_SendPendingPlayModeNotification (_PlaybackDidStop)
                0.022 ms     0.006 ms       0.028 samples    |   |   | uncached_frame_change
                                            0.111 samples    |   |   |   UpPlayer::IsTimeMunging
                0.001 ms     0.001 ms       0.083 samples    |   |   |   CmdRegistry::_HandleNotice
                0.015 ms     0.015 ms       0.028 samples    |   |   |   UpPlayer::SetPaused
            """))
        with trace as filepath:
            parsed = Trace.Reporter.LoadReport(filepath)
        
        self.assertEqual(len(parsed), 1)

        iters = 36
        self.assertEqual(parsed[0].iterationCount, iters)

        expected = BuildNodeDataTree([
            #    incl      excl                      samp  indent+tag
            (iters*0.040, iters*0.000, round(iters*0.028), "Main Thread"),
            (iters*0.040, iters*0.004, round(iters*0.028), "  AppIterationFinished"),
            (iters*0.036, iters*0.000, round(iters*0.028), "    AppExecuteQueuedLaterFunctions"),
            (iters*0.036, iters*0.012, round(iters*0.028), "      UpPlayer::_SendPendingPlayModeNotification"),
            (iters*0.024, iters*0.002, round(iters*0.028), "        UpPlayer::_SendPendingPlayModeNotification (_PlaybackDidStop)"),
            (iters*0.022, iters*0.006, round(iters*0.028), "          uncached_frame_change"),
            (iters*0.000, iters*0.000, round(iters*0.111), "            UpPlayer::IsTimeMunging"),
            (iters*0.001, iters*0.001, round(iters*0.083), "            CmdRegistry::_HandleNotice"),
            (iters*0.015, iters*0.015, round(iters*0.028), "            UpPlayer::SetPaused")
        ])
        self.AssertNodeTreesEqual(expected, GetNodeDataTree(parsed[0].tree))

        # The parser and reporter round to the nearest thousandths place, but 
        # also divide/multiply the iteration count respectively, so some 
        # precision seems to be lost, and we need to check with a weaker 
        # threshold.
        #
        # XXX: This threshold is only needed for the debug build, and seems 
        # higher than expected. We should investigate this further.
        self.AssertRoundTripValid(parsed, THRESHOLD*iters)

    def test_MultipleTrees(self):
        """
        Test reading traces with multiple trees.
        """
        trace = TraceTextFile(textwrap.dedent("""
            Tree view  ==============
            inclusive    exclusive        
            123.442 ms                    1 samples    Main Thread
            123.442 ms     0.120 ms       1 samples    | Kitchen_set_flattened.usd
            123.322 ms   123.320 ms       1 samples    |   SdfLayer::_WriteToFile
              0.002 ms     0.002 ms       1 samples    |   | Sdf_FileFormatRegistry::FindByExtension


            Tree view  ==============
            inclusive    exclusive        
              2.226 ms                    1 samples    Main Thread
              2.226 ms     0.035 ms       1 samples    | s035_1c_fx_confetti_162.usd
              2.191 ms     2.185 ms       1 samples    |   SdfLayer::_WriteToFile
              0.006 ms     0.006 ms       1 samples    |   | Sdf_FileFormatRegistry::FindByExtension
        """))
        with trace as filepath:
            parsed = Trace.Reporter.LoadReport(filepath)

        self.assertEqual(len(parsed), 2)
        self.assertEqual(parsed[0].iterationCount, 1)
        self.assertEqual(parsed[1].iterationCount, 1)

        expected1 = BuildNodeDataTree([
            # incl       excl samp  indent+tag
            (123.442,   0.000,   1, "Main Thread"),
            (123.442,   0.120,   1, "  Kitchen_set_flattened.usd"),
            (123.322, 123.320,   1, "    SdfLayer::_WriteToFile"),
            (  0.002,   0.002,   1, "      Sdf_FileFormatRegistry::FindByExtension")
        ])
        expected2 = BuildNodeDataTree([
            # incl   excl samp  indent+tag
            (2.226, 0.000,   1, "Main Thread"),
            (2.226, 0.035,   1, "  s035_1c_fx_confetti_162.usd"),
            (2.191, 2.185,   1, "    SdfLayer::_WriteToFile"),
            (0.006, 0.006,   1, "      Sdf_FileFormatRegistry::FindByExtension")
        ])

        self.AssertNodeTreesEqual(expected1, GetNodeDataTree(parsed[0].tree))
        self.AssertNodeTreesEqual(expected2, GetNodeDataTree(parsed[1].tree))

        self.AssertRoundTripValid(parsed)

if __name__ == '__main__':
    unittest.main()

