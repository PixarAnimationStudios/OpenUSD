#
# Copyright 2024 Pixar
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
r'''>>> import pickle4_ext
    >>> import pickle
    >>> def world_getinitargs(self):
    ...   return (self.get_country(),)
    >>> pickle4_ext.world.__getinitargs__ = world_getinitargs
    >>> pickle4_ext.world.__module__
    'pickle4_ext'
    >>> pickle4_ext.world.__safe_for_unpickling__
    1
    >>> pickle4_ext.world.__name__
    'world'
    >>> pickle4_ext.world('Hello').__reduce__()  # doctest: +PY310
    (<class 'pickle4_ext.world'>, ('Hello',))
    >>> pickle4_ext.world('Hello').__reduce__()  # doctest: +PY311
    (<class 'pickle4_ext.world'>, ('Hello',), None)
    >>> wd = pickle4_ext.world('California')
    >>> pstr = pickle.dumps(wd)
    >>> wl = pickle.loads(pstr)
    >>> print(wd.greet())
    Hello from California!
    >>> print(wl.greet())
    Hello from California!
'''

def run(args = None):
    import sys
    import doctest

    if args is not None:
        sys.argv = args

    # > https://docs.python.org/3.11/library/pickle.html#object.__reduce__
    # object.__reduce__() returns
    # - python 3.10 or prior: a 2-element tuple
    # - python 3.11 or later: a 3-element tuple (object's state added)
    PY310 = doctest.register_optionflag("PY310")
    PY311 = doctest.register_optionflag("PY311")

    class ConditionalChecker(doctest.OutputChecker):
        def check_output(self, want, got, optionflags):
            if (optionflags & PY311) and (sys.version_info[:2] < (3, 11)):
                return True
            if (optionflags & PY310) and (sys.version_info[:2] >= (3, 11)):
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
