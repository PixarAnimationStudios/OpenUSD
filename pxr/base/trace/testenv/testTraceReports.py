#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function

from pxr import Trace, Tf


gc = Trace.Collector()
gr = Trace.Reporter.globalReporter

def FullTrace():
    gc.BeginEvent('A')
    gc.BeginEvent('B')
    gc.BeginEvent('C')
    gc.EndEvent('C')
    gc.EndEvent('B')
    gc.EndEvent('A')
    
def BeginOnlyTrace():
    gc.BeginEvent('A')
    gc.BeginEvent('B')
    gc.BeginEvent('C')

def EndOnlyTrace():
    gc.EndEvent('C')
    gc.EndEvent('B')
    gc.EndEvent('A')

def PartialTrace():
    gc.BeginEvent('C')
    gc.EndEvent('C')
    gc.EndEvent('B')
    gc.EndEvent('A')

print("Test Complete Trace")
gc.enabled = True
FullTrace()
gc.enabled = False
gr.Report()
gr.ClearTree()

print("Test Begin Only Trace")
gc.enabled = True
BeginOnlyTrace()
gc.enabled = False
gr.Report()
gr.ClearTree()

print("Test End Only Trace")
gc.enabled = True
EndOnlyTrace()
gc.enabled = False
gr.Report()
gr.ClearTree()

print("Test Partial Trace")
gc.enabled = True
PartialTrace()
gc.enabled = False
gr.Report()
gr.ClearTree()


print("\nTest Complete Trace Chrome")
gc.enabled = True
FullTrace()
gc.enabled = False
gr.ReportChromeTracing()
gr.ClearTree()

print("\nTest Begin Only Trace Chrome")
gc.enabled = True
BeginOnlyTrace()
gc.enabled = False
gr.ReportChromeTracing()
gr.ClearTree()

print("\nTest End Only Trace Chrome")
gc.enabled = True
EndOnlyTrace()
gc.enabled = False
gr.ReportChromeTracing()
gr.ClearTree()

print("\nTest Partial Trace Chrome")
gc.enabled = True
PartialTrace()
gc.enabled = False
gr.ReportChromeTracing()
gr.ClearTree()
