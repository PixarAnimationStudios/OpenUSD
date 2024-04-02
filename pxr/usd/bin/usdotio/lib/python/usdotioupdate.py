# Copyright 2024 Gonzalo Garramuño for Signly
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
import sys, os, math

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
    import opentimelineio.opentime as otime
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
from usdotio.schema.track import Track
from usdotio.schema.functions import *
from usdotio.usdotioadd import UsdOtioAdd

#
# usdotio's omni classes should go here, but to simplify we are using
# IsA() comparisons against string CONSTANTS
#
import usdotio.schema.omni as omnischema


    
class UsdOtioUpdate:
    """A class to update an .usd file with sequencer information to a .usd
    file with an .otio Timeline.
    """
    def __init__(self, usd_file, output_file, path = '/', comment = True):
        """Constructor

        Args:
        usd_file (str): A valid .usd file with Sequencer information.
        output_file (str): Name of the output .usd file.
        path (str): USD path to attach the otio information to.
        comment (bool): Whether the output .usd file has a comment at the 
                        beginning.  For unit tests, you will want to
                        disable the comments.
        """

        #
        # Store these user parameters for easy access.
        #
        self.usd_file = usd_file
        self.output_file = output_file
        self.path = path
        self.comment = not comment

        #
        # OpenTimelineIO auxiliary variables 
        #
        self.timeline = None
        self.tracks   = []
        self.clip     = None
        self.available_range = None
        self.current_time = 0.0

        #
        # USD stage properties
        #
        self.startTimecode = 1
        self.endTimecode = 1000
        self.fps = 24.0

    def add_gap(self, track, index, duration):
        source_range = otime.TimeRange(otime.RationalTime(0.0, self.fps),
                                       otime.RationalTime(duration, self.fps))
        
        gap = otio.schema.Gap()
        gap.source_range = source_range

        if index >= 0:
            track.insert(index, gap)
        else:
            track.append(gap)
        
        
    def process_omnisound(self, stage, asset_path, asset_prim):
        name = asset_prim.GetName()
        
        target_url = get_asset(asset_prim, 'filePath')
        media = otio.schema.ExternalReference(target_url)
        media.name = name
        

        if self.available_range:
            media.available_range = self.available_range
            
        self.clip.media_reference = media

        
        
    def process_asset_clip(self, stage, track_path, asset_clip_prim):
        startTime = get_timecode(asset_clip_prim, 'startTime')
        endTime =  get_timecode(asset_clip_prim, 'endTime')

        playStart = get_timecode(asset_clip_prim, 'playStart')
        playEnd = get_timecode(asset_clip_prim, 'playEnd')

        #
        # Update current time to start of this asset
        #
        self.current_time = startTime - self.startTimecode

        source_range = otime.TimeRange.range_from_start_end_time_inclusive(
            otime.RationalTime(startTime, self.fps),
            otime.RationalTime(endTime, self.fps))

        self.available_range = None
        
        if not math.isnan(playStart) or not math.isnan(playEnd):

            if math.isnan(playStart):
                playStart = startTime

            if math.isnan(playEnd):
                playEnd = endTime
                
            self.available_range = \
                otime.TimeRange.range_from_start_end_time_inclusive(
                otime.RationalTime(playStart, self.fps),
                otime.RationalTime(playEnd, self.fps))

        name = asset_clip_prim.GetName()
        
        self.clip = otio.schema.Clip()
        self.clip.name = name
        self.clip.source_range = source_range

        valid_asset = False
        for usd_prim in asset_clip_prim.GetChildren():
            usd_path = usd_prim.GetPath()
            if usd_prim.IsA(omnischema.OMNI_SOUND):
                self.process_omnisound(stage, usd_path, usd_prim)
                valid_asset = True
                break
            else:
                pass

        track = self.tracks[-1]
        
        if valid_asset:
            #
            # Add the clip at the end
            #
            track.append(self.clip)

            #
            # Check if its start time is less than our current time
            #
            trimmed_range = self.clip.trimmed_range_in_parent()
            clip_start_time = trimmed_range.start_time.value
            if clip_start_time < self.current_time:
                gap_duration = self.current_time - clip_start_time
                previous_to_last_child = len(track) - 1
                self.add_gap(track, previous_to_last_child, gap_duration)
        else:
            print(f'Warning: AssetClip found but without a valid Asset')

        # Reset clip
        self.clip = None

        
                    
    def recurse_track(self, stage, track_path, track_prim):
        track_usd_type = track_prim.GetAttribute('trackType').Get()

        if not track_prim.IsA(omnischema.TRACK_AUDIO_TYPE) and \
           not track_prim.IsA(omnischema.TRACK_VIDEO_TYPE):
            return

        track_kind = track_usd_type

        name = track_prim.GetName()
        
        track = otio.schema.Track()
        track.name = name
        track.kind = track_kind

        #
        # Store the track in our internal list
        #
        self.tracks.append(track)

        
        for usd_prim in track_prim.GetAllChildren():
            usd_path  = usd_prim.GetPath()
            if usd_prim.IsA(omnischema.ASSET_CLIP):
                self.process_asset_clip(stage, usd_path, usd_prim)

        track_duration = track.duration().value
        sequence_duration = self.endTimecode - self.startTimecode
        if track_duration < sequence_duration:
            gap_duration = sequence_duration - track_duration
            self.add_gap(track, -1, gap_duration)
            
                
    def recurse_sequence(self, stage, sequence_prim):
        self.timeline = otio.schema.Timeline()

        #
        # Get global timing information for Stage
        #
        self.startTimecode = stage.GetStartTimeCode()
        self.endTimecode = stage.GetEndTimeCode()
        self.fps         = stage.GetTimeCodesPerSecond()

        #
        # Look for omni Tracks
        #
        for usd_prim in sequence_prim.GetAllChildren():
            usd_path  = usd_prim.GetPath()
            if usd_prim.IsA(omnischema.TRACK):
                self.current_time = 0.0
                self.recurse_track(stage, usd_path, usd_prim)

        #
        # Create a dummy OTIO Stack and append all valid OTIO Tracks
        #
        stack = otio.schema.Stack()
        for track in self.tracks:
            stack.append(track)
        self.timeline.tracks = stack
        
        
    def run(self):
        """
        Run the otio add algorithm.
        """
        
        #
        # Open the original scene file
        # 
        stage = Usd.Stage.Open(self.usd_file)
        
        #
        # Create an USD otio primitive at path/otio.
        #
        usd_path = self.path
        usd_prim = stage.GetPrimAtPath(usd_path)
        if not usd_prim:
            print(f'''USD path "{usd_path}" is invalid Sequence primitive!

Use -p <path> for passing the path to a new
path or an already existing Sequence primitive.

Valid Sequence primitives in stage:''')
            found = False
            for x in stage.Traverse():
                if x.IsA(omnischema.SEQUENCE):
                    print(f'\t{x} is a Sequence primitive.')
                    found = True
            if not found:
                print('\tNone')
            exit(1)
            
        
        if usd_prim: 
            prim_type = usd_prim.GetTypeName()
            if not usd_prim.IsA(omnischema.SEQUENCE):
                print(f'''USD path "{usd_path}" already has a primitive, 
of type {prim_type}!

Use -p <path> for passing the path to a new
path or an already existing Sequence primitive.

Valid Sequence primitives in stage:''')
                found = False
                for x in stage.Traverse():
                    if x.GetTypeName() == 'Sequence':
                        print(f'\t{x} is a Sequence primitive.')
                        found = True
                if not found:
                    print('\tNone')
                exit(1)

        
        self.recurse_sequence(stage, usd_prim)

        otio_add = UsdOtioAdd(self.output_file, None, self.output_file, '/',
                              not self.comment)
        
        otio_add.recurse_timeline(stage, '/otio', self.timeline)

        #
        # Remove the whole Sequence information
        #
        stage.RemovePrim(usd_path)
        
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
