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
import sys
import os

from PySide.QtGui import QApplication

import argparse

class Launcher(object):
    '''
    Base class for argument parsing and validation for UsdView
    
    Subclasses can choose to override
      -- GetHelpDescription()
      -- RegisterOptions()
      -- ParseOptions()
      -- ValidateOptions()
    '''
    def __init__(self):
        pass

    def Run(self):
        '''
        the main entry point to launch a process using UsdView.
        '''
        
        parser = argparse.ArgumentParser(prog=sys.argv[0],
                                         description=self.GetHelpDescription())
        self.RegisterPositionals(parser)
        self.RegisterOptions(parser)
        arg_parse_result = self.ParseOptions(parser)
        valid = self.ValidateOptions(arg_parse_result)
        if valid:
            self.__LaunchProcess(arg_parse_result)
            
    def GetHelpDescription(self):
        '''return the help description'''       
        return 'View a usd file'
        
    def RegisterPositionals(self, parser):
        '''
        register positional arguments on the ArgParser
        '''
        parser.add_argument('usdFile', action='store',
                            type=str, 
                            help='The file to view')
        
    def RegisterOptions(self, parser):
        '''
        register optional arguments on the ArgParser
        '''
        parser.add_argument('--renderer', action='store',
                            type=str, choices=['opt', 'simple'], dest='renderer',
                            help='Which renderer to use', default='opt')

        parser.add_argument('--select', action='store', default='/',
                            dest='primPath', type=str,
                            help='A prim path to initially select and frame')
        
        parser.add_argument('--clearsettings', action='store_true', 
                            dest='clearSettings', 
                            help='Restores usdview settings to default')
        
        parser.add_argument('--norender', action='store_true',
                            dest='noRender',
                            help='Display only hierarchy browser')

        parser.add_argument('--unloaded', action='store_true',
                            dest='unloaded', 
                            help='Do not load payloads')

        parser.add_argument('--timing', action='store_true',
                            dest='timing', 
                            help='echo timing stats to console. NOTE: timings will be unreliable when the --mallocTagStats option is also in use')

        parser.add_argument('--memstats', action='store', default='none',
                            dest='mallocTagStats', type=str,
                            choices=['none', 'stage', 'stageAndImaging'],
                            help='Use the Pxr MallocTags memory accounting system to profile USD, saving results to a tmp file, with a summary to the console.  Will have no effect if MallocTags are not supported in the USD installation.')
        
        parser.add_argument('--numThreads', action='store', 
                            type=int, default=0,
                            help='Number of threads used for processing' 
                                 '(0 is max, negative numbers imply max - N)')

        parser.add_argument('--ff', action='store',
                            dest='firstframe', type=int)

        parser.add_argument('--lf', action='store',
                            dest='lastframe', type=int)

        parser.add_argument('--complexity', action='store',
                            type=float, default=1.0, dest='complexity',
                            help='A float complexity value in the closed range [1,2]')
        parser.add_argument('--quitAfterStartup', action='store_true',
                            dest='quitAfterStartup',
                            help='quit immediately after start up')

    
    def ParseOptions(self, parser):
        '''
        runs the parser on the arguments
        '''
        return parser.parse_args()
    
    def ValidateOptions(self, arg_parse_result):
        '''
        Validate and potentially modifies the parsed arguments return True
        if the UsdView Process can launch.  If a child has overridden
        ParseOptions, ValidateOptions is an opportunity to move 
        '''
        if arg_parse_result.complexity < 1.0 or arg_parse_result.complexity > 2.0:
            newComplexity = max(min(2.0, arg_parse_result.complexity), 1.0)
            print >> sys.stderr, "WARNING: complexity %.1f is out of range [1.0, 2.0], using %.1f instead" %  \
                                 (arg_parse_result.complexity, newComplexity)
            arg_parse_result.complexity = newComplexity
        return True
            
    def __LaunchProcess(self, arg_parse_result):
        '''
        after the arguments have been parsed, launch the UI in a forked process
        '''
        # Initialize concurrency limit as early as possible so that it is
        # respected by subsequent imports.
        from pxr import Work
        Work.SetConcurrencyLimitArgument(arg_parse_result.numThreads)

        from mainWindow import MainWindow
        if arg_parse_result.clearSettings:
            MainWindow.clearSettings()

        # Find the resource directory
        resourceDir = os.path.dirname(
            os.path.realpath(__file__)).replace("\\", "/")

        # Create the Qt application
        app = QApplication(sys.argv)

        # apply the style sheet to it
        sheet = open(os.path.join(resourceDir, 'usdviewstyle.qss'), 'r')
        sheetString = sheet.read().replace('RESOURCE_DIR', resourceDir)
        app.setStyleSheet(sheetString)

        mainWindow = MainWindow(None, arg_parse_result)

        if arg_parse_result.quitAfterStartup:
            # Before we quit, process events one more time to make sure the
            # UI is fully populated (and to capture all the timing information
            # we'd want).
            app.processEvents()
            sys.exit(0)

        app.exec_()

