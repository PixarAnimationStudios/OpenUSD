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


#
# Normal python imports
#
import sys, os

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
from usdotio.schema.options import Options, LogLevel
from usdotio.schema.clip import Clip
from usdotio.schema.gap import Gap
from usdotio.schema.stack import Stack
from usdotio.schema.timeline import Timeline
from usdotio.schema.transition import Transition
from usdotio.schema.track import Track
from usdotio.schema.effect import Effect
from usdotio.schema.linear_time_warp import LinearTimeWarp

    
class UsdOtioAdd:
    """A class to add an .otio (OpenTimelineIO) file to a .usd file.
    """

    def __init__(self, usd_file, otio_file, output_file, path = '/',
                 comment = True):
        """Constructor

        Args:
        usd_file (str): A valid .usd file.
        otio_file (str): A valid OpenTimelineIO file (or one that adaptors
                         accept).
        output_file (str): Name of the output .usd file.
        path (str): USD path to attach the otio information to.
        comment (bool): Whether the output .usd file has a comment at the 
                        beginning.  For unit tests, you will want to
                        disable the comments.
        """

        #
        # Store these for easy access.
        #
        self.usd_file = usd_file
        self.otio_file = otio_file
        self.output_file = output_file
        self.path = path
        self.comment = not comment

        #
        # Initialize some counters
        #
        self.audio_track_index = 1
        self.clip_index = 1
        self.effect_index = 1
        self.gap_index = 1
        self.stack_index = 1
        self.track_index = 1
        self.transition_index = 1
        self.video_track_index = 1

    def create_effect(self, stage, usd_path, effect):
        if isinstance(effect, otio.schema.LinearTimeWarp):
            usd_path = usd_path + f'/LinearTimeWarp_{self.effect_index}'
            usd_effect_item = LinearTimeWarp(effect)
        else:
            usd_path = usd_path + f'/Effect_{self.effect_index}'
            usd_effect_item = Effect(effect)
            
        usd_effect_item.to_usd(stage, usd_path)

    def process_effects(self, stage, usd_path, effects):
        for effect in effects:
            self.create_effect(stage, usd_path, effect)
            self.effect_index += 1

    def process_child(self, stage, track_path, child):
        usd_item = None
        usd_path = None
        can_have_effects = True
                
        if isinstance(child, otio.schema.Clip):
            usd_path = track_path + f'/Clip_{self.clip_index}'
            usd_item = Clip(child)
            self.clip_index += 1
        elif isinstance(child, otio.schema.Transition):
            usd_path = track_path + f'/Transition_{self.transition_index}'
            usd_item = Transition(child)
            can_have_effects = False
            self.transition_index += 1
        elif isinstance(child, otio.schema.Gap):
            usd_path = track_path + f'/Gap_{self.gap_index}'
            usd_item = Gap(child)
            self.gap_index += 1
        elif isinstance(child, otio.schema.Stack):
            usd_path = track_path + f'/Stack_{self.stack_index}'
            usd_item = Stack(child)
            self.stack_index += 1
            self.recurse_stack(stage, usd_path, child)
        else:
            print('WARNING: Unknown child {child}')
                
        if usd_item:
            usd_item.to_usd(stage, usd_path)

            if can_have_effects:
                self.process_effects(stage, usd_path, child.effects)
                    
    def recurse_track(self, stage, track_path, track):
        for child in track:
            self.process_child(stage, track_path, child)

        if track.effects:
            self.process_effects(stage, track_path, track.effects)

    def recurse_stack(self, stage, stack_path, stack):
        for child in stack:
            if isinstance(child, otio.schema.Track):
                if child.kind == 'Video':
                    track_path = stack_path + f'/Video_{self.video_track_index}'
                    self.video_track_index += 1
                elif child.kind == 'Audio':
                    track_path = stack_path + f'/Audio_{self.audio_track_index}'
                    self.audio_track_index += 1
                else:
                    if Options.log_level >= LogLevel.WARNING:
                        print(f'Unknown track type {child}!')
                    continue
                track_item = Track(child)
                track_item.to_usd(stage, track_path)
                self.recurse_track(stage, track_path, child)
            else:
                self.process_child(stage, stack_path, child)

    def recurse_timeline(self, stage, usd_path, timeline):
        usd_otio_item = Timeline(timeline)
        usd_otio_item.to_usd(stage, usd_path)

        # Check if there's a stacks attribute.
        stack = None
        stack_path = usd_path + '/Stack'
        
        stack = timeline.tracks
        if stack:
            usd_stack_item = Stack(stack)
        else:
            usd_stack_item = Stack(otio.schema.Stack())
        usd_stack_item.to_usd(stage, stack_path)

        self.recurse_stack(stage, stack_path, stack)
                
    def run(self):
        """
        Run the otio add algorithm.
        """
        
        #
        # Open the original scene file
        # 
        stage = Usd.Stage.Open(self.usd_file)
                
        #
        # Try to validate the otio file
        #
        try:
            timeline = otio.adapters.read_from_file(self.otio_file)
        except ValueError as e:
            if self.otio_file.endswith('.otio'):
                print(f'ERROR: Corrupt .otio file "{self.otio_file}: {e}"!',
                      file=sys.stderr)
            else:
                print(f'ERROR: Could not convert "{self.otio_file}" with '
                      'any adapter!', file=sys.stderr)
            exit(1)
            
        #
        # Create an USD otio primitive at path/otio.
        #
        usd_path = self.path
        usd_prim = stage.GetPrimAtPath(usd_path)
        if usd_prim: 
            prim_type = usd_prim.GetTypeName()
            if prim_type != 'OtioTimeline':
                print(f'''USD path "{usd_path}" already has a primitive, 
of type {prim_type}!

Use -p <path> for passing the path to a new
path or an already existing OtioTimeline primitive.

Valid OtioTimeline primitives in stage:''')
                found = False
                for x in stage.Traverse():
                    if x.GetTypeName() == 'OtioTimeline':
                        print(f'\t{x} is an OtioTimeline primitive.')
                        found = True
                if not found:
                    print('\tNone')
                exit(1)


        self.recurse_timeline(stage, usd_path, timeline)
        
        
        #
        # Export modified stage to output file
        #
        if self.output_file == self.usd_file:
            if Options.log_level >= LogLevel.NORMAL:
                print('WARNING: Saving over original USD file.')
                Options.continue_prompt()
        else:
            #
            # Check if otio file already exists
            #
            if os.path.isfile(self.output_file):
                if Options.log_level >= LogLevel.NORMAL:
                    print(f'"{self.output_file}" already exists!  '
                          'Will overwrite it.')
                    Options.continue_prompt()
                

        stage.Export(self.output_file, addSourceFileComment=self.comment)
        if Options.log_level >= LogLevel.NORMAL:
            print(f'Saved "{self.output_file}".')
