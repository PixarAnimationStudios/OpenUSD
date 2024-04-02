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
    
from .box2d_mixin import Box2dMixin
from .external_reference import ExternalReference
from .image_sequence_reference import ImageSequenceReference
from .item import Item
from .missing_reference import MissingReference
from .options import Options, LogLevel

class Clip(Item, Box2dMixin):

    FILTER_KEYS = [
        'media_references',
        'available_image_bounds',
    ]
    
    def __init__(self, otio_item = None):
        super().__init__(otio_item)
        self.media_references = {}
        if not otio_item:
            self.jsonData = json.loads(otio.schema.Clip().to_json_string())

    def append_media_reference(self, ref_prim, key = 'DEFAULT_MEDIA'):
        self.media_references[key] = ref_prim
    
    def from_usd(self, usd_prim):
        super().from_usd(usd_prim)
        
        #
        # Traverse the stage to get the jsonData of each node
        #
        for x in usd_prim.GetChildren():
            usd_name = x.GetName()
            if x.IsA('OtioExternalReference'):
                ref_prim = ExternalReference()
                ref_prim.from_usd(x)
                self.append_media_reference(ref_prim)
            elif x.IsA('OtioImageSequenceReference'):
                ref_prim = ImageSequenceReference()
                ref_prim.from_usd(x)
                self.append_media_reference(ref_prim)
            elif x.IsA('OtioMissingReference'):
                ref_prim = MissingReference()
                ref_prim.from_usd(x)
                self.append_media_reference(ref_prim)
            elif x.IsA('OtioBox2d'):
                ref_prim = Box2d()
                self.imaging_bounds = ref_prim.from_usd(x)
            else:
                pass

        newdict = {}
        for key, val in self.media_references.items():
            newval = json.loads(val.to_json_string())
            newdict[key] = newval
    
        self.jsonData['media_references'] = newdict
        
        return self.jsonData

    
    def to_usd(self, stage, usd_path): 
        super().to_usd(stage, usd_path)
        
        self._set_box2d(stage, usd_path, 'available_image_bounds')
            
        usd_prim = self._create_usd(stage, usd_path, 'OtioClip')
        
        m = self.otio_item.media_reference
        if m:
            media_prim = None
            media_path = usd_path + '/media_reference'
            if isinstance(m, otio.schema.MissingReference):
                media_path = usd_path + '/missing_reference'
                media_prim = MissingReference(m)
                media_prim.to_usd(stage, media_path)
            elif isinstance(m, otio.schema.ExternalReference):
                media_path = usd_path + '/external_reference'
                media_prim = ExternalReference(m)
                media_prim.to_usd(stage, media_path)
            elif isinstance(m, otio.schema.ImageSequenceReference):
                media_path = usd_path + '/image_sequence_reference'
                media_prim = ImageSequenceReference(m)
                media_prim.to_usd(stage, media_path)
            else:
                print(f'WARNING: {m} is invalid!')

        return usd_prim


    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(Clip.FILTER_KEYS)
