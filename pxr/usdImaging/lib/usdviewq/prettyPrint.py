#
# Copyright 2016 Pixar
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
'''
Hopefully we can deprecate this since most of the array stuff is handled by the
arrayAttributeView
'''
from PySide import QtGui
from pxr import Gf

def progressDialog(title, value):
    dialog = QtGui.QProgressDialog(title, "Cancel", 0, value)
    dialog.setModal(True)
    dialog.setMinimumDuration(500)
    return dialog

def prettyPrint(v):
    """Returns a string representing a "detailed view" of the value v.
    This string is used in the watch window"""

    # Pretty-print a dictionary
    if isinstance(v, dict):
        result = "Dictionary contents:\n" 
        for pair in v.items():
            keystring = str(pair[0])
            valstring = prettyPrint(pair[1])
            result += "------\n%s:\n%s\n" % (keystring, valstring)

    # Pretty-print list
    elif isinstance(v, list):
        dialog = progressDialog("Pretty-printing list...", len(v))

        result = "[\n"
        for i in range(len(v)):
            dialog.setValue(i)
            result += str(i) + ": " + prettyPrint(v[i]) + "\n"
            if (dialog.wasCanceled()):
                return "Pretty-printing canceled"
        result += "]\n"
        dialog.done(0)

    # Pretty-print tuple
    elif isinstance(v, tuple):
        dialog = progressDialog("Pretty-printing tuple...", len(v))

        result = "(\n"
        for i in range(len(v)):
            dialog.setValue(i)
            result += str(i) + ": " + prettyPrint(v[i]) + "\n"
            if (dialog.wasCanceled()):
                return "Pretty-printing canceled"
        result += ")\n"
        dialog.done(0)
    else:
        from scalarTypes import ToString
        result = ToString(v)

    return result

