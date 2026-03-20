"""
>>> from module_nogil_ext import *
>>> get_value()
1234
>>> import sys, sysconfig
>>> Py_GIL_DISABLED = bool(sysconfig.get_config_var('Py_GIL_DISABLED'))
>>> if Py_GIL_DISABLED and sys._is_gil_enabled():
...    print('GIL is enabled and should not be')
... else:
...    print('okay')
okay
"""

from __future__ import print_function

def run(args = None):
    import sys
    import doctest

    if args is not None:
        sys.argv = args
    return doctest.testmod(sys.modules.get(__name__))

if __name__ == '__main__':
    print("running...")
    import sys
    status = run()[0]
    if (status == 0): print("Done.")
    sys.exit(status)
