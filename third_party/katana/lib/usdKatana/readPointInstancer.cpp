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
#include "usdKatana/attrMap.h"
#include "usdKatana/readPointInstancer.h"
#include "usdKatana/readXformable.h"
#include "usdKatana/usdInPrivateData.h"
#include "usdKatana/utils.h"

#include "pxr/usd/usdGeom/pointInstancer.h"

#include <FnGeolibServices/FnBuiltInOpArgsUtil.h>
#include <FnLogging/FnLogging.h>

#include <boost/unordered_set.hpp>

#include <pystring/pystring.h>

PXR_NAMESPACE_OPEN_SCOPE


FnLogSetup("PxrUsdKatanaReadPointInstancer");

namespace
{

    // Log an error and set attrs to create a Katana error location.
    //
    void
    _LogAndSetError(
        PxrUsdKatanaAttrMap& attrs,
        const std::string message)
    {
        FnLogError(message);
        attrs.set("type", FnKat::StringAttribute("error"));
        attrs.set("errorMessage", FnKat::StringAttribute(message));
    }

    // Convert the given relationship's targets into a vector of strings.
    //
    std::vector<std::string>
    _ConvertRelationshipTargets(UsdRelationship rel)
    {
        SdfPathVector targets;
        rel.GetForwardedTargets(&targets);
        std::vector<std::string> paths;
        paths.reserve(targets.size());
        TF_FOR_ALL(it, targets) {
            paths.push_back(it->GetString());
        }
        return paths;
    }

    // XXX _FillBoundFromExtent is based on UsdGeomBBoxCache::ComputeLocalBound
    //     (specifically UsdGeomBBoxCache::_GetBBoxFromExtentsHint and
    //     UsdGeomBBoxCache::_GetCombinedBBoxForIncludedPurposes).
    // Ideally we would just use UsdGeomBBoxCache, however it will compute the 
    // bound if it can't use the extentsHint which is an expensive operation we
    // want to avoid here.
    //
    const TfTokenVector& _purposeTokens =
            UsdGeomImageable::GetOrderedPurposeTokens();

    const TfTokenVector _includedPurposes { UsdGeomTokens->default_,
                                            UsdGeomTokens->render };

    typedef std::map<TfToken, GfRange3d, TfTokenFastArbitraryLessThan>
            _PurposeToRangeMap;

    bool
    _FillBoundFromExtent(
            const UsdAttribute& extentsAttribute,
            std::vector<GfRange3d>& output,
            double timeValue)
    {
        VtVec3fArray extents;
        if (!extentsAttribute or 
            !extentsAttribute.Get(&extents, timeValue))
        {
            return false;
        }

        _PurposeToRangeMap ranges;
        for (size_t i = 0; i < _purposeTokens.size(); ++i)
        {
            size_t idx = i*2;

            // If extents are not available for the value of purpose, it implies
            // that the rest of the bounds are empty. Hence, we can break.
            //
            if ((idx + 2) > extents.size())
            {
                break;
            }

            ranges[_purposeTokens[i]] = GfRange3d(extents[idx], extents[idx+1]);
        }

        GfRange3d combinedRange;
        TF_FOR_ALL(purposeIt, _includedPurposes)
        {
            _PurposeToRangeMap::const_iterator it = ranges.find(*purposeIt);
            if (it != ranges.end())
            {
                const GfRange3d& rangeForPurpose = it->second;
                if (!rangeForPurpose.IsEmpty())
                {
                    combinedRange = GfRange3d::GetUnion(
                            combinedRange, rangeForPurpose);
                }
            }
        }

        const GfVec3d& min = combinedRange.GetMin();
        const GfVec3d& max = combinedRange.GetMax();
        output.push_back(GfRange3d(min, max));

        return true;
    }

    // Compute bounds for the given instancer's prototypes using their extents
    // at the current time.
    //
    void
    _ComputePrototypeBoundsUsingExtents(
        const UsdGeomPointInstancer& instancer,
        const PxrUsdKatanaUsdInPrivateData& data,
        VtIntArray& outputIndices,
        std::vector<GfRange3d>& outputBounds)
    {
        const double currentTime = data.GetUsdInArgs()->GetCurrentTime();

        std::vector<std::string> prototypePaths =
                _ConvertRelationshipTargets(instancer.GetPrototypesRel());

        VtIntArray protoIndices;
        instancer.GetProtoIndicesAttr().Get(&protoIndices, currentTime);

        // Gather prototype extents.
        //
        TfToken extentsHintToken("extentsHint");

        std::vector<UsdAttribute> prototypeExtents;
        prototypeExtents.resize(prototypePaths.size());

        for (size_t i = 0; i < prototypePaths.size(); ++i)
        {
            const UsdPrim& prototypePrim =
                    data.GetUsdInArgs()->GetStage()->GetPrimAtPath(
                            SdfPath(prototypePaths[i]));
            if (prototypePrim)
            {
                prototypeExtents[i] = 
                        prototypePrim.GetAttribute(extentsHintToken);
            }
        }

        // Fill as many bounds as we can using the extents gathered above.
        //
        bool allFound = true;
        bool noneFound = false;

        std::vector<int> prototypeToBoundIndices;
        prototypeToBoundIndices.reserve(prototypeExtents.size());

        outputBounds.reserve(prototypeExtents.size());

        for (size_t i = 0; i < prototypeExtents.size(); ++i)
        {
            UsdAttribute& extentsAttribute = prototypeExtents[i];
            if (_FillBoundFromExtent(extentsAttribute, outputBounds,
                    currentTime))
            {
                prototypeToBoundIndices.push_back(i);
                noneFound = false;
            }
            else
            {
                prototypeToBoundIndices.push_back(-1);
                allFound = false;
            }
        }

        if (noneFound)
        {
            return;
        }

        // If bounds can be computed for all prototypes, the bound indices will
        // be the same as the prototype indices. If only some bounds could be
        // computed, the bound indices will be a subset of the prototype
        // indices.
        //
        if (allFound)
        {
            outputIndices = protoIndices;
        }
        else if (!noneFound)
        {
            outputIndices.reserve(protoIndices.size());

            for (size_t i = 0; i < protoIndices.size(); ++i)
            {
                int index = protoIndices[i];
                if (index < 0 || index >= prototypePaths.size())
                {
                    outputIndices.push_back(-1);
                }
                else
                {
                    outputIndices.push_back(prototypeToBoundIndices[index]);
                }
            }
        }
    }

    // Use previously computed instance matrix sample stored in outputMatrices
    // to create an additional sample with the translation components displaced
    // by the given velocities. Return true if successful.
    //
    bool
    _GenerateIntegratedMatricesUsingVelocity(
            const VtVec3fArray &velocities,
            const float velocityScale,
            const float fps,
            std::vector<GfMatrix4d>& outputMatrices,
            std::vector<float>& outputSampleTimes)
    {
        // NOTE We assume that velocities have already been validated.

        const size_t numInstances = velocities.size();

        // outputMatrics should contain a single matrix for each instance.
        //
        if (outputMatrices.size() != numInstances)
        {
            return false;
        }

        outputMatrices.reserve(numInstances * 2);

        const float frameDt = 1.0 / fps;

        // Loop over each instance, offsetting its previously computed transform
        // matrix by velocity.
        //
        for (size_t i = 0; i < numInstances; ++i)
        {
            GfMatrix4d mat = outputMatrices.at(i);
            mat *= GfMatrix4d(1).SetTranslate(GfVec3f(
                velocities[i][0] * velocityScale * frameDt,
                velocities[i][1] * velocityScale * frameDt,
                velocities[i][2] * velocityScale * frameDt));
            outputMatrices.push_back(mat);
        }

        outputSampleTimes.push_back(1.0);

        return true;
    }

    // Generate instance transform matrices for each motion sample. Return true
    // if successful.
    //
    bool
    _GenerateInstanceTransformMatrices(
            const UsdGeomPointInstancer& instancer,
            const PxrUsdKatanaUsdInPrivateData& data,
            bool useVelocity,
            const float velocityScale,
            const float fps,
            std::vector<GfMatrix4d>& outputMatrices,
            std::vector<float>& outputSampleTimes)
    {
        // NOTE We assume that the instancer's positions and velocities have
        // already been validated.

        const double currentTime = data.GetUsdInArgs()->GetCurrentTime();
        const std::vector<double> motionSampleTimes = data.GetMotionSampleTimes();

        VtIntArray protoIndices;
        instancer.GetProtoIndicesAttr().Get(&protoIndices, currentTime);
        const size_t numInstances = protoIndices.size();

        UsdAttribute positionsAttr = instancer.GetPositionsAttr();
        const std::vector<double> positionTimes =
                data.GetMotionSampleTimes(positionsAttr);

        VtVec3fArray velocities;
        instancer.GetVelocitiesAttr().Get(&velocities, currentTime);

        bool velocitiesValid = (useVelocity and !velocities.empty());

        // Determine number and value of motion samples.
        //
        size_t numXformSamples = 0;
        std::vector<double> xformSamples;

        if (motionSampleTimes.size() > 1 and positionTimes.size() > 1)
        {
            xformSamples = positionTimes;

            // If using velocities, we create one sample. An additional sample
            // will be created with the translation components offset by
            // velocity.
            //
            if (velocitiesValid)
            {
                numXformSamples = 1;
            }
            else
            {
                numXformSamples = positionTimes.size();
            }
        }
        else
        {
            xformSamples.push_back(currentTime);
            numXformSamples = 1;
        }

        outputMatrices.reserve(numInstances * numXformSamples);
        outputSampleTimes.reserve(numXformSamples);

        // Compute for each motion sample, across all instances.
        //
        for (size_t i = 0; i < numXformSamples; ++i)
        {
            double sampleTime = xformSamples[i];
            VtArray<GfMatrix4d> xforms;

            bool success = instancer.ComputeInstanceTransformsAtTime(
                    &xforms,
                    /*time*/ sampleTime,
                    /*baseTime*/ sampleTime,
                    /*doProtoXforms*/ UsdGeomPointInstancer::ExcludeProtoXform,
                    /*applyMask*/ UsdGeomPointInstancer::IgnoreMask);
            if (success and xforms.size() == numInstances)
            {
                for (size_t j = 0; j < xforms.size(); ++j)
                {
                    outputMatrices.push_back(xforms[j]);
                }
                outputSampleTimes.push_back(sampleTime);
            }
            else
            {
                return false;
            }
        }

        if (velocitiesValid)
        {
            return _GenerateIntegratedMatricesUsingVelocity(
                velocities, velocityScale, fps, outputMatrices,
                outputSampleTimes);
        }

        return true;
    }

    // Generate a name for an instance source using its full path.
    //
    std::string
    _GenerateInstanceSourceNameFromPath(
            const std::string& srcPath,
            int combineCount)
    {
        std::vector<std::string> tokens = TfStringTokenize(srcPath, "/");
        int size = tokens.size();
        std::string result;
        if(combineCount > 0 && combineCount < size) {
            std::vector<std::string> nameParts;
            for(int i = size - 1 - combineCount; i < size; ++i) {
                nameParts.push_back(tokens[i]);
            }
            result = TfStringJoin(nameParts, "_");
        }
        else {
            result = tokens[size - 1];
        }
        return result;
    }

    // XXX Copy of PxrUsdKatanaReadModel::_GetViewerProxyAttr.
    //
    FnKat::GroupAttribute
    _GetViewerProxyAttr(const PxrUsdKatanaUsdInPrivateData& data)
    {
        FnKat::GroupBuilder proxiesBuilder;

        proxiesBuilder.set("viewer.load.opType",
            FnKat::StringAttribute("StaticSceneCreate"));

        proxiesBuilder.set("viewer.load.opArgs.a.type",
            FnKat::StringAttribute("usd"));

        proxiesBuilder.set("viewer.load.opArgs.a.currentTime", 
            FnKat::DoubleAttribute(data.GetUsdInArgs()->GetCurrentTime()));

        proxiesBuilder.set("viewer.load.opArgs.a.fileName", 
            FnKat::StringAttribute(data.GetUsdInArgs()->GetFileName()));

        proxiesBuilder.set("viewer.load.opArgs.a.forcePopulateUsdStage", 
            FnKat::FloatAttribute(1));

        // XXX: Once everyone has switched to the op, change referencePath
        // to isolatePath here and in the USD VMP (2/25/2016).
        proxiesBuilder.set("viewer.load.opArgs.a.referencePath", 
            FnKat::StringAttribute(data.GetUsdPrim().GetPath().GetString()));

        proxiesBuilder.set("viewer.load.opArgs.a.rootLocation", 
            FnKat::StringAttribute(data.GetUsdInArgs()->GetRootLocationPath()));

        proxiesBuilder.set("viewer.load.opArgs.a.session",
                data.GetUsdInArgs()->GetSessionAttr());

        proxiesBuilder.set("viewer.load.opArgs.a.ignoreLayerRegex",
           FnKat::StringAttribute(data.GetUsdInArgs()->GetIgnoreLayerRegex()));

        return proxiesBuilder.build();
    }

} // anon namespace

void
PxrUsdKatanaReadPointInstancer(
        const UsdGeomPointInstancer& instancer,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& attrs,
        PxrUsdKatanaAttrMap& sourcesAttrMap,
        PxrUsdKatanaAttrMap& instancesAttrMap,
        PxrUsdKatanaAttrMap& instancerOpArgsAttrMap)
{
    const double currentTime = data.GetUsdInArgs()->GetCurrentTime();

    PxrUsdKatanaReadXformable(instancer, data, attrs);

    attrs.set("type", FnKat::StringAttribute("usd point instancer"));

    const std::string fileName = data.GetUsdInArgs()->GetFileName();
    attrs.set("info.usd.fileName", FnKat::StringAttribute(fileName));

    FnKat::GroupAttribute instancerOpArgs = instancerOpArgsAttrMap.build();

    const std::string katOutputPath = FnKat::StringAttribute(
            instancerOpArgs.getChildByName("outputLocationPath")).getValue("", false);
    if (katOutputPath.empty())
    {
        _LogAndSetError(attrs, "ERROR: No output location path specified");
        return;
    }

    //
    // Validate instance data.
    //
    // XXX Multi-sampled data is only validated for the current time.

    // Prototypes (required)
    //
    std::vector<std::string> prototypePaths =
            _ConvertRelationshipTargets(instancer.GetPrototypesRel());
    if (prototypePaths.empty())
    {
        _LogAndSetError(attrs, "ERROR: Instancer has no prototypes");
        return;
    }

    // Indices (required)
    //
    VtIntArray protoIndices;
    if (!instancer.GetProtoIndicesAttr().Get(&protoIndices, currentTime))
    {
        _LogAndSetError(attrs, "ERROR: Instancer has no prototype indices");
        return;
    }
    const size_t numInstances = protoIndices.size();

    // Ids (optional)
    //
    VtInt64Array ids;
    if (instancer.GetIdsAttr().Get(&ids, currentTime))
    {
        if (!ids.empty() and ids.size() != numInstances)
        {
            _LogAndSetError(attrs,
                    "ERROR: Mismatch in length of indices and ids");
            return;
        }
    }

    // Mask (optional)
    //
    std::vector<bool> pruneMaskValues =
            instancer.ComputeMaskAtTime(currentTime);
    if (!pruneMaskValues.empty() and pruneMaskValues.size() != numInstances)
    {
        _LogAndSetError(attrs,
                "ERROR: Mismatch in length of indices and mask");
        return;
    }

    // Positions (required)
    //
    VtVec3fArray positions;
    if (instancer.GetPositionsAttr().Get(&positions, currentTime))
    {
        if (positions.size() != numInstances)
        {
            _LogAndSetError(attrs,
                    "ERROR: Mismatch in length of indices and positions");
            return;
        }
    }
    else
    {
        _LogAndSetError(attrs, "ERROR: Instancer has no positions");
        return;
    }

    // Velocities (optional)
    //
    VtVec3fArray velocities;
    if (instancer.GetVelocitiesAttr().Get(&velocities, currentTime))
    {
        if (!velocities.empty() and velocities.size() != numInstances)
        {
            _LogAndSetError(attrs,
                    "ERROR: Mismatch in length of indices and velocities");
            return;
        }
    }

    //
    // Compute prototype bounds.
    //

    VtIntArray prototypeBoundIndices;
    std::vector<GfRange3d> prototypeBounds;
    _ComputePrototypeBoundsUsingExtents(
            instancer, data, prototypeBoundIndices, prototypeBounds);

    //
    // Compute instance transform matrices.
    //

    bool useVelocity = (bool)FnKat::FloatAttribute(
            instancerOpArgs.getChildByName("useVelocity")).getValue(0, false);

    float velocityScale = FnKat::FloatAttribute(
            instancerOpArgs.getChildByName("velocityScale")).getValue(-1, false);

    float fps = std::max(0.001f, FnKat::FloatAttribute(
            instancerOpArgs.getChildByName("fps")).getValue(24, false));

    std::vector<GfMatrix4d> xformSamples;
    std::vector<float> xformSampleTimes;

    if (!_GenerateInstanceTransformMatrices(
            instancer, data, useVelocity, velocityScale, fps,
            xformSamples, xformSampleTimes))
    {
        _LogAndSetError(attrs,
                "ERROR: Could not compute instance transform matrices");
        return;
    }

    //
    // Build sources. Keep track of which instances use them and aggregate their
    // bounds.
    //

    FnGeolibServices::StaticSceneCreateOpArgsBuilder sourcesBldr(false);

    int instancePathNameCombine = (int)FnKat::FloatAttribute(
            instancerOpArgs.getChildByName("instancePathNameCombine")
            ).getValue(1, false);

    // If sourceParentScope is a valid scope in the source's USD path
    // the source will be imported from under this scope. This is useful
    // when a source has Look bindings which point to Looks above the source
    // location. With this option the source can be imported from a path
    // which contains both the Looks scope and the source.
    //
    std::string sourceParentScope = FnKat::StringAttribute(
            instancerOpArgs.getChildByName("sourceParentScope")
            ).getValue("", false);
    sourceParentScope = pystring::lstrip(
                        pystring::rstrip(sourceParentScope, "/"), "/");
    bool useSourceParentScope = !sourceParentScope.empty();

    typedef std::map<std::string, std::pair<std::string, std::string> >
            SourceRescopeMap;
    SourceRescopeMap sourceRescopeMap;

    // Init aggregate bounds.
    //
    bool aggregateBoundsValid = false;
    std::vector<double> aggregateBounds;
    aggregateBounds.reserve(6);
    aggregateBounds[0] = std::numeric_limits<double>::max();
    aggregateBounds[2] = std::numeric_limits<double>::max();
    aggregateBounds[4] = std::numeric_limits<double>::max();
    aggregateBounds[1] = - std::numeric_limits<double>::max();
    aggregateBounds[3] = - std::numeric_limits<double>::max();
    aggregateBounds[5] = - std::numeric_limits<double>::max();

    std::vector<int> instanceIndices;
    instanceIndices.reserve(numInstances);

    std::vector<std::string> instanceSources;
    instanceSources.reserve(prototypePaths.size());

    std::map<std::string, int> instanceSourceIndexMap;

    std::vector<int> omitList;
    omitList.reserve(numInstances);

    boost::unordered_set<size_t> usedIds;
    boost::unordered_set<std::string> builtPrototypes;

    for (size_t i = 0, e = numInstances; i != e; ++i)
    {
        int index = protoIndices[i];

        if (index < 0 || index >= prototypePaths.size())
        {
            _LogAndSetError(attrs, TfStringPrintf(
                    "ERROR: prototype index %i out of range", index));
            return;
        }

        size_t id = i;
        if (ids.size() > i)
        {
            id = ids[i];
            if (usedIds.find(id) != usedIds.end())
            {
                _LogAndSetError(attrs, TfStringPrintf(
                        "ERROR: duplicate instance id '%lu", id));
                return;
            }
            usedIds.insert(id);
        }

        // Check to see if we are pruned.
        //
        bool isPruned = (!pruneMaskValues.empty() and
                         pruneMaskValues[i] == false);
        if (isPruned)
        {
            omitList.push_back(i);
        }

        std::string prototypePath = prototypePaths[index];

        // If the sourceParentScope option is set, the instance source
        // will be imported from the sourceParentScope path instead of the
        // prototype path. This way we can import Looks scopes above the
        // source and preserve material bindings. prototypePath becomes a
        // child path of sourceParentScope. prototypeChildPath will contain
        // the remaining path to the instance source.
        //
        std::string prototypeChildPath;
        if (useSourceParentScope)
        {
            auto rescopeIt = sourceRescopeMap.find(prototypePath);
            if (rescopeIt == sourceRescopeMap.end())
            {
                int parentScopeStrIdx = pystring::rfind(
                        prototypePath, sourceParentScope);
                if (parentScopeStrIdx > 0)
                {
                    int protoScopeStrIdx = pystring::find(
                            prototypePath, "/",
                            parentScopeStrIdx + sourceParentScope.size() + 1);
                    if (protoScopeStrIdx > 0)
                    {
                        std::pair<std::string, std::string> rescopePaths(
                                prototypePath.substr(0, protoScopeStrIdx),
                                prototypePath.substr(protoScopeStrIdx));
                        sourceRescopeMap.insert(std::make_pair(
                                    prototypePath, rescopePaths));
                        prototypePath = rescopePaths.first;
                        prototypeChildPath = rescopePaths.second;
                    }
                }
            }
            else
            {
                prototypePath = rescopeIt->second.first;
                prototypeChildPath = rescopeIt->second.second;
            }
        }

        bool sourceBuilt = (builtPrototypes.find(prototypePath) !=
                            builtPrototypes.end());

        // Determine both the relative and full path to this source.
        //
        // NOTE We exlude 'sources' from the relative path because the sources
        // builder will be used with interface.createChild("sources", ...)
        // rather than with interface.addSubOpAtLocation(...).
        //
        const std::string relativeSourcePath =
                _GenerateInstanceSourceNameFromPath(
                        prototypePath, instancePathNameCombine);
        std::ostringstream pathBuffer;
        pathBuffer << katOutputPath << "/sources/" << relativeSourcePath;
        std::string fullSourcePath = pathBuffer.str();

        if (!sourceBuilt)
        {
            sourcesBldr.setAttrAtLocation(relativeSourcePath,
                    "type", FnKat::StringAttribute("instance source"));
            sourcesBldr.setAttrAtLocation(relativeSourcePath,
                    "usdPrimPath", FnKat::StringAttribute(prototypePath));
            sourcesBldr.setAttrAtLocation(relativeSourcePath,
                    "usdPrimName", FnKat::StringAttribute("geo"));
        }

        // If the sourceParentScope option is set, modify the full path to be a
        // child of prototypePath.
        //
        if (useSourceParentScope and !prototypeChildPath.empty())
        {
            std::ostringstream pathBufferMod;
            pathBufferMod << fullSourcePath << "/geo" << prototypeChildPath;
            fullSourcePath = pathBufferMod.str();

            // The instance transform will be wrong if there's a non-identity
            // transform at the source's location. This deferred AttributeSet
            // will remove the transform attr.
            //
            FnGeolibServices::AttributeSetOpArgsBuilder attrSetBldr;
            attrSetBldr.setLocationPaths(
                    FnKat::StringAttribute(fullSourcePath));
            attrSetBldr.deleteAttr("xform");
            FnKat::GroupBuilder deferredOpArgs;
            deferredOpArgs.set("opType",
                    FnKat::StringAttribute("AttributeSet"));
            deferredOpArgs.set("resolveIds",
                    FnKat::StringAttribute("prefinalize"));
            deferredOpArgs.set("opArgs", attrSetBldr.build());
            sourcesBldr.setAttrAtLocation(relativeSourcePath,
                    "recursiveOps.RemoveInstanceSourceXform",
                    deferredOpArgs.build());
        }

        if (!prototypeBoundIndices.empty())
        {
            int boundIndex = prototypeBoundIndices[i];

            if (boundIndex >= 0 && boundIndex < prototypeBounds.size())
            {
                // Get this instance's bounds in parent-local coords
                //
                const GfRange3d range = prototypeBounds[boundIndex];

                // Transform bounds into parent-local coords (at all time
                // samples).
                //
                for (int timeSample = 0; timeSample < xformSampleTimes.size();
                        ++timeSample)
                {
                    GfMatrix4d matrix =
                            xformSamples[(timeSample * numInstances) + i];

                    // N.B. We don't apply the parent xform here as the
                    // aggregate bounds need to be in parent-local space.
                    //
                    GfBBox3d parentSpaceBBox(range, matrix);
                    GfRange3d parentRange = parentSpaceBBox.ComputeAlignedRange();
                    GfVec3d minRange(parentRange.GetMin());
                    GfVec3d maxRange(parentRange.GetMax());

                    // Update the aggregate bounds with the transformed bounds.
                    //
                    if (minRange[0] < aggregateBounds[0])
                    {
                        aggregateBounds[0] = minRange[0];
                    }
                    if (maxRange[0] > aggregateBounds[1])
                    {
                        aggregateBounds[1] = maxRange[0];
                    }

                    if (minRange[1] < aggregateBounds[2])
                    {
                        aggregateBounds[2] = minRange[1];
                    }
                    if (maxRange[1] > aggregateBounds[3])
                    {
                        aggregateBounds[3] = maxRange[1];
                    }

                    if (minRange[2] < aggregateBounds[4])
                    {
                        aggregateBounds[4] = minRange[2];
                    }
                    if (maxRange[2] > aggregateBounds[5])
                    {
                        aggregateBounds[5] = maxRange[2];
                    }
                }

                aggregateBoundsValid = true;
            }
        }

        if (!sourceBuilt)
        {
            instanceSourceIndexMap[fullSourcePath] = instanceSources.size();
            instanceSources.push_back(fullSourcePath);
            builtPrototypes.insert(prototypePath);
        }
        instanceIndices.push_back(instanceSourceIndexMap[fullSourcePath]);
    }

    //
    // Build instances.
    //

    FnGeolibServices::StaticSceneCreateOpArgsBuilder instancesBldr(false);

    std::vector<double *> xformValueSamples;
    xformValueSamples.reserve(xformSampleTimes.size());

    for (size_t i = 0; i < xformSampleTimes.size(); ++i)
    {
        xformValueSamples.push_back(xformSamples[numInstances * i][0]);
    }

    FnKat::DoubleAttribute instanceMatrixAttr(
            &xformSampleTimes[0], xformSampleTimes.size(),
                    const_cast<const double **>(&xformValueSamples[0]),
                            numInstances * 16, 16);

    instancesBldr.createEmptyLocation("instances", "instance array");

    instancesBldr.setAttrAtLocation("instances",
            "geometry.instanceSource",
                    FnKat::StringAttribute(instanceSources, 1));

    instancesBldr.setAttrAtLocation("instances",
            "geometry.instanceIndex",
                    FnKat::IntAttribute(&instanceIndices[0],
                            instanceIndices.size(), 1));

    instancesBldr.setAttrAtLocation("instances",
            "geometry.instanceMatrix", instanceMatrixAttr);

    if (!velocities.empty())
    {
        const VtValue& wrappedVels = VtValue(velocities);
        FnKat::FloatAttribute velocitiesAttr =
                PxrUsdKatanaUtils::ConvertVtValueToKatAttr(wrappedVels, true);
        instancesBldr.setAttrAtLocation("instances",
                "geometry.instanceVelocity", velocitiesAttr);
    }

    if (!omitList.empty())
    {
        instancesBldr.setAttrAtLocation("instances",
                "geometry.omitList",
                        FnKat::IntAttribute(&omitList[0], omitList.size(), 1));
    }

    // Transfer primvars.
    //
    const std::vector<UsdGeomPrimvar> primvars =
            instancer.GetAuthoredPrimvars();
    for (size_t i = 0; i < primvars.size(); ++i)
    {
        UsdGeomPrimvar primvar = primvars[i];

        TfToken name;
        SdfValueTypeName typeName;
        TfToken interpolation;
        int elementSize;

        primvar.GetDeclarationInfo(
                &name, &typeName, &interpolation, &elementSize);

        std::string katAttrName =
                "geometry.arbitrary." + name.GetString();

        instancesBldr.setAttrAtLocation("instances",
                katAttrName + ".scope", FnKat::StringAttribute(
                        interpolation.GetString()));
        instancesBldr.setAttrAtLocation("instances",
                katAttrName + ".inputType", FnKat::StringAttribute(
                        typeName.GetScalarType().GetAsToken().GetString()));
        if (elementSize != 1)
        {
            instancesBldr.setAttrAtLocation("instances",
                    katAttrName + ".elementSize",
                            FnKat::IntAttribute(elementSize));
        }

        // XXX Indexed primvars get flattened out.
        //
        VtValue primvarValue;
        primvar.ComputeFlattened(&primvarValue, currentTime);
        instancesBldr.setAttrAtLocation("instances",
                katAttrName + ".value",
                    PxrUsdKatanaUtils::ConvertVtValueToKatAttr(
                        primvarValue, true));
    }

    // Set attrs for pruning.
    //
    const bool spruneEnabled =
            FnAttribute::IntAttribute(
                instancerOpArgs.getChildByName("spruneEnabled")
                        ).getValue(0, false) != 0;
    if (spruneEnabled)
    {
        instancesBldr.setAttrAtLocation("instances",
                "geometry.pointInstancerId",
                        FnKat::StringAttribute(katOutputPath));
    }

    //
    // Set the final aggregate bounds.
    //

    if (aggregateBoundsValid)
    {
        attrs.set("bound", FnKat::DoubleAttribute(&aggregateBounds[0], 6, 2));
    }

    //
    // Set proxy attrs.
    //

    attrs.set("proxies", _GetViewerProxyAttr(data));

    //
    // Transfer builder results to our attr maps.
    //

    FnKat::GroupAttribute sourcesAttrs = sourcesBldr.build();
    for (int64_t i = 0; i < sourcesAttrs.getNumberOfChildren(); ++i)
    {
        sourcesAttrMap.set(
                sourcesAttrs.getChildName(i),
                sourcesAttrs.getChildByIndex(i));
    }

    FnKat::GroupAttribute instancesAttrs = instancesBldr.build();
    for (int64_t i = 0; i < instancesAttrs.getNumberOfChildren(); ++i)
    {
        instancesAttrMap.set(
                instancesAttrs.getChildName(i),
                instancesAttrs.getChildByIndex(i));
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
