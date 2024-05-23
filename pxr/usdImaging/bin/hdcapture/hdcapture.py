#!/pxrpythonsubst
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

import argparse

from pxr import Ar
from pxr import Usd
from pxr import UsdAppUtils

parser = argparse.ArgumentParser(
    prog='HdUsdWriterRendererDriver',
    description="""This tool will record information it receives from Hydra and dump it to a .usda file.
    This allows inspection of what Hydra "sees" after data is fed through SceneDelegate/RenderIndex or DataSource/SceneIndex.""")

parser.add_argument('-b', '--baseline', required=False, help='The .usda file to diff the output with.')
parser.add_argument('-i', '--input', required=True, help='The input USD file to render to text.')
parser.add_argument('-o', '--output', required=True, help='The filename to write the serialized USD to. Output format with the default delegate is USDA.')

args = parser.parse_args()

resolver = Ar.GetResolver()
resolver.CreateDefaultContextForAsset(args.input)
stage = Usd.Stage.Open(args.input)

driver = UsdAppUtils.UsdWriterDriver('HdUsdWriterRendererPlugin')
serialization_result = driver.Render(stage, 0, args.output)
if not serialization_result:
    print('Something went wrong invoking SerializeToUsd')
else:
    print(f"Wrote output to '{args.output}'")

def compare(baseline_file, target_file):
    result = UsdAppUtils.fuzzytextdiff.diff_files(baseline_file, target_file)
    if result != 0:
        print(f'Target \'{target_file}\' did not match the baseline \'{baseline_file}\'.')
    else:
        print('Target and baseline within tolerance.')
    return result

if (args.baseline):
    compare(args.baseline, args.output)
