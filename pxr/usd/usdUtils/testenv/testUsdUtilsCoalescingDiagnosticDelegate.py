#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf, Sdf, UsdUtils

# Constructing an sdf path with "" gives a tf warning 
def _genWarning():
    Tf.Warn('.')

EXPECTED_LINE_NO   = 13
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
