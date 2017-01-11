//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "usdKatana/usdInArgs.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/boundable.h"

#include <FnAttribute/FnDataBuilder.h>

PxrUsdKatanaUsdInArgs::PxrUsdKatanaUsdInArgs(
        UsdStageRefPtr stage,
        const std::string& rootLocation,
        const std::string& isolatePath,
        FnAttribute::GroupAttribute sessionAttr,
        const std::string& ignoreLayerRegex,
        double currentTime,
        double shutterOpen,
        double shutterClose,
        const std::vector<double>& motionSampleTimes,
        const std::set<std::string>& defaultMotionPaths,
        const StringListMap& extraAttributesOrNamespaces,
        bool verbose,
        const char * errorMessage) :
    _stage(stage),
    _rootLocation(rootLocation),
    _isolatePath(isolatePath),
    _sessionAttr(sessionAttr),
    _ignoreLayerRegex(ignoreLayerRegex),
    _currentTime(currentTime),
    _shutterOpen(shutterOpen),
    _shutterClose(shutterClose),
    _motionSampleTimes(motionSampleTimes),
    _defaultMotionPaths(defaultMotionPaths),
    _extraAttributesOrNamespaces(extraAttributesOrNamespaces),
    _verbose(verbose)
{
    _isMotionBackward = _motionSampleTimes.size() > 1 &&
        _motionSampleTimes.front() > _motionSampleTimes.back();

    if (errorMessage)
    {
        _errorMessage = errorMessage;
    }
}

PxrUsdKatanaUsdInArgs::~PxrUsdKatanaUsdInArgs() 
{
}

std::vector<GfBBox3d>
PxrUsdKatanaUsdInArgs::ComputeBounds(
        const UsdPrim& prim)
{
    std::vector<GfBBox3d> ret;

    std::vector<UsdGeomBBoxCache>& bboxCaches = _bboxCaches.local();

    // Initialize the bounding box cache if it hasn't yet been initialized.
    //
    bool needsInit = bboxCaches.size() != _motionSampleTimes.size();
    if (needsInit)
    {
        // XXX: selected purposes should be driven by the UI. 
        // See usdGeom/imageable.h GetPurposeAttr() for allowed values. 
        TfTokenVector includedPurposes;  
        includedPurposes.push_back(UsdGeomTokens->default_); 
        includedPurposes.push_back(UsdGeomTokens->render); 
    
        bboxCaches.resize(_motionSampleTimes.size(),  
            UsdGeomBBoxCache(
                _currentTime, includedPurposes, /* useExtentsHint */ true)); 
        
        for (size_t index = 0; index < _motionSampleTimes.size(); ++index)
        {
            double relSampleTime = _motionSampleTimes[index];
            double time = _currentTime + relSampleTime;
            bboxCaches[index].SetTime(time);
        }
    }

    FnKat::DoubleBuilder boundBuilder(6);

    // There must be one bboxCache per motion sample, for efficiency purposes.
    if (!TF_VERIFY(bboxCaches.size() == _motionSampleTimes.size()))
    {
        return ret;
    }

    for (size_t i = 0; i < _motionSampleTimes.size(); i++)
    {
        ret.push_back(bboxCaches[i].ComputeUntransformedBound(prim));
    }

    return ret;
}

UsdPrim 
PxrUsdKatanaUsdInArgs::GetRootPrim() const
{
    if (_isolatePath.empty()) {
        return _stage->GetPseudoRoot();
    }
    else {
        return _stage->GetPrimAtPath(SdfPath(_isolatePath));
    }
}
