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
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/base/gf/interval.h"
#include "pxr/usd/usdGeom/xform.h"

PxrUsdKatanaUsdInPrivateData::PxrUsdKatanaUsdInPrivateData(
        const UsdPrim& prim,
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs,
        const PxrUsdKatanaUsdInPrivateData* parentData)
    : _prim(prim), _usdInArgs(usdInArgs)
{
    // XXX: manually track instance and master path for possible
    //      relationship re-retargeting. This approach does not yet
    //      support nested instances -- which is expected to be handled
    //      via the forthcoming GetMasterWithContext.
    //
    if (prim.IsInstance())
    {
        if (prim.IsInMaster() and parentData)
        {
            SdfPath descendentPrimPath = 
                prim.GetPath().ReplacePrefix(
                    prim.GetPath().GetPrefixes()[0], SdfPath::ReflexiveRelativePath());

            _instancePath = parentData->GetInstancePath().AppendPath(descendentPrimPath);
        }
        else
        {
            _instancePath = prim.GetPath();
        }

        const UsdPrim& masterPrim = prim.GetMaster();            
        if (masterPrim)
        {
            _masterPath = masterPrim.GetPath();
        }
    }
    else if (parentData)
    {
        // Pass along instance and master paths to children.
        //
        if (not parentData->GetInstancePath().IsEmpty())
        {
            _instancePath = parentData->GetInstancePath();
        }

        if (not parentData->GetMasterPath().IsEmpty())
        {
            _masterPath = parentData->GetMasterPath();
        }
    }
}

const std::vector<double>
PxrUsdKatanaUsdInPrivateData::GetMotionSampleTimes(const UsdAttribute& attr) const
{
    double currentTime = _usdInArgs->GetCurrentTime();

    if (attr and not PxrUsdKatanaUtils::IsAttributeVarying(attr, currentTime))
    {
        static std::vector<double> noMotion = {0.0};
        return noMotion;
    }

    const std::vector<double> motionSampleTimes = _usdInArgs->GetMotionSampleTimes();

    // early exit if we don't have a valid attribute
    // or aren't asking for multiple samples
    if (not attr or motionSampleTimes.size() < 2)
    {
        return motionSampleTimes;
    }

    // Allowable error in sample time comparison.
    static const double epsilon = 0.0001;

    double shutterOpen = _usdInArgs->GetShutterOpen();
    double shutterClose = _usdInArgs->GetShutterClose();

    double shutterStartTime, shutterCloseTime;

    // Calculate shutter start and close times based on
    // the direction of motion blur.
    if (_usdInArgs->IsMotionBackward())
    {
        shutterStartTime = currentTime - shutterClose;
        shutterCloseTime = currentTime - shutterOpen;
    }
    else
    {
        shutterStartTime = currentTime + shutterOpen;
        shutterCloseTime = currentTime + shutterClose;
    }

    // get the time samples for our frame interval
    std::vector<double> result;
    if (not attr.GetTimeSamplesInInterval(
            GfInterval(shutterStartTime, shutterCloseTime), &result))
    {
        return motionSampleTimes;
    }

    bool foundSamplesInInterval = not result.empty();

    double firstSample, lastSample;

    if (foundSamplesInInterval)
    {
        firstSample = result.front();
        lastSample = result.back();
    }
    else
    {
        firstSample = shutterStartTime;
        lastSample = shutterCloseTime;
    }

    // If no samples were found or the first sample is later than the 
    // shutter start time then attempt to get the previous sample in time.
    if (not foundSamplesInInterval or (firstSample-shutterStartTime) > epsilon)
    {
        double lower, upper;
        bool hasTimeSamples;

        if (attr.GetBracketingTimeSamples(
                shutterStartTime, &lower, &upper, &hasTimeSamples))
        {
            // Insert the first sample as long as it is different
            // than what we already have.
            if (fabs(lower-firstSample) > epsilon)
            {
                result.insert(result.begin(), lower);
            }
        }
    }

    // If no samples were found or the last sample is earlier than the
    // shutter close time then attempt to get the next sample in time.
    if (not foundSamplesInInterval or (shutterCloseTime-lastSample) > epsilon)
    {
        double lower, upper;
        bool hasTimeSamples;

        if (attr.GetBracketingTimeSamples(
                shutterCloseTime, &lower, &upper, &hasTimeSamples))
        {
            // Append the last sample as long as it is different
            // than what we already have.
            if (fabs(upper-lastSample) > epsilon)
            {
                result.push_back(upper);
            }
        }
    }

    // convert from absolute to frame-relative time samples
    for (std::vector<double>::iterator I = result.begin();
            I != result.end(); ++I)
    {
        (*I) -= currentTime;
    }

    return result;
}
