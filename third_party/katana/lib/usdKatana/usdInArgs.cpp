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
#include "pxr/pxr.h"
#include "usdKatana/usdInArgs.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/boundable.h"

#include <FnAttribute/FnDataBuilder.h>

PXR_NAMESPACE_OPEN_SCOPE


PxrUsdKatanaUsdInArgs::PxrUsdKatanaUsdInArgs(
        UsdStageRefPtr stage,
        const std::string& rootLocation,
        const std::string& isolatePath,
        const std::string& sessionLocation,
        FnAttribute::GroupAttribute sessionAttr,
        const std::string& ignoreLayerRegex,
        double currentTime,
        double shutterOpen,
        double shutterClose,
        const std::vector<double>& motionSampleTimes,
        const StringListMap& extraAttributesOrNamespaces,
        bool prePopulate,
        bool verbose,
        const char * errorMessage) :
    _stage(stage),
    _rootLocation(rootLocation),
    _isolatePath(isolatePath),
    _sessionLocation(sessionLocation),
    _sessionAttr(sessionAttr),
    _ignoreLayerRegex(ignoreLayerRegex),
    _currentTime(currentTime),
    _shutterOpen(shutterOpen),
    _shutterClose(shutterClose),
    _motionSampleTimes(motionSampleTimes),
    _extraAttributesOrNamespaces(extraAttributesOrNamespaces),
    _prePopulate(prePopulate),
    _verbose(verbose)
{
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
        const UsdPrim& prim,
        const std::vector<double>& motionSampleTimes)
{
    std::vector<GfBBox3d> ret;

    std::map<double, UsdGeomBBoxCache>& bboxCaches = _bboxCaches.local();

    TfTokenVector includedPurposes;

    for (size_t i = 0; i < motionSampleTimes.size(); i++)
    {
        double relSampleTime = motionSampleTimes[i];

        std::map<double, UsdGeomBBoxCache>::iterator it =
            bboxCaches.find(relSampleTime);
        if (it == bboxCaches.end())
        {
            if (includedPurposes.size() == 0)
            {
                // XXX: selected purposes should be driven by the UI.
                // See usdGeom/imageable.h GetPurposeAttr() for allowed values.
                includedPurposes.push_back(UsdGeomTokens->default_);
                includedPurposes.push_back(UsdGeomTokens->render);
            }

            // Initialize the bounding box cache for this time sample if it
            // hasn't yet been initialized.
            UsdGeomBBoxCache bboxCache(_currentTime + relSampleTime,
                                       includedPurposes,
                                       /* useExtentsHint */ true);
            bboxCaches.insert(
                std::pair<double, UsdGeomBBoxCache>(relSampleTime, bboxCache));
            ret.push_back(bboxCache.ComputeUntransformedBound(prim));
        }
        else
        {
            ret.push_back(it->second.ComputeUntransformedBound(prim));
        }
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

PXR_NAMESPACE_CLOSE_SCOPE

