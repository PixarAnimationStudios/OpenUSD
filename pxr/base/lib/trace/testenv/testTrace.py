#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

import sys
import os
import time
import unittest

from pxr import Trace, Tf


# Helper function
def GetNodesByKey(reporter, key):
    def getNodesRecur(node, key):
        result = []
        if node.key == key:
            result.append(node)
        for child in node.children:
            result.extend(getNodesRecur(child, key))
        return result
    return getNodesRecur(reporter.aggregateTreeRoot, key)

class TestTrace(unittest.TestCase):
    def test_Trace(self):
        gc = Trace.Collector()
        self.assertIsInstance(gc, Trace.Collector)

        self.assertEqual(gc, Trace.Collector())
        self.assertEqual(gc, Trace.Reporter.globalReporter.GetCollector())

        gr = Trace.Reporter.globalReporter
        self.assertIsInstance(gr, Trace.Reporter)

        if not os.getenv('PXR_ENABLE_GLOBAL_TRACE', False):
            self.assertEqual(gc.enabled , False)
            self.assertEqual(gc.EndEvent('Begin') , 0)
            gc.enabled = True
        else:
            self.assertEqual(gc.enabled , True)

        # begin and end.
        gc.BeginEvent('Begin')
        gc.BeginEvent('Begin')
        gc.BeginEvent('Begin')

        gc.EndEvent('Begin')
        gc.EndEvent('Begin')
        gc.EndEvent('Begin')


        gc.enabled = False

        # this shouldn't add any events

        gc.BeginEvent('Begin')
        gc.EndEvent('Begin')

        gc.enabled = True
        gr.UpdateAggregateTree()

        beginNodes = GetNodesByKey(gr, 'Begin')
        print len(beginNodes)
        self.assertEqual(len(beginNodes) , 3)

        for eventNode in beginNodes:
            self.assertEqual(eventNode.key , 'Begin')
            self.assertTrue(eventNode.exclusiveTime > 0 and \
                eventNode.inclusiveTime > 0)

        gc.Clear()
        gr.ClearTree()

        self.assertEqual(len(GetNodesByKey(gr, 'Begin')) , 0)


        ## The following is more example usage than a test....  It is left in to
        ## show usage and to exercise the python decorators.

        class Test:
            @Trace.TraceMethod
            def __init__(self, arg):
                self.__arg = arg
            @Trace.TraceMethod
            def Func(self):
                pass

        @Trace.TraceFunction
        def testFunc2():
            t = Test(1.234)
            t.Func()

        @Trace.TraceFunction
        def testFunc():
            gc = Trace.Collector()

            gc.BeginEvent('Custom Paired Event')

            testFunc2()
                
            gc.EndEvent('Custom Paired Event')

        testFunc()


        self.assertEqual(gr.GetCollector() , gc)

        gr.ClearTree()
        Trace.TestAuto()
        gr.UpdateAggregateTree()

        # Should have generated a top-level event
        autoNodes = GetNodesByKey(gr, 'TestAuto')
        self.assertEqual(len(autoNodes), 1)

        self.assertEqual(autoNodes[0].key , 'TestAuto')

        self.assertTrue(autoNodes[0].exclusiveTime > 0)
        self.assertTrue(autoNodes[0].inclusiveTime > 0)

        gr.ClearTree()
        Trace.TestNesting()
        gr.Report()

        # Should have generated a top-level event
        nestingNodes = GetNodesByKey(gr, 'TestNesting')
        self.assertEqual(len(nestingNodes), 1)
        self.assertEqual(nestingNodes[0].key , 'TestNesting')
        self.assertTrue(nestingNodes[0].exclusiveTime > 0)
        self.assertTrue(nestingNodes[0].inclusiveTime > 0)


        Trace.TestNesting()
        gr.UpdateAggregateTree()
        rootNode = gr.aggregateTreeRoot
        # code cover and check some of the exposed parts of EventNode
        for child in rootNode.children :
            print "inc: ", "%.3f" % child.inclusiveTime 
            print "exc: ", "%.3f" % child.exclusiveTime
            print "cnt: ", "%d" % child.count
            child.expanded = True
            self.assertTrue(child.expanded)
            
        gr.Report(len(rootNode.children))
        gr.ClearTree()
        gc.Clear()

        Trace.TestCreateEvents()
        pythonEvent = 'PYTHON_EVENT'
        gc.BeginEvent(pythonEvent)
        gc.EndEvent(pythonEvent)

        gr.UpdateAggregateTree()
        self.assertEqual(len(GetNodesByKey(gr, Trace.GetTestEventName())), 1)

        gr.ReportTimes()

        gc.BeginEvent('Foo')
        gc.BeginEvent('Bar')
        gc.BeginEvent('Foo')

        gc.EndEvent('Foo')
        gc.EndEvent('Bar')
        gc.EndEvent('Foo')

        gr.Report()

        gc.enabled = False
        gr.Report()

        self.assertEqual(gc.EndEvent("BadEvent"), 0)
        gc.enabled = True

        # Also try a local reporter and scope
        lr = Trace.Reporter('local reporter', gc)
        self.assertEqual(lr.GetCollector() , gc)
        self.assertEqual(lr.GetLabel() , 'local reporter')

        gc.BeginEvent("LocalScope")
        gc.EndEvent("LocalScope")

        gc.enabled = False

        lr.Report()

        gc.enabled = True
        sleepTime = 1.0
        b = gc.BeginEvent("Test tuple")
        time.sleep(sleepTime)
        e = gc.EndEvent("Test tuple")
        elapsedSeconds = Trace.GetElapsedSeconds(b, e)
        gr.Report()
        self.assertTrue(abs(elapsedSeconds - sleepTime) < 0.05)

        print ""

if __name__ == '__main__':
    unittest.main()

