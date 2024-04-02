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

import json

import opentimelineio as otio

from .named_base import NamedBase
from .rational_time_mixin import RationalTimeMixin

class Transition(NamedBase, RationalTimeMixin):
    """A class defining a Transition.  It defaults to a SMPTE_Dissolve.
    """

    FILTER_KEYS = [
        'in_offset',
        'out_offset',
    ]

    def __init__(self, otio_item = None):
        super().__init__(otio_item)
        if not otio_item:
            self.jsonData = \
                json.loads(otio.schema.Transition().to_json_string())
        
    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(Transition.FILTER_KEYS)
        
    def from_usd(self, usd_prim):
        super().from_usd(usd_prim)
        
        for child in usd_prim.GetChildren():
            usd_type = child.GetTypeName()
            if child.IsA('OtioRationalTime'):
                usd_name = child.GetName()
                self.jsonData[usd_name] = self._create_rational_time(child)
            else:
                if Options.log_level >= LogLevel.NORMAL:
                    print('WARNING: (transition.py) Unknown child element ', \
                          child, 'attached to', child, '.')
        
        return self.jsonData
    
    def to_usd(self, stage, usd_path):
        self._set_rational_time(stage, usd_path, 'in_offset')
        self._set_rational_time(stage, usd_path, 'out_offset')
        
        usd_prim = self._create_usd(stage, usd_path, 'OtioTransition')
        return usd_prim
