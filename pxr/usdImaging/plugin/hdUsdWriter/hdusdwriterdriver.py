#
# Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

from pxr import Ar
from pxr import Usd
from pxr import UsdImagingGL

from pxr.HdUsdWriter import fuzzytextdiff

class HdUsdWriterRendererDriver:

    def __init__(self) -> None:
        self._renderParams = UsdImagingGL.RenderParams()

    def use_usd_imaging_engine(self):
        self.set_render_engine(UsdImagingGL.Engine)

    def set_render_engine(self, engine_class):
        self._renderer = engine_class()

    def use_usda_writer_renderer_plugin(self):
        self.set_renderer_plugin('HdUsdWriterRendererPlugin')

    def set_renderer_plugin(self, plugin):
        self._renderer.SetRendererPlugin(plugin)

    def load_stage(self, usd_file):
        resolver = Ar.GetResolver()
        resolver.CreateDefaultContextForAsset(usd_file)        

        self._stage = Usd.Stage.Open(usd_file)

    def render(self, output_file):
        pseudoRoot = self._stage.GetPseudoRoot()
        self._renderer.Render(pseudoRoot, self._renderParams)
        serialization_result = self._renderer.InvokeRendererCommand('SerializeToUsd', {'outputPath':output_file})

        if not serialization_result:
            print('Something went wrong invoking SerializeToUsd')

    def compare(self, baseline_file, target_file):
        result = fuzzytextdiff.diff_files(baseline_file, target_file)
        if result != 0:
            print(f'Target \'{target_file}\' did not match the baseline \'{baseline_file}\'.')
        else:
            print('Target and baseline within tolerance.')
        return result

    def render_and_compare(self, baseline_file, output_file):
        self.render(output_file)
        return self.compare(baseline_file, output_file)

    def set_time(self, frame):
        self._renderParams.frame = frame

    def stop(self):
        self._renderer.StopRenderer()
        pass