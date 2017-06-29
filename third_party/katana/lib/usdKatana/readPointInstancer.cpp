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

#include "pxr/base/gf/matrix4d.h"

#include <FnGeolibServices/FnBuiltInOpArgsUtil.h>
#include <FnGeolib/util/Path.h>
#include <FnLogging/FnLogging.h>

#include <boost/unordered_set.hpp>

#include <pystring/pystring.h>

PXR_NAMESPACE_OPEN_SCOPE


FnLogSetup("PxrUsdKatanaReadPointInstancer");

namespace
{
    typedef std::map<SdfPath, UsdPrim> _PathToPrimMap;
    typedef std::map<SdfPath, GfRange3d> _PathToRangeMap;
    typedef std::map<TfToken, GfRange3d, TfTokenFastArbitraryLessThan>
        _PurposeToRangeMap;

    // Log an error and set attrs to create a Katana error location.
    //
    void
    _LogAndSetError(
        PxrUsdKatanaAttrMap& attrs,
        const std::string message)
    {
        FnLogError(message);
        attrs.set("type", FnKat::StringAttribute("error"));
        attrs.set("errorMessage",
                  FnKat::StringAttribute(
                      "[ERROR PxrUsdKatanaReadPointInstancer]: " + message));
    }

    // XXX This is based on UsdGeomPointInstancer::ComputeExtentAtTime. Ideally,
    // we would just use UsdGeomPointInstancer, however it will compute the
    // bounds of its prototypes if it can't use their extentsHints, which is an
    // expensive operation we want to avoid here (see bug 147526).
    //
    bool
    _ComputeExtentAtTime(
        VtVec3fArray& extent,
        const UsdGeomPointInstancer& instancer,
        const UsdTimeCode baseTime,
        const VtIntArray& protoIndices,
        const SdfPathVector& protoPaths,
        const _PathToPrimMap& primCache,
        const std::vector<bool> mask,
        const FnKat::DoubleAttribute& instanceMatrixAttr)
    {
        const TfTokenVector purposes{UsdGeomTokens->default_,
                                     UsdGeomTokens->proxy,
                                     UsdGeomTokens->render};
        _PathToRangeMap rangeCache;
        GfRange3d extentRange;

        const size_t numSampleTimes =
            instanceMatrixAttr.getNumberOfTimeSamples();

        for (size_t i = 0; i < protoIndices.size(); ++i) {
            if (!mask.empty() && !mask[i]) {
                continue;
            }

            const int protoIndex = protoIndices[i];
            const SdfPath &protoPath = protoPaths[protoIndex];

            GfRange3d thisRange;

            // Check to see if this prototype's range was already computed. If
            // not, use its extents hint to compute one.
            _PathToRangeMap::const_iterator rcIt = rangeCache.find(protoPath);
            if (rcIt != rangeCache.end()) {
                thisRange = rcIt->second;
            } else {
                _PathToPrimMap::const_iterator pcIt = primCache.find(protoPath);
                const UsdPrim &protoPrim = pcIt->second;
                if (!protoPrim) {
                    continue;
                }

                VtVec3fArray extents;
                UsdAttribute extentsAttr =
                    protoPrim.GetAttribute(TfToken("extentsHint"));
                if (extentsAttr and extentsAttr.Get(&extents, baseTime)) {

                    // Get the ranges associated with each purpose.
                    _PurposeToRangeMap ranges;
                    for (size_t i = 0; i < purposes.size(); ++i) {
                        size_t idx = i * 2;
                        // If extents are not available for this purpose, it
                        // implies that the rest of the bounds are empty. Hence,
                        // we can break.
                        if ((idx + 2) > extents.size()) {
                            break;
                        }
                        ranges[purposes[i]] =
                            GfRange3d(extents[idx], extents[idx + 1]);
                    }

                    // Combine ranges for each purpose in order to generate the
                    // final range.
                    for (auto purpose : purposes) {
                        _PurposeToRangeMap::const_iterator rIt =
                            ranges.find(purpose);
                        if (rIt != ranges.end()) {
                            const GfRange3d &rangeForPurpose = rIt->second;
                            if (!rangeForPurpose.IsEmpty()) {
                                thisRange = GfRange3d::GetUnion(
                                    thisRange, rangeForPurpose);
                            }
                        }
                    }
                }

                // Store the result so we don't have to compute it again.
                rangeCache[protoPath] = thisRange;
            }

            // If a range could not be computed using extents, simply move on.
            if (thisRange.IsEmpty()) {
                continue;
            }

            for (size_t a = 0; a < numSampleTimes; ++a) {
                // Get the prototype bounding box with the instance transform
                // applied. We don't apply the parent transform here, as the
                // bounds need to be in parent-local space.
                float sampleTime = instanceMatrixAttr.getSampleTime(a);
                FnKat::DoubleAttribute::array_type instanceXforms =
                    instanceMatrixAttr.getNearestSample(sampleTime);
                GfMatrix4d instanceXform;
                double *matArray = instanceXform.GetArray();
                for (int j = 0; j < 16; j++)
                {
                    matArray[j] = instanceXforms[i * 16 + j];
                }
                GfBBox3d thisBounds(thisRange, instanceXform);
                extentRange.UnionWith(thisBounds.ComputeAlignedRange());
            }
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

    const std::string instancerPath = instancer.GetPath().GetString();

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
        _LogAndSetError(instancerAttrMap, "Instancer has no prototype indices");
        return;
    }
    const size_t numInstances = protoIndices.size();
    if (numInstances == 0)
    {
        _LogAndSetError(instancerAttrMap, "Instancer has no prototype indices");
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

    //
    // Compute instance transform matrices.
    //

    // XXX At some point, we'll use
    // UsdGeomPointInstancer::ComputeInstanceTransformsAtTime to compute the
    // instance transform matrix; until then, extract this information from the
    // input attrs.
    //
    FnKat::DoubleAttribute instanceMatrixAttr =
            inputAttrs.getChildByName("instanceMatrix");

    //
    // Compute prototype bounds.
    //

    bool aggregateBoundsValid = false;
    std::vector<double> aggregateBounds;

    // XXX Replace with UsdGeomPointInstancer::ComputeExtentAtTime.
    //
    VtVec3fArray aggregateExtent;
    if (_ComputeExtentAtTime(
            aggregateExtent, instancer, UsdTimeCode(currentTime), protoIndices,
            protoPaths, primCache, pruneMaskValues, instanceMatrixAttr)) {
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

            UsdRelationship materialBindingsRel =
                    UsdShadeMaterial::GetBindingRel(protoPrim);

            auto assetAPI = UsdModelAPI(protoPrim);
            std::string assetName;
            bool isReferencedModelPrim =
                    assetAPI.IsModel() and assetAPI.GetAssetName(&assetName);

            if (!materialBindingsRel or isReferencedModelPrim)
            {
                // The prim has no material bindings or is a referenced model
                // prim (meaning that materials are defined below it); start
                // building at the prototype path.
                //
                commonPrefixes.push_back(protoPath);
            }
            else
            {
                SdfPathVector materialPaths;
                materialBindingsRel.GetForwardedTargets(&materialPaths);
                for (auto materialPath : materialPaths)
                {
                    const SdfPath &commonPrefix =
                            protoPath.GetCommonPrefix(materialPath);
                    if (commonPrefix.GetString() == "/")
                    {
                        // XXX Unhandled case.
                        // The prototype prim and its material are not under the
                        // same parent; start building at the prototype path
                        // (although it is likely that bindings will be broken).
                        //
                        commonPrefixes.push_back(protoPath);
                    }
                    else
                    {
                        // Start building at the common ancestor between the
                        // prototype prim and its material.
                        //
                        commonPrefixes.push_back(commonPrefix);
                    }
                }
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

            if (protoPath.GetString() != buildPath)
            {
                // Finish generating the full path to the prototype.
                //
                fullProtoPath = fullProtoPath + "/geo" + pystring::replace(
                        protoPath.GetString(), buildPath, "");
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

    instancesBldr.setAttrAtLocation("instances",
            "geometry.instanceMatrix", instanceMatrixAttr);

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
