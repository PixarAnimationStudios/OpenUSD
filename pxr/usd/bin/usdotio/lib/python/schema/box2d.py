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
from .v2d import V2d

class Box2d(Base):
    """Class used to define an Imath::Box2d.
    """

    
    FILTER_KEYS = [
        'min',
        'max',
    ]
    
    def __init__(self, jsonData = {}):
        self.jsonData = jsonData.copy() # must copy the json data!

    def to_json_string(self):
        return self.jsonData

    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(Box2d.FILTER_KEYS)
        
    def from_usd(self, usd_prim):
        for x in usd_prim.GetChildren():
            if x.IsA('OtioV2d'):
                time = RationalTime()
                usd_name = x.GetName()
                self.jsonData[usd_name] = time.from_usd(x)
            else:
                print(f'WARNING: (box2d.py) Unknown node {usd_type} for '
                      f'{usd_prim}')

        return self.jsonData
    
    def to_usd(self, stage, usd_path):
        usd_prim = stage.DefinePrim(usd_path, 'OtioBox2d')
        
        vmin = V2d(self.jsonData['min'])
        vmin_path = usd_path + '/min'
        vmin_prim = vmin.to_usd(stage, vmin_path)
        
        vmax = V2d(self.jsonData['max'])
        vmax_path = usd_path + '/max'
        vmax_prim = vmax.to_usd(stage, vmax_path)
        
        return usd_prim
