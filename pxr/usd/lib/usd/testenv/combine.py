#!/pxrpythonsubst
import sys
import os

TRACE = False 

from Mentor.Runtime.Utility import FindDataFile, AssertEqual
from Mentor.Runtime import Runner, Fixture, AssertTrue


# --------------------------------------------------------------------------- #
# Selector Objects
#
# These implement the logic of selecting which test cases to run for a given
# iteration.
# --------------------------------------------------------------------------- #

def SimpleSelector(inputGroup):
    """Only returns the first producer in the inputGroup. This is primarily for
    debugging combine itself.
    """
    return inputGroup.producers[0]


class ProductMap(object):
    """Produces an all-pairs cartesian product of producers. The product map
    provides a GetSelector method which returns a selector for a specific 
    set of values in the map.
    """
    def __init__(self, inputGroups):
        import itertools
        counts = []
        resMap = {}
        i = 0
        for name in sorted(inputGroups.keys()):
            counts.append(len(inputGroups[name].producers))
            resMap[name] = i
            i += 1

        # Save the generator state
        self.products = itertools.product(*[range(i) for i in counts])
        self.inputGroupMap = resMap
        self.names = {}
        for n in inputGroups.keys():
            self.names[n] = [m.__name__ for m in inputGroups[n].producers] 
                                      
        # Setup iteration
        self._i = -1
        self._selection = None

    def __iter__(self):
        value = self.GetSelector(self._i + 1)
        while value:
            yield value
            value = self.GetSelector(self._i + 1)
        yield value

    def GetSelector(self, iteration):
        assert iteration >= self._i
        while self._i <= iteration:
            if self._i == iteration:
                break
            self._selection = self.products.next() 
            self._i += 1

        if TRACE:
            print self._selection
        if self._selection is None:
            return None

        selMap = {}
        for name in sorted(self.inputGroupMap.keys()):
            selMap[name] = self._selection[self.inputGroupMap[name]]


        l = lambda inputGroup, selMap=selMap: inputGroup.producers[selMap[inputGroup.name]]
        l.__doc__ = " ".join([name + "." + self.names[name][i] 
                              for name,i in selMap.iteritems()])
        return l


# --------------------------------------------------------------------------- #
# InputClasses
# --------------------------------------------------------------------------- #

class InputGroup(object):
    """A InputGroup is a collection of methods (producers) that can produce a
    value. Calling GetValue will use the selector to choose a producer, execute
    all required inputs for the producer, and memoise the output value.
    """

    def __init__(self, parent, producers):
        self.name = parent.__name__
        self.parent = parent
        self.producers = producers 
        self._isCached = False
        self._value = None

    def Reset(self):
        self._isCached = False
        self._value = None

    def PrintExecPlan(self, allInputGroups, selector, varset, forRepro):
        import inspect
        indent = " "*4
        if self.name in varset:
            return varset[self.name]

        func = selector(self)
        args = {} 
        for arg in inspect.getargspec(func).args:
            args[arg] = allInputGroups[arg].PrintExecPlan(allInputGroups, 
                                                           selector, 
                                                           varset, 
                                                           forRepro)

        varset[self.name] = "_" + self.name[0].lower() + self.name[1:]

        if forRepro:
            print indent, \
                  varset[self.name], "=", \
                  self.name + ".__dict__['" + func.__name__ + "'](",

            if len(args) == 0:
                print ")"
            else:
                print
                for i,arg in enumerate(args):
                    print indent*3,
                    print arg + "=" + args[arg],
                    if i < len(args) - 1:
                        print ","
                print ")"

            print indent
        else:
            print indent, self.name + "." + func.__name__
        return varset[self.name] 

    def GetValue(self, allInputGroups, selector):
        import inspect

        if self._isCached:
            return self._value

        args = []
        func = selector(self)

        if TRACE:
            print "Collecting args for ", self.name + "." + func.__name__

        for arg in inspect.getargspec(func).args:
            args.append(allInputGroups[arg].GetValue(allInputGroups, selector))
        
        if func.__doc__:
            print func.__doc__

        if TRACE:
            print "Executing", self.name + "." + func.__name__

        sys.stdout.flush()
        sys.stderr.flush()
        self._value = func(*args)
        self._isCached = True

        return self._value 


# --------------------------------------------------------------------------- #
# Mentor Support
# --------------------------------------------------------------------------- #

class UberFixture(Fixture):
    """The test cases for this fixture will be dynamically populated based on
    the input script."""
    pass


def _TestCase(inputGroups, selector):
    """Provides closure for a single test case in the mentor test fixture.
    """

    # A helper method to avoid repeating this code twice.
    def _Cleanup():
        if "Cleanup" in inputGroups:
            if TRACE:
                print "CLEANING UP----------------"
            inputGroups["Cleanup"].GetValue(inputGroups, selector)
        for v in inputGroups.values():
            v.Reset()

    # Run the Init class, if present.
    if "Init" in inputGroups:
        inputGroups["Init"].GetValue(inputGroups, selector)

    # Ensure a clean state before running tests.
    for v in inputGroups.values():
        v.Reset()

    #
    # Execute the dependency graph.
    #
    for name in sorted(inputGroups.keys()):
        if name == "Cleanup" or name == "Init":
            continue
        try:
            inputGroups[name].GetValue(inputGroups, selector)
        except Exception as exc:
            #
            # Test-specific reporting and abort (if requested)
            #
            if args.stopOnError:
                print "*** TEST FAILED."
                import traceback
                traceback.print_exc(exc)

            if args.printRepro or args.printExecPlan:
                print ""
                if args.printRepro:
                    print "def " + selector.testCaseName + "():"
                elif args.printExecPlan:
                    print "Test Cases (in execution order):"
                varset = {}
                if "Init" in inputGroups:
                    inputGroups["Init"].PrintExecPlan(inputGroups, selector, varset, args.printRepro)

                for name in sorted(inputGroups.keys()):
                    if name == "Cleanup" or name == "Init":
                        continue
                    inputGroups[name].PrintExecPlan(inputGroups, selector, varset, args.printRepro)

                if "Cleanup" in inputGroups:
                    inputGroups["Cleanup"].PrintExecPlan(inputGroups, selector, varset, args.printRepro)

                print
                if args.printRepro:
                    print selector.testCaseName + "()"

            if args.stopOnError:
                os._exit(os.EX_SOFTWARE)

            #
            # Attempt to cleanup even though we failed, but don't let a 
            # failed _Cleanup call obfuscate the true error.
            #
            try:
                _Cleanup()
            except:
                pass

            # bubble the error up to mentor
            raise

    # Final clean up, If this one fails, let it bubble up. This is kind of a 
    # consistency problem, since if this fails, we wont print or abort.
    _Cleanup()


# --------------------------------------------------------------------------- #
# Module Parser & Main Entry Point
# --------------------------------------------------------------------------- #

def ConstructFixture(uberFixture, args):
    """Module inspector and combination logic. 

    Aruguments: 
     * uberFixture should be a class in which test cases will be deposited.
     * See the argParser for details on what will be in args
    """

    import types

    sys.path.append(os.path.dirname(FindDataFile('combineScripts/')))

    mod = __import__(args.pyFile.replace(".py", ""))
    mod.TRACE = TRACE
    members = [mod.__dict__[pub] 
                for pub in mod.__dict__.keys() 
                    if not pub.startswith("_")]

    inputGroups = {}

    # first build a list of all equivalence classes
    for member in members:
        if type(member) == types.ClassType:
            methods = [member.__dict__[meth] for meth in member.__dict__.keys()
                        if not meth.startswith("_")]
            inputGroups[member.__name__] = InputGroup(member, methods)

    # Setup the iteration filter.
    i = 0
    iteration = None
    if args.runOnlyList:
        iteration = [int(it) for it in args.runOnlyList.split(",")]
        print 
        print "*** NOTICE: Skipping all combinations except %s" % str(iteration)
        print

    for selector in ProductMap(inputGroups):
        #
        # Express each test as a test case in the provided mentor fixture.
        # This will create a case-by-case table in the HTML report when a test
        # fails.
        #
        testCaseName = "Test" + str(i).rjust(6, "0")
        if iteration and not i in iteration:
            if TRACE:
                print "Skipping %d, desired iteration is %s" % (i, str(iteration))
            i += 1
            continue

        selector.testCaseName = testCaseName 
        func = lambda self, inputGroups=inputGroups, selector=selector: \
                      _TestCase(inputGroups, selector)

        # Without this, the Runner filters out the tests, even though they are
        # discovered.
        func.__name__ = testCaseName 

        # Update the UberFixture class definition.
        setattr(uberFixture, testCaseName, func)

        i += 1


def ParseArgs():
    import argparse
    parser = argparse.ArgumentParser(prog=sys.argv[0],
                                description='Combinatorial unit test generator') 

    parser.add_argument('--stopOnError', dest='stopOnError', 
                        help='Stop running tests when tests fail',
                        action='store_true')

    parser.add_argument('--printRepro', dest='printRepro', 
                        help='Print a repro function when tests fail',
                        action='store_true')

    parser.add_argument('--noExecPlan', dest='printExecPlan', 
                        help='Do not print the execution plan when tests fail',
                        action='store_false')

    parser.add_argument('--debug', dest='debug', 
                        help='Print debuging output',
                        action='store_true')

    # XXX default to testUsdApi.py until http://bug/74622 is fixed
    parser.add_argument('--pyFile', dest='pyFile', action='store',
                        help='The python module from which to generate tests.'
                        'The module must be importable from the sys path and '
                        'should not include directory names.',
                        required=True)

    parser.add_argument('--runOnly', dest='runOnlyList', action='store',
                        help='Only run the comma delimited list of test numbers')

    results = parser.parse_args()

    return results 


if __name__ == "__main__":
    args = ParseArgs()
    TRACE = args.debug

    # magic
    ConstructFixture(UberFixture, args)

    runner = Runner().Main()
