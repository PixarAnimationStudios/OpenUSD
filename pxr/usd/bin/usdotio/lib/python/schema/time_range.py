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

from .base import Base
from .rational_time_mixin import RationalTimeMixin

class TimeRange(Base, RationalTimeMixin):
    """Class holding a TimeRange.
    """
    
    FILTER_KEYS = [
        'start_time',
        'duration'
    ]
    
    def __init__(self, jsonData = {}):
        self.jsonData = jsonData.copy() # must copy the json data!
        self.otio_item  = None

    def to_json_string(self):
        return self.jsonData

    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(TimeRange.FILTER_KEYS)
        
    def from_usd(self, usd_prim):
        super().from_usd(usd_prim)
        
        for child_prim in usd_prim.GetChildren():
            usd_name = child_prim.GetName()
            usd_type = child_prim.GetTypeName()
            if child_prim.IsA('OtioRationalTime'):
                self.jsonData[usd_name] = self._create_rational_time(child_prim)
            else:
                print(f'WARNING: (time_range.py) Unknown node {usd_type} for '
                      f'{usd_prim}')

        return self.jsonData
    
    def to_usd(self, stage, usd_path):
        self._set_rational_time(stage, usd_path, 'start_time')
        self._set_rational_time(stage, usd_path, 'duration')
        
        usd_prim = self._create_usd(stage, usd_path, 'OtioTimeRange')
        
        
        return usd_prim
