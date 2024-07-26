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

import opentimelineio as otio

from .named_base import NamedBase
from .time_range_mixin import TimeRangeMixin

class Marker(NamedBase, TimeRangeMixin):
    """Class defining a marker (an annotation in the timeline/track/item).

    """
    
    FILTER_KEYS = [
        'marked_range',
    ]
    
    def __init__(self, otio_item = None):
        super().__init__(otio_item)
        if not otio_item:
            self.jsonData = \
                json.loads(otio.schema.Marker().to_json_string())

    def from_usd(self, usd_prim):
        super().from_usd(usd_prim)

        #
        # Traverse the stage to get the jsonData of each node
        #
        for x in usd_prim.GetChildren():
            usd_type = x.GetTypeName()
            if x.IsA('OtioTimeRange'):
                usd_name = x.GetName()
                self.jsonData[usd_name] = self._create_time_range(x)
            else:
                print(f'WARNING: (marker.py) Unknown node {usd_type}' \
                      f'attached to {usd_prim}!')
        
        return self.jsonData
    
    def to_usd(self, stage, usd_path):
        self._set_time_range(stage, usd_path, 'marked_range')
        
        usd_prim = self._create_usd(stage, usd_path, 'OtioMarker')
        return usd_prim
        
    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(Marker.FILTER_KEYS)
