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
#ifndef PXRUSDKATANA_ATTRUTILS_H
#define PXRUSDKATANA_ATTRUTILS_H

#include "pxr/pxr.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/usdInPrivateData.h"

#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/vt/value.h"

#include <FnAttribute/FnGroupBuilder.h>
#include <FnAttribute/FnDataBuilder.h>

#include <vector>

namespace FnKat = Foundry::Katana;

PXR_NAMESPACE_OPEN_SCOPE

struct PxrUsdKatanaUtils {

    /// Reverse a motion time sample. This is used for building
    /// multi-sampled attributes when motion blur is backward.
    static double ReverseTimeSample(double sample);

    /// Convert Pixar-style numVerts to Katana-style startVerts.
    static void ConvertNumVertsToStartVerts( const std::vector<int> &numVertsVec,
                                  std::vector<int> *startVertsVec );

    static void ConvertArrayToVector(const VtVec3fArray &a, std::vector<float> *r);

    /// Convert a VtValue to a Katana attribute.
    /// If asShaderParam is true, it will use the special encoding
    /// Katana uses for shading arguments.
    /// The pathsAsModel argument is used when trying to resolve asset paths.
    static FnKat::Attribute ConvertVtValueToKatAttr( const VtValue & val,
                                                     bool asShaderParam,
                                                     bool pathsAsModel = false );

    /// Extract the targets of a relationship to a Katana attribute.
    /// If asShaderParam is true, it will use the special encoding
    /// Katana uses for shading arguments.
    static FnKat::Attribute ConvertRelTargetsToKatAttr(
            const UsdRelationship &rel, 
            bool asShaderParam);

    /// Convert a VtValue to a Katana custom geometry attribute (primvar).
    /// Katana uses a different encoding here from other attributes, which
    /// requires the inputType and elementSize attributes.
    static void ConvertVtValueToKatCustomGeomAttr( const VtValue & val,
                                        int elementSize,
                                        const TfToken &roleName,
                                        FnKat::Attribute *valueAttr,
                                        FnKat::Attribute *inputTypeAttr,
                                        FnKat::Attribute *elementSizeAttr );

    /// Returns whether the given attribute is varying over time.
    static bool IsAttributeVarying(const UsdAttribute &attr, double currentTime);

    /// \brief Get the handle for the given shadingNode.
    ///
    /// If \p shadingNode is not a valid prim, this returns "".  Otherwise, this
    /// will walk up name space and prepend the name of any Scope prims above
    /// \p shadingNode until it encounters a prim that is not a Scope.  This is
    /// required to get material referencing in katana (since in katana, all nodes
    /// are in a flat namespace, whereas Usd does not make any such requirement).
    static std::string GenerateShadingNodeHandle(
        const UsdPrim& shadingNode);

    // Scan the model hierarchy for models with kind=camera.
    static SdfPathVector FindCameraPaths( const UsdStageRefPtr& stage );

    /// Convert the given SdfPath in the UsdStage to the corresponding
    /// katana location, given a scenegraph generator configuration.
    static std::string ConvertUsdPathToKatLocation(
            const SdfPath &path,
            const PxrUsdKatanaUsdInPrivateData& data);

    /// USD Looks can have Katana child-parent relationships, which means that
    /// we'll have to do some extra processing to find the correct path that
    /// these resolve to
    static std::string ConvertUsdMaterialPathToKatLocation(
            const SdfPath &path,
            const PxrUsdKatanaUsdInPrivateData& data);

    // XXX: should move these into readModel.h maybe.
    /// \name Model Utilities
    /// \{
    ///
    /// Usd identifies everything above leaf/component models as "model groups"
    /// However, katana has a meaningful (behaviorially) distinction between
    /// assemblies and groups.  This fn encapsulates the heuristics for when
    /// we translate a Usd modelGroup into an assembly, and when we don't
    static bool ModelGroupIsAssembly(const UsdPrim &prim);

    // this finds prims with kind=subcomponent, increasingly used in complex
    // Sets models.
    static bool PrimIsSubcomponent(const UsdPrim &prim);

    /// Indicates if a given group should have a viewer proxy based on heuristics
    /// having to do with number of children and how many are components (non-group
    /// models).
    static bool ModelGroupNeedsProxy(const UsdPrim &prim);

    /// Returns the asset name for the given prim.  It should be a model.  This
    /// will fallback to the name of the prim.
    static std::string GetAssetName(const UsdPrim& prim);

    /// Returns the model instance name of the given prim, based on its
    /// RiAttribute-encoding, and falling back to its prim name.
    static std::string GetModelInstanceName(const UsdPrim& prim);

    /// Returns true if the prim is a Model and is an Assembly or Component.
    /// Currently, we're only using this for determining when to log an error
    /// when accessing model data.
    static bool IsModelAssemblyOrComponent(const UsdPrim& prim);

    /// \}

    /// \name Bounds
    /// \{
    static bool IsBoundable(const UsdPrim& prim);

    static FnKat::DoubleAttribute ConvertBoundsToAttribute(
            const std::vector<GfBBox3d>& bounds,
            const std::vector<double>& motionSampleTimes,
            bool isMotionBackward,
            bool* hasInfiniteBounds);
    /// \}
    
    /// Build and return, as a group attribute for convenience, a map
    /// from instances to masters.  Only traverses paths at and below
    /// the given rootPath.
    static FnKat::GroupAttribute BuildInstanceMasterMapping(
            const UsdStageRefPtr& stage, const SdfPath &rootPath);
    
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // SGG_USD_UTILS_H

