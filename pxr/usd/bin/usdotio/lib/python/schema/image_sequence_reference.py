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

from .media_reference import MediaReference

class ImageSequenceReference(MediaReference):
    """Class that handles an image sequence media reference.
    """

    FILTER_KEYS = [
        # This is an enum, but it cannot be defined as such in the schema.usda
        'missing_frame_policy',
    ]
    
    def __init__(self, otio_item = None):
        super().__init__(otio_item)
        if not otio_item:
            self.jsonData = \
                json.loads(otio.schema.ImageSequenceReference().to_json_string())
    
    def to_usd(self, stage, usd_path):
        super().to_usd(stage, usd_path)
        usd_prim = self._create_usd(stage, usd_path,
                                   'OtioImageSequenceReference')
        return usd_prim

    def filter_attributes(self):
        super().filter_attributes()
        self._remove_keys(ImageSequenceReference.FILTER_KEYS)

    def set_usd_attributes(self, usd_prim):
        
        # USD does not accept variant sets in schemas
        policy = self.jsonData['missing_frame_policy']
        policy = policy.removeprefix('MissingFramePolicy.')
        self.set_usd_attribute(usd_prim, 'missing_frame_policy', policy) 
        
        super().set_usd_attributes(usd_prim)
