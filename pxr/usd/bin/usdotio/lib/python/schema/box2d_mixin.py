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


from .options import Options, LogLevel
from .box2d import Box2d

class Box2dMixin:
    def _set_box2d(self, stage, usd_path, name):
        box2d_prim = None
        json_data = self.jsonData.get(name)
        if json_data:
            box2d_path = usd_path + f'/{name}'
            box2d_prim = Box2d(json_data)
            box2d_prim.to_usd(stage, box2d_path)
            if Options.log_level == LogLevel.DEBUG:
                print(f'\t\tCreated time box2d at {box2d_path}')

        return box2d_prim
    
    def _create_box2d(self, usd_prim):
        box2d = Box2d()
        return box2d.from_usd(usd_prim)
