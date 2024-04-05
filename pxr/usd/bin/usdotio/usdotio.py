#!/usr/bin/python3.10
#
# Copyright 2024 Gonzalo Garramuño for Signly, Ltd.
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

VERSION = '0.0.4'



#
# Python standard imports
#
import sys, os
import argparse, json

#
# Instead of relying in the user environment being set, we will modify this
# from within python so that the script will work fine in all OSes.
#
script_dir = os.path.dirname(os.path.abspath(__file__))

# Split the path based on the directory separator
path_parts = script_dir.split(os.path.sep)

# Remove the last element from the path_parts list
path_parts.pop()

# Join the remaining parts back together to form the new path
install_path = os.path.sep.join(path_parts)

#
# Otio plugin dir
#
share_parts = [install_path, 'share', 'usd', 'examples', 'plugin', 'usdotio']
otio_plugin_dir = os.path.sep.join(share_parts)

#
# Add usdotio directory to PXR_PLUGINPATH_NAME
#
plugin_path = os.environ.get('PXR_PLUGINPATH_NAME', '')
os.environ['PXR_PLUGINPATH_NAME'] = plugin_path + os.pathsep + otio_plugin_dir

#
# Usd python dir
#
usd_python_parts = [install_path, 'lib', 'python']
usd_python_path = os.path.sep.join(usd_python_parts)

#
# Modify sys.path
# 
sys.path.insert(0, otio_plugin_dir)
sys.path.insert(0, usd_python_path)

#
# USD imports
#
try:
    from pxr import Usd
except ImportError:
    print(f'''
"pxr" python module not found.  Check PYTHONPATH and LD_LIBRARY_PATH on Linux or
macOS.
Check PATH on Windows.
''')
    exit(1)

#
# usdotio imports
#
from usdotio.schema.options import Options, LogLevel
from usdotio.usdotioadd import UsdOtioAdd
from usdotio.usdotiosave import UsdOtioSave
from usdotio.usdotioupdate import UsdOtioUpdate

class UsdOtio:
    """
    Class to add or extract an .otio json data file from a .usd fle
    """
    def __init__(self):
        self.path = self.output_file = self.otio_file = None
        self.parse_arguments()
        self.run()

    def run(self):
        """
        Run the conversion
        """
        if self.mode == 'add':
            engine = UsdOtioAdd(self.usd_file, self.otio_file,
                                self.output_file, self.path,
                                self.noComment)
        elif self.mode == 'save':
            engine = UsdOtioSave(self.usd_file, self.otio_file, self.path)
        elif self.mode == 'update':
            engine = UsdOtioUpdate(self.usd_file, self.output_file, self.path)
        else:
            raise RuntimeError('Uninplemented mode yet - Patches welcome!')
        engine.run()
        
    def run_otio_save(self):
        """
        Run the otio add algorithm.
        """
        #
        # Open the original scene file
        # 
        stage = Usd.Stage.Open(self.usd_file)

        usd_path = self.path

        #
        # Get primitive at path
        #
        usd_prim = stage.GetPrimAtPath(usd_path)
        if not usd_prim:
            print(f'Invalid USD path "{usd_path}" for Otio primitive!')
            print('Use -p <path> for passing the path to the '
                  'Otio primitive.\n')
            print('Valid Otio primitives in stage:\n')
            for x in stage.Traverse():
                if x.GetTypeName() == 'Otio':
                    print(f'\t{x} is an Otio primitive.')
            exit(1)

        #
        # Check we have an Otio primitive
        #
        prim_type = usd_prim.GetTypeName()
        if prim_type != 'OtioTimeline':
            print(f'Invalid Otio primitive type. Is {prim_type}. ')
            exit(1)

        json_data = usd_prim.GetAttribute('jsonData').Get()

        #
        # Check if otio file already exists
        #
        if os.path.isfile(self.otio_file):
            print(f'"{self.otio_file}" already exists.  Will overwrite it.')
            Options.continue_prompt()
        
        #
        # Write out the json data
        #
        with open(self.otio_file, 'w') as f:
            f.write(json_data)

        if Options.log_level:
            print('Extracted:\n\n', json_data)
    
    @staticmethod
    def valid_usd(usd):
        """
        Validate a USD file by checking its extension.

        Args:
            usd (str): USD filename
        """
        if usd.endswith('.usd') or usd.endswith('.usda') or \
           usd.endswith('.usdz') or usd.endswith('.usdc'):
            return
        print(f'ERROR: Invalid extension for USD file: {usd}',
              file=sys.stderr)
        exit(1)
        
    @staticmethod
    def parse_verbosity(value):
        """
        Parses the verbosity level argument, handling both enum names and integers.
        """
        try:
            return LogLevel[value.upper()]  # Try converting to enum member by name
        except KeyError:
            try:
                return int(value)  # Fallback to integer conversion
            except ValueError:
                raise argparse.ArgumentTypeError(f"Invalid verbosity level: {value}")

        
    def parse_arguments(self):
        """
        Parse the command-line's arguments
        """
    
        description=f"""
       usdotio v{VERSION}

       A program to embed and extract an .otio file from a .usd file.

       It can also convert Omniverse's sequencer to an .usd file with 
       embedded .otio data."""
            
        parser = argparse.ArgumentParser(
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=description)
        subparsers = parser.add_subparsers(dest='mode',
                                           help='Mode of operation')
        
            
        

        #
        # 'add' parser
        #
        add_parser = subparsers.add_parser('add', help='Add mode')
        add_parser.add_argument('-v', '--log', nargs='?',
                                default=LogLevel.NORMAL, dest='verbose',
                                type=self.parse_verbosity,
                                help='Set verbosity level: ' \
                                'debug trace verbose info normal quiet ' \
                                'or integer value')
        add_parser.add_argument('-y', '--yes', action='store_true',
                                help='Answer yes to all questions')
        add_parser.add_argument('-p', '--usd-path', type=str, nargs='?',
                                const='/otio', 
                                help='USD path to attach or extract .otio '
                                'primitive to.  If no path provides, defaults '
                                'to "/otio".')
        add_parser.add_argument('otio_file', type=str,
                                help='Name of .otio file to add.')
        add_parser.add_argument('usd_file', type=str,
                                help='Name of .usd file to add otio data')
        add_parser.add_argument('-o', '--usd-output-file', type=str, nargs='?',
                                help='USD output file.  '
                                'If no output file is provided, defaults to'
                                'overwrite the same usd file.')
        add_parser.add_argument('-n', '--noComment', action='store_true',
                                help='do not write a comment specifying how ' \
                                'the usd file was generated')

        #
        # 'save' parser
        #
        save_parser = subparsers.add_parser('save', help='Save mode')
        save_parser.add_argument('-v', '--log', nargs='?',
                                 default=LogLevel.NORMAL, dest='verbose',
                                 type=self.parse_verbosity,
                                 help='Set verbosity level: ' \
                                 'debug trace verbose info normal quiet ' \
                                 'or integer value')
        save_parser.add_argument('-y', '--yes', action='store_true',
                                 help='Answer yes to all questions')
        save_parser.add_argument('-p', '--usd-path', type=str, nargs='?',
                                 const='/otio', 
                                 help='USD path to attach or extract .otio '
                                 'primitive to.  If no path provides, defaults '
                                 'to "/".')
        save_parser.add_argument('otio_file', type=str,
                                 help='Name of .otio file to add or save.')
        save_parser.add_argument('usd_file', type=str,
                                 help='Name of .usd file to extract otio data')
        save_parser.add_argument('-n', '--noComment', action='store_true',
                                 help='do not write a comment specifying how ' \
                                 'the usd file was generated')
        
        #
        # 'update' parser
        #
        update_parser = subparsers.add_parser('update', help='Omniverse update sequencer to .otio conversion mode')
        update_parser.add_argument('-v', '--log', nargs='?',
                               default=LogLevel.NORMAL, dest='verbose',
                               type=self.parse_verbosity,
                                 help='Set verbosity level: ' \
                                 'debug trace verbose info normal quiet ' \
                                 'or integer value')
        update_parser.add_argument('-y', '--yes', action='store_true',
                               help='Answer yes to all questions')
        update_parser.add_argument('-p', '--usd-path', type=str, nargs='?',
                               const='/Sequence', 
                               help='USD path to attach or extract Sequencer '
                               'primitive to.  If no path provides, defaults '
                               'to "/Sequence".')
        update_parser.add_argument('usd_file', type=str, help='Name of .usd file to add or extract otio data')
        update_parser.add_argument('-o', '--usd-output-file', type=str,
                                   nargs='?',
                                   help='USD output file.  '
                                   'If no output file is provided, defaults to'
                                   'overwrite the same usd file.')
        update_parser.add_argument('-n', '--noComment', action='store_true',
                                   help='do not write a comment specifying how ' \
                                   'the usd file was generated')
        
        args = parser.parse_args()

        #
        # Copy arguments to class
        #
        self.mode = args.mode
        if not self.mode:
            print(parser.format_help())
            exit(1)
            
        Options.log_level = args.verbose
        Options.yes = args.yes
        self.usd_file = args.usd_file
        self.path = args.usd_path

        if self.mode != 'save':
            self.output_file = args.usd_output_file
            self.noComment = args.noComment

        if self.mode != 'update':
            self.otio_file = args.otio_file
            
        if not self.output_file:
            self.output_file = self.usd_file
        
        if Options.log_level == LogLevel.DEBUG:
            print('LogLevel mode enabled!')
            print('\nEnvironment:\n')
            print(f'PXR_PLUGINPATH_NAME={os.environ["PXR_PLUGINPATH_NAME"]}')
            print('')
            print(f'sys.path={os.path.pathsep.join(sys.path)}')
            
        if Options.log_level >= LogLevel.NORMAL.value:
            print(f'usdotio v{VERSION}')
            print('')
            print(f'Selected mode: {self.mode}')
            print('')
            
        #
        # Validate arguments
        #
        if self.otio_file:
            if self.otio_file.endswith('.otioz'):
                print(f'.otioz files currently are not supported.  Sorry!',
                      file=sys.stderr)
                exit(1)

        self.valid_usd(self.usd_file)
        if not os.path.exists(self.usd_file):
            print(f'"{self.usd_file}" does not exist!', file=sys.stderr)
            exit(1)
        if not os.path.isfile(self.usd_file):
            print(f'"{self.usd_file}" is not a file!', file=sys.stderr)
            exit(1)
            
        if self.output_file:
            self.valid_usd(self.output_file)
                
        if self.path:
            #
            # Sanitize path for usd
            #
            if self.path[0] != '/':
                self.path = '/' + self.path
        else:
            if self.mode != 'update':
                self.path = '/otio'
            else:
                self.path = '/Sequence'
            
                
            if self.mode == "add":
                if Options.log_level >= LogLevel.INFO.value:
                    print(f'\nAdding "{self.otio_file}" to\n'
                          f'USD path "{self.path}" in\n'
                          f'"{self.usd_file}"...')
            if self.output_file != self.usd_file:
                if Options.log_level >= LogLevel.INFO.value:
                    print(f'\nSaving to {self.output_file}')
            elif self.mode == 'save':
                if Options.log_level >= LogLevel.INFO.value:
                    print(f'\nGetting otio data from USD path "{self.path}"...\n')
                    print(f'Saving to "{self.otio_file}"')

if __name__ == '__main__':
    usd_otio = UsdOtio()
    exit(0)
    
