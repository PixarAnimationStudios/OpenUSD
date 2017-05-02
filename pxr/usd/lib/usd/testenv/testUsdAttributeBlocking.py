#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Sdf, Usd 

startTime = 101
endTime   = 120

# Generate a stage with a default attribute as well as an 
# attribute with time samples authored on it. These are the 
# two cases in which we can author blocks
def CreateTestAssets(fileName):
    stage = Usd.Stage.CreateNew(fileName)
    prim = stage.DefinePrim("/Sphere")
    defAttr = prim.CreateAttribute("size", Sdf.ValueTypeNames.Double, True)
    defAttr.Set(1.0)
    sampleAttr = prim.CreateAttribute("points", Sdf.ValueTypeNames.Double, False)
    for sample in range(startTime, endTime):
        sampleAttr.Set(sample, sample)

    return stage, defAttr, sampleAttr

# Test blocking of default value through the time-sample-wiping API
# Ensures that all samples are blown away upon calling Block()
def TestBlock(sampleAttr, defAttr):
    assert sampleAttr.GetNumTimeSamples() != 0
    assert not sampleAttr.GetResolveInfo().ValueIsBlocked()
    sampleAttr.Block()
    assert sampleAttr.GetNumTimeSamples() == 0
    assert sampleAttr.GetResolveInfo().ValueIsBlocked()
    assert not sampleAttr.Get()
    for sample in range(startTime, endTime):
        assert not sampleAttr.Get(sample)

# Test blocking of individual time samples
# Ensures users are able to pass the sentinel value through 
# for fine grained control over attribute blocking.
def TestIndividualTimeSampleBlocking(sampleAttr, defAttr):
    for sample in range(startTime, endTime):
        sampleAttr.Set(Sdf.ValueBlock(), sample)
        assert sampleAttr.Get(sample) is None
        # Time sample blocking is different than whole-value blocking
        assert not sampleAttr.GetResolveInfo().ValueIsBlocked()

# Ensure that passing the empty time code through works as expected
def TestDefaultValueBlocking(sampleAttr, defAttr):
    assert defAttr.Get()
    assert not defAttr.GetResolveInfo().ValueIsBlocked()
    defAttr.Set(Sdf.ValueBlock())
    assert not defAttr.Get()
    assert defAttr.GetResolveInfo().ValueIsBlocked()

if __name__ == '__main__':
    # We ensure that this is supported on all file formats
    formats = [".usda", ".usdc"]

    for fmt in formats:
        stage, defAttr, sampleAttr = CreateTestAssets('test' + fmt)
        TestBlock(sampleAttr, defAttr)
        del stage, defAttr, sampleAttr

        stage, defAttr, sampleAttr = CreateTestAssets('test' + fmt)
        TestIndividualTimeSampleBlocking(sampleAttr, defAttr)
        del stage, defAttr, sampleAttr

        stage, defAttr, sampleAttr = CreateTestAssets('test' + fmt)
        TestDefaultValueBlocking(sampleAttr, defAttr)
        del stage, defAttr, sampleAttr
