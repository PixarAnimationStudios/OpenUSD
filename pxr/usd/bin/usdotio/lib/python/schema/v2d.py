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

class V2d(Base):
    """Class used to parse an Imath::V2d

    """

    
    FILTER_KEYS = [
        'x',
        'y',
    ]
    
    def __init__(self, json_data = None):
        if json_data:
            self.jsonData = json_data.copy()
        else:
            self.jsonData = {
                'x' : 0,
                'y' : 0
            }
    
    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(RationalTime.FILTER_KEYS)
        
    def from_usd(self, usd_prim):
        super().from_usd(usd_prim)
        return self.jsonData
    
    def to_usd(self, stage, usd_path):
        usd_prim = stage.DefinePrim(usd_path, 'OtioV2d')
        self.set_usd_attributes(usd_prim)
        return usd_prim
    
    def set_usd_attributes(self, usd_prim):   
        attr = usd_prim.GetAttribute('x')
        attr.Set(self.jsonData['x'])
        
        attr = usd_prim.GetAttribute('y')
        attr.Set(self.jsonData['y'])
