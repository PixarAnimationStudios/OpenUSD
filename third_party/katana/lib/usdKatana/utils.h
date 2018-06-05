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
#include "usdKatana/api.h"
#include "usdKatana/attrMap.h"
#include "usdKatana/usdInPrivateData.h"

#include "pxr/usd/usd/relationship.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/vt/value.h"

#include <FnAttribute/FnGroupBuilder.h>
#include <FnAttribute/FnDataBuilder.h>
#include <FnGeolib/op/FnGeolibOp.h>

#include <type_traits>
#include <vector>

namespace FnKat = Foundry::Katana;

PXR_NAMESPACE_OPEN_SCOPE

class UsdCollectionAPI;

struct PxrUsdKatanaUtils {

    /// Reverse a motion time sample. This is used for building
    /// multi-sampled attributes when motion blur is backward.
    USDKATANA_API
    static double ReverseTimeSample(double sample);

    /// Convert Pixar-style numVerts to Katana-style startVerts.
    USDKATANA_API
    static void ConvertNumVertsToStartVerts( const std::vector<int> &numVertsVec,
                                  std::vector<int> *startVertsVec );

    USDKATANA_API
    static void ConvertArrayToVector(const VtVec3fArray &a, std::vector<float> *r);

    /// Convert a VtValue to a Katana attribute.
    /// If asShaderParam is true, it will use the special encoding
    /// Katana uses for shading arguments.
    /// The pathsAsModel argument is used when trying to resolve asset paths.
    USDKATANA_API
    static FnKat::Attribute ConvertVtValueToKatAttr( const VtValue & val,
                                                     bool asShaderParam,
                                                     bool pathsAsModel = false,
                                                     bool resolvePaths = true);

    /// Extract the targets of a relationship to a Katana attribute.
    /// If asShaderParam is true, it will use the special encoding
    /// Katana uses for shading arguments.
    USDKATANA_API
    static FnKat::Attribute ConvertRelTargetsToKatAttr(
            const UsdRelationship &rel, 
            bool asShaderParam);

    /// Convert a VtValue to a Katana custom geometry attribute (primvar).
    /// Katana uses a different encoding here from other attributes, which
    /// requires the inputType and elementSize attributes.
    USDKATANA_API
    static void ConvertVtValueToKatCustomGeomAttr( const VtValue & val,
                                        int elementSize,
                                        const TfToken &roleName,
                                        FnKat::Attribute *valueAttr,
                                        FnKat::Attribute *inputTypeAttr,
                                        FnKat::Attribute *elementSizeAttr );

    /// Returns whether the given attribute is varying over time.
    USDKATANA_API
    static bool IsAttributeVarying(const UsdAttribute &attr, double currentTime);

    /// \brief Get the handle for the given shadingNode.
    ///
    /// If \p shadingNode is not a valid prim, this returns "".  Otherwise, this
    /// will walk up name space and prepend the name of any Scope prims above
    /// \p shadingNode until it encounters a prim that is not a Scope.  This is
    /// required to get material referencing in katana (since in katana, all nodes
    /// are in a flat namespace, whereas Usd does not make any such requirement).
    USDKATANA_API
    static std::string GenerateShadingNodeHandle(
        const UsdPrim& shadingNode);

    // Scan the model hierarchy for models with kind=camera.
    USDKATANA_API
    static SdfPathVector FindCameraPaths( const UsdStageRefPtr& stage );

    /// Discover published lights (without a full scene traversal).
    USDKATANA_API
    static SdfPathVector FindLightPaths( const UsdStageRefPtr& stage );

    /// Convert the given SdfPath in the UsdStage to the corresponding
    /// katana location, given a scenegraph generator configuration.
    USDKATANA_API
    static std::string ConvertUsdPathToKatLocation(
            const SdfPath &path,
            const std::string &isolatePathString,
            const std::string &rootPathString,
            const std::string &sessionPathString = "",
            bool allowOutsideIsolation = false);
    USDKATANA_API
    static std::string ConvertUsdPathToKatLocation(
            const SdfPath &path,
            const PxrUsdKatanaUsdInPrivateData& data,
            bool allowOutsideIsolation = false);
    USDKATANA_API
    static std::string ConvertUsdPathToKatLocation(
            const SdfPath &path,
            const PxrUsdKatanaUsdInArgsRefPtr &usdInArgs,
            bool allowOutsideIsolation = false);

    /// USD Looks can have Katana child-parent relationships, which means that
    /// we'll have to do some extra processing to find the correct path that
    /// these resolve to
    USDKATANA_API
    static std::string _GetDisplayGroup(
            const UsdPrim &prim,
            const SdfPath& path);
    static std::string _GetDisplayName(const UsdPrim &prim);
    USDKATANA_API
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
    USDKATANA_API
    static bool ModelGroupIsAssembly(const UsdPrim &prim);

    // this finds prims with kind=subcomponent, increasingly used in complex
    // Sets models.
    USDKATANA_API
    static bool PrimIsSubcomponent(const UsdPrim &prim);

    /// Indicates if a given group should have a viewer proxy based on heuristics
    /// having to do with number of children and how many are components (non-group
    /// models).
    USDKATANA_API
    static bool ModelGroupNeedsProxy(const UsdPrim &prim);

    /// Creates the 'proxies' group attribute for consumption by the viewer.
    USDKATANA_API
    static FnKat::GroupAttribute GetViewerProxyAttr(
            const PxrUsdKatanaUsdInPrivateData& data);

    /// Returns the asset name for the given prim.  It should be a model.  This
    /// will fallback to the name of the prim.
    USDKATANA_API
    static std::string GetAssetName(const UsdPrim& prim);

    /// Returns the model instance name of the given prim, based on its
    /// RiAttribute-encoding, and falling back to its prim name.
    USDKATANA_API
    static std::string GetModelInstanceName(const UsdPrim& prim);

    /// Returns true if the prim is a Model and is an Assembly or Component.
    /// Currently, we're only using this for determining when to log an error
    /// when accessing model data.
    USDKATANA_API
    static bool IsModelAssemblyOrComponent(const UsdPrim& prim);

    /// \}

    /// \name Bounds
    /// \{
    USDKATANA_API
    static bool IsBoundable(const UsdPrim& prim);

    USDKATANA_API
    static FnKat::DoubleAttribute ConvertBoundsToAttribute(
            const std::vector<GfBBox3d>& bounds,
            const std::vector<double>& motionSampleTimes,
            bool isMotionBackward,
            bool* hasInfiniteBounds);
    /// \}
    
    /// Build and return, as a group attribute for convenience, a map
    /// from instances to masters.  Only traverses paths at and below
    /// the given rootPath.
    USDKATANA_API
    static FnKat::GroupAttribute BuildInstanceMasterMapping(
            const UsdStageRefPtr& stage, const SdfPath &rootPath);
    
};

/// Utility class for building a light list.
class PxrUsdKatanaUtilsLightListAccess {
public:
    /// Get the Usd prim at the current light path.
    USDKATANA_API
    UsdPrim GetPrim() const;

    /// Get the Katana location for the current light path.
    USDKATANA_API
    std::string GetLocation() const;

    /// Get the Katana location for a given Usd path.
    USDKATANA_API
    std::string GetLocation(const SdfPath& path) const;

    /// Add an attribute to lightList.
    template <class T>
    void Set(const std::string& name, const T& value)
    {
        static_assert(!std::is_base_of<FnKat::Attribute, T>::value,
                      "Directly setting Katana Attributes is not supported");
        _Set(name, VtValue(value));
    }

    /// Set linking for the light.
    USDKATANA_API
    bool SetLinks(const UsdCollectionAPI &collectionAPI,
                  const std::string &linkName);

    /// Append the string \p value to a custom string list named \p tag.
    /// These are built to the interface as attributes named \p tag.
    USDKATANA_API
    void AddToCustomStringList(const std::string& tag,const std::string& value);

protected:
    USDKATANA_API
    PxrUsdKatanaUtilsLightListAccess(
        FnKat::GeolibCookInterface &interface,
        const PxrUsdKatanaUsdInArgsRefPtr& usdInArgs);
    USDKATANA_API
    ~PxrUsdKatanaUtilsLightListAccess();

    /// Change the light path being accessed.
    USDKATANA_API
    void SetPath(const SdfPath& lightPath);

    /// Build into \p interface.
    USDKATANA_API
    void Build();

private:
    USDKATANA_API
    void _Set(const std::string& name, const VtValue& value);
    USDKATANA_API
    void _Set(const std::string& name, const FnKat::Attribute& attr);

private:
    FnKat::GeolibCookInterface& _interface;
    PxrUsdKatanaUsdInArgsRefPtr _usdInArgs;
    FnKat::GroupBuilder _lightListBuilder;
    std::map<std::string, FnKat::StringBuilder> _customStringLists;
    SdfPath _lightPath;
    std::string _key;
};

/// Utility class for building a light list.
class PxrUsdKatanaUtilsLightListEditor :
    public PxrUsdKatanaUtilsLightListAccess {
public:
    PxrUsdKatanaUtilsLightListEditor(
        FnKat::GeolibCookInterface &interface,
        const PxrUsdKatanaUsdInArgsRefPtr& usdInArgs) :
        PxrUsdKatanaUtilsLightListAccess(interface, usdInArgs) { }

    // Allow access to protected members.  PxrUsdKatanaUtilsLightListAccess
    // is handed out to calls that need limited access and this class is
    // used for full access.
    using PxrUsdKatanaUtilsLightListAccess::SetPath;
    using PxrUsdKatanaUtilsLightListAccess::Build;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SGG_USD_UTILS_H

