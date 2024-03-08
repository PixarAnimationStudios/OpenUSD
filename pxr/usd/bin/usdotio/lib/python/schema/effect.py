# Copyright 2024 Gonzalo Garramuño
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

class Effect(NamedBase):
    def __init__(self, otio_item = None):
        super().__init__(otio_item)
        if not otio_item:
            self.jsonData = \
                json.loads(otio.schema.Effect().to_json_string())
            
    def to_usd(self, stage, usd_path):
        usd_prim = self._create_usd(stage, usd_path, 'OtioEffect')
        return usd_prim
