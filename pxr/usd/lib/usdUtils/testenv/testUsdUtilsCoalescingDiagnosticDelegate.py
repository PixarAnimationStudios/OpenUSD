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
#

from pxr import Tf, Sdf, UsdUtils

# Constructing an sdf path with "" gives a tf warning 
def _genWarning():
    Tf.Warn('.')

EXPECTED_LINE_NO   = 30 
EXPECTED_FUNC_NAME = '__main__._genWarning'

delegate = UsdUtils.CoalescingDiagnosticDelegate()

# Test dumping of diagnostics (this will be compared in baseline/output.txt)
for i in range(0,10):
    _genWarning()

delegate.DumpCoalescedDiagnosticsToStdout()

# Test collection of unfiltered diagnostics
for i in range(0,3):
    _genWarning()

unfiltered = delegate.TakeUncoalescedDiagnostics()
assert len(unfiltered) == 3
for i in range(0,3):
    assert unfiltered[i].sourceLineNumber == EXPECTED_LINE_NO 
    assert unfiltered[i].sourceFunction == EXPECTED_FUNC_NAME

# Test collection of filtered diagnostics
for i in range(0,8):
    _genWarning()
filtered = delegate.TakeCoalescedDiagnostics()
assert len(filtered) == 1
filteredDiagnostic = filtered[0]

assert filteredDiagnostic.sharedItem.sourceLineNumber == EXPECTED_LINE_NO 
assert filteredDiagnostic.sharedItem.sourceFunction == EXPECTED_FUNC_NAME 

import pprint
pprint.pprint(filtered)
pprint.pprint(unfiltered)

for each in filtered:
    pprint.pprint(each)
    pprint.pprint(each.sharedItem)
    pprint.pprint(each.unsharedItems)
for each in unfiltered:
    pprint.pprint(each)
