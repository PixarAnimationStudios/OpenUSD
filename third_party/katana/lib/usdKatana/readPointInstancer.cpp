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
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/kind/registry.h"

#include "pxr/base/gf/transform.h"
#include "pxr/base/gf/matrix4d.h"

#include <FnAPI/FnAPI.h>
#include <FnGeolibServices/FnBuiltInOpArgsUtil.h>
#include <FnGeolib/util/Path.h>
#include <FnLogging/FnLogging.h>

#include <boost/unordered_set.hpp>

#include <pystring/pystring.h>

#if KATANA_VERSION_MAJOR >= 3
#include "vtKatana/array.h"
#endif // KATANA_VERSION_MAJOR >= 3

PXR_NAMESPACE_OPEN_SCOPE


FnLogSetup("PxrUsdKatanaReadPointInstancer");

namespace
{
    typedef std::map<SdfPath, UsdPrim> _PathToPrimMap;
    typedef std::map<SdfPath, GfRange3d> _PathToRangeMap;
    typedef std::map<TfToken, GfRange3d, TfTokenFastArbitraryLessThan>
        _PurposeToRangeMap;

    // Log an error and set attrs to show an error message in the Scene Graph.
    //
    void
    _LogAndSetError(
        PxrUsdKatanaAttrMap& attrs,
        const std::string& message)
    {
        FnLogError(message);
        attrs.set("errorMessage",
                  FnKat::StringAttribute(
                      "[ERROR PxrUsdKatanaReadPointInstancer]: " + message));
    }

    // Log a warning and set attrs to show a warning message in the Scene Graph.
    void
    _LogAndSetWarning(
        PxrUsdKatanaAttrMap &attrs,
        const std::string& message)
    {
        FnLogWarn(message);
        attrs.set("warningMessage",
                  FnKat::StringAttribute(
                      "[WARNING PxrUsdKatanaReadPointInstancer]: " + message));
    }

    // XXX This is based on UsdGeomPointInstancer::ComputeExtentAtTime. Ideally,
    // we would just use UsdGeomPointInstancer, however it does not account for
    // multi-sampled transforms (see bug 147526).
    //
    bool
    _ComputeExtentAtTime(
        VtVec3fArray& extent,
        PxrUsdKatanaUsdInArgsRefPtr usdInArgs,
        const std::vector<VtArray<GfMatrix4d>>& xforms,
        const std::vector<double>& motionSampleTimes,
        const VtIntArray& protoIndices,
        const SdfPathVector& protoPaths,
        const _PathToPrimMap& primCache,
        const std::vector<bool>& mask)
    {
        GfRange3d extentRange;

        const size_t numSampleTimes = motionSampleTimes.size();

        for (size_t i = 0; i < protoIndices.size(); ++i) {
            if (!mask.empty() && !mask[i]) {
                continue;
            }

            const int protoIndex = protoIndices[i];
            const SdfPath &protoPath = protoPaths[protoIndex];

            _PathToPrimMap::const_iterator pcIt = primCache.find(protoPath);
            const UsdPrim &protoPrim = pcIt->second;
            if (!protoPrim) {
                continue;
            }

            // Leverage usdInArgs for calculating the proto prim's bound. Note
            // that we apply the prototype's local transform to account for any
            // offsets.
            //
            std::vector<GfBBox3d> sampledBounds = usdInArgs->ComputeBounds(
                protoPrim, motionSampleTimes, /* applyLocalTransform */ true);

            for (size_t a = 0; a < numSampleTimes; ++a) {
                // Apply the instance transform to the bounding box for this
                // time sample. We don't apply the parent transform here, as the
                // bounds need to be in parent-local space.
                //
                GfBBox3d thisBounds(sampledBounds[a]);
                thisBounds.Transform(xforms[a][i]);
                extentRange.UnionWith(thisBounds.ComputeAlignedRange());
            }
        }

        if (extentRange.IsEmpty()) {
            return false;
        }

        const GfVec3d extentMin = extentRange.GetMin();
        const GfVec3d extentMax = extentRange.GetMax();

        extent = VtVec3fArray(2);
        extent[0] = GfVec3f(extentMin[0], extentMin[1], extentMin[2]);
        extent[1] = GfVec3f(extentMax[0], extentMax[1], extentMax[2]);

        return true;
    }

} // anon namespace

void
PxrUsdKatanaReadPointInstancer(
        const UsdGeomPointInstancer& instancer,
        const PxrUsdKatanaUsdInPrivateData& data,
        PxrUsdKatanaAttrMap& instancerAttrMap,
        PxrUsdKatanaAttrMap& sourcesAttrMap,
        PxrUsdKatanaAttrMap& instancesAttrMap,
        PxrUsdKatanaAttrMap& inputAttrMap)
{
    const double currentTime = data.GetCurrentTime();

    PxrUsdKatanaReadXformable(instancer, data, instancerAttrMap);

    // Get primvars for setting later. Unfortunatley, the only way to get them
    // out of the attr map is to build it, which will cause its contents to be
    // cleared. We'll need to restore its contents before continuing.
    //
    FnKat::GroupAttribute instancerAttrs = instancerAttrMap.build();
    FnKat::GroupAttribute primvarAttrs =
            instancerAttrs.getChildByName("geometry.arbitrary");
    for (int64_t i = 0; i < instancerAttrs.getNumberOfChildren(); ++i)
    {
        instancerAttrMap.set(instancerAttrs.getChildName(i),
                instancerAttrs.getChildByIndex(i));
    }

    instancerAttrMap.set("type", FnKat::StringAttribute("usd point instancer"));

    const std::string fileName = data.GetUsdInArgs()->GetFileName();
    instancerAttrMap.set("info.usd.fileName", FnKat::StringAttribute(fileName));

    FnKat::GroupAttribute inputAttrs = inputAttrMap.build();

    const std::string katOutputPath = FnKat::StringAttribute(
            inputAttrs.getChildByName("outputLocationPath")).getValue("", false);
    if (katOutputPath.empty())
    {
        _LogAndSetError(instancerAttrMap, "No output location path specified");
        return;
    }

    //
    // Validate instancer data.
    //

    const auto instancerSdfPath = instancer.GetPath();
    const std::string instancerPath = instancerSdfPath.GetString();

    UsdStageWeakPtr stage = instancer.GetPrim().GetStage();

    // Prototypes (required)
    //
    SdfPathVector protoPaths;
    instancer.GetPrototypesRel().GetTargets(&protoPaths);
    if (protoPaths.empty())
    {
        _LogAndSetError(instancerAttrMap, "Instancer has no prototypes");
        return;
    }

    _PathToPrimMap primCache;
    for (auto protoPath : protoPaths) {
        const UsdPrim &protoPrim = stage->GetPrimAtPath(protoPath);
        primCache[protoPath] = protoPrim;
    }

    // Indices (required)
    //
    VtIntArray protoIndices;
    if (!instancer.GetProtoIndicesAttr().Get(&protoIndices, currentTime))
    {
        _LogAndSetWarning(instancerAttrMap,
                          "Instancer has no prototype indices");
        return;
    }
    const size_t numInstances = protoIndices.size();
    if (numInstances == 0)
    {
        _LogAndSetWarning(instancerAttrMap,
                          "Instancer has no prototype indices");
        return;
    }
    for (auto protoIndex : protoIndices)
    {
        if (protoIndex < 0 || static_cast<size_t>(protoIndex) >= protoPaths.size())
        {
            _LogAndSetError(instancerAttrMap, TfStringPrintf(
                    "Out of range prototype index %d", protoIndex));
            return;
        }
    }

    // Mask (optional)
    //
    std::vector<bool> pruneMaskValues =
            instancer.ComputeMaskAtTime(currentTime);
    if (!pruneMaskValues.empty() and pruneMaskValues.size() != numInstances)
    {
        _LogAndSetError(instancerAttrMap,
                "Mismatch in length of indices and mask");
        return;
    }

    // Positions (required)
    //
    UsdAttribute positionsAttr = instancer.GetPositionsAttr();
    if (!positionsAttr.HasValue())
    {
        _LogAndSetError(instancerAttrMap, "Instancer has no positions");
        return;
    }

    //
    // Compute instance transform matrices.
    //
    
    // Gather frame-relative sample times and add them to the current time to
    // generate absolute sample times.
    //
    const std::vector<double> &motionSampleTimes =
        data.GetMotionSampleTimes(positionsAttr);
    const size_t sampleCount = motionSampleTimes.size();
    std::vector<UsdTimeCode> sampleTimes(sampleCount);
    std::transform(motionSampleTimes.begin(), motionSampleTimes.end(), 
                   sampleTimes.begin(), [currentTime](double motionSampleTime){
                        return UsdTimeCode(currentTime + motionSampleTime);});

    std::vector<VtArray<GfMatrix4d>> xformSamples(sampleCount);

    instancer.ComputeInstanceTransformsAtTimes(
            &xformSamples,
            sampleTimes,
            UsdTimeCode(currentTime),
            UsdGeomPointInstancer::IncludeProtoXform,
            UsdGeomPointInstancer::IgnoreMask);
    const size_t numXformSamples = xformSamples.size();

    if (numXformSamples == 0) {
        _LogAndSetError(instancerAttrMap, "Could not compute "
                                          "sample/topology-invarying instance "
                                          "transform matrix");
        return;
    }

    //
    // Compute prototype bounds.
    //

    bool aggregateBoundsValid = false;
    std::vector<double> aggregateBounds;

    // XXX Replace with UsdGeomPointInstancer::ComputeExtentAtTime.
    //
    VtVec3fArray aggregateExtent;
    if (_ComputeExtentAtTime(
            aggregateExtent, data.GetUsdInArgs(), xformSamples,
            motionSampleTimes, protoIndices, protoPaths, primCache,
            pruneMaskValues)) {
        aggregateBoundsValid = true;
        aggregateBounds.resize(6);
        aggregateBounds[0] = aggregateExtent[0][0]; // min x
        aggregateBounds[1] = aggregateExtent[1][0]; // max x
        aggregateBounds[2] = aggregateExtent[0][1]; // min y
        aggregateBounds[3] = aggregateExtent[1][1]; // max y
        aggregateBounds[4] = aggregateExtent[0][2]; // min z
        aggregateBounds[5] = aggregateExtent[1][2]; // max z
    }

    //
    // Build sources. Keep track of which instances use them.
    //

    FnGeolibServices::StaticSceneCreateOpArgsBuilder sourcesBldr(false);

    std::vector<int> instanceIndices;
    instanceIndices.reserve(numInstances);

    std::vector<std::string> instanceSources;
    instanceSources.reserve(protoPaths.size());

    std::map<std::string, int> instanceSourceIndexMap;

    std::vector<int> omitList;
    omitList.reserve(numInstances);

    std::map<SdfPath, std::string> protoPathsToKatPaths;

    

    for (size_t i = 0; i < numInstances; ++i)
    {
        int index = protoIndices[i];

        // Check to see if we are pruned.
        //
        bool isPruned = (!pruneMaskValues.empty() and
                         pruneMaskValues[i] == false);
        if (isPruned)
        {
            omitList.push_back(i);
        }

        const SdfPath &protoPath = protoPaths[index];

        // Compute the full (Katana) path to this prototype.
        //
        std::string fullProtoPath;
        std::map<SdfPath, std::string>::const_iterator pptkpIt =
                protoPathsToKatPaths.find(protoPath);
        if (pptkpIt != protoPathsToKatPaths.end())
        {
            fullProtoPath = pptkpIt->second;
        }
        else
        {
            _PathToPrimMap::const_iterator pcIt = primCache.find(protoPath);
            const UsdPrim &protoPrim = pcIt->second;
            if (!protoPrim) {
                continue;
            }

            // Determine where (what path) to start building the prototype prim
            // such that its material bindings will be preserved. This could be
            // the prototype path itself or an ancestor path.
            //
            SdfPathVector commonPrefixes;

            // If the proto prim itself doesn't have any bindings or isn't a
            // (sub)component, we'll walk upwards until we find a prim that
            // does/is. Stop walking if we reach the instancer or the usdInArgs
            // root.
            //
            UsdPrim prim = protoPrim;
            while (prim and prim != instancer.GetPrim() and
                   prim != data.GetUsdInArgs()->GetRootPrim())
            {
                UsdRelationship materialBindingsRel =
                        UsdShadeMaterial::GetBindingRel(prim);
                SdfPathVector materialPaths;
                bool hasMaterialBindings = (materialBindingsRel and
                        materialBindingsRel.GetForwardedTargets(
                            &materialPaths) and !materialPaths.empty());

                TfToken kind;
                std::string assetName;
                auto assetAPI = UsdModelAPI(prim);
                // If the prim is a (sub)component, it should have materials
                // defined below it.
                bool hasMaterialChildren = (
                        assetAPI.GetAssetName(&assetName) and
                        assetAPI.GetKind(&kind) and (
                            KindRegistry::IsA(kind, KindTokens->component) or
                            KindRegistry::IsA(kind, KindTokens->subcomponent)));

                if (hasMaterialChildren)
                {
                    // The prim has material children, so start building at the
                    // prim's path.
                    //
                    commonPrefixes.push_back(prim.GetPath());
                    break;
                }

                if (hasMaterialBindings)
                {
                    for (auto materialPath : materialPaths)
                    {
                        const SdfPath &commonPrefix =
                                protoPath.GetCommonPrefix(materialPath);
                        if (commonPrefix.GetString() == "/" || instancerSdfPath.HasPrefix(commonPrefix))
                        {
                            // XXX Unhandled case.
                            // The prim and its material are not under the same
                            // parent; start building at the prim's path
                            // (although it is likely that bindings will be
                            // broken).
                            //
                            commonPrefixes.push_back(prim.GetPath());
                        }
                        else
                        {
                            // Start building at the common ancestor between the
                            // prim and its material.
                            //
                            commonPrefixes.push_back(commonPrefix);
                        }
                    }
                    break;
                }

                prim = prim.GetParent();
            }

            // Fail-safe in case no common prefixes were found.
            if (commonPrefixes.empty())
            {
                commonPrefixes.push_back(protoPath);
            }

            // XXX Unhandled case.
            // We'll use the first common ancestor even if there is more than
            // one (which shouldn't appen if the prototype prim and its bindings
            // are under the same parent).
            //
            SdfPath::RemoveDescendentPaths(&commonPrefixes);
            const std::string buildPath = commonPrefixes[0].GetString();

            // See if the path is a child of the point instancer. If so, we'll
            // match its hierarchy. If not, we'll put it under a 'prototypes'
            // group.
            //
            std::string relBuildPath;
            if (pystring::startswith(buildPath, instancerPath + "/"))
            {
                relBuildPath = pystring::replace(
                        buildPath, instancerPath + "/", "");
            }
            else
            {
                relBuildPath = "prototypes/" +
                        FnGeolibUtil::Path::GetLeafName(buildPath);
            }

            // Start generating the full path to the prototype.
            //
            fullProtoPath = katOutputPath + "/" + relBuildPath;

            // Make the common ancestor our instance source.
            //
            sourcesBldr.setAttrAtLocation(relBuildPath,
                    "type", FnKat::StringAttribute("instance source"));

            // Author a tracking attr.
            //
            sourcesBldr.setAttrAtLocation(relBuildPath,
                    "info.usd.sourceUsdPath",
                    FnKat::StringAttribute(buildPath));

            // Tell the BuildIntermediate op to start building at the common
            // ancestor.
            //
            sourcesBldr.setAttrAtLocation(relBuildPath,
                    "usdPrimPath", FnKat::StringAttribute(buildPath));
            sourcesBldr.setAttrAtLocation(relBuildPath,
                    "usdPrimName", FnKat::StringAttribute("geo"));

            // Build an AttributeSet op that will delete the prototype's
            // transform, since we've already folded it into the instance
            // transforms via IncludeProtoXform.
            //
            FnGeolibServices::AttributeSetOpArgsBuilder asb;
            asb.deleteAttr("xform");

            if (protoPath.GetString() != buildPath)
            {
                // Finish generating the full path to the prototype.
                //
                fullProtoPath = fullProtoPath + "/geo" + pystring::replace(
                        protoPath.GetString(), buildPath, "");

                asb.setLocationPaths(fullProtoPath);
                sourcesBldr.addSubOpAtLocation(
                        relBuildPath + "/geo" + pystring::replace(
                                protoPath.GetString(), buildPath, ""),
                        "AttributeSet", asb.build());
            }
            else
            {
                asb.setLocationPaths(fullProtoPath + "/geo");
                sourcesBldr.addSubOpAtLocation(
                        relBuildPath + "/geo",
                        "AttributeSet", asb.build());
            }

            // Create a mapping that will link the instance's index to its
            // prototype's full path.
            //
            instanceSourceIndexMap[fullProtoPath] = instanceSources.size();
            instanceSources.push_back(fullProtoPath);

            // Finally, store the full path in the map so we won't have to do
            // this work again.
            //
            protoPathsToKatPaths[protoPath] = fullProtoPath;
        }

        instanceIndices.push_back(instanceSourceIndexMap[fullProtoPath]);
    }

    //
    // Build instances.
    //

    FnGeolibServices::StaticSceneCreateOpArgsBuilder instancesBldr(false);

    instancesBldr.createEmptyLocation("instances", "instance array");

    instancesBldr.setAttrAtLocation("instances",
            "geometry.instanceSource",
                    FnKat::StringAttribute(instanceSources, 1));

    instancesBldr.setAttrAtLocation("instances",
            "geometry.instanceIndex",
                    FnKat::IntAttribute(&instanceIndices[0],
                            instanceIndices.size(), 1));

#if KATANA_VERSION_MAJOR >= 3
    // If motion is backwards, make sure to reverse time samples.
    std::map<float, VtArray<GfMatrix4d>> timeToSampleMap;
    for (size_t a = 0; a < numXformSamples; ++a) {
        double relSampleTime = motionSampleTimes[a];
        timeToSampleMap.insert(
            {data.IsMotionBackward()
                 ? PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime)
                 : relSampleTime,
             xformSamples[a]});
    }
    auto instanceMatrixAttr = VtKatanaMapOrCopy(timeToSampleMap);
    instancesBldr.setAttrAtLocation("instances", "geometry.instanceMatrix",
                                    instanceMatrixAttr);
#else
    FnKat::DoubleBuilder instanceMatrixBldr(16);
    for (size_t a = 0; a < numXformSamples; ++a) {

        double relSampleTime = motionSampleTimes[a];

        // Shove samples into the builder at the frame-relative sample time. If
        // motion is backwards, make sure to reverse time samples.
        std::vector<double> &matVec = instanceMatrixBldr.get(
            data.IsMotionBackward()
                ? PxrUsdKatanaUtils::ReverseTimeSample(relSampleTime)
                : relSampleTime);

        matVec.reserve(16 * numInstances);
        for (size_t i = 0; i < numInstances; ++i) {

            GfMatrix4d instanceXform = xformSamples[a][i];
            const double *matArray = instanceXform.GetArray();

            for (int j = 0; j < 16; ++j) {
                matVec.push_back(matArray[j]);
            }
        }
    }
    instancesBldr.setAttrAtLocation("instances",
            "geometry.instanceMatrix", instanceMatrixBldr.build());
#endif // KATANA_VERSION_MAJOR > 3

    if (!omitList.empty())
    {
        instancesBldr.setAttrAtLocation("instances",
                "geometry.omitList",
                        FnKat::IntAttribute(&omitList[0], omitList.size(), 1));
    }

    instancesBldr.setAttrAtLocation("instances",
            "geometry.pointInstancerId",
                    FnKat::StringAttribute(katOutputPath));

    //
    // Transfer primvars.
    //

    FnKat::GroupBuilder instancerPrimvarsBldr;
    FnKat::GroupBuilder instancesPrimvarsBldr;
    for (int64_t i = 0; i < primvarAttrs.getNumberOfChildren(); ++i)
    {
        const std::string primvarName = primvarAttrs.getChildName(i);

        // Use "point" scope for the instancer.
        instancerPrimvarsBldr.set(primvarName, primvarAttrs.getChildByIndex(i));
        instancerPrimvarsBldr.set(primvarName + ".scope",
                FnKat::StringAttribute("point"));

        // User "primitive" scope for the instances.
        instancesPrimvarsBldr.set(primvarName, primvarAttrs.getChildByIndex(i));
        instancesPrimvarsBldr.set(primvarName + ".scope",
                FnKat::StringAttribute("primitive"));
    }
    instancerAttrMap.set("geometry.arbitrary", instancerPrimvarsBldr.build());
    instancesBldr.setAttrAtLocation("instances",
            "geometry.arbitrary", instancesPrimvarsBldr.build());

    //
    // Set the final aggregate bounds.
    //

    if (aggregateBoundsValid)
    {
        instancerAttrMap.set("bound", FnKat::DoubleAttribute(&aggregateBounds[0], 6, 2));
    }

    //
    // Set proxy attrs.
    //

    instancerAttrMap.set("proxies", PxrUsdKatanaUtils::GetViewerProxyAttr(data));

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
