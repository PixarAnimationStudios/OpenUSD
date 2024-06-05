#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import unittest
import time

from pxr import Trace, Tf

# Helper function.
def GetNodesByKey(reporter, key):
    def getNodesRecur(node, key):
        result = []
        if key in node.key:
            result.append(node)
        for child in node.children:
            result.extend(getNodesRecur(child, key))
        return result
    return getNodesRecur(reporter.aggregateTreeRoot, key)

class TestTraceReporter(unittest.TestCase):
    def test_Reporter(self):
        gc = Trace.Collector()
        gr = Trace.Reporter.globalReporter
        gr.shouldAdjustForOverheadAndNoise = False

        # Create a call graph with manually specified times so that the times in the 
        # report graph can be validated.

        def A():
            ts = 0.0
            gc.BeginEventAtTime('A', ts)
            for i in range(3):
                ts = B(ts)
            ts += 1 # Simulate 1 ms of work
            gc.EndEventAtTime('A', ts)

        def B(ts):
            gc.BeginEventAtTime('B', ts)
            for i in range(4):
                ts = C(ts)
            ts += 1 # Simulate 1 ms of work
            gc.EndEventAtTime('B', ts)
            return ts

        def C(ts):
            gc.BeginEventAtTime('C', ts)
            ts += 1 # Simulate 1 ms of work
            gc.EndEventAtTime('C', ts)
            return ts

        gc.enabled = True
        A()
        gc.enabled = False
        gr.Report()

        aNodes = GetNodesByKey(gr, 'A')
        self.assertEqual(len(aNodes), 1)
        self.assertEqual(aNodes[0].count, 1)
        self.assertAlmostEqual(aNodes[0].inclusiveTime, 16.0, delta=1e-4)
        self.assertAlmostEqual(aNodes[0].exclusiveTime, 1.0, delta=1e-4)

        bNodes = GetNodesByKey(gr, 'B')
        self.assertEqual(len(bNodes), 1)
        self.assertEqual(bNodes[0].count, 3)
        self.assertAlmostEqual(bNodes[0].inclusiveTime, 15.0, delta=1e-4)
        self.assertAlmostEqual(bNodes[0].exclusiveTime, 3.0, delta=1e-4)

        cNodes = GetNodesByKey(gr, 'C')
        self.assertEqual(len(cNodes), 1)
        self.assertEqual(cNodes[0].count, 12)
        self.assertAlmostEqual(cNodes[0].inclusiveTime, 12.0, delta=1e-4)
        self.assertAlmostEqual(cNodes[0].exclusiveTime, 12.0, delta=1e-4)

if __name__ == '__main__':
    unittest.main()
