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
#ifndef USDLUX_GENERATED_LIGHTLISTAPI_H
#define USDLUX_GENERATED_LIGHTLISTAPI_H

/// \file usdLux/lightListAPI.h

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLux/tokens.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

// -------------------------------------------------------------------------- //
// LIGHTLISTAPI                                                               //
// -------------------------------------------------------------------------- //

/// \class UsdLuxLightListAPI
///
/// API schema to support discovery and publishing of lights in a scene.
/// 
/// \section UsdLuxLightListAPI_Discovery Discovering Lights via Traversal
/// 
/// To motivate this API, consider what is required to discover all
/// lights in a scene.  We must load all payloads and traverse all prims:
/// 
/// \code
/// 01  // Load everything on the stage so we can find all lights,
/// 02  // including those inside payloads
/// 03  stage->Load();
/// 04  
/// 05  // Traverse all prims, checking if they have an applied UsdLuxLightAPI
/// 06  // (Note: ignoring instancing and a few other things for simplicity)
/// 07  SdfPathVector lights;
/// 08  for (UsdPrim prim: stage->Traverse()) {
/// 09      if (prim.HasAPI<UsdLuxLightAPI>()) {
/// 10          lights.push_back(i->GetPath());
/// 11      }
/// 12  }
/// \endcode
/// 
/// This traversal -- suitably elaborated to handle certain details --
/// is the first and simplest thing UsdLuxLightListAPI provides.
/// UsdLuxLightListAPI::ComputeLightList() performs this traversal and returns
/// all lights in the scene:
/// 
/// \code
/// 01  UsdLuxLightListAPI listAPI(stage->GetPseudoRoot());
/// 02  SdfPathVector lights = listAPI.ComputeLightList();
/// \endcode
/// 
/// \section UsdLuxLightListAPI_LightList Publishing a Cached Light List
/// 
/// Consider a USD client that needs to quickly discover lights but
/// wants to defer loading payloads and traversing the entire scene
/// where possible, and is willing to do up-front computation and
/// caching to achieve that.
/// 
/// UsdLuxLightListAPI provides a way to cache the computed light list,
/// by publishing the list of lights onto prims in the model
/// hierarchy.  Consider a big set that contains lights:
/// 
/// \code
/// 01  def Xform "BigSetWithLights" (
/// 02      kind = "assembly"
/// 03      payload = @BigSetWithLights.usd@   // Heavy payload
/// 04  ) {
/// 05      // Pre-computed, cached list of lights inside payload
/// 06      rel lightList = [
/// 07          <./Lights/light_1>,
/// 08          <./Lights/light_2>,
/// 09          ...
/// 10      ]
/// 11      token lightList:cacheBehavior = "consumeAndContinue";
/// 12  }
/// \endcode
/// 
/// The lightList relationship encodes a set of lights, and the
/// lightList:cacheBehavior property provides fine-grained
/// control over how to use that cache.  (See details below.)
/// 
/// The cache can be created by first invoking
/// ComputeLightList(ComputeModeIgnoreCache) to pre-compute the list
/// and then storing the result with UsdLuxLightListAPI::StoreLightList().
/// 
/// To enable efficient retrieval of the cache, it should be stored
/// on a model hierarchy prim.  Furthermore, note that while you can
/// use a UsdLuxLightListAPI bound to the pseudo-root prim to query the
/// lights (as in the example above) because it will perform a
/// traversal over descendants, you cannot store the cache back to the
/// pseduo-root prim.
/// 
/// To consult the cached list, we invoke
/// ComputeLightList(ComputeModeConsultModelHierarchyCache):
/// 
/// \code
/// 01  // Find and load all lights, using lightList cache where available
/// 02  UsdLuxLightListAPI list(stage->GetPseudoRoot());
/// 03  SdfPathSet lights = list.ComputeLightList(
/// 04      UsdLuxLightListAPI::ComputeModeConsultModelHierarchyCache);
/// 05  stage.LoadAndUnload(lights, SdfPathSet());
/// \endcode
/// 
/// In this mode, ComputeLightList() will traverse the model
/// hierarchy, accumulating cached light lists.
/// 
/// \section UsdLuxLightListAPI_CacheBehavior Controlling Cache Behavior
/// 
/// The lightList:cacheBehavior property gives additional fine-grained
/// control over cache behavior:
/// 
/// \li The fallback value, "ignore", indicates that the lightList should
/// be disregarded.  This provides a way to invalidate cache entries.
/// Note that unless "ignore" is specified, a lightList with an empty
/// list of targets is considered a cache indicating that no lights
/// are present.
/// 
/// \li The value "consumeAndContinue" indicates that the cache should
/// be consulted to contribute lights to the scene, and that recursion
/// should continue down the model hierarchy in case additional lights
/// are added as descedants. This is the default value established when
/// StoreLightList() is invoked. This behavior allows the lights within
/// a large model, such as the BigSetWithLights example above, to be
/// published outside the payload, while also allowing referencing and
/// layering to add additional lights over that set.
/// 
/// \li The value "consumeAndHalt" provides a way to terminate recursive
/// traversal of the scene for light discovery. The cache will be
/// consulted but no descendant prims will be examined.
/// 
/// \section UsdLuxLightListAPI_Instancing Instancing
/// 
/// Where instances are present, UsdLuxLightListAPI::ComputeLightList() will
/// return the instance-unique paths to any lights discovered within
/// those instances.  Lights within a UsdGeomPointInstancer will
/// not be returned, however, since they cannot be referred to
/// solely via paths.
/// 
///
/// For any described attribute \em Fallback \em Value or \em Allowed \em Values below
/// that are text/tokens, the actual token is published and defined in \ref UsdLuxTokens.
/// So to set an attribute to the value "rightHanded", use UsdLuxTokens->rightHanded
/// as the value.
///
class UsdLuxLightListAPI : public UsdAPISchemaBase
{
public:
    /// Compile time constant representing what kind of schema this class is.
    ///
    /// \sa UsdSchemaKind
    static const UsdSchemaKind schemaKind = UsdSchemaKind::SingleApplyAPI;

    /// Construct a UsdLuxLightListAPI on UsdPrim \p prim .
    /// Equivalent to UsdLuxLightListAPI::Get(prim.GetStage(), prim.GetPath())
    /// for a \em valid \p prim, but will not immediately throw an error for
    /// an invalid \p prim
    explicit UsdLuxLightListAPI(const UsdPrim& prim=UsdPrim())
        : UsdAPISchemaBase(prim)
    {
    }

    /// Construct a UsdLuxLightListAPI on the prim held by \p schemaObj .
    /// Should be preferred over UsdLuxLightListAPI(schemaObj.GetPrim()),
    /// as it preserves SchemaBase state.
    explicit UsdLuxLightListAPI(const UsdSchemaBase& schemaObj)
        : UsdAPISchemaBase(schemaObj)
    {
    }

    /// Destructor.
    USDLUX_API
    virtual ~UsdLuxLightListAPI();

    /// Return a vector of names of all pre-declared attributes for this schema
    /// class and all its ancestor classes.  Does not include attributes that
    /// may be authored by custom/extended methods of the schemas involved.
    USDLUX_API
    static const TfTokenVector &
    GetSchemaAttributeNames(bool includeInherited=true);

    /// Return a UsdLuxLightListAPI holding the prim adhering to this
    /// schema at \p path on \p stage.  If no prim exists at \p path on
    /// \p stage, or if the prim at that path does not adhere to this schema,
    /// return an invalid schema object.  This is shorthand for the following:
    ///
    /// \code
    /// UsdLuxLightListAPI(stage->GetPrimAtPath(path));
    /// \endcode
    ///
    USDLUX_API
    static UsdLuxLightListAPI
    Get(const UsdStagePtr &stage, const SdfPath &path);


    /// Returns true if this <b>single-apply</b> API schema can be applied to 
    /// the given \p prim. If this schema can not be a applied to the prim, 
    /// this returns false and, if provided, populates \p whyNot with the 
    /// reason it can not be applied.
    /// 
    /// Note that if CanApply returns false, that does not necessarily imply
    /// that calling Apply will fail. Callers are expected to call CanApply
    /// before calling Apply if they want to ensure that it is valid to 
    /// apply a schema.
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLUX_API
    static bool 
    CanApply(const UsdPrim &prim, std::string *whyNot=nullptr);

    /// Applies this <b>single-apply</b> API schema to the given \p prim.
    /// This information is stored by adding "LightListAPI" to the 
    /// token-valued, listOp metadata \em apiSchemas on the prim.
    /// 
    /// \return A valid UsdLuxLightListAPI object is returned upon success. 
    /// An invalid (or empty) UsdLuxLightListAPI object is returned upon 
    /// failure. See \ref UsdPrim::ApplyAPI() for conditions 
    /// resulting in failure. 
    /// 
    /// \sa UsdPrim::GetAppliedSchemas()
    /// \sa UsdPrim::HasAPI()
    /// \sa UsdPrim::CanApplyAPI()
    /// \sa UsdPrim::ApplyAPI()
    /// \sa UsdPrim::RemoveAPI()
    ///
    USDLUX_API
    static UsdLuxLightListAPI 
    Apply(const UsdPrim &prim);

protected:
    /// Returns the kind of schema this class belongs to.
    ///
    /// \sa UsdSchemaKind
    USDLUX_API
    UsdSchemaKind _GetSchemaKind() const override;

private:
    // needs to invoke _GetStaticTfType.
    friend class UsdSchemaRegistry;
    USDLUX_API
    static const TfType &_GetStaticTfType();

    static bool _IsTypedSchema();

    // override SchemaBase virtuals.
    USDLUX_API
    const TfType &_GetTfType() const override;

public:
    // --------------------------------------------------------------------- //
    // LIGHTLISTCACHEBEHAVIOR 
    // --------------------------------------------------------------------- //
    /// Controls how the lightList should be interpreted.
    /// Valid values are:
    /// - consumeAndHalt: The lightList should be consulted,
    /// and if it exists, treated as a final authoritative statement
    /// of any lights that exist at or below this prim, halting
    /// recursive discovery of lights.
    /// - consumeAndContinue: The lightList should be consulted,
    /// but recursive traversal over nameChildren should continue
    /// in case additional lights are added by descendants.
    /// - ignore: The lightList should be entirely ignored.  This
    /// provides a simple way to temporarily invalidate an existing
    /// cache.  This is the fallback behavior.
    /// 
    ///
    /// | ||
    /// | -- | -- |
    /// | Declaration | `token lightList:cacheBehavior` |
    /// | C++ Type | TfToken |
    /// | \ref Usd_Datatypes "Usd Type" | SdfValueTypeNames->Token |
    /// | \ref UsdLuxTokens "Allowed Values" | consumeAndHalt, consumeAndContinue, ignore |
    USDLUX_API
    UsdAttribute GetLightListCacheBehaviorAttr() const;

    /// See GetLightListCacheBehaviorAttr(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create.
    /// If specified, author \p defaultValue as the attribute's default,
    /// sparsely (when it makes sense to do so) if \p writeSparsely is \c true -
    /// the default for \p writeSparsely is \c false.
    USDLUX_API
    UsdAttribute CreateLightListCacheBehaviorAttr(VtValue const &defaultValue = VtValue(), bool writeSparsely=false) const;

public:
    // --------------------------------------------------------------------- //
    // LIGHTLIST 
    // --------------------------------------------------------------------- //
    /// Relationship to lights in the scene.
    ///
    USDLUX_API
    UsdRelationship GetLightListRel() const;

    /// See GetLightListRel(), and also 
    /// \ref Usd_Create_Or_Get_Property for when to use Get vs Create
    USDLUX_API
    UsdRelationship CreateLightListRel() const;

public:
    // ===================================================================== //
    // Feel free to add custom code below this line, it will be preserved by 
    // the code generator. 
    //
    // Just remember to: 
    //  - Close the class declaration with }; 
    //  - Close the namespace with PXR_NAMESPACE_CLOSE_SCOPE
    //  - Close the include guard with #endif
    // ===================================================================== //
    // --(BEGIN CUSTOM CODE)--

    /// Runtime control over whether to consult stored lightList caches.
    enum ComputeMode {
        /// Consult any caches found on the model hierarchy.
        /// Do not traverse beneath the model hierarchy.
        ComputeModeConsultModelHierarchyCache,
        /// Ignore any caches found, and do a full prim traversal.
        ComputeModeIgnoreCache,
    };

    /// Computes and returns the list of lights and light filters in
    /// the stage, optionally consulting a cached result.
    ///
    /// In ComputeModeIgnoreCache mode, caching is ignored, and this
    /// does a prim traversal looking for prims that have a UsdLuxLightAPI
    /// or are of type UsdLuxLightFilter.
    ///
    /// In ComputeModeConsultModelHierarchyCache, this does a traversal
    /// only of the model hierarchy. In this traversal, any lights that
    /// live as model hierarchy prims are accumulated, as well as any
    /// paths stored in lightList caches. The lightList:cacheBehavior
    /// attribute gives further control over the cache behavior; see the
    /// class overview for details.
    /// 
    /// When instances are present, ComputeLightList(ComputeModeIgnoreCache)
    /// will return the instance-uniqiue paths to any lights discovered
    /// within those instances.  Lights within a UsdGeomPointInstancer
    /// will not be returned, however, since they cannot be referred to
    /// solely via paths.
    USDLUX_API
    SdfPathSet ComputeLightList(ComputeMode mode) const;

    /// Store the given paths as the lightlist for this prim.
    /// Paths that do not have this prim's path as a prefix
    /// will be silently ignored.
    /// This will set the listList:cacheBehavior to "consumeAndContinue".
    USDLUX_API
    void StoreLightList(const SdfPathSet &) const;

    /// Mark any stored lightlist as invalid, by setting the
    /// lightList:cacheBehavior attribute to ignore.
    USDLUX_API
    void InvalidateLightList() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
