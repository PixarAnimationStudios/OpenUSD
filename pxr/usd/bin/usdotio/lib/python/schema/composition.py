#
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

import json

from .clip import Clip
from .gap import Gap
from .item import Item
from .options import Options, LogLevel
from .rational_time_mixin import RationalTimeMixin
from .time_range_mixin import TimeRangeMixin
from .transition import Transition

class Composition(Item, TimeRangeMixin, RationalTimeMixin):
    """Abstract class to handle a composition (ie. one that can hold Track,
    Stack, Clip, Marker in its "children" attribute)

    """

    FILTER_KEYS = [
        'children',
    ]
    
    def __init__(self, otio_item = None):
        super().__init__(otio_item)
        self.children = []

        
    def append_child(self, child):
        """Append a child to this Composition.

        Args:
        child (usdotio): usdOtio class to append, like usdOtio.clip.Clip

        Returns:
        None

        """
        self.children.append(child)

    def from_usd(self, usd_prim):
        super().from_usd(usd_prim)
        
        #
        # Traverse the stage to get the jsonData of each node
        #
        for child in usd_prim.GetChildren():
            usd_type = child.GetTypeName()
            child_prim = None
            if child.IsA('OtioTrack'):
                child_prim = self._create_track()
            elif child.IsA('OtioStack'):
                child_prim = self._create_stack()
            elif child.IsA('OtioClip'):
                child_prim = Clip()
            elif child.IsA('OtioGap'):
                child_prim = Gap()
            elif child.IsA('OtioTransition'):
                child_prim = Transition()
            elif child.IsA('OtioRationalTime'):
                time_prim = RationalTime()
                usd_name = child.GetName()
                self.jsonData[usd_name] = time_prim.from_usd(child)
                continue
            elif child.IsA('OtioTimeRange'):
                range_prim = TimeRange()
                usd_name = child.GetName()
                self.jsonData[usd_name] = range_prim.from_usd(child)
                continue
            else:
                if Options.log_level >= LogLevel.NORMAL:
                    print('WARNING: (composition.py) Unknown child element ', \
                          child, 'attached to', child_prim, '.')

            if child_prim:
                child_prim.from_usd(child)
                self.append_child(child_prim)
        
        json_strings = [json.loads(x.to_json_string()) for x in self.children]
        self.jsonData['children'] = json_strings
        
        return self.jsonData


    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(Composition.FILTER_KEYS)

        
    def _create_stack(self):
        """An auxiliary class to create a Stack.
        
        Args:
        None

        Returns:
        usdotio.schema.Stack: a valid usdOtio Stack.
        """
        # We need to use import it here to avoid cyclic dependencies
        from .stack import Stack
        return Stack()

    def _create_track(self):
        """An auxiliary class to create a Track.
        
        Args:
        None

        Returns:
        usdotio.schema.Track: a valid usdOtio Track.
        """
        # We need to use import it here to avoid cyclic dependencies
        from .track import Track
        return Track()
