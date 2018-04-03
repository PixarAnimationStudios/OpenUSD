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

print "Test Complete Trace"
gc.enabled = True
FullTrace()
gc.enabled = False
gr.Report()
gr.ClearTree()

print "Test Begin Only Trace"
gc.enabled = True
BeginOnlyTrace()
gc.enabled = False
gr.Report()
gr.ClearTree()

print "Test End Only Trace"
gc.enabled = True
EndOnlyTrace()
gc.enabled = False
gr.Report()
gr.ClearTree()

print "Test Partial Trace"
gc.enabled = True
PartialTrace()
gc.enabled = False
gr.Report()
gr.ClearTree()


print "\nTest Complete Trace Chrome"
gc.enabled = True
FullTrace()
gc.enabled = False
gr.ReportChromeTracing()
gr.ClearTree()

print "\nTest Begin Only Trace Chrome"
gc.enabled = True
BeginOnlyTrace()
gc.enabled = False
gr.ReportChromeTracing()
gr.ClearTree()

print "\nTest End Only Trace Chrome"
gc.enabled = True
EndOnlyTrace()
gc.enabled = False
gr.ReportChromeTracing()
gr.ClearTree()

print "\nTest Partial Trace Chrome"
gc.enabled = True
PartialTrace()
gc.enabled = False
gr.ReportChromeTracing()
gr.ClearTree()