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

    def Run(self):
        parser = argparse.ArgumentParser(prog=sys.argv[0],
                                         description=self.GetHelpDescription())
        self.RegisterOptions(parser)
        super(TestUsdView, self).RegisterPositionals(parser)
        arg_parse_result = super(TestUsdView, self).ParseOptions(parser)

        # We always set --defaultsettings to ensure a consistent test 
        # environment for our test scripts.
        arg_parse_result.defaultSettings = True
        super(TestUsdView, self).ValidateOptions(arg_parse_result)

        # Further, we ensure usdview won't try to create any state directory
        # to save its state, as that would be bad for running multiple tests
        # in parallel
        os.environ['PXR_USDVIEW_SUPPRESS_STATE_SAVING'] = "1"
        
        self.__LaunchProcess(arg_parse_result)

    def __LaunchProcess(self, arg_parse_result):
        callBack = self._ValidateTestFile(arg_parse_result.testScript)
        (app, appController) = (
            super(TestUsdView, self).LaunchPreamble(arg_parse_result))

        # Set a fixed size on the stage view so that any image tests get a
        # consistent resolution - but only if we've created a viewer
        if appController._stageView:
            appController._stageView.setFixedSize(597,540)

        # Process initial loading events
        app.processEvents()
        callBack(appController)
        # Process event triggered by the callbacks
        app.processEvents()
        # Trigger application shutdown.
        app.instance().closeAllWindows()
        return

    # Verify that we have a valid file as input, and it contains
    # our expected entry point for testing.
    #
    # Upon success, this will return the callback to be called after
    # launching Usdview and doing the initial load pass
    def _ValidateTestFile(self, filePath):
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
        callBack = localVars.get(TEST_USD_VIEW_CALLBACK_IDENT)
        if not callBack:
            sys.stderr.write('Invalid file supplied, must contain a function of '
                             'the signature' + TEST_USD_VIEW_CALLBACK_IDENT +
                             '(appController): {}\n'.format(filePath))
            sys.exit(1)

        errorMsg = ('Invalid function signature, must be of the form: \n ' +
                    TEST_USD_VIEW_CALLBACK_IDENT + ' (appController)\n'
                    'File: ' + filePath + '\n'
                    'Error: %s')

        if sys.version_info.major >= 3:
            (args, varargs, keywords, defaults, _, _, _) = \
                                               inspect.getfullargspec(callBack)
        else:
            (args, varargs, keywords, defaults) = inspect.getargspec(callBack)

        assert not varargs, errorMsg % 'Varargs are disallowed'
        assert not keywords, errorMsg % 'Kwargs are disallowed'
        assert not defaults, errorMsg % 'Defaults are disallowed'
        assert len(args) == 1, errorMsg % 'Incorrect number of args (1 expected)'

        return callBack


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
def _takeShot(self, fileName, iterations=10, waitForConvergence=False):
    self._processEvents(iterations, waitForConvergence)
    viewportShot = self.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

AppController._takeShot = _takeShot


if __name__ == '__main__':
    TestUsdView().Run()
