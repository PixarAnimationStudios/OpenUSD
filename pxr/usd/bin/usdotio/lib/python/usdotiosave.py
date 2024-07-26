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

#
# Standard python imports
#
import os

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
''', file=sys.stderr)
    exit(1)

#
# OpentimelineIO imports
#
try:
    import opentimelineio as otio
except ImportError:
    print(f'''
Python's opentimelineio module not found!

Please run:
    pip install opentimelineio
''', file=sys.stderr)
    exit(1)

#
# usdotio helper classes' imports here
#
from usdotio.schema.timeline import Timeline
from usdotio.schema.options import Options, LogLevel
    
class UsdOtioSave:
    """Class used to save (extract) an OpenTimelineIO (.otio) file from an
    USD (.usd, usda, etc.) with embedded information.
    """

    def __init__(self, usd_file, otio_file, usd_path = '/'):
        self.usd_file = usd_file
        self.otio_file = otio_file
        self.usd_path = usd_path

    def run(self):
        """
        Run the otio save (extract .otio) algorithm.
        """

        #
        # Open the original scene file
        #
        try:
            stage = Usd.Stage.Open(self.usd_file)
        except:
            print(f'ERROR: Could not open USD file {self.usd_file}',
                  file=sys.stderr)
            exit(1)
        #
        # Get an USD otio primitive at path/otio.
        #
        usd_prim = stage.GetPrimAtPath(self.usd_path)
        
        valid_path = True
        if not usd_prim:
            valid_path = False
            print(f'No primitive at USD path "{self.usd_path}"!')
        else: 
            usd_type = usd_prim.GetTypeName()
            if usd_type != 'OtioTimeline':
                valid_path = False
                print(f'''USD path "{self.usd_path}" not an OtioTimeline primitive, 
but type {usd_type}!

''')

        if not valid_path:
            print(f'''
Use -p <path> for passing the path to an already existing OtioTimeline primitive.

Valid OtioTimeline primitives in stage:''')
            found = False
            for x in stage.Traverse():
                if x.GetTypeName() == 'OtioTimeline':
                    print(f'\t{x} is an OtioTimeline primitive.')
                    found = True
            if not found:
                print('\tNone')
            exit(1)

        #
        # Create a timeline and extract the .otio json data from this usd
        # OtioTimeline primitive
        #
        timeline = Timeline()
        json_data = timeline.from_usd(usd_prim)
        
        #
        # Check if otio file already exists
        #
        if os.path.isfile(self.otio_file):
            if Options.log_level >= LogLevel.NORMAL:
                print(f'"{self.otio_file}" already exists!  Will overwrite it.')
                Options.continue_prompt()
            os.remove(self.otio_file)
        
        #
        # Write out the json data
        #
        if Options.log_level == LogLevel.DEBUG:
            with open(self.otio_file, 'w') as f:
                f.write(json_data)
                
            if Options.log_level >= LogLevel.NORMAL:
                print(f'''To tabulate and verify the file "{self.otio_file}" use otiocat:

otiocat "${self.otio_file}" > /tmp/test.otio
''')
        else:
            timeline = otio.schema.Timeline.from_json_string(json_data)
            timeline.to_json_file(self.otio_file)

        if Options.log_level >= LogLevel.NORMAL:
            print(f'Saved "{self.otio_file}".')
