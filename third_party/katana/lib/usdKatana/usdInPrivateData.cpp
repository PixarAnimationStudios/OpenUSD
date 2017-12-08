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
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/base/gf/interval.h"
#include "pxr/usd/usdGeom/xform.h"

#include <pystring/pystring.h>
#include <FnGeolib/util/Path.h>

PXR_NAMESPACE_OPEN_SCOPE


PxrUsdKatanaUsdInPrivateData::PxrUsdKatanaUsdInPrivateData(
        const UsdPrim& prim,
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs,
        const PxrUsdKatanaUsdInPrivateData* parentData)
    : _prim(prim), _usdInArgs(usdInArgs), _extGb(0)
{
    // None of the below is safe or relevant if the prim is not valid
    // This is most commonly due to an invalid isolatePath -- which is 
    // already reported as a katana error from pxrUsdIn.cpp
    if (!prim)
    {
        return;
    }
    
    
    // XXX: manually track instance and master path for possible
    //      relationship re-retargeting. This approach does not yet
    //      support nested instances -- which is expected to be handled
    //      via the forthcoming GetMasterWithContext.
    //
    if (prim.IsInstance())
    {
        if (prim.IsInMaster() && parentData)
        {
            SdfPath descendentPrimPath = 
                prim.GetPath().ReplacePrefix(
                    prim.GetPath().GetPrefixes()[0], 
                    SdfPath::ReflexiveRelativePath());

            _instancePath = parentData->GetInstancePath().AppendPath(
                descendentPrimPath);
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
        if (!parentData->GetInstancePath().IsEmpty())
        {
            _instancePath = parentData->GetInstancePath();
        }

        if (!parentData->GetMasterPath().IsEmpty())
        {
            _masterPath = parentData->GetMasterPath();
        }
    }

    //
    // Apply session overrides for motion.
    //

    const std::string primPath = prim.GetPrimPath().GetString();
    const std::string isolatePath = usdInArgs->GetIsolatePath();
    const std::string sessionPath = usdInArgs->GetSessionLocationPath();
    FnKat::GroupAttribute sessionAttr = usdInArgs->GetSessionAttr();

    // XXX: If an isolatePath has been specified, it means the PxrUsdIn is
    // probably loading USD contents below the USD root. This can prevent
    // overrides from trickling down the hierarchy, e.g. the overrides for /A/B
    // won't get applied to children if the isolatePath is /A/B/C/D.
    //
    // So, if the usdInArgs suggest that an isolatePath has been specified and
    // we don't have any parentData, we'll need to check if there are overrides
    // for the prim and any of its parents.
    //
    std::vector<std::string> pathsToCheck;
    if (!parentData and !isolatePath.empty() and
        pystring::startswith(primPath, isolatePath+"/"))
    {
        std::vector<std::string> parentLocs;
        Foundry::Katana::Util::Path::GetLocationStack(parentLocs, primPath);
        std::reverse(std::begin(parentLocs), std::end(parentLocs));
        for (size_t i = 0; i < parentLocs.size(); ++i)
        {
            pathsToCheck.push_back(FnKat::DelimiterEncode(
                    sessionPath + parentLocs[i]));
        }
    }
    else
    {
        pathsToCheck.push_back(FnKat::DelimiterEncode(sessionPath + primPath));
    }

    //
    // If a session override is specified, use its value. If no override exists,
    // try asking the parent data for its value. Otherwise, fall back on the
    // usdInArgs value.
    //

    bool overrideFound;

    // Current time.
    //
    overrideFound = false;
    for (size_t i = 0; i < pathsToCheck.size(); ++i)
    {
        FnKat::FloatAttribute currentTimeAttr =
            sessionAttr.getChildByName(
                "overrides."+pathsToCheck[i]+".currentTime");
        if (currentTimeAttr.isValid())
        {
            _currentTime = currentTimeAttr.getValue();
            overrideFound = true;
            break;
        }
    }
    if (!overrideFound)
    {
        if (parentData)
        {
            _currentTime = parentData->GetCurrentTime();
        }
        else
        {
            _currentTime = usdInArgs->GetCurrentTime();
        }
    }

    // Shutter open.
    //
    overrideFound = false;
    for (size_t i = 0; i < pathsToCheck.size(); ++i)
    {
        FnKat::FloatAttribute shutterOpenAttr =
                sessionAttr.getChildByName(
                    "overrides."+pathsToCheck[i]+".shutterOpen");
        if (shutterOpenAttr.isValid())
        {
            _shutterOpen = shutterOpenAttr.getValue();
            overrideFound = true;
            break;
        }
    }
    if (!overrideFound)
    {
        if (parentData)
        {
            _shutterOpen = parentData->GetShutterOpen();
        }
        else
        {
            _shutterOpen = usdInArgs->GetShutterOpen();
        }
    }

    // Shutter close.
    //
    overrideFound = false;
    for (size_t i = 0; i < pathsToCheck.size(); ++i)
    {
        FnKat::FloatAttribute shutterCloseAttr =
                sessionAttr.getChildByName(
                    "overrides."+pathsToCheck[i]+".shutterClose");
        if (shutterCloseAttr.isValid())
        {
            _shutterClose = shutterCloseAttr.getValue();
            overrideFound = true;
            break;
        }
    }
    if (!overrideFound)
    {
        if (parentData)
        {
            _shutterClose = parentData->GetShutterClose();
        }
        else
        {
            _shutterClose = usdInArgs->GetShutterClose();
        }
    }

    // Motion sample times.
    //
    // Fallback logic is a little more complicated for motion sample times, as
    // they can vary per attribute, so store both the overridden and the
    // fallback motion sample times for use inside GetMotionSampleTimes.
    //
    for (size_t i = 0; i < pathsToCheck.size(); ++i)
    {
        FnKat::Attribute motionSampleTimesAttr =
                sessionAttr.getChildByName(
                    "overrides."+pathsToCheck[i]+".motionSampleTimes");
        if (motionSampleTimesAttr.isValid())
        {
            // Interpret an IntAttribute as "use usdInArgs defaults"
            //
            if (motionSampleTimesAttr.getType() == kFnKatAttributeTypeInt)
            {
                _motionSampleTimesOverride = usdInArgs->GetMotionSampleTimes();
                break;
            }
            // Interpret a FloatAttribute as an explicit value override
            //
            if (motionSampleTimesAttr.getType() == kFnKatAttributeTypeFloat)
            {
                FnAttribute::FloatAttribute attr(motionSampleTimesAttr);
                const auto& sampleTimes = attr.getNearestSample(0.0f);;
                if (!sampleTimes.empty())
                {
                    for (float sampleTime : sampleTimes)
                        _motionSampleTimesOverride.push_back(
                            (double)sampleTime);
                    break;
                }
            }
        }
    }
    if (parentData)
    {
        _motionSampleTimesFallback = parentData->GetMotionSampleTimes();
    }
    else
    {
        _motionSampleTimesFallback = usdInArgs->GetMotionSampleTimes();
    }
}

const bool
PxrUsdKatanaUsdInPrivateData::IsMotionBackward() const
{
    if (_motionSampleTimesOverride.size() > 0)
    {
        return (_motionSampleTimesOverride.size() > 1 &&
            _motionSampleTimesOverride.front() >
            _motionSampleTimesOverride.back());
    }
    else
    {
        return (_motionSampleTimesFallback.size() > 1 &&
            _motionSampleTimesFallback.front() >
            _motionSampleTimesFallback.back());
    }
}

const std::vector<double>
PxrUsdKatanaUsdInPrivateData::GetMotionSampleTimes(
    const UsdAttribute& attr) const
{
    static std::vector<double> noMotion = {0.0};

    if ((attr && !PxrUsdKatanaUtils::IsAttributeVarying(attr, _currentTime)) ||
            _motionSampleTimesFallback.size() < 2)
    {
        return noMotion;
    }

    // If an override was explicitly specified for this prim, return it.
    //
    if (_motionSampleTimesOverride.size() > 0)
    {
        return _motionSampleTimesOverride;
    }

    //
    // Otherwise, try computing motion sample times. If they can't be computed,
    // fall back on the parent data's times.
    //

    // Early exit if we don't have a valid attribute.
    //
    if (!attr)
    {
        return _motionSampleTimesFallback;
    }

    // Allowable error in sample time comparison.
    static const double epsilon = 0.0001;

    double shutterStartTime, shutterCloseTime;

    // Calculate shutter start and close times based on
    // the direction of motion blur.
    if (IsMotionBackward())
    {
        shutterStartTime = _currentTime - _shutterClose;
        shutterCloseTime = _currentTime - _shutterOpen;
    }
    else
    {
        shutterStartTime = _currentTime + _shutterOpen;
        shutterCloseTime = _currentTime + _shutterClose;
    }

    // get the time samples for our frame interval
    std::vector<double> result;
    if (!attr.GetTimeSamplesInInterval(
            GfInterval(shutterStartTime, shutterCloseTime), &result))
    {
        return _motionSampleTimesFallback;
    }

    bool foundSamplesInInterval = !result.empty();

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
    if (!foundSamplesInInterval || (firstSample-shutterStartTime) > epsilon)
    {
        double lower, upper;
        bool hasTimeSamples;

        if (attr.GetBracketingTimeSamples(
                shutterStartTime, &lower, &upper, &hasTimeSamples))
        {
            if (lower > shutterStartTime)
            {
                // Did not find a sample ealier than the shutter start. 
                // Return no motion.
                return noMotion;
            }

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
    if (!foundSamplesInInterval || (shutterCloseTime-lastSample) > epsilon)
    {
        double lower, upper;
        bool hasTimeSamples;

        if (attr.GetBracketingTimeSamples(
                shutterCloseTime, &lower, &upper, &hasTimeSamples))
        {
            if (upper < shutterCloseTime)
            {
                // Did not find a sample later than the shutter close. 
                // Return no motion.
                return noMotion;
            }

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
        (*I) -= _currentTime;
    }

    return result;
}

void PxrUsdKatanaUsdInPrivateData::setExtensionOpArg(
        const std::string & name, FnAttribute::Attribute attr) const
{
    if (!_extGb)
    {
        _extGb = new FnAttribute::GroupBuilder;
    }

    _extGb->set("ext." + name, attr);
}


FnAttribute::Attribute PxrUsdKatanaUsdInPrivateData::getExtensionOpArg(
        const std::string & name, FnAttribute::GroupAttribute opArgs) const
{
    if (name.empty())
    {
        return opArgs.getChildByName("ext");
    }
    
    return opArgs.getChildByName("ext." + name);
}


FnAttribute::GroupAttribute
PxrUsdKatanaUsdInPrivateData::updateExtensionOpArgs(
        FnAttribute::GroupAttribute opArgs) const
{
    if (!_extGb)
    {
        return opArgs;
    }
    
    return FnAttribute::GroupBuilder()
        .update(opArgs)
        .deepUpdate(_extGb->build())
        .build();
}

PxrUsdKatanaUsdInPrivateData *
PxrUsdKatanaUsdInPrivateData::GetPrivateData(
        const FnKat::GeolibCookInterface& interface)
{
    return static_cast<PxrUsdKatanaUsdInPrivateData*>(
            interface.getPrivateData());
}

PXR_NAMESPACE_CLOSE_SCOPE

