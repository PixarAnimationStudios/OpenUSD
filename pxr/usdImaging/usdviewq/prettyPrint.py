#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
'''
Hopefully we can deprecate this since most of the array stuff is handled by the
arrayAttributeView
'''
from .qt import QtWidgets

def progressDialog(title, value):
    dialog = QtWidgets.QProgressDialog(title, "Cancel", 0, value)
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
        from .scalarTypes import ToString
        result = ToString(v)

    return result

