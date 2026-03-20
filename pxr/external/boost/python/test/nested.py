#
# Copyright 2024 Pixar
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
    >>> from nested_ext import *
    
    >>> X
    <class 'nested_ext.X'>
    
    >>> X.__module__
    'nested_ext'
    
    >>> X.__name__
    'X'
    
    >>> X.Y # doctest: +py2
    <class 'nested_ext.Y'>

    >>> X.Y # doctest: +py3
    <class 'nested_ext.X.Y'>
    
    >>> X.Y.__module__
    'nested_ext'
    
    >>> X.Y.__name__
    'Y'

    >>> getattr(X.color, "__qualname__", None) # doctest: +py3
    'X.color'

    >>> repr(X.color.red) # doctest: +py2
    'nested_ext.color.red'

    >>> repr(X.color.red) # doctest: +py3
    'nested_ext.X.color.red'

    >>> repr(X.color(1)) # doctest: +py2
    'nested_ext.color(1)'

    >>> repr(X.color(1)) # doctest: +py3
    'nested_ext.X.color(1)'

    >>> test_function.__doc__.strip().split('\\n')[0] # doctest: +py3
    'test_function( (X)arg1, (X.Y)arg2) -> None :'
    
'''

def run(args = None):
    import sys
    import doctest

    if args is not None:
        sys.argv = args

    py2 = doctest.register_optionflag("py2")
    py3 = doctest.register_optionflag("py3")

    class ConditionalChecker(doctest.OutputChecker):
        def check_output(self, want, got, optionflags):
            if (optionflags & py3) and (sys.version_info[0] < 3):
                return True
            if (optionflags & py2) and (sys.version_info[0] >= 3):
                return True
            return doctest.OutputChecker.check_output(self, want, got, optionflags)

    runner = doctest.DocTestRunner(ConditionalChecker())
    for test in doctest.DocTestFinder().find(sys.modules.get(__name__)):
        runner.run(test)

    return doctest.TestResults(runner.failures, runner.tries)
    
if __name__ == '__main__':
    print("running...")
    import sys
    status = run()[0]
    if (status == 0): print("Done.")
    sys.exit(status)
