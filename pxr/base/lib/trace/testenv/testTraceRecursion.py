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

import sys, os
from pxr import Trace, Tf


# ----------------------------------------------------------------------------
# WriteReport writes out the trace report to a specific filename, and cleans
# the reporter and the collector.  It also wipes the contents of the report
# clear of any timing data so that diffs on different platforms pass.
#
def WriteReport(reporter, collector, filename):
    # We report twice, the first time to stdout, and the second time to 
    # a file that we will compare against a baseline.  The reason
    # we output to stdout is because we want to make sure that repeatedly
    # calling Report on the same tree will not change the output, and because it
    # is easier to see all the output in one place in the test results.
    reporter.Report() 
    reporter.Report(filename)
    reporter.ClearTree()
    collector.Clear()



# ----------------------------------------------------------------------------

gc = Trace.Collector()
gr = Trace.Reporter.globalReporter

gc.enabled = True
gr.foldRecursiveCalls = True

# ----------------------------------------------------------------------------
# A test for recursion where we are nested 5 levels deep.
gc.BeginEventAtTime('Begin', 0.0)   #  Begin
gc.BeginEventAtTime('Begin', 0.0)   #    Begin
gc.BeginEventAtTime('Begin', 0.0)   #      Begin
gc.BeginEventAtTime('Begin', 0.0)   #        Begin
gc.BeginEventAtTime('Begin', 0.0)   #          Begin

gc.EndEventAtTime('Begin',1.0)
gc.EndEventAtTime('Begin',2.0)
gc.EndEventAtTime('Begin',3.0)
gc.EndEventAtTime('Begin',4.0)
gc.EndEventAtTime('Begin',5.0)

WriteReport(gr, gc, "recursion_5begins.out")


# ----------------------------------------------------------------------------
# A typical recursive pattern where each subtree should be called twice in the
# final output.
gc.BeginEventAtTime('R', 0.0)    # R
gc.BeginEventAtTime('A', 0.0)    #   A
gc.BeginEventAtTime('B', 0.0)    #     B
gc.BeginEventAtTime('C', 0.0)    #       C
gc.BeginEventAtTime('A', 0.0)    #         A
gc.BeginEventAtTime('B', 0.0)    #           B
gc.BeginEventAtTime('C', 0.0)    #             C
gc.EndEventAtTime('C',1.0)
gc.EndEventAtTime('B',2.0)
gc.EndEventAtTime('A',3.0)
gc.EndEventAtTime('C',4.0)
gc.EndEventAtTime('B',5.0)
gc.EndEventAtTime('A',6.0)
gc.EndEventAtTime('R',7.0)

WriteReport(gr, gc, "recursion_typical.out")


# ----------------------------------------------------------------------------
# A test where the recursion causes a new node in the outer most scope.
gc.BeginEventAtTime('A', 0.0)      #  A
gc.BeginEventAtTime('B', 0.0)      #    B
gc.EndEventAtTime('B',1.0)    #    C
gc.BeginEventAtTime('C',1.0)      #      B
gc.BeginEventAtTime('B',1.0)      #        A
gc.BeginEventAtTime('A',1.0)      #          D
gc.BeginEventAtTime('D',1.0)
gc.EndEventAtTime('D', 2.0)
gc.EndEventAtTime('A', 3.0)
gc.EndEventAtTime('B', 4.0)
gc.EndEventAtTime('C', 5.0)
gc.EndEventAtTime('A', 6.0)

WriteReport(gr, gc, "recursion_newnode.out")

# ----------------------------------------------------------------------------
# A test where there is a recursion within a recursion.
gc.BeginEventAtTime('A', 0.0)
gc.BeginEventAtTime ('B', 0.0)
gc.EndEventAtTime   ('B', 1.0)
gc.BeginEventAtTime ('C', 1.0)
gc.BeginEventAtTime  ('E', 1.0)
gc.EndEventAtTime    ('E', 2.0)
gc.BeginEventAtTime  ('A', 2.0)
gc.BeginEventAtTime   ('D', 2.0)
gc.BeginEventAtTime    ('A', 2.0)
gc.BeginEventAtTime     ('D', 2.0)
gc.EndEventAtTime       ('D', 3.0)
gc.EndEventAtTime      ('A', 4.0)
gc.EndEventAtTime     ('D', 5.0)
gc.EndEventAtTime    ('A', 6.0)
gc.BeginEventAtTime  ('F', 6)
gc.EndEventAtTime    ('F', 7.0)
gc.EndEventAtTime   ('C', 8.0)
gc.BeginEventAtTime ('E', 8.0)
gc.EndEventAtTime   ('E', 9.0)
gc.EndEventAtTime  ('A', 10.0)

WriteReport(gr, gc, "recursion_inner.out")


# ----------------------------------------------------------------------------
# Test for bug 1817 -- this is a case where two branches are done in an order
# such that we end up merging a non-marker into a marker.
gc.BeginEventAtTime('C', 0.0)
gc.BeginEventAtTime('A', 0.0)
gc.BeginEventAtTime('C', 0.0)
gc.BeginEventAtTime('C', 0.0)
gc.BeginEventAtTime('D', 0.0)
gc.BeginEventAtTime('E', 0.0)
gc.BeginEventAtTime('A', 0.0)
gc.EndEventAtTime  ('A', 1.0)
gc.EndEventAtTime  ('E', 2.0)
gc.EndEventAtTime  ('D', 3.0)
gc.BeginEventAtTime('C', 3.0)
gc.BeginEventAtTime('D', 3.0)
gc.BeginEventAtTime('E', 3.0)
gc.EndEventAtTime  ('E', 4.0)
gc.EndEventAtTime  ('D', 5.0)
gc.EndEventAtTime  ('C', 6.0)
gc.EndEventAtTime  ('C', 7.0)
gc.BeginEventAtTime('D', 7.0)
gc.BeginEventAtTime('E', 7.0)
gc.BeginEventAtTime('A', 7.0)
gc.EndEventAtTime  ('A', 8.0)
gc.EndEventAtTime  ('E', 9.0)
gc.EndEventAtTime  ('D', 10.0)
gc.EndEventAtTime  ('C', 11.0)
gc.EndEventAtTime  ('A', 12.0)
gc.EndEventAtTime  ('C', 13.0)

WriteReport(gr, gc, "recursion_1817.out")

# ----------------------------------------------------------------------------
# This tests the algorithm for times.  
# The times are [exclusive/inclusive].
# A [14/105]
# |
# |-- B [12/78]
# |   |
# |   |-- C [10/55]
# |   |   |
# |   |   |-- A [4/10]
# |   |   |   |
# |   |   |   |-- B [2/3]
# |   |   |   |   |
# |   |   |   |   |-- D [1/1]
# |   |   |   |
# |   |   |   |-- E [3/3]
# |   |   |
# |   |   |-- B [9/35]
# |   |       |
# |   |       |-- C [6/11]
# |   |       |   |
# |   |       |   |-- A [5/5]
# |   |       |
# |   |       |-- D [7/7]
# |   |       |
# |   |       |-- F [8/8]
# |   |
# |   |-- F [11/11]
# |
# |-- E [13/13]

gc.BeginEventAtTime('A', 0.0)
gc.BeginEventAtTime ('B', 0.0)
gc.BeginEventAtTime  ('C', 0.0)
gc.BeginEventAtTime   ('A', 0.0)
gc.BeginEventAtTime    ('B', 0.0)
gc.BeginEventAtTime     ('D', 0.0)
gc.EndEventAtTime       ('D', 1.0)
gc.EndEventAtTime      ('B', 3.0)
gc.BeginEventAtTime    ('E', 3.0)
gc.EndEventAtTime      ('E', 6.0)
gc.EndEventAtTime     ('A', 10.0)
gc.BeginEventAtTime   ('B', 10.0)
gc.BeginEventAtTime    ('C', 10.0)
gc.BeginEventAtTime     ('A', 10.0)
gc.EndEventAtTime       ('A', 15.0)
gc.EndEventAtTime      ('C', 21.0)
gc.BeginEventAtTime    ('D', 21.0)
gc.EndEventAtTime      ('D', 28.0)
gc.BeginEventAtTime    ('F', 28.0)
gc.EndEventAtTime      ('F', 36.0)
gc.EndEventAtTime     ('B', 45.0)
gc.EndEventAtTime    ('C', 55.0)
gc.BeginEventAtTime  ('F', 55.0)
gc.EndEventAtTime    ('F', 66.0)
gc.EndEventAtTime   ('B', 78.0)
gc.BeginEventAtTime ('E', 78.0)
gc.EndEventAtTime   ('E', 97.0)
gc.EndEventAtTime  ('A', 105.0)

WriteReport(gr, gc, "recursion_timing.out")

# ----------------------------------------------------------------------------
# Tests a case where the tree is processed in such an order that we visit a
# recursive node with no children before we visit the node into which it is
# merged, but that does have children.

gc.BeginEventAtTime('R', 0.0)         # R
gc.BeginEventAtTime('A', 0.0)         #   A
gc.BeginEventAtTime('B', 0.0)         #     B
gc.BeginEventAtTime('C', 0.0)         #       C
gc.BeginEventAtTime('B', 0.0)         #         B
gc.EndEventAtTime('B', 1.0)      #     A
gc.EndEventAtTime('C', 2.0)      #       B
gc.EndEventAtTime('B', 3.0)      #         C
gc.BeginEventAtTime('A', 3.0)
gc.BeginEventAtTime('B', 3.0)
gc.BeginEventAtTime('C', 3.0)
gc.EndEventAtTime('C', 4.0)
gc.EndEventAtTime('B', 5.0)
gc.EndEventAtTime('A', 6.0)
gc.EndEventAtTime('A', 7.0)
gc.EndEventAtTime('R', 8.0)

WriteReport(gr, gc, "recursion_branch.out")

# ----------------------------------------------------------------------------
# Tests a case where the merged children already exist as a marker in the
# parent.  The order of the branches is very important for this test case.

gc.BeginEventAtTime('A', 0)       #  A
gc.BeginEventAtTime('A', 0)       #    A
gc.BeginEventAtTime('B', 0)       #      B
gc.EndEventAtTime('B', 1.0)    #    C
gc.EndEventAtTime('A', 2.0)    #      B
gc.BeginEventAtTime('C', 2.0)       #        A
gc.BeginEventAtTime('B', 2.0)       #          B
gc.BeginEventAtTime('A', 2.0)       #            A
gc.BeginEventAtTime('B', 2.0)
gc.BeginEventAtTime('A', 2.0)
gc.EndEventAtTime('A', 3.0)
gc.EndEventAtTime('B', 4.0)
gc.EndEventAtTime('A', 5.0)
gc.EndEventAtTime('B', 6.0)
gc.EndEventAtTime('C', 7.0)
gc.EndEventAtTime('A', 8.0)

WriteReport(gr, gc, "recursion_marker_merge.out")

print "Test SUCCEEDED"
