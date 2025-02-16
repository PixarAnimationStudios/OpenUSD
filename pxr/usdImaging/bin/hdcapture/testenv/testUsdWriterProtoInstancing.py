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

from pxr import Ar, Usd, UsdAppUtils

if __name__ == '__main__':
    resolver = Ar.GetResolver()
    resolver.CreateDefaultContextForAsset('testUsdWriterProtoInstancing.usda')
    stage = Usd.Stage.Open('testUsdWriterProtoInstancing.usda')

    driver = UsdAppUtils.UsdWriterDriver('HdUsdWriterRendererPlugin')
    serialization_result = driver.render(stage, 0, 'testUsdWriterProtoInstancingOut.usda')

    def compare(baseline_file, target_file):
        result = UsdAppUtils.fuzzytextdiff.diff_files(baseline_file, target_file)
        if result != 0:
            print(f'Target \'{target_file}\' did not match the baseline \'{baseline_file}\'.')
        else:
            print('Target and baseline within tolerance.')
        return result
    
    result = compare('testUsdWriterProtoInstancingBaseline.usda', 'testUsdWriterProtoInstancingOut.usda')
    assert result == 0
