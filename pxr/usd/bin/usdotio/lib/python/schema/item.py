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

import json

from .composable import Composable
from .effect import Effect
from .linear_time_warp import LinearTimeWarp
from .marker import Marker
from .time_range_mixin import TimeRangeMixin

class Item(Composable, TimeRangeMixin):
    """Base abstract class for Items.  It handles "effects",
    "markers" and "source_range" if present.

    Derived classes are expected to override to_usd().
    And derived classes that have additional special attributes
    might need to override also from_usd(). 

    See: .clip.Clip.

    """

    
    FILTER_KEYS = [
        'effects',
        'markers',
        'source_range',
    ]
    
    def __init__(self, otio_item = None):
        super().__init__(otio_item)
        self.markers = []
        self.effects = []
 
    def append_effect(self, effect):
        self.effects.append(effect)
        
    def append_marker(self, marker):
        self.markers.append(marker)

    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(Item.FILTER_KEYS)
        
    def from_json_string(self, s):
        self.jsonData = json.loads(s)

    def from_usd(self, usd_prim):
        super().from_usd(usd_prim)

        #
        # Traverse the stage to get the jsonData of each node
        #
        for child in usd_prim.GetChildren():
            if child.IsA('OtioTimeRange'):
                usd_name = child.GetName()
                self.jsonData[usd_name] = self._create_time_range(child)
            elif child.IsA('OtioLinearTimeWarp'):
                timewarp_prim = LinearTimeWarp()
                timewarp_prim.from_usd(child)
                self.append_effect(timewarp_prim)
            elif child.IsA('OtioEffect'):
                effect_prim = Effect()
                effect_prim.from_usd(child)
                self.append_effect(effect_prim)
            elif child.IsA('OtioMarker'):
                marker_prim = Marker()
                marker_prim.from_usd(child)
                self.append_marker(marker_prim)
            else:
                pass
        
        json_strings = [json.loads(x.to_json_string()) for x in self.effects]
        self.jsonData['effects'] = json_strings
        
        json_strings = [json.loads(x.to_json_string()) for x in self.markers]
        self.jsonData['markers'] = json_strings

        
        return self.jsonData


    def to_usd(self, stage, usd_path):
        self._set_time_range(stage, usd_path, 'source_range')
        
        marker_index = 1
        for m in self.otio_item.markers:
            marker_path = usd_path + f'/marker_{marker_index}'
            marker_prim = Marker(m)
            marker_prim.to_usd(stage, marker_path)

            marker_index += 1
