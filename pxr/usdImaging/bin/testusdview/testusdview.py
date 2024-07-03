#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# This is a wrapper around Usdview's launcher code to allow injection of a 
# script for testing purposes. Tests can be supplied in the form of a python
# script which provides a function testUsdviewInputFunction(appController)
# as the entry point. This is passed to testusdview with the (required) 
# --testScript argument. Other than this signature requirement, scripts are
# free to do whatever they want with mainwindow for testing.

import pxr.Usdviewq as Usdviewq
from pxr.Usdviewq import AppController
from pxr.Usdviewq.qt import QtWidgets

import sys, os, inspect, argparse

TEST_USD_VIEW_CALLBACK_IDENT = "testUsdviewInputFunction"

class TestUsdView(Usdviewq.Launcher):
    def RegisterOptions(self, parser):
        super(TestUsdView, self).RegisterOptions(parser)
        parser.add_argument('--testScript', action='store', required=True,
                            help="The test script to run after loading. "
                                 "This script must expect to take a single")

    def ParseOptions(self, parser):
        arg_parse_result = super(TestUsdView, self).ParseOptions(parser)

        # We always set --defaultsettings to ensure a consistent test 
        # environment for our test scripts.
        arg_parse_result.defaultSettings = True
        return arg_parse_result

    def OverrideMaxSamples(self):
        return False

    def LaunchPreamble(self, arg_parse_result):
        # Load the test script. This must be done before the creation
        # of the AppController in the base class' LaunchPreamble.
        self._LoadTestFile(arg_parse_result.testScript)
        return super(TestUsdView, self).LaunchPreamble(arg_parse_result)

    def LaunchProcess(self, arg_parse_result, app, appController):
        # Set a fixed size on the stage view so that any image tests get a
        # consistent resolution - but only if we've created a viewer
        if appController._stageView:
            appController._stageView.setFixedSize(597,540)

        # Process initial loading events
        app.processEvents()
        self.callBack(appController)
        # Process event triggered by the callbacks
        app.processEvents()
        # Trigger application shutdown.
        app.instance().closeAllWindows()

    # Verify that we have a valid file as input, and it contains
    # our expected entry point for testing.
    #
    # Upon success, this will set the callback to be called after
    # launching Usdview and doing the initial load pass into
    # self.callBack.
    def _LoadTestFile(self, filePath):
        if not os.path.exists(filePath):
            sys.stderr.write('Invalid file supplied, does not exist: {}\n'
                .format(filePath))
            sys.exit(1)

        if not os.access(filePath, os.R_OK):
            sys.stderr.write('Invalid file supplied, must be readable: {}\n'
                .format(filePath))

        # Ensure that we are passed a valid python file
        _, ext = os.path.splitext(filePath)
        if ext != '.py':
            sys.stderr.write('Invalid file supplied, must be a python file: '
                '{}\n'.format(filePath))
            sys.exit(1)

        # Grab the function from the input python file
        # if it doesn't contain our expected callback fn, bail
        localVars = {}
        with open(filePath) as inputFile:
            code = compile(inputFile.read(), filePath, 'exec')
            exec(code, localVars)
        self.callBack = localVars.get(TEST_USD_VIEW_CALLBACK_IDENT)
        if not self.callBack:
            sys.stderr.write('Invalid file supplied, must contain a function of '
                             'the signature' + TEST_USD_VIEW_CALLBACK_IDENT +
                             '(appController): {}\n'.format(filePath))
            sys.exit(1)

        errorMsg = ('Invalid function signature, must be of the form: \n ' +
                    TEST_USD_VIEW_CALLBACK_IDENT + ' (appController)\n'
                    'File: ' + filePath + '\n'
                    'Error: %s')

        (args, varargs, keywords, defaults, _, _, _) = \
                                           inspect.getfullargspec(self.callBack)

        assert not varargs, errorMsg % 'Varargs are disallowed'
        assert not keywords, errorMsg % 'Kwargs are disallowed'
        assert not defaults, errorMsg % 'Defaults are disallowed'
        assert len(args) == 1, errorMsg % 'Incorrect number of args (1 expected)'

# Monkey patch AppController to add helper test interface methods

def _processEvents(self, iterations=10, waitForConvergence=False):
    # Qt does not guarantee that a single call to processEvents() will
    # process all events in the event queue, and in some builds, on
    # some platforms, we sporadically or even repeatably see test failures
    # in which the selection does not change, presumably because the
    # events were not all getting processed, when we simply called
    # processEvents once.  So we do it a handful of times whenever we
    # generate an event, to increase our odds of success.
    for x in range(iterations):
        QtWidgets.QApplication.processEvents()

    # Need to wait extra for progressive rendering images to converge
    if (waitForConvergence):
        # Wait until the image is converged
        while not self._stageView._renderer.IsConverged():
            QtWidgets.QApplication.processEvents()

AppController._processEvents = _processEvents

# Take a shot of the viewport and save it to a file.
def _takeShot(self, fileName, iterations=10, waitForConvergence=False,
              cropToAspectRatio=False):
    self._processEvents(iterations, waitForConvergence)
    viewportShot = self.GrabViewportShot(cropToAspectRatio=cropToAspectRatio)
    viewportShot.save(fileName, "PNG")

AppController._takeShot = _takeShot


if __name__ == '__main__':
    TestUsdView().Run()
